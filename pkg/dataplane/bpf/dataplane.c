// +build ignore

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

// 定义一个 Map，用于存储计数
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, __u64);
} kprobe_map SEC(".maps");

// 每次调用 mkdir 时触发
SEC("kprobe/sys_mkdir")
int count_mkdir(void *ctx) {
  __u32 key = 0;
  __u64 *val;

  val = bpf_map_lookup_elem(&kprobe_map, &key);
  if (val) {
    __sync_fetch_and_add(val, 1); // 原子自增
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";