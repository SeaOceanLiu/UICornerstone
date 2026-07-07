# WinFrame 控件设计文档

## 1. 概述

WinFrame 是一个可拖拽、可调整大小的顶层容器控件，替代原来的 Dialog。它提供一个标题栏（带关闭按钮）和一个客户区域面板。用户可将任意控件添加到客户区域中。

### 1.1 结构示意

```
┌─────────────────────────────────────┐
│  Title Bar               [×]       │  ← TitleBar (Panel, drag handler)
├─────────────────────────────────────┤
│                                     │
│  Client Panel                       │  ← ClientPanel (Panel, 内容区域)
│  (用户在此添加控件)                   │
│                                     │
│                                     │
└─────────────────────────────────────┘
  ↑ 边缘 4px resize handle
```

### 1.2 与 Dialog 的差异


| 特性               | Dialog        | WinFrame              |
| ------------------ | ------------- | --------------------- |
| 标题栏拖拽         | 不支持        | 支持                  |
| 窗口边缘调整大小   | 不支持        | 支持（4px 热区）      |
| 鼠标光标反馈       | 无            | 边缘双箭头光标        |
| OK 按钮 / TextArea | 内置          | 移除（用户自行添加）  |
| 客户区域 Panel     | 无显式分离    | 有公共`m_clientPanel` |
| 标题栏 / 关闭按钮  | 内部封装      | Getter 暴露，可定制细节  |
| Builder 模式       | DialogBuilder | WinFrameBuilder       |
| JSON 布局支持      | 支持          | 支持                  |

## 2. 类设计

### 2.1 类层次

```
Control
  └── ControlImpl
        └── Panel
              └── WinFrame  (新增, 同时继承 enable_shared_from_this<WinFrame>)
```

**颜色类型**：WinFrame 所有公开 API 使用 `SColor`，以实现图形库无关。内部调用底层 `Panel`/`ControlBase` 方法时直接传递 `SColor`（`ControlBase` 已统一使用 `SColor`）。

### 2.2 新增成员变量

```cpp
// ===== 子控件（private，通过 Getter 暴露）=====
private:
    shared_ptr<Panel>  m_titleBar;       // 标题栏面板
    shared_ptr<Label>  m_titleLabel;     // 标题栏中的 Label
    shared_ptr<Button> m_closeButton;    // 关闭按钮
    shared_ptr<Panel>  m_clientPanel;    // 客户区域面板

// ===== 拖拽/缩放状态 =====
    bool    m_dragging;                  // 是否正在拖拽
    SPoint  m_dragOffset;                // 鼠标点击位置相对于 WinFrame 左上角的偏移

    bool    m_resizing;                  // 是否正在调整大小
    uint8_t m_resizeFlags;               // 位标志：kLeft/kRight/kTop/kBottom
    SRect   m_startScreenRect;          // 缩放开始时的屏幕坐标矩形
    SRect   m_startLocalRect;           // 缩放开始时的 parent-local 矩形
    SPoint  m_resizeStartMouse;         // 缩放开始时的鼠标位置（屏幕坐标）

    float   m_edgeMargin;                // 边缘热区宽度（默认为 4.0f）
    bool    m_resizable;                 // 是否允许用户调整大小（默认为 true）
    uint8_t m_lastEdgeFlags;             // 上次边缘检测缓存（用于子控件覆盖后恢复光标）

    // 光标对象（每个实例独立拥有，避免 static 缓存的跨后端问题）
    Cursor *m_cursorDefault;             // 默认箭头光标
    Cursor *m_cursorSizeWE;              // 水平双箭头光标
    Cursor *m_cursorSizeNS;              // 垂直双箭头光标
    Cursor *m_cursorSizeNWSE;            // 左上-右下双箭头光标
    Cursor *m_cursorSizeNESW;            // 右上-左下双箭头光标

// ===== 焦点作用域 =====
    // m_isFocusBoundary = true 继承自基类 ControlImpl
    // 构造函数中调用 GET_FOCUSMANAGER->registerBoundary(this)
```

**光标设计说明**：

- 使用 `Cursor*` 替代 `SDL_Cursor*`，通过 `Cursor::createSystem(SystemCursorType)` 工厂方法创建
- 每个 WinFrame 实例拥有独立的光标对象，在构造函数中通过 `Cursor::getDefault()` 和 `Cursor::createSystem()` 初始化
- 在析构函数中通过 `delete` 释放（`m_cursorDefault` 为 `getDefault()` 返回的单例引用，不释放）
- 避免使用 static 数组缓存，因为不同后端的光标实现不同，缓存会导致跨实例状态污染

### 2.3 常量

```cpp
static constexpr float MIN_WIDTH   = 100.0f;    // 最小宽度
static constexpr float MIN_HEIGHT  = 60.0f;     // 最小高度
```

### 2.4 新增接口

```cpp
// ===== 子控件访问（通过 Getter 获取，再调用其 API 进行定制）=====
shared_ptr<Panel>  getTitleBar();
shared_ptr<Label>  getTitleLabel();
shared_ptr<Button> getCloseButton();
shared_ptr<Panel>  getClientPanel();

// ===== 快捷颜色设置（使用 SColor）=====
void setWinFrameBGColor(const SColor& color);     // 设置 WinFrame 整体背景色
void setWinFrameBorderColor(const SColor& color); // 设置边框颜色
void setTitleBarBGColor(const SColor& color);      // 设置标题栏背景色
void setTitleTextColor(const SColor& color);       // 设置标题文字颜色

// ===== show 时置顶 =====
void show() override;  // 调用 Panel::show() 后，将自身从父容器移除并重新添加，确保在最前端

// ===== 边缘热区 =====
void  setEdgeMargin(float margin);
float getEdgeMargin() const;

// ===== 缩放控制 =====
void  setResizable(bool resizable);
bool  isResizable() const;

// ===== 便捷方法 =====
void setTitle(const string& title);
string getTitle() const;
void addToClient(shared_ptr<Control> control);  // 委托给 m_clientPanel->addControl()
```

