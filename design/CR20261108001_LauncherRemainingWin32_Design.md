# CR20261108001 — 启动器剩余 Win32 能力封装：窗口阴影 + 宿主进程/路径（分析 + 设计）

> 状态：**已关闭（2026-11-09；设计器联调通过）**——NoShadow SDL3 Windows best-effort；SFML/raylib 诊断忽略；进程/路径仍归 LauncherPlatform
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261108001）
> 关联：CR20261105003（窗口可见性）、CR20261105004（Borderless/AlwaysOnTop/Foreground）

## 1. 结论先行

本 CR 建议拆为两部分：

- **A. 窗口阴影/非客户区**：纳入引擎窗口 API，新增 `UIWindowFlags::NoShadow`（创建期）；**仅 Windows + SDL3 提供实际实现**，其他后端/平台明确不支持/降级。
- **B. 进程/路径/错误 UI**：**不纳入 UICornerstone UI 引擎**，继续由 LauncherPlatform 负责；引擎不新增 `CreateProcess`/Toolhelp32/ MessageBoxW 封装。

理由：进程生命周期与平台路径不是渲染库职责；引擎若接管会引入跨平台、权限、退出清理、日志/管道生命周期等新契约，超出窗口外观范围。

## 2. A. 窗口阴影/非客户区

### 2.1 现状与目标

- `Borderless` 只去除标题栏/关闭按钮，不承诺去除 Windows DWM 外扩阴影（CR05004 已明确）。
- Launcher 仍需 `CS_DROPSHADOW`/DWM/region 逻辑；本 CR 让 **SDL3 Windows** 后端在创建期统一处理，宿主不再直接碰 DWM。
- 其他后端/平台不伪装成功。

### 2.2 API

新增**应用层窗口标志**（不是 SDL 原始 flag）：

```c
#define UICORN_WINDOW_FLAG_NO_SHADOW 0x08000000u /* 应用层保留位；不传给 SDL 原生 flags */
```

- `Window.h`/Binding 暴露 `UIWindowFlags::NoShadow`；
- 只在 `createWindow` 消费；**不新增运行期 SetWindowShadow API**（避免 BackendCallbacks 再升版本；Launcher 创建期即可）；
- `Borderless | Hidden | NoShadow` 可组合；
- 不改变 flags=0/旧宿主行为。

### 2.3 SDL3 Windows 实现

- 创建时从 `UIWindowFlags::NoShadow` 移除该应用位后再传给 `SDL_CreateWindowAndRenderer`，避免未知 SDL flag。
- **B5：未显式 Hidden 时的首帧处理**：若带 `NoShadow` 但未带 `Hidden`，SDL3 backend 内部先以 `SDL_WINDOW_HIDDEN` 创建，完成 DWM/region 处理后再 `SDL_ShowWindow`；调用方仍表现为“创建后可见”，但首帧无阴影。
- 取 `SDL_PROP_WINDOW_WIN32_HWND_POINTER` 的 `HWND` 在创建后、首次显示前完成；若属性为空/获取失败（**B7**），诊断一次并继续创建/显示，不得空指针调用 DWM。
- **B9：内部隐藏的状态一致性**：`Borderless|NoShadow`（无 Hidden）create 返回后 `Window::isVisible()`（实际接口名为准）必须为 true；`SetWindowVisible`/`GetWindowVisible` 与 SDL 实际状态一致；不得因内部隐藏改变调用方“创建后可见”的语义。
- 在 SDL3 backend 内部执行：
  - `DwmSetWindowAttribute(DWMWA_NCRENDERING_POLICY, DWMNCRP_DISABLED)`；
  - 需要时处理 `CS_DROPSHADOW`/窗口 region；仅在 Windows 头/DWM 可用时编译；
  - 失败只诊断，不阻止窗口创建；语义为 **best-effort 无阴影**。
- 非 Windows SDL3：忽略 `NoShadow` 并诊断一次；不承诺无阴影。
- **B6：核心不得在进入 backend 前剥掉 NoShadow**。核心内部/`UIBackendCallbacks` 的 flags 原样保留 `0x08000000`，供 SDL3/SFML/raylib backend 消费；仅在各自调用 **SDL/SFML/raylib 原生创建 API 前** mask。CallbackWindow/插件回调原样转发 NoShadow（旧 backend 忽略未知位但不崩）。
- 验收：backend 收到 NoShadow；原生 flags 不含该位；旧 flags=0 不变。

### 2.4 SFML / raylib / headless

- SFML：没有跨平台无阴影 API；忽略 `NoShadow` 并诊断；Windows 平台层若必须无阴影，继续 LauncherPlatform。
- raylib：`nativeHandle()` 为 nullptr（当前实现），无法可靠取 HWND；不支持，忽略并诊断；不假成功。
- headless：无窗口，标志无意义。
- 能力矩阵：

