# P062_CellNodeColor_Design — ListView 单元格文字色 API + TreeView 节点级着色通道

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-03 追加：P0-62①/②）
> 前置：P0-50~P0-61 批已实施并同步
> 状态：**已放行，已实施**（2026-10-03）
> 复核：`requirements/P062_CellNodeColor_Design_复核意见.md`（通过，放行实施；五问全确认 + 1 项实施注意）
> 实施备注：① ListView `CellStyle.hasTextColor` 稀疏 + 绘制改判（修复"仅设背景变黑字"）+ `ListViewSetCellTextColor` API/Binding；② TreeView 节点着色专用 API 3 个（直写字段 + hasStyle）+ `TreeNode.hasBgStyle/hasTextStyle/textColorMask` 稀疏与未设态回退（bg/阴影单色四态同色、text 未设态回退节点 normal）+ 背景/边框绘制独立稀疏 + JSON items 简写键解析（text-color 字符串/四态、background-color、text-shadow+偏移）+ schema description；③ 通用 item-* 属性链同步稀疏标记（单态 bg 四态同色、text mask）；④ 测试：test_listview 42/0、test_treeview 全 PASS（含 JSON 解析断言）、像素探针 5/5（单元格红字/节点背景精确色）；⑤ 回归仅 4 项预存失败；⑥ 文档：CABI/Binding 速查表 + listview/treeview 页

---

## 1. 问题确认（源码核实）

### P0-62① ListView 单元格文字色：**缺口确认（含附带缺陷）**
| 项 | 现状 | 位置 |
|---|---|---|
| 数据字段 | `CellStyle.textColor` **已有** | ListView.h:42-51 |
| 绘制支持 | **已生效**：`if (cs.textColor.alpha() > 0) tc = cs.textColor;`（cell > row > 控件级链） | ListView.cpp 单元格绘制段 |
| ABI setter | **缺失**：`ListViewSetCellStyle`（bg+fontSize）与 `ListViewSetCellShadow`（阴影色+偏移）均不含文字色 | UICornerstoneAPI.h:581-583 |
| **附带缺陷（联测发现）** | `CellStyle` **无 `hasTextColor` 稀疏标记**，而 `textColor` 缺省 `SColor`（alpha=1 黑）→ 现有 `SetCellStyle` 仅设背景时，draw 会采用默认黑覆盖行/控件级文字色（"设单元格背景 → 文字变黑"） | 同绘制段 + CellStyle 默认值 |

### P0-62② TreeView 节点级着色：**能力已存在（报告前提需更正）；缺口为"专用 API + JSON 键"**
- **运行时能力已完备**（泛型属性链）：`SetString(tree,"item-id",id)` 定位后：
  - `item-background` / `item-border` / `item-text` / `item-text-shadow`（四态对象与单态键；设置即 `hasStyle=true`，TreeView.cpp:872-886 / 904-913）；
  - `item-shadow`（开关，:984-989）、`item-shadow-offset-x/y`（`kTreeItemShadowOffsetX/Y`，:1019-1022）、`item-font`/`item-font-size`/`item-leading-gap` 等；
  - 读回对称（:888-892 / :1071-1076）。
- **真实缺口**：① 无"id 参数专用 API"（设计器需两步 `SetString` 定位，且 `item-id` 是共享目标状态，易误写）；② **JSON items 无样式键解析**（`parseItems` 仅 id/label/expanded/leadingGap/alignment/itemFont/itemFontSize/leadingControl；schema `tree-view.items` 描述亦无样式键——连 `item-*` 都未解析，属声明式缺口）。

## 2. 修改方案

### 2.1 P0-62① ListView 单元格文字色
- `CellStyle` 增 `bool hasTextColor = false`（稀疏）；
- 绘制稀疏化：`if (cs.hasTextColor) tc = cs.textColor;`（未显式设色 → 走行样式/控件级链，修复"设背景变黑字"缺陷）；
- 新 ABI（独立函数保 ABI 兼容）：
  ```c
  int UICornerstone_ListViewSetCellTextColor(UIInstance, UIControlHandle lv, int row, int col,
      uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  ```
  （取现有 `CellStyle` 或默认 → 设 textColor + `hasTextColor=true` → `setCellStyle`；越界返回 0）；
