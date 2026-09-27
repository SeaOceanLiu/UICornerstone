# SchemaVisual_Refactor_Design — schema 视觉属性按类型下放 + 视觉能力补齐（三层合一）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-22，**P0-26**：common 视觉键下放各类型 def）
> 讨论定案（2026-09-23）：① slider 支持四态；② menu 采用 colors.text 三态（normal/hover/disabled）+ 统一覆盖各角色；③ dialog/popup/confirm-popup 仅背景/边框、无四态、无文字；④ status-bar/tab-control 走"渲染助手"路线并最大化复用代码；⑤ border-visible 随视觉组下放；⑥ colors 子对象严格化；⑦ 不设过渡期（现有 strict/non-strict 机制已够）；⑧ 文档先改用户手册+schema，其余后补
> 前置：P0-24/P0-25 已单独落地（2026-09-23，随批发布）
> 状态：**已放行，已实施**（复核：`requirements/SchemaVisual_Refactor_Design_复核意见.md`，2026-09-23 通过——§8 全部定案确认 + 设计器随批 4 项）
> **实施记录（2026-09-23）**：
> 1. **schema**：common 瘦身至 9 键（id/flow-weight/enabled/context-menu/events/margin/rect/scale/visible）；新增态子集 def（state-color-4/3d/3p/1）+ 组 def（colors-full/basic/basic1/bg/menu）+ 25 类型按矩阵声明；顶层 `text-shadow` 死键删除；`border-visible` 随视觉组下放。`validate_layout --strict` 三布局 + schema-only + tools ctest 9/9 全 PASS。
> 2. **渲染助手**：`TextDraw::withShadow/withShadowCached`（include/TextDraw.h）+ `ControlImpl::resolveStateColor`/`isTextColorFamilyKey`；Label 重构为调用方（像素回归通过）。
> 3. **背景/边框补齐**：Actor/LuotiAni（LuotiAni 原手动边框块移除、统一 afterDraw）；**勘误新增 ListView**（原未绘背景/边框，同批补 beforeDraw/afterDraw + 移除手动 focus ring）。
> 4. **转发补齐**：ColorPicker（关闭态：colors.text 四态/单态键、shadow、font-size、font）、Slider（valueLabel：colors.text 四态/单态键、shadow/offset、font-size、font）、ProgressBar（textLabel：text-shadow 四态、shadow/offset；懒建 Label 后转发）、StatusBar（colors.text 四态、shadow、font；原常量迁入四态缺省）、TabControl（colors.text 四态、**selected-text**、shadow、font）、Menu（MenuBar 统一覆盖：background 三态/border/text 三态/shadow/font；MenuPanel 属性化 + border-visible；MenuItem 文本按面板色 + 阴影 + disabled 色）。
> 5. **item/cell 阴影**：TreeView `item-text-shadow`+`item-shadow-offset-x/y`（item-id 定位 + 读回）；ListView `CellStyle` 阴影字段 + **新 C ABI `UICornerstone_ListViewSetCellShadow`（方案 A）** + Binding 包装 + DynamicApi。
> 6. **测试**：test_p0_getter §11（9 类能力读回全 PASS）；test_capture_cabi（image 背景像素 PASS）；全量回归仅 2 项预存失败（test_menu_cabi/test_treeview_cabi，与批次无关）。
> 7. **文档**：properties.html 新增 7.1.1 视觉能力矩阵 + TabControl selected-text + TreeView item 阴影行 + StatusBar font 行 + Menu 视觉键行；7.1 章节改为"布局/交互类通用属性"并指向矩阵；declarative-syntax.html colors 行补态子集说明；capi.html ListView 专用函数补 ListViewSetCellShadow；schema tab-control 补 `selected-text`（其余控件页后补）。
> 8. **复核修正（2026-09-23 用户复核发现）**：
>    - **TreeView/ListView 文本四态**：两者原自有 `SColor m_textColor` **遮蔽基类 StateColor**（属性路径更新基类、绘制读自有 → 文本色一直无效）；移除遮蔽 + 缺省色迁入基类 + 绘制按状态解析 → `colors.text` 四态真正生效（矩阵成立）。
>    - **Menu 态转发**：补 `setTextStateColor`/`setTextShadowStateColor`（JSON `colors.text` 三态 → item 角色色；阴影色三态），新增面板 hover 文本色与 item 阴影态解析。
>    - **StatusBar/TabControl 阴影四态**：补 `setTextShadowStateColor` + 阴影色 StateColor 化 + 绘制按状态解析。
>    - schema/文档同步：`selected-text` 入 schema；键↔常量配对审计 0 不匹配、新键文档覆盖齐全。

