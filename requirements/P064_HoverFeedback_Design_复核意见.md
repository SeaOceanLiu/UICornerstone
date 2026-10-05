# P064_HoverFeedback_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-04
- 对象：`design/P064_HoverFeedback_Design.md`（P0-64①/②，状态：待评审）
- 结论：**通过，放行实施**（附 2 项实施注意：豁免按状态位扩展至 pressed；设计器侧 hover 字段格式说明）

## 1. 评审要点

### P0-64① TreeView 叠加 ✓（独立核实）

- `TreeView.cpp:234-243`：`if (hasBgStyle) { fill bg } else if (selected) … else if (hover) …`——节点背景替换高亮属实 ✓；P0-63① 同机制叠加方案正确。

### 新发现：StatusBar 段背景覆盖 hover ✓（价值高）

- 抽验属实：hover 高亮先绘（:273-276）→ 段背景循环后绘覆盖 → 设段背景后 hover 无反馈。同类缺陷，纳入本批正确。

### 五控件盘点抽验 ✓（全部属实）

| 控件 | 引擎盘点 | 抽验 |
|---|---|---|
| TreeView | `hover`/`selected` 键（kTreeHover/kTreeSelected，TreeView.cpp:961-962） | ✓ 常量值即通用词 "hover"/"selected" |
| ComboBox | `item-hover`/`item-selected`/`item-disabled`（ComboBox.cpp:946-948） | ✓ kItemHover 等（PropertyNames.h:49-51） |
| Menu | `hover` + `text-hover`（Menu.cpp:666/680） | ✓ m_hoverColor/m_hoverTextColor 可设 |
| ListView | 成员存在、无属性通道 | ✓ |
| StatusBar | 常量 kHoverColor、无通道 + 覆盖缺陷 | ✓ |

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 叠加豁免规则（显式 hover 态 → 不叠加；选中仍叠加）？ | **确认，建议按状态位扩展**：显式设过 **hover 位** → hover 不叠加；显式设过 **pressed 位** → pressed 不叠加（同一 4 态通道，否则 pressed 洗色）；selected 无显式通道（4 态不含 selected）→ 恒叠加（区分选中）；disabled 不叠加 ✓ |
| 2 | per-item 覆盖范围（TreeView 对象四态本批；ListView cell/StatusBar 段暂缓）？ | **确认**：TreeView JSON `background-color` 对象四态纳入本批；ListView cell / StatusBar 段显式 hover 色暂缓（控件级通道 + 叠加已覆盖体验） |
| 3 | ComboBox 逐项维持暂缓？ | **确认维持暂缓**：控件级 `item-hover`/`item-selected` 已提供 hover 彩色体验（属性面板可配）——用户点名诉求已由控件级通道满足；逐项待真实场景再立项 |
| 4 | 键名复用通用 `hover`/`selected`？ | **确认**（与 TreeView 一致；常量值即通用词，零新键） |

## 3. 实施注意（放行附项）

1. **豁免掩码**：TreeView 对象/显式态路径置 `bgMask` 对应位（hover/pressed）；单色路径 mask=0 → 全态叠加 ✓；禁用态不叠加。
2. **设计器随批（供参考，无需引擎动作）**：
   - TreeView items 行式格式：显式 hover 色经**键前缀**解析（拟 `hb#` 悬停背景 / `ht#` 悬停字体色）→ 模型/JSON `background-color`/`text-color` 对象四态（复用现有 b#/t#/s# 前缀家族）；
   - ListView/StatusBar 控件级 `hover`/`selected` 新键：schema 同步后属性面板自动出现（colors 组），无需设计器代码改动；
   - StatusBar 段级显式 hover：随暂缓，行式格式不加段 hover 字段。
3. **叠加绘制顺序**：StatusBar 段 bg 叠加在段 bg 之后、leadingControl/文字之前（避免文字被洗色）——与 ListView P0-63① 一致。

## 4. 放行

**结论：放行实施**。① 缺陷（含新发现的 StatusBar 同类）确认准确；② 真实缺口提炼（ListView/StatusBar 缺通道、TreeView 缺 JSON 显式态）与"零新键"方案合理；盘点表可直接作为后续 hover 能力基线。实施完成后同步 subModules，设计器随批：tree 显式 hover 前缀接线 + 联测（叠加像素/豁免/键读写回）。

---

## 5. 增补（2026-10-04 用户拍板——撤销暂缓）

**背景**：§2 问题 2 答复中"ListView cell / StatusBar 段显式 hover 色暂缓"经**用户明确要求撤销**——逐项显式 hover 色纳入本批。请引擎在实施前补充设计（或随实施增补）：

