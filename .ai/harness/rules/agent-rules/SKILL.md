---
name: agent-rules
description: Agent通用规则定义，定义Agent工作时需要遵守的规范。触发词：工作规范、输出规则、文件规则。
---

# Agent 通用规则

本文件定义了Agent在工作时应遵守的通用规则。

## 规则1：临时文件目录

**规则**：所有临时文件应保存到 `.ai/.tmp/ai/` 目录下

**说明**：
- 临时文件是指处理过程中的中间文件，非最终交付物
- 每个Agent在处理任务前，应先创建 `.ai/.tmp/ai/` 目录（如果不存在）
- 临时文件包括：审查输入、审查报告、临时分析文件等
- 任务完成后，根据需要决定是否清理临时文件
- 工作目录基于 `.ai/` 所在目录，即 `.ai/` 的父目录

**示例**：
```
# 如果 .ai/ 位于 /project/.ai/，则临时文件目录为
/project/.ai/.tmp/ai/reviewer_input_20260101_120000.md
/project/.ai/.tmp/ai/reviewer_report_20260101_120500.md
/project/.ai/.tmp/ai/temp_analysis_20260101_121000.md
```

## 规则2：输出目录规则

**规则**：只有 Docs Agent 产生最终输出，其他 Agent 不产生最终输出

**说明**：
- **Docs Agent**：最终输出保存到 `.ai/docs/ai/` 目录
- **其他 Agent**（search, engineering, system-planner, reviewer）：仅输出临时文件到 `.ai/.tmp/ai/`，不产生最终输出
- 其他 Agent 如需输出审查结果或报告，也保存到 `.ai/.tmp/ai/`

**示例**：
```
# Docs Agent 最终输出
.ai/docs/ai/analysis-report.md
.ai/docs/ai/architecture-diagram.svg

# 其他 Agent 临时输出
.ai/.tmp/ai/reviewer_input_20260101_120000.md
.ai/.tmp/ai/analysis_result_20260101_121000.md
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
.ai/.tmp/ai/review_input_20260101_120000.md
.ai/.tmp/ai/review_report_20260101_120500.md

# 分析任务
.ai/.tmp/ai/analysis_20260101_121000.md
```

## 规则4：文件命名规范

- 使用英文命名
- 遵循 kebab-case（如 `analysis-report.md`）
- Agent定义文件：`agent.md`
- Skill定义文件：`SKILL.md`
- 规则定义文件：`SKILL.md`

## 规则5：审查流程规则

**规则**：所有Agent的输出都需要经过Reviewer Agent审查通过后才能继续下一步

**说明**：
- 每个Agent完成工作后，其输出（临时文件、报告等）都需要提交给Reviewer Agent进行审查
- 审查内容：格式规范性、内容完整性、结构合理性
- 审查结果：
  - `approve`：审查通过，可以继续下一步
  - `needs_modification`：需要根据反馈进行调整后重新提交审查
  - `reject`：审查不通过，需要重新设计方案
- 只有审查通过后才能进入下一阶段工作
- 中间文件保存到 `.ai/.tmp/ai/` 目录供审查使用

**示例**：
```
# 审查流程
1. Agent完成工作，输出到 .ai/.tmp/ai/agent_output_20260101_120000.md
2. 提交给Reviewer Agent审查
3. 审查结果为approve -> 继续下一步
4. 审查结果为needs_modification -> 根据反馈修改后重新提交
5. 审查结果为reject -> 重新设计方案
```

## 规则6：超时复盘规则

**规则**：任务超时后需要进行复盘分析并记录问题，避免下次重复

**说明**：
- 在执行任务之前，Agent应评估预计需要的时间
- 如果任务超时，需要输出以下信息：
  - 超时原因分析：为什么会超时
  - 问题定位：是评估的执行时间有问题，还是Agent、工具、Workflow或其他问题
  - 改进建议：如何优化以避免下次超时
- 复盘记录保存到 `.ai/.tmp/ai/timeout_review_时间戳.md`
- 下次遇到类似任务时，参考之前的复盘记录进行优化

**复盘记录模板**：
```markdown
# 超时复盘记录

## 任务信息
- 任务类型：
- 预估时间：
- 实际耗时：
- 超时原因：

## 问题定位
- [ ] 评估时间不合理
- [ ] Agent能力不足
- [ ] 工具效率问题
- [ ] Workflow设计问题
- [ ] 其他原因：

## 改进建议
- 
```

**示例**：
```
# 超时复盘
.ai/.tmp/ai/timeout_review_20260101_120000.md
```