### 2.5 resizeFlags 位标志

```cpp
static constexpr uint8_t kLeft   = 0x01;
static constexpr uint8_t kRight  = 0x02;
static constexpr uint8_t kTop    = 0x04;
static constexpr uint8_t kBottom = 0x08;
```

## 3. 界面布局（构造函数内创建）

### 3.1 标题栏（TitleBar）

```
addControl(m_titleBar = PanelBuilder(this, {0, 0, selfWidth, WINDOW_TITLE_HEIGHT})
    .setBGColor(SColor(173, 216, 230, 255))    // 浅蓝
    .setBorderVisible(false)
    .build());

m_titleBar->addControl(m_titleLabel = LabelBuilder(m_titleBar.get(), {0, 0, selfWidth, WINDOW_TITLE_HEIGHT})
    .setFontSize(WINDOW_TITLE_HEIGHT - 4)
    .setAlignmentMode(AM_CENTER)
    .setFont(FontName::HarmonyOS_Sans_SC_Regular)
    .setCaption(m_title)
    .build());
```

- `m_titleBar`（Panel）：标题栏容器，用户可修改其背景色、边框等
- `m_titleLabel`（Label）：标题文字，用户可修改字体、字号、颜色、对齐方式等
- 标题栏自身的事件处理：鼠标按下时检查是否在关闭按钮区域内；若不在则触发拖拽（见 4.2）

### 3.2 关闭按钮（CloseButton）

```
addControl(m_closeButton = ButtonBuilder(this, {selfWidth - WINDOW_TITLE_HEIGHT, 0, WINDOW_TITLE_HEIGHT, WINDOW_TITLE_HEIGHT})
    .setNormalStateActor(    make_shared<Actor>(this, fs::path("assets/images/cross_up.png"), true))
    .setHoverStateActor(     make_shared<Actor>(this, fs::path("assets/images/cross_over.png"), true))
    .setPressedStateActor(   make_shared<Actor>(this, fs::path("assets/images/cross_down.png"), true))
    .setBackgroundStateColor(StateColor(
        SColor(0x50,0x50,0x50,0xFF),
        SColor(0x60,0x60,0x60,0xFF),
        SColor(0x40,0x40,0x40,0xFF),
        SColor(0x50,0x50,0x50,0xFF)))
    .setOnClick([this](shared_ptr<Button>) { hide(); })
    .setTransparent(false)
    .build());
```

- `m_closeButton`（Button）：关闭按钮，用户可修改其图标、颜色、点击回调等
- 图片路径使用直接文件系统字符串常量（如 `"assets/images/cross_up.png"`），而非 `ResourceLoader::RID_*` 资源标识符
- **注意**：关闭按钮通过 `addControl` 直接添加到 WinFrame，而不是添加到 TitleBar

### 3.3 客户区域（ClientPanel）

```cpp
m_clientRect = {0, WINDOW_TITLE_HEIGHT, selfWidth, selfHeight - WINDOW_TITLE_HEIGHT};
addControl(m_clientPanel = PanelBuilder(this, m_clientRect)
    .setBGColor(SColor(48, 48, 48, 255))  // 深灰
    .setTransparent(false)
    .setBorderVisible(false)
    .build());
```

- 用户通过 `getClientPanel()` 获取 Panel 指针，调用 `addControl()` 添加自己的控件

### 3.4 置顶逻辑

WinFrame 在以下时机自动置顶：

1. **调用 `show()` 时**
2. **获得焦点时**（用户点击 WinFrame 任意区域——标题栏、关闭按钮、客户区或子控件）

```cpp
// 封装的置顶方法
void WinFrame::bringToFront() {
    Control* p = getParent();
    if (!p) return;
    auto* parentImpl = dynamic_cast<ControlImpl*>(p);
    if (!parentImpl) return;
    auto& children = parentImpl->getChildren();
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == this) {
            auto self = *it;
            children.erase(it);
            children.push_back(self);
            break;
        }
    }
}

void WinFrame::show() {
    Panel::show();
    bringToFront();
}
```

**焦点置顶**：在 `handleEvent` 的最顶部（分发到子控件之前）检查 `MouseDown + LeftButton`，若命中则置顶：

```cpp
if (hasPos && event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
    if (getDrawRect().contains(mousePos.x, mousePos.y)) {
        bringToFront();
        consumedByFocus = true;
    }
}
```

**原理**：`ControlImpl::handleEvent` 逆向遍历 `m_children`，最后添加的子控件最优先处理事件，视觉上也最后绘制（位于顶层）。通过将自身移到 children 列表末尾实现"置顶"效果。

### 3.5 焦点作用域边界

WinFrame 是焦点作用域边界（`m_isFocusBoundary = true`），限制 Tab 导航在其内部：

**构造函数**：
```cpp
WinFrame::WinFrame(...) : Panel(...) {
    m_isFocusBoundary = true;  // 注册为焦点作用域边界
    GET_FOCUSMANAGER->registerBoundary(this);
}
```

**置顶时聚焦**（在 `handleEvent` 的 `bringToFront()` 之后）：
```cpp
// 点击 WinFrame 时使其作用域内的第一个控件获得焦点
GET_FOCUSMANAGER->focusFirstInScope(this);
```

**隐藏时释放焦点**（`hide()` 中）：
```cpp
void WinFrame::hide() {
    ControlImpl::hide();
    // 如果当前聚焦控件在 WinFrame 内部，释放焦点
    Control* fc = GET_FOCUSMANAGER->getCurrentFocused();
    if (fc && isDescendantOf(this, fc)) {
        fc->setFocused(false, false);
    }
    GET_FOCUSMANAGER->unregisterBoundary(this);
}
```

