# CR20261109001 — Launcher 任务栏隐藏：窗口 Utility 标志（分析 + 设计）

> 状态：**已关闭（2026-11-09；设计器联调与人工验收通过）**——Utility flag、延迟显示、SDL3 Windows EXSTYLE 幂等 fallback、SFML/raylib mask；binding enum 已补
> 评审：`Temp/CR20261109001_LauncherUtility_Design_评审意见.md`（有条件通过，已补 B1~B4）
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261109001）
> 关联：CR20261105003（Hidden/Visible）、CR20261105004（Borderless/AlwaysOnTop/Foreground）、CR20261108001（NoShadow）

## 1. 目标与范围

Launcher 当前 `Borderless|Hidden|NoShadow` 不再使用 Win32 `WS_EX_TOOLWINDOW`，会重新出现在任务栏/Alt+Tab。

本 CR 增加 **创建期** `UIWindowFlags::Utility`：

- SDL3 使用原生 `SDL_WINDOW_UTILITY`（0x00020000），Utility 窗口不显示在任务栏/窗口列表；
- 仅创建期生效，不新增运行期 API，不改 `UIBackendCallbacks` 版本；
- 旧 flags=0 行为不变；
- 进程/路径/错误 UI 仍归 LauncherPlatform，不在本 CR 范围。

## 2. API

`include/UICornerstoneAPI.h`：

```c
#define UICORN_WINDOW_FLAG_UTILITY 0x00020000u /* 对齐 SDL_WINDOW_UTILITY */
```

同步：

- `Window.h` / Binding `UIWindowFlags::Utility`；
- API/Binding 注释标明：高位应用层/原生 SDL 标志，`uint32_t` 宽度，仅创建期；
- 创建期标志，与 `Borderless|Hidden|NoShadow` 可组合；
- 不新增 `SetUtility`/查询；不读取 Utility 以外的平台头。

## 3. 后端实现

### 3.1 SDL3

- `Utility` 原样加入 `SDL_CreateWindowAndRenderer` flags（与 Borderless/Hidden 相同）；不 mask。
- Windows 优先依赖 SDL3 的 `SDL_WINDOW_UTILITY`（通常映射 `WS_EX_TOOLWINDOW` 等价效果）。
- **Fallback（仅 Windows，best-effort）**：窗口创建后取 HWND，在内部 `_WIN32` 代码中检查并补 `WS_EX_TOOLWINDOW`；这不是新的公开 API，Launcher 仍不直接调用 Win32。
- 非 Windows SDL3 按 SDL3 Utility 语义；不额外承诺平台窗口列表行为。

### 3.2 SFML / raylib / headless

- SFML/raylib 无对应跨平台 Utility 能力：在调用各自原生创建 API 前 **mask `0x00020000`**，诊断一次，窗口正常创建；不得把未知位透传。
- headless：无窗口，标志无意义。
- 能力矩阵：

| 后端/平台 | Utility |
|-----------|---------|
| SDL3 Windows | ✅ 原生优先；backend 内部 EXSTYLE 幂等 fallback |
| SDL3 其他平台 | SDL3 语义 |
| SFML | ❌ 诊断忽略 |
| raylib | ❌ 诊断忽略 |
| headless | — |

### 3.3 创建流程与首帧（B1）

统一使用“创建期延迟显示”：

```text
needDeferredShow = (flags & (Utility | NoShadow)) && !(flags & Hidden)
createFlags = flags | (needDeferredShow ? SDL_WINDOW_HIDDEN : 0)

create(SDL)
if (Utility)  applyUtilityFallback()   // 先 EXSTYLE
if (NoShadow) applyNoShadow()          // 后 DWM/region
if (needDeferredShow) SDL_ShowWindow()
// 显式 Hidden：保持隐藏，等待 SetWindowVisible
```

- `Utility|Borderless`（无 Hidden、无 NoShadow）也先隐藏创建，完成 Utility fallback 后再显示，避免任务栏/Alt+Tab 首帧闪现。
- `Utility|Borderless|NoShadow`（无 Hidden）复用同一路径：先 Utility，再 NoShadow，最后 Show；首帧无标题栏/关闭按钮。
- `Utility|Borderless|Hidden|NoShadow`：创建期保持隐藏；所有外观处理先完成，后续 `SetWindowVisible` 只负责显示。
- 非 Utility/非 NoShadow 且无 Hidden：保持原有“创建即可见”，不增加延迟显示。

### 3.4 Windows Utility fallback 时序与失败路径（B2）

