# AI Harness - Agent 和 Skill 工具集

本目录包含可复用的 Agent 和 Skill 定义，方便在其他项目中直接使用。

## 目录结构

```
harness/
├── README.md                    # 本文件
├── rules/                        # 规则定义
│   └── agent-rules/             # Agent通用规则
│       └── SKILL.md
├── agent/                       # Agent 定义
│   ├── search/                  # 搜索研究员
│   │   └── agent.md
│   ├── engineering/             # 工程协调者
│   │   └── agent.md
│   ├── system-planner/         # 系统规划师
│   │   └── agent.md
│   ├── reviewer/               # 方案评审员
│   │   └── agent.md
│   └── docs/                   # 文档处理员
│       └── agent.md
└── skill/                      # Skill 定义
    ├── docs-reader/            # 文档读取技能
    │   └── SKILL.md
    ├── document-parser/        # 文档解析技能
    │   └── SKILL.md
    ├── markdown-converter/     # Markdown转换技能
    │   └── SKILL.md
    └── drawio/                 # Drawio图表生成技能
        └── SKILL.md
```

## Agent 列表

| 英文标识 | 中文名 | 职责 | 输出位置 |
|----------|--------|------|----------|
| `search` | 搜索研究员 | 搜索和信息收集任务 | `.ai/.tmp/ai/` |
| `engineering` | 工程协调者 | 协调多个子代理完成复杂任务 | `.ai/.tmp/ai/` |
| `system-planner` | 系统规划师 | 功能分解、需求分析、API设计、技术架构设计 | `.ai/.tmp/ai/` |
| `project-reviewer` | 方案评审员 | 审查工作提案和输出内容 | `.ai/.tmp/ai/` |
| `project-docs` | 文档处理员 | 读取各种格式文件，转换为Markdown | `.ai/docs/ai/` |

## Skill 列表

| 名称 | 描述 |
|------|------|
| `docs-reader` | 读取各种格式的文件（代码、配置文件、文档等） |
| `document-parser` | 解析文档结构，提取标题、列表、表格、代码块 |
| `markdown-converter` | 将各种格式转换为标准Markdown |
| `drawio` | 生成Drawio格式的图表 |

## Rules 列表

| 名称 | 描述 |
|------|------|
| `agent-rules` | Agent通用规则，包含中间文件规则、输出目录规则、文件命名规范 |

## 使用方式

### 在本项目中使用

1. **Agent**：直接在 Trae 中使用 `@Agent名` 调用
2. **Skill**：在提示词中使用 `invoke skill` 激活
3. **Rules**：Agent工作时应自动遵守规则定义

### 复制到其他项目

将整个 `harness` 目录复制到目标项目的 `.ai/` 目录下：

```bash
cp -r .ai/harness /path/to/other-project/.ai/
```

复制后更新 `.ai/AGENTS.md` 以索引新项目中的 Agent。

## 文件格式

### Agent 格式
- 文件名：`agent.md`
- 包含：中文名、英文标识、提示词、调用时机

### Skill 格式
- 目录名：技能名（kebab-case）
- 包含：`SKILL.md` + YAML frontmatter
- 遵循 Anthropic SKILL.md 规范

### Rule 格式
- 目录名：规则名（kebab-case）
- 包含：`SKILL.md` + YAML frontmatter

## 版本

- v1.0.0 - 初始版本
- 包含 5 个 Agent、4 个 Skill、1 个 Rule