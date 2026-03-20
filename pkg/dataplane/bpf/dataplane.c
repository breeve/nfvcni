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

struct bpf_config {
  __u8 node_mac[6];
  __u32 node_ip; // HOST ByteOrder(LittleEndian)
};

struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, struct bpf_config);
} config_map SEC(".maps");

#define CHECK_BOUNDS(ptr, data_end)                                            \
  if ((void *)(ptr) + sizeof(*(ptr)) > (data_end))                             \
    return XDP_DROP;

static __always_inline enum xdp_action arp_process(struct xdp_md *ctx,
                                                   void *ether_next_data);

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

struct arp_payload {
  unsigned char sha[6];
  __u32 spa;
  unsigned char tha[6];
  __u32 tpa;
} __attribute__((packed));

static __always_inline enum xdp_action arp_process(struct xdp_md *ctx,
                                                   void *ether_next_data) {
  __u32 zero = 0;
  struct bpf_config *config = bpf_map_lookup_elem(&config_map, &zero);
  if (!config) {
    return XDP_DROP;
  }
  bpf_printk("Config MAC: %pM\n", config->node_mac);
  __u32 net_ip = bpf_htonl(config->node_ip); // 字节序有问题
  bpf_printk("Config IP: %pI4 (HostHex: 0x%x)\n", &net_ip, config->node_ip);

  void *data_end = (void *)(long)ctx->data_end;
  struct arphdr *arp_header = (struct arphdr *)ether_next_data;
  if (!ethernet_arp_hdr(ctx, ether_next_data, &arp_header)) {
    return XDP_DROP;
  }

  struct arp_payload *payload = NULL;
  if (!proto_payload(ctx, (void *)(arp_header + 1), sizeof(struct arp_payload),
                     (void **)&payload)) {
    return XDP_DROP;
  }

  __u16 op = bpf_ntohs(arp_header->ar_op);
  if (op == ARPOP_REQUEST) {
    __u32 target_ip = payload->tpa;
    bpf_printk("Debug: Got ARP Request for IP: %pI4\n", &target_ip);

    struct ethhdr *eth = NULL;
    if (!ethernet_ethhdr(ctx, &eth)) {
      return XDP_DROP;
    }

    // 1. gratuitou arp
    bool is_gratuitous = (payload->spa == payload->tpa);
    if (is_gratuitous) {
      // TODO update arp table
      bpf_printk("Debug: ARP cache update\n");

      __u32 source_ip = payload->spa;
      bpf_printk("Debug: Gratuitous ARP detected from %pI4\n", &source_ip);
      return XDP_DROP;
    }

    // 2. check
    if (target_ip != bpf_ntohl(config->node_ip)) {
      bpf_printk("Debug: ARP Request is others, drop\n");
      return XDP_DROP;
    }

    // 3. arp reply

    // ethernet header
    __u8 temp_mac[6];
    __builtin_memcpy(eth->h_dest, eth->h_source, 6);
    __builtin_memcpy(eth->h_source, config->node_mac, 6);

    // vlan
    // todo

    // arp header
    arp_header->ar_op = bpf_htons(ARPOP_REPLY);
    // arp payload
    target_ip = payload->tpa;
    payload->tpa = payload->spa;
    payload->spa = target_ip;
    __builtin_memcpy(payload->tha, payload->sha, 6);
    __builtin_memcpy(payload->sha, eth->h_source, 6);

    bpf_printk("Debug: Sending ARP Reply\n");
    return XDP_TX;
  }

  if (op == 2) { // ARPOP_REPLY
    __u32 source_ip = payload->spa;
    bpf_printk("Debug: Got ARP Reply from %pI4\n", &source_ip);
    // 如果你的 CNI 需要记录邻居表，可以在这里更新 BPF Map
    return XDP_PASS;
  }
  return XDP_DROP;
}
