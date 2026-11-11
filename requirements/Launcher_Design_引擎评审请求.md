# 启动 Launcher 设计 — 引擎评审请求

- 请求方：CornerstoneDesigner
- 日期：2026-11-05
- 设计文档：`D:/GitSpace/CornerstoneDesigner/design/09_启动Launcher设计.md`
- 结论请求：**请确认是否可实施，并指出与引擎边界冲突或需要引擎补充的 API**

## 1. 方案摘要

- `CornerstoneLauncher.exe` 为独立进程，使用**一个** UICorstone 实例、代码创建 loading 控件（不加载 JSON）。
- Launcher 创建 `CornerstoneDesigner.exe` 子进程；Designer 带 `--startup-hidden` 启动，主窗口在 ready 前隐藏。
- Launcher 与 Designer 通过命名管道传输阶段进度；Designer 在 `LoadLayout/ApplyTheme` 前后发 `layout_start/layout_done/theme/ready`。
- 主窗口 ready 后 Designer 显示、Launcher 退出。
- 选择独立进程的原因：当前引擎多实例是同一线程/主循环模型，SDL/字体/事件/Binding 全局状态未提供跨线程安全契约；两个独立进程不共享这些状态。

## 2. 请引擎重点确认

### Q1 独立进程 + UICorstone 实例

Launcher 和 Designer 各为一个独立进程，各自创建一个 UICorstone 实例并加载同一套 DLL/assets。是否存在引擎侧禁止或未覆盖的问题？

### Q2 主窗口隐藏/显示

Designer 需要在 `UICorstone::Create` 后隐藏主窗口，ready 时显示：

- `GetNativeWindowHandle()` 返回值是否可直接作为 SDL_Window* 传给 SDL3？
- 还是建议通过 `SDL_GetWindows`/窗口 ID 查找？
- 是否应由引擎增加 `UIWindowFlags::Hidden` 或 `SetWindowVisible(bool)`，由宿主不再直接碰 SDL？
- 若本期不改引擎，宿主使用 SDL3 动态解析隐藏窗口是否可接受？

### Q3 代码创建 loading 控件

Launcher 计划只使用 `CreatePanel/CreateLabel/CreateProgressBar` 和颜色/尺寸/文字属性，不调用 `LoadLayout`。这是否属于引擎支持范围？有无初始化顺序、字体/assets、主题依赖需要满足？

### Q4 进度管道

Designer 写行分隔 JSON，Launcher 非阻塞读取；不修改引擎日志。请确认：

- 无需引擎启动回调即可实现阶段进度；
- 引擎日志/平台日志与管道互不影响；
- 子进程异常/退出时是否有引擎侧需要配合的清理。

### Q5 首帧与资源

Launcher 自己创建一个 UICorstone 实例，预计增加约 200–300ms；首帧目标 <1s。请确认：

- Launcher 使用 SDL3 后端是否与 Designer 的 SDL3 进程互不干扰；
- 是否有更轻的“无窗口/延迟窗口”创建方式；
- 是否需要为 Launcher 单独裁剪 assets/DLL。

### Q6 跨平台

Launcher 使用 UICorstone 以保留跨平台潜力；进程创建/命名管道/窗口隐藏由 `LauncherPlatform` 抽象。请确认引擎不依赖 Windows-only 的 Launcher 行为，或指出限制。

## 3. 请引擎回复格式

建议在本文末尾追加：

```
### 引擎评审（YYYY-MM-DD）
- 结论：通过 / 有条件通过 / 不通过
- Q1：...
- Q2：...
- 需要引擎补充的 API/CR：...
- 其它风险：...
```

如引擎认为需要新增 API，请同时给出草案/CR 编号；设计器侧不修改引擎源码。
