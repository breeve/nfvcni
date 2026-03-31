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
      // pass normal pkt.
      *vlan_id = in_if_config->l2.vlan_id;
      return XDP_PASS;
    } else {
      // drop vlan pkt.
      return XDP_DROP;
    }
  } else {
    // trunk
    if (vlan_header == NULL) {
      if (in_if_config->l2.vlan_id == 0) {
        // drop normal pkt when native_id is not configured.
        return XDP_DROP;
      }
      // pass normal pkt, and set native_id as vlan_id.
      *vlan_id = in_if_config->l2.vlan_id;
      return XDP_PASS;
    } else {
      __u32 vlan_tci = bpf_ntohs(vlan_header->h_vlan_TCI) & 0x0FFF;
      if (vlan_tci == in_if_config->l2.vlan_id) {
        // pass vlan pkt.
        *vlan_id = in_if_config->l2.vlan_id;
        return XDP_PASS;
      }

      if (vlan_tci < in_if_config->l2.vlan_range_start ||
          vlan_tci > in_if_config->l2.vlan_range_end) {
        // drop vlan pkt, it is out of range.
        return XDP_DROP;
      }

      // pass vlan pkt, and set vlan_tci as vlan_id.
      *vlan_id = vlan_tci;
      return XDP_PASS;
    }
  }
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

struct flood_context {
  struct __sk_buff *skb;
  struct dp_metadata md;
};

struct xdp_l2_send_context {
#define vlan_header_orignal_SET 0x01
  __u32 flages;
  struct xdp_md *ctx;
  struct ethhdr ether_header_orignal;
  struct vlan_hdr vlan_header_orignal;
  struct iface_config out_config;
  __u32 vlan_id;
};

static __always_inline int xdp_l2_decap_vlan_and_send(void *_ctx) {
  struct xdp_l2_send_context *send_ctx = (struct xdp_l2_send_context *)_ctx;
  struct xdp_md *ctx = send_ctx->ctx;
  struct ethhdr *ether_header_original = &send_ctx->ether_header_orignal;
  struct vlan_hdr *vlan_header_orignal = &send_ctx->vlan_header_orignal;
  struct iface_config *out_config = &send_ctx->out_config;

  if (bpf_xdp_adjust_head(ctx, (int)sizeof(struct vlan_hdr))) {
    return -1;
  }
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  if (data + sizeof(struct ethhdr) > data_end) {
    return -1;
  }

  struct ethhdr *new_eth = data;
  __builtin_memcpy(new_eth->h_dest, ether_header_original->h_dest, 6);
  __builtin_memcpy(new_eth->h_source, ether_header_original->h_source, 6);
  new_eth->h_proto = vlan_header_orignal->h_vlan_encapsulated_proto;
  if (bpf_redirect(out_config->ifindex, 0) != XDP_REDIRECT) {
    return -1;
  }
  return 0;
}

static int xdp_l2_send_directly(void *_ctx) {
  struct xdp_l2_send_context *send_ctx = (struct xdp_l2_send_context *)_ctx;
  struct iface_config *out_config = &send_ctx->out_config;
  if (bpf_redirect(out_config->ifindex, 0) != XDP_REDIRECT) {
    return -1;
  }
  return 0;
}

static int xdp_l2_encap_vlan_and_send(void *_ctx) {
  struct xdp_l2_send_context *send_ctx = (struct xdp_l2_send_context *)_ctx;
  struct xdp_md *ctx = send_ctx->ctx;
  struct ethhdr *ether_header_original = &send_ctx->ether_header_orignal;
  struct iface_config *out_config = &send_ctx->out_config;
  __u32 vlan_id = send_ctx->vlan_id;

  if (bpf_xdp_adjust_head(ctx, -(int)sizeof(struct vlan_hdr))) {
    return -1;
  }
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  if (data + sizeof(struct ethhdr) + sizeof(struct vlan_hdr) > data_end) {
    return -1;
  }

  struct ethhdr *new_eth = data;
  struct vlan_hdr *new_vlan = (void *)(new_eth + 1);
  __builtin_memcpy(new_eth->h_dest, ether_header_original->h_dest, 6);
  __builtin_memcpy(new_eth->h_source, ether_header_original->h_source, 6);
  new_eth->h_proto = bpf_htons(ETH_P_8021Q);
  new_vlan->h_vlan_TCI = bpf_htons(vlan_id & 0x0FFF);
  new_vlan->h_vlan_encapsulated_proto = ether_header_original->h_proto;

  if (bpf_redirect(out_config->ifindex, 0) != XDP_REDIRECT) {
    return -1;
  }
  return 0;
}

