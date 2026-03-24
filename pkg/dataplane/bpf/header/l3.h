#ifndef __L3_H__
#define __L3_H__
#if 0

// vmlinux first
#include "vmlinux.h"

// bpf second
#include "bpf_endian.h"
#include "bpf_helpers.h"

// others
#include "forward_config.h"
#include "linux_header.h"
#include "protocol.h"

struct arp_payload {
  unsigned char sha[6];
  __u32 spa;
  unsigned char tha[6];
  __u32 tpa;
} __attribute__((aligned(8)));

static __always_inline enum xdp_action arp_process(struct xdp_md *ctx,
                                                   void *ether_next_data) {
  __u32 zero = 0;
  struct bpf_config *config = bpf_map_lookup_elem(&config_map, &zero);
  if (!config) {
    return XDP_DROP;
  }
  bpf_printk("Config MAC: %pM\n", config->node_mac);
  __u32 net_ip = config->node_ip;
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
#endif

#endif