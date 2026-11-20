# CR20261119001 — TreeView 行内编辑器点击事件未派发给编辑器（分析 + 设计）

> 状态：**closed（2026-11-20 设计评审/代码复核通过；设计器单父回归通过；引擎项 closed）**
> 来源：`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_2026-11.md` → CR20261119001
> 关联：CR20261117002（TreeView 行内编辑器）、`TreeView::handleEvent`、`ControlImpl::handleEvent/dispatchToChild`
> 说明：本 CR 由设计器联调发现后，引擎先完成修复并交付；本文档为事后补记的分析/设计，供评审确认并作为审计记录。

## 1. 现象与影响

- 设计器属性面板 TreeView 的行内编辑器（CheckBox/EditBox 等）在**单父**（仅挂 TreeView）时：
  - CheckBox 点击不切换；
  - EditBox 点击不聚焦、无法输入；
  - 控件绘制正常、程序化 Set/Get 正常。
- 设计器此前把编辑器同时保留在 `m_propDynamic` children（双父），父容器可命中并派发，因此问题被掩盖；CR20261117002 修复单父后暴露。

## 2. 根因

`TreeView::handleEvent` 的 MouseDown 分支：

```cpp
if (node->editorControl && node->editorControl->isContainsPoint(x, y)) {
    return false;   // ← 直接返回，未派发子控件
}
```

- `TreeView` 返回 false 后，事件回到父容器（`propDynamic`）；
- 父容器 `ControlImpl::handleEvent` 只会派发给自己的 children；
- 编辑器现在只挂在 TreeView children，父容器无法转交；
- 因此编辑器收不到 MouseDown（MouseUp 同样无法形成完整点击）。

`leadingControl` 分支存在同构问题（返回 false）。

## 3. 目标与非目标

### 3.1 目标

1. 点击行内编辑器时，事件在 TreeView 内部派发给对应子控件；
2. 编辑器完成自身交互（CheckBox 切换、EditBox 聚焦/光标/拖选）；
3. 不改变树选中行、不触发展开/折叠、不进入双击标签编辑；
4. 修复 `leadingControl` 同构问题；
5. 不新增 C ABI/Binding/JSON/schema 字段。

### 3.2 非目标

- 不改变编辑器布局/宽度/行高（CR20261117002）；
- 不改变树行点击/箭头/双击标签编辑语义；
- 不重新设计双父/单父归属（设计器负责单父约束）；
- 不新增“编辑器事件单独回调”。

## 4. 设计

### 4.1 派发路径

`TreeView::handleEvent` MouseDown 命中编辑器时：

```cpp
ControlImpl::handleEvent(event);   // 内部 dispatchToChild → 编辑器 handleEvent
return true;                       // 已消费；不选中行/不展开
```

- `ControlImpl::handleEvent` 会先处理右键菜单/可见性，再 `dispatchToChild(event)`；
- 编辑器在 TreeView children 中，坐标命中后收到 MouseDown；
- 编辑器按下时自行 `setFocused(true)`/捕获（EditBox/CheckBox 既有逻辑）；
- 后续 MouseUp 走 TreeView 末尾的 `ControlImpl::handleEvent(event)`，通过 captured child 派发，形成完整点击。

### 4.2 leadingControl 一致性

```cpp
if (node->leadingControl && leading->isContainsPoint(x, y)) {
    selectNode(node->id);              // 保持“点击前置控件选中行”
    ControlImpl::handleEvent(event);   // 派发给 leading 控件
    return true;                       // 消费，避免父容器再派发
}
```

- 语义与旧版“选中行 + 控件交互”一致；
- 单父下不再依赖父容器。

### 4.3 事件边界

| 场景 | 行为 |
|---|---|
| 编辑器不可见/禁用 | `dispatchToChild` 不派发；返回 true 消费，避免误选中行 |
| 编辑器与行/箭头重叠 | 先判断编辑器命中，优先编辑器 |
| 编辑器在滚动/裁剪外 | `isContainsPoint` 使用实际 drawRect，不命中则走行逻辑 |
| MouseUp | TreeView 末尾 `ControlImpl::handleEvent` 派发 captured child |
| 双击标签编辑 | MouseUp 分支已排除 `editorControl` 区域，不进入 `beginEditNode` |
| 树选中行 | 编辑器分支不调用 `selectNode`，保持原选中 |
| 右键 | 不在此分支处理，沿用既有右键菜单路径 |

### 4.4 备选方案

1. `dispatchToChild(event)` 直接调用：可行，但绕过 `ControlImpl::handleEvent` 的右键/可见性统一入口；选择用后者。
2. 返回 false 让父容器转交：不可行（父容器无 TreeView 子控件路径）。
3. 设计器维持双父：临时兼容，违背单父约束，长期易双绘/双事件；不采纳。

## 5. 影响面与 ABI

