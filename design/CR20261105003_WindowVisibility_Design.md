# CR20261105003 — 窗口可见性 API：创建期隐藏与运行期显示/隐藏（分析 + 设计）

> 状态：**已关闭（2026-11-06；设计器联测通过）**——Create(Hidden)、SetWindowVisible 往返、子视口拒绝均通过
> 发布：`UICornerstone.dll` SHA256 `1e0ba452...`；设计与实现已同步子模块；C ABI `test_window_visibility` 通过
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261105003）
> 关联：Launcher 设计 v0.2、CR20261105001/CR20261105002

## 1. 背景

Launcher 需要 Designer 主窗口在 ready 前隐藏：

- 当前 `UICornerstone_GetNativeWindowHandle()` 在 SDL3 后端返回 `SDL_Window*`，但其他后端不是，不能作为跨后端契约；
- 当前没有创建期隐藏标志；
- 当前没有通用运行期 `SetWindowVisible`，非 SDL3 宿主无法在不重建实例的情况下显示/隐藏窗口。

目标：提供后端无关的最小窗口可见性 API，不改变现有 `Create`/`Show` 语义，旧代码不传新标志时行为不变。

## 2. 公开 API 设计

### 2.1 窗口标志

在 `include/UICornerstoneAPI.h` 增加：

```c
#define UICORN_WINDOW_FLAG_HIDDEN 0x00000008u /* 对齐 SDL_WINDOW_HIDDEN */
```

同步：

- `include/Window.h` 的 `UIWindowFlags::Hidden`；
- `binding/include/UICornerstone.h` 的 `UIWindowFlags::Hidden`；
- `UIInstanceConfig.windowFlags` 的注释/文档；
- 不新增版本字段；旧宿主 flags=0 行为不变。

### 2.2 C ABI

```c
/* 1=显示，0=隐藏；成功 1 / 失败 0；不重建实例/控件，不改变布局。 */
UICORNERSTONE_API int UICornerstone_SetWindowVisible(UIInstance instance, int visible);

/* 可选查询：out 非空写入当前可见性；headless/无窗口返回 0。 */
UICORNERSTONE_API int UICornerstone_GetWindowVisible(UIInstance instance, int* visible);
```

语义：

- **仅独立窗口实例允许操作**：`instance->owner == nullptr && instance->ownsBackend == true`；子视口/共享后端实例（`owner != nullptr` 或 `ownsBackend == false`）返回 0，**绝不转操作 owner 主窗口**；`Create(Hidden)` 不向子视口传播。
- `instance` 无效、未初始化、无窗口（headless）返回 0；
- `GetWindowVisible` 返回**引擎本地可见状态**：窗口被最小化、遮挡或未置顶时仍返回 true；与 `GetNativeWindowHandle`/OS focus 无关；无窗口/子视口/headless 返回 0。 `visible == nullptr` 返回 0，不写入/不崩溃。
- 不触发 `LoadLayout`/`ApplyTheme`/reflow/主题重算；
- 不主动清除焦点、选中、滚动或输入状态；OS 可能因此产生 FocusLost，由现有事件系统按原语义处理；
- **幂等**：已显示再显示、已隐藏再隐藏仍返回 1（成功）；仅在非法实例/无窗口/后端不支持时返回 0；
- 不保证显示时自动抢焦点或置顶；需要置顶由宿主/平台层处理；
- 调用线程：与现有帧循环同线程（UI 线程）；跨线程调用未定义。

### 2.3 Binding

```cpp
bool UICornerstone::SetWindowVisible(bool visible);
bool UICornerstone::IsWindowVisible() const;
```

- 动态 API 增加 `fnSetWindowVisible`、可选 `fnGetWindowVisible`；
- 旧 DLL 无入口时 `SetWindowVisible` 返回 false、`IsWindowVisible` 返回 false；缺失入口诊断**只打印一次**，避免启动日志刷屏；
- 头文件/手册标注 backend 能力限制。

## 3. 后端实现

### 3.1 Window 接口

`include/Window.h` 增加：

```cpp
virtual bool setVisible(bool visible) { (void)visible; return false; }
virtual bool isVisible() const { return !isHeadless(); }
virtual bool isHeadless() const { return false; }
```

- 默认失败/保守值，避免新增后端忘记实现时虚假成功；
- 后端跟踪自身可见状态，避免依赖平台查询差异。

### 3.2 SDL3

