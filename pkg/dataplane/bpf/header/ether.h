#ifndef __ETHER_H
#define __ETHER_H

// vmlinux first
#include "vmlinux.h"

// bpf second
#include "bpf_helpers.h"

// others
// ...

#define ETH_P_8021Q 0x8100  /* 802.1Q VLAN Extended Header  */
#define ETH_P_8021AD 0x88A8 /* 802.1ad Service VLAN		*/
#define ETH_P_ARP 0x0806    /* Address Resolution packet	*/
#define ETH_P_IP 0x0800     /* Internet Protocol packet	*/
#define ETH_P_IPV6 0x86DD   /* IPv6 over bluebook		*/

static __always_inline bool ethernet_ethhdr(struct xdp_md *ctx,
                                            struct ethhdr **eth_out) {
  void *data = (void *)(long)ctx->data;
  struct ethhdr *eth = NULL;
  eth = data;
  void *data_end = (void *)(long)ctx->data_end;
  if ((void *)(eth + 1) > data_end) {
    return false;
  }

  *eth_out = eth;
  return true;
}

static __always_inline bool ethernet_vlan_hdr(struct xdp_md *ctx,
                                              struct ethhdr *eth,
                                              struct vlan_hdr **vhdr_out) {
  struct vlan_hdr *vhdr = (struct vlan_hdr *)(eth + 1);

  void *data_end = (void *)(long)ctx->data_end;
  if ((void *)(vhdr + 1) > data_end) {
    return false;
  }

  *vhdr_out = vhdr;
  return true;
}

static __always_inline bool header_next(struct xdp_md *ctx, void *header_next,
                                        void **header_next_out) {
  void *data_end = (void *)(long)ctx->data_end;
  if (header_next > data_end) {
    return false;
  }

  *header_next_out = header_next;
  return true;
}

static __always_inline bool ethernet_arp_hdr(struct xdp_md *ctx,
                                             void *header_next_out,
                                             struct arphdr **arphdr_out) {
  struct arphdr *arphdr = NULL;
  arphdr = (struct arphdr *)(header_next_out);
  void *data_end = (void *)(long)ctx->data_end;
  if ((void *)(arphdr + 1) > data_end) {
    return false;
  }

  *arphdr_out = arphdr;
  return true;
}

#endif
