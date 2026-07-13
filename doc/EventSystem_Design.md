# UICornerstone 事件系统设计文档

## 1. 概述

UICornerstone 的事件系统处理所有用户输入（鼠标、键盘、触摸、文本）和窗口事件（缩放、移动、关闭），将后端无关的输入事件分发给控件树。

### 1.1 设计目标

- **后端无关**：事件类型和数据结构不依赖任何图形后端（SDL3/SFML/Raylib）
- **跨 DLL 安全**：事件数据结构为固定大小 POD，可在 DLL 边界传递
- **双向兼容**：`Event` 类的旧 API（`EventName` + `std::any`）能自动映射到新 API（`EventType` + union）
- **可扩展**：通过 `BeforeEventHandlingWatcher` / `AfterEventHandlingWatcher` 机制支持控件级事件监听

### 1.2 核心文件

| 文件 | 角色 |
|------|------|
| `include/EventTypes.h` | 核心枚举 + 事件数据结构体 |
| `include/StateMachine.h` | `Event` 类 + `StateMachine` 模板 |
| `include/EventQueue.h` | `EventQueue` 单例 + 事件队列 + watcher |
| `include/InputBackend.h` | `InputBackend` 抽象接口 |
| `include/FocusManager.h` | `FocusManager` 焦点管理 |
| `include/ControlBase.h` | `Control` 接口 + `TopControl::eventLoopEntry()` |
| `src/MainWindow.cpp` | `processEvents()` 主循环 |
| `src/EventQueue.cpp` | 队列操作 + watcher 通知 |
| `src/ControlBase.cpp` | `handleEvent()` 子控件分派 + `triggerEvent()` |
| `src/Bench.cpp` | `inputControl()` + Tab 焦点拦截 |
| `src/FocusManager.cpp` | 焦点导航完整实现 |

---

## 2. 核心类型

### 2.1 EventType 枚举

`include/EventTypes.h` 定义了所有事件类型：

```cpp
enum class EventType : uint8_t {
    None,
    MouseMove, MouseDown, MouseUp, MouseWheel,
    KeyDown, KeyUp, TextInput, TextEditing,
    WindowResize, WindowMoved, WindowClose,
    FocusGained, FocusLost,
    FingerDown, FingerUp, FingerMotion,
    Custom,              // 控件内部自定义事件
};
```

### 2.2 事件数据结构体

所有事件数据结构体都是固定大小、可平凡复制的 POD，安全跨越 DLL 边界：

| 结构体 | 字段 | 关联 EventType |
|--------|------|----------------|
| `EventMousePos` | `float x, y` | `MouseMove`, `FingerDown/Up/Motion` |
| `EventMouseButton` | `float x, y; MouseButton button` | `MouseDown`, `MouseUp` |
| `EventMouseWheel` | `float x, y; float scrollX, scrollY` | `MouseWheel` |
| `EventKey` | `KeyCode keycode; KeyMod mod; int32_t scancode; bool repeat` | `KeyDown`, `KeyUp` |
| `EventTextInput` | `char text[32]` | `TextInput` |
| `EventTextEditing` | `char text[32]; int32_t start; int32_t length` | `TextEditing` |
| `EventResize` | `int width, height` | `WindowResize` |
| `EventWindowMoved` | `int x, y` | `WindowMoved` |
| `EventFocus` | `bool focused` | `FocusGained`, `FocusLost` |

### 2.3 Event 类

`Event` 类使用 union 存储事件数据：

```cpp
class Event {
public:
    EventType m_type;
    union {
        EventMousePos    mousePos;
        EventMouseButton mouseButton;
        EventMouseWheel  mouseWheel;
        EventKey         keyEvent;
        EventTextInput   textInput;
        EventTextEditing textEditing;
        EventResize      resizeEvent;
        EventWindowMoved windowMoved;
        EventFocus       focusEvent;
        char _pad;
    };
    int   customInt;     // 外部 union — 可同时设置
    void* customPtr;     // 外部 union — 可同时设置
};
```

