# P064_HoverFeedback_Design — TreeView/StatusBar 背景叠加缺陷 + 跨控件 hover 彩色可配

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-04 追加：P0-64①/②）
> 前置：P0-63 批已实施并同步（含 ListView 叠加机制与 alpha 常量）
> 状态：**已放行，已实施**（2026-10-04；含用户拍板撤销暂缓的 ③④）
> 复核：`requirements/P064_HoverFeedback_Design_复核意见.md`（两轮均通过；①~④ 全部确认；ComboBox 维持暂缓）
> 实施备注：① TreeView/StatusBar 背景行 hover/选中半透明叠加（复用 alpha 常量；disabled 不叠加；TreeView bgMask 按状态位豁免——对象/专用路径置位，pressed 同豁免）；② ListView/StatusBar 复用通用 `hover`/`selected` 键（ListView 零新键、StatusBar `hover` 成员缺省 36,142,222）；③ `CellStyle.hoverBgColor/hasHoverBg` + `ListViewSetCellHoverBackgroundColor`（selected 优先、无常态 bg 可用）；④ `StatusItem.hoverBackground/hasHoverBackground` + `StatusBarSetItemHoverBackgroundColor` + TreeView `setNodeHoverBackgroundColor` 专用 API；JSON：TreeView/StatusBar `background-color` 对象四态解析；附带修复：TreeView `hitTestRow` 改用 `getDrawRect()`（免首绘缓存依赖）+ selected 行背景按 Normal 解析（显式 hover 色不渗入选中）；测试：test_listview 50/0、test_treeview/test_statusbar P0-64b PASS、静态像素探针 11/11（显式原色/叠加混合/selected 优先精确色）；回归仅 4 项预存失败

---

## 1. 问题确认（源码核实 + 五控件盘点）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-64① | TreeView 节点设背景后 hover/选中无反馈 | **确认** | TreeView.cpp:234-243：`if (hasBgStyle) { fill bg } else if (selected) … else if (hover) …` —— 节点背景**替换**高亮（与 P0-63① ListView 同族，P0-62② 拆分 bg/边框时未做叠加） |
| （新发现） | StatusBar 段背景同上 | **确认（同类缺陷）** | StatusBar::draw：hover 高亮先绘（:273-276），**段背景循环后绘覆盖**（P0-55 段循环内 fill）→ 设段背景后 hover 无反馈 |
| P0-64② | 逐项/控件级 hover 彩色体验（跨控件） | **盘点如下** | — |

**五控件 hover/选中 配置现状盘点**：

| 控件 | 控件级 hover/选中 配置 | 通道 | 逐项 hover 覆盖 | 缺口 |
|---|---|---|---|---|
| TreeView | ✓ `setHoverColor/setSelectedColor` | 属性键 **`hover`/`selected`**（kTreeHover/kTreeSelected，TreeView.cpp:961-962） | node bg 四态（对象路径 `item-background` 可显式态） | ① 叠加缺陷（本批修）；JSON `background-color` 仅单色（无显式态） |
| ListView | 成员存在（m_hoverColor/m_selectedColor） | **无属性通道** ✗ | cell bg 单色；P0-63① 叠加已提供反馈 | 补 `hover`/`selected` 键通道 |
| ComboBox | ✓ item 三色 | 属性键 **`item-hover`/`item-selected`/`item-disabled`**（ComboBox.cpp:946-948） | ComboBoxItem 无逐项（P0-63⑥ 暂缓） | **无需**（hover 彩色由控件级键覆盖）；逐项按用户点名见 Q-3 |
| StatusBar | 常量 kHoverColor(36,142,222) | **无属性通道** ✗；且 hover 被段 bg 覆盖 | 段 bg 单色（无 state） | 补 `hover` 键 + 叠加修复 |
| Menu | ✓ bar `background.hover`（P0-33）+ MenuPanel item `hover`（Menu.cpp:666/680，MenuBar 传播） | 属性键 `background.hover` | 无逐项（未要求） | **无需** |

**结论**：② 的真实缺口 = ListView 缺控件级通道、StatusBar 缺通道+叠加、TreeView 缺 JSON 显式态；Menu/ComboBox 已可配（ComboBox 逐项为 ⑥ 暂缓项，需用户/设计器再次确认）。

> **增补（2026-10-04 用户拍板）**：**撤销**"ListView cell / StatusBar 段显式 hover 色暂缓"——用户明确要求逐项显式 hover 色，**P0-64③④ 纳入本批**（§2.3 已更新）；设计器行式格式新增键前缀 **`hb#`（悬停背景色）**（TreeView 节点 / ListView 单元格 / StatusBar 段），`ht#` 暂不加。

## 2. 修改方案

### 2.1 P0-64①（含 StatusBar 同类）背景与反馈叠加
- **TreeView**（与 P0-63① ListView 同机制）：
  - `hasBgStyle` 行：填 bg 后，若该行 hover/selected → 半透明高亮叠加（复用 `ConstDef::LIST_HIGHLIGHT_OVERLAY_ALPHA`；选中优先 hover；disabled 不叠加）；
  - **显式态豁免（稀疏协同）**：节点对象路径设有 hover 态背景（新增 `TreeNode.bgMask`，仅对象/显式态路径置位）→ hover 时使用显式色、不再叠加 tint（避免洗色）；选中仍叠加（区分选中与悬停）；
  - 单色四态同色（bgMask=0）→ 沿用叠加（修复反馈缺陷）。
