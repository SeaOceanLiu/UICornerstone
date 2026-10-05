# P063_UnifiedFormatFollowup_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-03
- 对象：`design/P063_UnifiedFormatFollowup_Design.md`（P0-63①~⑦，状态：待评审）
- 结论：**通过，放行实施**（含 1 处我方报告更正：P0-63⑦ 系基于过期 Temp 副本的误报）

## 1. 逐项答复（§4 待审核问题）

| # | 问题 | 答复 |
|---|---|---|
| 1 | P0-63⑥ ComboBox 逐项样式：本期不做 or 最小子集？ | **同意本期不做**（理由成立：控件级下拉项样式已完备、无真实用例、跨控件 item 抽象宜沉淀）。用户问询将转达；若后续坚持，按最小子集（`text-color` 四态 + `background-color` 单色 + 半透明叠加）平移 |
| 2 | P0-63① 半透明高亮叠加方案/范围？ | **确认**：仅 cell-bg 单元格、选中优先于 hover、disabled 不叠加、alpha 常量化（魔鬼数字规范）。建议叠加色直接复用 `m_hoverColor`/`m_selectedColor` 的 RGB + 常量 alpha（不引入新配色语义） |
| 3 | P0-63②③ `hasFontName` 稀疏修复？ | **确认**。两处无条件覆盖已复核属实：单元格 `fn = cs.fontName`（ListView.cpp:699）、列头 `fontFor(st.fontName, st.fontSize > 0 ? … : m_fontSize)`（:595）——仅设背景/色/字号会重置字体为默认 regular。独立 API + 不隐式覆盖 ✓ |
| 4 | P0-63④ `fontSize<=0` 用控件级字号驱动 + 专用 API？ | **确认**。`getNodeFont`（TreeView.cpp:111）门槛修复语义正确（名称生效、字号随控件级）；设计器过渡随同步删除 ✓ |
| 5 | P0-63⑤ 段级字体名本批纳入或后置？ | **建议本批纳入**：改动面小（单字段 + 缓存键扩展 + 1 API + 1 JSON 键），且统一行式格式的字体名支持面将完整（设计器已解析 `font-name` 入模型，同步后即可接线） |
| 6 | P0-63⑦ 按"schema 已 6 项 + 清理历史示例"处理？ | **确认，且更正我方报告**：引擎核实属实——`subModules/UICornerstone/docs/schema/declarative-ui.schema.json` 与 EXE 侧构建拷贝（CMakeLists:54-57 自 subModules 复制）**均 6 项**；"28 token"来源为我方参考的过期 `Temp/schema/declarative-ui.schema.json`（gitignored 副本）与历史设计文档示例。无需引擎扩字体；历史文档/注释示例清理（引擎侧）同意。设计器侧无代码改动（运行期 schema 来源正确） |

## 2. 实施注意（放行附项）

1. **P0-63①**：叠加仅作用于 `bgColor.alpha() > 0` 的单元格（与现状绘制门槛一致）；叠加后文字/网格线绘制顺序不变（避免文字被叠加色覆盖）。
2. **P0-63②③**：API 名称参数经 `FontNameFromString`（无效 token 静默回退 regular——既定设计 ✓）；设计器随批接线 rows/columns 的 `font-name` 字段并删除"模型保留"注记。
3. **P0-63④**：专用 API `TreeViewSetNodeFont(name, size)` 落地后，设计器删除过渡（名称单独设置时读控件级字号写入节点）与 `SetEnum` 两步调用（如 API 一次完成定位+设置）。
4. **P0-63⑤**：`fontForSize` → (fontName, pixelSize) 缓存键扩展时，注意与 P0-57 的缓存失效路径一致（`m_font.reset()`/`ensureFont`/`relayout`）。
5. **P0-63⑦**：设计器侧将清理误导性 Temp 副本引用（复核流程改用 subModules 版 schema），避免后续误报。

## 3. 放行

**结论：放行实施**。现状核实（含②③ 附带覆盖缺陷、④ 门槛缺陷）质量高；① 叠加方案为合理取舍（备选亮度调制不推荐 ✓）；⑥ 暂缓判断成立；⑦ 更正确认（我方误报，已更正）。实施完成后同步 subModules，设计器随批：字体名接线（rows/columns/statusbar）+ tree 过渡删除 + 端到端联测（字体名 6 token、hover 叠加、hasFontName 稀疏回归）。
