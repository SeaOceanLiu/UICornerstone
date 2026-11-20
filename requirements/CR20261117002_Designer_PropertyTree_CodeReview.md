# CR20261117002 设计器属性面板 TreeView 迁移 — 代码检视问题列表

- 检视对象：`D:/GitSpace/CornerstoneDesigner/src/CanvasPane.cpp` / `include/Designer/CanvasPane.h`（2026-11-19 工作区）
- 检视方：UICornerstone 引擎
- 范围：`ensurePropTree` / `setPropTreeMode` / `buildPropertyTree` / 行池与展开折叠路径
- 结论：接口与联调结果符合预期，但发现 **2 个高优先级 + 3 个中优先级**问题，建议修复后再作为属性面板长期方案。

## P1（高）

### P1-1 编辑器控件同时挂在两棵树（双父/双绘），切换平铺模式后父链不确定

- 位置：
  - 行池创建：`CanvasPane.cpp:4133-4145`，首次创建时 `m_propDynamic.AddChild(c)`；
  - 树挂接：`CanvasPane.cpp:3666` `TreeViewSetNodeEditorControl`；
  - 引擎：`TreeView::syncRowControls()` → `ControlImpl::addControl()` 只把控件加入 TreeView children，并 `setParent(tree)`，**不会从原父移除**。
- 现象/风险：
  - 同一编辑器控件同时存在于 `m_propDynamic` 与 `m_propTree` 的 children；
  - 树模式下一帧可能被两个父各绘制一次（同坐标时不易发现），事件/命中也可能重复；
  - `setPropTreeMode(false)`（bench / 组件实例）只隐藏树，没有把编辑器重新挂回 `m_propDynamic`；`takePoolRow` 对已存在行只 `SetVisible(true)`，不会 `AddChild`，平铺模式依赖旧父链/旧 rect，容易被后续引擎 `setParent` 语义变化破坏。
- 附：`m_propTree` 本身由 `m_ui->CreateTreeView` 创建（引擎 C ABI 会 `bench->addControl`），随后 `m_propDynamic.AddChild(m_propTree)` 也形成双父；树可能被 bench 与属性面板各绘制一次。
- 建议：
  0. 创建后先从 bench 摘除 `m_propTree`（或引擎补“带父创建”入口）再挂到 `m_propDynamic`；
  1. 挂树前显式 `m_propDynamic.RemoveChild(editor)`，离开树模式时 `m_propDynamic.AddChild(editor)` 并重设 rect；
  2. 或在 `setPropTreeMode(false)` 统一遍历 `m_rowPool` 重挂到 `m_propDynamic`、更新 rect/visible；
  3. 引擎侧保持“借用不拥有”，但设计器必须负责单父约束（可加 debug 断言 `editor.GetParent()==m_propDynamic`）。

### P1-2 “折叠组只建标题/占位”与实现不符，全部字段/编辑器仍会创建

- 位置：`CanvasPane.cpp:4681-4708` 先完整生成 `m_dynamicRows`（含所有字段编辑器），`buildPropertyTree` 再遍历全部行；`CanvasPane.cpp:3698-3705` 对折叠组仅额外加 `ph:` 占位，真实子节点仍在循环中创建。
- 现象/风险：
  - 折叠只影响树显示，**没有减少行/编辑器创建**；属性多、TextArea 多时仍是全量创建成本；
  - 与交付说明“折叠组/图元只创建标题节点和占位子节点”不一致；
  - 展开时 `m_pendingPropRowsRebuild` → `rebuildDynamicRows()` 再次全量创建，性能收益不存在。
- 建议：
  - 若要懒加载：`rebuildDynamicRows` 在树模式下遇到折叠组/图元时跳过对应字段行创建，只登记“组元数据”；展开事件时仅生成该组行；
  - 或明确放弃懒加载并修正文档/交付说明，避免后续按错误前提优化。

## P2（中）

### P2-1 展开/折叠触发全量重建，树滚动位置与编辑器焦点丢失

