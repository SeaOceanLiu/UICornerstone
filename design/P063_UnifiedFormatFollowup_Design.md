# P063_UnifiedFormatFollowup_Design — 统一行式格式联测：反馈叠加/字体名 API/节点字体/段字体/字体枚举

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-03 追加：P0-63①~⑦）
> 前置：P0-62 批已实施并同步
> 状态：**已放行，已实施**（2026-10-03；P0-63⑥ 评审确认本期不做）
> 复核：`requirements/P063_UnifiedFormatFollowup_Design_复核意见.md`（通过；六问全部确认 + 1 处设计器报告更正[⑦]；⑥ 同意暂缓）
> 实施备注：① cell bg 行 hover/选中半透明叠加（alpha=ConstDef::LIST_HIGHLIGHT_OVERLAY_ALPHA 0.35，选中优先/disabled 不叠加/仅 bg alpha>0 单元格）；②③ CellStyle/HeaderStyle 增 hasFontName 稀疏 + `ListViewSetCellFontName`/`SetColumnHeaderFontName`（修复"仅设背景/色重置字体"）；④ `getNodeFont` 门槛修复（fontSize<=0 用控件级字号驱动）+ `TreeViewSetNodeFont`（getNodeFont 转 public 供测试）；⑤ `StatusItem.fontName/hasFontName` + fontForSize 键 (name,size) 缓存 + `StatusBarSetItemFontName` + JSON `font-name`；⑦ 文档清理无效字体示例（CABI_Property_Design/Menu_Enhancement/TreeView_Enhancement/PropertyNames 注释）；测试：test_listview 45/0、test_treeview P0-63b PASS、静态像素探针 6/6（hover/selected 叠加混合精确色）；回归仅 4 项预存失败

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-63① | 单元格背景设置后 hover/选中无视觉反馈 | **确认** | ListView 绘制顺序：行高亮（selected/hover 实色填充）→ **单元格背景后绘覆盖其上**（ListView.cpp:676-680"覆盖高亮之上"）→ 设了 cell bg 的行高亮被完全盖住 |
| P0-63② | 单元格字体名有字段无 API | **确认（含潜在覆盖缺陷）** | `CellStyle.fontName` 有字段且 draw 使用，但 `SetCellStyle`（bg+fontSize）、`SetCellTextColor`（色）均不含名称 → 无 setter；且 draw `fn = cs.fontName` **无条件覆盖**（style 仅设背景时字体被重置为默认 regular）——需 `hasFontName` 稀疏 |
| P0-63③ | 列头字体名有字段无 API | **确认（同族缺陷）** | `HeaderStyle.fontName` 有字段；`SetColumnHeaderStyle`（color+fontSize）无名称；`fontFor(st.fontName, …)` 仅以 fontSize>0 判定 → 仅设色/字号时也会用默认 regular → 需 `hasFontName` 稀疏 + 名称 API |
| P0-63④ | TreeView 逐节点字体：仅设名称不生效（缺陷）+ 专用 API | **确认** | `getNodeFont`（TreeView.cpp:111）：`if (!node \|\| node->fontSize <= 0) return m_font;` → 仅设 `fontName`（fontSize=0）永不生效（逐节点字体被整体跳过）；专用 API 缺失（需 `item-id` 两步定位） |
| P0-63⑤ | StatusBar 段级字体名（可选） | **确认现状** | `StatusItem` 无 `fontName`（P0-58 已有段级 fontSize）；`fontForSize` 固定用控件级 `m_fontName`，缓存键仅字号 |
| P0-63⑥ | ComboBox 逐项样式（用户问询） | **确认现状**（`ComboBoxItem` 仅 label/value/disabled）；**必要性建议见 §2.5（建议暂缓）** | 控件级下拉项色已具备（m_itemHoverColor/m_itemSelectedColor/m_itemDisabledColor，ComboBox.h:46+）；下拉自绘分派（ComboBoxListPanel::draw） |
| P0-63⑦ | schema fontName 28 token vs FontName 枚举 6 | **报告与仓库现状不符（已复核）** | 仓库与 subModules 的 `docs/schema/declarative-ui.schema.json` `$defs/font-name` **均为 6 项**（asul-bold/harmonyos×2/maplemono/muyao/quando，P0-48 扩展② 已收窄）；"28 token" 来源应为**历史设计文档示例**：`design/CABI_Property_Design.md:1110`、`Menu_Enhancement_Design.md:113`、`TreeView_Enhancement_Design.md:101`、`PropertyNames.h:564` 注释中的无效示例 `harmonyos-sans-sc-bold`（资源集无此字体）。`FontNameFromString` 静默回退属既定设计（P0-47 批复核确认） |

## 2. 修改方案

### 2.1 P0-63① 单元格背景与交互反馈并存（半透明高亮叠加）
- 机制：行高亮保留现状（非 cell-bg 行不变）；**含 cell 背景的单元格**在 cell bg 填充后，若该行 hover/selected → 以**半透明高亮色叠加**（同 rect）：
  - 叠加色 = 主题高亮 RGB + 固定 alpha（新增 `ConstDef` 常量，如 `kListHighlightOverlayAlpha = 0.35f`，按魔鬼数字规范常量化）；
  - 选中优先于 hover（与现判定一致）；disabled 行不叠加。
- 备选（不推荐）：cell bg 亮度调制（每态派生色，复杂且主题色不可控语义）。
- 效果：per-cell 着色与交互反馈并存（浅色 cell bg 上仍可见高亮）。

