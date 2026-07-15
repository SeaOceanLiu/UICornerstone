# 焦点系统设计文档

## 1. 概述

在桌面 GUI 系统中，焦点（Focus）决定键盘事件的目标控件。目前 EditBox、TextArea、Slider 各自管理自己的 `m_focused`，没有统一的基类 API 和焦点视觉反馈。本文档设计一套统一的焦点系统，涵盖：

1. **基类焦点 API** — `Control/ControlImpl` 新增统一的 `m_focused`、`setFocused()`、焦点生命周期回调
2. **焦点环绘制** — 自动在聚焦控件周围绘制 2px 蓝色环（Solid/Dashed 可配）
3. **FocusManager** — 集中管理 Tab 顺序和 Tab/Shift+Tab 导航
4. **各控件迁移** — EditBox/TextArea/Slider 迁移到基类焦点；Button/CheckBox 新增键盘激活支持
5. **焦点可见性策略** — 支持"始终显示"和"仅键盘 Tab 导航时显示"两种模式

### 1.1 设计目标

- 所有交互性控件（EditBox、TextArea、Slider、Button、CheckBox）默认支持焦点
- 纯展示控件（Label、ProgressBar、ScrollBar、Panel）默认不参与焦点
- 焦点环样式、颜色、可见性策略均可配置
- Tab 导航按 tabIndex 顺序自动遍历，支持递归进入子容器内的可聚焦控件
- 焦点作用域机制：WinFrame 激活时 Tab 限定在该 WinFrame 内部
- Button/CheckBox 新增键盘激活支持
- CheckBox 三态循环（Unchecked → Checked → Mixed）
- 向后兼容现有控件的焦点行为

## 2. 焦点环样式

### 2.1 样式枚举

```cpp
enum class FocusRingStyle {
    Solid,    // 实线 3px（三层叠加）
    Dashed    // 虚线 3px
};
```

- **Solid**：通过 3 层 `drawRect()` 叠加实现 ≈3px 边框
- **Dashed**：沿 4 条边按固定间距绘制短线段

### 2.2 绘制方法

```cpp
void ControlImpl::drawFocusRing() {
    if (!m_focused || !m_showFocusRing) return;
    if (!m_focusRingAlwaysVisible && !m_focusByKeyboard) return;
    if (!m_frameDrawRectValid) return;

    SRect dr = m_frameDrawRect;
    auto* rd = getRenderDevice();
    if (!rd) return;

    if (m_focusRingStyle == FocusRingStyle::Solid) {
        // 3-layer ring: black outer (visible on light bg),
        // white middle (visible on dark bg), color accent inner
        rd->setDrawColor(SColor(0, 0, 0, 150));
        rd->drawRect({dr.left, dr.top, dr.width, dr.height});
        rd->setDrawColor(SColor(255, 255, 255, 150));
        rd->drawRect({dr.left + 1, dr.top + 1, dr.width - 2, dr.height - 2});
        rd->setDrawColor(m_focusRingColor);
        rd->drawRect({dr.left + 2, dr.top + 2, dr.width - 4, dr.height - 4});
    } else {
        // Dashed: 3-pass, one per layer (black→white→color)
        for (int pass = 0; pass < 3; pass++) {
            float inset = (float)(pass + 1);
            SColor c;
            switch (pass) {
                case 0: c = SColor(0, 0, 0, 150); break;
                case 1: c = SColor(255, 255, 255, 150); break;
                default: c = m_focusRingColor; break;
            }
            drawDashedRect(dr, inset, c);
        }
    }
}
```

> **对比策略**：黑+白保证在任何背景色下至少有一条线可见。颜色层保留自定义色，作为最内层的装饰。`m_focusRingColor`（默认 (66,133,244)）仍可通过 `setFocusRingColor()` 设置。

### 2.3 可见性策略

| 策略 | `focusRingAlwaysVisible` | 鼠标点击聚焦 | Tab 导航聚焦 |
|------|:------------------------:|:------------:|:------------:|
| 始终显示（默认） | `true` | 显示环 | 显示环 |
| 仅键盘 Tab | `false` | **不**显示环 | 显示环 |

默认 `m_focusRingAlwaysVisible = true`，焦点环始终显示在聚焦控件上。如果希望环只在 Tab 键盘导航时出现（鼠标点击不显示），可调用 `setFocusRingAlwaysVisible(false)`。

