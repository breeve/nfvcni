# 控制面规划方案

## 1. 现有代码分析

### 1.1 数据面实现

**当前架构**:
- 位置: `pkg/dataplane/`
- 核心组件:
  - `main.go`: CLI 入口，读取配置文件启动
  - `bpf/bpf.go`: eBPF 程序加载和接口绑定
  - `config/config.go`: 配置结构定义

**工作流程**:
1. 读取 `config.yaml` 配置文件
2. 加载 eBPF 程序到内核
3. 将配置写入 eBPF Maps
4. 附加 XDP/TC 程序到指定网络接口

**当前局限**:
- 配置静态化，无法动态更新
- 缺少与 Kubernetes/CNI 的集成
- 缺少对网络状态变化的感知能力

## 2. 控制面需求分析

### 2.1 核心需求

| 需求 | 描述 | 优先级 |
|------|------|--------|
| 动态配置管理 | 支持运行时动态添加/删除端口和配置 | P0 |
| CNI 集成 | 支持 Kubernetes CNI 标准接口 | P0 |
| 状态同步 | 与数据面状态保持同步 | P1 |
| 生命周期管理 | 端口的创建、删除、更新操作 | P1 |
| 事件通知 | 支持配置变更事件通知 | P2 |

### 2.2 功能拆解

```
控制面功能
├── CNI 接口实现
│   ├── ADD command (添加网络)
│   ├── DEL command (删除网络)
│   ├── CHECK command (检查状态)
│   └── VERSION command (版本查询)
├── 配置管理
│   ├── 端口配置管理
│   ├── VLAN 配置管理
│   └── FDB 表管理
├── 数据面通信
│   ├── eBPF Map 交互
│   ├── 状态同步机制
│   └── 实时配置更新
└── 辅助功能
    ├── 日志和监控
    └── 健康检查
```

## 3. 技术设计方案

### 3.1 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                         控制面                                │
├─────────────────────────────────────────────────────────────┤
│  CNI Server (gRPC)          │    Config Manager             │
│  - ADD/DEL/CHECK            │    - Port Manager            │
│  - 标准 CNI 接口             │    - VLAN Manager           │
│                             │    - FDB Manager             │
├─────────────────────────────────────────────────────────────┤
│                         Data Access                          │
├─────────────────────────────────────────────────────────────┤
│                      数据面 (pkg/dataplane)                  │
│  - eBPF Maps                │    - XDP/TC Programs         │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 目录结构

```
pkg/
├── controlplane/              # 控制面代码
│   ├── main.go               # 控制面入口
│   ├── cmd/                  # 命令行
│   │   └── cni.go           # CNI 命令实现
│   ├── server/               # 服务端
│   │   ├── cni.go           # CNI gRPC 服务
│   │   └── config.go        # 配置管理服务
│   ├── client/              # 客户端 (与数据面交互)
│   │   └── dataplane.go     # 数据面客户端
│   ├── types/               # 类型定义
│   │   └── types.go
│   └── utils/               # 工具函数
│       └── logger.go
└── dataplane/               # 数据面 (现有)
    ├── main.go
    ├── bpf/
    └── config/
```

### 3.3 核心组件设计

#### 3.3.1 CNI 接口

```go
// CNI 命令接口
type CNIInterface interface {
    Add(args *AddArgs) error
    Delete(args *DeleteArgs) error
    Check(args *CheckArgs) error
    Version() string
}

// Add 参数
type AddArgs struct {
    ContainerID string
    NetNS       string
    IfName      string
    Args        string
    CNIConf     []byte
}
```

#### 3.3.2 配置管理器

```go
type ConfigManager interface {
    AddPort(name string, config *PortConfig) error
    DeletePort(name string) error
    UpdatePort(name string, config *PortConfig) error
    GetPort(name string) (*PortConfig, error)
    ListPorts() []*PortConfig
}
```

#### 3.3.3 数据面交互

```go
// 与数据面通信的客户端
type DataplaneClient interface {
    // 端口管理
    AttachPort(ifindex uint32, config *PortConfig) error
    DetachPort(ifindex uint32) error

    // FDB 表操作
    AddFdbEntry(mac string, vlan int, ifindex uint32) error
    DelFdbEntry(mac string, vlan int) error

    // 状态查询
    GetFdbTable() ([]FdbEntry, error)
}
```

### 3.4 数据面扩展

为支持控制面，需要扩展现有数据面：

1. **gRPC 服务**: 在数据面增加 gRPC 服务供控制面调用
2. **动态配置**: 支持运行时更新 eBPF Maps
3. **状态推送**: 支持将数据面状态推送到控制面

```go
// pkg/dataplane/server/server.go
type Server struct {
    grpcServer *grpc.Server
    configMap  *ebpf.Map
    fdbMap     *ebpf.Map
}

func (s *Server) AttachPort(ctx context.Context, req *AttachRequest) (*AttachResponse, error)
func (s *Server) DetachPort(ctx context.Context, req *DetachRequest) (*DetachResponse, error)
func (s *Server) GetFdbTable(ctx context.Context, req *FdbRequest) (*FdbResponse, error)
```

## 4. API 设计

### 4.1 CNI API

| 方法 | 路径 | 描述 |
|------|------|------|
| ADD | POST /cni/add | 添加网络接口 |
| DEL | POST /cni/del | 删除网络接口 |
| CHECK | POST /cni/check | 检查网络接口状态 |
| VERSION | GET /cni/version | 获取 CNI 版本 |

### 4.2 管理 API (内部)

| 方法 | 路径 | 描述 |
|------|------|------|
| POST | /admin/ports | 添加端口 |
| DELETE | /admin/ports/{name} | 删除端口 |
| PUT | /admin/ports/{name} | 更新端口配置 |
| GET | /admin/ports | 获取所有端口 |
| GET | /admin/ports/{name} | 获取指定端口 |
| GET | /admin/fdb | 获取 FDB 表 |

## 5. 实现步骤

### Phase 1: 基础架构 (2-3天)

1. 创建 `pkg/controlplane` 目录结构
2. 实现 CNI 主接口框架
3. 实现配置管理基础功能
4. 编写单元测试

### Phase 2: 数据面扩展 (2-3天)

1. 在数据面增加 gRPC 服务
2. 实现端口 Attach/Detach 接口
3. 实现 FDB 表操作接口
4. 集成现有 eBPF 代码

### Phase 3: CNI 集成 (2-3天)

1. 实现完整的 CNI ADD/DEL 命令
2. 实现网络命名空间管理
3. 实现 Veth pair 创建和管理
4. 与 Kubernetes 集成测试

### Phase 4: 高级功能 (2-3天)

1. 实现状态同步机制
2. 实现事件通知
3. 增加监控和日志
4. 性能优化和调优

## 6. 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|----------|
| eBPF Map 并发更新冲突 | 中 | 高 | 使用锁或原子操作 |
| gRPC 服务稳定性 | 中 | 高 | 增加超时和重试机制 |
| 网络命名空间泄漏 | 低 | 高 | 使用 defer 确保清理 |
| 与 CNI 版本兼容 | 中 | 中 | 支持多版本 CNI API |

## 7. 后续建议

1. **优先实现**: Phase 1 + Phase 2，先实现基础功能
2. **测试策略**: 使用 integration tests 验证 CNI 接口
3. **文档**: 补充 API 文档和使用示例
4. **CI/CD**: 增加单元测试覆盖率检查