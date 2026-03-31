// +build ignore

// vmlinux first
#include "forward_config.h"
#include "linux_header.h"
#include "vmlinux.h"

// bpf second
#include "bpf_helpers.h"

// others
#include "l2.h"

char __license[] SEC("license") = "Dual MIT/GPL";

SEC("xdp")
int xdp_process(struct xdp_md *ctx) {
  bpf_printk("Debug: xdp_process called, ingress_ifindex: %d\n",
             ctx->ingress_ifindex);
  __u32 key = 0;
  struct forward_config *cache =
      bpf_map_lookup_elem(&xdp_forward_config_cache, &key);
  if (cache == NULL) {
    bpf_printk("Error: Forward config cache lookup failed\n");
    return XDP_DROP;
  }

  __u32 ingress_ifindex = ctx->ingress_ifindex;
  struct iface_config_item *in_if_config =
      bpf_map_lookup_elem(&iface_config_map, &ingress_ifindex);
  if (in_if_config == NULL) {
    bpf_printk("Error: Incoming interface config not found for ifindex %d\n",
               ingress_ifindex);
    return XDP_DROP;
  }

  bpf_spin_lock(&in_if_config->lock);
  __builtin_memcpy(&cache->in_if_config, &in_if_config->config,
                   sizeof(struct iface_config));
  bpf_spin_unlock(&in_if_config->lock);

  if (cache->in_if_config.mode == IF_MODE_L2) {
    bpf_tail_call(ctx, &xdp_jmp_table, XDP_ID_L2);
  } else if (cache->in_if_config.mode == IF_MODE_L3) {
    bpf_tail_call(ctx, &xdp_jmp_table, XDP_ID_L3);
  }

  bpf_printk("Error: Invalid mode %d for ifindex %d\n",
             cache->in_if_config.mode, cache->in_if_config.ifindex);

  return XDP_DROP;
}

SEC("xdp")
int xdp_l2_process(struct xdp_md *ctx) {
  struct iface_config *in_if_config = NULL;
  struct ethhdr *ether_header = NULL;
  struct vlan_hdr *vlan_header = NULL;
  void *ether_next_data = NULL;

  bpf_printk("Debug: xdp_l2_process called, ingress_ifindex: %d\n",
             ctx->ingress_ifindex);

  __u32 key = 0;
  struct forward_config *cache =
      bpf_map_lookup_elem(&xdp_forward_config_cache, &key);
  if (cache == NULL || cache->in_if_config.ifindex == 0) {
    bpf_printk("Error: Forward config cache miss or invalid\n");
    return XDP_DROP;
  }
  in_if_config = &cache->in_if_config;

  // 1. ethernet
  if (!ethernet_ethhdr(ctx, &ether_header)) {
    bpf_printk("Error: Failed to parse Ethernet header\n");
    return XDP_DROP;
  }
  if (!header_next(ctx, ether_header + 1, &ether_next_data)) {
    bpf_printk("Error: Packet too short for Ethernet header\n");
    return XDP_DROP;
  }

  // 2. vlan
  // vlan_tci: parse from pkt.
  // vlan_id: VLAN Scope ID.
  __u16 eth_proto = bpf_ntohs(ether_header->h_proto);
  if (eth_proto == ETH_P_8021Q || eth_proto == ETH_P_8021AD) {
    if (!ethernet_vlan_hdr(ctx, ether_header, &vlan_header)) {
      bpf_printk("Error: Failed to parse VLAN header\n");
      return XDP_DROP;
    }

    eth_proto = bpf_ntohs(vlan_header->h_vlan_encapsulated_proto);
    if (eth_proto == ETH_P_8021Q || eth_proto == ETH_P_8021AD) {
      bpf_printk("Error: QinQ not supported\n");
      return XDP_DROP;
    }

    __u32 vlan_tci = bpf_ntohs(vlan_header->h_vlan_TCI) & 0x0FFF;
    if (vlan_tci == 0) {
      bpf_printk("Debug: Single VLAN detected, but ID is zero\n");
      return XDP_DROP;
    }
    bpf_printk("Debug: Single VLAN detected, ID: %d\n", vlan_tci);

    if (!header_next(ctx, vlan_header + 1, &ether_next_data)) {
      bpf_printk("Error: Packet too short for VLAN header\n");
      return XDP_DROP;
    }
  }
  __u32 vlan_id = 0;
  if (XDP_PASS !=
      l2_recv_vlan(ctx, in_if_config, ether_header, vlan_header, &vlan_id)) {
    bpf_printk("Error: l2_recv_vlan failed, dropping packet\n");
    return XDP_DROP;
  }

  fdb_learning(ctx, ether_header, vlan_header, vlan_id);
  fdb_lookup(ctx, ether_header, vlan_header, vlan_id);

  struct fdb_value *fdb_result = &cache->fdb_value;
  if (fdb_result->ifindex == 0) {
    bpf_printk("Debug: FDB lookup miss for MAC %02x:%02x:%02x:%02x:%02x:%02x, "
               "VLAN ID %d\n",
               ether_header->h_source[0], ether_header->h_source[1],
               ether_header->h_source[2], ether_header->h_source[3],
               ether_header->h_source[4], ether_header->h_source[5], vlan_id);
    return fdb_flood_tag(ctx, ether_header, vlan_header, vlan_id);
  }

  __u32 out_if_index = fdb_result->ifindex;
  struct iface_config_item *out_if_config_item =
      bpf_map_lookup_elem(&iface_config_map, &out_if_index);
  if (!out_if_config_item) {
    bpf_printk("Error: Outgoing interface config not found for ifindex %d\n",
               out_if_index);
    return XDP_DROP;
  }
  bpf_spin_lock(&out_if_config_item->lock);
  __builtin_memcpy(&cache->out_if_config, &out_if_config_item->config,
                   sizeof(struct iface_config));
  bpf_spin_unlock(&out_if_config_item->lock);

  return xdp_l2_out(ctx, ether_header, vlan_header, vlan_id);
}

