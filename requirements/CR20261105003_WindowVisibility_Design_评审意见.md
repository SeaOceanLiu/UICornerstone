# CR20261105003_WindowVisibility_Design 评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105003_WindowVisibility_Design.md`
- 结论：**有条件通过（补 owner/子视口保护后实施）**

## 1. 认可

- API 方向正确：`UIWindowFlags::Hidden` + `SetWindowVisible/GetWindowVisible`，不改 `GetNativeWindowHandle` 语义。
- Hidden 值 `0x8` 与 SDL3 `SDL_WINDOW_HIDDEN` 一致；SDL3 创建期隐藏、SFML 构造后隐藏、raylib 全局窗口/headless 约束都做了区分。
- 线程限定 UI 线程、失败返回 0、旧 DLL 缺入口降级、headless 返回 0，方向正确。
- Launcher 的 ready/显示顺序和 SDL3 native 临时降级已与设计器文档一致。

## 2. 必须补充

### B1. 子视口必须禁止窗口可见性操作

当前 `UIContext` 中 `CreateViewport` 创建的子视口：

- `owner != nullptr`；
- `ownsBackend == false`；
- `window` 从 owner 继承（不是独立窗口）。

如果 `SetWindowVisible/GetWindowVisible` 只判断 `window != nullptr`，对子视口调用会**隐藏/显示整个 owner 主窗口**，影响设计器画布/预览等多视口场景。

必须明确：

- 仅 `owner == nullptr && ownsBackend == true`（独立窗口实例）允许 `SetWindowVisible/GetWindowVisible`；
- 子视口、headless、共享后端实例返回 0，并诊断/不修改 owner；
- `Create(Hidden)` 对子视口无意义，不传播；
- 补子视口/主窗口回归：隐藏子视口不得隐藏主窗口。

### B2. `GetWindowVisible` 的语义

- 返回的是引擎本地可见状态，还是 OS 实际 mapped/visible？设计写“本地状态”，请明确：窗口被最小化/遮挡时仍返回 true；
- 与 `GetNativeWindowHandle`/焦点无关；
- 旧 DLL 无入口时 Binding `IsWindowVisible()` 的返回值（建议 false + 诊断）要固定。

### B3. `Create(Hidden)` 与后端细节

- SDL3 的 `SDL_CreateWindowAndRenderer` 传 Hidden 后，窗口不会出现在任务栏/桌面；确认 renderer 创建、首帧 ProcessEvents/Render 正常。
- SFML 构造后 `setVisible(false)` 仍可能闪一帧；验收需明确允许/不接受；若 Launcher 要求无闪烁，SFML 需标注降级。
- raylib `FLAG_WINDOW_HIDDEN` 必须在 `InitWindow` 前设置；headless 第二实例不得改变首实例全局状态。
- 所有后端的 `m_visible` 初始化与 `isVisible` 本地状态要一致。

### B4. 与 Launcher 的集成验收

- Designer `--startup-hidden` 使用新 API 后，ready 前不可见、ready 后显示，无闪烁/抢焦点；
- 如果新 API 不可用（旧 DLL/非 SDL3），Designer 必须按设计降级（直接显示或 SDL3 native），不能保持隐藏导致“启动后无窗口”；
- Launcher 关闭/子进程失败时，如果主窗口已隐藏，必须有可见性兜底。

## 3. 非阻塞建议

1. 规范命名：C ABI 用 `SetWindowVisible`/`GetWindowVisible`，Binding 用 `SetWindowVisible/IsWindowVisible`，文档同时标注 backend 能力。
2. `Window::setVisible` 默认返回 false，避免新增后端“假成功”；`isVisible` 默认值不要误导；每个后端显式覆盖。
3. 增加 `Create(Hidden)` 的 Windows 任务栏/焦点测试；OS 最小化后 `GetWindowVisible` 的语义写入文档。
4. 回归 `auto=2/auto=3` 不应经过 Launcher；Launcher 集成另测。
5. CR 状态仍为设计器 open/引擎 open，完成实现与回归后再关闭。

## 4. 验收矩阵补充

- owner 主窗口隐藏/显示正常；子视口调用返回 0，主窗口不变。
- headless 第二实例、共享 owner 实例返回 0；非法实例不崩。
- SDL3 Hidden 首帧无窗口闪烁；Show/Hide 往返后控件、布局、主题、焦点字段不变。
- SFML/raylib 按能力返回；raylib 首实例与 headless 第二实例互不影响。
- 旧 DLL Binding 缺入口 false/不崩；新 DLL Binding 正常。
- Launcher ready 前隐藏、ready 后显示；新 API 不可用时降级不产生隐藏残留。
