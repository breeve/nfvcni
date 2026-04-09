---
name: agent-rules
description: Agent通用规则定义，定义Agent工作时需要遵守的规范。触发词：工作规范、输出规则、文件规则。
---

# Agent 通用规则

本文件定义了Agent在工作时应遵守的通用规则。

## 规则1：临时文件目录

**规则**：所有临时文件应保存到工程根目录的 `.tmp/ai/` 目录下

**说明**：
- 临时文件是指处理过程中的中间文件，非最终交付物
- 每个Agent在处理任务前，应先创建 `.tmp/ai/` 目录（如果不存在）
- 临时文件包括：审查输入、审查报告、临时分析文件等
- 任务完成后，根据需要决定是否清理临时文件

**示例**：
```
.tmp/ai/reviewer_input_20260101_120000.md
.tmp/ai/reviewer_report_20260101_120500.md
.tmp/ai/temp_analysis_20260101_121000.md
```

## 规则2：输出目录规则

**规则**：只有 Docs Agent 产生最终输出，其他 Agent 不产生最终输出

**说明**：
- **Docs Agent**：最终输出保存到 `docs/ai/` 目录
- **其他 Agent**（search, engineering, system-planner, reviewer）：仅输出临时文件到 `.tmp/ai/`，不产生最终输出
- 其他 Agent 如需输出审查结果或报告，也保存到 `.tmp/ai/`

**示例**：
```
# Docs Agent 最终输出
docs/ai/analysis-report.md
docs/ai/architecture-diagram.svg

# 其他 Agent 临时输出
.tmp/ai/reviewer_input_20260101_120000.md
.tmp/ai/analysis_result_20260101_121000.md
```

## 规则3：唯一文件名规则

**规则**：每个任务的输入输出文件应使用唯一文件名，避免覆盖

**说明**：
- 文件名格式：`任务类型_时间戳.md`（如 `review_input_20260101_120000.md`）
- 时间戳格式：YYYYMMDD_HHMMSS
- 任务类型：review_input, review_report, analysis, plan 等

**示例**：
```
# 审查任务
.tmp/ai/review_input_20260101_120000.md
.tmp/ai/review_report_20260101_120500.md

# 分析任务
.tmp/ai/analysis_20260101_121000.md
```

## 规则4：文件命名规范

- 使用英文命名
- 遵循 kebab-case（如 `analysis-report.md`）
- Agent定义文件：`agent.md`
- Skill定义文件：`SKILL.md`
- 规则定义文件：`SKILL.md`