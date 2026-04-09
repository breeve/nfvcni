---
source_file: dataplane gRPC implementation
file_type: go
encoding: utf-8
size: -
---

# 数据面 gRPC 服务实现文档

## 概述

数据面 gRPC 服务实现，提供控制面与数据面的通信接口。

## 目录结构

```
pkg/dataplane/server/
├── server.go              # gRPC 服务端实现
├── proto/
│   ├── dataplane.proto   # Proto 定义
│   ├── dataplane.pb.go  # 生成的 Go 代码
│   └── dataplane_grpc.pb.go  # 生成的 gRPC 代码
```

## 服务定义

### DataplaneService

| 方法 | 描述 |
|------|------|
| AttachPort | 附加端口到数据面 |
| DetachPort | 从数据面分离端口 |
| AddFdbEntry | 添加 FDB 表项 |
| DelFdbEntry | 删除 FDB 表项 |
| GetFdbTable | 获取 FDB 表 |
| GetPortConfig | 获取端口配置 |
| UpdatePortConfig | 更新端口配置 |

## gRPC 服务端

```go
type Server struct {
    proto.UnimplementedDataplaneServiceServer
    grpcServer     *grpc.Server
    ifaceConfigMap *ebpf.Map
    fdbMap         *ebpf.Map
    ports          map[uint32]*PortInfo
    links          map[uint32]interface{}
}
```

### 主要方法

#### AttachPort
```go
func (s *Server) AttachPort(ctx context.Context, req *proto.AttachPortRequest) (*proto.AttachPortResponse, error)
```

#### GetFdbTable
```go
func (s *Server) GetFdbTable(ctx context.Context, req *proto.GetFdbTableRequest) (*proto.GetFdbTableResponse, error)
```

## 与现有代码集成

服务需要与现有的 eBPF 代码集成：
- `IfaceConfigMap`: 端口配置 Map
- `FdbTable`: FDB 表 Map

## 使用方法

启动 gRPC 服务：
```go
srv := server.NewServer()
srv.SetMaps(ifaceConfigMap, fdbMap)
srv.Start(":50051")
```

## Proto 文件

详细定义见 [dataplane.proto](file:///home/zhangfeng5/github/nfvcni/pkg/dataplane/server/proto/dataplane.proto)