## 3. Control/ControlImpl 焦点 API

### 3.1 新增成员变量

```cpp
// === ControlImpl 新增 ===

// 焦点状态
bool m_focused;                 // 是否聚焦，默认 false
bool m_focusable;               // 能否聚焦，默认 false
int  m_tabIndex;                // Tab 顺序，-1 = 不参与 Tab 导航
bool m_focusByKeyboard;         // true=通过 Tab 聚焦，false=通过鼠标

// 焦点环配置
bool             m_showFocusRing;           // 是否绘制焦点环，默认 true
bool             m_focusRingAlwaysVisible;  // 是否始终显示，默认 true
SColor           m_focusRingColor;          // 环颜色，默认 (66,133,244)
FocusRingStyle   m_focusRingStyle;          // Solid 或 Dashed
```

### 3.2 新增方法（Control 接口 + ControlImpl 实现）

```cpp
// === Control 接口新增 ===
virtual void setFocused(bool focused, bool byKeyboard = false);
virtual bool getFocused() const;
virtual bool isFocusable() const;
virtual int  getTabIndex() const;
virtual void setTabIndex(int index);
virtual void setFocusable(bool focusable);

// 焦点生命周期回调（子类可覆盖）
virtual void onFocusGained(bool byKeyboard);
virtual void onFocusLost();

// 焦点环配置
virtual void setShowFocusRing(bool show);
virtual void setFocusRingAlwaysVisible(bool always);
virtual void setFocusRingColor(SColor color);
virtual void setFocusRingStyle(FocusRingStyle style);
```

### 3.3 ControlImpl 实现逻辑

```cpp
void ControlImpl::setFocused(bool focused, bool byKeyboard) {
    if (m_focused == focused) return;
    m_focused = focused;
    m_focusByKeyboard = byKeyboard && focused;
    if (focused) onFocusGained(byKeyboard);
    else onFocusLost();
    // 同步 FocusManager 状态
    FocusManager* fm = MAINWIN ? MAINWIN->getFocusManager() : nullptr;
    if (fm) fm->notifyControlFocused(this, byKeyboard);
}

void ControlImpl::onFocusGained(bool byKeyboard) {
    // 基类默认空实现
    // 子类可以覆盖：EditBox 启始终闪烁，Slider 无操作
}

void ControlImpl::onFocusLost() {
    // 基类默认空实现
    // 子类可以覆盖：EditBox 停止光标闪烁，Slider 停止键盘重复
}
```

### 3.4 绘制流水线修改

`afterDraw()` 在控件绘制完内容后调用，绘制边框和焦点环：

```cpp
void ControlImpl::afterDraw() {
    drawBorder(&m_frameDrawRect);
    drawFocusRing();     // 放在边框之后，3层叠加（黑+白+颜色）
    m_frameDrawRectValid = false;
}
```

`drawFocusRing()` 内部使用 `m_frameDrawRect`（由 `beforeDraw()` 设置的当前帧绘制矩形），并通过 `m_frameDrawRectValid` 守卫确保仅在当前帧有效时才绘制。Solid 模式下以 `inset 0/1/2` 绘制 3 层 `drawRect()`，Dashed 模式下以 3 次循环绘制虚线。

## 4. FocusManager

### 4.1 类设计

```cpp
class FocusManager {
public:
    FocusManager();
    ~FocusManager();

    // 注册/注销可聚焦控件
    void registerControl(Control* ctl);
    void unregisterControl(Control* ctl);

    // Tab 导航（走查焦点作用域内的下一个/上一个控件）
    bool focusNext(Control* current);   // Tab
    bool focusPrev(Control* current);   // Shift+Tab

    // 由 ControlImpl::setFocused 调用，同步 FocusManager 状态
    // 注意：不会再次调用 setFocused（避免递归），仅更新 m_currentFocused
    void notifyControlFocused(Control* ctl, bool byKeyboard);

    // 查询
    Control* getCurrentFocused() const { return m_currentFocused; }

private:
    vector<Control*> m_controls;   // 全树所有可聚焦控件（flat list）
    Control* m_currentFocused = nullptr;
};
```

### 4.2 注册机制

FocusManager 不存储树结构，只维护一个 flat list。焦点作用域和树遍历在 Tab 导航时实时计算。

**注册时机**（控件主动调用 `FocusManager::registerControl(this)`）：

