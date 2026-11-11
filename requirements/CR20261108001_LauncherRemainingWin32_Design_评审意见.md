# CR20261108001_LauncherRemainingWin32_Design 评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-08
- 对象：`D:/GitSpace/UIControls/design/CR20261108001_LauncherRemainingWin32_Design.md`
- 结论：**有条件通过（A 窗口阴影可实施；B 进程/路径不纳入引擎的边界需接受）**

## 1. 认可

- **A（NoShadow）**方向正确：应用层 `0x08000000` 不直接透传 SDL，避免未知原生 flags；只在创建期生效，避免 BackendCallbacks 再升版本。
- SDL3 Windows 使用官方 `SDL_PROP_WINDOW_WIN32_HWND_POINTER` 取 HWND，DWM/CS_DROPSHADOW/region 在 backend 内部 best-effort，边界合理。
- SFML/raylib/headless 明确不支持并诊断，不假成功；能力矩阵清晰。
- **B**：进程生命周期/路径/错误 UI 不放进 UI 引擎，保留 LauncherPlatform，符合引擎职责边界；不再承诺“完全无 Win32”也应在设计/手册中明示。

## 2. 实施前需补

### B1. NoShadow 的验收强度与 best-effort 语义冲突

§2.3 写“best-effort 无阴影”，§2.5 却要求“外围无可见 DWM 阴影”作为通过条件。DWM、站点策略、圆角、透明窗口等可能导致阴影无法完全移除。

建议：

- 将验收改为“引擎已执行 NoShadow 处理；若平台策略仍绘制阴影，记录为已知限制”，或明确哪些 Windows 版本必须硬通过；
- 不要把 best-effort 结果当作跨后端/跨平台保证。

### B2. NoShadow 必须在首次显示前完成

`Borderless|Hidden|NoShadow` 是 Launcher 的关键组合；DWM 样式/region 应在 SDL 窗口创建后、首次 `ShowWindow/SetWindowVisible(true)` 前应用，避免首帧闪出阴影。

设计需补：

- backend 在 `createWindow` 内完成 NoShadow 处理，或保证调用方显示前已处理；
- 验收覆盖 `Borderless|Hidden|NoShadow` 的隐藏→显示顺序，不能只测已显示窗口。

### B3. 所有 backend/callback 路径必须 mask 0x08000000

NoShadow 是应用层位，不能传给 SDL/SFML/raylib 原生 flags：

- SDL3 传给 `SDL_CreateWindowAndRenderer` 前移除；
- SFML/raylib 也必须 mask 后再判断 Resizable/Borderless 等；
- CallbackWindow/BackendCallbacks 路径不能把未知高位透传给插件；
- 建议在核心统一 mask 一次，并在各 backend 再做防御；
- 验收：flags=NoShadow 时 SDL 原生 flags 不含该位；旧 flags=0 行为不变。

### B4. B 的边界需在 API/手册明确

设计第 3 节已说明不封装进程/路径/MessageBoxW；请同步到：

- Launcher 设计 v0.7/用户手册：进程/路径仍由 LauncherPlatform 处理；
- CR 状态说明：本 CR 只解决窗口阴影；不能把“完全去除 Win32 代码”作为验收。

## 3. 非阻塞建议

1. `NoShadow` 高位保留位与 `Vsync` 等已有位写入 API 注释，并在 Binding 中同步。
2. Windows DWM 头/库只在 SDL3 Windows backend 编译，不进入公共头；非 Windows 不引入。
3. raylib 当前 `nativeHandle()` 为 nullptr 的判断要保留；未来有 HWND 时再扩展。
4. 测试补：NoShadow 与 Hidden/Borderless/AlwaysOnTop/Foreground 组合、Show/Hide 往返、像素外围检查（能测则测，不能测记录）。
5. 进程/LauncherPlatform 的 Win32 代码不计入本 CR 的失败项；由设计器侧自测。

## 4. 放行结论

**有条件通过**。A 部分补 B1（best-effort 验收口径）、B2（首次显示前处理）、B3（高位 mask）后可实施；B 部分明确不纳入引擎，进程/路径继续归 LauncherPlatform。本 CR 不能宣称完全去除 Win32 API，只能移除窗口阴影相关的 DWM/EnumWindows/SetWindowLongPtr/region 兜底。
