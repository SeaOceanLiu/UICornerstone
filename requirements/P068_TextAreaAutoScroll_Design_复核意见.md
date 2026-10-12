# P068_TextAreaAutoScroll_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-05
- 对象：`design/P068_TextAreaAutoScroll_Design.md`（P0-68，状态：待评审）
- 结论：**通过，放行实施**（Q-1 确认缺省 false；附 1 项设计器随批范围补充）

## 1. 评审要点

- §1 根因确认 ✓（与设计器核实一致：`m_autoScroll` 缺省 true、无 setter、`ensureCursorVisible` 双处覆盖）；行为细节（调用点覆盖/`setText` 不调用/`scrollToBottom` 公开）核实价值高 ✓。
- §2 方案 ✓：属性语义二分（日志式/编辑式）、通用 SetBool 直通（零 C ABI 新增）、不动既有逻辑 ✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | 缺省值：false（语义优先）vs true（零破坏） | **确认 false**：编辑式为 TextArea 通用语义（本缺陷正源于缺省 true）；仓库内无依赖缺省 true 者；日志式显式 `auto-scroll=true` 或 `scrollToBottom()`（文档化）。回落方案不采纳（设计器亦将显式设 false，双保险） |
| Q-2 | 属性名 `auto-scroll` / `kAutoScroll` | **确认** |
| Q-3 | false 下 `setText` 整体替换不自动到底 | **确认**（编辑式语义一致；文档标注） |
| Q-4 | 范围：仅属性暴露 + 缺省决策 | **确认** |

## 3. 设计器随批（补充范围）

1. **动态区全部 TextArea**（普通文本行 + 结构化 items/columns/rows 行）创建后显式 `SetBool("auto-scroll", false)`——**不依赖引擎缺省**（双保险，防引擎回落方案）。
2. **工具箱放置的 TextArea（tArea）**：`createViewportControl` 创建后同样 `SetBool("auto-scroll", false)`——使**设计出的应用**运行时 TextArea 为编辑式；应用作者可经属性面板（auto-scroll 行，schema 驱动自动出现）切日志式 ✓。
3. 加载路径（打开/导入）经模型写回自动携带该属性 ✓（属性面板 bool 行已有模型同步）。

## 4. 放行

**结论：放行实施**。属性暴露 + 缺省 false + 文档/测试计划完整；设计器随批覆盖"动态区 + 工具箱放置"两类创建路径。实施同步后设计器联测（动态区多行文本光标上移跟随；设计出的 TextArea 运行时编辑跟随）。