- 位置：展开/折叠回调 `3592/3602` 置 `m_pendingPropRowsRebuild`；`7271-7275` 全量 `rebuildDynamicRows()`；`3635` `TreeViewClearItems`。
- 风险：属性面板滚动位置跳回顶部；正在编辑的编辑器被摘除/重挂，焦点与 IME 状态丢失；大数据量时展开/折叠卡顿。
- 建议：保存/恢复 TreeView 滚动偏移（引擎可补 `TreeViewGet/SetScrollOffset` 或宿主经 scrollBar 访问），只重建目标组子树；至少恢复焦点节点与滚动值。

### P2-2 `label-width` 固定 130，窄面板下编辑器可能被压成 0 宽

- 位置：`ensurePropTree` `3573` 固定 `kLabelWidth=130`；`syncDynamicWidth` `7220` 只更新树 rect。
- 风险：splitter 把属性面板拖窄到 < 130+gap 时，引擎 `editorWidth=0` 可用宽为 0，编辑器跳过绘制；用户看不到编辑控件，也无横向滚动兜底。
- 建议：按面板宽度动态设置 `label-width`（如 `min(130, max(72, w*0.45))`），或为编辑器设置最小可用宽并在不足时启用横向滚动/提示。

### P2-3 CheckBox 编辑器 caption 被清空后未恢复

- 位置：`attachEditor` `3660-3662`：树模式下把所有 `CheckBox` 编辑器 `SetString(kCaption,"")`。
- 风险：行池控件复用；切回平铺/其他属性行时若该行未重新写 caption，会显示空标签；同一控件也可能在树/平铺两种语义间串用。
- 建议：挂树前保存原 caption，离开树模式或复用行时恢复；或仅对树模式专用实例清空，不复用同一 CheckBox 作为平铺行。

## 非阻塞建议

- `keyByHandle` 仅覆盖 `m_rowPool`；若后续新增直接创建、未入池的 `m_dynamicRows`，这些行会被静默跳过。建议建树时对未命中行做一次诊断或统一入池。
- 组节点 id `pg:<组名>` 依赖组名唯一；若 schema 不同段出现同名组，可能冲突。建议使用稳定路径（如 `pg:<section>:<group>`）。
- “轮廓阴影 · X”挂到父组依赖父组先创建；建议建树前先按组层级排序或两遍创建。
- `attachEditor` 先 `SetNodeRowHeight` 再 `SetNodeEditorControl`，会触发两次 TreeView 全量 rebuild；可在一次性 API/延迟提交中合并（性能）。
- 折叠组占位节点 `ph:` 在展开到 tick 重建之间会短暂显示“…”；可改为树节点子项延迟生成或占位不可见。

## 建议验证（设计器侧）

- 正常控件（树模式）→ bench → 组件实例：平铺属性行仍可见、可编辑、无重复绘制；
- 折叠组展开/折叠后滚动位置与编辑器焦点保持；
- 窄面板（宽度 < 200）编辑器仍可用；
- 同一 CheckBox 行在树/平铺模式切换后 caption 正确；
- 大量字段/多 TextArea 下折叠组创建耗时与展开耗时对比（验证是否真正懒加载）。

---

本轮仅检视设计器代码并记录问题；未修改设计器文件，未改引擎代码。

---

## 复核结果（2026-11-19，设计器修复后）

已核对设计器修复代码：

- **P1-1 已闭合**：`attachPropTreeEditor` 先 `m_propDynamic.AddChild(editor)`（C ABI 会从旧父/TreeView 摘除）再 `RemoveChild`，再交给 TreeView；`setPropTreeMode(false)` 按 `m_treeAttachedHandles` 重挂回 `m_propDynamic` 并清空树节点；`m_propTree` 经 C ABI `AddChild` 从 bench 摘除，单父成立。
- **P1-2 已闭合**：`rebuildDynamicRows` 在折叠组/图元时跳过字段行创建（`m_collapsedGroups.count` / `m_collapsedPrims.count`），`emitPropertyTreeSpec` 仅登记标题 + `ph:` 占位；回归断言检查 `nud:font-size`/内嵌 alignment 行未入池。
- **P2-1 已闭合（增量路径）**：展开/折叠走 `m_treeDeltas` + `applyPropertyTreeDelta`，不再 `TreeViewClearItems`；跳过 `m_rowRefreshers`，保留焦点/未提交文本；回归以 `m_propTreeIdSeq` 递增确认增量。
- **P2-2 已闭合**：`propTreeLabelWidth` 随面板宽自适应（45%，72~130，窄面板下限 48），`syncDynamicWidth`/`setPropTreeMode` 更新。
- **P2-3 已闭合**：CheckBox caption 挂树前按句柄备份，离开树模式恢复；回归覆盖 playing→空→恢复。
- 非阻塞：未入行池行诊断、轮廓阴影父组顺序、`ph:` 同帧替换、非默认行高才调 `SetNodeRowHeight` 均已处理/说明。

