# nfvcni E2E 测试 - 详细实施计划

## 用户确认

- ✅ 暂不需要 CNI 插件完整集成测试
- ✅ 暂不需要 CI/CD 中运行 e2e 测试
- ✅ 测试报告格式默认 JUnit XML (Ginkgo 原生支持)

## 测试优先级

基于 nfvcni 项目当前状态，优先测试：

1. **控制面 REST API** - 已实现完成，测试价值最高
2. **数据面 gRPC** - 已实现完成，测试价值其次
3. **配置同步** - 控制面 → 数据面，集成测试

---

## 详细 Milestone

### M1: 测试基础设施 (第1天)

| 任务 | 说明 |
|------|------|
| M1.1 | 安装依赖工具 (kind, ginkgo, kubectl) |
| M1.2 | 创建 `test/e2e/` 目录结构 |
| M1.3 | 初始化 Go module 和依赖 |
| M1.4 | 配置测试环境变量 |
| M1.5 | 验证 Go 构建通过 |

```
test/e2e/
├── go.mod
├── go.sum
├── Makefile
├── e2e_suite_test.go
├── framework/
│   └── framework.go
├── controlplane/
│   ├── health_test.go
│   ├── port_test.go
│   └── fdb_test.go
└── dataplane/
    └── grpc_test.go
```

### M2: 基础测试框架 (第2天)

| 任务 | 说明 |
|------|------|
| M2.1 | 创建 Ginkgo suite (`e2e_suite_test.go`) |
| M2.2 | 实现测试辅助函数 (`framework.go`) |
| M2.3 | 实现 HTTP 客户端封装 |
| M2.4 | 实现 gRPC 客户端封装 |
| M2.5 | 验证 suite 可运行 |

### M3: 控制面 API 测试 (第3-4天)

| 任务 | 说明 | 测试用例数 |
|------|------|----------|
| M3.1 | `/health` 健康检查测试 | 2 |
| M3.2 | `POST /admin/ports` 添加Port | 4 |
| M3.3 | `GET /admin/ports` 列表Port | 2 |
| M3.4 | `GET /admin/ports/{name}` 查询单个Port | 2 |
| M3.5 | `PUT /admin/ports/{name}` 更新Port | 3 |
| M3.6 | `DELETE /admin/ports/{name}` 删除Port | 2 |
| M3.7 | 错误码测试 (400, 404, 409) | 3 |

### M4: FDB 测试 (第5天)

| 任务 | 说明 | 测试用例数 |
|------|------|----------|
| M4.1 | `GET /admin/fdb` 查询FDB表 | 2 |
| M4.2 | FDB 条目验证 | 2 |
| M4.3 | 空表格测试 | 1 |

### M5: 数据面 gRPC 测试 (第6-7天)

| 任务 | 说明 | 测试用例数 |
|------|------|----------|
| M5.1 | gRPC 连接测试 | 1 |
| M5.2 | `AttachPort` 测试 | 3 |
| M5.3 | `DetachPort` 测试 | 2 |
| M5.4 | `AddFdbEntry` 测试 | 3 |
| M5.5 | `DelFdbEntry` 测试 | 2 |
| M5.6 | `GetFdbTable` 测试 | 2 |
| M5.7 | 错误场景测试 | 3 |

### M6: 集成测试 (第8天)

| 任务 | 说明 |
|------|------|
| M6.1 | 控制面 → 数据面 配置同步验证 |
| M6.2 | 边界条件测试 |
| M6.3 | 并发测试 (可选) |
| M6.4 | 测试报告生成 |

---

## 测试用例统计

| 模块 | 用例数 |
|------|-------|
| 健康检查 | 2 |
| Port CRUD | 15 |
| FDB | 5 |
| gRPC | 16 |
| 集成 | 4 |
| **总计** | **~42** |

---

## Makefile 目标

```makefile
.PHONY: e2e-setup
e2e-setup:  # 安装 e2e 测试依赖

.PHONY: e2e
e2e:       # 运行 e2e 测试

.PHONY: e2e-controlplane
e2e-controlplane:  # 运行控制面测试

.PHONY: e2e-dataplane
e2e-dataplane:      # 运行数据面测试

.PHONY: e2e-clean
e2e-clean:        # 清理测试环境
```

---

## 时间估算

| Milestone | 天数 | 累计 |
|----------|------|------|
| M1 | 1 | 1 |
| M2 | 1 | 2 |
| M3 | 2 | 4 |
| M4 | 1 | 5 |
| M5 | 2 | 7 |
| M6 | 1 | **8** |

**预计总时间**: 8 天 (按每天工作8小时)

---

## 实施前置条件

- [x] 控制面代码已实现 (REST API)
- [x] 数据面 gRPC 已实现
- [x] 测试规划方案已审查通过

---

## 待用户确认

确认后即可开始实施 M1。