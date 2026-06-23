# 手柄控件（HandleControl）设计文档

## 1. 概述

手柄控件（HandleControl）是一种交互式 UI 控件，允许用户通过拖拽一组视觉手柄来**调整另一个控件的位置和大小**。类似 Photoshop、Figma、Unity Editor 等设计工具中的变换工具（Transform Gizmo / Bounding Box Handles）。

### 1.1 典型使用场景

- **运行时编辑器**：在运行的应用程序中提供控件布局调整能力
- **UI 调试工具**：可视化调整控件的边界和位置
- **设计器/画布工具**：允许用户在画布上自由排布控件
- **窗口管理器**：替代 WinFrame 的边缘缩放，提供更直观的 8 方向缩放

### 1.2 职责边界

| 职责 | 非职责 |
|------|--------|
| 提供 8 个缩放手柄 + 1 个移动手柄 | 改变目标控件的父子关系 |
| 拖拽手柄实时更新目标 `setRect()` | 管理目标控件的生命周期 |
| 绘制虚线选择框标识目标边界 | 提供撤销/重做 |
| 手柄悬停时光标反馈 | 支持多选（第一期不考虑） |
| 自动跟随目标位置变化 | 管理目标控件的内部布局 |

---

## 2. 类设计

### 2.1 完整 API

```cpp
class HandleControl : public ControlImpl {
public:
    HandleControl();
    ~HandleControl() override;

    // ── 附加/分离 ──
    // 附加到目标控件。内部存储 shared_ptr 的 weak_ptr 以监控目标生命周期。
    void setTarget(shared_ptr<Control> target);

    // 获取当前目标（原始指针，仅用于查询）
    Control* getTarget() const;

    // 从目标分离，清理手柄绘制
    void detach();

    // ── 配置 ──
    // 手柄方块大小（像素），默认 8.0f
    void setHandleSize(float size);
    float handleSize() const;

    // 目标最小尺寸约束，默认 20x20
    void setMinSize(float w, float h);
    SSize minSize() const;

    // 手柄颜色：填充色 + 边框色，默认 白色填充 + 蓝色边框
    void setHandleColor(SColor fill, SColor border);
    SColor handleFillColor() const;
    SColor handleBorderColor() const;

    // 拖拽高亮颜色，默认 蓝色填充
    void setActiveColor(SColor color);
    SColor activeColor() const;

    // 选择框颜色，默认 蓝色
    void setSelectionColor(SColor color);
    SColor selectionColor() const;

    // ── 手柄可见性控制 ──
    void setCornerHandlesVisible(bool show);   // 4 角手柄，默认 true
    void setEdgeHandlesVisible(bool show);     // 4 边中点手柄，默认 true
    void setMoveHandleVisible(bool show);      // 中心移动手柄，默认 true
    void setSelectionBoxVisible(bool show);    // 虚线选择框，默认 true

    // ── Control 接口重写 ──
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;

private:
    // 手柄类型
    enum class HandleType : uint8_t {
        None,           // 不在手柄上
        Move,           // 中心移动手柄
        NW, N, NE,      // 上排：左上角、上边中点、右上角
        E, SE, S, SW, W // 下排：右边中点、右下角、下边中点、左下角、左边中点
    };

    // ── 拖拽状态 ──
    bool       m_resizing    = false;
    bool       m_dragging    = false;
    HandleType m_activeHandle = HandleType::None;

    SRect  m_startTargetRect;      // 拖拽开始时的目标控件 rect（局部坐标）
    SRect  m_startScreenRect;      // 拖拽开始时的目标控件 rect（屏幕坐标）
    SPoint m_startMousePos;        // 拖拽开始时的鼠标位置（屏幕坐标）

    // ── 目标引用 ──
    Control*         m_target     = nullptr;   // 热路径：快速访问
    weak_ptr<Control> m_targetWeak;            // 生命周期监控

    // ── 配置 ──
    float  m_handleSize       = 8.0f;
    float  m_minWidth         = 20.0f;
    float  m_minHeight        = 20.0f;
    SColor m_handleFill       = SColor(255, 255, 255, 255);  // 白色
    SColor m_handleBorder     = SColor(0, 120, 255, 255);    // 蓝色
    SColor m_activeFill       = SColor(0, 120, 255, 255);    // 蓝色
    SColor m_selectionColor   = SColor(0, 120, 255, 255);    // 蓝色

    bool m_showCornerHandles  = true;
    bool m_showEdgeHandles    = true;
    bool m_showMoveHandle     = true;
    bool m_showSelectionBox   = true;

    // ── 光标缓存（持久化，避免频繁创建/销毁）──
    Cursor* m_cursorDefault = nullptr;
    Cursor* m_cursorMove    = nullptr;
    Cursor* m_cursorWE      = nullptr;
    Cursor* m_cursorNS      = nullptr;
    Cursor* m_cursorNWSE    = nullptr;
    Cursor* m_cursorNESW    = nullptr;

    void ensureCursors();
    void cleanupCursors();

    // ── 手柄区域缓存（每次 draw/event 前更新，固定 9 个）──
    struct HandleArea {
        SRect      rect;      // 手柄的包围盒（屏幕坐标）
        HandleType type;
    };
    HandleArea m_handleAreas[9];
    int m_handleAreaCount = 0;

    // ── 内部方法 ──
    // 坐标转换
    SRect  targetToScreen();             // 目标局部 rect → 屏幕 rect
    SPoint screenToTargetLocal(float sx, float sy);

    // 手柄管理
    void     updateHandleAreas();        // 由 draw() 调用，重建 m_handleAreas
    HandleType hitTestHandle(float mx, float my);  // 命中检测

    // 缩放操作（8 方向完整实现）
    void startResize(HandleType type, const SPoint& mousePos);
    void updateResize(const SPoint& mousePos);
    void endResize();

    // 移动操作
    void startDrag(const SPoint& mousePos);
    void updateDrag(const SPoint& mousePos);
    void endDrag();

    // 光标切换
    void setResizeCursor(HandleType type);

    // 绘制
    void drawHandle(const SRect& rect, HandleType type, bool active);
    void drawSelectionBox(const SRect& targetScreenRect);
    void drawMoveHandle(const SRect& targetScreenRect);
};
```

