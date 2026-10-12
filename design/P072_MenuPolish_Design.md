# P072_MenuPolish_Design — 菜单润色（阴影半径 / bar pressed schema / addMenu 状态回放 / item 级颜色）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-07 追加：P0-72）
> 前置：P0-69/P0-71 批已实施并同步
> 状态：**已放行，已实施**（2026-10-07；Q-1 menus[i] 层级；Q-2 分散键；Q-3 阴影后续；Q-4 回放清单确认）
> 复核：`requirements/P072_MenuPolish_Design_复核意见.md`（通过）
> 实施备注：① `kShadowRadius`（MenuPanel float 读写 + `drawShadow` 用成员 + menus[i] JSON + schema 描述）；② schema `colors-menu.background` → `state-color-4`（依赖 P074④ 逐键，pressed → bar m_activeBgColor）；③ `MenuBar::applyStateToPanel`（11 色/字号/字体名/行高比/文本阴影/阴影半径）+ `addMenu` 回放 + bar `font-size` 属性直通（原仅 value 别名）+ parseMenuBar per-menu 属性后置（避免被回放覆盖）；④ item 级颜色（MenuItem textColor/bg/hoverBg + 绘制回退 + JSON `text-color`/`background-color`/`hover-background-color` + CABI `item-text`（SetStateColor）/`item-background`/`item-hover-background` 定向 + schema/文档）；验证：**P0-72 OK**（bar→panel 回放 bg/text/font/shadow-radius、item 定向、JSON items 颜色）；回归仅 4 项预存；文档 menu/properties/declarative-syntax/treeview/statusbar 同步

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-72① | 菜单面板阴影半径不可调 | **确认** | `m_shadowRadius` 仅构造初始化（src/Menu.cpp:317）后再无引用（**死字段**）；`drawShadow()`（:800-820）直接使用常量 `MenuColors::PANEL_SHADOW_BLUR`（:48 = 8.0f）；`setFloatProperty` 仅 `shadow-offset-x/y`（:699-712）→ 无属性入口 |
| P0-72② | 主菜单激活（pressed）背景 schema 不可配 | **确认** | 运行期 bar `setColorProperty(kStatePressed) → m_activeBgColor`（:1290）读写齐备（:1311）；但 schema `colors-menu.background` = `state-color-3d`（N/H/D，无 pressed）→ 属性面板无行。注：`colors-menu` **仅被 menu-bar 引用**（schema:1093），panel/item 经 bar 传播、无独立 schema 引用 |
| P0-72③ | `addMenu` 不回放 bar 当前状态（缺陷） | **确认** | `addMenu`（:1042-1055）仅 ensureFont/fontSize/itemHeightRatio/hide/context；`setColorProperty`（:1286-1305）、`setTextStateColor`（:1268-1271）、`setFontSize`（:1246-1254）、`setEnumProperty(font)`（:1363-1377）均只传播**既有** `m_entries` → 结构化重建（RemoveMenu→AddMenu）后新面板回默认色/字 |
| P0-72④ | 无菜单项级颜色（对齐 StatusBar） | **确认** | MenuPanel 全部颜色为控件级成员（:671-686）；定向属性模式已有（`m_itemTargetId` + kTreeItemFontSize 等，:599-636）；`MenuItem`（include/Menu.h:97-116）无颜色字段；绘制读面板级色 |

## 2. 修改方案

### 2.1 ① `shadow-radius`（MenuPanel 阴影半径）
- `PropertyNames::kShadowRadius = "shadow-radius"`；
- MenuPanel `setFloat/getFloatProperty` 分支（>0 生效，默认 8）；`drawShadow()` 使用 `m_shadowRadius`（替换常量，默认行为不变）；
- JSON：`menus[i].shadow-radius`（number）→ panel；schema `menus` items 描述补充；
- CABI/Binding：通用 `SetFloat/GetFloat` 直通（零新增接口）。

### 2.2 ② schema：`colors-menu.background` 升为四态
- schema：`colors-menu.background` 由 `state-color-3d` → `state-color-4`（含 pressed）；
- 解析侧依赖 ④（P074）逐键整改后，`pressed` → `setColorProperty("pressed")` → bar `m_activeBgColor` 自动生效；MenuPanel/MenuItem 收到 pressed 仅存储（无绘制语义，无害）。

### 2.3 ③ `addMenu` 状态回放（重构）
- 新增 `MenuBar::applyStateToPanel(MenuPanel*)`：把 bar 当前状态应用到面板——
  - 颜色 11 键：background / hover / **pressed(active)** / disabled / border / border-hover / border-disabled / text / text-hover / text-disabled / text-shadow（各态）；
  - 字号 `fontSize`、字体名（`setFontName`）、`itemHeightRatio`、文本阴影（enabled/offset）、（①的）`shadow-radius`；
- `addMenu` 末尾调用；各 setter 改为循环调用同一实现（消除"只传播既有面板"的分叉）；
- 设计器过渡（syncMenus 手动回放）可在同步后删除。

### 2.4 ④ 菜单项级颜色（对齐 statusbar/tree 家族）
- **JSON items 键**（`populateMenuPanel` 解析，键名与 statusbar/tree 家族一致）：
  - `text-color`：字符串（=normal）或 `{normal,hover,pressed,disabled}` 对象；
  - `background-color`：字符串或 `{normal,hover}`；`hover-background-color` 别名（与 ListView cell 同款）；
- **MenuItem 字段**：`textColor`（StateColor + hasTextColor）、`background`/`hoverBackground`（SColor + has 标志）；
- **绘制回退链**：item 显式 → 面板级（现状）；面板级 → bar（③ 回放保证）；
- **CABI/Binding 定向**：沿用 `m_itemTargetId`（`item-id` 定位）模式，新增颜色键 `item-text`（SetStateColor 通道）/ `item-background` / `item-hover-background`（SetColor）——或统一 `item-colors` 对象（Q-2）；
- schema `menus` items 描述 + docs/controls/menu.html 同步。

## 3. 测试计划
- **属性**：shadow-radius 读写（getFloat 回读）+ 像素探针（半径变化 → 阴影范围/边界变化）；
- **②**：bar `SetColor("pressed")/GetColor("pressed")` 读写回 + schema strict 校验（4 态）；
- **③**：SetColor/SetFontSize/SetFontName → `MenuBarAddMenu` 新面板 → 逐键读回相等（CABI 探针/测试）；
- **④**：JSON items `text-color`（字符串/四态对象）/`background-color`/`hover-background-color` 解析 + CABI 定向读写 + 像素探针（某项独立变色）；
- 回归：test_menu/test_property_cabi/menu 样例 + 全量（仅 4 项预存可接受）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | `shadow-radius` 层级：`menus[i]`（每菜单）vs menu-bar 全局传播 | `menus[i]`（与 font/item-height-ratio 同层） |
| Q-2 | item 级 CABI 键名：`item-text`/`item-background`/`item-hover-background` vs 统一 `item-colors` | 分散键（与既有 `item-id`+`item-font-size` 定位模式一致） |
| Q-3 | ④ 范围：text 四态 + 背景两态；item 级文本阴影是否同批？ | 四态 text + 背景两态先行；阴影后续 |
| Q-4 | ③ 回放清单（11 色 + 字号 + 字体名 + 行高比 + 文本阴影 + 阴影半径） | 确认 |
