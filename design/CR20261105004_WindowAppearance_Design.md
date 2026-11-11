# CR20261105004 — 跨平台窗口外观 API：Borderless + AlwaysOnTop（分析 + 设计）

> 状态：**已实施（2026-11-06；待设计器联测）**——SDL3 Borderless/AlwaysOnTop/Foreground 已验证；SFML/raylib 已编译，B4 运行期探针待补
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261105004）
> 关联：CR20261105003（窗口可见性）、Launcher 设计 v0.2

## 0. 能力验证状态（截至 2026-11-06）

本设计先做了**静态能力核对**（头文件/后端源码），未把运行期探针结果当作已验证事实：

| 后端 | 静态核对结果 | 运行期验证 |
|------|--------------|------------|
| SDL3 | `SDL_WINDOW_BORDERLESS`、`SDL_SetWindowAlwaysOnTop` 存在 | 未做（实施前探针） |
| SFML | `sf::Style::None` 可创建无边框；头文件无跨平台 AlwaysOnTop API | 未做；置顶按不支持设计 |
| raylib | `FLAG_WINDOW_UNDECORATED`、`FLAG_WINDOW_TOPMOST`、`SetWindowState` 存在 | 未做；运行期隐藏/置顶需实测 |

因此在实施前必须补三后端运行期探针：SDL3 Borderless/AlwaysOnTop、SFML Style::None、raylib UNDECORATED/TOPMOST；探针结果与设计不一致时先修订文档再编码。

## 1. 背景

Launcher 需要无标题栏、无关闭按钮并保持置顶。当前只能通过 Win32 `EnumWindows/SetWindowLongPtr/SetWindowPos` 兜底，不能作为跨平台接口。

现有 `UIWindowFlags::Hidden` / `SetWindowVisible` 只解决可见性；新增：

- 创建期无边框：`UIWindowFlags::Borderless`；
- 运行期置顶：`SetWindowAlwaysOnTop`。

## 2. 公开 API

### 2.1 窗口标志

`include/UICornerstoneAPI.h`：

```c
#define UICORN_WINDOW_FLAG_BORDERLESS 0x00000010u /* 对齐 SDL_WINDOW_BORDERLESS */
```

同步 `Window.h`、`binding/include/UICornerstone.h` 的 `UIWindowFlags::Borderless`；不新增版本字段，默认 flags=0 行为不变。

### 2.2 C ABI

```c
/* 1=置顶，0=取消；成功 1 / 失败 0；幂等。 */
UICORNERSTONE_API int UICornerstone_SetWindowAlwaysOnTop(UIInstance instance, int onTop);
/* 可选查询：引擎本地状态；无窗口/headless/子视口返回 0。 */
UICORNERSTONE_API int UICornerstone_GetWindowAlwaysOnTop(UIInstance instance, int* onTop);
/* 可选：尽力将独立窗口带到前台（不保证 OS 强制焦点）；成功 1/失败 0。 */
UICORNERSTONE_API int UICornerstone_SetWindowForeground(UIInstance instance);
```

语义：

- 仅独立窗口实例：`owner==nullptr && ownsBackend==true`；子视口/共享后端/headless 返回 0，不操作 owner。
- 幂等；失败返回 0 并诊断；不重建实例/控件，不改变布局/焦点字段/主题。
- 同线程 UI 调用；跨线程未定义。
- `GetWindowAlwaysOnTop` 返回引擎本地状态；`onTop==nullptr` 返回 0；旧 DLL Binding 返回 false。

### 2.3 Binding

```cpp
bool SetWindowAlwaysOnTop(bool onTop);
bool IsWindowAlwaysOnTop() const;
bool SetWindowForeground();
```

`UIWindowFlags::Borderless` 同步暴露；旧 DLL 缺入口 false + 单次诊断。

## 3. 后端实现

### 3.1 Window 接口

```cpp
virtual bool setAlwaysOnTop(bool onTop) { (void)onTop; return false; }
virtual bool isAlwaysOnTop() const { return false; }
virtual bool setForeground() { return false; }
```

- 后端显式覆盖，跟踪本地状态；默认失败避免假成功。
- Borderless 是创建期 flag，不提供运行期切换 API（如后续需要另立）。

### 3.2 SDL3