- 创建：`UIWindowFlags::Hidden` 透传为 `SDL_WINDOW_HIDDEN`（`SDL_CreateWindowAndRenderer` 的 flags；注意仍掩掉 `Vsync` 保留位）。
- 运行期：`SDL_HideWindow` / `SDL_ShowWindow`；成功后更新本地 `m_visible`。
- `isVisible` 返回本地状态；`nativeHandle` 不变。
- 创建时 Hidden 不应出现首帧可见窗口；`SDL_CreateWindowAndRenderer` 仍创建 renderer，`ProcessEvents/Render` 可正常运行，任务栏不出现窗口。
- `isVisible` 只反映引擎本地状态；最小化/被遮挡不改变返回值。

### 3.3 SFML

- 创建：SFML 无创建期 style 隐藏位；构造后立即 `setVisible(false)`，记录状态。**允许一帧平台级闪烁的降级行为**，文档标注；Launcher 若要求无闪烁，SFML 走平台层或等待后续能力。
- 运行期：`sf::WindowBase::setVisible(bool)`（按当前 SFML 版本可用 API）；失败返回 0。
- `isVisible` 返回本地状态，不查询 OS mapped 状态。
- 现有 `SFMLSetCursorWindow`/render device 不受影响。

### 3.4 raylib

- 创建：`UIWindowFlags::Hidden` 映射 `FLAG_WINDOW_HIDDEN`，必须在 `InitWindow` 前设置；仅首个真实窗口实例有效。
- 运行期：**先实测** raylib 运行时窗口隐藏能力。当前头文件未确认存在 `HideWindow/ShowWindow`；若只有 `SetWindowState(FLAG_WINDOW_HIDDEN)`/`ClearWindowState`，以实测是否真正隐藏/显示为准；无效时返回 0 并标注降级，不能假成功。
- 运行期隐藏状态更新与首实例绑定；headless/第二实例不得改全局。
- headless 实例（多实例非首实例）返回 0，不得读取/修改全局窗口状态；首实例的 `m_visible` 独立维护。
- 原有单窗口架构/`s_windowCount` 逻辑不变；隐藏首实例不影响 headless 实例的逻辑断言。

### 3.5 headless / 无窗口

- `Window::isHeadless()==true` 或 `instance->window==nullptr`：`SetWindowVisible` 返回 0，`GetWindowVisible` 返回 0；
- 不新增离屏可见性状态。

## 4. 与现有 API/行为的关系

- `UICornerstone_GetNativeWindowHandle` 保持原语义；新增 API 不改变它，也不承诺句柄类型跨后端统一。
- `Create` 不传 `Hidden`：所有后端保持当前可见行为。
- **复用已有窗口**（`CreateSDL3WindowFromExisting` 等）：Hidden 标志不承诺作用于外部已有窗口；Launcher 不使用路径，实现返回 0/不修改外部窗口。
- `SetWindowVisible` 不改变窗口大小、位置、缩放、渲染后端、实例上下文、控件树、主题、焦点字段；仅调用平台显示/隐藏。
- **B1 子视口保护**：只有 `owner==nullptr && ownsBackend==true` 才允许操作；子视口/共享实例返回 0，不修改 owner；`Create(Hidden)` 对子视口无意义。
- 隐藏期间仍可 `ProcessEvents/Render`（是否绘制由平台决定）；帧循环不强制暂停。
- Designer 的 `--startup-hidden` 优先使用新 API；旧 DLL 无入口时降级到 SDL3 native handle（仅 SDL3）。
- **B4 降级顺序**：新 API 缺失且 native handle 为空/非 SDL3 时，Designer 必须直接显示主窗口，不得保持隐藏；隐藏失败也不能留下不可见窗口。

## 5. 测试/验收

1. SDL3：`Create(Hidden)` 首帧不可见，无窗口闪烁；`SetWindowVisible(1/0)` 往返；控件树/焦点/布局/主题不变。
2. SFML：`Hidden` 构造后不可见；运行期显示/隐藏往返；记录可能的一帧闪烁。
3. raylib：首实例 `Hidden`/显示隐藏；headless 第二实例返回 0 且不影响首实例。
4. **子视口**：隐藏子视口返回 0，owner 主窗口可见性/焦点/布局不变；`Create(Hidden)` 不传播到子视口。headless/共享实例/非法实例返回 0。
5. `GetWindowVisible` 与 `SetWindowVisible` 一致；最小化/遮挡时仍为 true；旧 DLL Binding 返回 false 不崩。
6. `SetWindowVisible` 幂等：重复显示/隐藏返回 1；`GetWindowVisible(NULL)` 返回 0。
7. raylib：实测 `SetWindowState/ClearWindowState` 隐藏/显示能力；无效返回 0 并记录；headless 第二实例返回 0 不影响首实例。
8. Launcher 集成：Designer `--startup-hidden` 后 ready 前不可见；ready 后显示；无抢焦点；新 API 不可用时按 SDL3 native 降级，不得保持隐藏无窗口。
9. 回归：Debug/Release `auto=2`（272/0）、`auto=3`、`validate_layout --strict`、`test_cr_theme`；旧布局/主题/窗口行为零变化。
10. 文档/API 映射：C ABI/Binding/WindowFlags/Launcher 文档同步。