| 层 | 影响 |
|---|---|
| C++ | 仅 `TreeView::handleEvent` 行为；无类布局/vtable 变化（`ControlImpl::handleEvent` 既有 virtual） |
| C ABI/Binding | 无新增/变更 |
| JSON/Schema | 无 |
| 设计器 | 可移除双父兼容层，恢复单父；点击回归应通过 |
| 回归 | 树行选中/箭头/双击编辑/leading 点击需回归 |

## 6. 测试计划

新增 T28（`test_cr_treeview_editor`）：

1. 点击 CheckBox 编辑器 → check-state 切换；
2. 点击 EditBox 编辑器 → `getFocused()` 为真；
3. 上述点击前后 `getSelectedId()` 不变；
4. 点击编辑器外行区域 → 仍正常选中行（已补）；
5. leadingControl 点击、禁用/隐藏编辑器、滚动后可见编辑器（未单列，留给设计器真实面板回归/后续 T29）。

三后端各跑；SDL3 全量回归仅允许 4 项历史失败。

## 7. 实施记录（补记，2026-11-20）

- 修改：`src/TreeView.cpp` MouseDown 编辑器分支改为 `ControlImpl::handleEvent(event); return true;`；leading 分支改为 `selectNode + handleEvent + return true`。
- 测试：`test/test_cr_treeview_editor.cpp` 增加 T28；三后端 **29/29**。
- 回归：`test_treeview` Enhanced/JSON/Scale2x 全通过；SDL3 全量回归仅 4 项历史失败；`validate_layout --strict` PASS。
- 交付：Release DLL SHA256 `102c260f24e5dc2867dfeabc5a52c0055eda79c232f7f98f829ef982b3d4d69e`；已同步设计器 `subModules/UICornerstone`。
- 设计器侧：等待移除临时双父兼容层并复跑点击回归。

## 8. 评审与代码检视过程（补记）

1. **设计器联调发现**：单父编辑器点击无效；设计器登记 CR20261119001，并临时恢复双父兼容层。
2. **引擎定位**：检视 `TreeView::handleEvent` 与 `ControlImpl::handleEvent/dispatchToChild`，确认 MouseDown 编辑器分支提前 `return false`，父容器无法转交 TreeView 子控件；`leadingControl` 分支同构。
3. **引擎修复**：按 §4.1/§4.2 修改；补 T28。
4. **引擎自检/回归**：三后端 29/29、`test_treeview`、SDL3 全量回归、strict；Record Release hash。
5. **设计器复核**（待）：移除双父兼容层，恢复单父，复跑 CheckBox/EditBox/选中行/leading 点击回归。
6. **设计/代码评审（2026-11-20）**：设计器复核通过（无阻塞）；确认 §4.1/§4.2 派发路径、影响面与“先实施后补设计”的审计记录。非阻塞项：T28 仅覆盖 CheckBox/EditBox/选中行（已补编辑器外行点击；leading/滚动/禁用场景留给真实面板回归）、文档“编辑器优先于箭头”已与代码顺序统一、MouseUp 无重复 `m_pressedRow`、测试夹具改为单父。
7. **流程偏差说明**：本 CR 属于联调阻塞缺陷，引擎先实施了修复，事后补本设计文档；设计已由设计器评审接受，后续 CR 仍按“先设计评审、后实施”执行。

## 9. 风险与回退

| 风险 | 处理 |
|---|---|
| 编辑器禁用/不可见仍消费事件 | 保持“优先编辑器、消费不选中”，文档明确；回归 |
| leading 点击行为变化 | 仍选中行 + 派发；`test_treeview` 回归 |
| 事件重复派发 | 编辑器分支 `return true`，父容器不再处理 |
| 编辑器捕获后 MouseUp 丢失 | TreeView 末尾 `ControlImpl::handleEvent` 走 captured child；T28 |
| 单父/双父兼容层冲突 | 设计器移除双父后复跑；引擎不依赖双父 |

## 10. 验收对照

| CR 验收项 | 设计落点 |
|---|---|
| 点击 CheckBox 切换 | §4.1 / T28 |
| 点击 EditBox 聚焦 | §4.1 / T28 |
| 树选中行不变 | §4.1/§4.3 / T28 |
| leading 同构修复 | §4.2 |
| 无 ABI/schema 变化 | §5 |
| 三后端/回归 | §6/§7 |

## 修订记录

- 2026-11-20 v1：补记 CR20261119001 分析/设计、实施记录、评审与代码检视过程。
- 2026-11-20 v2：设计评审通过；T28 补编辑器外行点击（三后端 29/29）；编辑器优先顺序与代码统一；记录设计器移除双父兼容层。
- 2026-11-20 v3（关闭）：设计器以最终 DLL `102c260f...` 复跑单父回归通过；CR 设计器/引擎两项 closed。
