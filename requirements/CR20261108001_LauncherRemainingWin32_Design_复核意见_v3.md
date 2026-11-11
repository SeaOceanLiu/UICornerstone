# CR20261108001_LauncherRemainingWin32_Design 复核意见 v3

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-08
- 对象：修订版 v2 `D:/GitSpace/UIControls/design/CR20261108001_LauncherRemainingWin32_Design.md`
- 结论：**有条件通过（仅剩验收文字/状态一致性修正）**

## 1. 前次意见闭合

- B5 已闭合：NoShadow 且无 Hidden 时，SDL3 backend 内部先 `SDL_WINDOW_HIDDEN` 创建，处理 DWM/region 后再 `SDL_ShowWindow`；`Borderless|Hidden|NoShadow` 仍走 hidden→show。方案正确。
- B6 已闭合：核心保留 NoShadow 给 backend/CallbackWindow，仅在调用 SDL/SFML/raylib 原生创建 API 前 mask；表述已消除“核心提前剥掉”的歧义。
- B7 已闭合：HWND 属性为空/失败时诊断一次、继续创建/显示、不空指针调用 DWM，并纳入测试。

## 2. 剩余需修正

### B8. 硬性验收仍写“首帧无可见阴影”，与 best-effort 冲突

§2.5 第一条硬性验收：

> `Borderless|NoShadow`（非 Hidden）内部隐藏创建→处理后显示，首帧无可见阴影

但下一条又明确“若 Windows 版本/站点策略/圆角/透明仍绘制阴影，记录为已知平台限制，不作为失败”。在 DWM/系统策略不可控的前提下，“首帧无可见阴影”不能作为硬性通过条件。

建议改为：

- **硬性**：NoShadow 处理已在首次显示前完成；首帧无标题栏/关闭按钮；隐藏→显示顺序正确；无崩溃。
- **软性/记录项**：尽量无可见阴影；若平台仍绘制，记录 Windows 版本/策略与截图，不作为失败。

这样既保留 B5 的“显示前处理”要求，也不把不可控 DWM 行为当作硬失败。

### B9. 内部隐藏创建后的可见状态需明确

`Borderless|NoShadow`（无 Hidden）内部 `SDL_WINDOW_HIDDEN` 创建、处理后再 `SDL_ShowWindow`，请明确：

- 引擎 `Window` 的可见状态在 `create` 返回后为 true；
- `SetWindowVisible(false/true)`/`IsWindowVisible` 与 SDL 状态一致；
- 不改变调用方“创建后可见”的既有语义。

非阻塞，但建议在实现/测试中覆盖，避免内部隐藏导致状态机错位。

## 3. 非阻塞建议

- §6 表格 B3 仍写“核心统一 mask 一次”，与 B6 的“核心保留、原生 API 前 mask”表述不完全一致，建议同步修正，避免实施时误读。
- DWM 头/库仅 SDL3 Windows；非 Windows 不链接 dwmapi。
- `NoShadow` 高位 API/Binding 注释与能力矩阵保持。

## 4. 放行结论

**有条件通过**。补 B8（硬性验收去掉“首帧无可见阴影”，改为“显示前已处理 + 记录平台限制”）并确认 B9 的可见状态语义后可实施。B1~B7 已闭合；进程/路径仍归 LauncherPlatform，本 CR 不宣称完全去除 Win32。