- 创建：`UIWindowFlags::Borderless` 透传为 `SDL_WINDOW_BORDERLESS`（与 Hidden/Resizable 同）。
- 运行期：`SDL_SetWindowAlwaysOnTop`；更新本地 `m_alwaysOnTop`。
- 前台：`SDL_RaiseWindow`（尽力，不保证 OS 焦点）；失败返回 0。
- `isAlwaysOnTop` 返回本地状态；`SDL_SetWindowBordered` 本期不暴露。

### 3.3 SFML

- 创建：Borderless → `sf::Style::None`（无标题栏/关闭按钮）；不再添加 Titlebar/Close/Resize。
- 运行期置顶：SFML 公开 API 未提供跨平台置顶；`setAlwaysOnTop` 返回 0并记录一次不支持诊断。
- 若平台层（Win32/macOS）后续需要，另立平台实现，不伪装跨平台成功。
- 前台：`sf::WindowBase::requestFocus()`（尽力，不保证 OS 焦点）。

### 3.4 raylib

- 创建：Borderless → `FLAG_WINDOW_UNDECORATED`；在 `InitWindow` 前设置。
- 运行期置顶：`SetWindowState(FLAG_WINDOW_TOPMOST)` / `ClearWindowState(FLAG_WINDOW_TOPMOST)`；以 `IsWindowState`/本地状态确认为准；headless 实例返回 0，不碰全局首实例。
- 前台：`SetWindowFocused()`（尽力，不保证 OS 焦点）；headless 返回 0。
- 首实例/headless 边界沿用 CR5003。

### 3.5 CallbackWindow / 插件回调

- `UIBackendCallbacks.version=2` 末尾追加：
  - `setWindowAlwaysOnTop` / `getWindowAlwaysOnTop`；
  - **`setWindowForeground(UIWindowHandle)`**（本 CR B1 补齐，可选，NULL 返回 false/诊断一次）。
- v1 回调表不读取/不安装新增字段；v2 为 NULL 时降级，不假成功。
- `CallbackWindow` 转发；插件模式与静态模式行为一致。
- Borderless 已随 `createWindow(flags)` 透传，无需新增回调。

### 3.6 Borderless 的阴影/非客户区语义

- `Borderless` 的契约是：**无标题栏、无关闭按钮、无系统装饰**；**不承诺无 OS 阴影**。
- Windows DWM 可能在无边框窗口外绘制阴影；引擎本期不操作 DWM/CS_DROPSHADOW/窗口 region，避免把平台私有实现塞入跨平台 API。
- Launcher 若必须完全无阴影，需保留 `LauncherPlatform` 的 Windows 阴影处理；后续如要引擎统一，应另立平台能力 CR，不宣称本 CR 已覆盖。
- 验收需检查窗口外围无标题栏/关闭按钮；若仍有系统阴影，记录为已知平台行为。

### 3.7 BackendCallbacks ABI（B3）

- `UIBackendCallbacks.version` 已存在；本 CR 将新回调表版本定为 **2**。
- 新增字段只追加在结构末尾；**新 core 仅在 `callbacks->version >= 2` 时读取**新增字段。
- **新 core 接受 version 1/2**：version=1 走旧路径、新增能力返回 false；`BackendManager::initialize()` 的检查需从“仅 version == 1”改为“version == 1 或 2”。
- **新 backend DLL（version=2）+ 旧 core**：旧核心现有检查会拒绝 version !=1，**安全失败**；不支持、也不承诺向后兼容。版本配套政策仍要求 DLL/core/Binding/宿主同批。
- **旧 backend DLL（version=1）+ 新 core**：允许启动，新增 AlwaysOnTop/Foreground 回调为 NULL，API 返回 0/诊断一次；不得访问新增字段。
- 回调表 version 与 `BackendAPI.version` 是两套版本，不要混用；本 CR 只调整 `UIBackendCallbacks.version` 的接受范围。
- 静态模式与插件模式行为一致；CallbackWindow 读取新增回调前做 version/空指针检查。
- 回归：新 core + v1 backend DLL、新 core + v2 backend DLL；旧 core + v2 backend DLL 预期安全失败（记录为版本不配套，不作为兼容缺陷）。

## 4. 兼容性与能力矩阵