| 操作 | 触发 |
|------|:----|
| 控件构造时 `m_focusable == true && m_tabIndex >= 0` | 自动注册 |
| `setFocusable(true)` | 注册 |
| `setTabIndex(n)`（n≥0） | 注册 |
| 控件析构 | `FocusManager::unregisterControl(this)` |

> 新增子控件时，若其 focusable，则子控件在构造中自行注册，无需父控件递归注册。FocusManager 不感知树结构——树结构在导航时通过父指针链实时查询。

### 4.3 焦点作用域（Focus Scope）

**问题**：WinFrame 弹出时，Tab 不应跳到 WinFrame 外部的控件；Bench 的直系子控件也不应跳到 WinFrame 内部。

**解决方案**：引入焦点作用域边界。

```cpp
// ControlImpl 新增
bool m_isFocusBoundary;  // 默认 false；WinFrame、Bench 设为 true
```

- **作用域边界容器**的 Tab 导航限制在其 **直接后代** 内（即子控件的子控件…，但不跨越另一个作用域边界）
- **Bench**：`m_isFocusBoundary = true`，作用域为所有不在任何 WinFrame 内的控件 + 最顶层 WinFrame 内的控件（详见下方作用域穿透规则）
- **WinFrame**：`m_isFocusBoundary = true`，作用域仅限该 WinFrame 内的控件
- **ColorPicker**：`m_isFocusBoundary = true`，弹出色板时 Tab 限定在 ColorPicker 内部

**作用域穿透规则**：Tab 导航不穿越作用域边界。当前聚焦控件的作用域定义为**向上追溯最近的一个 `m_isFocusBoundary == true` 的祖先**（若没有，则作用域为 Bench）。

```
Bench (scope boundary)                ← 根作用域
├── Label（不可聚焦，忽略）
├── Button1                ← Tab: Button1 → ButtonX（同作用域）
│                            Ctrl+Tab → 跳至 WinFrameA
├── WinFrameA (boundary)   ← Tab: EditBoxA ↔ ButtonA（WinFrameA 内部）
│   ├── EditBoxA             Ctrl+Tab → 跳至 WinFrameB
│   └── ButtonA              Ctrl+Shift+Tab → 跳至 Bench
└── WinFrameB (boundary)   ← Tab: SliderB（WinFrameB 内部）
    └── SliderB              Ctrl+Tab → 跳至 Bench
```

**Tab 导航规则**：
- 若当前在 Button1：作用域 = Bench → next = 同一作用域的下一个（跳过 WinFrameA/B 内部）
- 若当前在 EditBoxA：作用域 = WinFrameA → next = WinFrameA 内部的下一个
- 若 WinFrameA 关闭或隐藏：其内部控件自动从 focusNext 的结果中过滤（见 4.4）

**WinFrame 显示/隐藏的影响**：
- WinFrame 显示在最前面时：其内部的控件可参与 Tab 导航，**但限定在该 WinFrame 内部**
- WinFrame 隐藏/关闭时：其内部的控件在 `focusNext()`/`focusPrev()` 中跳过
- 在 WinFrame 外部按 Tab：只遍历**不在任何 WinFrame 内**的控件，**不进入**任何 WinFrame
- 在 WinFrame 内部按 Tab：只遍历**该 WinFrame 内**的控件，**不跳出**到外部

若要进入一个 WinFrame 进行操作，默认必须通过鼠标点击 WinFrame 或其内部控件，或通过 **Ctrl+Tab** 跨作用域导航（见 4.5）。

### 4.4 Tab 导航实现

```cpp
bool FocusManager::focusNext(Control* current) {
    // 1. 确定当前的作用域
    Control* scope = current ? findFocusScope(current) : nullptr;

    // 2. 从 current 开始，在 m_controls 中找下一个同作用域的控件
    auto it = current ? std::find(m_controls.begin(), m_controls.end(), current)
                      : m_controls.end();
    if (it != m_controls.end()) ++it;

    Control* candidate = nullptr;
    for (int i = 0; i < (int)m_controls.size(); i++) {
        if (it == m_controls.end()) it = m_controls.begin();  // 循环

        Control* c = *it;
        if (isInScope(c, scope) && c->isVisible() && c->isEnabled()) {
            candidate = c;
            break;
        }
        ++it;
    }

    // 3. 切换焦点
    if (candidate && candidate != current) {
        if (current) current->setFocused(false, false);
        candidate->setFocused(true, true);
        m_currentFocused = candidate;
        return true;
    }
    return false;  // 作用域内没有其他可选控件
}
```