## 6. 风险与限制

- SFML 无创建期隐藏位，可能出现一帧闪烁；只能作为可接受降级，不能承诺与 SDL3 同等无闪烁。
- raylib 单窗口/headless 语义必须保持；不能因隐藏 API 影响首实例全局窗口。
- OS 显示/隐藏可能改变焦点；引擎不主动清除焦点字段，但 FocusLost/FocusGained 事件仍按现有语义分发，需 Launcher 验收。
- 显示窗口不保证置顶/抢焦点；Launcher 仍需在 ready 协议中保证“先显示再 ready”。
- 跨线程调用不支持；如需异步显示，宿主应投递到 UI 线程。
- 子视口共享 owner window：必须按 B1 拒绝操作，否则会把画布/预览当独立窗口隐藏。
- `GetWindowVisible` 是本地状态，不是 OS mapped/visible；最小化/遮挡仍为 true。

## 7. 实施顺序

1. Flags/Window 接口 + SDL3（最小可用）。
2. C ABI/Binding + docs。
3. SFML/raylib 实现与 headless 回归。
4. Launcher 切到新 API，保留 SDL3 native handle 降级。
5. 全量回归与发布同步。

## 修订记录

- 2026-11-05 初稿：基于 Launcher v0.2 评审请求与现有三后端 Window 实现；待设计器评审后实施。
- 2026-11-05 按评审修订：子视口/owner 保护、本地可见性语义、三后端细节、Launcher 降级/兜底验收。
- 2026-11-05 二次复核修订：raylib 运行期能力实测、GetWindowVisible 空指针、SetWindowVisible 幂等、headless/子视口组合、SFML 闪烁、旧 DLL 诊断一次、Launcher 降级顺序。
- 2026-11-05 三次复核通过：修正测试编号；补复用窗口 Hidden 不承诺；状态转为待实施。

## 8. 设计评审响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| B1 子视口保护 | §2.2/§4 明确仅 `owner==nullptr && ownsBackend==true` 允许；子视口/共享实例返回 0，不修改 owner；Create(Hidden) 不传播；补回归。 |
| B2 GetWindowVisible | 固定为引擎本地可见状态；最小化/遮挡仍 true；与 native handle/focus 无关；旧 DLL Binding false。 |
| B3 后端细节 | SDL3 renderer/首帧/任务栏；SFML 明确允许一帧闪烁降级；raylib InitWindow 前 flag、headless 不碰全局；本地 m_visible 一致。 |
| B4 Launcher | 新 API 不可用时 SDL3 native 降级；失败不得保持隐藏无窗口；ready/显示/退出兜底纳入验收。 |
| 命名 | C ABI `SetWindowVisible/GetWindowVisible`；Binding `SetWindowVisible/IsWindowVisible`。 |
| 默认值 | `Window::setVisible` 默认 false、各后端显式覆盖；`GetWindowVisible` 保守返回 0。 |

## 9. 二次复核响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| raylib 运行期隐藏 | 明确先实测 `SetWindowState/ClearWindowState`；无效返回 0 并标注降级，不假成功。 |
| GetWindowVisible 空指针 | `visible==nullptr` 返回 0，不写入/不崩溃。 |
| SetWindowVisible 幂等 | 同态调用返回 1；仅非法/无窗口/后端不支持返回 0。 |
| headless/子视口 | owner==nullptr 但 window==nullptr 返回 0；与 owner/ownsBackend guard 组合覆盖。 |
| SFML 首帧 | 允许一帧闪烁写入 Launcher 验收；不接受时只能平台层处理，不宣称无闪烁。 |
| 旧 DLL 诊断 | 缺失入口 false，诊断只打印一次。 |
| Launcher 降级顺序 | 新 API 缺失且 native handle 空/非 SDL3 时直接显示，不得保持隐藏。 |
- 2026-11-05 实施：UIWindowFlags::Hidden、SetWindowVisible/GetWindowVisible、Window/CallbackWindow 三后端接口、Binding、C ABI 测试与 release 同步。
