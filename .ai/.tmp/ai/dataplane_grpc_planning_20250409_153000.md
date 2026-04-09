# 数据面 gRPC 服务规划

## 1. 概述

为支持控制面与数据面的通信，需要在数据面实现 gRPC 服务。控制面通过 gRPC 调用数据面，完成端口管理、FDB 表操作等功能。

## 2. 服务定义

### 2.1 gRPC 服务接口

```protobuf
service DataplaneService {
    // 端口管理
    rpc AttachPort(AttachPortRequest) returns (AttachPortResponse);
    rpc DetachPort(DetachPortRequest) returns (DetachPortResponse);

    // FDB 表操作
    rpc AddFdbEntry(AddFdbEntryRequest) returns (AddFdbEntryResponse);
    rpc DelFdbEntry(DelFdbEntryRequest) returns (DelFdbEntryResponse);
    rpc GetFdbTable(GetFdbTableRequest) returns (GetFdbTableResponse);

    // 端口配置
    rpc GetPortConfig(GetPortConfigRequest) returns (GetPortConfigResponse);
    rpc UpdatePortConfig(UpdatePortConfigRequest) returns (UpdatePortConfigResponse);
}
```

### 2.2 请求/响应结构

```protobuf
message AttachPortRequest {
    uint32 ifindex = 1;
    PortConfig config = 2;
}

message AttachPortResponse {
    bool success = 1;
    string error = 2;
}

message DetachPortRequest {
    uint32 ifindex = 1;
}

message DetachPortResponse {
    bool success = 1;
    string error = 2;
}

message AddFdbEntryRequest {
    string mac = 1;
    uint32 vlan_id = 2;
    uint32 ifindex = 3;
}

message AddFdbEntryResponse {
    bool success = 1;
    string error = 2;
}

message DelFdbEntryRequest {
    string mac = 1;
    uint32 vlan_id = 2;
}

message DelFdbEntryResponse {
    bool success = 1;
    string error = 2;
}

message GetFdbTableRequest {}

message GetFdbTableResponse {
    repeated FdbEntry entries = 1;
}

message FdbEntry {
    string mac = 1;
    uint32 vlan_id = 2;
    uint32 ifindex = 3;
    int64 last_seen = 4;
}

message PortConfig {
    string name = 1;
    string mode = 2;
    L2PortConfig l2 = 3;
}

message L2PortConfig {
    string vlan_mode = 1;
    uint32 vlan_id = 2;
    uint32 start_vlan_id = 3;
    uint32 end_vlan_id = 4;
}

message GetPortConfigRequest {
    uint32 ifindex = 1;
}

message GetPortConfigResponse {
    PortConfig config = 1;
    string error = 2;
}

message UpdatePortConfigRequest {
    uint32 ifindex = 1;
    PortConfig config = 2;
}

message UpdatePortConfigResponse {
    bool success = 1;
    string error = 2;
}
```

## 3. 目录结构

在 `pkg/dataplane/` 下新增：

```
pkg/dataplane/
├── main.go              # 现有入口
├── bpf/
│   └── bpf.go           # 现有 eBPF 代码
├── config/
│   └── config.go        # 现有配置
├── server/
│   ├── server.go        # gRPC 服务端实现
│   ├── proto/
│   │   └── dataplane.pb.go  # 生成的 proto 代码
│   └── gen.go           # proto 生成脚本
└── client/              # (可选) 供测试使用
```

## 4. 实现步骤

### Step 1: 创建 proto 文件 (5分钟)
- 定义 gRPC 服务和消息类型
- 生成 Go 代码

### Step 2: 实现 gRPC 服务端 (15分钟)
- 创建 server/server.go
- 实现各个 RPC 方法
- 与现有 eBPF 代码集成

### Step 3: 修改 main.go (5分钟)
- 添加 gRPC 服务启动逻辑
- 添加命令行参数支持

## 5. 技术要点

1. **eBPF Map 交互**: 使用 cilium/ebpf 库进行 Map 读写
2. **并发安全**: 使用 sync.Mutex 保护共享资源
3. **错误处理**: 返回详细的错误信息
4. **资源清理**: 确保退出时正确关闭资源

## 6. 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|----------|
| eBPF Map 操作失败 | 中 | 高 | 增加错误日志和重试机制 |
| gRPC 服务崩溃 | 低 | 高 | 添加健康检查和恢复机制 |
| 端口冲突 | 中 | 中 | 使用唯一标识符 |