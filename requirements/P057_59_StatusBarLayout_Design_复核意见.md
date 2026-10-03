# P057_59_StatusBarLayout_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-02
- 对象：`design/P057_59_StatusBarLayout_Design.md`（P0-57/58/59，状态：待评审）
- 结论：**通过，放行实施**（附 4 项实施注意：P0-59 补 absolute 清空语义、设计器须注册控件 id、窗体级贴边预览限制说明、P0-58 纳入）

## 1. 评审要点

### 现状核实（§1）✓（源码抽验一致）

- P0-57：`StatusBar::setFontSize` 仅 relayout 缺 `m_font.reset()`；审计结论（TreeView/TabControl/ListView/MenuPanel/MenuBar/ColorPicker 均正确、仅 StatusBar 遗漏）抽验无误 ✓。
- P0-58：`StatusItem` 无段级字号/阴影字段 ✓。
- P0-59：`layout` 仅解析期构造 engine（无运行时分发）；`Panel` 无 getEnum/getFloat 覆写；schema `anchor`/`anchorOffset` 仅 panel def ✓；**已有基础抽验**：`child-id`（setStringProperty Panel.cpp:113-120）+ `child-anchor`/`child-anchor-offset-x/y`（setEnum/setFloat）+ `reflowChildren` 按 engine 类型分派（:74-82）——可复用 ✓。

### 方案（§2）✓

- **P0-57**：单行修复 + 审计范围克制（仅改缺陷点）✓。
- **P0-58**：稀疏语义（fontSize=0 继承 / hasShadow）+ 绘制与测量统一按段字体 + 阴影优先级（段→控件级）——与 ListView per-column 家族一致 ✓。
- **P0-59**：① 运行期 `layout` 分发（构造 engine + setLayoutEngine + reflowChildren）+ 读回；② 锚点读回（map 命中返回/未命中 0）；③ schema 下放 common——三点齐备 ✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | P0-58 是否纳入本批？ | **纳入**（用户已问询；实现量小、与段色同族） |
| 2 | 锚点读回未设置返回 0（设计器显示默认行）？ | **确认**（与"未设/默认 top-left"区分，设计器空选显示） |
| 3 | 运行期切换 gap/padding 默认 0？ | **确认**（本期不新增；后续需要再列） |
| 4 | schema anchor/anchorOffset 移入 common + panel def 删除？ | **确认**（anchor 是子控件级属性，任意类型可声明；parser 本就按 children[i]["anchor"] 读取） |
| 5 | P0-58 JSON 键名 font-size/text-shadow/text-shadow-offset-x/y？ | **确认** |

## 3. 实施注意（放行附项）

1. **P0-59 补 "absolute" 清空语义（重要）**：`SetEnum("layout", "absolute")`（或空串）→ 清除布局引擎（`setLayoutEngine(nullptr)`）+ reflow，回到自由布局；`GetEnum("layout")` 无引擎返回 **"absolute"** 而非 0——否则切到 v-flow/anchor 后无法切回、读回也无法区分"未设"与"绝对"。设计器"布局模式"下拉含五档：绝对/h-flow/v-flow/anchor/grid。
2. **设计器须注册控件 id（重要，集成前提）**：`findChildById` 走 `ctx->controlsById`（Panel.cpp:122-128），而设计器放置控件时**未调用 `SetControlId`**（模型 id 未入引擎 id 表）→ `child-id` 定位失败、锚点/流权重/网格写入全部无效。设计器随批：`placeControl` 创建后 `m_ui->SetControlId(ctl, modelId)`（Binding 已有，UICornerstone.h:122；顺带使 `FindControl` 对放置控件可用）；删除时随 `DestroyControl` 清理。引擎侧无需改动，请在文档/验收注明该前提。
3. **窗体级贴边预览限制（说明，非阻塞）**：画布根（bench）无布局引擎且设计器不可设 → 预览中"窗口级"贴边暂不可现。随批先闭环**容器内锚定**（放置 Panel → 布局模式 anchor → 子控件锚定 → 拖 Panel 手柄缩放验证贴边）；窗口级待导出（阶段五：根面板 anchor 模式）或 bench 支持后再闭环。设计器属性面板行文案可标注"（父容器 anchor 布局下生效）"。
4. **P0-58 小点**：段级字体的 relayout 测量与绘制垂直居中均按段字体（设计已覆盖）；`fontForSize` 缓存建议设上限或复用控件级字号命中（段数少，可不限，按实现成本定）。

## 4. 放行

**结论：放行实施**。三项现状核实与方案（字号缓存修复/段级样式/运行期布局与锚点读回）质量高，边界（稀疏语义/读回未设语义/schema 下放）清晰；附 4 项实施注意（absolute 清空语义/控件 id 注册前提/窗体级预览限制/段字体缓存）。实施完成后同步 subModules，设计器随批：SetControlId 注册 + 布局模式下拉 + 锚定下拉与偏移行 + P0-58 行式扩展（段级字号/阴影），端到端联测。
