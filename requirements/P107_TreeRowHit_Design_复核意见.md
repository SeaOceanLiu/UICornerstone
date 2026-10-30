# P107_TreeRowHit_Design — 设计复核意见

> 评审方：CornerstoneDesigner（设计器侧）
> 评审对象：`D:\GitSpace\UIControls\design\P107_TreeRowHit_Design.md`（2026-10-21 待评审版）
> 关联需求：`Temp/UICornerstone_配合修改清单_第三批.md` → P0-107（中）
> 结论：**可批，附实现约束**（补边界判断与缓冲语义后实施）

---

## 1. 结论摘要

方案①（只读 `TreeViewGetNodeAt`，不做右键自动选中）符合设计器预期 ✓。右键流程明确为：`GetNodeAt(光标)` → 命中非空才弹菜单 → 动作前 `TreeViewSelectNode` + 模型选中 ✓。

需要补充两点实现约束，否则不满足文档自身的验收第 4 条：

1. 现有 `hitTestRow` 不判 x、不判边界 ✗——坐标在树外/滚动条上也会返回行；
2. `cap=128` 截断会产生"静默命中错误节点"✗——建议返回所需长度或由 binding 重试。

---

## 2. 事实核对

| # | 设计文档表述 | 核实结果 | 证据 |
|---|---|---|---|
| ① | TreeView 仅左键选中 | 成立 ✓（右键无选中逻辑） | `src/TreeView.cpp:446` |
| ② | `hitTestRow` 窗口坐标、含滚动偏移与 stride；`getNodeAtRow` 取 flat 行节点 | 基本成立，但**忽略 mx 且无边界判断** ✗ | `src/TreeView.cpp:527-534`（`(void)mx;`）；`546-549` |
| ③ | C ABI 无坐标→节点查询 | 成立 ✓ | `include/UICornerstoneAPI.h` |
| ④ | 绘制/命中口径一致（无变行高问题） | 成立 ✓：绘制按统一 `stride` 排布（`getStride()`），单行字体只影响垂直居中，不影响行高 | `src/TreeView.cpp:218-228`；`src/TreeView.h:153` |
| ⑤ | 坐标口径 = 窗口全局坐标 | 成立 ✓：设计器右键坐标来自 SDL `getMouseState`（窗口坐标，与 MouseDown 同源） | `src/App.cpp:216-231` |

---

## 3. 实现约束（必须）

1. **边界判断**：CABI impl 在调用 `hitTestRow` 前必须做：
   - x/y 位于树内容区（树矩形**扣除垂直/水平滚动条**，与绘制裁剪区 `cr` 口径一致）；
   - y ∈ [0, `m_flatRows.size() × stride`)（`hitTestRow` 之外的尾部空白返回 0）；
   - 不满足 → 返回 0（binding 空串）。
2. **共享内部函数**：把"内容区包含判断 + hitTestRow"抽成内部 helper，MouseDown 与新 API 共用，避免后续口径漂移。
3. **缓冲语义**：`cap` 不足时**返回所需长度**（snprintf 语义）或提供长度查询；binding 先查后取。最小缓冲建议 ≥256 字节。不得静默截断后当作完整 id（设计器 id 用户可编辑，截断会导致动作作用到错误节点）。
4. **DynamicApi 兼容**：`fnTreeViewGetNodeAt` 缺失（旧 DLL）时 binding 返回空串，不崩溃。
5. **纯查询**：不改变选中/悬停/展开状态（与文档 2.3 一致 ✓）。

---

## 4. 验收矩阵补充

在文档第 3 节基础上增加：

1. **缩进/箭头区域**右键 → 仍命中该行（x 不参与行判定，但须在内容区内）；
2. **垂直滚动条区域**右键 → 未命中（0）；
3. **最后一行下方空白** → 未命中；
4. **折叠/展开后**同一点命中的可见行正确；
5. **滚动非整行偏移**（`m_scrollOffset` 非 stride 整数倍）命中正确；
6. **DPI 缩放 ≠ 1.0** 下命中正确（窗口坐标换算）；
7. **空树** → 0；
8. **`cap` 过小** → 按第 3.3 条约定返回所需长度/空串，不产生截断 id。

---

## 5. 设计器侧落地计划（随 DLL 同步）

1. 右键流程：`pollRight` → 树矩形内先 `TreeViewGetNodeAt` → 空串则忽略（含滚动条区域）→ 非空记录目标 id 并弹菜单；
2. 动作前：`selectItem(id)` + `TreeViewSelectNode(m_canvasTree, id)`（`m_treeSync` 守卫），保证模型/树选中一致；
3. `@bench`（画布根节点）命中 → 菜单按"画布"处理（删除/上移/下移禁用或忽略，移到根不适用）；
4. 现有 `treeDelete/treeMoveInParent/treeMoveToRoot` 改为作用于"右键行 id"而非 `m_selIds.front()`；
5. 回归：右键不同行、滚动后右键、折叠后右键、右键空白不弹菜单。

---

## 6. 待引擎确认

| # | 问题 | 设计器建议 |
|---|---|---|
| Q-A | `cap` 语义（返回所需长度 vs 长度查询） | 返回所需长度（snprintf 语义）最简 |
| Q-B | 是否允许 x 位于缩进/空白但仍属内容区 | 允许（与现左键选中一致） |
| Q-C | 右键是否顺带更新 hover 行 | 不要求；设计器不依赖 |
