结构重要topic
1、go和ebpf直接的数据同步问题

go 更新配置，这个时候 ebpf 还在跑，会发生什么？可能会跑乱吗？通过per_cpu_cache，把要用到的信息cache起来，能解决吗

ebpf更新一些统计数据到go中，多cpu的时候，通过 per_cpu 设计，可以避免

2、相同 ebpf 不同CPU之间是不是会竞争一些map？会不会有问题？同样，per_cpu 能解决？

3、不同 ebpf 之间，怎么传递数据？

| 方式                           | 场景                                       | 优缺点                                |
| ------------------------------ | ------------------------------------------ | ------------------------------------- |
| 1. 控制面 (Go) 同步            | 跨主机或长周期配置                         | 慢，经过内核/用户态切换。             |
| 2. skb->cb / xdp_md->data_meta | 同一个包在不同 Hook 点传递                 | 极快，随包移动。适合 XDP 传给 TC。    |
| 3. Tail Call (尾调用)          | 逻辑拆分（如：主程序 -> L3 处理）          | 共享 ctx，性能极高。                  |
| 4. 共享 Map                    | 两个独立的 eBPF 程序（如：流量监控和转发） | 最通用。只要加载时关联同一个 Map FD。 |

```
// 假设这是你的配置 Value 结构
struct iface_config {
    __u32 ifindex;
    __u32 vlan_id;
    __u32 mode;
    struct bpf_spin_lock lock;
};

// 转发逻辑中的“原子快照”
static __always_inline int process_packet(struct xdp_md *ctx) {
    __u32 key = 0; // 假设根据某种逻辑确定的 Key

    // 1. 从全局 Map 查找配置引用（指针）
    struct iface_config *config_ptr = bpf_map_lookup_elem(&config_map, &key);
    if (!config_ptr) return XDP_PASS;

    // 2. 局部变量（栈内存），用于存放“快照”
    struct iface_config local_cfg;

    // 3. 【关键：临界区】只锁定拷贝动作
    bpf_spin_lock(&config_ptr->lock);

    // 这里执行内存拷贝，速度极快（通常几条 CPU 指令）
    // 使用 __builtin_memcpy 确保 Verifier 满意
    __builtin_memcpy(&local_cfg, config_ptr, sizeof(local_cfg));

    bpf_spin_unlock(&config_ptr->lock);

    // 4. 【后续逻辑】全部基于 local_cfg
    // 即便此时 Go 更新了 config_map，local_cfg 里的 mode 和 vlan_id 依然是匹配的
    if (local_cfg.mode == IF_MODE_L2) {
        return handle_l2_forward(ctx, &local_cfg);
    }

    return XDP_PASS;
}
```

# case: use cache

