---
name: markdown-converter
description: 将各种格式转换为标准Markdown输出。处理代码高亮、表格转换、链接修复、图片路径等。当需要生成Markdown文档、统一文档格式、导出为Markdown时使用此技能。触发词：转换为Markdown、生成Markdown、导出Markdown、Markdown转换。
---

# Markdown转换技能

此技能用于将各种格式的文档转换为标准Markdown格式输出。

## 转换能力

### 1. 源代码到Markdown
转换各种编程语言的源代码为Markdown代码块：

**支持的语言标识**：
- Python → python
- JavaScript/TypeScript → javascript/typescript
- Java → java
- Go → go
- Rust → rust
- C/C++ → c/cpp
- C# → csharp
- Ruby → ruby
- PHP → php
- Swift → swift
- Kotlin → kotlin
- SQL → sql
- Shell → bash/shell
- JSON/YAML → json/yaml

### 2. 表格转换
将不同格式的表格数据转换为Markdown表格：

**支持格式**：
- CSV → Markdown表格
- HTML表格 → Markdown表格
- 制表符分隔 → Markdown表格
- JSON数组 → Markdown表格

### 3. 链接处理
处理和修复各种链接：

- 相对路径转绝对路径
- 图片路径修复
- 锚点链接处理
- URL编码处理

### 4. 图片处理
处理图片引用：

- 本地图片路径转换为相对路径
- 生成图注
- 支持的图片格式：png, jpg, jpeg, gif, svg, webp

### 5. 格式规范化
统一文档格式：

- 标题层级规范化
- 列表缩进统一
- 代码块语言标识
- 脚注格式标准化

## 转换规则

### 代码块处理
```
原格式：
```python
def hello():
    print("Hello")
```

输出：
```python
def hello():
    print("Hello")
```
```

### 表格处理
```
CSV输入：
name,age,role
Alice,25,Developer
Bob,30,Designer

Markdown输出：
| name | age | role    |
|------|-----|---------|
| Alice| 25  | Developer|
| Bob  | 30  | Designer |
```

### 标题处理
- H1使用单行#标记
- H2使用双行##标记
- 标题后不添加额外空行
- 保持原始标题文字

## 使用方法

### 基础转换
```
转换为Markdown：/path/to/source.txt
```

### 指定输出
```
转换为Markdown并保存为：output.md
```

### 批量转换
```
转换整个目录：/path/to/directory/
```

## 输出要求

### 格式标准
- 使用GFM (GitHub Flavored Markdown)
- 代码块必须有语言标识
- 表格使用标准格式
- 列表使用-或1.标记

### 元数据
输出包含：
- 源文件类型
- 转换时间
- 文件大小
- 字符统计

## 错误处理

- 不支持的格式：提示支持的格式列表
- 转换失败：返回原始内容
- 编码问题：尝试多种编码后报告
- 大文件：分块处理并合并