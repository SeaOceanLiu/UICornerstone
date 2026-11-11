# CR20261108001_LauncherRemainingWin32_Design 复核意见 v2

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-08
- 对象：修订版 `D:/GitSpace/UIControls/design/CR20261108001_LauncherRemainingWin32_Design.md`
- 结论：**有条件通过（需补两项实现边界，不改总体方向）**

## 1. 前次意见闭合

- B1 已拆分为硬性验收（无标题栏/关闭按钮、首次显示前处理、Show/Hide）与软性目标（best-effort 无阴影，平台限制记录）；可接受。
- B2 已明确 NoShadow 在 SDL3 `createWindow` 内、首次显示前处理；方向正确，但见新增 B5。
- B3 已要求核心 + backend 双 mask，并覆盖 CallbackWindow；方向正确，但 mask 位置需澄清，见 B6。
- B4 已同步进程/路径/错误 UI 边界，并明确本 CR 不宣称完全去除 Win32；可接受。

## 2. 新增需补项

### B5. NoShadow 但未带 Hidden 时，窗口可能已经显示

设计写“窗口创建后、首次显示/SetWindowVisible(true) 前处理”，但 SDL3 默认创建即显示；若调用方仅传 `Borderless|NoShadow`（不带 `Hidden`），处理 DWM 时窗口已经可见，无法保证“首帧无阴影”。

需明确二选一：

1. **推荐**：NoShadow 且未显式 Hidden 时，backend 内部先以隐藏方式创建，处理 DWM/region/CS 后再显示；对调用方仍表现为“创建后可见”，但首帧无阴影。
2. 若不实现内部隐藏，则明确 **NoShadow 仅支持与 Hidden 组合**，并在 API/能力矩阵/测试中写死；`Borderless|NoShadow` 不承诺首帧。

验收需覆盖：`Borderless|NoShadow`（非 Hidden）创建后首帧、以及 `Borderless|Hidden|NoShadow` hidden→show 两条路径。

### B6. 核心 mask 不能把 NoShadow 从 backend-facing flags 中剥掉

“核心 `Create` 统一 mask 一次，各 backend 再防御性 mask”表述有歧义：若核心在进入 backend **之前**就把 `0x08000000` 去掉，SDL3 backend 将收不到 NoShadow，无法执行 DWM；CallbackWindow 也无法转发给新插件。

需明确：

- **核心保留 NoShadow 在内部 `UIWindowFlags`/backend-facing flags 中**；
- 仅在调用 SDL/SFML/raylib 原生创建 API 前 mask；
- CallbackWindow/BackendCallbacks 的 v2/现有 flags 原样保留 NoShadow，由新 backend 消费；旧 backend 忽略未知位但不崩溃；
- 测试验证：backend 收到 NoShadow；原生 flags 不含该位。

### B7. HWND 获取失败路径

`SDL_PROP_WINDOW_WIN32_HWND_POINTER` 失败/为空时应诊断一次并继续创建窗口（best-effort），不得崩溃或空指针调用 DWM；此路径应纳入测试矩阵。

## 3. 非阻塞建议

- `NoShadow` 高位仍需 API/Binding 注释；`DwmSetWindowAttribute` 仅 SDL3 Windows 编译，非 Windows 不链接 dwmapi。
- `NoShadow` 是应用层位，建议在能力矩阵/API 文档标注“仅 SDL3 Windows 实际语义；SFML/raylib/headless 忽略”。
- 进程/路径仍由 LauncherPlatform 自测；本 CR 验收不包含 CreateProcessW/Toolhelp/MessageBoxW。

## 4. 放行结论

**有条件通过**。补 B5（无 Hidden 时内部隐藏创建，或明确仅支持 Hidden）、B6（核心保留 flag、只在原生 API 前 mask）、B7（HWND 失败安全降级）后可实施。前次 B1~B4 已闭合；进程/路径边界维持不纳入引擎，不能以本 CR 宣称完全去除 Win32。