辅助函数：

```cpp
// 向上追溯到最近的作用域边界
Control* findFocusScope(Control* ctl) {
    while (ctl) {
        if (ctl->isFocusBoundary()) return ctl;
        ctl = ctl->getParent();
    }
    return nullptr;  // 到根了，作用域为 Bench
}

// 判断 candidate 是否在 scope 的作用域内
bool isInScope(Control* candidate, Control* scope) {
    if (!scope) return true;  // 无限制
    Control* cs = findFocusScope(candidate);
    return cs == scope;       // 必须在同一个作用域边界内
}
```

> **visible/enabled 过滤**：Tab 导航跳过 `!isVisible()` 或 `!isEnabled()` 的控件，确保隐藏/禁用的控件不可通过 Tab 到达。

### 4.5 跨作用域导航（Ctrl+Tab）

**问题**：当两个 WinFrame 同时可见时，Tab 被限制在各自作用域内，用户无法通过键盘从一个 WinFrame 切换到另一个。

**解决方案**：Ctrl+Tab / Ctrl+Shift+Tab 在焦点作用域边界（WinFrame）之间切换。

```cpp
class FocusManager {
public:
    // ... 已有方法 ...

    // 跨作用域导航
    bool focusNextScope();     // Ctrl+Tab
    bool focusPrevScope();     // Ctrl+Shift+Tab

    // 在当前作用域内找第一个可聚焦控件
    bool focusFirstInScope(Control* scope);

private:
    vector<Control*> m_controls;    // 全树所有可聚焦控件
    vector<Control*> m_boundaries;  // 所有作用域边界容器（Bench + 各 WinFrame）
    Control* m_currentFocused = nullptr;
};
```

**跨作用域导航流程**：

```
用户按 Ctrl+Tab
  → Bench::handleEvent
    → event->keyEvent.keycode == KeyCode::Tab && (Ctrl held)
      → GET_FOCUSMANAGER->focusNextScope()

focusNextScope():
  1. 若 m_currentFocused == nullptr：聚焦第一个作用域内的第一个控件
  2. 否则：确定当前作用域 boundary
  3. 在 m_boundaries 中找下一个可见的作用域边界（跳过隐藏的 WinFrame）
  4. 找到的边界内找第一个可见且启用的可聚焦控件 → setFocused(true, true)
  5. 若未找到（作用域内无可用控件），继续找下一个边界（循环）

focusFirstInScope(scope):
  // 在 scope 的后代中找第一个 focusable 的控件，用于 WinFrame 激活时
  for each c in m_controls:
    if isDescendantOf(scope, c) && c->isVisible() && c->isEnabled():
      c->setFocused(true, true)
      return true
  return false（作用域内无控件）
```

**边界注册**：当控件的 `m_isFocusBoundary` 设为 `true` 时，自动向 FocusManager 注册为边界。

```
Bench     → 注册到 m_boundaries（排序第 0，始终存在）
WinFrameA → 注册到 m_boundaries（按显示顺序排序）
WinFrameB → 注册到 m_boundaries
```

> **Scope→Scope 转换行为**：
> - Ctrl+Tab 在 WinFrame A 内按下：跳到 WinFrame B 的第一个控件（如果 B 可见且有 focusable 控件）
> - Ctrl+Tab 在 Bench 直系控件按下：跳到第一个可见 WinFrame 的第一个控件
> - Ctrl+Shift+Tab 反向遍历
> - 循环：`... → WinFrameA → WinFrameB → Bench → WinFrameA → ...`

### 4.6 初次 Tab 和显式聚焦

- **初次按 Tab**（`m_currentFocused == nullptr`）：从 `m_controls` 中找到第一个可见且启用的控件，作用域为 `findFocusScope(candidate)`
- **鼠标点击聚焦**：鼠标点击某个 control 时，`setFocused(true, false)` → `m_currentFocused = ctl`。此时 `m_focusByKeyboard == false`，焦点环不显示（如果 `focusRingAlwaysVisible == false`）
- **键盘 Tab 聚焦**：`setFocused(true, true)` → `m_focusByKeyboard == true`，焦点环显示

### 4.7 notifyControlFocused 同步机制

`ControlImpl::setFocused()` 在更新控件自身焦点状态后，末尾调用 `FocusManager::notifyControlFocused()` 以确保管理系统状态一致：