**Tab 行为**：
- 聚焦在 WinFrame 内部控件时按 Tab：只在 WinFrame 内部控件之间循环
- 聚焦在 WinFrame 外部时按 Tab：不进入任何 WinFrame
- Ctrl+Tab / Ctrl+Shift+Tab：在 Bench 和所有可见 WinFrame 之间切换作用域

## 4. 事件处理

### 4.1 处理优先级

```
WinFrame::handleEvent(event)
  │
  ├─ 0. 焦点置顶（最先执行）
  │      └─ MouseDown + LeftButton + 点在 WinFrame 内 → bringToFront()
  │
  ├─ 1. 拖拽/缩放进行中的拦截
  │      └─ MouseMove / MouseUp + LeftButton → 优先处理
  │      └─ MouseUp 结束缩放 → 若鼠标仍在边缘则保持缩放光标
  │
  ├─ 2. 边缘检测
  │      └─ MouseDown + LeftButton + 在边缘上 → 开始缩放
  │      └─ MouseMove + 在边缘上 → 切换光标
  │
  ├─ 3. 子控件处理（ControlImpl::handleEvent）
  │      └─ 关闭按钮：点击 → hide()
  │      └─ 客户区域：传递给子控件
  │
  ├─ 3b. 重新应用边缘光标（子控件可能覆盖）
  │
  ├─ 3c. 鼠标离开 WinFrame 边界 → 恢复默认光标
  │
  └─ 4. 标题栏拖拽（子控件未处理时）
         └─ MouseDown + LeftButton + 在标题栏上 → 开始拖拽
```

### 4.2 拖拽实现

#### 4.2.1 缩放对拖拽的影响分析

拖拽只修改 `m_rect.left` 和 `m_rect.top`。从绘制公式可知：

```
drawRect.left = m_rect.left × parentScaleXX + parentDrawRect.left
drawRect.top  = m_rect.top  × parentScaleYY + parentDrawRect.top
```

**关键观察**：`m_rect.left/top` 的缩放因子仅为 `parentScale`（父容器缩放），**不受 WinFrame 自身的 `xScale/yScale` 影响**。因此拖拽逻辑在任何 `xScale/yScale` 下都是一致的——只需正确完成屏幕坐标 ↔ parent-local 坐标的转换即可。

#### 4.2.2 拖拽流程

```cpp
// 开始拖拽（在 titleBar 区域按下鼠标左键，且子控件未消耗该事件）
MouseDown + LeftButton:
{
    // 步骤 1：屏幕坐标 → 父容器局部坐标
    Control* parent = getParent();
    SRect parentDraw = parent->getDrawRect();
    float parentX = (mousePos.x - parentDraw.left) / parent->getScaleXX();
    float parentY = (mousePos.y - parentDraw.top)  / parent->getScaleYY();

    // 步骤 2：计算偏移（父容器局部空间）
    m_dragging = true;
    m_dragOffset = { parentX - m_rect.left, parentY - m_rect.top };
}

// 拖拽中
MouseMove && m_dragging:
{
    Control* parent = getParent();
    SRect parentDraw = parent->getDrawRect();
    float parentX = (mousePos.x - parentDraw.left) / parent->getScaleXX();
    float parentY = (mousePos.y - parentDraw.top)  / parent->getScaleYY();

    float newLeft = parentX - m_dragOffset.x;
    float newTop  = parentY - m_dragOffset.y;
    setRect({newLeft, newTop, m_rect.width, m_rect.height});
}

// 结束拖拽
MouseUp + LeftButton && m_dragging:
    m_dragging = false;
```

**验证**（`S=2`, `P=1`, 鼠标向右拖动标题栏 10px 屏幕坐标）：

```
parentX_start = (screenX_start - parentDraw.left) / 1
parentX_move  = (screenX_start + 10 - parentDraw.left) / 1 = parentX_start + 10

newLeft = parentX_start + 10 - (parentX_start - m_rect.left) = m_rect.left + 10

视觉上：
  drawRect.left 变化 = 10 × 1 = 10px ✓
  drawRect.width 不变（因为只改了 left，没改 width）✓
```

### 4.3 缩放实现

**核心策略：全在屏幕坐标下运算，最后再转换回 parent-local 坐标。**

原因：当 `xScale ≠ 1` 或 `yScale ≠ 1` 时，`m_rect` 的变化与屏幕像素变化不是简单的 1:1 关系。如果在 parent-local 坐标下直接加鼠标增量，会导致缩放后的窗口视觉反馈与鼠标不同步。

#### 4.3.1 缩放缩放率分析

WinFrame 的两个缩放率及其作用域：

```
getScaleXX() = m_xScale * parent->getScaleXX()   // 累计水平缩放（用于宽度的绘制）
getScaleYY() = m_yScale * parent->getScaleYY()   // 累计垂直缩放（用于高度的绘制）
parent->getScaleXX()                               // 仅父容器缩放（用于位置的绘制）
```

绘制公式（`getDrawRect`）：

```
drawRect.left   = m_rect.left * parentScaleXX + parentDrawRect.left
drawRect.top    = m_rect.top  * parentScaleYY + parentDrawRect.top
drawRect.width  = m_rect.width  * getScaleXX()
drawRect.height = m_rect.height * getScaleYY()
```

从公式可知：


| 修改目标        | 影响的绘制项      | 缩放因子                                    |
| --------------- | ----------------- | ------------------------------------------- |
| `m_rect.left`   | `drawRect.left`   | `parentScaleXX`                             |
| `m_rect.top`    | `drawRect.top`    | `parentScaleYY`                             |
| `m_rect.width`  | `drawRect.width`  | `getScaleXX()` = `m_xScale * parentScaleXX` |
| `m_rect.height` | `drawRect.height` | `getScaleYY()` = `m_yScale * parentScaleYY` |

**问题**：若直接在 parent-local 空间简单加减鼠标增量，当 `m_xScale ≠ 1` 时，左边缘移动造成的宽度变化与鼠标移动不同步。

