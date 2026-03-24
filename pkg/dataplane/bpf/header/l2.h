#ifndef __L2_H__
#define __L2_H__

// vmlinux first
#include "vmlinux.h"

// bpf second
#include "bpf_endian.h"
#include "bpf_helpers.h"

// others
#include "forward_config.h"
#include "linux_header.h"
#include "protocol.h"

static __always_inline void fdb_learning(struct xdp_md *ctx,
                                         struct ethhdr *ether_header,
                                         struct vlan_hdr *vlan_header,
                                         __u32 vlan_id) {
  struct fdb_key key = {};
  __builtin_memcpy(key.mac, ether_header->h_source, 6);
  key.vlan_id = (__u16)vlan_id;

  struct fdb_value_item *existing = bpf_map_lookup_elem(&fdb_table, &key);
  if (!existing) {
    struct fdb_value_item new_val = {};
    new_val.value.ifindex = ctx->ingress_ifindex;
    new_val.value.last_seen = bpf_ktime_get_ns();
    int ret = bpf_map_update_elem(&fdb_table, &key, &new_val, BPF_NOEXIST);
    if (ret == 0) {
      bpf_printk("Debug: new fdb item\n");
    } else if (ret == -EEXIST) {
      bpf_printk("Debug: new fdb item, insert others CPU\n");
    } else if (ret == -E2BIG) {
      bpf_printk("Debug: new fdb item, map is full\n");
    }
    return;
  }

  __u32 in_ifindex = ctx->ingress_ifindex;
  __u64 now = bpf_ktime_get_ns();

  bpf_spin_lock(&existing->lock);
  if (existing->value.ifindex != in_ifindex ||
      (now - existing->value.last_seen) > SECOND) {
    existing->value.ifindex = in_ifindex;
    existing->value.last_seen = now;
    bpf_spin_unlock(&existing->lock);
    return;
  }
  bpf_spin_unlock(&existing->lock);

  bpf_printk("Debug: fdb learning skip");
  return;
}

static __always_inline void fdb_lookup(struct xdp_md *ctx,
                                       struct ethhdr *ether_header,
                                       struct vlan_hdr *vlan_header,
                                       __u32 vlan_id) {
  __u32 zero = 0;
  struct forward_config *cache =
      bpf_map_lookup_elem(&xdp_forward_config_cache, &zero);
  if (cache == NULL) {
    return;
  }

  struct fdb_key key = {};
  __builtin_memcpy(key.mac, ether_header->h_source, 6);
  key.vlan_id = (__u16)vlan_id;

  struct fdb_value_item *val;

  val = bpf_map_lookup_elem(&fdb_table, &key);
  if (!val) {
    return;
  }
  if (val->value.ifindex == 0) {
    return;
  }

  bpf_spin_lock(&val->lock);
  __builtin_memcpy(&cache->fdb_value, &val->value, sizeof(struct fdb_value));
  bpf_spin_unlock(&val->lock);

  return;
}

static __always_inline int l2_recv_vlan(struct xdp_md *ctx,
                                        struct iface_config *in_if_config,
                                        struct ethhdr *ether_header,
                                        struct vlan_hdr *vlan_header,
                                        __u32 *vlan_id) {
  if (in_if_config->l2.vlan_mode == VLAN_ACCESS) {
    // access
    if (vlan_header == NULL) {
      if (0 == in_if_config->l2.vlan_id) {
        return XDP_DROP;
      }
      // access 可以接受普通报文
      *vlan_id = in_if_config->l2.vlan_id;
      return 0;
    } else {
      // access 需要拒绝和自己vlan_id不一致的vlan报文
      __u32 vlan_tci = bpf_ntohs(vlan_header->h_vlan_TCI) & 0x0FFF;
      if (vlan_tci != in_if_config->l2.vlan_id) {
        return XDP_DROP;
      }

      *vlan_id = vlan_tci;
      return 0;
    }
  } else {
    // trunk
    if (vlan_header == NULL) {
      // trunk 可以接受普通报文，但是需要配置
      // native_id，使用native_id作为其报文的vlan_tag，在后续的fdb查找逻辑进行处理
      if (in_if_config->l2.vlan_id == 0) {
        return XDP_DROP;
      }
      *vlan_id = in_if_config->l2.vlan_id;
      return 0;
    } else {
      __u32 vlan_tci = bpf_ntohs(vlan_header->h_vlan_TCI) & 0x0FFF;
      if (vlan_tci == in_if_config->l2.vlan_id) {
        // trunk 可以接受一个vlanid等于nativeid的报文，哪怕这个nativeid不在trunk
        // range内
        *vlan_id = vlan_tci;
        return 0;
      }

      // 不在 trunk range, drop
      if (vlan_tci < in_if_config->l2.vlan_range_start ||
          vlan_tci > in_if_config->l2.vlan_range_end) {
        return -1;
      }

      // 在 trunk range, pass
      *vlan_id = vlan_tci;
      return 0;
    }
  }
}

