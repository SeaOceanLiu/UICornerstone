# P066_ListViewJson_Design — ListView JSON 导入缺口（列头字号/字体名 + 单元格对象样式）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-04 追加：P0-66①②，导入/导出 JSON 缺口）
> 前置：P0-62~P0-65 批已实施并同步
> 状态：**已放行，已实施**（2026-10-04）
> 复核：`requirements/P066_ListViewJson_Design_复核意见.md`（通过；Q1 收窄为 `header-font-size` 单键，无别名）
> 实施备注：① columns 增 `header-font-size`（单键，无别名）+ `header-font-name` 解析；② 对象 cell 解析（text/background-color 串或对象 normal+hover/hover-background-color 别名/text-color/font-size/font-name/text-shadow+偏移 → 粒度写入；字符串兼容；对象 cell 不再读空串）；③ 粒度方法 `setCellBackgroundColor/setCellFontSize/setCellShadow`（保留字段、不触发 hasBg）+ `ListViewSetCellShadow` 改走粒度路径（**修复 P0-65① 引入的"仅阴影常态填黑"回归**）；schema columns 两键 + rows description；测试：test_listview 55/0（含 shadow-only 回归）、P0-66 解析/像素探针 12/12、样例 `layouts/p066_listview.json`（ctest strict 9/9）；回归仅 4 项预存失败

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-66① | columns 缺列头字号/字体名 JSON 解析 | **确认** | `parseListView` columns 循环（LayoutParser.cpp:659-685）仅读 title/width/sortable + `header-text`/`header-background`/`header-shadow`(+偏移)；`HeaderStyle.fontSize/fontName/hasFontName` 字段与 API（`setColumnHeaderStyle`/`setColumnHeaderFontName`）齐备，**仅缺 JSON 入口**；schema columns items 亦无对应键 |
| P0-66② | rows cells 缺单元格样式 JSON 解析 | **确认（含对象 cell 读空串缺陷）** | rows 解析（LayoutParser.cpp:693-700）：`cells.push_back(c.is_string() ? c.get<string>() : string())` → **对象 cell 被读成空串**；`CellStyle`（bg/hoverBg/textColor/fontSize/fontName/shadow）无任何 JSON 入口（styles 仅 API） |
| **P0-66③** | **附带回归（P0-65① 引入，联测发现）** | **确认** | `UICornerstone_ListViewSetCellShadow`（UICornerstoneAPI.cpp:1903-1915）经 `getCellStyle` → 修改 → `setCellStyle` 回写；P0-65① 后 `setCellStyle` **强制 `hasBg=true`** → **仅设阴影的单元格常态填黑**（默认 CellStyle.bgColor 黑 α=1）。需粒度化引擎方法修复 |

**导出侧备注（设计器随批）**：模型键 `header-color` ↔ 引擎 `header-text` 映射转换；画布尺寸经顶层 `viewport` 键（引擎已支持 ✓）。

## 2. 修改方案

### 2.1 P0-66① columns：列头字号/字体名
- columns 元素新增解析：
  - `header-font-size`（number → `HeaderStyle.fontSize`，>0 生效；**兼容别名 `header-size`**，引擎族一致优先——Q-1）；
  - `header-font-name`（string → `fontName` + `hasFontName=true`）；
- 解析路径：`HeaderStyle st = getColumnHeaderStyle(ci)` → 赋值 → `setColumnHeaderStyle(ci, st)`（既有 header-text 同款）；
- schema：columns items 增 `header-font-size`（number）+ `header-font-name`（string）。

### 2.2 P0-66② rows cells 对象形态
- cells 元素支持**对象**（字符串保持兼容）：
  ```json
  { "text": "值",
    "background-color": "#112233" 或 { "normal": "#112233", "hover": "#445566" },
    "hover-background-color": "#445566",          // 可选别名（与对象 hover 等价）
    "text-color": "#dd2222",
    "font-size": 14, "font-name": "maplemono-nf-cn-regular",
    "text-shadow": "#000000", "text-shadow-offset-x": 2, "text-shadow-offset-y": 2 }
  ```
  映射：`background-color` 字符串 → `bgColor + hasBg`；对象 → normal→`bgColor+hasBg`、hover→`hoverBgColor+hasHoverBg`（pressed/disabled cells 不支持，忽略）；`text-color` 字符串 → `textColor+hasTextColor`（cells 单色）；`font-size/font-name/text-shadow(+偏移)` 对应字段（font-name 置 `hasFontName`；shadow 置 `shadowEnabled`）；
- **键形态与 statusbar/tree items 家族一致**（`background-color`/`text-color`/`font-name`/`text-shadow`/`text-shadow-offset-x/y`，常量已齐；别名 `hover-background-color` 新增常量若采纳——Q-2）。

### 2.3 P0-66③ 引擎粒度化（修复附带回归，同时服务解析）
- ListView 新增引擎方法（精确保留既有字段、不触发 `hasBg`）：
  ```cpp
  void setCellBackgroundColor(int row, int col, SColor color);   // bgColor + hasBg
  void setCellFontSize(int row, int col, int size);              // fontSize
  void setCellShadow(int row, int col, SColor color, float ox, float oy);   // shadowColor + offsets + shadowEnabled
  ```
- `UICornerstone_ListViewSetCellShadow` 改走 `setCellShadow`（修复"仅设阴影常态填黑"）；`UICornerstone_ListViewSetCellStyle` 仍为常态背景全量入口（hasBg 语义不变）；
- parser cells 解析改用粒度方法组合写入（不再经 `setCellStyle`，避免 hasBg 误置）。

### 2.4 schema
- list-view columns items：+`header-font-size`、`header-font-name`；
- rows description 更新：`[{id,cells[](字符串或对象样式),cellControls}]`（元素宽松，不设 additionalProperties）。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | columns `header-font-size`/`header-font-name` 解析 → 引擎字段/绘制生效 | 引擎断言 + 像素 |
| 2 | 对象 cell 解析（text/背景（串+对象四态）/悬停背景/文字色/字号/字体名/阴影+偏移）→ CellStyle 全字段 + 稀疏位正确 | 引擎断言（getCellStyle） |
| 3 | 字符串 cell 兼容不变；对象 cell 不再读成空串 | 引擎断言 |
| 4 | `ListViewSetCellShadow` 仅设阴影 → 常态不填黑（附带回归修复） | 引擎断言 + 像素 |
| 5 | schema ctest strict（样例布局含新键） | tools ctest |
| 6 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **① 键名**：`header-font-size`（引擎族一致，推荐）+ `header-size` 兼容别名？或仅用 `header-size`？
2. **② 背景键形态**：`background-color` 对象四态（主）+ `hover-background-color` 别名（兼容）都支持？`text-color` 保持字符串单色（cells 无态）确认？
3. **③ 粒度引擎方法**（不动 ABI 签名，`SetCellShadow` 内部改调用；parser 组合式写入）确认？
4. **schema**：columns 两键 + rows 宽松 description 更新确认？

## 5. 配套改动

- 代码：`LayoutParser.cpp`（columns 两键 + cells 对象解析）、`ListView.h/.cpp`（粒度方法 + SetCellShadow 路径）、`PropertyNames.h`（别名键若采纳）、`UICornerstoneAPI.cpp`（SetCellShadow 改走粒度方法）；
- schema：columns items + rows description；
- 测试：解析断言/像素/`SetCellShadow` 回归 + 样例布局（ctest strict）+ 全量回归；
- 文档：listview 页（columns/cells JSON 键表）+ capi 速查（如需）+ 设计文档标注 + 复核归档 + make_release + 同步。