关键设计点：
- `customInt` 和 `customPtr` 在 **union 之外**，可独立设置（`EditBox::setFocused()` 需要同时设 `ON_FOCUS + this`）
- `copyUnion()` 用 `switch(m_type)` 复制正确的 union 成员
- 不存在 `EventName` + `std::any` 旧构造函数（Phase 12 迁移后移除，仅 `ON_FOCUS` 以 `customInt` 保留）

### 2.4 辅助枚举

**KeyCode**（`uint16_t`）：
- ASCII 范围（0x20-0x7E）：可打印字符直接用 ASCII 值，跨后端一致
- 控制键（0x08-0x1B）：`Backspace=0x08`, `Tab=0x09`, `Return=0x0D`, `Escape=0x1B`, `Del=0x7F`
- 扩展范围（0x100+）：`Left/Right/Up/Down`, `Home/End`, `PageUp/PageDown`, `F1-F24`, 修饰键, 小键盘等

**KeyMod**（`uint16_t`，位掩码）：
- 值匹配 SDL_Keymod 实现零成本映射：`LShift=0x0001`, `RShift=0x0002`, `LCtrl=0x0040`, `RCtrl=0x0080` 等
- 复合：`Shift=LShift|RShift`, `Ctrl=LCtrl|RCtrl`

**MouseButton**（`uint8_t`）：值匹配 SDL 按键编号：`None=0, Left=1, Right=2, Middle=3, X1=4, X2=5`

---

## 3. 事件流转

### 3.1 完整流程

```
InputBackend::pollEvent(event)
  → Event(EventType, union 字段)
    → MainWindow::processEvents()
      → WindowClose? → 退出
      → WindowResize? → onWindowResized()
      → WindowMoved? → onWindowMoved()
      → 其他 → BENCH->inputControl(event)
        → EventQueue::pushEventIntoQueue(event)   [入队]
        → (在 eventLoopEntry 中出队)
          → notifyBeforeEventHandlingWatchers()
          → Bench::handleEvent(event)              [分派]
          → notifyAfterEventHandlingWatchers()
```

### 3.2 InputBackend → Event 转换

每个后端实现 `InputBackend::pollEvent(Event&)`，将原生事件填入 `Event` union：

- SDL3：`src/backend/sdl3/InputBackend.cpp` — 使用 `SDL_GetKeyboardState` / `SDL_GetMouseState` 等
- SFML：`src/backend/sfml/InputBackend.cpp` — 使用 `sf::Event` / `sf::Keyboard` / `sf::Mouse`
- Raylib：`src/backend/raylib/InputBackend.cpp` — 使用 `IsKeyPressed` / `IsMouseButtonPressed` / `GetCharPressed` 等（需 `newFrame()` 每帧调用 `PollInputEvents()`）

### 3.3 MainWindow 事件入口

`MainWindow::processEvents()` 在 `src/MainWindow.cpp`：

```cpp
void MainWindow::processEvents() {
    m_inputBackend->newFrame();               // raylib 需要每帧 PollInputEvents
    Event event;
    while (m_inputBackend->pollEvent(event)) {
        switch (event.m_type) {
        case EventType::WindowClose:
            m_running = false;
            break;
        case EventType::WindowResize:
            onWindowResized(event.resizeEvent.width, event.resizeEvent.height);
            break;
        case EventType::WindowMoved:
            onWindowMoved(event.windowMoved.x, event.windowMoved.y);
            break;
        default:
            auto sharedEvent = make_shared<Event>(event);
            m_bench->inputControl(sharedEvent);
            if (m_app) m_app->onEvent(event);  // AppCallbacks
            break;
        }
    }
}
```

### 3.4 Bench → 控件树分派

`Bench::inputControl()` 入队事件，`eventLoopEntry()` 出队并分派：

```cpp
void TopControl::eventLoopEntry() {       // ControlBase.h
    shared_ptr<Event> eventInQueue;
    while (eventInQueue = popEventFromQueue()) {
        EventQueue::notifyBeforeEventHandlingWatchers(eventInQueue);
        handleEvent(eventInQueue);          // 进入控件树
        EventQueue::notifyAfterEventHandlingWatchers(eventInQueue);
    }
}
```

### 3.5 控件树事件分派

`ControlImpl::handleEvent()` 按 Z 顺序反向迭代子控件，遇到 `handleEvent()` 返回 `true` 时停止传播：

