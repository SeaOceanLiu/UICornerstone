# P060_AnchorReflowSparse_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-03
- 对象：`design/P060_AnchorReflowSparse_Design.md`（P0-60，状态：待评审）
- 结论：**通过，放行实施**（附 1 项低优先建议：offset-only 条目激活锚定语义）

## 1. 评审要点

### 现状核实（§1）✓（源码抽验一致）

- ① `Panel::setEnumProperty(kChildAnchor)`（Panel.cpp:180-190）/ `setFloatProperty(kChildAnchorOffsetX/Y)`（:140-155）更新 map 后无 reflow ✓；`setChildAnchorProps` 纯存储 ✓。
- ② `AnchorLayout::applyAnchor`（LayoutEngine.cpp:312-320）未命中即默认 top-left 并强制写 rect（非稀疏）✓。
- 同族审计（`child-flow-weight` :131-139 / `child-grid-*` :158-177 同样仅存储不重排）✓。

### 兼容评估抽验 ✓

- `test_layout_advanced` anchorPanel **6 子控件全部显式锚定**（top-left/top-right/bottom-left/bottom-right/center/**fill**）→ 稀疏化零影响，与设计评估一致 ✓。

### 方案（§2）✓

- setter 补 `reflowChildren()`（无引擎早退零开销）+ 解析路径不变（统一一次重排）——最小且自洽 ✓。
- `applyAnchor` 未命中 `continue`（保持现有 rect）——稀疏语义与 per-column/per-item 家族一致 ✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | flow-weight / grid-* 运行期 setter 一并补 reflow？ | **确认一并**（否则运行期流式/网格调整同样不即时生效） |
| 2 | 稀疏未锚定者的 resize 语义：保持自身 rect？ | **确认保持自身 rect**（纯自由布局；无锚点无法定义跟随方式） |
| 3 | 触发点：属性 setter 处分派 reflow（解析路径不变）？ | **确认**（解析期逐子重排不可取） |
| 4 | 设计器过渡：删 `forceReflow`；`syncModelsFromEngine` 保留？ | **确认**（forceReflow 删除；模型同步仍需保留——锚定重排会移动控件，模型必须跟随） |

## 3. 实施注意（放行附项）

1. **offset-only 条目激活锚定（低优先）**：`setFloatProperty(kChildAnchorOffsetX/Y)` 以 `m_anchorItemProps[child]` 默认构造创建条目（anchor=默认 top-left）——**仅设偏移**的未锚定子控件会被稀疏逻辑视为"已锚定（top-left）"并参与重排（跳到左上）。建议 `AnchorInfo` 增 `bool hasAnchor`（仅显式设过 anchor 才激活管理），或文档注明"偏移仅在已设锚点后生效"。设计器侧 UI 顺序（先锚定再调偏移）可降低触发概率。
2. **锚定档位补 `fill`**：schema `common.anchor` enum 实为 **14 值**（含 `fill` 四边拉伸，applyAnchor 已支持）——设计文档 §2.2 未列值集，非引擎问题；设计器下拉已补 `fill（四边）`。
3. 验收建议补：offset-only 不激活锚定（若采纳 `hasAnchor`）。

## 4. 放行

**结论：放行实施**。现状核实与兼容评估（anchorPanel 全锚定抽验）无误，方案（setter 即时重排 + 稀疏语义）最小自洽；附 1 项低优先建议。实施同步后设计器删 `forceReflow` 过渡（保留 `syncModelsFromEngine`），联测：底拉伸即时贴底 + 切布局不破坏未锚定控件 + 画布选中恢复。