示例：`xScale = 2`，`parentScale = 1`

```
鼠标向右拖动左边缘 10px（屏幕坐标）：
  若 dl = 10（parent-local 空间），宽度的视觉变化 = dl * m_xScale * parentScale = 10*2*1 = 20px
  这与鼠标 10px 不符，视觉反馈会"加速"一倍
```

#### 4.3.2 解决方案：屏幕坐标优先

1. 缩放开始时，记录 WinFrame 的**当前屏幕坐标系下的矩形**（`m_startScreenRect = getDrawRect()`）
2. 记录缩放开始时的鼠标屏幕坐标（`m_resizeStartMouse`）
3. 每次 `MouseMove` 时：
   a. 在屏幕坐标下计算新矩形
   b. 转换回 parent-local 坐标
   c. 调用 `setRect()`

```
// 步骤 1：屏幕坐标下计算新矩形
SRect newScreenRect = m_startScreenRect;
float dx = currentScreenX - m_resizeStartMouse.x;
float dy = currentScreenY - m_resizeStartMouse.y;

if (flags & kLeft)   { newScreenRect.left += dx; newScreenRect.width -= dx; }
if (flags & kRight)  { newScreenRect.width += dx; }
if (flags & kTop)    { newScreenRect.top += dy; newScreenRect.height -= dy; }
if (flags & kBottom) { newScreenRect.height += dy; }

// 步骤 2：转换回 parent-local 坐标
Control* parent = getParent();
SRect parentDrawRect = parent->getDrawRect();

float newLocalLeft   = (newScreenRect.left   - parentDrawRect.left) / parent->getScaleXX();
float newLocalTop    = (newScreenRect.top    - parentDrawRect.top)  / parent->getScaleYY();
float newLocalWidth  = newScreenRect.width  / getScaleXX();
float newLocalHeight = newScreenRect.height / getScaleYY();

// 步骤 3：约束最小尺寸（在 local 坐标下）
if (newLocalWidth  < MIN_WIDTH)  newLocalWidth  = MIN_WIDTH;
if (newLocalHeight < MIN_HEIGHT) newLocalHeight = MIN_HEIGHT;

// 对于左边缘/上边缘，最小尺寸时需调整位置以保持右/下边缘不动
if ((flags & kLeft) && newLocalWidth <= MIN_WIDTH) {
    newLocalLeft = m_startLocalRect.right() - MIN_WIDTH;
    newLocalWidth = MIN_WIDTH;
}
if ((flags & kTop) && newLocalHeight <= MIN_HEIGHT) {
    newLocalTop = m_startLocalRect.bottom() - MIN_HEIGHT;
    newLocalHeight = MIN_HEIGHT;
}

setRect({newLocalLeft, newLocalTop, newLocalWidth, newLocalHeight});
```

**关键验证**（`xScale=2`, `parentScale=1`, 鼠标向右拖拽右边缘 10px）：

```
dx_screen = 10
newScreenRect.width = startScreenWidth + 10
newLocalWidth = (startScreenWidth + 10) / (2 * 1) = startLocalWidth + 5
视觉宽度变化 = 5 * 2 * 1 = 10px ✓ （与鼠标移动一致）
```

#### 4.3.3 新增成员变量

```cpp
SRect   m_startScreenRect;     // 缩放开始时的屏幕坐标矩形
SRect   m_startLocalRect;      // 缩放开始时的 parent-local 矩形（即原来的 m_rect）
SPoint  m_resizeStartMouse;    // 缩放开始时的鼠标屏幕位置
```

#### 4.3.4 边缘检测函数

`detectResizeZone` 在 **WinFrame 自身局部坐标**下计算，因为 `m_edgeMargin` 是 local 单位：

```
SPoint localMouse = screenToLocal(screenX, screenY);
SRect& r = m_rect;

uint8_t flags = 0;
if (localMouse.x - 0            < m_edgeMargin) flags |= kLeft;
if (r.width  - localMouse.x     < m_edgeMargin) flags |= kRight;
if (localMouse.y - 0            < m_edgeMargin) flags |= kTop;
if (r.height - localMouse.y     < m_edgeMargin) flags |= kBottom;
```

其中 `screenToLocal` 将屏幕坐标转换为相对于 WinFrame 自身的未缩放局部坐标：

```
localX = (screenX - getDrawRect().left) / getScaleXX()
localY = (screenY - getDrawRect().top)  / getScaleYY()
```

**边缘热区配置**：

- 默认值：`4.0f`（parent-local 单位）
- 通过 `setEdgeMargin(float)` 动态修改
- 在 JSON 中通过 `"edgeMargin"` 属性设置（见第 8 节）
- 设置后立即生效，下次 `MouseMove` 时使用新值

#### 4.3.5 缩放光标

使用 `Cursor*` 抽象接口，避免直接依赖 `SDL_Cursor*`：

```cpp
// WinFrame 成员光标对象（构造函数中初始化）
m_cursorDefault(Cursor::getDefault()),
m_cursorSizeWE(Cursor::createSystem(SystemCursorType::EW_Resize)),
m_cursorSizeNS(Cursor::createSystem(SystemCursorType::NS_Resize)),
m_cursorSizeNWSE(Cursor::createSystem(SystemCursorType::NWSE_Resize)),
m_cursorSizeNESW(Cursor::createSystem(SystemCursorType::NESW_Resize))

// setResizeCursor 实现
void WinFrame::setResizeCursor(uint8_t flags) {
    Cursor* cursor = m_cursorDefault;
    switch (flags & 0x0F) {
        case 0:                                     cursor = m_cursorDefault; break;
        case kLeft|kRight:                          cursor = m_cursorSizeWE;  break;
        case kTop|kBottom:                          cursor = m_cursorSizeNS;  break;
        case kLeft:  case kRight:                   cursor = m_cursorSizeWE;  break;
        case kTop:   case kBottom:                  cursor = m_cursorSizeNS;  break;
        case kLeft|kTop:      case kRight|kBottom:  cursor = m_cursorSizeNWSE; break;
        case kRight|kTop:     case kLeft|kBottom:   cursor = m_cursorSizeNESW; break;
        case kLeft|kRight|kTop|kBottom:             cursor = m_cursorSizeWE;  break;
    }
    if (cursor) {
        Cursor::setCurrent(cursor);
    }
}
```

