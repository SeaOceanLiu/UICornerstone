# CR20261110002 — StatusBar / ContextMenu action 在 C++ Binding 导出运行时被丢弃（分析 + 设计）

> 状态：**已关闭（2026-11-10；设计器联调通过）**——Binding thunk 允许 null sender
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261110002）
> 关联：CR20261109002（StatusBar item action）、CR20261109003（ContextMenu item action）、C++ Binding `RegisterAction`

## 1. 问题与根因

导出 C++ 工程中，StatusBar item 的 `events.onClick` 已在 LayoutParser 中查到 handler；C ABI `status-item-click` 正常，但 `events.c` 的 action 不执行。

根因在 **C++ Binding 的 action thunk**：

```cpp
// binding/src/UICornerstone.cpp
if (!ctl || !t) return;                 // ← StatusBar/ContextMenu 传 nullptr 时直接丢弃
auto state = std::make_shared<ControlState>();
state->instance = t->inst;
state->handle = ctl;
t->cb(Control(std::move(state)));
```

- `LayoutParser` 对 StatusBar / ContextMenu item action 按现有语义调用 `handler(nullptr)`（它们不是完整的 `Control` sender）；
- C ABI `UICornerstone_RegisterAction` 的 `UIActionCallback` 允许 `ctl == nullptr`，C 导出宿主直接调用 `events.c`，所以 C ABI 路径正常；
- C++ Binding 的 thunk 却把 null 视为错误并提前返回，导致导出 C++ 工程的 action 永远不执行。
- ContextMenu item action 同样受影响。

这不是 StatusBar 绑定链或 id 问题；DLL `dab958f…` 已包含 CR20261109002 代码。

## 2. 方案选择

### 方案 A（采用）：Binding thunk 允许 null sender

修改 thunk：

```cpp
if (!t) return;
auto state = std::make_shared<ControlState>();
state->instance = t->inst;
state->handle = ctl;       // 可为 null：action 无控件 sender
t->cb(Control(std::move(state)));
```

- `Control` 是 invalid（`IsValid()==false`、`Handle()==nullptr`），但 action 仍被调用；
- exported `main.cpp` 的 wrapper `onXxx(c.Handle(), nullptr)` 会把 `nullptr` 传给 `events.c`，与 C ABI 语义一致；
- 不在 LayoutParser 里伪造非空 sender，避免捕获/生命周期环（StatusBar 拥有 StatusItem，MenuItem 自身持有 onClick）。

### 方案 B（排除）：LayoutParser 传非空 Control

- StatusBar 需要构造非拥有 `shared_ptr<Control>` 或捕获 `bar`，可能形成 StatusItem→lambda→StatusBar 环；
- ContextMenu item 需要 weak 捕获，增加生命周期复杂度；
- 且不能覆盖未来其它“无 Control sender”的 action；
- 仅修两个调用点，不如绑定层统一允许 null。

### 方案 C（排除）：导出端 wrapper 判断

只能修生成的 C++ main，不修 Binding 本身；用户自行注册 Action 仍会踩坑。

**定稿：方案 A。**

## 3. 语义与兼容

- `ActionCallback` 可能收到 invalid `Control`；`Handle()` 为 null，所有属性/操作调用静默无效；
- 普通 Button/MenuBar 等 action 仍收到有效 `Control`，行为不变；
- C ABI `UIActionCallback` 仍收到 null，行为不变；
- 旧导出工程无需改 events.c/main.cpp，重新链接新 Binding 即可；
- 不新增 C ABI/Binding API；不需要 LayoutParser 改动；
- 文档需明确：StatusBar/ContextMenu item action 无控件 sender，回调必须容忍 null/invalid Control。

## 4. 测试/验收

1. **C++ Binding StatusBar**：RegisterAction → LoadLayout → 点击 status item，callback 调用一次，`Control.Handle()==nullptr`；
2. **C++ Binding ContextMenu**：右键/点击菜单项，callback 调用一次；
3. **C ABI 回归**：`status-item-click` 与 action 仍正常；
4. **普通控件**：Button/MenuBar action 仍收到有效 handle，C++ wrapper 行为不变；
5. **null Control 安全**：action 内调用 `ctl.IsValid()/GetId()/GetString()` 不崩溃、返回空；
6. 导出 `Temp/First Project.csd` 的 C++ 工程实测 status_1 Sector1/Sector2 printf；
7. 回归 `test_contextmenu`、`test_statusbar`、`test_menu`、`test_window_visibility`、`auto=2`/`auto=3`/strict。

建议新增 Binding 侧测试（可复用导出工程或 `test_cr_binding_null_action`），避免只测 C ABI。

## 5. 文档/交付

- `docs/appendix/binding.html` 的 `RegisterAction` 说明补：
  - StatusBar/ContextMenu item 等无控件 sender 的 action 会传入 invalid `Control`（`Handle()==nullptr`）；
  - 回调内使用 Control 前应 `IsValid()` 或容忍 null。
- `binding/include/UICornerstone.h` `ActionCallback` 注释同步；
- C ABI 文档标注 `UIControlHandle` 可为 null（已有语义，补文字）；
- 无 schema/属性变更。

## 6. 风险

| 风险 | 处理 |
|---|---|
| 旧回调假设 Control 一定有效 | 这是原先“静默丢弃”的根因；文档要求容忍 null；状态栏/菜单项本无控件 sender |
| 普通 action 收到 null 误调用 | 仅当引擎 sender 为 null 时才如此；Button/Menu 等仍非空 |
| Binding/引擎版本混用 | 同批配套；不新增 ABI 版本，旧核心仍传 null |
| 重复回调 | 不改变 LayoutParser 调用次数；只修复 thunk 丢弃 |

## 7. 实施顺序

1. Binding thunk 允许 null + ControlState 构造；
2. Binding 注释/文档；
3. C++ Binding 导出工程回归 + 新增测试；
4. 全量回归、Release 同步。

## 修订记录

- 2026-11-10 初稿：定位 Binding thunk `if (!ctl...) return`；采用允许 null sender 方案，避免 LayoutParser 生命周期改动；补 C++ Binding/ContextMenu 测试与文档。

## 8. 实施记录（2026-11-10）

- `binding/src/UICornerstone.cpp` 的 action thunk 已移除 `!ctl` 提前返回；null sender 构造 invalid Control 后仍调用 `t->cb`。
- `binding/include/UICornerstone.h` 补 `ActionCallback` 的 invalid Control 注释。
- C ABI `UICornerstone_RegisterAction` 行为不变（仍允许 NULL ctl）。
- Binding Debug 库编译通过；全量回归 **59 OK + 4 项预存失败**（新增 CR 测试包含 ContextMenu/StatusBar）。
- Release 已同步，DLL SHA256 `a227d8110a63978dca885809f127b3b66c40d22016b5573be71cdeeb0df5561f`；test_cr_contextmenu_dispatch/test_cr_statusbar_runtime 通过。
- 联调（2026-11-10）：导出 First Project.csd StatusBar/ContextMenu action 通过；testBtn 右键菜单通过；CR 设计器/引擎均 closed。
