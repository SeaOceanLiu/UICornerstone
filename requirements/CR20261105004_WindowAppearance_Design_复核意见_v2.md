# CR20261105004_WindowAppearance_Design 二次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-06
- 对象：`D:/GitSpace/UIControls/design/CR20261105004_WindowAppearance_Design.md`（修订版）
- 前次意见：`Temp/CR20261105004_WindowAppearance_Design_评审意见.md`
- 结论：**有条件通过（B1/B2/B4 已闭合；B3 ABI 兼容声明仍需修正）**

## 1. 已闭合

- **B1**：增加 `SetWindowForeground`，SDL3 `RaiseWindow`、SFML `requestFocus`、raylib `SetWindowFocused`；尽力语义，不保证 OS 强制焦点，合理。
- **B2**：明确 Borderless 只承诺无标题栏/系统装饰，不承诺无 OS 阴影；Windows 阴影留给 LauncherPlatform/后续平台能力。设计边界清楚，不再把阴影误算进跨平台 API。
- **B4**：保留三后端运行期探针门禁，未通过先修订设计，正确。
- Borderless + resize、失败诊断、子视口拒绝、Get 本地状态等补充合理。

## 2. 仍需修正

### B3. `UIBackendCallbacks` 版本兼容声明与旧核心行为矛盾

当前 `BackendManager.cpp` 对回调查表的检查是：

```cpp
if (!callbacks || callbacks->version != 1) {
    printf("BackendManager: invalid callback table
");
    return false;
}
```

因此：

- **新核心 + 旧后端 DLL（version=1）**：可以按 version 检查后不读新增字段，成立。
- **新后端 DLL（version=2） + 旧核心**：旧核心会因 `version != 1` 直接失败，不可能“忽略新增字段、保持旧行为”。

设计必须二选一：

1. **不支持新 DLL + 旧核心**，明确版本配套（推荐，简单且安全）；文档写“旧核心拒绝 v2，返回失败/诊断”。
2. 设计新的兼容机制（例如新核心接受 1/2，旧核心无法修改，无法做到双向兼容；或另立能力查询/注册接口）。

当前设计第 3.7 节“新后端 DLL + 旧核心忽略新增字段”不成立，需要删除或改成“不支持并安全失败”。

另外需明确：

- `UIBackendCallbacks.version` 已经存在（非本 CR 新增）；新增字段只能追加末尾，version>=2 时读取；
- 旧 core 读取 v2 时不能越界（旧 core 只读 version 后即失败，安全）；
- 新 core 读取 v1 时不得访问新增字段；
- 回调表版本与 `BackendAPI.version` 的关系不要混淆。

## 3. 非阻塞说明

1. **B2 与“完全不使用 Windows API”目标**：本 CR 不承诺去除 DWM 阴影；Launcher 仍需保留 Win32 阴影处理。若目标是完全由引擎封装，需要另立平台能力 CR，不能宣称本 CR 已完成。
2. **前台激活仍是 best-effort**：Windows 前台锁、全屏应用、UAC 焦点策略可能拒绝；Launcher 应保留超时/降级，不应依赖 `SetWindowForeground` 必然成功。
3. **B4 探针**：实现前必须记录 SDL3/SFML/raylib 实测结果；尤其 raylib `SetWindowState(FLAG_WINDOW_TOPMOST)` 与 `SetWindowFocused` 的实际行为。
4. **SFML Borderless**：`Style::None` 去掉 resize，Launcher 固定尺寸可接受；文档已注明。
5. **Launcher 进程启动**：`CreateProcessW`/PID/文件进度仍归 `LauncherPlatform`，不属于本 CR；CR 交付后只移除窗口样式/置顶/前台 Win32 代码，保留进程层。

## 4. 放行结论

**有条件通过**。B1/B2/B4 已闭合；修正 B3 的版本配套声明、补 `version` 读取和旧核心行为测试后，可进入实施。B2 的 Windows 阴影限制和进程启动边界保持明确。
