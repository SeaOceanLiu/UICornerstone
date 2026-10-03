# P057_59_StatusBarLayout_Design — StatusBar 字号缓存/段级样式 + 运行时布局模式与锚点读回

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-02 追加：P0-57 / P0-58 / P0-59）
> 前置：P0-53~56 批已实施并同步
> 状态：**已放行，已实施**（2026-10-02）
> 复核：`requirements/P057_59_StatusBarLayout_Design_复核意见.md`（通过，放行实施；五问全确认 + 4 项实施注意已落实）
> 实施备注：① P0-57 字号缓存失效（setFontSize 补 m_font.reset + 段级字体缓存清理）；② P0-58 段级字号（fontForSize 缓存）/文字阴影（has 稀疏，段→控件级）+ ABI/Binding/JSON/schema；③ P0-59 运行期 layout 五档切换（absolute/空串=清除引擎；GetEnum 无引擎返回 "absolute"）、child-id 锚点读回（未设返回 0）、schema anchor/anchorOffset 下放 common（14 值 enum + margin）并从 panel def 移除；④ 实施注意：设计器须 SetControlId 注册（引擎无需改，已注明）、窗体级贴边预览限制（容器内闭环）已写入文档；⑤ 测试：test_statusbar 28/0（含 P0-57 断言）、test_property_cabi 145/0（P0-59 全链）、样例 p0_statusbar_list.json 扩展（anchor/font-size/text-shadow）ctest strict 9/9；⑥ 回归仅 4 项预存失败；⑦ 文档：CABI/Binding 速查表 + statusbar 键表 + declarative-syntax 6.5 运行期说明

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-57 | StatusBar 运行期 font-size 不生效 | **确认** | `StatusBar::setFontSize`（StatusBar.cpp:127-129）仅 `m_fontSize=size; relayout()`，**未 `m_font.reset()`** → `ensureFont()`（`if (m_font) return;`）早退，字号/测量宽度按旧字体。**审计结论**：TreeView（m_font.reset+m_nodeFonts.clear+ensureFont）、TabControl（reset+ensureFont+relayout）、ListView（reset+m_fontCache.clear+ensureFont）、MenuPanel（reset+ensureFont+updateItemsFont）、MenuBar（reset+ensureFont+面板转发）、ColorPicker（`setClosedFontSize`→`recreateClosedState`）均正确；EditBox 经 `loadFontInternal`；ProgressBar 转发内部 Label；**仅 StatusBar 遗漏**（`StatusBar::setFontName` 已含 reset ✓，唯 setFontSize 缺） |
| P0-58 | 段级字号/文字阴影（可选） | **确认现状** | `StatusItem` 无 `fontSize`/阴影字段（P0-55 仅补了段色）；字号/阴影仅控件级（`m_fontSize`/`m_shadowEnabled`+`m_shadowColor`+`m_shadowOffset`），绘制与 relayout 测量固定用 `m_font` |
| P0-59 | 运行时布局模式切换 + 锚点读回 | **确认三点** | ① 容器 `layout` 仅解析期（LayoutParser.cpp:1315-1351 构造 engine → `setLayoutEngine`），运行期无属性分发（`Panel::setEnumProperty` 无 `kLayout` 分支）、无重排；② Panel 无 `getEnumProperty`/`getFloatProperty` 覆写 → `GetEnum("anchor")`/`GetFloat("anchor-offset-x/y")` 返回 0；③ schema `anchor`/`anchorOffset` 仅在 `panel` def（:1183-1186），非 common。**已有基础**：子级运行期写入链路完整（`child-id` 定位 + `child-anchor` / `child-anchor-offset-x/y` / `child-grid-*` / `child-flow-weight`，Panel.cpp:114-189）；`Panel::reflowChildren()`（:74-82）按 engine 类型分派，可直接复用 |

## 2. 修改方案

### 2.1 P0-57 字号缓存失效（单行修复 + 审计结论）
- `StatusBar::setFontSize`：补 `m_font.reset()`（relayout 已有）；审计结论见上表（其余控件无同类缺陷，本批不再改动）。

### 2.2 P0-58 段级字号 / 文字阴影（对齐 ListView per-column 稀疏语义）
- `StatusItem` 扩展：
  ```cpp
  float  fontSize = 0.0f;                    // 0 = 继承控件级
  SColor shadowColor;  bool hasShadow = false;   // 段级阴影（未设继承控件级阴影）
  SPoint shadowOffset{1.0f, 1.0f};
  ```