```cpp
void ControlImpl::setFocused(bool focused, bool byKeyboard) {
    // ... 更新 m_focused, 调用 onFocusGained/onFocusLost ...
    FocusManager* fm = MAINWIN ? MAINWIN->getFocusManager() : nullptr;
    if (fm) fm->notifyControlFocused(this, byKeyboard);
}
```

**要点**：
1. **调用时机**：`setFocused()` 末尾，所有焦点回调完成后
2. **不递归**：`notifyControlFocused` **不会**再次调用 `setFocused`，仅更新 `m_currentFocused` 成员，避免递归死循环
3. **作用**：确保鼠标点击聚焦后，`FocusManager::getCurrentFocused()` 返回正确的当前聚焦控件

### 4.8 MainWindow 持有

```cpp
// include/MainWindow.h
#define GET_FOCUSMANAGER (MAINWIN->getFocusManager())

class MainWindow {
    // 新增
    FocusManager* getFocusManager() { return &m_focusManager; }
private:
    FocusManager m_focusManager;
};
```

## 5. 各控件迁移方案

### 5.1 EditBox

**当前**：自有 `m_focused`、`setFocused()`、`beforeEventHandlingWatcher`（ON_FOCUS Custom 事件）。
**迁移**：

```cpp
// 移除
bool m_focused;  // → 改调用基类 setFocused()

// 保留
int32_t m_cursorBlinkTime;
bool m_cursorVisible;
bool m_focusWatcherRegistered;

// 新增覆盖
void onFocusGained(bool byKeyboard) override {
    m_cursorVisible = true;
    m_cursorBlinkTime = 0;
    // 注：cursor 始终显示（即使环仅 Tab 显示）
}

void onFocusLost() override {
    m_cursorVisible = false;
    clearSelection();
}

// 修改 setFocused()
void EditBox::setFocused(bool focused, bool byKeyboard) {
    if (m_focused == focused) return;  // 基类 m_focused
    
    if (focused && !m_focusWatcherRegistered) {
        // 注册 ON_FOCUS 观察器（仅一次）
    }
    if (focused) {
        // 推送 ON_FOCUS 事件通知其他 EditBox
    }
    
    ControlImpl::setFocused(focused, byKeyboard);
}
```

焦点失去检测：保留 `beforeEventHandlingWatcher`，当检测到其他 EditBox 获得焦点时，调用 `setFocused(false)`。

### 5.2 TextArea

继承 EditBox，无需额外修改。

### 5.3 Slider

**当前**：自有 `m_focused`、`setFocused()`、`beforeEventHandlingWatcher`（MouseDown 观察器）。
**迁移**：

```cpp
// 移除
bool m_focused;
bool m_focusWatcherRegistered;

// 修改 setFocused() → 基类版本
void Slider::setFocused(bool focused, bool byKeyboard) {
    if (m_focused == focused) return;
    
    if (focused && !m_focusWatcherRegistered) {
        // 注册 MouseDown 观察器
    }
    
    ControlImpl::setFocused(focused, byKeyboard);
}

void onFocusLost() override {
    m_repeatKey = 0;  // 停止键盘重复
}
```

### 5.4 Button

**当前**：无焦点概念，通过 MouseDown/MouseUp 触发 onClick。
**已实现**：

```cpp
// 构造函数（已实现）
Button::Button(...) : ControlImpl(...) {
    setFocusable(true);   // 注册到 FocusManager，参与 Tab 导航
}

// handleEvent 键盘处理
if (event->m_type == EventType::KeyDown && m_focused) {
    if (event->keyEvent.keycode == KeyCode::Return ||
        event->keyEvent.keycode == KeyCode::Space) {
        setState(ControlState::Pressed);
        if (m_onClick) {
            m_onClick(getThis());
        }
        return true;
    }
}

// KeyUp 恢复状态
if (event->m_type == EventType::KeyUp && m_focused) {
    if (event->keyEvent.keycode == KeyCode::Return ||
        event->keyEvent.keycode == KeyCode::Space) {
        setState(ControlState::Normal);
        return true;
    }
}
```

> 使用 `setFocusable(true)` 而非直接赋值 `m_focusable = true`，确保 `FocusManager::registerControl()` 被调用。键盘激活时先设 `Pressed` 状态（触发视觉反馈）再调用 `onClick`。

### 5.5 CheckBox

