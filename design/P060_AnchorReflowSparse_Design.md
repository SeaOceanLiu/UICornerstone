# P060_AnchorReflowSparse_Design — 锚点 setter 即时重排 + applyAnchor 稀疏语义

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-03 追加：P0-60，P0-59 联测实测缺陷）
> 前置：P0-57~59 批已实施并同步
> 状态：**已放行，已实施**（2026-10-03）
> 复核：`requirements/P060_AnchorReflowSparse_Design_复核意见.md`（通过，放行实施；四问全确认 + 1 项低优先建议）
> 实施备注：① 锚点/偏移/流权重/网格运行期 setter 补 `reflowChildren()`（解析路径不变）；② `applyAnchor` 稀疏（未命中 continue）；③ 低优先建议落实：`AnchorInfo.hasAnchor`（仅显式设 anchor 激活，offset-only 不激活；读回未激活返回 0；解析设置显式锚点即置位）；④ 测试：test_property_cabi 158/0（即时重排/稀疏/offset-only）；⑤ 回归仅 4 项预存失败；⑥ 文档：declarative-syntax 6.5 补即时生效与稀疏语义

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-60① | 设 `anchor`/`anchor-offset-x/y` 不触发重排（仅切布局/父 resize 才应用） | **确认** | `Panel::setEnumProperty`（kChildAnchor，Panel.cpp:180-190）与 `setFloatProperty`（kChildAnchorOffsetX/Y，:140-155）更新 `m_anchorItemProps` 后**无 `reflowChildren()`**；`setChildAnchorProps`（Panel.h:48）为纯存储内联。实测"设底拉伸后控件停在旧位置/跑到顶部"（旧位置即未重排；跑到顶部=见 ②） |
| P0-60② | 未设锚点子控件被按默认 top-left 处理，切 anchor 后全部塌左上堆叠 | **确认** | `AnchorLayout::applyAnchor`（LayoutEngine.cpp:312-320）：`anchorProps` 未命中 → `anchor = kAlignLowerTopLeft; offset = 0` 并强制写 rect（非稀疏） |
| 同族审计 | 运行期 `child-flow-weight`（:131-139）与 `child-grid-*`（:158-177）同样仅存储、不重排 | **确认（建议一并处理，见 Q-1）** | 与锚点 setter 同模式 |

**兼容影响评估**：
- 现有使用 anchor 布局的布局样例（`layouts/test_layout_advanced.json` anchorPanel）**全部子控件均显式 `anchor`** → 稀疏化对其零影响；
- `layouts/p0_statusbar_list.json` 中 label 的 anchor 挂于无布局引擎的 panel（本不生效）→ 不受影响；
- 切换 `layout=anchor` 后未锚定子控件保持原 rect（不再塌左上）。

## 2. 修改方案

### 2.1 P0-60① 运行期 setter 即时重排
- `Panel::setEnumProperty(kChildAnchor)` / `setFloatProperty(kChildAnchorOffsetX/Y)`：更新 map 后补 `reflowChildren()`（布局引擎存在时立即应用；无引擎时 reflow 内部早退，无开销）；
- 一致性（Q-1）：`kChildFlowWeight` / `kChildGridRow/Col/RowSpan/ColSpan` 同样补 reflow；
- 解析路径保持现状（`setChildAnchorProps` 纯存储 + panel children 循环后统一一次 `reflowChildren()`，避免逐子重排）。

### 2.2 P0-60② applyAnchor 稀疏语义
- `AnchorLayout::applyAnchor`：`anchorProps` **未命中 → `continue`**（保持现有 rect，不参与锚点管理）；命中者按既有锚点/偏移逻辑应用；
- 语义：未锚定子控件为"自由布局"——父容器 resize / 布局重排时保持自身 rect（不跟随）；与 per-column/per-item 稀疏样式家族一致；
- `AnchorInfo` 默认值（top-left）仅在显式命中且未指定时使用（保持解析缺省语义）。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | 容器已为 anchor 布局：运行期 `SetEnum("anchor","bottom-stretch")` 后子控件 rect **立即**变化（无需切布局/父 resize） | test_property_cabi（GetRect 前后对比） |
| 2 | 运行期 `SetFloat("anchor-offset-x")` 立即生效；读回一致 | 同上 |
| 3 | 稀疏：切 `layout=anchor` 后，**未锚定**子控件 rect 不变；仅显式锚定者重排 | test_property_cabi 断言（rootPanel 多子控件场景） |
| 4 | 现有 anchor 样例（全显式锚定）位置不变 | test_layout_advanced 回归 |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **一致性范围**：`child-flow-weight` 与 `child-grid-*` 运行期 setter 是否一并补 reflow？（建议一并——否则流式/网格运行期调整同样不即时生效）
2. **稀疏未锚定者的 resize 语义**：保持自身 rect（纯自由布局，**推荐**）？还是"仅切换瞬间保持、之后跟随父 resize"（无锚点无法定义跟随方式，不推荐）？
3. **触发点**：在属性 setter 处分派 reflow（推荐，解析路径不变）？还是让 `setChildAnchorProps` 内部触发（解析期逐子重排，不推荐）？
4. **设计器过渡**：`forceReflow` / `syncModelsFromEngine` 过渡代码在同步后删除——确认？

## 5. 配套改动

- 代码：`src/Panel.cpp`（运行期 setter 触发 reflow，锚点/流权重/网格）、`src/LayoutEngine.cpp`（applyAnchor 稀疏）；
- 测试：test_property_cabi 扩展（即时重排 + 稀疏断言）、test_layout_advanced 回归；
- 文档：declarative-syntax 6.5（锚点运行期即时生效 + 稀疏语义/未锚定者不随 resize）；设计文档标注 + 复核归档 + make_release + 同步。
