#ifndef __FORWARD_CONFIG_H
#define __FORWARD_CONFIG_H

// vmlinux first
#include "vmlinux.h"

// bpf second
#include "bpf_helpers.h"

#define SECOND 1000000000ULL

struct iface_config_l2 {
  __u32 vlan_id; // access.vlanid or trunk.nativeID
#define VLAN_ACCESS 0x00
#define VLAN_TRUNK 0x01
  __u32 vlan_mode;
  __u32 vlan_range_start; // [start,end]
  __u32 vlan_range_end;
} __attribute__((aligned(8)));

struct iface_config {
  struct bpf_spin_lock lock;
  __u32 ifindex;
  __u8 name[64];
  __u8 node_mac[6];
#define IF_MODE_L2 0x00
#define IF_MODE_L3 0x01
  __u16 mode;

  struct iface_config_l2 l2 __attribute__((aligned(8)));
} __attribute__((aligned(8)));

struct iface_config_item {
  struct bpf_spin_lock lock;
  struct iface_config config;
} __attribute__((aligned(8)));

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 256);
  __type(key, __u32); // ifindex
  __type(value, struct iface_config_item);
} iface_config_map SEC(".maps");

struct fdb_key {
  __u8 mac[6];
  __u16 vlan_id;
} __attribute__((aligned(8)));

struct fdb_value {
  __u32 ifindex;
  __u64 last_seen; // 用于老化逻辑（Aging）
} __attribute__((aligned(8)));

struct fdb_value_item {
  struct bpf_spin_lock lock;
  struct fdb_value value;
} __attribute__((aligned(8)));

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 10240); // 预分配 1w 条记录，足够小规模 CNI 使用
  __type(key, struct fdb_key);
  __type(value, struct fdb_value_item);
} fdb_table SEC(".maps");

struct forward_config {
  struct iface_config in_if_config;
  struct iface_config out_if_config;
  struct fdb_value fdb_value;
} __attribute__((aligned(8)));

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, struct forward_config);
} forward_config_cache SEC(".maps");

enum {
  PROG_ID_L2 = 0,
  PROG_ID_L3 = 1,
  PROG_ID_MAX,
};

SEC("xdp") int xdp_l2_process(struct xdp_md *ctx);
SEC("xdp") int xdp_l3_process(struct xdp_md *ctx);

struct {
  __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
  __uint(max_entries, PROG_ID_MAX);
  __uint(key_size, sizeof(__u32));
  __uint(value_size, sizeof(__u32));
  __array(values, int());
} jmp_table SEC(".maps") = {
    .values =
        {
            [PROG_ID_L2] = &xdp_l2_process,
            [PROG_ID_L3] = &xdp_l3_process,
        },
};

#endif