**当前**：无焦点概念，通过 MouseDown/MouseUp 切换选中状态。CheckBox 支持三态：`Unchecked → Checked → Mixed → Unchecked → ...`

**已实现**：

```cpp
// 构造函数（已实现）
CheckBox::CheckBox(...) : ControlImpl(...) {
    setFocusable(true);   // 注册到 FocusManager，参与 Tab 导航
}

// handleEvent 键盘处理
if (event->m_type == EventType::KeyDown && m_focused) {
    if (event->keyEvent.keycode == KeyCode::Space) {
        // 循环三态：Unchecked → Checked → Indeterminate → Unchecked
        int nextState = (static_cast<int>(m_checkState) + 1) % 3;
        setCheckState(static_cast<CheckState>(nextState));
        if (m_onCheckChanged) {
            m_onCheckChanged(shared_from_this(), oldState, m_checkState);
        }
        return true;
    }
}
```

> Space 键的三态循环与鼠标点击一致（`+1 % 3`），确保键盘和鼠标用户体验相同。`setFocusable(true)` 而非直接 `m_focusable = true` 确保 FocusManager 注册。

### 5.6 WinFrame

**当前**：`m_isFocusBoundary = false`（默认），Tab 会无限制地进入 WinFrame 内部。
**迁移**：

```cpp
// WinFrame 构造
m_isFocusBoundary = true;

// 当用户点击 WinFrame 标题栏使其显示在最前面时，焦点进入该 WinFrame 内部
void WinFrame::onActivate() {
    GET_FOCUSMANAGER->focusFirstInScope(this);
}

// 当 WinFrame 隐藏时，释放其内部控件的焦点
void WinFrame::setVisible(bool visible) {
    ControlImpl::setVisible(visible);
    if (!visible) {
        Control* fc = GET_FOCUSMANAGER->getCurrentFocused();
        if (fc && isDescendantOf(this, fc)) {
            fc->setFocused(false, false);
        }
    }
}
```

> **跨作用域键盘切换**：Ctrl+Tab / Ctrl+Shift+Tab 在 Bench 和各个可见 WinFrame 之间轮转焦点，无需鼠标。

## 6. Bench Tab 键拦截

Bench 的 `handleEvent()` 中新增 Tab 键处理：

```cpp
bool Bench::handleEvent(shared_ptr<Event> event) {
    // 拦截 Tab / Ctrl+Tab（在传递给子控件之前）
    if (event->m_type == EventType::KeyDown &&
        event->keyEvent.keycode == KeyCode::Tab) {

        bool ctrl = (event->keyEvent.mod & (KeyMod::LCtrl | KeyMod::RCtrl)) != 0;
        bool shift = (event->keyEvent.mod & KeyMod::Shift) != 0;

        if (ctrl) {
            // 跨作用域导航（WinFrame 切换）
            if (shift)
                GET_FOCUSMANAGER->focusPrevScope();
            else
                GET_FOCUSMANAGER->focusNextScope();
        } else {
            // 作用域内导航
            Control* current = GET_FOCUSMANAGER->getCurrentFocused();
            if (shift)
                GET_FOCUSMANAGER->focusPrev(current);
            else
                GET_FOCUSMANAGER->focusNext(current);
        }
        return true;  // 消费事件
    }
    // ... 原有事件处理
}
```

## 7. 默认配置总表

| 控件 | `focusable` | `tabIndex` | `isFocusBoundary` | `showFocusRing` | `focusRingAlwaysVisible` | 键盘激活 |
|------|:-----------:|:----------:|:-----------------:|:---------------:|:------------------------:|:--------:|
| EditBox | `true` | 自动分配 | `false` | `true` | **`true`** | 文本输入/快捷键 |
| TextArea | `true` | 自动分配 | `false` | `true` | **`true`** | 继承 EditBox |
| Slider | `true` | 自动分配 | `false` | `true` | **`true`** | 方向键/PgUp/Home/End |
| Button | `true` | 自动分配 | `false` | `true` | **`true`** | Enter/Space → onClick |
| CheckBox | `true` | 自动分配 | `false` | `true` | **`true`** | Space → 三态循环 |
| Label | `false` | -1 | `false` | `false` | `false` | — |
| ProgressBar | `false` | -1 | `false` | `false` | `false` | — |
| Panel | `false` | -1 | `false` | `false` | `false` | — |
| ColorPicker | `false` | -1 | **`true`** | `false` | `false` | — |
| WinFrame | `false` | -1 | **`true`** | `false` | `false` | — |
| Bench | `false` | -1 | **`true`** | `false` | `false` | — |