- 引擎：`setStatusItemFontSize(id, float)` / `setStatusItemTextShadow(id, SColor, ox, oy)`；
- 绘制/测量：per-item 字体（`fontForSize` 小缓存，0→控件级 `m_font`）；`relayout` 测量与绘制垂直居中均按段字体；阴影优先级 = per-item（hasShadow）→ 控件级（m_shadowEnabled）；
- ABI/Binding：`StatusBarSetItemFontSize(bar, id, size)` / `StatusBarSetItemTextShadow(bar, id, color, ox, oy)`；
- JSON items：`font-size`（number，0=继承）、`text-shadow`（color）、`text-shadow-offset-x/y`（number）——偏移键新增常量；
- 若评审认为需求弱，可后置本项（Q-1）。

### 2.3 P0-59 运行时布局模式切换 + 锚点读回 + schema 下放
- ① **运行期切换**：`Panel::setEnumProperty` 增 `kLayout` 分支（值 = `h-flow`/`v-flow`/`anchor`/`grid`）→ 构造对应 engine（gap/padding 默认 0）→ `setLayoutEngine` + `reflowChildren()`；`Panel::getEnumProperty(kLayout)` → `engine->getType()`（无 engine → 返回 0）；
- ② **锚点读回**（配合 child-id）：`Panel::getEnumProperty(kChildAnchor)` / `getFloatProperty(kChildAnchorOffsetX/Y)`——map 命中返回值；未命中返回 0（未设置；设计器显示默认行，见 Q-2）；
- ③ **schema 下放**：`anchor`（enum 13 值：9 锚点 + `top/bottom/left/right-stretch`）与 `anchorOffset`（`$ref margin`）从 `panel` def 移入 `common`（所有控件节点可声明；panel def 删除避免重复，见 Q-4）；
- 设计器随批：属性面板"布局模式"下拉（容器）+ "锚定"下拉 + 偏移行；预览贴边验证。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | P0-57：运行期 font-size 改变 → 段 hitRect 宽度随新字号变化（旧实现不变） | test_statusbar 断言（ABI 设 font-size + 直读 hitRect） |
| 2 | P0-58：段级 fontSize 生效（hitRect/字体高度）、段级阴影绘制；未设继承控件级 | test_statusbar 断言 + 像素探针 |
| 3 | P0-59：`SetEnum("layout","anchor")` 后子项按锚点重排（GetRect 验证）；`GetEnum("anchor")`/`GetFloat("anchor-offset-x/y")` 读回；`SetEnum("layout","h-flow")` 切回 | 引擎断言 + C ABI 探针 |
| 4 | schema：anchor/anchorOffset 在 common；validate_layout strict 全绿（示例用 anchor 的控件） | ctest |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **P0-58 是否纳入本批**（低优先级）？纳入则按 §2.2 实施；后置亦可。
2. **P0-59 锚点读回未设置语义**：返回 0（未设置，设计器显示默认行）而非默认 `top-left`——确认？
3. **运行期布局切换的 gap/padding**：本期默认 0（不新增运行期 `gap`/`padding` 属性）——确认？
4. **schema 下放**：anchor/anchorOffset 移入 common 并从 panel def 删除（避免重复定义）——确认？
5. **P0-58 JSON 键名**：`font-size` / `text-shadow` / `text-shadow-offset-x/y`（偏移键新增常量）——确认？

## 5. 配套改动

- 代码：`StatusBar.cpp/h`（字号 reset + 段级样式）、`Panel.cpp/h`（layout 运行时分发 + 锚点读回）、`PropertyNames.h`、`UICornerstoneAPI.h/cpp`、`binding/*`、`LayoutParser.cpp`（items 段级键）；
- schema：common 下放 anchor/anchorOffset；panel def 删除；状态栏 items description 更新（段级字号/阴影）；
- 测试：test_statusbar 断言 + C ABI 探针（布局切换/锚点读回）+ 像素探针；文档（cabi/binding 速查表 + statusbar 控件页 + 声明式语法 6.5 措辞）；设计文档标注 + 复核归档 + make_release + 同步。