1. **ListView 单元格显式 hover 背景色**：`CellStyle` 增 `hoverBgColor + hasHoverBg`（稀疏掩码，对齐 `hasTextColor`）；API（独立函数或带 state 变体，引擎定形）；绘制显式态优先、未设走 P0-63① 叠加；JSON `hover-background-color`（或对象四态）；Binding。selected 态可选（引擎评估）。
2. **StatusBar 段级显式 hover 背景色**：`StatusItem` 增 `hoverBackground + hasHoverBackground`；API；绘制显式态优先、未设走控件级叠加（本批机制）；JSON `hover-background-color`（或对象四态）；Binding。
3. **设计器格式定案**：行式格式新增键前缀 **`hb#`（悬停背景色）**——TreeView 节点 / ListView 单元格 / StatusBar 段按各引擎支持面接线；`ht#` 暂不加（用户拍板：一个 hover 前缀即可）。
4. 验收：像素探针（显式 hover 色 vs 未设叠加）+ C ABI 断言（字段/掩码）+ 回归。

同步清单已更新（`UICornerstone_配合修改清单_第三批.md` → P0-64③/④）。

---

## 6. 修订版复核（2026-10-04 第二轮）

- 对象：`P064_HoverFeedback_Design.md` 修订版（§1 增补说明 + §2.3 重写 + §3 验收 6/7 + §4 六问）
- 结论：**通过，放行实施**（附 1 项实施注意：TreeView 豁免按掩码状态位，含 pressed）

### 修订内容核对 ✓

- §2.3 通用绘制规则（显式 hover → 原色不叠加；未设 → 叠加；selected 本批不显式化、继续叠加）——与用户拍板一致 ✓；
- ③ `CellStyle.hoverBgColor/hasHoverBg` + `ListViewSetCellHoverBackgroundColor` + "仅 hoverBg（无常态 bg）也可用" ✓（绘制需不依赖常态 bg 存在——正常态 alpha 0 不填、悬停显式填）；
- ④ `StatusItem.hoverBackground/hasHoverBackground` + `StatusBarSetItemHoverBackgroundColor` ✓；
- TreeView JSON `background-color` 对象四态 + 建议专用 hover API（Q5）✓；
- JSON 形态：items 类对象四态、ListView cells API-only ✓（rows cells 本就无 JSON 样式解析，维持合理）。

### §4 六问答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 豁免规则（显式 hover 原色不叠加、选中仍叠加）？ | **确认**；附注意：TreeView 豁免**按 `bgMask` 状态位**判定——hover 位 → hover 不叠加；**pressed 位 → pressed 不叠加**（对象路径可显式设 pressed，否则洗色；③④ 的 `hb#` 仅置 hover 位） |
| 2 | 三控件均用独立函数 `Set…HoverBackgroundColor`？ | **确认**（保 ABI 清晰；不采用现有背景 API 加 state 变体） |
| 3 | ComboBox 逐项维持暂缓？ | **确认维持**（控件级 `item-hover` 已覆盖 hover 彩色体验） |
| 4 | 控件级键复用通用 `hover`/`selected`？ | **确认** |
| 5 | TreeView 专用 `TreeViewSetNodeHoverBackgroundColor`？ | **推荐采纳**：`hb#` 行式接线需要单次调用（JSON 对象/通用 item-background 两步链是声明式/通用通道，不便于命令式随批） |
| 6 | JSON `background-color` 对象四态 + ListView cells API-only？ | **确认** |

### 实施注意

1. **豁免掩码**：TreeView `bgMask` 按状态位置位（hover/pressed 各自独立豁免）；③④ 显式 hover 色为单字段（仅 hover 态，无掩码歧义）。
2. **③ 绘制**：cellStyles 条目存在但常态 bg alpha=0 → 常态不填（回退行/控件链）；hover 时 `hasHoverBg` → 显式填（优先于 P0-63① 叠加）；文字/字体未设仍走行/控件链。
3. **④ 绘制**：段 hover 检测 = `m_hoveredItem`；显式 hover 色填于段 bg 之后、leadingControl/文字之前（避免文字洗色）。
4. **设计器随批**（格式定案）：`hb#` 接线路径——TreeView 走专用 API（Q5 采纳后）、rows cells 走 `SetCellHoverBackgroundColor`、statusbar 段走 `SetItemHoverBackgroundColor`；控件级 `hover`/`selected` 经属性面板（schema 同步后自动出现）；模型 JSON 存储对齐引擎形态（tree/statusbar 的 `background-color` 对象四态）。

### 放行

**结论：修订版通过，放行实施**。③④ 纳入本批后方案完整；验收 6/7 覆盖显式/未设/仅 hoverBg 三情形。实施同步后设计器随批 + 端到端联测（`hb#` 三控件、叠加像素、豁免、键读写回）。