```
  __u32 in_ifindex = ctx->ingress_ifindex;
  if (existing->ifindex != in_ifindex ||
      (bpf_ktime_get_ns() - existing->last_seen) > 1000000000ULL) {
    existing->ifindex = in_ifindex;
    existing->last_seen = bpf_ktime_get_ns();

    // 注意：由于 existing 是直接指向 Map 内存的指针，
    // 在某些内核版本/Map类型下，你可以直接修改指针值（直接映射），
    // 但最稳妥、跨版本的方法还是写回：
    int ret = bpf_map_update_elem(&fdb_table, &key, existing, BPF_EXIST);
    if (ret == 0) {
      bpf_printk("Debug: update fdb item\n");
    } else {
      bpf_printk("Debug: update fdb item error %x\n", ret);
    }

    return;
  }

  if (existing->ifindex != ctx->ingress_ifindex ||
      (bpf_ktime_get_ns() - existing->last_seen) > 1000000000ULL) {
    existing->ifindex = ctx->ingress_ifindex;
    existing->last_seen = bpf_ktime_get_ns();

    // 注意：由于 existing 是直接指向 Map 内存的指针，
    // 在某些内核版本/Map类型下，你可以直接修改指针值（直接映射），
    // 但最稳妥、跨版本的方法还是写回：
    int ret = bpf_map_update_elem(&fdb_table, &key, existing, BPF_EXIST);
    if (ret == 0) {
      bpf_printk("Debug: update fdb item\n");
    } else {
      bpf_printk("Debug: update fdb item error %x\n", ret);
    }

    return;
  }

  bpf_printk("Debug: fdb learning skip");
  return;
}

第一种ok，第二种不对，报错：
200: (6b) *(u16 *)(r10 -2) = r1       ; R1=scalar() R10=fp0 fp-8=mm??????
; __builtin_memcpy(key.mac, ether_header->h_source, 6);
201: (6b) *(u16 *)(r10 -4) = r2       ; R2_w=scalar() R10=fp0 fp-8=mmmm????
202: (67) r3 <<= 8                    ; R3_w=scalar(umax=65280,var_off=(0x0; 0xff00))
203: (4f) r3 |= r0                    ; R0_w=scalar(umax=255,var_off=(0x0; 0xff)) R3_w=scalar()
204: (67) r5 <<= 16                   ; R5_w=scalar(umax=16711680,var_off=(0x0; 0xff0000))
205: (67) r4 <<= 24                   ; R4_w=scalar(umax=4278190080,var_off=(0x0; 0xff000000),s32_max=2130706432)
206: (4f) r4 |= r5                    ; R4_w=scalar() R5_w=scalar(umax=16711680,var_off=(0x0; 0xff0000))
207: (4f) r4 |= r3                    ; R3_w=scalar() R4_w=scalar()
208: (63) *(u32 *)(r10 -8) = r4       ; R4_w=scalar() R10=fp0 fp-8=mmmmmmmm
209: (bf) r2 = r10                    ; R2_w=fp0 R10=fp0
;
210: (07) r2 += -8                    ; R2_w=fp-8
; struct fdb_value *existing = bpf_map_lookup_elem(&fdb_table, &key);
211: (18) r1 = 0xffff944bd7a7f000     ; R1_w=map_ptr(off=0,ks=8,vs=12,imm=0)
213: (85) call bpf_map_lookup_elem#1          ; R0_w=map_value_or_null(id=2,off=0,ks=8,vs=12,imm=0)
214: (bf) r7 = r0                     ; R0_w=map_value_or_null(id=2,off=0,ks=8,vs=12,imm=0) R7_w=map_value_or_null(id=2,off=0,ks=8,vs=12,imm=0)
; if (!existing) {
215: (55) if r7 != 0x0 goto pc+27 243: R0_w=map_value(off=0,ks=8,vs=12,imm=0) R6=ctx(off=0,imm=0) R7_w=map_value(off=0,ks=8,vs=12,imm=0) R8=pkt(off=0,r=18,imm=0) R9=pkt(off=14,r=18,imm=0) R10=fp0 fp-8=mmmmmmmm fp-24=????mmmm fp-32=pkt
; if (existing->ifindex != ctx->ingress_ifindex ||
243: (71) r1 = *(u8 *)(r7 +1)         ; R1_w=scalar(umax=255,var_off=(0x0; 0xff)) R7_w=map_value(off=0,ks=8,vs=12,imm=0)
244: (67) r1 <<= 8                    ; R1_w=scalar(umax=65280,var_off=(0x0; 0xff00))
245: (71) r2 = *(u8 *)(r7 +0)         ; R2_w=scalar(umax=255,var_off=(0x0; 0xff)) R7_w=map_value(off=0,ks=8,vs=12,imm=0)
246: (4f) r1 |= r2                    ; R1_w=scalar() R2_w=scalar(umax=255,var_off=(0x0; 0xff))
247: (71) r2 = *(u8 *)(r7 +2)         ; R2_w=scalar(umax=255,var_off=(0x0; 0xff)) R7_w=map_value(off=0,ks=8,vs=12,imm=0)
248: (67) r2 <<= 16                   ; R2_w=scalar(umax=16711680,var_off=(0x0; 0xff0000))
249: (71) r3 = *(u8 *)(r7 +3)         ; R3_w=scalar(umax=255,var_off=(0x0; 0xff)) R7_w=map_value(off=0,ks=8,vs=12,imm=0)
250: (67) r3 <<= 24                   ; R3_w=scalar(umax=4278190080,var_off=(0x0; 0xff000000),s32_max=2130706432)
251: (4f) r3 |= r2                    ; R2_w=scalar(umax=16711680,var_off=(0x0; 0xff0000)) R3_w=scalar()
252: (4f) r3 |= r1                    ; R1_w=scalar() R3_w=scalar()
253: (b7) r8 = 12                     ; R8_w=12
; if (existing->ifindex != ctx->ingress_ifindex ||
254: (61) r1 = *(u32 *)(r6 +12)       ; R1_w=scalar(umax=4294967295,var_off=(0x0; 0xffffffff)) R6=ctx(off=0,imm=0)
; if (existing->ifindex != ctx->ingress_ifindex ||
255: (5d) if r3 != r1 goto pc+26      ; R1_w=scalar(umax=4294967295,var_off=(0x0; 0xffffffff)) R3_w=scalar(umax=4294967295,var_off=(0x0; 0xffffffff))
; (bpf_ktime_get_ns() - existing->last_seen) > 1000000000ULL) {
256: (85) call bpf_ktime_get_ns#5     ; R0=scalar()
; (bpf_ktime_get_ns() - existing->last_seen) > 1000000000ULL) {
257: (71) r2 = *(u8 *)(r7 +5)         ; R2_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
258: (67) r2 <<= 8                    ; R2_w=scalar(umax=65280,var_off=(0x0; 0xff00))
259: (71) r1 = *(u8 *)(r7 +4)         ; R1_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
260: (4f) r2 |= r1                    ; R1_w=scalar(umax=255,var_off=(0x0; 0xff)) R2_w=scalar()
261: (71) r3 = *(u8 *)(r7 +6)         ; R3_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
262: (67) r3 <<= 16                   ; R3_w=scalar(umax=16711680,var_off=(0x0; 0xff0000))
263: (71) r1 = *(u8 *)(r7 +7)         ; R1_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
264: (67) r1 <<= 24                   ; R1_w=scalar(umax=4278190080,var_off=(0x0; 0xff000000),s32_max=2130706432)
265: (4f) r1 |= r3                    ; R1_w=scalar() R3_w=scalar(umax=16711680,var_off=(0x0; 0xff0000))
266: (4f) r1 |= r2                    ; R1_w=scalar() R2_w=scalar()
267: (71) r2 = *(u8 *)(r7 +9)         ; R2_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
268: (67) r2 <<= 8                    ; R2_w=scalar(umax=65280,var_off=(0x0; 0xff00))
269: (71) r3 = *(u8 *)(r7 +8)         ; R3_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
270: (4f) r2 |= r3                    ; R2_w=scalar() R3_w=scalar(umax=255,var_off=(0x0; 0xff))
271: (71) r3 = *(u8 *)(r7 +10)        ; R3_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
272: (67) r3 <<= 16                   ; R3_w=scalar(umax=16711680,var_off=(0x0; 0xff0000))
273: (71) r4 = *(u8 *)(r7 +11)        ; R4_w=scalar(umax=255,var_off=(0x0; 0xff)) R7=map_value(off=0,ks=8,vs=12,imm=0)
274: (67) r4 <<= 24                   ; R4_w=scalar(umax=4278190080,var_off=(0x0; 0xff000000),s32_max=2130706432)
275: (4f) r4 |= r3                    ; R3_w=scalar(umax=16711680,var_off=(0x0; 0xff0000)) R4_w=scalar()
276: (4f) r4 |= r2                    ; R2_w=scalar() R4_w=scalar()
277: (67) r4 <<= 32                   ; R4_w=scalar(smax=9223372032559808512,umax=18446744069414584320,var_off=(0x0; 0xffffffff00000000),s32_min=0,s32_max=0,u32_max=0)
278: (4f) r4 |= r1                    ; R1_w=scalar() R4_w=scalar()
; (bpf_ktime_get_ns() - existing->last_seen) > 1000000000ULL) {
279: (1f) r0 -= r4                    ; R0_w=scalar() R4_w=scalar()
280: (b7) r1 = 1000000001             ; R1_w=1000000001
; if (existing->ifindex != ctx->ingress_ifindex ||
281: (2d) if r1 > r0 goto pc+54       ; R0_w=scalar(umin=1000000001) R1_w=1000000001
282: (0f) r6 += r8                    ; R6_w=ctx(off=12,imm=0) R8=12
; existing->ifindex = ctx->ingress_ifindex;
283: (61) r1 = *(u32 *)(r6 +0)
dereference of modified ctx ptr R6 off=12 disallowed
processed 190 insns (limit 1000000) max_states_per_insn 0 total_states 13 peak_states 13 mark_read 7
-- END PROG LOAD LOG --
libbpf: prog 'xdp_prog_func': failed to load: -13
libbpf: failed to load object 'pkg/dataplane/bpf/bpf_bpfel.o'
Error: failed to load object file
make: *** [Makefile:36: bpf_check] Error 255

```

