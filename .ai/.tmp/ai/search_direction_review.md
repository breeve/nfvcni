# 搜索方向复盘分析

## 问题分析：为何遗漏经典论文？

### 1. 规划阶段的错误

| 问题 | 描述 | 教训 |
|------|------|------|
| **关键词偏差** | 搜索时使用了 "TIMELY"、"DCQCN"、"HPCC" 等具体算法名，而不是 "RDMA SIGCOMM 2015" | 应该从会议/年份入手 |
| **时间范围偏移** | 过度关注2023-2025年最新论文，忽视了2015-2019奠基性论文 | 需要平衡"经典"和"最新" |
| **厂商关注点偏差** | 搜索主要围绕阿里/谷歌/微软，忽视了学术基础论文 | 需要覆盖学术+工业双向 |

### 2. 具体遗漏原因

| 用户提供的论文 | 遗漏原因 |
|---------------|----------|
| RDMA over Commodity Ethernet (SIGCOMM 2015) | 搜索时用了"RoCE"而非"Ethernet" |
| iSwitch (SIGCOMM 2015) | 微软内部项目，未公开宣传 |
| Cloud-Scale Acceleration Architecture | 微软研究院白皮书，非正式论文 |
| Accelerating Distributed ML Training (NSDI 2019) | 搜索关键词偏重"RDMA"而非"ML training" |
| Learning in-Network Congestion Control (SIGCOMM 2022) | 搜索未覆盖ACL/DL在网络控制的应用 |

### 3. 可能还遗漏的方向

| 方向 | 典型论文 | 状态 |
|------|----------|------|
| **RDMA编程接口/Verbs** | rdma-core, libibverbs文档 | 需补充 |
| **硬件加速/SmartNIC** | BlueField系列, SmartNIC offload | 已有但可补充 |
| **RDMA安全** | RDMA安全漏洞、加密 | 缺失 |
| **RDMA虚拟化** | SR-IOV, 容器网络 | 缺失 |
| **中文论文/国内会议** | 中国网络大会、清华学报 | 缺失 |
| **RDMA基准测试** | perftest, rdma-core测试 | 缺失 |

---

## 改进措施

### 搜索策略优化

```
之前:
  搜索 "TIMELY RDMA congestion control"
  搜索 "DeepSeek RDMA optimization"
  
改进后:
  搜索 "SIGCOMM 2015 RDMA papers"
  搜索 "SIGCOMM NSDI datacenter network year:2015-2020"
```

### 搜索流程优化

1. **先确定会议+年份**：SIGCOMM 2015-2020, NSDI 2015-2020
2. **再细分方向**：拥塞控制/存储/ML/硬件
3. **补充厂商实践**：从论文引用中追溯

---

*此复盘文档应保存到审查临时目录*