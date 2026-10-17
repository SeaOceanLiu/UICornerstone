# P096_ReorderSelfContained_Design — 内置重排自含（reorderable 无需 draggable/droppable）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-12 追加：P0-96 中）
> 前置：P0-93/94 通用拖拽框架已实施并同步
> 状态：**已放行，已实施**（2026-10-17；复核：`requirements/P096_097_复核意见.md`——通过，一处必改）
> 复核修正（必改）：细化 2 判据 `NONE`→**`UI_DRAG_KIND_CONTROL`**——`captureDragPayload`（UICornerstoneAPI.cpp:1009）首行即置 CONTROL，NONE 仅捕获前存在（按原判据防护会完全失效）；已按复核意见实现。
> 实施备注（2026-10-17）：① 门控 `isDraggable() || isReorderable()` + CONTROL 判据取消（点击语义保留）；② `findDropTarget(owner,x,y,src)` 自目标回退（外部 droppable 优先）；③ 复核建议 1 采纳：tab 落点单轴（lx）计算 + clamp [0,n]；④ **实施中发现并修复**：hint→索引换算未补偿"摘除源"位移——tree 同父源在目标前时 BEFORE/AFTER 各偏移 1、tab 槽位未按"摘除后最终索引"换算（旧探针仅覆盖源在目标后场景未暴露）；已统一补偿 + 自落点（BEFORE/AFTER/INTO 自身）无操作守卫（不发 `reordered`）；⑤ 复核建议 2 采纳：自目标同时发 `drop`（target=源）+ `reordered`/`tab-reordered`——已记入 P093_094 §7.6 与用户手册 5.7.5；⑥ 复核建议 3：4 项预存失败 = `test_menu_cabi` / `test_multiviewport` / `test_multiviewport_cabi` / `test_treeview_cabi`（与本项改动面无关，回归保持仅此 4 项）；⑦ 探针 `Temp/temp_p096_097_probe` **8/8**；P0-93/94 探针回归 **8/8**（test 8 落点语义修正为 AFTER）；⑧ Debug/Release 零警告。

---

## 1. 复现与现状核实

**复现（设计器）**：勾选"可重排"后预览窗口拖页签**不换序**；需同时勾选"可拖拽/可放置"（设计器已加过渡耦合）才可用。

**源码核实（双缺口，与清单一致）**：

| # | 缺口 | 依据 |
|---|---|---|
| ① 起拖门控 | MouseDown 命中链仅 `isDraggable()` 进入 Pending → **纯 reorderable 永不起拖** | `src/UICornerstoneAPI.cpp:1163-1172` |
| ② 自目标解析 | `findDropTarget` 仅认命中链 `isDroppable()`；`applyBuiltInReorder` 要求 `s.source == s.target` → 纯 reorderable **无自目标** | `src/UICornerstoneAPI.cpp:1087-1097` / `:1108` |
| ③ 意图 | P0-93/94 设计 §3：`reorderable` = 引擎内置拖动（自含）；用户手册 5.7.5 亦按"仅置 reorderable"描述 → **实现与文档不符** | `design/P093_094_DragDrop_Design.md` / `docs/advanced/drag-drop.html` |

结论：当前内置重排实际需 **reorderable + draggable + droppable 三旗标**（引擎探针因同设三旗标未暴露）。

## 2. 方案（建议 a + 一处细化）

1. **起拖门控**（`DragPreHandleEvent` MouseDown）：命中链接受 `isDraggable() || isReorderable()`；
2. **细化：载荷空取消**——捕获载荷后，若源 `!isDraggable() && s->kind == UI_DRAG_KIND_CONTROL` → `dragReset` 并返回 false（不进入 Pending）。防止"从 tab 页体/树空白/列表空白"误起拖（仅 reorderable 的源只有真正命中可重排项才起拖；`drag-data/drag-userdata` 的 HOST 覆盖不受影响）；
3. **自目标解析**（`findDropTarget` 增 `Control* src` 参数）：
   - 命中链先找 `isDroppable()` → **外部目标优先**（保持既有语义）；
   - 未命中且 `src` 为 `isReorderable()` 且命中链包含 `src` 自身 → 返回 `src`（自目标）；
   - `MouseMove` 调用处传入 `s->source`；
4. `applyBuiltInReorder` / `computeDropHint` **无需改动**（已支持 source==target；tree/list/tab 的 hint 计算对自身同样适用）。

## 3. 行为矩阵（修后）

| 源旗标 | 光标位置 | 结果 |
|---|---|---|
| reorderable-only | 源可重排项上拖动 | 自目标 → 内置换序（`tab-reordered`/`reordered`）✓ |
| reorderable-only | tab 页体 / 树·列表空白 | 不起拖（细化 2）或拖起后无目标 → 无换序 ✓ |
| reorderable + 外部 droppable | 拖到外部 droppable 控件 | **外部目标优先** → `drop` 事件；无内置换序 ✓ |
| reorderable + droppable（自身） | 源上 | 命中链 droppable=自身 → 自目标 ✓（与兜底一致） |
| draggable-only | 任意 | 行为不变（外部 drop / 无目标中止）✓ |
| 三旗标（设计器过渡现状） | 任意 | 与修后语义一致，无回归 ✓ |

安全守卫（既有）：`TreeView::moveNode` 自父/后代循环守卫（`src/TreeView.cpp:577-592`）→ 自拖 INTO 自身后代返回 false、不发 `reordered` ✓。

## 4. 验收与测试矩阵

1. **tab reorderable-only**：起拖（阈值）→ 自目标 INDEX → `tab-reordered` + 顺序变化；
2. **tree reorderable-only**：BEFORE/AFTER/INTO 三分带换序/换父 → `reordered`；INTO 自身后代 → 无事件；
3. **list reorderable-only**：行换序 → `reordered`；
4. **负例**：tab 页体起拖不触发 drag-start；树空白起拖不触发；阈值内点击不触发（既有）；
5. **回归**：P0-94 探针 8 用例全过（含外部 droppable、拒绝、Esc、公开 API）；全量回归仅 4 项预存失败。

## 5. 影响面与文档

- 改动面：`src/UICornerstoneAPI.cpp`（门控 + 目标解析，局部）；无 ABI/schema 变更；
- 用户手册 5.7（已按自含描述）与实现对齐，**无需改**；`design/P093_094_DragDrop_Design.md` 实施备注补一行；
- 设计器：实施同步后**可删过渡耦合**（勾选 reorderable 同步写 draggable+droppable 的代码与 `modelPropsJson` 补键）——保留亦无冲突（三旗标语义兼容）；
- 风险：低。唯一注意点=门控放宽后，reorderable 源在"载荷 NONE"场景由细化 2 兜住。

## 6. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 起拖门控细化：reorderable-only 且载荷 NONE → 取消 Pending（防页体/空白误起拖） | 采纳（建议） |
| Q-2 | 自目标判定：命中链包含源自身（光标在源子树内）才视为自目标；源外空白 → 无目标 | 确认 |
| Q-3 | 外部 droppable 优先：reorderable+外部目标时发 `drop` 不发内置换序 | 确认（保持既有） |
| Q-4 | 设计器过渡耦合实施后可删 | 确认（可删；保留兼容） |
