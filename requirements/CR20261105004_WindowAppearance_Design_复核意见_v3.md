# CR20261105004_WindowAppearance_Design 三次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-06
- 对象：`D:/GitSpace/UIControls/design/CR20261105004_WindowAppearance_Design.md`（修订版 v2）
- 结论：**有条件通过（仅剩 B1 回调表遗漏，需补后实施）**

## 1. 已闭合

- **B2**：明确 Borderless 不承诺无 OS 阴影；Windows 阴影保留 LauncherPlatform；边界清楚。
- **B3**：新 core 接受回调表 version 1/2；v1 不读新增字段；新 backend DLL v2 + 旧 core 明确安全失败；版本概念区分正确。
- **B4**：运行期探针门禁保留。
- raylib/SFML 前台 API、Borderless+resize、子视口拒绝、失败诊断均已覆盖。

## 2. 仍需修正

### B1 剩余：`UIBackendCallbacks` 缺 `setWindowForeground`

设计已增加：

- `Window::setForeground()`
- C ABI `UICornerstone_SetWindowForeground`
- Binding `SetWindowForeground`

但 §3.5 的 `UIBackendCallbacks` 只列了：

- `setWindowAlwaysOnTop`
- `getWindowAlwaysOnTop`

**没有 `setWindowForeground`**。插件模式/`CallbackWindow` 无法实现前台激活，与 B1 目标不一致。

必须在 v2 回调表末尾追加：

```c
int (*setWindowForeground)(UIWindowHandle);
```

并在：

- `CallbackWindow::setForeground()` 转发；
- v1 回调表不读取/不安装该字段；
- v2 为 NULL 时返回 false/诊断一次；
- 静态模式与插件模式一致；
- 验收补“v2 回调表 foreground 可用 / NULL 降级”。

## 3. 非阻塞说明

1. **B2 目标边界**：本 CR 不解决 DWM 阴影；Launcher 的完全无阴影仍依赖 LauncherPlatform，不能宣称已经完全去掉 Windows API。
2. **B4**：三后端探针仍未记录结果，实施前必须补；raylib TOPMOST/SetWindowFocused 尤其需要实测。
3. **`SetWindowForeground` best-effort**：OS 焦点策略可能拒绝，Launcher 保留超时/降级。
4. **进程启动**：`CreateProcessW`/PID/文件进度仍归 LauncherPlatform，不属于本 CR。

## 4. 放行结论

**有条件通过**。补 `UIBackendCallbacks.version=2` 的 `setWindowForeground` 字段、CallbackWindow 转发、v1/v2 读取门禁与测试后，可进入实施。