struct tc_l2_send_context {
  struct __sk_buff *skb;
  __u32 out_if_index;
  __u32 vlan_id;
};

static int tc_l2_decap_vlan_and_send(void *_ctx) {
  struct tc_l2_send_context *send_ctx = (struct tc_l2_send_context *)_ctx;
  struct __sk_buff *skb = send_ctx->skb;
  __u32 out_if_index = send_ctx->out_if_index;
  __u32 vlan_id = send_ctx->vlan_id;

  long ret = bpf_skb_vlan_pop(skb);
  if (ret != 0) {
    return -1;
  }
  ret = bpf_clone_redirect(skb, out_if_index, 0);
  if (ret != 0) {
    return -1;
  }
  ret = bpf_skb_vlan_push(skb, bpf_htons(ETH_P_8021Q), vlan_id);
  if (ret != 0) {
    return -1;
  }
  return 0;
}

static int tc_l2_send_directly(void *_ctx) {
  struct tc_l2_send_context *send_ctx = (struct tc_l2_send_context *)_ctx;
  struct __sk_buff *skb = send_ctx->skb;
  __u32 out_if_index = send_ctx->out_if_index;
  if (bpf_clone_redirect(skb, out_if_index, 0) != 0) {
    return -1;
  }
  return 0;
}

static int tc_l2_encap_vlan_and_send(void *_ctx) {
  struct tc_l2_send_context *send_ctx = (struct tc_l2_send_context *)_ctx;
  struct __sk_buff *skb = send_ctx->skb;
  __u32 out_if_index = send_ctx->out_if_index;
  __u32 vlan_tci = send_ctx->vlan_id;

  long ret = bpf_skb_vlan_push(skb, bpf_htons(ETH_P_8021Q), vlan_tci);
  if (ret != 0) {
    return -1;
  }

  ret = bpf_clone_redirect(skb, out_if_index, 0);
  if (ret != 0) {
    return -1;
  }

  ret = bpf_skb_vlan_pop(skb);
  if (ret != 0) {
    return -1;
  }

  return 0;
}

static __always_inline int l2_out(struct iface_config *out_if_config,
                                  struct iface_config *in_if_config,
                                  __u32 vlan_id, bool is_vlan_pkt,
                                  int (*l2_decap_vlan_and_send)(void *send_ctx),
                                  int (*l2_encap_vlan_and_send)(void *send_ctx),
                                  int (*l2_send_directly)(void *send_ctx),
                                  void *send_ctx) {
  if (out_if_config->mode != IF_MODE_L2) {
    bpf_printk("Error: Output iface is not in L2 mode, ifindex: %d\n",
               out_if_config->ifindex);
    return -1;
  }

  if (out_if_config->ifindex == in_if_config->ifindex) {
    bpf_printk("Error: Input and output interfaces are the same, ifindex: %d\n",
               out_if_config->ifindex);
    return -1;
  }
  __u32 out_if_vlan_id = out_if_config->l2.vlan_id;

  if (out_if_config->l2.vlan_mode == VLAN_ACCESS) {
    if (vlan_id != out_if_vlan_id) {
      bpf_printk("Error: VLAN ID mismatch, expected: %d, actual: %d\n",
                 out_if_vlan_id, vlan_id);
      return -1;
    }
    if (is_vlan_pkt) {
      if (vlan_id == 0) {
        bpf_printk("Error: VLAN ID is zero\n");
        return -1;
      }
      return l2_decap_vlan_and_send(send_ctx);
    } else {
      return l2_send_directly(send_ctx);
    }
  } else {
    if (out_if_vlan_id == 0) {
      bpf_printk("Error: Outgoing interface VLAN ID is zero\n");
      return -1;
    }

    if (vlan_id == out_if_vlan_id) {
      if (is_vlan_pkt) {
        if (vlan_id == 0) {
          bpf_printk("Error: VLAN ID is zero\n");
          return -1;
        }
        return l2_decap_vlan_and_send(send_ctx);
      } else {
        return l2_send_directly(send_ctx);
      }
    }

    if (vlan_id >= out_if_config->l2.vlan_range_start &&
        vlan_id <= out_if_config->l2.vlan_range_end) {
      if (is_vlan_pkt) {
        if (vlan_id == 0) {
          bpf_printk("Error: VLAN ID is zero\n");
          return -1;
        }
        return l2_send_directly(send_ctx);
      } else {
        return l2_encap_vlan_and_send(send_ctx);
      }
    }

    bpf_printk("Error: VLAN ID %d is out of range for output interface, range: "
               "[%d, %d]\n",
               vlan_id, out_if_config->l2.vlan_range_start,
               out_if_config->l2.vlan_range_end);
    return -1;
  }

  bpf_printk("Error: Unhandled case in l2_out, vlan_id: %d, is_vlan_pkt: %d\n",
             vlan_id, is_vlan_pkt);
  return -1;
}