- 处理时机：**必须在首次 `SDL_ShowWindow` 前**；顺序固定为 **Utility（EXSTYLE）→ NoShadow（DWM/CS_DROPSHADOW/region）→ Show**。
- 幂等检查：读取 `GWL_EXSTYLE`；若已包含 `WS_EX_TOOLWINDOW`（SDL3 已正确处理），跳过 `SetWindowLongPtr`/`SetWindowPos`，避免额外 Alt+Tab/焦点副作用。
- 未包含时：
  - `SetWindowLongPtrW(hwnd, GWL_EXSTYLE, style | WS_EX_TOOLWINDOW)`；
  - 成功后 `SetWindowPos(hwnd, nullptr, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)`；
  - 刷新任务栏/窗口列表状态。
- `GetWindowLongPtr/SetWindowLongPtr/SetWindowPos` 失败，或 HWND 为空：诊断一次，**继续创建/显示，不崩溃、不回滚**；不重试。
- `GetWindowLongPtr` 前置 `SetLastError(0)`；返回 0 时结合 `GetLastError()` 判定失败。
- fallback 仅 `_WIN32` 编译；非 Windows 不引入 Win32 头/库；不改变公共头/Launcher。
- 采用“EXSTYLE 已存在则跳过”的幂等策略，而不是无条件重复应用；若 SDL3 原生已正确隐藏，不产生额外 `SetWindowPos` 副作用。

### 3.5 Utility 透传/mask 边界（B3）

- 核心只校验/转发 flags，不在进入 backend 前 mask Utility。
- SDL3 backend 收到的原生 flags 必须包含 `0x00020000`（`Utility` 位）；只对 NoShadow 做 SDL 前 mask。
- SFML/raylib 只在调用各自原生创建 API 前 mask `0x00020000`；BackendCallbacks/CallbackWindow 保留该位。
- `UIBackendCallbacks` v1/v2 结构不变；旧 backend 忽略未知位，不崩溃。
- 旧 flags=0 行为不变。

### 3.6 与现有标志组合

- `Utility|Borderless|Hidden|NoShadow`：创建期隐藏、无边框/无阴影；显示后不在任务栏/窗口列表。
- `Utility|Borderless` / `Utility|Borderless|NoShadow`（无 Hidden）：内部隐藏创建，Utility fallback（及 NoShadow）完成后再显示。
- `SetWindowVisible/AlwaysOnTop/Foreground` 对 Utility 窗口继续有效，不新增语义。
- 显式 `Hidden` 不与内部延迟显示冲突：显示由 `SetWindowVisible(true)` 触发。

## 4. 测试/验收

### 4.1 自动化

1. SDL3 Windows：`Utility|Borderless|Hidden|NoShadow` 创建；不崩溃；Show/Hide、AlwaysOnTop、Foreground、无标题栏/阴影正常；控件/布局/主题/焦点不变。
2. `Utility|Borderless`（非 Hidden）与 `Utility|Borderless|NoShadow`（非 Hidden）：内部隐藏创建→处理后显示；首帧无标题栏/关闭按钮。
3. SFML/raylib/headless：Utility 诊断忽略、窗口正常创建、不崩溃；原生 flags 不含 0x20000。
4. 旧 flags=0 不变；静态/插件模式；`UIBackendCallbacks` v1/v2 兼容不回归。
5. 回归：`test_window_visibility`、`test_window`、`auto=2`、`auto=3`、`strict`、`test_cr_theme`。
6. Launcher：`auto=2` ready 后退出，任务栏无残留；失败/超时/取消路径不回归；`Utility` 与 NoShadow 组合不出现任务栏首帧。

### 4.2 任务栏/Alt+Tab 实测记录（B4）

不能只靠截图判断；探针记录：

- Windows 版本/构建号、主题、窗口管理方式、缩放/多显示器；
- Utility 窗口是否出现在：任务栏、Alt+Tab、Win+Tab；
- Show/Hide 往返、AlwaysOnTop、Foreground、焦点变化；
- `GWL_EXSTYLE` 修改前/后值；`WS_EX_TOOLWINDOW` 是否原本已有；
- fallback 状态：`skipped`（已存在，未调用 SetWindowLongPtr）/`applied`（SetWindowLongPtr + SetWindowPos 成功）/`failed`（HWND/EXSTYLE/SetWindowPos 失败）及诊断；
- 测试 `Utility|Borderless`、`Utility|Borderless|NoShadow`、`Utility|Borderless|Hidden|NoShadow` 三组。
- 探针日志/布局保存到 `Temp/`，结果写入实施记录。

## 5. 风险与限制

