# WinFrameShadow_CheckBoxCaptionFix_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-21
- 对象：`design/WinFrameShadow_CheckBoxCaptionFix_Design.md`（状态：待评审）——**#13**（WinFrame textShadow 色/偏移转发）、**#15**（CheckBox releaseCaption 顺序 + caption 文本/字号宿主持久化）、**#16**（确认闭环）
- 结论：**通过，放行实施**（4 个待审核问题答复见 §2，附 2 项补充覆盖请求见 §3）

## 1. 分项评审

### #15（CheckBox releaseCaption 顺序 + caption 文本/字号宿主持久化）✓

- **顺序修复**（保存 old 后 removeControl，对照 Button::setCaption 正确模式）✓。
- **文本/字号宿主持久化**（`m_captionText` 字段 + `createCaption` 统一应用 + parser/builder 路由）——**与 Button 模式一致** ✓，覆盖设计器"拖动后文本丢失"场景。
- **字号路由连带**（parser:1842/1846 直写 label font → `setCaptionSize`）：好的连带修复（防"拖动后字号回落"）✓。

### #13（WinFrame textShadow 四态 + text 三态转发 + get 读回）✓

- `setColorProperty` 补 textShadow 四态 + `getColorProperty` 读回（对照 `setTitleTextColor` 同步模式）✓。
- **状态可达性**沿用 P0-7 说明（WinFrame 交互不进 hover/pressed；色值转发完整、程序化 setState 可达）✓。
- offset 转发（上批已实施）本批核实回归 ✓。

### #16（CheckBox caption 字符串分发确认）✓

上批已实施（setString/getString + onPropertyChanged 布局刷新）——确认闭环 ✓。

## 2. 对 §7 待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | #15 范围（顺序 + 文本/字号持久化 + parser/builder 路由；字号路由连带防"拖动后字号回落"） | **确认** |
| 2 | #13 枚举范围（textShadow 四态 + text 三态 + get 读回） | **确认** |
| 3 | #16 确认闭环 | **确认** |
| 4 | 验证载体（test_colorfixes 扩展） | **确认** |

## 3. 补充覆盖请求（2 项，建议随批）

### 3.1 CreateCheckBox 工厂的 text 参数路由（文档未覆盖）

- **现状**：`UICornerstone_CreateCheckBox(instance, text, ...)` → `getCaption()->setCaption(text)`（**直写 label，不经宿主字段**）→ **recreate 后文本丢失**（与 parser/builder 缺口同源，设计文档 §2 已列 parser/Builder 但**工厂路径未列**）。
- **建议**：工厂路径同样经宿主字段（或 `m_captionText` 同步），与 parser/builder 路由修正对齐。
- **验收补**：`CreateCheckBox(inst, "T1", ...)` → `setRect` 触发 recreate → caption 文本仍 "T1"（与 #15-2 同断言）。

### 3.2 CheckBox caption 各态色的宿主持久化（关联 P0-5 直控路径）

- **现状**：设计器颜色组对 check-box 的 `text.*`/`textShadow.*` 各态色经 `GetPtr("caption-label")` **直控 caption 自身字段**——**CheckBox `setRect` recreate 后 caption 重建，直控色丢失**（回读回落缺省）。
- **建议**（与文本/字号持久化同模式）：CheckBox 宿主补 text/textShadow 四态字段的存储与应用（`createCaption` builder 应用宿主 4 态）；或明确该场景"色不持久"由应用重建后重设。
- **设计器现状**：normal 态可用（`GetBool/GetColor` 读宿主缺省生效路径）；各态色待上述任一路径。

## 4. 其余确认

- **WinFrame title Label 无重建路径**（§2 自查 ✓）——win-frame 无同型缺陷 ✓。
- **ColorPicker `recreateClosedState` remove 先于 reset** ✓（无同型缺陷）。
- **三后端无涉**、**hasCallback O(1)**（引用前批结论）✓。

## 5. 放行

**结论：放行实施**（4 项待审核确认 + §3 两项补充覆盖请求）。实施完成后同步 subModules，设计器随批联测：

1. **CheckBox 拖动**：无残留/无重影（#15-1）；文本/字号持久（#15-2/3）；
2. **WinFrame**：textShadow 四态 + text 三态 + offset 转发生效（#13-1/2）；
3. **颜色组全类型联测**（Button/CheckBox/WinFrame/Label 的 text/textShadow/background/border 各态）——颜色组补全闭环收尾（05/history 更新）。