这个报错日志非常经典，它揭示了 LLVM 编译器在优化你的代码时，产生了一个令校验器（Verifier）无法理解的“骚操作”。

### 1. 深度拆解：为什么报错？

关键看这一行：
`282: (0f) r6 += r8  ; R6_w=ctx(off=12,imm=0) R8=12`
`283: (61) r1 = *(u32 *)(r6 +0)`

**翻译一下：**

1.  **R6** 本来是 `ctx` 指针。
2.  编译器为了优化，竟然把 `R8`（数值为 12）加到了 `R6` 上。此时 `R6` 变成了 `ctx + 12`。
3.  下一行尝试从 `R6 + 0` 取值。

**校验器的逻辑：**
eBPF 严禁对 `ctx` 指针进行任何算术运算（指针加减）。即使你只是想读取偏移量为 12 的 `ingress_ifindex`，你也必须写成 `*(u32 *)(ctx + 12)`，而不能先把指针移动 12 位再取值。一旦 `R6` 被做了加法，它就被标记为 **"modified ctx ptr"**，从此这个寄存器就“废了”，再也不能用来解引用。

---

### 2. 为什么第一种写法可以，第二种不行？

你的第一种写法使用了局部变量：
`__u32 in_ifindex = ctx->ingress_ifindex;`

