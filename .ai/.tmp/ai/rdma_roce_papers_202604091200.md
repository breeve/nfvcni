# RDMA/RoCE 论文与参考资料索引表

> 整理日期：2026-04-09

## 论文与参考资料汇总表

### 一、基础与概述

| # | 论文/资料名称 | 来源 | 年份 | 链接 | 验证状态 |
|---|---------------|------|------|------|----------|
| 1 | RoCE in the Data Center | NVIDIA 白皮书 | 2020 | https://network.nvidia.com/related-docs/whitepapers/roce_in_the_data_center.pdf | ✅ |
| 2 | 一文读懂 RDMA / RoCE / iWARP | CSDN 博客 | 2025 | https://m.toutiao.com/group/7625100204043190827/ | ✅ |
| 3 | RoCEv2 高性能传输协议与 Lossless 无损网络 | CSDN 博客 | 2025 | https://blog.csdn.net/Jmilk/article/details/145773002 | ✅ |
| 4 | 大模型底层依赖层：RoCE协议 | CSDN 博客 | 2025 | https://blog.csdn.net/hiwangwenbing/article/details/153937097 | ✅ |
| 5 | 以太网RDMA网卡综述 | 计算机研究与发展 | 2025 | https://crad.ict.ac.cn/article/doi/10.7544/issn1000-1239.202331036 | ✅ |

### 二、拥塞控制算法

| # | 论文/资料名称 | 来源 | 年份 | 链接 | 验证状态 |
|---|---------------|------|------|------|----------|
| 1 | DCQCN: Congestion Control for Large-Scale RDMA Deployments | SIGCOMM | 2015 | https://courses.cs.duke.edu/fall19/compsci514/papers/dcqcn.pdf | ✅ |
| 2 | TIMELY: RTT-based Congestion Control for the Datacenter | SIGCOMM | 2015 | http://radhikam.web.illinois.edu/TIMELY-sigcomm-talk.pdf | ✅ |
| 3 | ECN or Delay: Lessons Learnt from Analysis of DCQCN and TIMELY | CoNEXT | 2016 | https://dl.acm.org/doi/10.1145/2999572.2999593 | ⚠️ |
| 4 | HPCC: High Precision Congestion Control | SIGCOMM | 2019 | https://minlanyu.seas.harvard.edu/writeup/sigcomm19.pdf | ✅ |
| 5 | Swift: Delay is Simple and Effective for Congestion Control | SIGCOMM | 2020 | https://research.google/pubs/swift-delay-is-simple-and-effective-for-congestion-control-in-the-datacenter/ | ✅ |
| 6 | IRN: Revisiting Network Support for RDMA | SIGCOMM | 2018 | https://arxiv.org/abs/1806.08159 | ✅ |
| 7 | Rethinking Intra-host Congestion Control in RDMA Networks | APNet | 2024 | https://conferences.sigcomm.org/events/apnet2024/slides/Session2/Session2-2-RHCC.pdf | ✅ |
| 8 | DCP: Revisiting RDMA Reliability for Lossy Fabrics | SIGCOMM | 2025 | https://singgel.blog.csdn.net/article/details/152935829 | ✅ |

### 三、大厂实践论文

| # | 论文/资料名称 | 来源/厂商 | 年份 | 链接 | 验证状态 |
|---|---------------|-----------|------|------|----------|
| 1 | HPCC: High Precision Congestion Control | 阿里/SIGCOMM | 2019 | https://minlanyu.seas.harvard.edu/writeup/sigcomm19.pdf | ✅ |
| 2 | When Cloud Storage Meets RDMA | 阿里/NSDI | 2021 | https://www.usenix.org/system/files/nsdi21-gao.pdf | ✅ |
| 3 | Jupiter Rising: A Decade of Clos Topologies | 谷歌/SIGCOMM | 2015 | https://cacm.acm.org/magazines/2016/9/206261-jupiter-rising/fulltext | ✅ |
| 4 | B4: Experience with a Globally-Deployed Software Defined WAN | 谷歌/SIGCOMM | 2014 | http://www.cs.ucsd.edu/classes/sp15/cse291-b/papers/p3-jain.pdf | ✅ |
| 5 | B4 and After: Managing Hierarchy | 谷歌/SIGCOMM | 2018 | https://courses.cs.duke.edu/compsci514/cps214/current/readings/b4after-sigcomm18.pdf | ✅ |
| 6 | SocksDirect: Datacenter Sockets can be Fast and Compatible | 微软/SIGCOMM | 2019 | https://www.microsoft.com/en-us/research/publication/socksdirect-datacenter-sockets-can-be-fast-and-compatible/ | ✅ |
| 7 | Empowering Azure Storage with RDMA | 微软/NSDI | 2023 | https://www.microsoft.com/en-us/research/publication/empowering-azure-storage-with-rdma/ | ✅ |
| 8 | RDMA at Hyperscale: Experience and Future Directions | 微软/APNet | 2023 | https://conferences.sigcomm.org/events/apnet2023/slides/Industry/bai-1500-0630.pdf | ✅ |
| 9 | RDMA over Ethernet for Distributed AI Training at Meta Scale | 谷歌/SIGCOMM | 2024 | http://cs.stanford.edu/~keithw/sigcomm2024/sigcomm24-final246-acmpaginated.pdf | ✅ |

