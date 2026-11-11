# CR20261105004_WindowAppearance_Design 评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-06
- 对象：`D:/GitSpace/UIControls/design/CR20261105004_WindowAppearance_Design.md`
- 结论：**有条件通过（需补前台激活/无阴影语义与回调 ABI，再实施）**

## 1. 认可

- `UIWindowFlags::Borderless` + `SetWindowAlwaysOnTop/GetWindowAlwaysOnTop` 的方向正确。
- 子视口/共享实例拒绝、幂等、默认 flags 不变、旧 DLL false 等边界与 CR05003 一致。
- SFML 无跨平台置顶时明确返回不支持，不做假成功；raylib headless 不碰全局窗口，方向正确。
- 手柄/窗口 API 不要求 `GetNativeWindowHandle` 跨后端统一，边界清晰。

## 2. 阻塞项

### B1. 缺少窗口前台激活 API

当前 Launcher 实际仍需 Win32 `SetForegroundWindow`/窗口枚举才能把 Designer 从后台带到前台；`AlwaysOnTop` 只改变 Z-order，不保证焦点/前台。设计说“Launcher 去掉 Win32 样式兜底”但只提供 Borderless/AlwaysOnTop，无法完全去掉 Win32。

建议二选一：

1. 在 CR05004 增加 `SetWindowForeground/RaiseWindow`（owner 独立窗口；子视口/headless 0；不保证 OS 强制焦点，但提供统一调用）；或
2. 明确 Launcher 只使用 AlwaysOnTop，前台激活留在 `LauncherPlatform`，不宣称该部分已跨平台。

建议选 1，否则“避免 OS API”的目标仍缺一块。

### B2. Borderless 不等于无阴影

Borderless 只能去掉标题栏/边框；Windows DWM 仍可能绘制外扩阴影。当前 Launcher 使用 `CS_DROPSHADOW`、`DWMNCRP_DISABLED`、`SetWindowRgn` 等 Win32 处理，CR05004 未定义这些语义。

设计需明确：

- `Borderless` 是否承诺“无标题栏、无关闭按钮、无系统阴影”；
- Windows 下引擎是否内部处理 CS_DROPSHADOW/DWM/region；若不做，Launcher 仍需平台层。
- 增加验收：窗口外围无阴影、非客户区不出现透明/黑边。

### B3. `UIBackendCallbacks` 追加函数的 ABI 兼容

设计计划在 `UIBackendCallbacks` 追加 `setWindowAlwaysOnTop/getWindowAlwaysOnTop`，但该结构当前没有 `structSize`/版本字段；旧后端 DLL 返回的静态结构较短，新核心读取新增指针可能越界/未定义。

需补：

- `UIBackendCallbacks` 的 `structSize`/版本或能力位，新增字段只在 size 足够时读取；
- 旧插件/旧 DLL 缺失时返回 false/诊断一次；
- 静态模式与插件模式行为一致；
- 回归旧 backend DLL + 新 core，以及新 backend DLL + 旧 core 的安全失败。

### B4. 运行期能力探针必须先做

设计目前只有静态头文件核对；raylib `FLAG_WINDOW_TOPMOST` 是否真正置顶、SFML `Style::None` 副作用、SDL3 Borderless 是否有首帧闪烁都未实测。应在编码前跑 SDL3/SFML/raylib 探针，并把结果写入设计；不通过则先修订能力矩阵。

## 3. 非阻塞建议

1. **Borderless 与 resize**：SFML `Style::None` 同时去掉 resize；Launcher 固定尺寸可以接受，但要在 API/文档明确 Borderless 后不能依赖 Resizable。
2. **AllwaysOnTop 失败原因**：返回 0 时记录 backend unsupported/headless/invalid，便于 Launcher 降级。
3. **`GetWindowAlwaysOnTop` 本地状态**：与 `GetWindowVisible` 一致；最小化/遮挡不影响。
4. **Launcher 当前 Win32 代码**：CR 交付前保留，但标记为临时；CR 交付后删除 `SetWindowLongPtr/SetWindowPos` 样式代码，仅保留 `LauncherPlatform` 的进程/路径能力。
5. **进程启动**：不属于 CR05004；继续由 `LauncherPlatform` 封装 CreateProcessW/posix_spawn，不放入 UI 引擎。
6. **测试补充**：Bring to front 与 AlwaysOnTop 组合、Borderless+Hidden 创建、窗口阴影像素断言；auto 回归 `test_window_visibility`。

## 4. 放行结论

方向通过，但 B1（前台激活/去掉 Win32 的完整路径）、B2（无阴影语义）、B3（BackendCallbacks ABI）、B4（三后端探针）必须在实施前闭合。建议把 B1/B2 并入 CR05004，避免再拆小 CR 造成窗口能力碎片化；进程启动仍归 LauncherPlatform。
