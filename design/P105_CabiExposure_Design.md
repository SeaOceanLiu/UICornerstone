# P105_CabiExposure_Design — 控件树交互所需 C ABI 暴露（拖拽落点访问器 + ContextMenu 条目句柄）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-20 追加：P0-105 中）
> 前置：P0-94（拖拽框架）/ P0-98（click 统一通路）/ P0-103（click 延迟一帧）已实施
> 状态：**已放行，已实施**（2026-10-20；复核：`requirements/P105_复核意见.md`——通过，附 2 条小建议）
> 实施备注（2026-10-20）：① **binding `Event` 拖拽访问器全套落地**（`IsDragStart/Over/Drop/DragEnd/Reordered/TabReordered` + `GetDragKind/Id/Index/Text/UserData/Source` + `GetHintKind/Index/TargetId` + `GetDropped`；零 ABI 变更）；② **采纳复核建议 1**：`GetNodeId()` 增事件名守卫（仅 select/expand/collapse 有效，其他事件返回空串——防 drag 联合体未定义值）；③ **`ContextMenuAddItemEx`**（CABI `UIControlHandle` + Binding `Control`，保留旧函数）→ `SetCallback("click")` 统一通路（P0-98/103 延迟语义）；④ **采纳复核建议 2**：验收探针同时断言 `GetDragId()=='n1'` 与 `GetNodeId()==''`；⑤ 验证：探针 `temp_p105_probe` **3/3**——drag accessors（dragId='n1'/hintKind=AFTER(3)/target='n2'/dropped=1/nodeId=''）/ ContextMenuAddItemEx（句柄有效+click 延迟一帧）/ 旧 API 兼容（1）；P0-93/94、96/97、98/99/100、101、102、103、104 探针全过；全量回归 **51 OK + 仅 4 项预存**；Debug/Release 零警告。

---

## 1. 需求与现状核实

**背景**：设计器下一步实施"控件树右键 + 拖拽调整父子（P2）"，模型同步受阻于两处 C ABI 缺口（C++ 层均已具备）。

| # | 设计器声明 | 核实结果 |
|---|---|---|
| ① | 拖拽/重排事件缺落点字段（C ABI 未携带） | **部分不准确**：C ABI `UIEventData.data.drag.hint.{kind,index,targetId}` + `dropped` **已完整携带**（`ControlBase.cpp` fireCCallback Drag 映射 ✓，P0-94 已交付）；**真实缺口在 binding `Event`**——无任何拖拽访问器（连 `drag.payload` 都没有）；`GetNodeId()` 读 `treeNode.id` 联合体成员，对 drag 事件为**未定义值** ✗ |
| ② | ContextMenu 条目无回调通路 | ✓ 属实：C ABI `UICornerstone_ContextMenuAddItem`（API:687）**返回 0/1、条目句柄丢失**（impl：创建 MenuItem → `ContextMenu::addItem(shared_ptr)` → 句柄不可达）✗；引擎 C++ `ContextMenu::addItem(caption, onClick)` 有回调 ✓ 但未经 C ABI 暴露 ✗ |
| ③ | TreeView 行命中（可选） | C++ `hitTestRow` 已 public ✓；C ABI 无（设计器标注"如后续需要再议"——**本批不扩面**）✓ |

**模型回写可行性**：`reordered` 的新父节点可由 `hintKind + hintTargetId + hintIndex` 结合宿主树模型推导——BEFORE/AFTER：新父=`getParentId(hintTargetId)`、索引=`childIndex(hintTargetId)+(AFTER?1:0)`（含源摘除位移补偿，引擎已按此执行）；INTO：新父=`hintTargetId`。**暴露上述字段即可完整回写** ✓。

## 2. 方案