| 能力 | SDL3 | SFML | raylib | headless |
|------|------|------|--------|----------|
| Borderless 创建 | ✅ | ✅ Style::None | ✅ UNDECORATED | — |
| AlwaysOnTop 运行期 | ✅ | ❌ 返回 0 | ✅ TOPMOST | ❌ 0 |
| GetAlwaysOnTop | ✅ 本地 | — | ✅ 本地 | 0 |
| SetWindowForeground | ✅ RaiseWindow | ✅ requestFocus | ✅ SetWindowFocused | ❌ 0 |
| 无系统阴影 | 不承诺 | 不承诺 | 不承诺 | — |

- 默认 flags 0 行为不变；Hidden/SetWindowVisible 语义不变。
- `GetNativeWindowHandle` 仍为 backend-specific，不作为本 API 的替代。

## 5. 测试/验收

1. SDL3：`Create(Borderless)` 无标题栏/关闭按钮；首帧无边框闪烁；`SetWindowAlwaysOnTop(1/0)` 往返/幂等；`SetWindowForeground` 调用成功；控件树/布局/主题/焦点字段不变。
2. Borderless 阴影语义：外围无标题栏/关闭按钮；系统阴影按设计记录；不得出现透明/黑边非客户区。
2. 子视口/共享实例/headless/非法实例/空指针返回 0，不崩。
3. SFML：Borderless 创建成功；AlwaysOnTop 返回 0 并诊断，不假成功。
4. raylib：Borderless + TOPMOST 往返；headless 第二实例返回 0 不影响首实例。
5. Binding：旧 DLL 缺入口 false；新 DLL 正常；`UIWindowFlags::Borderless` 可用。
6. ABI：旧 backend DLL(version=1)+新 core 正常启动、新增能力 false/诊断；新 backend DLL(version=2)+旧 core 安全失败（版本不配套）；新 core 接受 1/2；v2 foreground 回调可用、NULL 降级；静态/插件一致。
6. Launcher：去掉 Win32 样式兜底，ready 前 Hidden、ready 后显示、无标题栏、置顶、失败/超时行为不变。
7. 回归：Debug/Release `auto=2`（272/0）、`auto=3`、`validate_layout --strict`、`test_cr_theme`、`test_window_visibility`。
8. 文档/API 映射/schema（非布局）同步；三后端能力矩阵标注。

## 6. 风险与限制

- SFML 无跨平台置顶 API，只能返回不支持；Launcher 若必须 SFML 置顶需平台层实现，不能宣称跨平台。
- Borderless 创建期 SFML 用 `Style::None` 会同时去掉 resize 边框；若需要无标题但可缩放，需另立运行期/平台能力。
- 置顶可能受 OS 策略/全屏窗口影响，API 只保证调用成功，不保证永远在最前。
- 子视口/共享实例必须严格拒绝，避免操作 owner 主窗口。
- raylib 单窗口/headless 语义不能因置顶 API 改变。
- Borderless 不保证去除 OS 阴影；Launcher 的 Windows 阴影处理需保留到平台能力后续 CR。
- 前台激活是尽力行为，OS 可能拒绝抢焦点；Launcher 需有超时/降级。
- `UIBackendCallbacks` 版本门禁必须实现；否则旧 backend DLL 存在越界读取风险。

## 7. 实施顺序

1. Flags/Window 接口 + SDL3（Borderless + AlwaysOnTop）。
2. C ABI/Binding + CallbackAdapters/插件回调 + docs。
3. SFML/raylib 能力实现与矩阵测试。
4. Launcher 去 Win32 兜底，三后端回归、发布同步。

## 修订记录

- 2026-11-06 初稿：基于 Launcher 需求、CR5003 窗口 API 与三后端现有能力（SDL3 Borderless/AlwaysOnTop、SFML Style::None/无置顶、raylib UNDECORATED/TOPMOST）；待评审后实施。
- 2026-11-06 按评审修订：B1 前台激活 API、B2 Borderless 阴影语义、B3 UIBackendCallbacks version=2、B4 运行期探针门禁；补 Get/失败诊断/Launcher 临时 Win32 边界。
- 2026-11-06 二次复核修订：新 core 接受回调表 version 1/2；v1 不读新字段；v2 后端 + 旧 core 明确安全失败；不改“旧核心忽略新字段”的错误声明。
- 2026-11-06 三次复核修订：v2 回调表补 `setWindowForeground`，CallbackWindow 转发与 v1/v2 门禁；补 ABI 用例。

