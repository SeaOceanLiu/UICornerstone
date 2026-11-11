# CR20261105004_WindowAppearance_Design 四次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-06
- 对象：`D:/GitSpace/UIControls/design/CR20261105004_WindowAppearance_Design.md`（修订版 v3）
- 结论：**通过（附 1 处文档一致性修正；B4 探针作为实施门禁）**

## 1. 已闭合

- **B1**：`UIBackendCallbacks.version=2` 末尾已补 `setWindowForeground(UIWindowHandle)`，CallbackWindow 转发、v1 不读取、v2 NULL 降级，完整。
- **B2**：Borderless 不承诺无 OS 阴影；Windows DWM 阴影明确留给 LauncherPlatform/后续平台 CR，边界清楚。
- **B3**：新 core 接受 v1/v2；v1 不读新字段；新 DLL v2 + 旧 core 安全失败；版本概念和回归用例已明确。
- **B4**：三后端运行期探针保留为实施门禁。
- `Borderless`+resize、前台 best-effort、子视口拒绝、失败诊断均覆盖。

## 2. 需修正的文字

§8 设计评审响应表中的 B3 行仍写：

> “旧 DLL/新核心安全 false/诊断，新 DLL/旧核心忽略”

但 §3.7 已改为“新 DLL v2 + 旧 core **安全失败**，不承诺忽略”。请将该行统一为“安全失败”，否则设计与响应表自相矛盾。

## 3. 实施注意

1. **B4 探针先做**：SDL3 Borderless/AlwaysOnTop、SFML Style::None、raylib UNDECORATED/TOPMOST/SetWindowFocused；未通过先修订能力矩阵，不能先编码。
2. **`BackendManager` 版本检查**：从 `version != 1` 改为接受 1/2；新增字段只在 `version >= 2` 且非空时读取。
3. **CallbackWindow v1/v2**：v1 不安装新增回调；v2 为 NULL 时 false/诊断一次；静态/插件一致。
4. **Linux/Windows 阴影**：本 CR 不解决 DWM 阴影；Launcher 当前 Win32 `CS_DROPSHADOW/DWM/SetWindowRgn` 仍保留在 LauncherPlatform。
5. **进程启动**：`CreateProcessW`/PID/进度文件仍归 LauncherPlatform，不纳入本 CR。
6. **验收**：除现有 auto/theme 回归外，补 `test_window_visibility`、ARI（v1/v2 backend）和 `SetWindowForeground` 的 best-effort 用例；窗口外围像素检查只要求无标题栏/关闭按钮，阴影按设计记录为已知平台行为。

## 4. 放行结论

**通过**。补正 §8 B3 行并执行 B4 探针后，可进入实施；其余设计已与二次/三次评审意见一致。Windows 阴影和进程启动仍明确不属于本 CR。