---

## 1. 目标与范围

**三层合一**：schema 声明 = 布局合法 = 视觉有效。消除"common 全量声明 → 无视觉控件也出现无效行"的引擎/设计器视觉理解差异；对**有意义但缺失**的能力做引擎补齐（用户决策），对**语义不成立**的能力（图形控件文字等）不做声明。

范围：
- **schema**：`colors`/`font`/`font-size` 从 common 下放 25 个类型 def（按能力矩阵）；`border-visible` 随视觉组下放；删除死键顶层 `text-shadow`；新增状态色组 def 并严格化。
- **运行时补齐**（9 项）：image/animation 背景/边框；color-picker 关闭态文字（色/字号/字体/shadow，四态）；slider valueLabel（色/字号/字体/shadow，四态）；menu 文本（色三态/字号/字体/shadow）；status-bar/tab-control 文本（色/shadow + 字体名）经渲染助手；list-view/tree-view item shadow（色+偏移）；progress-bar shadow 转发。
- **不动**：解析层（schema 仅校验层）；布局键运行时语义；common 其余布局/交互键（id/rect/scale/margin/events 等）。

## 2. 现状核实（源码事实）

| 项 | 事实 | 影响 |
|---|---|---|
| schema 结构 | 36 defs = 25 类型 def + common + 10 辅助 def；25 类型全部 `allOf common`；common 14 键 | 下放基数 25 |
| `colors`（common） | 自由 object，无子组/状态约束；解析层按组 `is_object()` + `parseStateColor`（normal/hover/pressed/disabled 可选） | 组 def 可严格化（additionalProperties:false） |
| 顶层 `text-shadow`（common） | **死键**：解析层从不读取顶层，仅读 `colors.text-shadow`（LayoutParser.cpp:2577-2580）；4 处引用均在 colors 组内 | 直接从 schema 删除 |
| `font`/`font-size` | 经 `applyFontDecl` 解析（含继承链） | 下放时保留原定义 |
| 校验器 | 控件级未知键检查已存在：`collectDefPropsInto`（本类型 properties ∪ allOf 展开 common）→ strict=Error / 非 strict=Warn（LayoutValidator.cpp:147/297/331） | **无需新增过渡机制**；下放后自动按新声明集判定 |
| Actor/LuotiAni 绘制 | 不调用 `beforeDraw/afterDraw`（无背景/边框）；未 override `update` → 基类 hover 状态机生效 | 加两行即四态背景/边框 |
| panel/shape/splitter/scroll-bar | 均调用 `beforeDraw/afterDraw`，`StateColor` 四态已具备 | **零改动**，仅 schema 声明 |
| ColorPicker | 关闭态 `m_closedLabel`（内部 Label）；键 `closed-text`（单色）/`closed-font-size`；无关闭态 font/shadow 键 | 补通用键转发（含四态） |
| Slider | `m_valueLabel`（内部 Label）；键 `label`（单色）/`label-font-size`/`label-font`；无 shadow | 补 colors.text 四态 + shadow 转发 |
| Menu | 文本色为内部常量（MenuColors：bar 文本/hover、item 文本/hover/disabled）；无属性暴露；字体键已有（`font`/`font-size`/`label-font-size`） | 补 colors.text 三态映射 + shadow（渲染助手） |
| StatusBar | 文本色为常量 `kTextColor`（StatusBar.cpp:19/185）；`font-size`/`item-height` 有；字体名无 | 补 colors.text + 字体名 + shadow |
| TabControl | 文本色为常量 `kSelTextColor/kNormTextColor`（选中/常态）；`font-size` 有；字体名无 | 补 colors.text + 字体名 + shadow；**选中页签色另定**（§9-1） |
| ProgressBar | **已是内嵌 Label**（`m_textLabel`），文本四态色/字号/字体均已转发 | 仅补 shadow 转发（含 offset） |
| ListView | CellStyle 有 `textColor/fontName/fontSize`（C ABI 未暴露 textColor）；单元格文本自绘 | item/cell shadow 需键/ABI + 两遍绘制 |
| TreeView | item 级仅 `item-font`/`item-font-size`，**无 item 颜色键** | item shadow 需新键（颜色+偏移） |
| Label 阴影实现 | `m_shadowEnabled` + `m_shadowOffset` + 四态 `m_textShadowColor`，draw 内偏移两遍绘制（Label.cpp:354-400） | **抽取渲染助手**供复用 |

