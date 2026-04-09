---
source_file: controlplane implementation
file_type: go
encoding: utf-8
size: -
---

# Controlplane 实现文档

## 概述

nfvcni 控制面实现，提供动态配置管理功能。

## 目录结构

```
pkg/controlplane/
├── main.go           # 主程序入口
├── cmd/
│   └── cni.go       # CNI 命令实现
├── server/
│   ├── server.go    # HTTP 服务端
│   └── config.go    # 配置管理器
├── client/
│   └── dataplane.go # 数据面客户端
├── types/
│   └── types.go     # 类型定义
└── utils/
    └── logger.go    # 日志工具
```

## 功能模块

### 1. 类型定义 (types/types.go)

- `PortConfig`: 端口配置结构
- `FdbEntry`: FDB 表项
- `PortStatus`: 端口状态
- `InterfaceMode`: 接口模式 (l2/l3/vlanif)
- `L2VlanMode`: VLAN 模式 (access/trunk)
- `Error`: 错误类型

### 2. 配置管理器 (server/config.go)

```go
type ConfigManager interface {
    AddPort(name string, config *PortConfig) error
    DeletePort(name string) error
    UpdatePort(name string, config *PortConfig) error
    GetPort(name string) (*PortConfig, error)
    ListPorts() []*PortConfig
}
```

特性：
- 线程安全 (使用 sync.RWMutex)
- 支持端口的增删改查

### 3. HTTP 服务端 (server/server.go)

提供 REST API：

| 方法 | 路径 | 描述 |
|------|------|------|
| GET | /admin/ports | 获取所有端口 |
| POST | /admin/ports | 添加端口 |
| GET | /admin/ports/{name} | 获取指定端口 |
| PUT | /admin/ports/{name} | 更新端口 |
| DELETE | /admin/ports/{name} | 删除端口 |
| GET | /admin/fdb | 获取 FDB 表 |
| GET | /health | 健康检查 |

### 4. 数据面客户端 (client/dataplane.go)

```go
type DataplaneClient interface {
    AttachPort(ctx context.Context, ifindex uint32, config *PortConfig) error
    DetachPort(ctx context.Context, ifindex uint32) error
    AddFdbEntry(ctx context.Context, mac string, vlan int, ifindex uint32) error
    DelFdbEntry(ctx context.Context, mac string, vlan int) error
    GetFdbTable(ctx context.Context) ([]*FdbEntry, error)
    Close() error
}
```

注意：当前为存根实现，需要在数据面 gRPC 服务实现后进行集成。

### 5. CNI 命令 (cmd/cni.go)

```go
type CNIExecutor interface {
    Add(args *AddArgs) error
    Delete(args *DeleteArgs) error
    Check(args *CheckArgs) error
    Version() string
}
```

## 使用方法

```bash
# 启动控制面
go run pkg/controlplane/main.go -a :8080 -d localhost:50051

# 参数说明
# -a, --addr: 服务监听地址 (默认 :8080)
# -d, --dataplane: 数据面 gRPC 地址
```

## 待实现

1. 数据面 gRPC 服务端 (pkg/dataplane/server)
2. 完整的 DataplaneClient 实现
3. 与 CNI 插件集成