| 后端/平台 | NoShadow |
|-----------|----------|
| SDL3 Windows | ✅ best-effort |
| SDL3 非 Windows | ❌ 诊断忽略 |
| SFML | ❌ 诊断忽略 |
| raylib | ❌ 诊断忽略 |
| headless | — |

### 2.5 验收

- SDL3 Windows（硬性）：`Borderless|Hidden|NoShadow` hidden→show 顺序正确；`Borderless|NoShadow`（非 Hidden）内部隐藏创建、**NoShadow 处理在首次显示前完成**后显示；首帧无标题栏/关闭按钮；无崩溃；控件/布局/主题/焦点不变。
- HWND 获取失败：诊断一次、窗口正常创建/显示、不崩溃（B7）。
- 无阴影（软性/记录项）：引擎已执行 DWM/region/CS_DROPSHADOW 处理；尽量无可见阴影；若 Windows 版本/站点策略/圆角/透明仍绘制阴影，记录 Windows 版本/策略与截图，**不作为失败**；能像素检查则记录结果。
- 非支持后端：创建正常、诊断一次、无崩溃；不得宣称无阴影。
- 旧 flags=0 行为不变；`NoShadow` 不污染 SDL flags。
- Launcher 移除 `EnumWindows/SetWindowLongPtr/SetWindowPos/DWM` 后，ready 前隐藏、ready 后显示、无标题栏、置顶/前台、失败/超时行为不变（Windows SDL3）。

## 3. B. 进程/路径/错误 UI（不纳入引擎）

明确边界：

- **不新增** `UICornerstone_StartProcess/WaitProcess/KillProcess` 等 UI 引擎 API；不封装 `CreateProcessW`/`PROCESS_INFORMATION`/Toolhelp32/POSIX spawn。
- **不新增** `GetExecutableDirectory`/`GetBasePath` C ABI；路径/UTF-8/错误提示由 LauncherPlatform 维护。
- 理由：进程生命周期、权限、退出清理、stdout/文件进度与 UI 渲染无关；引擎保持“窗口+控件树”职责；Launcher 已是独立进程，平台层已有进程/路径抽象。
- 若未来需要通用 Host SDK，另立独立 `CornerstoneHost`/`LauncherPlatform` 组件，不放入 UICornerstone UI 引擎。
- 同步到 Launcher 设计/用户手册：进程/路径/错误 UI 仍归 LauncherPlatform；本 CR 只解决窗口阴影，**不能把“完全去除 Win32 API”作为验收**。

## 4. 测试/回归

- SDL3 Windows：NoShadow 像素/外围检查；Borderless+Hidden+NoShadow 创建并 hidden→show；SetWindowVisible/AlwaysOnTop/Foreground 组合。
- SFML/raylib/headless：NoShadow 诊断忽略、窗口/控件正常。
- Launcher：去掉目标 Win32 调用后启动链路；失败/超时/取消/ready 不回归。
- 回归：`test_window_visibility`、`test_window`、`auto=2`、`auto=3`、`strict`、`test_cr_theme`。
- 进程/路径因不纳入引擎，不在引擎测试矩阵；由 LauncherPlatform 自测。

## 5. 风险与限制

- `NoShadow` 仅 Windows SDL3 有实际语义；跨平台不可承诺。若产品要求跨平台无阴影，需要各平台原生实现，另立 CR。
- DWM/站点策略/窗口圆角可能影响阴影行为；失败只能 best-effort。
- `NoShadow` 是应用层高位，必须从所有原生 flags 中 mask；否则可能触发 SDL/后端未知位。
- 不新增运行期阴影切换；若 Launcher 需要动态开关，另立 API/回调表版本升级 CR。
- 进程/路径不由引擎提供，Launcher 仍保留 LauncherPlatform；不能宣称“完全无 Win32 代码”。

## 修订记录

- 2026-11-08 初稿：基于 CR20261108001 与 CR5003/5004 现有窗口 API；建议窗口阴影纳入引擎（NoShadow，Windows SDL3 best-effort），进程/路径保持在 LauncherPlatform；待设计器评审。
- 2026-11-08 按评审修订：B1 best-effort/硬性验收拆分；B2 首次显示前处理；B3 核心+backend 双 mask；B4 进程/路径边界同步。
- 2026-11-08 二次复核修订：B5 无 Hidden 时内部隐藏创建；B6 核心保留 NoShadow、原生 API 前 mask；B7 HWND 失败安全降级。
- 2026-11-08 三次复核修订：B8 无阴影改软性记录项；B9 内部隐藏可见状态一致；§6 B3 表述统一。
- 2026-11-08 四次复核通过：修正 `UIBackendCallbacks`/`isVisible` 名称与 `NoShadow|Resizable`/HWND 失败测试；设计阶段关闭。

## 6. 评审响应（2026-11-08）

