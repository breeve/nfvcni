# nfvcni 仓库分析文档

## 项目概述

| 属性 | 内容 |
|------|------|
| **项目名称** | nfvcni (NFV CNI) |
| **项目类型** | Go + eBPF 实现的网络数据平面 |
| **主要功能** | 实现 L2/L3 交换机数据平面，支持 VLAN、FDB 学习 |
| **模块名称** | github.com/breeve/nfvcni |
| **Go版本** | 1.26.0 |

---

## 1. 项目架构

```
nfvcni/
├── pkg/dataplane/          # 主代码目录
│   ├── main.go            # 程序入口 (CLI)
│   ├── config/            # 配置模块
│   │   ├── config.go      # 配置结构体定义
│   │   └── config.yaml    # 配置文件示例
│   └── bpf/               # eBPF 模块
│       ├── bpf.go         # Go与BPF交互代码
│       ├── dataplane.c    # eBPF C代码核心
│       ├── gen.go         # 代码生成
│       ├── dataplane_bpfel.go  # 编译后的BPF对象
│       └── header/        # BPF头文件
├── doc/                   # 文档
├── dist/                  # Docker文件
├── Makefile              # 构建脚本
├── go.mod                # Go依赖
└── README.md             # 项目说明
```

---

## 2. 核心技术栈

| 类别 | 技术/库 |
|------|---------|
| **控制面 (Go)** | Go 1.26, spf13/cobra, gopkg.in/yaml.v3 |
| **数据面 (eBPF)** | cilium/ebpf, XDP, TC (Traffic Control) |
| **构建工具** | clang, llvm, bpftool, go generate |

---

## 3. 功能特性

### 3.1 网络模式

| 模式 | 说明 |
|------|------|
| **L2 (Layer 2)** | 二层交换，基于MAC地址转发 |
| **L3 (Layer 3)** | 三层交换，支持IP层转发 |
| **VLANIF** | VLAN接口模式 |

### 3.2 VLAN 支持

| VLAN模式 | 说明 |
|----------|------|
| **Access** | 接入模式，标记/去除VLAN tag |
| **Trunk** | 干线模式，支持多VLAN通过 |

### 3.3 FDB 学习

- **MAC学习**: 自动学习源MAC地址
- **老化机制**: 1秒超时后刷新FDB条目
- **转发查找**: 根据MAC+VLAN查找目的端口

---

## 4. 代码结构详解

### 4.1 控制面 (Go)

**main.go** - CLI入口

```go
// 使用cobra构建命令行工具
// 支持 -c/--config 指定配置文件
// 加载配置后调用 bpf.Agent() 启动数据平面
```

**bpf.go** - BPF交互核心

- 加载BPF对象文件
- 配置接口（iface_config_map）
- 附加XDP/TC程序到网卡
- 启动FDB表监控循环

### 4.2 数据面 (eBPF)

**dataplane.c** - XDP处理流程

| 处理阶段 | 函数 | 说明 |
|----------|------|------|
| 入口 | `xdp_process` | 主入口，根据接口模式分发 |
| L2处理 | `xdp_l2_process` | 二层交换处理 |
| L3处理 | `xdp_l3_process` | 三层交换处理 |

**关键BPF Map**

| Map名称 | 类型 | 用途 |
|---------|------|------|
| `iface_config_map` | HASH | 接口配置存储 |
| `fdb_table` | HASH | MAC地址学习表 |
| `xdp_forward_config_cache` | PERCPU_ARRAY | 转发配置缓存 |
| `xdp_jmp_table` | PROG_ARRAY | Tail Call跳转表 |

---

## 5. 配置说明

**config.yaml 示例**

```yaml
ports:
  - name: "eth0"
    mode: "l2"           # L2 或 L3 模式
    l2:
      vlanMode: "access"  # access 或 trunk
      vlanId: 0
  - name: "eth1"
    mode: "l2"
    l2:
      vlanMode: "access"
      vlanId: 0
```

---

## 6. 构建与运行

### 构建步骤

```bash
# 1. 安装依赖
make deps          # 安装 bpf2go, clang, llvm, libbpf-dev 等

# 2. 生成BPF代码
make dataplane_bpf # 生成 vmlinux.h 和 BPF 对象文件

# 3. 编译程序
make dataplane    # 编译 dataplane 二进制

# 4. Docker构建
make bpf_image    # 构建BPF镜像
make dataplane_docker # 在Docker中编译
```

### 运行方式

```bash
./bin/dataplane -c config.yaml
```

---

## 7. 技术亮点

1. **Go与eBPF无缝集成**: 使用cilium/ebpf库
2. **高性能数据平面**: 基于XDP实现内核旁路
3. **线程安全设计**: 使用bpf_spin_lock保护共享数据
4. **per-CPU缓存**: 避免多核竞争，提高性能
5. **Tail Call优化**: 使用跳转表实现灵活的分发逻辑

---

## 8. 依赖关系

```
github.com/breeve/nfvcni
├── github.com/cilium/ebpf      (v0.21.0) - eBPF库
├── github.com/spf13/cobra       (v1.9.1)  - CLI框架
└── gopkg.in/yaml.v3             (v3.0.1)  - YAML解析
```