## 3. 能力矩阵（终版，按讨论定案）

| 类型 | colors 组 | shadow（enabled+offset） | font / font-size |
|---|---|---|---|
| label / button / check-box / win-frame | **full**（background/border/text/text-shadow，四态） | ✓（既有） | ✓ / ✓ |
| edit-box / text-area / combo-box / numeric-up-down | full（四态） | ✗（决策：输入控件不加阴影） | ✓ / ✓ |
| list-view / tree-view | full（四态） | ✓ **item 级**（item shadow 色+偏移） | ✓ / ✓ |
| progress-bar | full（四态） | ✓ **补齐**（转发 textLabel） | ✓ / ✓ |
| status-bar / tab-control | full（四态） | ✓ **补齐**（渲染助手） | ✓ / ✓（字体名补齐） |
| slider | full（四态，决策①） | ✓ **补齐**（valueLabel 转发） | ✓ / ✓ |
| color-picker | full（四态；关闭态转发） | ✓ **补齐**（关闭态转发） | ✓ / ✓ |
| menu-bar（含 panel/item 运行时映射） | **background 三态**（normal/hover/pressed）+ **border 单态** + **text 三态**（normal/hover/disabled）+ text-shadow 三态（决策② + 用户确认：背景/边框需可定制以匹配应用整体网格） | ✓ **补齐**（渲染助手） | ✓ / ✓ |
| panel / shape | **basic**（background/border，四态） | ✗ | ✗ |
| image / animation | basic（四态）**补齐绘制** | ✗ | ✗ |
| splitter / scroll-bar | **bg-only**（background，四态） | ✗ | ✗ |
| dialog / popup / confirm-popup | basic（**仅 normal 单态**，决策③） | ✗ | ✗ |

> 注：colors 组内"态子集"可裁剪（如 menu text 仅 normal/hover/disabled；dialog basic 仅 normal），schema 以 def 组合表达。

## 4. schema 结构设计

### 4.1 新增/调整 defs

```json
"state-color-4":  { "type":"object", "properties":{ "normal":$color,"hover":$color,"pressed":$color,"disabled":$color }, "additionalProperties":false },
"state-color-3d": { ... 仅 normal/hover/disabled ... },   // menu 文本
"state-color-3p": { ... 仅 normal/hover/pressed ... },    // menu 背景
"state-color-1":  { ... 仅 normal ... },                  // dialog basic / menu border
"colors-full":  { "type":"object", "properties":{ "background":$sc4,"border":$sc4,"text":$sc4,"text-shadow":$sc4 }, "additionalProperties":false },
"colors-basic": { ... background/border ... },
"colors-bg":    { ... background ... },
"colors-menu":  { ... background:$sc3p, border:$sc1, text:$sc3d, text-shadow:$sc3d ... }
```
- 顶层 `text-shadow`（common）**删除**（死键）；文本阴影色统一经 `colors.text-shadow`。
- `border-visible` 从 common 下放到 basic/full 组类型（image/animation 不声明；splitter/scroll-bar 不声明）。