### 2.2 手柄类型与光标映射

| HandleType | 位置 | 光标 | 缩放维度 |
|-----------|------|------|---------|
| Move | 目标中心 | `Move` | —（移动位置） |
| NW | 左上角 | `NWSE_Resize` | 宽高同时缩放 |
| N | 上边中点 | `NS_Resize` | 高度 |
| NE | 右上角 | `NESW_Resize` | 宽高同时缩放 |
| E | 右边中点 | `EW_Resize` | 宽度 |
| SE | 右下角 | `NWSE_Resize` | 宽高同时缩放 |
| S | 下边中点 | `NS_Resize` | 高度 |
| SW | 左下角 | `NESW_Resize` | 宽高同时缩放 |
| W | 左边中点 | `EW_Resize` | 宽度 |

> **Windows 光标兼容性**：此 SDL3 fork 中 `SDL_SetCursor` 不持久（
> `WM_SETCURSOR` 在鼠标移动时复位光标）。HandleControl 在
> `#ifdef _WIN32` 路径下直接调用 Win32 `SetCursor(LoadCursor(...))`，
> 非 Windows 平台仍使用 `Cursor::setCurrent`。详见 WinFrame.cpp 相同处理。

---

## 3. 坐标系统

### 3.1 自身定位

HandleControl 与目标控件是**同级兄弟节点**，添加到同一父容器。HandleControl 的位置跟随目标，始终略大于目标包围盒以容纳手柄的外延部分：

```cpp
// 在 draw() 中主动校正位置
void HandleControl::draw() {
    if (!m_target) return;

    SRect tr = m_target->getRect();       // 目标控件局部坐标
    float h = m_handleSize / 2.0f;
    setRect({
        tr.left - h,
        tr.top - h,
        tr.width  + m_handleSize,
        tr.height + m_handleSize
    });

    // ... 绘制手柄和选择框
}
```