```cpp
bool ControlImpl::handleEvent(shared_ptr<Event> event) {
    if (!getVisible() || !getEnable()) return false;
    // 后续子控件分派逻辑（逆向 Z-order）
}
```

### 3.6 Watcher 机制

`EventQueue` 支持控件注册为特定事件类型的观察者：

- **BeforeWatcher**：事件进入控件树前通知。若返回 `true`，事件被消费（`EventQueue::notifyBeforeEventHandlingWatchers` 从 `EventQueue` 触发，返回 true → 跳过 `handleEvent`）。
- **AfterWatcher**：事件被控件树处理后通知。EventQueue 只负责通知，消费状态由 watcher 自己决定。

使用场景：
- `EditBox` — 监听外部 `MouseDown`（通过 `ON_FOCUS` 自定义事件 `customInt`）失去焦点
- `Popup` / `Dialog` — 监听 `MouseDown` 外部点击关闭、`KeyDown` ESC/Enter 关闭
- `ColorPicker` — ESC=取消，Enter=确定

---

## 4. 控件 handleEvent 模式

所有控件的 `handleEvent()` 遵循统一模式：

```cpp
bool Button::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;

    // Step 1: 从 union 提取鼠标位置
    float mx, my; bool gotPos = false;
    if (event->m_type == EventType::MouseMove) {
        mx = event->mousePos.x; my = event->mousePos.y; gotPos = true;
    } else if (event->m_type == EventType::MouseDown ||
               event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    } else if (event->m_type == EventType::FingerDown || ...) {
        mx = event->mousePos.x; my = event->mousePos.y; gotPos = true;
    }

    // Step 2: 位置事件 → 检测包含性
    if (gotPos) {
        SRect drawRect = getDrawRect();
        if (drawRect.contains(mx, my)) {
            // 按 EventType + MouseButton 分派
            if (event->m_type == EventType::MouseDown &&
                event->mouseButton.button == MouseButton::Left) { ... return true; }
            // ...
        }
    }

    // Step 3: 键盘事件
    if (event->m_type == EventType::KeyDown && getFocused()) { ... return true; }

    // Step 4: 回退到子控件分派
    return ControlImpl::handleEvent(event);
}
```

特点：
- 零 `std::any_cast` / `try-catch`
- 使用 `EventType` switch 代替 `EventName` 比较
- `< 0.01ms` 事件分派延迟（非阻塞）

---

## 5. StateMachine 集成

`StateMachine` 模板类（`include/StateMachine.h`）提供有限状态机支持：

```cpp
template<typename State>
class StateMachine {
protected:
    State m_currentState;
    unordered_map<State, EnterLeaveHandler> m_enterStateHandlers;
    unordered_map<State, EnterLeaveHandler> m_leaveStateHandlers;
    unordered_map<State, EventHandler> m_stateEventHandlers;

public:
    void setState(State s);
    bool stateEvent(shared_ptr<Event>);
    void registerEnterStateHandler(State, EnterLeaveHandler);
    void registerLeaveStateHandler(State, EnterLeaveHandler);
    void registerStateEventHandler(State, EventHandler);
};
```

用法：控件可以按当前状态注册不同的事件处理器，例如：
- 正常状态 → 处理鼠标点击
- 编辑状态 → 处理键盘输入
- 禁用状态 → 忽略所有事件

---

## 6. 焦点系统

### 6.1 FocusManager

`FocusManager` 管理所有可聚焦控件：

- **注册**：控件调用 `setFocusable(true)` 时自动注册
- **注销**：控件销毁时自动注销
- **边界**（FocusBoundary）：某些容器（`Bench`、`Popup`）是焦点边界，Tab 在边界内循环
- **Tab 循环**：`Tab` → `focusNext()`，`Shift+Tab` → `focusPrev()`，在边界内环绕
- **跨边界**：`Ctrl+Tab` → `focusNextScope()`，`Ctrl+Shift+Tab` → `focusPrevScope()`

### 6.2 Bench 焦点拦截

`Bench::handleEvent()` 拦截 Tab 键（`src/Bench.cpp`）：