## 8. 文件变更清单

### 8.1 初始实现变更

| 操作 | 文件 | 说明 |
|------|------|------|
| 新增 | `include/FocusManager.h` | FocusManager 类声明 |
| 新增 | `src/FocusManager.cpp` | 实现 |
| 修改 | `include/ControlBase.h` | Control 接口 + ControlImpl 新增焦点成员/方法（含 `m_isFocusBoundary`） |
| 修改 | `src/ControlBase.cpp` | 实现焦点方法；`afterDraw()` 调用 `drawFocusRing()`；`findFocusScope()`/`isInScope()` |
| 修改 | `include/WinFrame.h` | `m_isFocusBoundary = true` + `registerBoundary()` |
| 修改 | `src/WinFrame.cpp` | WinFrame 显示/隐藏时更新作用域状态；`hide()` 释放焦点 |
| 修改 | `include/EditBox.h` | 移除自有 `m_focused`，构造函数调用 `setFocusable(true)` |
| 修改 | `src/EditBox.cpp` | `setFocused()` 改调基类；覆盖 `onFocusGained()`/`onFocusLost()` |
| 修改 | `include/Slider.h` | 移除自有 `m_focused`/`m_focusWatcherRegistered` |
| 修改 | `src/Slider.cpp` | `setFocused()` 改调基类；覆盖 `onFocusLost()` 清 `m_repeatKey`；`draw()` 调用 `ControlImpl::afterDraw()` |
| 修改 | `include/Button.h` | 构造函数调用 `setFocusable(true)` |
| 修改 | `src/Button.cpp` | `handleEvent()` 新增 `KeyDown(Enter/Space)` → onClick |
| 修改 | `include/CheckBox.h` | 构造函数调用 `setFocusable(true)` |
| 修改 | `src/CheckBox.cpp` | `handleEvent()` 新增 `KeyDown(Space)` → 三态循环 |
| 修改 | `include/Bench.h` | `m_isFocusBoundary = true` |
| 修改 | `src/Bench.cpp` | `handleEvent()` 拦截 Tab → FocusManager |
| 修改 | `include/MainWindow.h` | `GET_FOCUSMANAGER` 宏；`m_focusManager` 成员 |
| 修改 | `CMakeLists.txt` | 添加 `FocusManager.cpp` |

### 8.2 焦点环 3 层对比优化（当前 session）

| 操作 | 文件 | 说明 |
|------|------|------|
| 修改 | `src/ControlBase.cpp` | `drawFocusRing()` 改为 3 层：黑(外, inset 0) + 白(中, inset 1) + 颜色(内, inset 2)，保证任何背景可见 |
| 修改 | `include/ControlBase.h` | `m_focusRingAlwaysVisible` 默认值改为 `true` |
| 修改 | `src/backend/sfml/InputBackend.cpp` | 添加 `unicode < 0x20 || unicode == 0x7F` 控制字符过滤，避免 Tab 注入为文本输入 |
| 修改 | `test/test_button.cpp` | 图片路径从硬编码绝对路径改为相对路径（`assets/images/*.png`） |
| 修改 | `test/test_winframe.cpp` | 添加 2 Button + 1 EditBox 在 WinFrame 1、1 Button + 1 EditBox 在 WinFrame 2 用于焦点测试 |

## 9. 实现状态

所有 10 个步骤均已实现并在 SDL3/SFML/Raylib 三后端编译通过。关键验证点：

| 验证项 | 状态 |
|--------|:----:|
| 焦点环 3 层对比（黑+白+颜色） | ✅ 任何背景可见 |
| Tab 作用域内导航 | ✅ |
| Ctrl+Tab 跨作用域导航 | ✅ |
| Button Enter/Space 键盘激活 | ✅ |
| CheckBox Space 三态循环 | ✅ |
| WinFrame 焦点边界 | ✅ |
| EditBox/Slider 基类焦点迁移 | ✅ |
| SFML TextInput 控制字符过滤 | ✅ |

## 10. 后续优化

- 焦点环动画效果（呼吸/渐入）
- 自定义焦点环图案（非矩形轮廓）
- EditBox/TextArea 在 Tab 导航时自动选中文本
- 焦点调试可视化（显示当前 focus scope）
