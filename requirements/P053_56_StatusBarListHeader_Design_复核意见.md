# P053_56_StatusBarListHeader_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-02
- 对象：`design/P053_56_StatusBarListHeader_Design.md`（P0-53/54/55/56，状态：待评审）
- 结论：**通过，放行实施**（附 3 项实施注意：P0-55 未设态回退、schema 色键形态、文档笔误）

## 1. 评审要点

### 现状核实（§1）✓（源码抽验一致）

- P0-53：`updateStatusItemText` 仅改文本（StatusBar.cpp:46-50），add/remove/setItemSize 均 relayout——唯独文本遗漏 ✓。
- P0-54：Label 相对约定（`m_fontFile` = `fonts/…`）与 StatusBar/Menu/TabControl/TreeView/ListView 绝对约定（`pathPrefix + "/" + rel`）并存；内存 provider 精确键匹配 ✓。
- P0-55：`StatusItem` 无颜色字段；draw 恒用控件级四态色（StatusBar.cpp:188）✓。
- P0-56：`m_headerBgColor/m_headerTextColor` 内部常量、`HeaderStyle` 无背景/阴影字段 ✓。

### 方案（§2）✓

- **P0-53**：文本实际变化才重排（同文本写入不重排）——正确最小修复 ✓。
- **P0-54 主修**：统一为相对约定后，文件系统 provider（`basePath / path`）与内存 provider（相对键）**均正常**——方向正确；防御回退（前缀剥离重试）成本低、不改既有精确命中优先级 ✓。
- **P0-55**：稀疏语义（hasTextColor/hasBackground）+ 绘制（先背景后文本）+ ABI/JSON/Binding 齐备 ✓；背景铺满 hitRect（VSCode 风格）合理。
- **P0-56**：控件级 4+1 键 + per-column 稀疏扩展 + 绘制优先级（per-column → 控件级 → 内部常量）；ABI 与 `ListViewSetCellShadow` 同风格 ✓；schema 键建议（header-*）与 columns[] per-column 同名同义（作用域=列）清晰 ✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | P0-54 主修 + 防御回退两者都做？ | **确认两者都做**（主修解决根因，回退覆盖第三方绝对路径场景） |
| 2 | P0-55 text-color 双形态（字符串=normal/对象=四态）+ 背景仅单色？ | **确认** |
| 3 | P0-55 段背景铺满 hitRect（含内边距）？ | **确认**（VSCode 风格） |
| 4 | P0-56 控件级 header-text 单色（不做四态）+ per-column 同名同义？ | **确认** |
| 5 | P0-56 阴影 = 色 + 偏移两键（无 enabled，未设色=不绘制）？ | **确认**（对齐 SetCellShadow/text-shadow 家族） |

## 3. 实施注意（放行附项）

1. **P0-55 未设态回退（重要）**：`StateColor` 缺省构造为 `DEFAULT_NORMAL_COLOR (23,23,24)`——若 per-item 只设 normal（ABI `state=NULL` 是最常见路径），则 hover/pressed 时 `textColor[state]` 解析为**黑色**（"仅设 normal 致 hover 变黑"缺陷复发，设计器状态栏 bg/text 已踩过同坑）。建议回退链：**显式设过的态 → 该段 normal → 控件级 `resolveStateColor`**（实现可加 per-item 态掩码，或在 setter 中把未设态同步为 normal）。
2. **schema 色键形态**：新增色键（控件级 `header-text/header-background/header-shadow`、status-bar items 的 `text-color/background-color`、columns[] per-column 色键）请用 `{"$ref": "#/$defs/color"}`（`x-color` 标记）——设计器按 `x-color` 识别并渲染 **ColorPicker**；若用 `type:"string"` 会落入 string 分支（TextArea 手输 hex，体验不符）。
3. **文档笔误**：§2.3 ABI 签名 `UICornerstone_StatusBarSetItemTextColor(UIInstance, UICornerstone bar, …)` → 应为 `UIControlHandle bar`。
4. **设计器随批提醒**（同步后）：删 P0-53 临时段增删重排、删 P0-54 双键注册；status-bar 行式格式扩展 `文本|#RRGGBB[|#背景色]`；columns 行式格式扩展背景/阴影；新增 `x-color` 独立键 → ColorPicker 行支持。

## 4. 放行

**结论：放行实施**。四项现状核实与方案（重排修复/路径统一/分段着色/表头样式）质量高、边界（稀疏语义/绘制优先级/JSON 双形态）清晰；附 3 项实施注意（未设态回退/schema 色键形态/文档笔误）。实施完成后同步 subModules，设计器随批：删两处过渡代码 + 行式格式扩展 + x-color 行支持，端到端联测。
