// +build ignore

// vmlinux first
#include "vmlinux.h"

// bpf second
#include "bpf_endian.h"
#include "bpf_helpers.h"

// others
#include "ether.h"

char __license[] SEC("license") = "Dual MIT/GPL";

#define MAX_MAP_ENTRIES 16

/* Define an LRU hash map for storing packet count by source IPv4 address */
struct {
  __uint(type, BPF_MAP_TYPE_LRU_HASH);
  __uint(max_entries, MAX_MAP_ENTRIES);
  __type(key, __u32);   // source IPv4 address
  __type(value, __u32); // packet count
} xdp_stats_map SEC(".maps");

#define CHECK_BOUNDS(ptr, data_end)                                            \
  if ((void *)(ptr) + sizeof(*(ptr)) > (data_end))                             \
    return XDP_DROP;

enum xdp_action arp_process(struct xdp_md *ctx, void *ether_next_data);

SEC("xdp")
int xdp_prog_func(struct xdp_md *ctx) {
  struct ethhdr *ether_header = NULL;
  void *ether_next_data = NULL;

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
    ret = arp_process(ctx, ether_next_data);
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

enum xdp_action arp_process(struct xdp_md *ctx, void *ether_next_data) {
  struct arphdr *arp_header = (struct arphdr *)ether_next_data;
  if (!ethernet_arp_hdr(ctx, ether_next_data, &arp_header)) {
    return XDP_DROP;
  }
  bpf_printk("Debug: ARP packet received, OP:%X\n",
             bpf_ntohs(arp_header->ar_op));

  // 分别处理请求，响应
  return XDP_DROP;
}