即使父容器的 LayoutEngine 移动了 HandleControl，下一帧 `draw()` 自动拉回正确位置。

### 3.2 拖拽坐标转换

与 WinFrame 缩放机制（`src/WinFrame.cpp`）完全一致：

```
MouseDown: record start state
    m_startTargetRect = m_target->getRect()          // 局部坐标
    m_startScreenRect = m_target->getDrawRect()      // 屏幕坐标
    m_startMousePos   = mousePos                     // 屏幕坐标

MouseMove: compute delta in screen coords → convert to local coords
    float dx = mousePos.x - m_startMousePos.x
    float dy = mousePos.y - m_startMousePos.y

    父容器 = m_target->getParent()->getDrawRect()
    float scaleX = m_target->getParent()->getScaleXX()
    float scaleY = m_target->getParent()->getScaleYY()

    float newLeft = (m_startScreenRect.left + dx - 父容器.left) / scaleX
    float newTop  = (m_startScreenRect.top  + dy - 父容器.top ) / scaleY

    // 8 方向缩放/移动逻辑基于 newLeft/newTop + startTargetRect 计算
    m_target->setRect({...})

MouseUp: cleanup
    m_resizing = false
    m_dragging = false
```

### 3.3 手柄命中检测

手柄方块的中心点位于目标包围盒的 8 个关键位置 + 中心。每个手柄的 `SRect` 以该中心点扩展 `m_handleSize` 大小（屏幕坐标）。缓存为固定数组 `HandleArea m_handleAreas[9]`（非 vector，避免每帧分配）：

```
目标屏幕包围盒: {sx, sy, sw, sh}

NW 中心: (sx,      sy)           N 中心: (sx+sw/2, sy)
NE 中心: (sx+sw,   sy)           E 中心: (sx+sw,   sy+sh/2)
SE 中心: (sx+sw,   sy+sh)        S 中心: (sx+sw/2, sy+sh)
SW 中心: (sx,      sy+sh)        W 中心: (sx,      sy+sh/2)
Move中心: (sx+sw/2, sy+sh/2)

手柄 rect: {centerX - h, centerY - h, m_handleSize, m_handleSize}
  (其中 h = m_handleSize / 2)
```

---

## 4. 交互逻辑

### 4.1 事件处理优先级

```
HandleControl::handleEvent(event)
├── 非位置事件 → 返回 false（不消费）
├── 目标已过期 → 隐藏自身，返回 false
├── 拖拽/缩放中 → 消费 MouseMove/MouseUp，每帧调用 setResizeCursor 保持光标
│   ├── m_resizing → setResizeCursor(m_activeHandle) + updateResize
│   └── m_dragging → setResizeCursor(Move) + updateDrag
├── 命中手柄 → 消费事件 + setResizeCursor 保持光标
│   ├── MouseDown → startResize / startDrag + setResizeCursor
│   ├── MouseMove → setResizeCursor (命中测试)
│   └── MouseUp   → endResize / endDrag
│                    + updateHandleAreas + hitTest
│                    + setResizeCursor(ht)  ← 释放后保持手柄光标
└── 未命中手柄 → 返回 false（穿透到目标控件）
```

关键设计：**非拖拽时事件穿透**。只有点击/拖拽在手柄上时 HandleControl 才消费事件，其他情况返回 `false`，事件继续传递给目标控件和兄弟控件。

### 4.2 SE 手柄缩放逻辑（参考实现）

SE 是最简单的手柄（只有宽高增加，无位置偏移）：