static __always_inline int l2_out(struct xdp_md *ctx,
                                  struct ethhdr *ether_header,
                                  struct vlan_hdr *vlan_header, __u32 vlan_id) {
  __u32 key = 0;
  struct forward_config *cache =
      bpf_map_lookup_elem(&xdp_forward_config_cache, &key);
  if (cache == NULL || cache->out_if_config.ifindex == 0) {
    return XDP_DROP;
  }

  struct iface_config *out_config = &cache->out_if_config;
  if (out_config->mode != IF_MODE_L2) {
    return XDP_DROP;
  }

  // 注意：一旦调用 bpf_xdp_adjust_head，之前的 ether_header 和 vlan_header
  // 指针将全部失效 我们需要先把以太网头部的 MAC 地址暂存在栈上
  struct ethhdr eth_tmp;
  __builtin_memcpy(&eth_tmp, ether_header, sizeof(struct ethhdr));

  if (vlan_header != NULL) {
    struct vlan_hdr vlan_tmp;
    __builtin_memcpy(&vlan_tmp, vlan_header, sizeof(struct vlan_hdr));

    // --- 原报文带 VLAN 标签 ---
    if (out_config->l2.vlan_mode == VLAN_ACCESS) {
      // 1. Decap VLAN: 移除 4 字节的 VLAN Tag
      if (bpf_xdp_adjust_head(ctx, (int)sizeof(struct vlan_hdr)))
        return XDP_DROP;

      // 重新获取指针并恢复 MAC 地址（此时协议类型需要修正为原始协议）
      void *data = (void *)(long)ctx->data;
      void *data_end = (void *)(long)ctx->data_end;
      if (data + sizeof(struct ethhdr) > data_end) {
        return XDP_DROP;
      }

      struct ethhdr *new_eth = data;
      __builtin_memcpy(new_eth->h_dest, eth_tmp.h_dest, 6);
      __builtin_memcpy(new_eth->h_source, eth_tmp.h_source, 6);
      new_eth->h_proto = vlan_tmp.h_vlan_encapsulated_proto; // 还原内层协议类型
    } else {
      // 2. Trunk 模式：保持现状，直接转发
    }
  } else {
    // --- 原报文不带 VLAN 标签 ---
    if (out_config->l2.vlan_mode == VLAN_ACCESS) {
      // 3. 直接转发：已经在同一个逻辑 VLAN 内，无需改动
    } else {
      // 4. Encap VLAN: 插入 4 字节 VLAN Tag
      // 将 head 向前移动 4 字节
      if (bpf_xdp_adjust_head(ctx, -(int)sizeof(struct vlan_hdr)))
        return XDP_DROP;

      void *data = (void *)(long)ctx->data;
      void *data_end = (void *)(long)ctx->data_end;
      // 必须重新做边界检查
      if (data + sizeof(struct ethhdr) + sizeof(struct vlan_hdr) > data_end) {
        return XDP_DROP;
      }

      struct ethhdr *new_eth = data;
      struct vlan_hdr *new_vlan = (void *)(new_eth + 1);

      // 构造新的头部
      __builtin_memcpy(new_eth->h_dest, eth_tmp.h_dest, 6);
      __builtin_memcpy(new_eth->h_source, eth_tmp.h_source, 6);
      new_eth->h_proto = bpf_htons(ETH_P_8021Q);

      new_vlan->h_vlan_TCI =
          bpf_htons(vlan_id & 0x0FFF); // 填入 FDB 查到的 VLAN ID
      new_vlan->h_vlan_encapsulated_proto = eth_tmp.h_proto; // 记录原始协议
    }
  }

  // 最后统一执行重定向
  return bpf_redirect(out_config->ifindex, 0);
}

static __always_inline int fdb_flood_tag(struct xdp_md *ctx,
                                         struct ethhdr *ether_header,
                                         struct vlan_hdr *vlan_header,
                                         __u32 vlan_id) {
  int ret = bpf_xdp_adjust_meta(ctx, -(int)sizeof(struct dp_metadata));
  if (ret < 0) {
    return XDP_DROP;
  }

  void *data = (void *)(long)ctx->data;
  void *data_meta = (void *)(long)ctx->data_meta;
  void *data_end = (void *)(long)ctx->data_end;
  if (data_meta + sizeof(struct dp_metadata) > data) {
    return XDP_DROP;
  }

  struct dp_metadata *md = data_meta;
  __builtin_memset(md, 0, sizeof(struct l2_metadata));
  md->magic = DP_META_MAGIC;
  md->meta_type = DP_META_L2;
  if (vlan_header != NULL) {
    md->l2.flags |= L2_VLAN_PACKET;
  }
  md->l2.flags |= L2_FLOOD;
  md->l2.vlan_id = vlan_id;
  return XDP_PASS;
}

static __always_inline int fdb_flood(struct __sk_buff *skb,
                                     struct dp_metadata *md) {
  // todo
  return TC_ACT_SHOT;
}

#endif