- **StatusBar**：段背景填于段循环内 hover 高亮之后 → 改为「段 bg 填完后，若该段为 m_hoveredItem → 以 `m_hoverColor` 半透明叠加」（同机制；控件级 hover 色见 2.2）。

### 2.2 P0-64② 控件级通道补齐（零新键，复用通用名）
- **ListView**：`setColorProperty/getColorProperty` 增 `hover`/`selected`（kTreeHover/kTreeSelected 同名通用键）→ `m_hoverColor/m_selectedColor`；
- **StatusBar**：增 `hover` 键 → 新成员 `m_hoverColor`（缺省=现常量值 36,142,222）；get 对称。

### 2.3 P0-64②③④ 逐项显式 hover 背景（稀疏家族；③④ 已撤销暂缓、纳入本批）
- **通用绘制规则（三控件一致）**：显式 hover 背景（`hasHoverBg` 且该行/段处于 hover）→ **使用显式色、不叠加**；未设 → 走叠加机制（ListView P0-63① / TreeView / StatusBar 本批）；**选中态**：不在本批显式化（`hb#` 仅悬停），selected 继续叠加；
- **P0-64③ ListView 单元格**：`CellStyle` 增 `SColor hoverBgColor; bool hasHoverBg = false;`；API `ListViewSetCellHoverBackgroundColor(lv,row,col,rgba)`（独立函数保 ABI）+ Binding；draw：**仅 hoverBg（无常态 bg）也可用**（常态不填、悬停填显式色）；
- **P0-64④ StatusBar 段**：`StatusItem` 增 `SColor hoverBackground; bool hasHoverBackground = false;`；API `StatusBarSetItemHoverBackgroundColor(bar,id,rgba)` + Binding；draw：段 bg 后若该段 hovered → 显式色（有）或叠加 tint（无）；
- **TreeView**：① JSON items `background-color` 支持**对象四态**（per-state 掩码，复用 `text-color` 形态）→ 显式 hover 色走 §2.1 豁免；② 为对齐 `hb#` 行式接线，建议同批增加 `TreeViewSetNodeHoverBackgroundColor(tree,id,rgba)`（等价 `bgColor.hover` + `bgMask` 置位）——见 Q-5；
- **JSON 键形态（引擎定形提案）**：items 类（TreeView/StatusBar）用 **`background-color` 对象四态**（与 `text-color` 家族一致，避免新增 `hover-background-color` 单键）；ListView rows cells 目前**无 JSON 样式解析**（styles 仅 API），本批维持 API-only（如需 JSON 单元格样式另行立项）；
- **ComboBox 逐项（继续暂缓，Q-3）**：`item-hover` 控件级键已覆盖 hover 彩色体验；逐项样式仅按用户真实场景再立项（P0-63⑥ 结论维持）。

### 2.4 设计器随批
- 行式格式 hover 字段：TreeView 节点背景四态对象接线；其余控件 hover 体验经控件级 `hover`/`selected` 键（已在格式内）——检查清单待同步后确认。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | TreeView 节点单色 bg 行 hover/选中可见叠加（像素：blend 精确值） | 静态像素探针（复用 P0-63 模式） |
| 2 | TreeView 显式 hover 态 bg → hover 原色（不叠加）；选中仍叠加 | 像素探针 |
| 3 | StatusBar 段 bg + hover 可见叠加 | 像素探针 |
| 4 | ListView/StatusBar `hover`/`selected` 键读写回 | C ABI 断言 |
| 5 | TreeView JSON `background-color` 对象四态解析（掩码） | 引擎断言 |
| 6 | ③ cell 显式 hover 色：hover 显式色（非叠加混合）；未设继续叠加；仅 hoverBg（无常态 bg）可用 | 静态像素探针 |
| 7 | ④ 段显式 hover 色：hover 显式色；未设继续叠加 tint | 静态像素探针 |
| 8 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **叠加豁免规则**：显式 hover 背景（③④/TreeView 对象态）→ hover 原色不叠加、**选中仍叠加**——确认？
2. **API 形态**：三控件均用独立函数 `Set…HoverBackgroundColor`（保 ABI 清晰；不采用现有背景 API 加 state 变体）——确认？
3. **ComboBox 逐项样式**：维持 P0-63⑥ 暂缓（控件级 `item-hover` 已覆盖 hover 彩色体验）？或本次用户点名需最小子集（`text-color`+`background-color`）？
4. **键名**：ListView/StatusBar 控件级 hover/selected 复用通用名 `hover`/`selected`（与 TreeView 一致）——确认？
5. **TreeView 专用 hover API**：为对齐 `hb#` 行式接线，同批增加 `TreeViewSetNodeHoverBackgroundColor`（推荐）？或 TreeView 走 JSON 对象/通用 item-background 通道即可？
6. **JSON 键形态**：items 类用 `background-color` 对象四态（推荐，免新键）；ListView cells 维持 API-only（无 JSON 样式）——确认？

## 5. 配套改动

- 代码：`TreeView.h/.cpp`（叠加 + bgMask + JSON 对象 + 专用 hover API 若采纳）、`StatusBar.h/.cpp`（叠加 + hover 键/成员 + 段 hoverBg/API）、`ListView.h/.cpp`（hover/selected 键 + cell hoverBg/API）、`UICornerstoneAPI.h/cpp`、`binding/*`、`LayoutParser.cpp`（background-color 对象）；
- 测试：静态像素探针（TreeView/StatusBar 叠加与豁免）+ C ABI 断言 + 回归；文档（CABI 速查表 + treeview/statusbar/listview 页）；设计文档标注 + 复核归档 + make_release + 同步。