```cpp
void HandleControl::updateResize(const SPoint& mousePos) {
    float dx = mousePos.x - m_startMousePos.x;
    float dy = mousePos.y - m_startMousePos.y;

    SRect newScreen = m_startScreenRect;
    // 根据活动手柄类型修改 newScreen
    switch (m_activeHandle) {
    case HandleType::SE:
        newScreen.width  += dx;
        newScreen.height += dy;
        break;
    case HandleType::NW:
        newScreen.left   += dx;
        newScreen.top    += dy;
        newScreen.width  -= dx;
        newScreen.height -= dy;
        break;
    case HandleType::E:
        newScreen.width  += dx;
        break;
    case HandleType::S:
        newScreen.height += dy;
        break;
    // ... 全部 8 个方向
    }

    // 屏幕坐标 → 局部坐标
    auto* parent = m_target->getParent();
    SRect parentDraw = parent->getDrawRect();
    float scaleX = parent->getScaleXX();
    float scaleY = parent->getScaleYY();

    float newLocalLeft   = (newScreen.left - parentDraw.left) / scaleX;
    float newLocalTop    = (newScreen.top  - parentDraw.top)  / scaleY;
    float newLocalWidth  = newScreen.width  / m_target->getScaleXX();
    float newLocalHeight = newScreen.height / m_target->getScaleYY();

    // 最小尺寸约束
    newLocalWidth  = max(newLocalWidth,  m_minWidth  / m_target->getScaleXX());
    newLocalHeight = max(newLocalHeight, m_minHeight / m_target->getScaleYY());

    m_target->setRect({newLocalLeft, newLocalTop, newLocalWidth, newLocalHeight});
}
```

### 4.3 移动逻辑

```cpp
void HandleControl::updateDrag(const SPoint& mousePos) {
    float dx = mousePos.x - m_startMousePos.x;
    float dy = mousePos.y - m_startMousePos.y;

    SRect newScreen = m_startScreenRect;
    newScreen.left += dx;
    newScreen.top  += dy;

    auto* parent = m_target->getParent();
    SRect parentDraw = parent->getDrawRect();

    float newLocalLeft = (newScreen.left - parentDraw.left) / parent->getScaleXX();
    float newLocalTop  = (newScreen.top  - parentDraw.top)  / parent->getScaleYY();

    m_target->setRect({
        newLocalLeft, newLocalTop,
        m_startTargetRect.width,
        m_startTargetRect.height
    });
}
```

---

## 5. Z-order 管理

### 5.1 原则

1. **目标控件的 Z-order 不受影响**——HandleControl 从不操作目标在父容器 `m_children` 中的位置
2. **HandleControl 自身在顶层**——始终在父容器 children 末尾，即最后绘制、最先收到事件

### 5.2 `setAlwaysOnTop` 机制

在 `ControlImpl` 层面添加通用 `setAlwaysOnTop(bool)` 标志，替代 in-per-frame 的 `ensureTopmost()`。新增的 `addControl` 逻辑在每次添加子控件后自动把带此标志的控件挪回末尾：

```cpp
// ── ControlBase.h ──

class ControlImpl : public Control {
public:
    void setAlwaysOnTop(bool on) { m_alwaysOnTop = on; }
    bool isAlwaysOnTop() const { return m_alwaysOnTop; }

private:
    bool m_alwaysOnTop = false;
    void stabilizeTopmostChildren();
};
```

```cpp
// ── ControlBase.cpp —— addControl 末尾追加 ──

void ControlImpl::addControl(shared_ptr<Control> child) {
    if (child == nullptr) return;
    if (std::find(m_children.begin(), m_children.end(), child) != m_children.end())
        return;
    m_children.push_back(child);
    child->setParent(this);
    child->setRenderDevice(getRenderDevice());

    stabilizeTopmostChildren();  // 保持 alwaysOnTop 子控件在末尾
}

void ControlImpl::stabilizeTopmostChildren() {
    // 收集所有 alwaysOnTop 的子控件
    vector<shared_ptr<Control>> topmost;
    for (auto it = m_children.begin(); it != m_children.end(); ) {
        if ((*it)->isAlwaysOnTop()) {
            topmost.push_back(std::move(*it));
            it = m_children.erase(it);
        } else {
            ++it;
        }
    }
    // 重新追加到末尾（保持它们之间的相对顺序）
    for (auto& child : topmost) {
        m_children.push_back(std::move(child));
    }
}
```

**特性**：
- 任何有此标志的子控件都会自动保持在 children 末尾
- 支持**多个** alwaysOnTop 子控件（如同时存在的 HandleControl + ContextMenu）
- O(n) per addControl（罕见操作），无需每帧检查
- 对非 alwaysOnTop 的 99% 控件零影响