## 8. 设计评审响应（2026-11-06）

| 评审项 | 处理 |
|--------|------|
| B1 前台激活 | 增加 `UICornerstone_SetWindowForeground`/Binding，三后端 best-effort（SDL RaiseWindow / SFML requestFocus / raylib SetWindowFocused）；不保证 OS 强制焦点，子视口/headless 0。 |
| B2 无阴影语义 | 明确 Borderless 仅去除装饰，不承诺无 OS 阴影；Windows 阴影由 LauncherPlatform 保留，后续平台能力另立 CR；补非客户区验收。 |
| B3 BackendCallbacks ABI | `version` 升 2；新增字段追加末尾且仅在 version>=2 读取；旧 DLL/新核心安全 false/诊断，新 DLL v2/旧核心安全失败（不忽略）；静态/插件一致。 |
| B4 运行期探针 | 保留“实施前必须跑三后端探针”门禁；未通过先修订能力矩阵。 |
| Borderless+resize | 文档明确 Borderless 后不能依赖 Resizable（SFML Style::None 去掉 resize）。 |
| 失败诊断/本地状态 | AlwaysOnTop 失败记录 unsupported/headless/invalid；Get 返回本地状态，最小化/遮挡不影响。 |
| Launcher Win32 | CR 交付后仅保留 LauncherPlatform 的阴影/进程能力；样式/置顶/前台切换走引擎 API。 |

## 9. 二次复核响应（2026-11-06）

| 评审项 | 处理 |
|--------|------|
| B3 版本配套矛盾 | 采纳方案 1：新 core 接受 version 1/2，旧 backend 走旧路径；新 backend v2 + 旧 core 明确安全失败，不承诺忽略新增字段。 |
| 读取门禁 | 新增字段仅在 version>=2 读取；v1 不访问新字段；`BackendManager` 检查改为接受 1/2。 |
| 版本概念 | 明确回调表 version 与 BackendAPI.version 是两套版本；本 CR 只改回调表接受范围。 |
| B2 阴影 | 本 CR 不承诺去除 DWM 阴影；LauncherPlatform 保留 Windows 阴影处理；完全封装另立 CR。 |
| 前台激活 | 保持 best-effort；Launcher 保留超时/降级，不依赖必然成功。 |
| B4 探针 | 实施前记录三后端实测，尤其 raylib TOPMOST/SetWindowFocused。 |
| 进程启动边界 | CreateProcessW/PID/进度仍归 LauncherPlatform，不属于本 CR。 |

## 10. 三次复核响应（2026-11-06）

| 评审项 | 处理 |
|--------|------|
| B1 回调表遗漏 | 在 §3.5 明确 v2 回调表末尾增加 `setWindowForeground(UIWindowHandle)`；CallbackWindow 转发；v1 不读取/安装；v2 NULL 降级并诊断一次；补 ABI 用例。 |
| B2 边界 | 保持“不解决 DWM 阴影、LauncherPlatform 保留 Windows 阴影”的明确限制。 |
| B4 探针 | 保留实施前门禁；raylib TOPMOST/SetWindowFocused 实测优先。 |
| 前台 best-effort | 保持不保证 OS 焦点，Launcher 超时/降级。 |

## 11. 实施与验证（2026-11-06）

- 已实现 `UIWindowFlags::Borderless`、`SetWindowAlwaysOnTop/GetWindowAlwaysOnTop`、`SetWindowForeground` 及 Binding。
- `UIBackendCallbacks.version=2`；v1/v2 版本检查、CallbackWindow 转发和插件回调已实现。
- `test_window_visibility` 扩展通过：Borderless+Hidden 创建、AlwaysOnTop 往返/幂等、Foreground、子视口拒绝。
- SDL3 运行期已验证；SFML Borderless 已编译、AlwaysOnTop 按设计返回不支持；raylib 已编译，TOPMOST/SetWindowFocused 运行期探针待补。
- Release 已同步：`UICornerstone.dll` SHA256 `3a53d466b01f2c6433b5d0b8cd2b2d8e382d3fa9978391240301f342ab12c35f`。
