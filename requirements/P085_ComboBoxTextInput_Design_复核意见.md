# P085_ComboBoxTextInput_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-10
- 对象：`design/P085_ComboBoxTextInput_Design.md`（P0-84 根因定案，状态：待评审）
- 结论：**通过，放行实施**（根因与设计器全部实测证据链吻合，逐项复核确认）

## 1. 根因确认——✓（与设计器证据逐条对齐）

引擎定位：`ComboBox::handleEvent` 非编辑模式对 TextInput **无条件 `return true`**（src/ComboBox.cpp:157-160；`m_editable` 缺省 false）→ `ControlImpl::handleEvent` 逆序子遍历下，**同容器中位于聚焦 EditBox 之后的默认 ComboBox 先吞事件** ✗。设计器侧逐条验证吻合：

| 设计器实测 | 与根因对齐 |
|---|---|
| 画布行 id/标题内联输入失效（光标/焦点正常、无字符、无状态提示） | 画布行集**构建序 = 类型 → id(EditBox) → 标题(EditBox) → 缩放模式(ComboBox) → 背景色**——ComboBox 在 EditBox 之后，逆序先见 → 吞 ✓ |
| 控件 id 行正常（同面板/同容器/同 API/同回调） | 控件行组序 = 背景色/边框色/字体及阴影/常规——**字体 ComboBox 在 id 行之前**（逆序下 EditBox 先见）→ 正常 ✓ |
| 画布设置对话框正常 | 对话框仅两个 EditBox、**无 ComboBox** → 正常 ✓ |
| "池复用传染"（共享实验连累控件行） | 行重建改变 child 顺序，ComboBox 落到 EditBox 之后即吞 ✓ |
| 冷启动仍失效；排除 activeViewport/创建位置/空值/手柄 | 顺序恒定 → 与启动路径无关 ✓；手柄越界为独立问题（已修）✓ |

- 引擎 p084c 合法拓扑探针（TabControl/树/池、无 ComboBox）全过 → TabControl/树/池均无关 ✓，与设计器"逐行对比无差异（仅顺序）"完全一致 ✓。

## 2. 修法复核——✓

- `return getFocused();`（仅自身聚焦时消费）✓——保留原意图（防只读显示文本被写），未聚焦放行链上其它控件 ✓；
- Q-2 同类审计：`NumericUpDown` readOnly TextInput 返回 false ✓、`EditBox/TextArea` 查 `m_focused` ✓、`MenuBar` watcher 仅 KeyDown/MouseDown ✓——**仅 ComboBox 需修** ✓ 确认；
- 不改编辑模式 ComboBox（转发内部 EditBox）与点击/下拉行为 ✓。

## 3. 测试计划补充（设计器侧）

- 设计器联测即用户复现步骤：画布选中 → 内联 id/标题**直接输入** → 字符出现、状态提示出现、模型/导出同步；
- 重点回归：**缩放模式 ComboBox 自身**点击展开/选择正常（修复后其聚焦消费语义不变）；控件行、背景色行、对话框不受影响；
- 设计器随批：`DebugValidateHandle` 行池自检保留（长期）；"画布设置…"对话框保留为便捷入口（不再必需）；过渡伪只读已全部清退。

## 4. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | 修法 `return getFocused()` | **确认** |
| Q-2 | 仅 ComboBox 需修 | **确认**（审计结论一致） |
| Q-3 | 测试矩阵 | **确认**（建议补"多 ComboBox 与 EditBox 全排列"已列 ✓；设计器侧补画布行直编复测） |
| Q-4 | P0-84 收尾（a/b/c 硬化保留、本项结案） | **确认**（`Debug_ValidateHandle` 作为长期自检保留） |

## 5. 放行

**结论：放行实施**。根因定案与设计器全部实测吻合（含"唯一变量=顺序"这一关键推论）；修法最小且保留原语义；落地后设计器画布行内联输入应即通，P0-84 全链结案。
