# P086_087_Design — 菜单项 id 注册（FindControl）/ Binding SetCallback 空回调解绑

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-10 追加：P0-87、P0-86）
> 前置：P0-85 批已实施并同步（P0-84 全链结案）
> 状态：**已放行，已实施**（2026-10-10；Q-1~Q-4 确认；补充 1.1「空分支在 slot 前返回」已落实）
> 复核：`requirements/P086_087_复核意见.md`（通过；设计器侧独立核实含"无 nullptr 调用方"）
> 实施备注：① **P0-87**：`populateMenuPanel` 解析 item `id` 时同步 `m_controlsById[id] = item`（仅非空 id；子菜单经同一递归覆盖；ContextMenu items 同路径天然覆盖）；schema menus items 描述 + menu.html 注明"FindControl 可达 / 与控件 id 同表重名覆盖"；② **P0-86**：引擎 `ControlImpl::setCallbackProperty` 新增 **nullptr=解绑（erase，return 1）**；Binding `Control::SetCallback` 空函数分支**置于 slot 创建之前**（传 nullptr 解绑，防槽堆积）；capi/binding 速查注明；③ 验证：**P0-86/87 探针 OK**——87a) `FindControl(miUndo/miSubItem)` 非空；87b) enabled 点击触发（按 JSON onClick 路径设 m_onClick）；87c) `enabled=0` 读回 0 且点击忽略；86) fire=1 → unbind rc=1 → 解绑后不再触发（无崩溃）；④ 全量构建 0 错误、回归仅 4 项预存；⑤ **附带观察（未改，供参考）**：`MenuItem::handleEvent` 的 `fireCCallback(kEventClick)` 以 `m_onClick` 为门控——仅注册 CABI click 回调（无 C++/JSON onClick）时不触发；设计器路径经 JSON `events.onClick` 已设 m_onClick，不受影响；如需"CABI 回调独立可用"可另立项（一行调整：m_onClick 判空拆开）；⑥ LoadLayout 的 `controlsById` 合并点不清表（重复 LoadLayout 残留旧项句柄，生命周期由持有池兜底）——按复核 2.3 仅注明行为，非阻塞

---

## 1. P0-87 确认：菜单项 `id` 未注册进 id 表（FindControl 不可达）

**核实**：
- `LayoutParser::populateMenuPanel` 已解析 item `"id"` → `setItemId`（src/LayoutParser.cpp:1870-1873），但**未注册** `m_controlsById`（其余控件类型逐类注册：label/ani/shape/listview/tabcontrol/actor/button/editbox…，如 :483/:511/:631/:807/:880/:936/:1180/:1255）；
- `UICornerstone_FindControl` 查 `instance->controlsById`（API:1144-1145），该表由 LoadLayout 合并 parser 的 `getAllControlIds()/findControlById()`（API:1099-1102）→ 菜单项不在表中 → `FindControl("miUndo")` 返回空 ✓；
- 属性派发已通：`kEnabled → setEnable`（ControlBase.cpp:983）+ `MenuItem::handleEvent` 检查 `getEnable()` 忽略交互 + disabled 状态色生效 → **只需补注册**即可实现"撤销栈状态动态置灰"。

**方案**：
- `populateMenuPanel` 中解析 `id` 时同步 `m_controlsById[id] = item`（子菜单项经同一递归函数，天然覆盖）；
- 文档：`menus` items `id` 说明补"注册进实例 id 表，`FindControl` 可达"（schema 描述 + docs/controls/menu.html）。

## 2. P0-86 确认：Binding 空回调注册 thunk → bad_function_call

**核实**：
- `Control::SetCallback`（binding/src/Control.cpp:109-121）对**空 `std::function`** 仍注册 `CallbackThunk` + 引擎回调；触发时 `(*fn)(ev)`（:107）→ **`std::bad_function_call`** ✗（与设计器实测 VC 运行库异常一致）；
- **设计文表述修正**：设计器称"引擎语义=解绑（传 nullptr）"**不准确**——`ControlImpl::setCallbackProperty`（src/ControlBase.cpp:1027-1031）对 `cb == nullptr` **直接 return 0（不解绑、旧回调保留）**；各控件覆写（CheckBox/ColorPicker/ComboBox/Popup 等）均汇流该基类实现。

**方案**：
- **引擎**：`ControlImpl::setCallbackProperty`：`!event → 0`；`!cb → m_cCallbacks.erase(event); return 1`（**新增解绑语义**；覆写均汇流基类，自动生效）；
- **Binding**：`Control::SetCallback(event, callback)` 空函数 → 调 `fnSetCallback(instance, handle, event, nullptr, nullptr)`（解绑）并直接返回；非空维持现逻辑（slot 注册表保留——仅生命周期持有，无行为影响；Q-2）；
- **文档**：binding.html `SetCallback` 行注明"空 `EventCallback` = 解绑"；capi.html `SetCallback` 行注明"`cb=nullptr` = 解绑（P0-86）"；设计器可将"no-op 解绑"过渡改回空回调解绑（可选）。

## 3. 测试计划
- **P0-87**：引擎探针——LoadLayout 菜单（含子菜单项 `id`）→ `FindControl(id)` 非空且可 `SetBool("enabled",0/1)`；`enabled=0` 时点击项不触发 onClick（引擎驱动）且读回 0；恢复 1 正常；主/子菜单项各一；
- **P0-86**：引擎探针——`SetCallback(click, cb)` → 触发回调计数 1；`SetCallback(click, nullptr)` → 返回 1（解绑）且触发不再计数（无崩溃）；再注册恢复；Binding 侧：samples 编译 + 语义文档（binding 无自动化测试基建，Q-3）；
- 回归：test_menu/test_property_cabi（菜单段）+ 全量（仅 4 项预存）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 引擎 `setCallbackProperty(event, nullptr)` 语义改为"解绑（erase）" | 确认（原 return 0 语义无实际用途；覆写均汇流） |
| Q-2 | Binding 空回调解绑时是否清理 slot 注册表（现仅持有不清理） | 不清理（生命周期持有，无行为影响；清理需事件粒度映射，收益低） |
| Q-3 | P0-86 测试：引擎探针 + samples 编译 + 文档（无 binding 自动化基建） | 确认；如需 binding 自动化可另立项 |
| Q-4 | P0-87 注册范围：主菜单 + 子菜单项全量；文档位置（schema 描述 + menu.html） | 确认 |