### 四、PFC与无损网络问题

| # | 论文/资料名称 | 来源 | 年份 | 链接 | 验证状态 |
|---|---------------|------|------|------|----------|
| 1 | Tagger: Practical PFC Deadlock Prevention in Data Center Networks | SIGCOMM | 2017 | https://dl.acm.org/doi/pdf/10.1145/3143361.3143382 | ⚠️ |
| 2 | Deadlocks in datacenter: why do they form and how to avoid them | HotNets | 2016 | - | - |
| 3 | DCFIT: Initial Trigger-Based PFC Deadlock Detection in the Data Plane | arXiv | 2020 | https://ar5iv.org/html/2009.13446 | ✅ |
| 4 | 大模型训练无损网络(PFC流量控制+ECN拥塞控制) | CSDN 博客 | 2021 | https://blog.csdn.net/bandaoyu/article/details/115346857 | ✅ |

### 五、新兴AI公司实践

| # | 论文/资料名称 | 来源/厂商 | 年份 | 链接 | 验证状态 |
|---|---------------|-----------|------|------|----------|
| 1 | 多GPU传输RDMA动态切片技术 | DeepSeek 专利 | 2025 | https://blog.csdn.net/mobingyu/article/details/145748166 | ✅ |
| 2 | DeepSeek在K8s集群中的部署与运维 | CSDN 博客 | 2026 | https://blog.csdn.net/qinzhenyan/article/details/158317831 | ✅ |
| 3 | MiniMax-01: Scaling Foundation Models with Lightning Attention | arXiv | 2025 | https://arxiv.org/pdf/2501.08313v1 | ✅ |
| 4 | Deploy DeepSeek-R1 on Amazon EKS using vLLM | AWS | 2025 | https://repost.aws/ja/articles/ARck47n_4dRPe7J5S4f5e6-g/ | ✅ |
| 5 | Thousand-GPU Large-Scale Training | arXiv | 2026 | https://arxiv.org/abs/2603.11101v1 | ✅ |
| 6 | NVIDIA Ethernet for xAI Colossus | NVIDIA | 2024 | https://investor.nvidia.com/news/press-release-details/2024/ | ✅ |

### 六、存储与系统

| # | 论文/资料名称 | 来源 | 年份 | 链接 | 验证状态 |
|---|---------------|------|------|------|----------|
| 1 | An RDMA-First Object Storage System with SmartNIC Offload | arXiv | 2025 | https://arxiv.org/pdf/2509.13997 | ✅ |
| 2 | Lustre over iWARP RDMA vs IB-FDR | Chelsio 白皮书 | - | https://www.chelsio.com/wp-content/uploads/resources/Lustre-Over-iWARP-vs-IB-FDR.pdf | ✅ |
| 3 | A Deep Dive into RDMA (RoCE-v2) Networking | Intel GitHub Wiki | 2024 | https://github.com/intel/intel-technology-enabling-for-openshift/wiki/A-Deep-Dive-into-RDMA-(RoCE%E2%80%90v2)-Networking | ✅ |
| 4 | Scaling LLM Training Using RDMA over Converged Ethernet | SCinet | 2025 | https://scinet.supercomputing.org/community/documents/159/ | ✅ |

### 七、技术博客与教程

| # | 标题 | 来源 | 链接 | 验证状态 |
|---|------|------|------|----------|
| 1 | RDMA for Storage Ethernet: RoCE vs. iWARP Guide | Intelligent Visibility | https://intelligentvisibility.com/rdma-roce-iwarp-guide | ✅ |
| 2 | RoCE Introduction | RoCE Initiative | https://www.roceinitiative.org/roce-introduction/ | ✅ |
| 3 | The Evolution of RoCE: From RoCEv1 to RoCEv2 | orhanergun.net | https://orhanergun.net/the-evolution-of-roce-from-rocev1-to-rocev2 | ✅ |
| 4 | 大规模RDMA AI组网技术创新 | CSDN 博客 | https://blog.csdn.net/Jmilk/article/details/145792856 | ✅ |
| 5 | 突破多GPU传输极限！RDMA动态切片 | CSDN 博客 | https://blog.csdn.net/mobingyu/article/details/145748166 | ✅ |

---

## 说明

- **验证状态**：✅ 表示链接可访问，⚠️ 表示需要进一步验证
- **按阅读顺序排列**：从基础概述 → 核心技术 → 实践案例 → 新兴技术
- **覆盖范围**：学术界（SIGCOMM/NSDI/OSDI）、工业界（阿里/谷歌/微软/Meta/DeepSeek）