```cpp
if (event->m_type == EventType::KeyDown) {
    if (event->keyEvent.keycode == KeyCode::Tab) {
        if (event->keyEvent.mod & KeyMod::Ctrl)
            GET_FOCUSMANAGER->focusNextScope();
        else
            GET_FOCUSMANAGER->focusNext(GET_FOCUSMANAGER->getCurrentFocused());
        return true;
    }
    if (event->keyEvent.keycode == KeyCode::Tab &&
        (event->keyEvent.mod & KeyMod::Shift))
        GET_FOCUSMANAGER->focusPrev(GET_FOCUSMANAGER->getCurrentFocused());
        return true;
    }
    // ... Ctrl+Tab ...
}
```

### 6.3 焦点环绘制

`ControlImpl::drawFocusRing()` 绘制三层焦点环，在任何背景色下保证可见：

```
黑线（inset 0, alpha 150）
+ 白线（inset 1, alpha 150）
+ 用户颜色（inset 2, alpha 255）
```

只响应用户颜色。默认 `m_focusRingAlwaysVisible = true`。

---

## 7. 向后兼容设计

Phase 12 迁移保留了旧 `EventName` 枚举和 `std::any` 参数，但仅在 `EventType::Custom` 内部路由中使用：

| 旧 API | 新 API | 迁移状态 |
|--------|--------|----------|
| `EventName::MOUSE_LBUTTON_DOWN` | `EventType::MouseDown` + `mouseButton` | ✅ 已迁移 |
| `EventName::TEXT_INPUT` | `EventType::TextInput` + `textInput` | ✅ 已迁移 |
| `EventName::ON_FOCUS` | `EventType::Custom` + `customInt = ON_FOCUS` | ✅ 保留（内部使用） |

---

## 8. 跨 DLL 安全

- `EventType` 为 `uint8_t` — 所有平台大小一致
- `KeyCode` 为 `uint16_t`，`KeyMod` 为 `uint16_t`，`MouseButton` 为 `uint8_t` — 固定大小
- 所有事件数据结构体为平凡可复制 POD — 通过 `UIBackendCallbacks` 桥接安全传递
- `Event` 类通过 `shared_ptr<Event>` 在队列中传递

---

## 9. 三后端 InputBackend 差异

| 特性 | SDL3 | SFML | Raylib |
|------|------|------|--------|
| 事件轮询 | `SDL_PollEvent` | `window.pollEvent` | `PollInputEvents`（在 `newFrame()` 中） |
| 键盘状态 | `SDL_GetKeyboardState` | `sf::Keyboard::isKeyPressed` | `IsKeyDown` |
| 鼠标位置 | `SDL_GetMouseState` | `sf::Mouse::getPosition(window)` | `GetMousePosition` |
| 文本输入 | `SDL_StartTextInput` | `sf::Event::TextEntered` | `GetCharPressed` |
| 按键重复 | SDL 原生支持 | 需手动实现 | 需手动实现 |
| newFrame | 空操作 | 空操作 | `PollInputEvents()` + 重置相位机状态 |

---

## 10. 测试验证

测试文件 `test/test_fromsource_{sdl3,sfml,raylib}.cpp` 通过 C ABI 桥接验证事件系统的跨后端正确性：

- 所有三后端的以下事件类型已验证：MouseMove/MouseDown/MouseUp/KeyDown/KeyUp/TextInput/MouseWheel/WindowResize/WindowClose
- 控件响应（Button 点击、EditBox 文本输入、Slider 拖拽）在所有后端一致

---

## 11. 变更历史

| 日期 | 变更 |
|------|------|
| 2026-06-02 | Phase 6：InputBackend 抽象 + EventTypes.h + 双轨驱动（EventName + EventType） |
| 2026-06-05 | Phase 12：所有 8 控件 handleEvent 从旧 API 迁移至 union API；`Event(EventName,any)` 构造自动映射 |
| 2026-06-07 | Raylib InputBackend 实现 + `newFrame()` 机制 |
| 2026-07-11 | ColorPicker/Dialog FocusBoundary + watcher 集成 |
| 2026-07-12 | `customInt`/`customPtr` 移出 union，支持同时设置 |