SEC("xdp")
int xdp_l3_process(struct xdp_md *ctx) {
  struct iface_config *config = NULL;
  struct ethhdr *ether_header = NULL;
  void *ether_next_data = NULL;

  __u32 key = 0;
  struct forward_config *cache =
      bpf_map_lookup_elem(&xdp_forward_config_cache, &key);
  if (cache == NULL || cache->in_if_config.ifindex == 0) {
    return XDP_DROP;
  }
  config = &cache->in_if_config;

  // 1. ethernet
  if (!ethernet_ethhdr(ctx, &ether_header)) {
    return XDP_DROP;
  }
  if (!header_next(ctx, ether_header + 1, &ether_next_data)) {
    return XDP_DROP;
  }

  // 2. vlan
  __u16 eth_proto = bpf_ntohs(ether_header->h_proto);
  if (eth_proto == ETH_P_8021Q || eth_proto == ETH_P_8021AD) {
    struct vlan_hdr *vlan_header = NULL;
    if (!ethernet_vlan_hdr(ctx, ether_header, &vlan_header)) {
      return XDP_DROP;
    }

    eth_proto = bpf_ntohs(vlan_header->h_vlan_encapsulated_proto);
    if (eth_proto == ETH_P_8021Q || eth_proto == ETH_P_8021AD) {
      bpf_printk("Error: QinQ not supported\n");
      return XDP_DROP;
    }

    bpf_printk("Debug: Single VLAN detected, ID: %d, not supported\n",
               bpf_ntohs(vlan_header->h_vlan_TCI) & 0x0FFF);
    if (!header_next(ctx, vlan_header + 1, &ether_next_data)) {
      return XDP_DROP;
    }
    return XDP_DROP;
  }

  enum xdp_action ret = XDP_DROP;
  switch (eth_proto) {
  case ETH_P_ARP:
    // ret = arp_process(ctx, ether_next_data);
    break;
  case ETH_P_IP:
    // 4.2 TODO: IPv4 逻辑
    bpf_printk("Debug: IPv4 packet received\n");
    break;
  case ETH_P_IPV6:
    // 假设暂时不支持 IPv6
    bpf_printk("Error: IPv6 not supported\n");
    break;
  default:
    // 4.3 其他不支持协议
    bpf_printk("Error: Unsupported protocol 0x%x\n", eth_proto);
    return XDP_DROP;
  }

  return ret;
}

SEC("tc")
int tc_ingress(struct __sk_buff *skb) {
  bpf_printk("Debug: tc_ingress called, ingress_ifindex: %d\n",
             skb->ingress_ifindex);

  void *data = (void *)(long)skb->data;
  void *data_meta = (void *)(long)skb->data_meta;

  if (data_meta + sizeof(struct dp_metadata) > data) {
    // pass to kernel
    return TC_ACT_OK;
  }

  struct dp_metadata *md = data_meta;
  if (DP_META_MAGIC != md->magic) {
    // pass to kernel
    return TC_ACT_OK;
  }

  if (md->meta_type == DP_META_L2) {
    bpf_tail_call(skb, &tc_jmp_table, TC_INGRESS_ID_L2);
  } else if (md->meta_type == DP_META_L3) {
    bpf_tail_call(skb, &tc_jmp_table, TC_INGRESS_ID_L3);
  }

  // pass to kernel
  return TC_ACT_OK;
}

SEC("tc") int tc_ingress_l2(struct __sk_buff *skb) {
  void *data = (void *)(long)skb->data;
  void *data_meta = (void *)(long)skb->data_meta;
  if (data_meta + sizeof(struct dp_metadata) > data) {
    return TC_ACT_SHOT;
  }

  struct dp_metadata *md = data_meta;
  if (md->magic != DP_META_MAGIC) {
    return TC_ACT_SHOT;
  }

  if (md->l2.flags & L2_FLOOD) {
    return fdb_flood(skb, md);
  }

  return TC_ACT_SHOT;
}

SEC("tc") int tc_ingress_l3(struct __sk_buff *skb) { return TC_ACT_SHOT; }