### 4.2 各类型 def 声明

- full：`colors:$colors-full` + `shadow:$shadow` + `font` + `font-size`（label/button/check-box/win-frame/edit-box/text-area/combo-box/numeric-up-down/list-view/tree-view/progress-bar/status-bar/tab-control/slider/color-picker）。
- menu：`colors:$colors-menu` + `border-visible` + `shadow` + `font` + `font-size`（menu-bar；运行时覆盖 bar/panel/item 角色）。
- basic：`colors:$colors-basic` + `border-visible`（panel/shape/image/animation）。
- basic-1：`colors:$colors-basic`（态限 normal；`border-visible` 一并下放）（dialog/popup/confirm-popup）。
- bg-only：`colors:$colors-bg`（splitter/scroll-bar）。
- common 保留：id/flow-weight/enabled/context-menu/events/margin/rect/scale/visible。

### 4.3 兼容与校验

- **不设过渡机制**（决策⑦）：非 strict 校验天然 Warn（现状），`--strict` 为 Error（验收用）。
- 引擎自身布局排查：`layouts/*.json` 3 个逐键核对（下放后不得出现类型未声明键）；设计器布局由其同步排查。
- 组件/动态类型（control-any、组件实例）沿用宽松放行。

## 5. 运行时补齐实现（最大化复用）

### 5.1 渲染助手（决策④：抽 Label 阴影逻辑）

```cpp
// 新增（TextRenderer 扩展或独立 helper；Label 重构为调用方，行为不变）
void drawTextWithShadow(TextRenderer* r, Font* font, const std::string& text,
                        float x, float y, const SColor& color,
                        const SColor& shadowColor, const SPoint& shadowOffset, bool shadowEnabled);
```
调用方：Label（重构）、StatusBar、TabControl、MenuBar/MenuPanel（含 item）、ListView/TreeView（item/cell 文本）。

### 5.2 逐项补齐

| 控件 | 改动 | 键 |
|---|---|---|
| Actor / LuotiAni | draw 首尾加 `beforeDraw()/afterDraw()`（背景/边框，四态经基类状态机） | colors.basic（既有属性系统键） |
| ColorPicker | 通用键转发 `m_closedLabel`：colors.text（四态）/font/font-size/shadow/shadow-offset-x/y；`closed-*` 保留（兼容） | 通用键 |
| Slider | `m_valueLabel` 转发 colors.text（四态）/shadow/shadow-offset-x/y；`label*` 保留 | 通用键 |
| MenuBar/MenuPanel | **背景/边框补齐**（成员已有、需属性分发）：colors.background 三态映射 normal→BAR_BG/PANEL_BG、hover→BAR_HOVER_BG/ITEM_HOVER_BG、pressed→BAR_ACTIVE_BG（item 按下沿用 hover，文档注明）；colors.border 单态→PANEL_BORDER + `border-visible` 控制（bar 自身无边框，文档注明）；colors.text 三态映射 normal→bar/item 文本、hover→bar/item hover、disabled→ITEM_DISABLED；shadow/offset 经渲染助手；字体名补齐 | colors.background（3 态）+ border + text（3 态） |
| StatusBar | 文本色改由 `colors.text` 四态解析（替换常量 kTextColor）；字体名键补齐（`font`）；shadow/offset 经渲染助手 | colors.text + font + shadow |
| TabControl | 常态页签文本走 `colors.text` 四态；选中页签色见 §9-1；字体名补齐；shadow 经渲染助手 | colors.text + font + shadow |
| ListView | CellStyle 增 shadow 字段（**单色**+偏移，与 `CellStyle.textColor` 单色对齐）；**新增 C ABI `UICornerstone_ListViewSetCellShadow`（方案 A：纯增量）** + Binding 包装 + DynamicApi 项；渲染两遍 | `cell-text-shadow` + `cell-shadow-offset-x/y` |
| TreeView | item 级 shadow 键（**单色**+偏移，与 `m_textColor` 单色对齐）+ 渲染两遍 | `item-text-shadow` + `item-shadow-offset-x/y` |
| ProgressBar | shadow/offset 转发 `m_textLabel` | `shadow`/`shadow-offset-x/y` |