### 5.3 HandleControl 使用

```cpp
void HandleControl::setTarget(shared_ptr<Control> target) {
    if (m_target) detach();
    m_target = target.get();
    m_targetWeak = target;

    Control* parent = target->getParent();
    if (parent) {
        parent->addControl(shared_from_this());  // 添加到末尾
        setAlwaysOnTop(true);                     // 标记：以后也保持末尾
        setVisible(true);
    }
}
```

`setAlwaysOnTop(true)` 只需在 `setTarget` 中调用一次，后续父容器任何 `addControl` 调用都会自动把 HandleControl 挪回末尾，无需每帧干预。

### 5.4 分离清理

```cpp
void HandleControl::detach() {
    m_target = nullptr;
    m_targetWeak.reset();
    m_resizing = false;
    m_dragging = false;
    m_activeHandle = HandleType::None;
    setAlwaysOnTop(false);  // 清除标志

    Control* parent = getParent();
    if (parent) {
        parent->removeControl(shared_from_this());
    }
}
```

---

## 6. 目标生命周期管理

### 6.1 自动感知删除

```cpp
void HandleControl::draw() {
    // 检查目标是否存活
    if (m_targetWeak.expired()) {
        // 目标已被销毁，自动隐藏
        m_target = nullptr;
        m_targetWeak.reset();
        setVisible(false);
        return;
    }
    if (!m_target || !m_target->getVisible()) {
        setVisible(false);
        return;
    }
    // ...
}

bool HandleControl::handleEvent(shared_ptr<Event> event) {
    if (m_targetWeak.expired()) {
        setVisible(false);
        m_target = nullptr;
        m_targetWeak.reset();
        return false;
    }
    // ...
}
```

目标被删除后，`shared_ptr` 析构触发 `weak_ptr` 过期，下次 `draw()`/`handleEvent()` 检测到后自动隐藏，无需外部 `detach()` 调用。

---

## 7. 绘制

### 7.1 虚线选择框

使用 `DrawingContext::drawLine()` 配合 `PenStyle::Dash` 笔触绘制 4 条线段：

```cpp
void HandleControl::drawSelectionBox(const SRect& targetScreen) {
    if (!m_showSelectionBox) return;

    DrawingContext ctx(getRenderDevice());
    SPen pen(m_selectionColor, 1.5f, PenStyle::Dash);
    ctx.setPen(pen);

    float l = targetScreen.left;
    float t = targetScreen.top;
    float r = targetScreen.right();
    float b = targetScreen.bottom();

    ctx.drawLine(l, t, r, t);  // 上
    ctx.drawLine(r, t, r, b);  // 右
    ctx.drawLine(r, b, l, b);  // 下
    ctx.drawLine(l, b, l, t);  // 左
}
```

`DrawingContext::drawLine()` 检测到非实线笔触自动路由到 `drawDashedLine()`，按标准 Dash 图案（`{4w, 2w}` = `{6, 3}` 像素）渲染。

### 7.2 手柄方块

```cpp
void HandleControl::drawHandle(const SRect& rect, HandleType type, bool active) {
    DrawingContext ctx(getRenderDevice());

    // 填充（白底，拖拽中变蓝）
    ctx.setPen(SPen(active ? m_activeFill : m_handleFill, 0));
    ctx.setBrush(SBrush(active ? m_activeFill : m_handleFill));
    ctx.drawRect(rect, true);

    // 边框（蓝色）
    ctx.setPen(SPen(m_handleBorder, 1.0f));
    ctx.setBrush(SBrush::NoBrush);
    ctx.drawRect(rect, false);
}
```

### 7.3 中心十字移动手柄

与方块风格一致：蓝边白底十字。蓝线 5px 宽、两端比白线各多 1px，确保白色被蓝色完全包围：