- Binding：`ListViewSetCellTextColor(lv, row, col, UIColor)` + DynamicApi；
- 既有 `ListViewSetCellStyle` / `ListViewSetCellShadow` 行为不变（不置 hasTextColor）。

### 2.2 P0-62② TreeView 节点级着色专用 API + JSON 键
- **专用 API（3 个，内部直接写节点字段 + `hasStyle=true`，不依赖 `item-id` 共享状态）**：
  ```c
  int UICornerstone_TreeViewSetNodeTextColor(UIInstance, UIControlHandle tree, const char* id,
      uint8_t r,g,b,a, const char* state);            // state 可 NULL=normal
  int UICornerstone_TreeViewSetNodeBackgroundColor(UIInstance, UIControlHandle tree, const char* id,
      uint8_t r,g,b,a);
  int UICornerstone_TreeViewSetNodeShadow(UIInstance, UIControlHandle tree, const char* id,
      uint8_t r,g,b,a, float offsetX, float offsetY); // 置 shadowEnabled=true
  ```
  未找到节点返回 0；稀疏——未设项继承控件级（既有 draw 链）；
- **JSON items 样式键（`parseItems` 内解析，与 ListView 表头 P0-56 同族命名）**：
  | 键 | 形态 | 映射 |
  |---|---|---|
  | `text-color` | 字符串=normal / 对象=四态 | `node->textColor`（对应态） |
  | `background-color` | 字符串（单色） | `node->bgColor.setNormal` |
  | `text-shadow` | 字符串 | `node->textShadowColor.setNormal` + `shadowEnabled=true` |
  | `text-shadow-offset-x/y` | number（缺省 1,1） | `node->shadowOffsetX/Y` |
  任一命中 → `hasStyle=true`；
- schema：`tree-view.items` description 更新（元素宽松，无需 additionalProperties 改动）；
- Binding：3 个封装 + DynamicApi。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | 单元格文字色 API 生效（像素级：设红色后单元格文字红）；未设 hasTextColor 的单元格走行/控件级（回归：仅设背景不变黑字） | C ABI 探针（CaptureControl 像素） |
| 2 | 节点三 API：设色/背景/阴影后节点视觉生效（像素）+ 未设项继承控件级 | C ABI 探针 + test_treeview 断言（hasStyle/字段） |
| 3 | JSON items 样式键解析（字符串/四态/偏移）→ 节点字段正确 | 样例布局 + 引擎断言 |
| 4 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **P0-62② 现状更正确认**：运行时节点着色能力已存在（`item-id` + `item-*` 属性链）——专用 id 参数 API 仍需要（建议要：便利 + 避免共享目标状态误写）？
2. **P0-62① 附带缺陷修复**：`CellStyle.hasTextColor` 稀疏化（"仅设背景不再变黑字"）——确认一并修复？
3. **JSON `text-color` 四态对象支持**（对齐 P0-55 statusbar 段色）——确认？`background-color` 单色确认？
4. **节点阴影 API 形态**：`TreeViewSetNodeShadow(color, ox, oy)`（置开关；无独立 enable 参数）——确认？
5. **ListView 单元格 JSON 样式键**（rows cells 内样式）本批是否一并？设计器未要求（其行式格式走 API 绑定）——建议后置，确认？

## 5. 配套改动

- 代码：`ListView.h/.cpp`（hasTextColor + 绘制稀疏）、`TreeView.h/.cpp`（专用方法可选，主要走字段直写）、`UICornerstoneAPI.h/cpp`、`binding/*`、`LayoutParser.cpp`（items 样式键）；
- schema：tree-view items description；
- 测试：C ABI 探针（像素 + 字段）+ test_listview/test_treeview 断言；文档（CABI/Binding 速查表 + listview/treeview 控件页）；设计文档标注 + 复核归档 + make_release + 同步。