**Win32 平台的特殊处理**：

由于 SDL3 的 `SDL_SetCursor` 在 `WM_SETCURSOR` 机制下会被 SDL 内部默认光标覆盖（见 SDL Issue #12163/#12564），Win32 平台额外使用 `SetCursor` + `LoadCursorA` 作为回退，确保光标显示可靠：

```cpp
#ifdef _WIN32
    (void)cursor;
    static HCURSOR hcursors[16] = {NULL};
    int idx = flags & 0x0F;
    if (!hcursors[idx]) {
        switch (idx) {
            case 0:                                     hcursors[idx] = LoadCursorA(NULL, IDC_ARROW); break;
            case kLeft|kRight:                          hcursors[idx] = LoadCursorA(NULL, IDC_SIZEWE); break;
            case kTop|kBottom:                          hcursors[idx] = LoadCursorA(NULL, IDC_SIZENS); break;
            case kLeft:  case kRight:                   hcursors[idx] = LoadCursorA(NULL, IDC_SIZEWE); break;
            case kTop:   case kBottom:                  hcursors[idx] = LoadCursorA(NULL, IDC_SIZENS); break;
            case kLeft|kTop:      case kRight|kBottom:  hcursors[idx] = LoadCursorA(NULL, IDC_SIZENWSE); break;
            case kRight|kTop:     case kLeft|kBottom:   hcursors[idx] = LoadCursorA(NULL, IDC_SIZENESW); break;
            default:                                    hcursors[idx] = LoadCursorA(NULL, IDC_ARROW); break;
        }
    }
    if (hcursors[idx]) {
        SetCursor(hcursors[idx]);
    }
#else
    Cursor::setCurrent(cursor);
#endif
```

### 4.4 光标设置时机

- 在 `WinFrame::handleEvent` 的 `MouseMove` 分支中检测边缘
- 通过 `setResizeCursor()` 切换光标（内部委托到 `Cursor::setCurrent()`）
- 当鼠标离开边缘区时，恢复为默认箭头光标
- 子控件处理后，在第 3b 步重新应用边缘光标（子控件可能已覆盖光标，如 Label 设置手形光标）
- `m_lastEdgeFlags` 缓存上次边缘检测结果，避免子控件处理后丢失光标状态
- **Step 3c**：鼠标离开 WinFrame 边界时（`!bMouseInsideWinFrame`），立即恢复默认光标并清零 `m_lastEdgeFlags`，防止 cursor 停留在缩放箭头
- **MouseUp 结束 resize**：松开鼠标时，检查鼠标当前是否仍在边缘——若在边缘则保持缩放光标而非恢复默认，避免光标闪烁

### 4.5 Panel::handleEvent 在缩放时的行为

Panel（ControlImpl）的 handleEvent 会逆向遍历子控件。WinFrame override handleEvent，在以下情况拦截事件：

- 正在拖拽/缩放 → 拦截 MouseMove / MouseUp + LeftButton
- 鼠标在边缘 → 拦截 MouseDown + LeftButton（开始缩放）
- 鼠标在标题栏且子控件未消耗 → 拦截并开始拖拽

其他事件正常传递给子控件。

### 4.6 新版 handleEvent 实现

使用 union-based Event API，基于 `event->m_type` 分支判断：

```cpp
bool WinFrame::handleEvent(shared_ptr<Event> event) {
    if (!getVisible() || !getEnable()) return false;

    SPoint mousePos;
    bool hasPos = false;
    if (event->m_type == EventType::MouseMove) {
        mousePos = SPoint(event->mousePos.x, event->mousePos.y);
        hasPos = true;
    } else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mousePos = SPoint(event->mouseButton.x, event->mouseButton.y);
        hasPos = true;
    }

    // Step 0: Focus-to-front on MouseDown + Left within WinFrame
    if (hasPos && event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        if (getDrawRect().contains(mousePos.x, mousePos.y)) {
            bringToFront();
            consumedByFocus = true;
        }
    }

    // Step 1: Drag/Resize in progress
    if (m_dragging && hasPos) { ... }
    if (m_resizing && hasPos) { ... }

    // Step 2: Edge detection
    if (hasPos && bMouseInsideWinFrame && !m_dragging && !m_resizing) {
        // Detect edge flags, start resize / set cursor
    }

    // Step 3: Children
    bool consumed = ControlImpl::handleEvent(event);

    // Step 3b: Re-apply edge cursor (children may have overwritten it)
    if (hasPos && ... && event->m_type == EventType::MouseMove) {
        if (m_lastEdgeFlags && m_resizable)
            setResizeCursor(m_lastEdgeFlags);
        else
            setResizeCursor(0);
    }

    // Step 3c: Mouse left WinFrame — restore default cursor
    if (!bMouseInsideWinFrame && event->m_type == EventType::MouseMove) {
        if (m_lastEdgeFlags != 0) {
            setResizeCursor(0);
            m_lastEdgeFlags = 0;
        }
    }

    // Step 4: Title bar drag
    if (hasPos && !consumed && !m_dragging && !m_resizing) {
        if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
            SPoint localMouse = screenToLocal(mousePos.x, mousePos.y);
            // Check title bar area (excluding edge margin and close button)
        }
    }
    ...
}
```