- **第一种情况**：编译器在程序最开始就生成了指令：从 `ctx + 12` 取值存入寄存器或栈。此时 `ctx` 指针（R6）保持原样，没有被修改。后续你用的都是那个存好的数值。
- **第二种情况**：由于你在 `if` 逻辑里多次直接写 `ctx->ingress_ifindex`，编译器的优化器（Optimizer）试图“聪明地”复用指针。它发现反正要读偏移 12 的位置，干脆把指针永久挪过去。这种优化在普通 C 程序里没问题，但在 eBPF 里触犯了红线。

---

### 3. 终极修复方案

**永远不要在逻辑判断或循环中多次访问 `ctx` 的成员。** 在函数入口处做一次性“快照”是解决 99% 校验器指针问题的标准做法。

#### 修改后的代码：

```c
// 1. 必须在函数入口，趁 ctx 指针还没被编译器搞乱，先取出来
__u32 in_ifindex = ctx->ingress_ifindex;

// 2. 之后所有的逻辑，全部使用 in_ifindex，严禁再出现 ctx->ingress_ifindex
if (existing->ifindex != in_ifindex ||
    (bpf_ktime_get_ns() - existing->last_seen) > 1000000000ULL) {

    existing->ifindex = in_ifindex; // 使用局部变量
    existing->last_seen = bpf_ktime_get_ns();

    int ret = bpf_map_update_elem(&fdb_table, &key, existing, BPF_EXIST);
    // ...
    return;
}
```

---

### 4. 额外发现：字节码里的“异常”读取

通过你的日志我还发现了一个细节：
`243: (71) r1 = *(u8 *)(r7 +1)`
`244: (67) r1 <<= 8`
`245: (71) r2 = *(u8 *)(r7 +0)`
`246: (4f) r1 |= r2`

你的编译器没有使用 `u32` 直接读取 `ifindex`，而是一个字节一个字节地读取再拼凑。这通常是因为你的结构体定义里使用了 `__attribute__((packed))`，或者 `ifindex` 字段没有对齐。

**建议：**
检查 `fdb_value` 结构体。如果是为了节省空间用了 `packed`，在 XDP 这种追求极限性能的地方，建议取消 `packed`，保证 4 字节对齐。这样编译器可以直接用一根指令 `(61) r1 = *(u32 *)(r7 +0)` 读完，性能更高。

---

### 总结

- **根因**：编译器优化导致 `ctx` 指针发生了位移运算。
- **对策**：坚持使用第一种方式，**先取值到栈，再进行逻辑处理**。

**既然 FDB 学习的 Bug 解决了，你的 VLAN 封装部分有没有也遇到类似的指针报错？通常 `adjust_head` 之后的指针访问更容易触发这种 `-13` 错误。**