## 6. 影响面与风险

- **schema 收紧**：使用未声明键的既有布局在 `--strict` 下报错（非 strict 仅 Warn）——引擎 3 个布局自查；设计器布局自查（其 noVisual 特例删除后纯 schema 驱动）。
- **补齐行为新增**：各补齐项均为"新增可达能力"，不改既有默认视觉（menu/statusbar/tabcontrol 默认色保持现值；label 文本色默认不变）。
- **渲染助手重构**：Label 绘制改为调用助手，需像素回归（test_label / test_capture_cabi）。
- **C ABI 增补**：ListView cell shadow 需新 ABI（1 个）+ Binding 包装；其余补齐均经既有通用属性通道（零新 ABI）。
- 文档量：properties.html 7.1 通用属性 → 能力矩阵 + 分组说明；相关控件页同步；schema 注释。

## 7. 验收测试

| # | 验收 | 测试 |
|---|---|---|
| 1 | 各补齐项读写往返 + 像素/视觉（image 边框、colorpicker 关闭态色/shadow、slider label 四态、menu/statusbar/tabcontrol 文本色与 shadow、progressbar shadow、list/tree item shadow） | 新增 test_visual_补齐（C ABI 探针）+ 既有像素测试扩展 |
| 2 | schema：`validate_layout --strict` 对 3 个引擎布局 PASS；类型未声明键在 strict 下 Error、非 strict Warn | tools 校验 + tools/tests 扩展 |
| 3 | Label 重构无回归 | test_label / test_capture_cabi / 全量回归 |
| 4 | 三后端编译 0 错误 | ALL_BUILD |

## 8. 待审核问题

**已确认（2026-09-23 讨论）**：① TabControl 新增专用键 `selected-text`（选中页签"更亮"语义保留）；② TreeView `item-text-shadow` + `item-shadow-offset-x/y`；ListView `cell-text-shadow` + `cell-shadow-offset-x/y`（新增 1 个 C ABI）；③ menu 背景/边框需可定制（colors.background 三态 + border 单态 + `border-visible`）；④ dialog/popup 单态 JSON `{"background":{"normal":"#..."}}` 可接受；⑤ `border-visible` 下放：full/basic/menu 组声明，splitter/scroll-bar 不声明。

**剩余问题**：无（全部确认，2026-09-23）。

1. **ListView cell shadow C ABI**：**方案 A**（新增 `UICornerstone_ListViewSetCellShadow(inst, lv, row, col, r,g,b,a, offX, offY)`，纯增量；方案 B 扩签名因动态解析（GetProcAddress）属破坏性变更，已否决）。
2. **item/cell shadow 态数**：**单色**——源码核实：TreeView `m_textColor`（TreeView.h:117）与 ListView `CellStyle.textColor`/`m_textColor`（ListView.h:224）均为单色 SColor，item/cell 文本无四态 → 按"text 无四态则 shadow 无四态"规则取单色。
3. **menu item 按下背景**：pressed 映射 bar 激活色（BAR_ACTIVE_BG）、item 按下沿用 hover——确认简化。

## 9. 待提交配套改动（实施时随批）

- 用户手册：properties.html（7.1 改能力矩阵/分组；各控件行按矩阵）、declarative-syntax.html（colors 组态子集说明）、README（控件能力表如需）。
- schema：`docs/schema/declarative-ui.schema.json` 重构 + validate --strict 验收。
- tools/tests：校验器用例（strict Error / 非 strict Warn、态子集）。
- requirements/ 归档（新版清单已归档）；复核意见归档后 make_release 同步 CornerstoneDesigner。