关键变更：
- `event->m_type == EventType::MouseDown/Up/Move` 替代 `EventName::MOUSE_LBUTTON_DOWN/MOUSE_MOVING/MOUSE_LBUTTON_UP`
- `event->mouseButton.x/y` 和 `event->mouseButton.button` 获取鼠标位置和按键
- `event->mousePos.x/y` 获取移动事件位置
- 不再使用 `std::any_cast<shared_ptr<SPoint>>` 和 `try/catch`
- 不再使用 `EventQueue::isPositionEvent` 判断

## 5. 标题栏点击区域判定

- 标题栏区域：`rect(0, 0, m_rect.width, WINDOW_TITLE_HEIGHT)`
- 排除关闭按钮区域：`rect(m_rect.width - WINDOW_TITLE_HEIGHT, 0, WINDOW_TITLE_HEIGHT, WINDOW_TITLE_HEIGHT)`
- 排除左/右边缘热区（`m_edgeMargin` 宽度）
- 排除上边缘热区（`m_edgeMargin` 高度）
- 在子控件通过 `ControlImpl::handleEvent` 处理后，检查事件是否已被消耗
- 若未被消耗且在标题栏区域内，则开始拖拽

## 6. 坐标转换策略

### 6.1 三种坐标系


| 坐标系                | 含义                       | 来源                              | 用途               |
| --------------------- | -------------------------- | --------------------------------- | ------------------ |
| **屏幕坐标**          | SDL 窗口的物理像素坐标     | `event->mousePos/mouseButton`     | 鼠标位置、事件判断 |
| **DrawRect 坐标**     | 控件绘制到屏幕上的实际矩形 | `getDrawRect()`                   | 绘制、hit-test     |
| **parent-local 坐标** | 相对父容器的未缩放坐标     | `m_rect`                          | 位置存储、setRect  |

### 6.2 坐标转换公式

```
// 屏幕坐标 → 控件自身局部（用于边缘检测、标题栏判定）
localX = (screenX - getDrawRect().left) / getScaleXX()
localY = (screenY - getDrawRect().top)  / getScaleYY()

// 屏幕坐标 → 父容器局部（用于拖拽位置更新）
parentLocalX = (screenX - parentDrawRect.left) / parent->getScaleXX()
parentLocalY = (screenY - parentDrawRect.top)  / parent->getScaleYY()

// 父容器局部坐标 → 屏幕坐标
drawRect.left   = m_rect.left   * parent->getScaleXX() + parentDrawRect.left
drawRect.top    = m_rect.top    * parent->getScaleYY() + parentDrawRect.top
drawRect.width  = m_rect.width  * getScaleXX()
drawRect.height = m_rect.height * getScaleYY()
```

### 6.3 各操作使用的坐标系


| 操作                           | 运算坐标系             | 原因                                    |
| ------------------------------ | ---------------------- | --------------------------------------- |
| 边缘检测（`detectResizeZone`） | WinFrame 自身局部      | `EDGE_MARGIN` 是 local 单位             |
| 标题栏点击判定                 | WinFrame 自身局部      | `m_rect` 的 title bar 区域是 local 单位 |
| 拖拽位置更新                   | 父容器局部             | `m_rect.left/top` 是 parent-local 单位  |
| 缩放 rect 计算                 | 屏幕坐标 → 父容器局部 | 见 4.3.2 缩放率分析                     |

### 6.4 缩放对坐标转换的影响（详细分析）

**定义**：

```
S  = WinFrame 自身的 xScale（m_xScale）
P  = 父容器的累计缩放（parent->getScaleXX()）
总缩放 = S × P = getScaleXX()
```

**场景**：WinFrame 的 `m_rect = {100, 100, 200, 100}`，`S = 2`，`P = 1`。

```
drawRect.left   = 100 × 1 + parentDrawRect.left  = 100 + parentLeft
drawRect.width  = 200 × 2 × 1 = 400

视觉上宽度 = 400px（是 parent-local 宽度 200 的两倍）
```

**鼠标向右拖动右边缘 10px（屏幕坐标）**：

```
直接 parent-local 方法 ❌：
  m_rect.width += 10         → new width = 210
  视觉宽度变化 = 10 × 2 = 20px
  反馈"加速"一倍，与实际鼠标 10px 不符

屏幕坐标优先法 ✅：
  newScreenWidth = startScreenWidth + 10
  newLocalWidth  = newScreenWidth / (S × P) = 410 / 2 = 205
  setRect(m_rect.left, m_rect.top, 205, m_rect.height)
  视觉宽度变化 = 5 × 2 = 10px ✓
```

**鼠标向左拖动左边缘 10px（屏幕坐标）**：

```
屏幕坐标法：
  newScreenLeft = startScreenLeft + 10   // 左边缘右移 10px
  newScreenWidth = startScreenWidth - 10 // 宽度减少 10px

  newLocalLeft = (screenLeft + 10 - parentDrawRect.left) / P
              = (drawRect.left + 10 - parentDrawRect.left) / P
              = startLocalLeft + 10 / P = 100 + 10 = 110

  newLocalWidth = (startScreenWidth - 10) / (S × P)
               = (400 - 10) / 2 = 195

  验证：
    视觉位置变化 = 10 × 1 = 10px ✓ （左边缘右移 10px）
    视觉宽度变化 = 195 × 2 - 200 × 2 = -10px ✓ （宽度减少 10px）
    右边缘 = 110 + 195 = 305（parent-local）= 视觉上 305px
    原右边缘 = 100 + 200 = 300（parent-local）= 视觉上 300px
    右边缘变化 = 5px（parent-local），视觉变化 = 5 × 2 = 10px
    但鼠标只移动了 10px... ❌ 右边缘不应该动！

问题：
  P = 1, S = 2 时，左边缘移动 10px（parent-local）会改变宽度的视觉缩放：
  宽度变化 = -dl × S × P = -10 × 2 = -20px
  但鼠标只移动了 10px，宽度不应减少 20px

根因：
  m_rect.left 的缩放因子是 P（父容器缩放）
  m_rect.width 的缩放因子是 S × P（总缩放）
  两者不一致！

修正（屏幕坐标法自动修正，见 4.3.2 的步骤 3 最小尺寸处理）：
  用屏幕坐标法，左边缘移动后宽度自然正确：
  newLocalLeft = startLocalLeft + dx/P = 100 + 10 = 110
  newLocalWidth = (startScreenWidth - dx) / (S×P) = (400-10)/2 = 195

  这已经自动考虑了 P 和 S×P 的不一致。
  不需要特殊修正即可保证右边缘不动。

验证右边缘：
  原右边缘 screen = 100×1 + parentLeft + 200×2 = parentLeft + 500
  新右边缘 screen = 110×1 + parentLeft + 195×2 = parentLeft + 500 ✓
```