### 残留（非阻塞）

1. `m_treeAttachedHandles` / `m_treeCaptionBackup` 在 `buildPropertyTree` 全量重建与增量 RemoveNode 后只增不减；被移除节点的编辑器句柄仍留在集合中，`setPropTreeMode(false)` 会把这些旧句柄一并重挂回 `m_propDynamic`。当前行池复用会随后隐藏未用行，影响有限，但建议在重建/摘除时按 `m_propTreeChildren` 同步裁剪，避免长期累积与无意义重挂。
2. `setPropTreeMode(false)` 只恢复 `m_treeCaptionBackup` 中仍存在的句柄；若编辑器在树模式期间被其他路径复用/清空，建议恢复后再按行池语义重建 caption，防止残留空标题。
3. 建议增加一次“树模式 → bench → 组件实例 → 普通控件”连续切换回归，确认单父/可见性/滚动条在多次切换后仍稳定。

结论：设计器修复已覆盖 P1/P2 主要问题，残留项不影响当前联调；引擎侧无需再改代码。

---

## 复核残留处理结果（2026-11-19）

- **残留 1 已处理**：新增 `pruneTreeEditorSets(attachedNow)`，全量建树/增量子树完成后按实际挂树编辑器裁剪 `m_treeAttachedHandles` / `m_treeCaptionBackup`；被移除编辑器重挂回 `m_propDynamic` 并恢复 caption。
- **残留 2 已处理**：`setPropTreeMode(false)` 先按备份恢复，再按 `poolCheckboxCaption(kind)` 重建 TreeView/树模式期间被复用的 CheckBox caption（`chk:*` / `chkshadow:enabled` / `emb:chk:*`）。
- **残留 3 已处理**：新增“树模式 → bench → 组件实例 → 普通控件 → 再切换”回归，校验树可见性与编辑器句柄集清空/重新挂接。
- 动态行 18 号字：`kPropLblFont=18`、`kPropLblH=26`，TreeView `font-size=18`，行池控件统一 18 号字，TextArea/items 行高 98/120；标签列 90~190；文档同步。

### 新的轻微观察（非阻塞）

- `pruneTreeEditorSets` 重挂被移除编辑器时未显式 `SetVisible(false)`，当前依赖 `hideUnusedPoolRows()` 先隐藏未使用行；若未来调整重建顺序，可能出现短暂“脱离树后仍可见”的残留。建议在 prune 中按 `m_dynamicRows`/`m_rowPoolUsed` 明确设置可见性，或统一由行池可见性收口。
- 增量子树的 `attachedNow` 由 specs 全量生成；若 `TreeViewAddNode` 失败导致某编辑器未实际挂树，仍会被视为 attached，prune 不会回收。建议以实际 `attachPropTreeEditor` 成功/`m_propTreeChildren` 为准构建集合。

结论：残留项已按计划处理，当前无阻塞；上述两点为健壮性建议，不影响联调。

---

## 最终复核（2026-11-20）

- 引擎修订 DLL `102c260f24e5dc2867dfeabc5a52c0055eda79c232f7f98f829ef982b3d4d69e` 已同步设计器 Debug/Release 构建目录。
- 设计器恢复单父后点击回归通过（CheckBox 切换/恢复、编辑器不选中行、EditBox 聚焦）；Debug/Release auto=2 399/0、auto=3 REPORT-OK、strict PASS。
- 引擎 T28 三后端 29/29；`test_treeview`、SDL3 全量回归、strict 通过。
- 剩余非阻塞：T28 未单列 leading/禁用/滚动场景；leading 仍按坐标派发全部子控件（文档已注明）；引擎测试文件未发现重复 printf（当前源码仅一处汇总输出，旧输出可能来自旧二进制）。
- CR20261119001：设计器 closed / 引擎 closed。
