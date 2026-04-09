# Agent 索引

本项目定义了以下自定义 Agent，使用方式请参考 [harness/README.md](harness/README.md)

## Agent 列表

| 英文标识 | 中文名 | 描述 | 路径 |
|----------|--------|------|------|
| `search` | 搜索研究员 | 专门负责搜索和信息收集任务 | `harness/agent/search/agent.md` |
| `engineering` | 工程协调者 | 协调多个子代理完成复杂任务 | `harness/agent/engineering/agent.md` |
| `system-planner` | 系统规划师 | 功能分解、需求分析、API设计、技术架构设计 | `harness/agent/system-planner/agent.md` |
| `project-reviewer` | 方案评审员 | 审查工作提案和输出内容 | `harness/agent/reviewer/agent.md` |
| `project-docs` | 文档处理员 | 读取各种格式文件，转换为Markdown | `harness/agent/docs/agent.md` |

> 注意：所有 Agent 定义已移至 `.ai/harness/` 目录，方便在其他项目中复用。