### 2.2 P0-63②③ ListView 单元格/列头字体名 API + hasFontName 稀疏
- `CellStyle.hasFontName` / `HeaderStyle.hasFontName`；draw 改判：`if (cs.hasFontName) fn = cs.fontName;`、`hf = fontFor(st.hasFontName ? st.fontName : m_fontName, …)`（未设 → 控件级，修复"仅设背景/色被重置 regular"）；
- API（独立保 ABI）：`ListViewSetCellFontName(lv,row,col,name)` / `ListViewSetColumnHeaderFontName(lv,col,name)` + Binding；
- 既有 `SetCellStyle`/`SetColumnHeaderStyle` 不置 hasFontName（不隐式覆盖控件级）。

### 2.3 P0-63④ TreeView 节点字体名生效 + 专用 API
- `getNodeFont` 修复：`fontSize<=0` 时以**控件级 `m_fontSize`** 驱动节点字体（名称生效、字号随控件级）：
  ```cpp
  int effSize = node->fontSize > 0 ? node->fontSize : static_cast<int>(m_fontSize);
  if (node->fontName == m_fontName && effSize == static_cast<int>(m_fontSize)) return m_font;
  … loadFontFromMemoryWithText(..., effSize * getScaleXX(), "W")
  ```
- 专用 API：`TreeViewSetNodeFont(tree, id, name, size)`（size<=0 = 继承控件级；设置后清 `m_nodeFonts`）+ Binding；
- 设计器过渡（名称单独设置时读控件级字号写入节点）随同步删除。

### 2.4 P0-63⑤ StatusBar 段级字体名（低优先，建议随批小实施）
- `StatusItem` 增 `FontName fontName` + `bool hasFontName=false`；
- `fontForSize` 扩展为按键 **(fontName, pixelSize)** 缓存（未设 → 控件级 m_fontName）；
- API `StatusBarSetItemFontName(bar,id,name)` + JSON items 键 `font-name` + Binding；
- 若评审认为需求弱可后置（Q-4）。

### 2.5 P0-63⑥ ComboBox 逐项样式——**必要性讨论（建议暂缓/不做）**
- **建议：本期不做**（若确需仅做最小子集，见下）。
- 理由：
  1. 需求来源为"用户问询"而非既定场景，无真实用例支撑；
  2. **控件级下拉项样式已完备**（item-hover / item-selected / item-disabled 色 + 控件级字体/字号），逐项差异着色在下拉选择器中属罕见诉求；
  3. 完整"统一属性序"（背景/字体色/字号/字体名/阴影/偏移）实现面大：`ComboBoxItem` 模型扩展 + 下拉自绘分派 + **hover/选中与逐项背景叠加语义**（P0-63① 同族）+ ABI/Binding/JSON/schema/测试全链；
  4. ListView/TreeView item 着色刚落地，跨控件"item 样式"抽象宜沉淀后按真实需求平移，避免提前泛化。
- 若评审/用户坚持支持，建议**最小子集**：`text-color`（字符串/四态）+ `background-color`（单色），hover/selected 采用半透明叠加（复用 §2.1 机制）；字体/字号/阴影/偏移后置。

### 2.6 P0-63⑦ 字体枚举不一致——现状更正确认
- 仓库与 subModules 的 schema 均为 **6 项**，无 28 token enum；`FontNameFromString` 静默回退为既定设计（资源集实际 6 字体）；
- 建议：① 设计器确认其 schema 来源（应取 subModules 同步版，勿用历史副本）；② 随批清理历史设计文档/注释中的无效字体示例（`harmonyos-sans-sc-bold` 等，改 `maplemono-nf-cn-regular` 或删除）——文档整改，非引擎功能。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | ① cell bg 行 hover/selected 可见高亮叠加（像素：设 cell bg 后 hover 行出现高亮差异） | 像素探针 |
| 2 | ②③ 字体名 API 生效 + 仅设背景/色不再重置字体（hasFontName 稀疏回归） | 引擎断言 + 像素 |
| 3 | ④ 节点仅设字体名生效（字号随控件级）；专用 API 写入即生效 | 引擎断言 + 像素 |
| 4 | ⑤ 段级字体名生效（若纳入） | 像素 + 引擎断言 |
| 5 | ⑦ 无效示例清理后文档无 `harmonyos-sans-sc-bold` 残留 | grep |
| 6 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **P0-63⑥ ComboBox 逐项样式**：同意**本期不做**（推荐）？或需最小子集（`text-color` + `background-color`）？
2. **P0-63①**：确认半透明高亮叠加方案（alpha 常量）？叠加范围仅 cell-bg 单元格（推荐）？
3. **P0-63②③**：`hasFontName` 稀疏修复（既有 SetCellStyle/SetColumnHeaderStyle 不再隐式覆盖字体）——确认？
4. **P0-63④**：`fontSize<=0` 用控件级字号驱动节点字体名——确认？专用 API `TreeViewSetNodeFont(name,size)`——确认？
5. **P0-63⑤**：段级字体名本批纳入或后置？
6. **P0-63⑦**：确认按"schema 已 6 项 + 清理历史示例"处理（无需引擎扩字体）？

## 5. 配套改动

- 代码：`ListView.h/.cpp`（hasFontName + 叠加高亮 + 2 API）、`TreeView.h/.cpp`（getNodeFont + API）、`StatusBar.h/.cpp`（②⑤ 若纳入）、`ConstDef.h`（alpha 常量）、`UICornerstoneAPI.h/cpp`、`binding/*`、`LayoutParser.cpp`（items `font-name`）；
- 文档：CABI/Binding 速查表 + listview/treeview/statusbar 页 + 历史设计文档无效字体示例清理；
- 测试：像素探针 + 引擎断言 + 回归；设计文档标注 + 复核归档 + make_release + 同步。