```cpp
void HandleControl::drawMoveHandle(const SRect& targetScreen) {
    float cx = targetScreen.left + targetScreen.width  / 2;
    float cy = targetScreen.top  + targetScreen.height / 2;
    float s = m_handleSize;

    DrawingContext ctx(getRenderDevice());

    // 蓝色边框（稍长，完全包裹白色）
    ctx.setPen(SPen(m_handleBorder, 5.0f));
    ctx.drawLine(cx,     cy - s - 1, cx,     cy + s + 1);
    ctx.drawLine(cx - s - 1, cy,     cx + s + 1, cy);

    // 白色填充
    ctx.setPen(SPen(m_handleFill, 3.0f));
    ctx.drawLine(cx,     cy - s, cx,     cy + s);
    ctx.drawLine(cx - s, cy,     cx + s, cy);
}
```

### 7.4 绘制顺序

```
HandleControl::draw()
1. 检测目标存活
2. 校正自身位置（setRect 跟随目标）
3. 更新手柄区域缓存（updateHandleAreas）——供 hitTest 使用
4. 绘制虚线选择框
5. 绘制 8 个手柄方块（跳过 Move 类型，由下步单独绘制）
6. 绘制中心十字移动手柄
```

---

## 8. CMake 集成

```cmake
# 在 CMakeLists.txt 的 CORE_SOURCES 中添加
set(CORE_SOURCES
    ...
    src/HandleControl.cpp
    ...
)

# 头文件自动包含（已有 include/ 目录）
```

---

## 9. 测试计划

新建 `test/test_handlecontrol.cpp`，依赖 `test_button.cpp` 基础框架。

| # | 测试用例 | 验证内容 |
|---|---------|---------|
| 1 | 基本附加 | 附加到 Button，8 个手柄 + 选择框 + 移动手柄正确显示 |
| 2 | SE 缩放 | 拖拽 SE 手柄，目标宽度和高度同步增加 |
| 3 | NW 缩放 | 拖拽 NW 手柄，位置偏移 + 宽高减小（对角固定） |
| 4 | E 边缩放 | 拖拽 E 手柄，仅宽度变化，高度和 Y 不变 |
| 5 | S 边缩放 | 拖拽 S 手柄，仅高度变化，宽度和 X 不变 |
| 6 | 移动 | 拖拽中心移动手柄，X/Y 变化，宽高不变 |
| 7 | 最小尺寸 | 缩放到小于 20×20 时被钳位 |
| 8 | 光标反馈 | 悬停在不同手柄上光标正确切换 |
| 9 | 事件穿透 | 点击目标控件本身（非手柄区域），事件正常传递 |
| 10 | 目标删除 | 删除目标控件后 HandleControl 自动隐藏 |
| 11 | 缩放容器 | 在 Scaled Panel 内的控件，缩放操作坐标正确 |
| 12 | WinFrame 内操作 | 操作 WinFrame 子 Panel 内的控件，手柄叠加正常 |

---

## 10. 实施阶段

| 阶段 | 内容 | 文件 | 预估行数 |
|------|------|------|---------|
| P1 — 核心 | 类声明 + API + SE 缩放 + 移动 + 目标跟随 | `HandleControl.h` + `.cpp` | ~250 |
| P2 — 完整 | 8 方向缩放 + 最小尺寸 + 虚线选择框 | `.cpp` | ~150 |
| P3 — 视觉 | 手柄绘制 + 光标反馈 + 高亮 + 分离 | `.cpp` | ~100 |
| P4 — 集成 | CMake + 测试 + 构建验证 | `CMakeLists.txt` + `test_handlecontrol.cpp` | ~200 |
| **合计** | | | **~700** |

---

## 11. 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| 坐标缩放 bug 导致错位 | 高 | 继承 WinFrame 已验证的坐标转换公式 |
| 父容器新增子控件导致 Z-order 丢失 | 高 | `addControl` 中 `stabilizeTopmostChildren()` 自动保持 |
| 父容器 reflow 覆盖手柄位置 | 中 | `draw()` 中每帧主动校正自身 rect |
| 手柄方块超出父容器裁剪区域 | 低 | 可考虑设置 `m_isTransparent = false` 禁用裁剪（默认 Panel 行为） |