| 评审项 | 处理 |
|--------|------|
| B1 best-effort 验收冲突 | 拆分硬性验收（无标题栏/关闭按钮、首次显示前处理、Show/Hide）与软性目标（引擎已执行 NoShadow；平台仍绘制阴影记录为已知限制，不作跨平台失败）。 |
| B2 首次显示前处理 | 明确 NoShadow 在 SDL3 `createWindow` 内、首次显示前完成；补 `Borderless|Hidden|NoShadow` hidden→show 验收。 |
| B3 高位 mask | 核心保留 NoShadow 给 backend/CallbackWindow；各 backend 仅在调用原生 API 前防御性 mask；原生 flags 不含 0x08000000；补检查。 |
| B4 边界同步 | 设计/手册/Launcher 文档注明进程/路径/错误 UI 仍归 LauncherPlatform；本 CR 不宣称完全去除 Win32。 |
| 非阻塞 | API/Binding 注释高位；DWM 头/库仅 SDL3 Windows；raylib nativeHandle 判断保留；组合与像素测试补充。 |

## 7. 二次复核响应（2026-11-08）

| 评审项 | 处理 |
|--------|------|
| B5 NoShadow 无 Hidden 首帧 | 选择方案 1：SDL3 backend 内部先 `SDL_WINDOW_HIDDEN` 创建，处理 DWM/region 后再 `SDL_ShowWindow`；调用方仍表现为创建后可见。补 `Borderless|NoShadow` 首帧验收。 |
| B6 mask 位置 | 核心不剥掉 NoShadow；内部/backup flags 保留给 backend；仅在各 backend 调原生 API 前 mask；CallbackWindow/插件原样转发；测试 backend 收到、原生不含。 |
| B7 HWND 失败 | 属性为空/失败时诊断一次并继续创建/显示；不得空指针 DWM；纳入测试。 |
| 非阻塞 | NoShadow 高位注释、DWM 仅 SDL3 Windows 链接、能力矩阵/进程边界保持。 |

## 8. 三次复核响应（2026-11-08）

| 评审项 | 处理 |
|--------|------|
| B8 可见阴影验收冲突 | 硬性改为“NoShadow 处理已在首次显示前完成、无标题栏/关闭按钮、顺序正确、无崩溃”；无阴影改为软性/记录项，平台仍绘制时记录版本/策略/截图，不作失败。 |
| B9 内部隐藏状态 | 明确 create 返回后 `isVisible()==true`；`SetWindowVisible/IsWindowVisible` 与 SDL 状态一致；补测试。 |
| §6 B3 表述 | 统一为“核心保留 NoShadow，各 backend 原生 API 前 mask”，不再写核心统一剥掉。 |

## 9. 四次复核响应（2026-11-08）

| 评审项 | 处理 |
|--------|------|
| 文档类型名 | `SUIBackendCallbacks` 笔误改为 `UIBackendCallbacks`。 |
| 可见状态接口 | 以实际 `Window::isVisible()` / `SetWindowVisible`/`GetWindowVisible` 为准；create 返回后为 true。 |
| 测试补充 | 增加 `NoShadow|Resizable`、HWND 获取失败降级、两条首帧路径。 |
| DWM 效果 | 软性记录，不硬判像素；按 Windows 版本/策略留档。 |

## 10. 实施记录（2026-11-09）

- 新增 `UIWindowFlags::NoShadow`（0x08000000 应用层位）；SDL3 Windows 使用 DWM NCRENDERING/region best-effort；SFML/raylib 不传原生 flags 并诊断忽略。
- `Borderless|NoShadow` 无 Hidden 时 SDL3 内部隐藏创建、NoShadow 处理后再显示；`Borderless|Hidden|NoShadow` 保持 hidden→show。
- 核心保留 NoShadow 给 backend；各后端仅原生 API 前 mask；旧 flags=0 不变。
- `test_window_visibility` 增加 `Borderless|Hidden|NoShadow` 与 `Borderless|NoShadow` 创建/可见性检查；全量回归 55 OK + 4 预存失败。
- 阴影像素/平台策略仍为软性记录项，未作硬性通过条件；Release 已同步，DLL SHA256 `69f71b818af31c26eeafcd56c9d72f846205f3525f8fa9910be20eec8b2798b5`。
- 进程/路径/错误 UI 仍归 LauncherPlatform，本 CR 未新增进程 API。

## 11. 联调结论（2026-11-09）

- 设计器 Debug/Release 编译通过；auto=2、Debug auto=3、strict 通过。
- `test_window_visibility`、`test_window` 通过；DLL SHA256 `69f71b818af31c26eeafcd56c9d72f846205f3525f8fa9910be20eec8b2798b5` 两侧一致。
- Launcher 已使用 Borderless|Hidden|NoShadow、SetWindowAlwaysOnTop/Foreground；移除 DWM/EnumWindows/SetWindowLongPtr/SetWindowRgn/置顶/前台兜底。
- 进程/路径/错误提示保留 LauncherPlatform；符合 CR B 边界。
- 子模块仍未初始化（`-cd9038b`），但工作区 DLL 已同步；父仓 gitlink/release commit 尚未更新。