- “Utility 不显示任务栏/Alt+Tab”最终由 OS/窗口管理器决定；Windows SDL3 原生通常有效，但需实测；EXSTYLE fallback 仍是 best-effort。
- SFML/raylib 不支持 Utility，不能承诺跨后端一致。
- 非 Windows SDL3 仅按 SDL3 语义，不承诺任务栏/窗口列表行为。
- Windows fallback 只在 SDL3 backend 内编译，不进入公共头/Launcher。
- `Utility` 是原生 SDL 位，不能与 NoShadow 的应用层 mask 混淆：NoShadow 在 SDL 前 mask，Utility 保留。
- EXSTYLE 切换可能影响 Alt+Tab/焦点；通过“已设置则跳过”降低副作用，实测确认。
- 进程/路径/错误 UI 仍归 LauncherPlatform。

## 6. 实施顺序

1. Flags + API/Binding/注释；SDL3 Utility 透传、延迟显示、Windows 幂等 fallback。
2. SFML/raylib mask + 诊断；能力矩阵/文档。
3. 三后端探针（任务栏/Alt+Tab）+ Launcher 集成回归。
4. 全量回归、发布同步。

## 修订记录

- 2026-11-09 初稿：基于 Launcher 任务栏隐藏需求与 SDL3 `SDL_WINDOW_UTILITY`；SDL3 原生优先、Windows backend best-effort TOOLWINDOW fallback，SFML/raylib 诊断忽略；待评审。
- 2026-11-09 v1（按评审意见 B1~B4 修订）：
  - Utility 且未显式 Hidden（无论有无 NoShadow）统一内部隐藏创建，应用 Utility/NoShadow 后再 Show；
  - 明确 Utility→NoShadow→Show 顺序、EXSTYLE 幂等检查、`SetWindowPos(SWP_FRAMECHANGED...)`、失败诊断后继续；
  - 明确核心/SDL3/SFML/raylib 的 Utility 保留与 mask 边界；
  - 补充任务栏/Alt+Tab 实测记录项与 fallback 状态记录；
  - 补充高位标志注释、32 位宽度、非 Windows SDL3 语义限制。

## 7. 实施记录（2026-11-09）

- `UICORN_WINDOW_FLAG_UTILITY`（0x00020000）与 `UIWindowFlags::Utility` 已加入；API/Binding 注释标明创建期与 32 位宽度。
- SDL3：Utility 原样透传；`Utility|Borderless`（无 Hidden/NoShadow）内部隐藏创建，Utility→NoShadow→Show；SDL3 实测已自带 `WS_EX_TOOLWINDOW`，fallback 状态为 `skipped`，未重复 SetWindowLongPtr/SetWindowPos。
- SFML/raylib：原生创建前 mask Utility/NoShadow 并诊断；headless 无窗口。
- `test_window_visibility` 新增 `Utility|Borderless`、`Utility|Borderless|NoShadow`、`Utility|Borderless|Hidden|NoShadow`，共 7 项相关检查全部通过；支持 `hold=N` 保留 Utility 窗口用于任务栏/Alt+Tab 人工观察。
- 全量回归：56 OK + 4 项预存失败；`test_window` 通过。
- 交互式任务栏/Alt+Tab 截图受当前 shell 非交互桌面限制未能自动采集；设计器 Launcher 侧仍需按 B4 记录实际任务栏/Alt+Tab/Win+Tab 与 EXSTYLE。
- `docs/appendix/capi.html`、`binding.html`、`design/API_Mapping_Table.md` 已补 Utility 能力说明。
- Release 已同步，DLL SHA256 `e805073f8e2e5979256100d92c640f84dcc7b0e40c7701effc20e22d67c2bcf3`；SDL3 实测 Utility 创建后 EXSTYLE 已含 `WS_EX_TOOLWINDOW`，fallback 为 `skipped`。
- 交付缺口修复（2026-11-09）：`binding/include/UICornerstone.h` 已补 `UIWindowFlags::Utility`（引用同一 C 宏），Binding Debug/示例编译通过；release/binding 与设计器子模块头已同步。DLL 内容未变，SHA256 保持 `e805...`。

## 8. 联调结论（2026-11-09）

- binding 头 `UIWindowFlags::Utility` 已补，Launcher 改用命名枚举；DLL SHA256 `e805073f8e2e5979256100d92c640f84dcc7b0e40c7701effc20e22d67c2bcf3` 两侧一致。
- Designer Debug/Release 编译通过；`test_window_visibility` RESULT OK；auto=2、auto=3、strict 通过；Launcher auto=2 ready。
- EXSTYLE 实测 `0x188`：`WS_EX_TOOLWINDOW=True`、`WS_EX_APPWINDOW=False`；测试窗口与 Launcher 一致。
- 人工验收：任务栏无图标、Alt+Tab 不出现；显示、置顶、前台、无标题栏/阴影正常。
- CR 设计器/引擎均为 closed。
