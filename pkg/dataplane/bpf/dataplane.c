// +build ignore

#include "vmlinux.h"

#include "bpf_endian.h"
#include "bpf_helpers.h"

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

SEC("xdp")
int xdp_prog_func(struct xdp_md *ctx) {
  void *data_end = (void *)(long)ctx->data_end;
  void *data = (void *)(long)ctx->data;

  // 1. 提取以太网头并判定
  struct ethhdr *eth = data;
  if ((void *)(eth + 1) > data_end) {
    return XDP_DROP; // 内存越界，丢弃
  }

  // 获取协议号
  __u16 eth_proto = bpf_ntohs(eth->h_proto);

  // 指向以太网头之后的负载位置
  void *cursor = (void *)(eth + 1);

  // 2. 处理 VLAN
  // 检查是否是 VLAN (0x8100) 或 Q-in-Q (0x88a8)
  if (eth_proto == ETH_P_8021Q || eth_proto == ETH_P_8021AD) {
    struct vlan_hdr *vhdr = cursor;
    if ((void *)(vhdr + 1) > data_end)
      return XDP_DROP;

    // 提取真正的内层协议号
    eth_proto = bpf_ntohs(vhdr->h_vlan_encapsulated_proto);
    cursor = (void *)(vhdr + 1);

    // 2.2 判定是不是 QinQ (不支持多层)
    if (eth_proto == ETH_P_8021Q || eth_proto == ETH_P_8021AD) {
      bpf_printk("Error: QinQ not supported\n");
      return XDP_DROP;
    }

    // 2.1 TODO: 处理单层 VLAN 逻辑
    bpf_printk("Debug: Single VLAN detected, ID: %d\n",
               bpf_ntohs(vhdr->h_vlan_TCI) & 0x0FFF);
  }

  // 3. 此时 eth_proto 已经是剥离 VLAN 后的协议号
  // 4. 协议分发
  switch (eth_proto) {
  case ETH_P_ARP:
    // 4.1 TODO: ARP 逻辑
    bpf_printk("Debug: ARP packet received\n");
    return XDP_PASS;

  case ETH_P_IP:
    // 4.2 TODO: IPv4 逻辑
    bpf_printk("Debug: IPv4 packet received\n");
    return XDP_PASS;

  case ETH_P_IPV6:
    // 假设暂时不支持 IPv6
    bpf_printk("Error: IPv6 not supported\n");
    return XDP_DROP;

  default:
    // 4.3 其他不支持协议
    bpf_printk("Error: Unsupported protocol 0x%x\n", eth_proto);
    return XDP_DROP;
  }

  return XDP_DROP;
}