### 2.1 binding `Event` 增拖拽访问器（C ABI 不变）
```cpp
// 事件鉴别（按 eventName）
bool IsDragStart() / IsDragOver() / IsDrop() / IsDragEnd() / IsReordered() / IsTabReordered() const;
// 载荷（drag.payload）
int GetDragKind() const; std::string GetDragId() const; int GetDragIndex() const;
std::string GetDragText() const; void* GetDragUserData() const; UIControlHandle GetDragSource() const;
// 落点（drag.hint）+ dropped
int GetHintKind() const; int GetHintIndex() const; std::string GetHintTargetId() const; int GetDropped() const;
```
- 全部经 `m_raw->data.drag.*` 读取（零 ABI 变更）；文档注明"新父节点推导口径"（见 §1）；
- `IsReordered()/IsTabReordered()` 按事件名判断（与既有 `IsClick()` 同款风格）。

### 2.2 ContextMenu 条目句柄（新增 Ex，保留旧函数）
```c
/* P0-105：创建条目并返回句柄（失败 NULL）；宿主随后 SetCallback(item, "click", ...) 绑定动作
   （P0-98 统一通路，P0-103 延迟一帧语义）。旧 ContextMenuAddItem 保留（0/1 兼容）。 */
UICORNERSTONE_API UIControlHandle UICornerstone_ContextMenuAddItemEx(
    UIInstance instance, UIControlHandle menu, const char* caption, const char* shortcut);
```
- 引擎 impl：与旧函数同构造（菜单面板持有 → 句柄有效），返回
  `reinterpret_cast<UIControlHandle>(static_cast<Control*>(item.get()))`；
- Binding：`Control ContextMenuAddItemEx(Control& menu, const std::string& caption, const std::string& shortcut = "")`（无效返回空 Control；旧 bool 版保留）；
- 动态动作绑定：`item.SetCallback("click", ...)` ✓（引擎 P0-98 动态项 click + P0-103 延迟语义均已验证）。

### 2.3 不扩面（Q-3）
- `TreeView::hitTestRow` 的 C ABI 暴露不在本批（设计器内部树交互用内置换序 + 自有命中即可）。

## 3. 验收与测试矩阵

1. **拖拽落点访问器**：tree `reorderable` 内置换序 → `reordered` 回调内：`GetDragId()==源节点`、`GetHintTargetId/Index/Kind` 与 `Raw()->data.drag.hint.*` 一致、`GetDropped()==1`；INTO/BEFORE/AFTER 三分支各断言；
2. **ContextMenuEx**：`ContextMenuAddItemEx` 返回非空句柄 → `SetCallback("click")` → 合成点击 → 回调触发（**延迟一帧**）+ 菜单关闭；`shortcut` 生效；
3. **兼容回归**：旧 `ContextMenuAddItem` 返回 1、条目可点击（无回调）行为不变；
4. 全量回归仅 4 项预存；P0-93/94/96/97/98~104 探针回归。

## 4. 影响面与文档

- 源码：`binding/include/Event.h` + `binding/src/Event.cpp`（访问器）、`include/UICornerstoneAPI.h`（Ex 声明）、`src/UICornerstoneAPI.cpp`（Ex impl）、`binding/include/UICornerstone.h` + `binding/src/UICornerstone.cpp` + `binding/src/DynamicApi.*`（Ex 解析/封装）；
- 无 JSON/schema 变更；C ABI：**仅新增**（旧函数不动）；
- 文档：capi/binding 速查（Ex 行 + 拖拽访问器）、用户手册 menu 页（右键菜单动态条目示例）、4.14/5.7 交叉引用；API_Mapping_Table 补行。

## 5. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | ContextMenu 条目句柄：新增 Ex（兼容）vs 修改旧函数返回值 | **新增 Ex**（ABI 兼容，旧调用零改动） |
| Q-2 | 拖拽访问器范围：完整 payload + hint + dropped | 采纳（设计器模型回写所需全集） |
| Q-3 | TreeView hitTestRow C ABI 本批不做 | 采纳（不扩面） |
