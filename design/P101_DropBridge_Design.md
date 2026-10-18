# P101_DropBridge_Design — 真实 OS 拖放未达回调（UIEvent 桥缺 DropFile 转换）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-17 追加：P0-101 高）
> 前置：P0-100 已实施并同步（合成通路正常；真实通路缺口为本批）
> 状态：**已放行，已实施**（2026-10-17；复核：`requirements/P101_复核意见.md`——通过）
> 实施备注（2026-10-17）：① `UIEventDropFile` 改指针式（`{const char* path; float x,y; int isText;}`，超长路径不截断）；② `bridge_pollEvent` 增 DropFile 分支（DLL 侧 thread_local 缓冲）；③ `CallbackInputBackend::pollEvent` 增 DropFile 转换（同步拷贝）；④ `uiEventToEvent` 改指针式；⑤ **`PushUIEvent` 深拷贝**——注入队列条目改 `UIContext::QueuedUIEvent{UIEvent ev; std::string dropPath;}`（采纳复核补充 4：push 后源串立即释放仍正确；消费时指针重指队列内拷贝）；⑥ `MainWindow::processEvents`（Mode 1）轮询 switch 与注入队列均补 DropFile 分发；⑦ 验证：探针 `temp_p101_probe`（转正）**5/5**——真实 WM_DROPFILES（中文/空格）/ 真实**超长路径 307 字符** / 指针式合成+立即释放 / DROP_TEXT / 解绑；P0-98/99/100 探针 9/9（合成同步改指针式）；P0-93/94、P0-96/97 探针回归 OK；全量回归仅 4 项预存；Debug/Release 零警告；⑧ 复核补充 2 观察（`bridge_pollEvent` 亦无 `WindowMoved`）：`MainWindow::onWindowMoved` 仅更新内部 `m_pos`（`getWindowPos` 读取），无用户可见依赖——按复核意见**不扩面**，记为 backlog。
> ⑨ 调试观察（供后续排查参考）：`UICornerstone_GetNativeWindowHandle` 返回后端 `Window*` 包装对象（**非** OS HWND）——合成 WM_DROPFILES 需经 `EnumWindows`（类名 `SDL_app` + 本进程 PID）取真实 HWND；宿主视角该值为不透明句柄（API 文档措辞可后续澄清，本批未改）。

---

## 1. 复现（引擎侧已 100% 复现，与设计器现象一致）

探针 `Temp/temp_p101_probe`（无需人工拖拽）：`CreateInstanceFromPlugin("sdl3")` + `SetDropFileCallback` + **PostMessage WM_DROPFILES**（合成真实 Shell 拖放消息，走完整 SDL/后端/引擎链路）→ **回调不触发（cnt=0）** ✗；同探针 `PushUIEvent(UI_EVENT_DROP_FILE)` → 回调正常 ✓（与设计器"合成 OK、实机 ✗"完全吻合）。

## 2. 根因（逐环日志定位，证据链完整）

事件链与实测：

```
SDL(WM_DROPFILES) → 后端 DLL SDL3InputBackend::pollEvent
   （P0-100 映射 DropFile ✓，实测返回 true type=17）
→ ★ bridge_pollEvent（src/backend/BackendBridge.h:163-226）
   —— Event → UIEvent 转换 switch 无 EventType::DropFile 分支 → default: return 0 静默丢弃 ✗
→ CallbackInputBackend::pollEvent（src/CallbackAdapters.cpp:249）
   —— 收 false → 引擎 pumpInstanceEvents 循环跳过 ✗
→ 实例回调从未触发
```

- **断点即 `bridge_pollEvent` 的 `default: return 0`**（BackendBridge.h:221-222）；该桥为引擎（static/DLL 侧 `CallbackInputBackend`）与后端插件 DLL 之间的 **UIEvent C ABI 通道**，ABI 中无 DropFile 表示。
- 合成通路经 `UICornerstone_PushUIEvent` 直达引擎注入队列（**不过桥**）→ 正常（解释设计器证据②）。
- 设计器 DLL md5/时间戳核验通过仍失败的原因：两侧都"构建正确"，缺口在**桥转换**而非某个 DLL 陈旧。
- 附带发现：`MainWindow::processEvents`（Mode 1 owned-loop）default 分支同样不处理 DropFile（本批一并补）。

## 3. 方案

### 3.1 ABI：`UIEventDropFile` 改**指针式载荷**（长路径不截断——设计器核心动机）
```c
typedef struct { const char* path; float x; float y; int isText; } UIEventDropFile;
```
- 语义：`path` 指向 NUL 结尾字符串，**生产者保证在被消费前有效**；引擎在转换时**立即拷贝**为内部 `std::string`（桥用 DLL 侧 `thread_local` 缓冲，下一次 poll 覆盖——消费是同步的，安全）。
- 原内联 120B 布局**作废**：>120B 超长路径会被截断，无法满足"拖超长路径 .csd 打开"的核心用例。

### 3.2 `bridge_pollEvent`（BackendBridge.h）
增 `case EventType::DropFile:`——`thread_local std::string` 缓冲拷贝 `event.dropPath`；`ue->data` 填 `UIEventDropFile{path 指针, x, y, isText=event.customInt}`。

### 3.3 `CallbackInputBackend::pollEvent`（CallbackAdapters.cpp）
增 `case UI_EVENT_DROP_FILE:`——读指针拷贝 → `event.dropPath`；x/y；`customInt = isText`；`m_type = EventType::DropFile`。

### 3.4 `uiEventToEvent`（UICornerstoneAPI.cpp）
改读指针式布局（宿主/探针合成的指针有效期约定同 3.1）。

### 3.5 `MainWindow::processEvents`（MainWindow.cpp，Mode 1）
增 `case EventType::DropFile: DispatchDropFileEvent(m_context, event); break;`（复用 P0-100 已加的内部钩子；include DragDrop.h）。

### 3.6 文档与探针
- `UIEventDropFile` 头注释/说明更新（指针语义）；
- P100 设计文档补"桥缺口修复"实施备注；
- 探针 `temp_p101_probe` **转正**为真实通路回归：WM_DROPFILES 合成 + 超长路径（>260）+ 中文/空格 + DROP_TEXT。

## 4. 验收与测试矩阵

1. **真实通路（探针）**：PostMessage WM_DROPFILES → 回调收到完整路径 + 坐标 + `isText=0`；**>260 超长路径完整传递**；含中文/空格路径；
2. 合成通路（指针式 UI_EVENT_DROP_FILE）不回归；解绑、多实例 windowID 路由不回归；
3. P0-98/99/100 探针全过；全量回归仅 4 项预存；Debug/Release 零警告；
4. 设计器联测：资源管理器**真实拖** `.csd`（含超长路径）入窗口 → 打开成功。

## 5. 影响面

- 源码：`src/backend/BackendBridge.h`（所有后端插件共享，一次修复覆盖 sdl3/sfml/raylib）、`src/CallbackAdapters.cpp`、`src/UICornerstoneAPI.cpp`、`src/MainWindow.cpp`、`include/UICornerstoneAPI.h`（布局+注释）；
- 无 JSON/schema 变更；C ABI：`UI_EVENT_DROP_FILE` 载荷布局变更（发布仅一轮；设计器未依赖内联布局——其合成探针同步改指针式）。

## 6. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 指针式载荷（长路径不截断）vs 内联 120B（截断） | 指针式 |
| Q-2 | Mode 1（MainWindow owned-loop）本批一并补 DropFile 分发 | 是 |
| Q-3 | 探针转正为真实通路回归（WM_DROPFILES 合成，无需人工拖拽） | 是 |
