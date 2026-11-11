# CR20261108001_LauncherRemainingWin32_Design 复核意见 v4

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-08
- 对象：修订版 v3 `D:/GitSpace/UIControls/design/CR20261108001_LauncherRemainingWin32_Design.md`
- 结论：**通过（设计阶段）**

## 1. 前次意见闭合

- **B1**：硬性验收与 best-effort 无阴影已拆分；无可见阴影改为软性/记录项，平台限制记录版本/策略/截图，不再作为失败。
- **B2**：NoShadow 在 SDL3 `createWindow` 内、首次显示前完成。
- **B3/B6**：核心保留 NoShadow 给 backend/CallbackWindow；各 backend 仅在调用 SDL/SFML/raylib 原生 API 前 mask；原生 flags 不含 0x08000000。
- **B4**：进程/路径/错误 UI 明确不纳入引擎，继续 LauncherPlatform；不宣称完全去除 Win32。
- **B5**：NoShadow 无 Hidden 时内部隐藏创建，处理后再显示；`Borderless|Hidden|NoShadow` 保留 hidden→show。
- **B7**：HWND 获取失败诊断一次、继续创建/显示、不空指针调用 DWM。
- **B8**：硬性验收改为“NoShadow 处理在首次显示前完成、无标题栏/关闭按钮、顺序正确、无崩溃”；阴影软性记录。
- **B9**：明确 create 返回后 `isVisible()==true`，`SetWindowVisible/IsWindowVisible` 与 SDL 状态一致。
- §6 B3 表述已统一为“核心保留、原生 API 前 mask”。

## 2. 非阻塞备注

- 文档中 `SUIBackendCallbacks` 疑为笔误，实施时以实际 `UIBackendCallbacks`/flags 参数为准；不影响设计结论。
- `Window::isVisible()` 以现有实际 API 名称为准（`isWindowVisible`/等价接口），只要满足 B9 语义。
- 建议实现时补 `NoShadow|Resizable` 组合和 `NoShadow` + 失败 HWND 的回归，避免只覆盖 Launcher 的 `Hidden` 路径。
- `DwmNCRenderingPolicy` 在不同 Windows 版本/主题下的效果可能不同；测试按软性记录，不硬判像素。

## 3. 放行结论

**设计阶段通过**。A 部分（NoShadow/SDL3 Windows/统一 mask/隐藏首帧/失败降级）与 B 部分（进程/路径/错误 UI 不纳入引擎）边界清晰，可按设计实施。

实施后验收仍需覆盖：后端收到 NoShadow、原生 flags 不含高位、`Borderless|Hidden|NoShadow` 与 `Borderless|NoShadow` 两条首帧路径、HWND 失败降级、旧 flags=0 不变，以及现有 `test_window_visibility`/`test_window`/auto=2/auto=3/strict/test_cr_theme 回归。