### 6.5 拖拽位置的缩放

拖拽只修改 `m_rect.left/top`，这些字段的缩放因子仅为 `parentScale`（不含自身缩放）：

```
m_dragOffset.x = parentLocalX - m_rect.left
               = (screenX - parentDrawRect.left) / parentScaleX - m_rect.left

新位置：
m_rect.left = (screenX - parentDrawRect.left) / parentScaleX - m_dragOffset.x
```

不受 WinFrame 自身 `xScale/yScale` 影响，因为位置是相对于父容器的未缩放坐标。

## 7. WinFrame::draw()——关闭按钮向量 X 叠加层

### 7.1 设计动机

所有后端共享的跨平台回退方案：在 `Panel::draw()` 绘制的 PNG 关闭按钮之上，叠加绘制向量 X 标记。

当某个后端（如 Raylib）的纹理混合模式未生效或 PNG 透明通道渲染异常时，向量 X 确保关闭按钮在任何情况下都可见。

### 7.2 实现方式

```cpp
void WinFrame::draw() {
    Panel::draw();  // 先绘制标准控件内容（含 PNG 关闭按钮）

    // 在关闭按钮上方叠加绘制向量 X
    SRect btnRect = m_closeButton->getDrawRect();
    float cx = btnRect.left + btnRect.width / 2;
    float cy = btnRect.top + btnRect.height / 2;
    float half = (std::min(btnRect.width, btnRect.height) * 0.3f);
    float thickness = std::max(1.0f, half * 0.3f);

    // 根据按钮状态选择颜色
    SColor xColor;
    switch (m_closeButton->getState()) {
        case ControlState::Hover:   xColor = SColor(255, 255, 255, 255); break;  // 白
        case ControlState::Pressed: xColor = SColor(255, 255, 100, 255); break; // 黄
        default:                    xColor = SColor(200, 200, 200, 255); break;  // 浅灰
    }

    auto* rd = getRenderDevice();
    rd->setDrawColor(xColor);

    // 6 条线组成 X（每条对角线由 3 条平行线组成，实现粗线效果）
    // 第一组对角线（\）
    rd->drawLine(cx - half + 0.0f, cy - half, cx + half + 0.0f, cy + half);
    rd->drawLine(cx - half - 1.0f, cy - half, cx + half - 1.0f, cy + half);
    rd->drawLine(cx - half + 1.0f, cy - half, cx + half + 1.0f, cy + half);

    // 第二组对角线（/）
    rd->drawLine(cx + half + 0.0f, cy - half, cx - half + 0.0f, cy + half);
    rd->drawLine(cx + half - 1.0f, cy - half, cx - half - 1.0f, cy + half);
    rd->drawLine(cx + half + 1.0f, cy - half, cx - half + 1.0f, cy + half);
}
```

### 7.3 设计要点

- 在 `Panel::draw()` 之后绘制，确保 X 在所有子控件之上
- 通过 3 条平行线叠加实现粗 X 效果（避免后端 line-width 差异）
- 颜色随关闭按钮状态变化：Normal=浅灰(200,200,200)、Hover=白(255,255,255)、Pressed=黄(255,255,100)
- 无额外纹理依赖，纯向量绘制，跨后端一致
- 不依赖纹理混合模式，透明通道渲染问题不影响 X 可见性

## 8. JSON 布局支持

在 LayoutParser 中新增 `"WinFrame"` 类型，替换原来的 `"Dialog"`：

```json
{
    "type": "WinFrame",
    "id": "myWinFrame",
    "rect": { "x": 100, "y": 100, "w": 400, "h": 300 },
    "colors": {
        "background": { "normal": "#303030FF" },
        "border": { "normal": "#606060FF" },
        "titleBar": { "bg": { "normal": "#2A2A50FF" } },
        "titleText": { "normal": "#FFFFFFFF" }
    },
    "title": "标题",
    "edgeMargin": 4.0,
    "children": [
        { "type": "Label", "caption": "Hello" },
        { "type": "Button", "caption": "OK" }
    ],
    "events": {
        "onClose": "onWinFrameClosed"
    }
}
```

- `"title"`：标题栏文字（可选，默认 "WinFrame"）
- `"edgeMargin"`：边缘热区宽度，parent-local 单位（可选，默认 4.0）
- `"resizable"`：是否允许用户调整大小（可选，默认 true）
- `"colors"`：颜色配置（可选）
  - `"background"` / `"border"`：WinFrame 整体背景/边框色（标准 Panel 颜色格式）
  - `"titleBar"`：标题栏面板颜色（`"bg"` 等，传递给 `m_titleBar`）
  - `"titleText"`：标题文字颜色（传递给 `m_titleLabel`）
- `"children"`：添加到客户区域 Panel 中的子控件（可选）
- `"events"`：支持 `"onClose"` 事件（关闭时触发回调，可选）

### 8.1 LayoutParser 实现要点

