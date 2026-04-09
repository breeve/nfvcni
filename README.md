# NFVCNI

基于 eBPF 的 L2/L3 交换机数据平面实现。

## 项目概述

NFVCNI 是一个使用 Go 语言和 eBPF 技术实现的高性能网络数据平面项目，提供 L2 和 L3 交换功能。

### 核心特性

- **L2 交换**：基于 FDB (Forwarding Database) 的 MAC 地址学习和转发
- **L3 交换**：IP 层转发功能
- **VLAN 支持**：Access 和 Trunk 模式
- **高性能**：使用 XDP (eXpress Data Path) 实现数据包快速路径处理
- **线程安全**：使用 bpf_spin_lock 保护共享数据

## 技术架构

### 核心技术栈

- **Go 1.26.0** - 控制平面实现
- **cilium/ebpf v0.21.0** - eBPF 库
- **Cobra** - CLI 框架
- **YAML** - 配置管理
- **eBPF** - 数据平面 (XDP/TC)

### 数据平面组件

| 组件 | 描述 |
|------|------|
| XDP | eXpress Data Path，高性能数据包处理 |
| TC | Traffic Control，流量控制程序 |
| BPF Map | HASH, PERCPU_ARRAY, PROG_ARRAY 类型 |
| Tail Call | 程序跳转表 |

### BPF Map 架构

- `iface_config_map` - 接口配置存储 (HASH, 256 条目)
- `fdb_table` - FDB 转发表 (HASH, 10240 条目)
- `xdp_forward_config_cache` - 转发配置缓存 (PERCPU_ARRAY)
- `xdp_jmp_table` - XDP 程序跳转表 (PROG_ARRAY)

## 目录结构

```
nfvcni/
├── .ai/                      # Agent 配置
│   ├── AGENTS.md            # Agent 索引
│   └── harness/             # Agent 实现
│       ├── agent/           # Agent 定义
│       └── skill/           # Skill 定义
├── doc/                      # 技术文档
│   ├── ebpf.md              # eBPF 技术要点
│   ├── dev.md               # 开发文档
│   └── test.md              # 测试文档
├── pkg/dataplane/           # 数据平面实现
│   ├── main.go              # CLI 入口
│   ├── bpf/                 # eBPF 程序
│   │   ├── bpf.go           # Go 与 BPF 交互
│   │   ├── dataplane.c     # eBPF C 代码
│   │   ├── dataplane_bpfel.o # 编译后的 BPF
│   │   └── header/          # BPF 头文件
│   └── config/              # 配置管理
│       ├── config.go        # 配置结构体
│       └── config.yaml      # 配置示例
├── Makefile                 # 构建脚本
└── go.mod                   # Go 依赖
```

## 快速开始

### 构建环境

```bash
# 安装依赖
make deps
```

### 构建数据平面

```bash
# 构建 dataplane
make dataplane
```

### 配置示例

编辑 `pkg/dataplane/config/config.yaml`:

```yaml
ports:
  - name: eth0
    mode: l2
    l2:
      vlanMode: access
      vlanId: 100
```

### 运行

```bash
./bin/dataplane -c config.yaml
```

## 开发说明

### BPF 程序编译

```bash
# 编译 eBPF 程序
make dataplane_bpf

# 或者使用 Docker
make dataplane_docker
```

### eBPF 验证

```bash
# 检查 BPF 程序
make bpf_check
```

### 提取 vmlinux BTF

```bash
make gateway_dataplane_vmlinux_h
```

## 文档

- [eBPF 技术要点](doc/ebpf.md) - 包含 FDB 学习、VLAN 处理、BPF Map 设计等
- [开发文档](doc/dev.md)
- [测试文档](doc/test.md)

## 依赖

- Go 1.26+
- clang
- llvm
- libbpf-dev
- linux-tools-common
- bpftool