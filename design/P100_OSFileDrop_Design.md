# P100_OSFileDrop_Design — OS 文件拖放事件暴露（drop-file，资源管理器拖入）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-17 追加：P0-100 中）
> 状态：**已放行，已实施**（2026-10-17；复核：`requirements/P098_099_100_复核意见.md`——通过，附 2 点集成注意）
> 实施备注（2026-10-17）：① EventType::DropFile + Event.dropPath/dropX/dropY（std::string 不限长）+ SDL3 后端映射（DROP_FILE/DROP_TEXT，windowID 过滤，data 原样 UTF-8 透传）；② `UICornerstone_SetDropFileCallback`（ctl=NULL；`data.dropFile.{path,x,y,isText}`）；③ **接入约定已写入 API 头注释 + 手册 5.7.6 + binding 声明**："泵内触发——仅轻量记录，重活延帧"（复核注意 1）；④ 内部队列拖放经 `DispatchDropFileEvent`（DragDrop.h 内部钩子 + eventLoopEntry 前置）——**>260 长路径走 C++ 层直注内部 Event 验证**（复核注意 2；PushUIEvent 载荷 ≤120B）；⑤ 验证：探针 7（回调 path/坐标/isText）/8（解绑）/9（307 字符长路径 + DROP_TEXT）全过；⑥ **P0-101 桥缺口修复**（真实 OS 拖放曾因 `bridge_pollEvent` 无 DropFile 转换被静默丢弃；已修复——指针式桥接、超长路径不截断，详见 `P101_DropBridge_Design.md`）。

---

## 1. 背景与现状核实

**背景**：用户需打开超长路径（>MAX_PATH）项目文件；Windows 文件对话框在未启用 LongPathsEnabled 时无法导航超长路径（设计器已提供命令行入口过渡）。**资源管理器拖放**是无需注册表的自然入口（拖 `.csd` 到窗口 → 打开）。

| # | 断言 | 核实结果 |
|---|---|---|
| ① | SDL3 后端不处理 DROP 事件 | ✓ `src/backend/sdl3/InputBackend.cpp::pollEvent`（:207-360）无 `SDL_EVENT_DROP_*`（全库零命中）；未映射事件被丢弃，宿主无从拦截（事件由引擎消费）✗ |
| ② | 内部 Event 无 drop 载体 | ✓ `include/EventTypes.h` EventType 无 drop 类型；`include/StateMachine.h` Event = union 数据 + customInt/customPtr（无字符串成员） |
| ③ | C ABI 无 drop 入口 | ✓ UIEventType（API 头:75-87）无 drop；UIEventData 有 `strVal` ✓ 与 `eventName` ✓（可承载路径/事件名）；无实例级事件回调 API |
| ④ | **真实通路缺口（P0-101 发现，已修复）** | 后端 DLL → 引擎经 `bridge_pollEvent`（`src/backend/BackendBridge.h`，UIEvent C ABI 桥）转换——原 switch **无 `DropFile` 分支 → `default: return 0` 静默丢弃** ✗：合成通路（PushUIEvent 不过桥）正常、真实 OS 拖放永不到达回调（DLL 侧映射正确、md5 核验亦无法发现）；已修复——指针式桥接 + `CallbackInputBackend`/`uiEventToEvent` 同步转换 + `PushUIEvent` 深拷贝（`P101_DropBridge_Design.md`） |

## 2. 方案

### 2.1 内部事件
- `EventType` 增 `DropFile`（EventTypes.h）；
- `Event` 增非 union 成员：`std::string dropPath; float dropX, dropY;`（**长路径不设上限**；拷贝构造/赋值同步拷贝）；
- 语义：`dropPath`=绝对路径；`dropX/Y`=落点窗口像素（SDL3 `SDL_DropEvent` 带 x/y；无则 -1）。

### 2.2 后端（SDL3 InputBackend）
- 两个窗口过滤 switch 增 `SDL_EVENT_DROP_FILE`（`SDL_DropEvent.windowID` 过滤，多窗口正确路由）；
- 映射：`data` → `dropPath`；`x/y` → `dropX/Y`；
- 可选同批：`SDL_EVENT_DROP_TEXT` → 同 `DropFile`（`customInt=1` 标记 text，Q-2）。

### 2.3 引擎路由 + C ABI
- `UICornerstone_ProcessEvents` 收到 `DropFile` → 调实例级回调（若注册；未注册忽略）；
- 新 API：`UICornerstone_SetDropFileCallback(UIInstance instance, UICallback cb, void* userData)`（`cb=nullptr` 解绑）；
- 回调参数：`ctl=nullptr`；`UIEventData.eventName="drop-file"`；union 增 `struct { const char* path; float x, y; int isText; } dropFile;`（`path`=实例缓冲，回调期有效）；
- `UIEventType` 增 `UI_EVENT_DROP_FILE` + UIEvent 载荷布局（path ≤112B + x/y）+ `PushUIEvent` 映射（宿主/探针可合成）。

### 2.4 Binding
- `UICornerstone::SetDropFileCallback(std::function<void(const std::string& path, float x, float y)>)`（实例级；空 function 解绑）。

## 3. 验收与测试矩阵
1. 探针合成 `UI_EVENT_DROP_FILE`（带路径）→ 回调收到**完整路径 + 坐标**；
2. 解绑（`cb=nullptr`）后不再触发；
3. **长路径（>260）完整传递**（核心动机）；
4. 多实例/多窗口：drop 按 windowID 路由到对应实例（SDL 层过滤）；
5. 未注册回调时事件被安全忽略（无崩溃/无日志噪音）；
6. 全量回归（仅 4 项预存）；
7. 设计器联测（人工）：资源管理器拖 `.csd` 入窗口 → 打开成功（超长路径用例）。

## 4. 影响面
- 源码：`include/EventTypes.h`、`include/StateMachine.h`、`src/backend/sdl3/InputBackend.cpp`、`src/UICornerstoneAPI.cpp/.h`、binding（DynamicApi/UICornerstone）；
- 文档：capi/binding 速查增 `SetDropFileCallback` 行；5.7.6 边界注（OS 文件拖入走实例回调）；integration 宿主接入说明；
- 无 JSON/schema 变更。

## 5. 待确认（Q）
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 回调形态：专用 `SetDropFileCallback`（vs 通用实例事件回调框架） | 专用（最小面；通用框架后续按需） |
| Q-2 | `DROP_TEXT` 一并支持（`customInt=1` 标记） | 同批支持 |
| Q-3 | 坐标字段进 ABI（`dropFile.x/y`） | 进（落点定位用途） |