```cpp
shared_ptr<WinFrame> LayoutParser::parseWinFrame(const json& j, Control* parent) {
    SRect rect = parseRect(j["rect"]);
    auto winFrame = make_shared<WinFrame>(parent, rect);

    if (j.contains("title") && j["title"].is_string()) {
        winFrame->setTitle(j["title"].get<string>());
    }

    if (j.contains("edgeMargin") && j["edgeMargin"].is_number()) {
        winFrame->setEdgeMargin(j["edgeMargin"].get<float>());
    }

    // 颜色解析
    if (j.contains("colors") && j["colors"].is_object()) {
        const json& colors = j["colors"];
        parseControlColors(winFrame.get(), colors);  // 复用通用颜色解析（background/border）

        if (colors.contains("titleBar") && colors["titleBar"].is_object()) {
            const json& tb = colors["titleBar"];
            if (tb.contains("bg") && tb["bg"].is_object()) {
                winFrame->getTitleBar()->setBackgroundStateColor(
                    parseStateColor(tb["bg"], StateColor::Type::Background));
            }
        }
        if (colors.contains("titleText") && colors["titleText"].is_object()) {
            winFrame->getTitleLabel()->setTextStateColor(
                parseStateColor(colors["titleText"], StateColor::Type::Text));
        }
    }

    // 子控件添加到 ClientPanel
    if (j.contains("children")) {
        parseChildren(winFrame->getClientPanel().get(), j);
    }

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = winFrame;
    }

    winFrame->create();
    winFrame->hide();
    return winFrame;
}
```

## 9. Builder 模式

```cpp
class WinFrameBuilder {
private:
    shared_ptr<WinFrame> m_winFrame;
public:
    WinFrameBuilder(Control* parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

    // 快捷颜色设置（使用 SColor）
    WinFrameBuilder& setWinFrameBGColor(const SColor& color);
    WinFrameBuilder& setWinFrameBorderColor(const SColor& color);
    WinFrameBuilder& setTitleBarBGColor(const SColor& color);
    WinFrameBuilder& setClientBGColor(const SColor& color);

    // 标题栏细节
    WinFrameBuilder& setTitle(const string& title);
    WinFrameBuilder& setTitleFont(FontName font);
    WinFrameBuilder& setTitleFontSize(int size);
    WinFrameBuilder& setTitleTextColor(const SColor& color);
    WinFrameBuilder& setTitleAlignment(AlignmentMode align);

    // 边缘热区
    WinFrameBuilder& setEdgeMargin(float margin);

    // 缩放
    WinFrameBuilder& setResizable(bool resizable);

    // 客户区域
    WinFrameBuilder& addToClient(shared_ptr<Control> control);

    // 事件
    WinFrameBuilder& setOnClose(OnClickHandler handler);

    shared_ptr<WinFrame> build();
};
```

Builder 仅提供最常见属性的快捷设置。对于 Builder 未覆盖的定制需求，用户可通过 Getter 获取子控件后进行修改：

```cpp
auto wf = WinFrameBuilder(parent, rect)
    .setTitle("My Window")
    .build();

// 通过 Getter 定制标题 Label 细节
wf->getTitleLabel()->setFont(FontName::MapleMono_NF_CN_Regular);
wf->getTitleLabel()->setFontSize(20);
wf->getTitleLabel()->setTextNormalStateColor(SColor(255, 255, 0, 255));

// 通过 Getter 定制关闭按钮
wf->getCloseButton()->setCaption("×");
wf->getCloseButton()->setCaptionSize(16);

// 通过 Getter 定制标题栏面板
wf->getTitleBar()->setNormalStateBGColor(SColor(50, 50, 80, 255));
wf->getTitleBar()->setBorderVisible(true);
wf->getTitleBar()->setBorderStateColor(StateColor(SColor(100, 100, 100, 255)));
```

## 10. 移除内容

- 完全移除 `Dialog.h` / `Dialog.cpp`
- 从 `Bench.h` 中移除 `#include "Dialog.h"`，改为 `#include "WinFrame.h"`
- 从 `LayoutParser.h` 中移除 `parseDialog` 声明，添加 `parseWinFrame`
- 从 `LayoutParser.cpp` 中移除 `parseDialog` 实现和 `"Dialog"` 类型分支，添加 `"WinFrame"` 分支
- 从 `test_layout.cpp` 和 `layouts/test_layout.json` 中将 Dialog 引用替换为 WinFrame
- 从 `CMakeLists.txt` 中将 `src/Dialog.cpp` 替换为 `src/WinFrame.cpp`

## 11. 测试计划

1. **基本功能**：构建 WinFrame 实例，显示标题栏和关闭按钮
2. **标题栏拖拽**：鼠标按下标题栏拖拽，WinFrame 跟随移动
3. **拖拽缩放正确性（xScale=2）**：WinFrame 的 xScale=2，拖拽时鼠标移动 10px，窗口位置变化 10px（不受自身缩放影响）
4. **边缘缩放（xScale=1）**：鼠标在 4px 边缘出现双箭头光标，拖拽可改变大小
5. **边缘缩放（xScale=2）**：鼠标在边缘出现双箭头光标，拖拽右边缘 10px，视觉宽度变化 10px（屏幕坐标法正确）
6. **左边缘缩放（xScale=2）**：鼠标拖拽左边缘，右边缘保持不动（验证 P 和 S×P 的不一致修正）
7. **边缘热区配置**：设置 edgeMargin=8，确认 8px 内均可触发缩放
8. **最小尺寸**：缩放至 <100x60 时被限制
9. **客户区域**：向 ClientPanel 添加 Label，确认显示正常
10. **关闭按钮**：点击关闭按钮触发 hide()，向量 X 与 PNG 图标重叠显示正常
11. **向量 X 颜色**：Normal 浅灰、Hover 白、Pressed 黄，三种状态颜色正确
12. **show() 置顶**：先打开两个 WinFrame，然后对第一个调用 show()，确认它出现在最前端
13. **JSON 解析**：`test_layout.json` 中将 Dialog 替换为 WinFrame，包含 edgeMargin 属性
14. **坐标边界**：拖拽时鼠标移出 WinFrame 仍继续追踪，释放后停止