static __always_inline int xdp_l2_out(struct xdp_md *ctx,
                                      struct ethhdr *ether_header,
                                      struct vlan_hdr *vlan_header,
                                      __u32 vlan_id) {
  __u32 key = 0;
  struct forward_config *cache =
      bpf_map_lookup_elem(&xdp_forward_config_cache, &key);
  if (cache == NULL || cache->out_if_config.ifindex == 0) {
    bpf_printk("Error: Forward config cache miss or invalid\n");
    return XDP_DROP;
  }

  struct iface_config *out_config = &cache->out_if_config;
  struct iface_config *in_config = &cache->in_if_config;
  struct xdp_l2_send_context send_ctx;
  __builtin_memset(&send_ctx, 0, sizeof(struct xdp_l2_send_context));
  send_ctx.ctx = ctx;
  send_ctx.vlan_id = vlan_id;
  __builtin_memcpy(&send_ctx.ether_header_orignal, ether_header,
                   sizeof(struct ethhdr));
  if (vlan_header != NULL) {
    send_ctx.flages |= vlan_header_orignal_SET;
    __builtin_memcpy(&send_ctx.vlan_header_orignal, vlan_header,
                     sizeof(struct vlan_hdr));
  }
  __builtin_memcpy(&send_ctx.out_config, out_config,
                   sizeof(struct iface_config));

  int ret = l2_out(out_config, in_config, vlan_id, vlan_header != NULL,
                   xdp_l2_decap_vlan_and_send, xdp_l2_encap_vlan_and_send,
                   xdp_l2_send_directly, &send_ctx);
  if (ret != 0) {
    bpf_printk(
        "Error: l2_out failed, out_ifindex: %d, in_ifindex: %d, vlan_id: %d\n",
        out_config->ifindex, in_config->ifindex, vlan_id);
    return XDP_DROP;
  }

  bpf_printk(
      "Debug: l2_out success, out_ifindex: %d, in_ifindex: %d, vlan_id: %d\n",
      out_config->ifindex, in_config->ifindex, vlan_id);
  // send success, so DROP current ctx.
  return XDP_DROP;
}

static long tc_l2_out(struct bpf_map *map, const void *key, void *value,
                      void *ctx) {
  struct iface_config_item *out_if_config_item =
      (struct iface_config_item *)value;
  struct flood_context *flood_ctx = (struct flood_context *)ctx;
  struct iface_config out_if_config;
  bpf_spin_lock(&out_if_config_item->lock);
  __builtin_memcpy(&out_if_config, &out_if_config_item->config,
                   sizeof(struct iface_config));
  bpf_spin_unlock(&out_if_config_item->lock);

  struct iface_config in_if_config;
  __u32 ingress_ifindex = flood_ctx->skb->ingress_ifindex;
  struct iface_config_item *in_if_config_item =
      bpf_map_lookup_elem(&iface_config_map, &ingress_ifindex);
  if (in_if_config_item == NULL) {
    return XDP_DROP;
  }
  bpf_spin_lock(&in_if_config_item->lock);
  __builtin_memcpy(&in_if_config, &in_if_config_item->config,
                   sizeof(struct iface_config));
  bpf_spin_unlock(&in_if_config_item->lock);

  struct dp_metadata *md = &flood_ctx->md;
  __u32 vlan_id = md->l2.vlan_id;

  struct tc_l2_send_context send_ctx = {
      .skb = flood_ctx->skb,
      .out_if_index = out_if_config.ifindex,
      .vlan_id = vlan_id,
  };
  int ret = l2_out(&out_if_config, &in_if_config, vlan_id,
                   md->l2.flags & L2_VLAN_PACKET, tc_l2_decap_vlan_and_send,
                   tc_l2_encap_vlan_and_send, tc_l2_send_directly, &send_ctx);
  if (ret != 0) {
    // return 0, continue others iface.
    return 0;
  }
  // success, return 0.
  return 0;
}

static __always_inline int fdb_flood(struct __sk_buff *skb,
                                     struct dp_metadata *md) {
  struct flood_context ctx = {
      .skb = skb,
  };
  __builtin_memcpy(&ctx.md, md, sizeof(struct dp_metadata));

  int ret = bpf_for_each_map_elem(&iface_config_map, tc_l2_out, &ctx, 0x0);
  if (ret < 0) {
    return TC_ACT_SHOT;
  }

  // DROP self.
  return TC_ACT_SHOT;
}

#endif
