# ScrollBar 滚动条设计文档

## 1. 概述

ScrollBar（滚动条）是一种用于控制内容滚动位置的 UI 控件，支持垂直和水平两种方向。

## 2. 功能规格

### 2.1 核心功能

- **方向支持**：支持垂直和水平两种方向
- **范围控制**：支持设置最小值、最大值、页大小、步长
- **拖拽交互**：支持鼠标拖拽滑块
- **点击交互**：支持点击轨道快速滚动
- **滑块计算**：根据值和范围自动计算滑块位置和大小

### 2.2 方向定义

```cpp
enum class ScrollBarOrientation {
    Vertical,   // 垂直滚动条
    Horizontal  // 水平滚动条
};
```

## 3. 接口设计

### 3.1 ScrollBar 类

```cpp
class ScrollBar: public ControlImpl {
    friend class ScrollBarBuilder;
public:
    using OnPositionChangedHandler = std::function<void (shared_ptr<ScrollBar>, float, float, float, float)>;
    
private:
    ScrollBarOrientation m_orientation;
    float m_minValue;
    float m_maxValue;
    float m_value;
    float m_pageSize;
    float m_stepSize;
    float m_thickness;
    float m_minThumbLength;
    
    bool m_thumbHovered;
    bool m_thumbPressed;
    bool m_dragging;
    float m_dragOffset;
    
    SRect m_thumbRect;
    SRect m_trackRect;
    
    OnPositionChangedHandler m_onPositionChanged;
    
private:
    void calculateThumbRect();
    void calculateTrackRect();
    float valueToPosition(float value) const;
    float positionToValue(float position) const;
    bool isPointInThumb(float x, float y);
    bool isPointInTrack(float x, float y);
    void notifyPositionChanged(float oldValue = 0);
    
public:
    ScrollBar(Control *parent, SRect rect, ScrollBarOrientation orientation = ScrollBarOrientation::Vertical,
               float xScale = 1.0f, float yScale = 1.0f);
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    
    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;
    
    void setValue(float value);
    float getValue() const;
    void setRange(float minValue, float maxValue);
    void setPageSize(float pageSize);
    void setStepSize(float stepSize);
    void setOrientation(ScrollBarOrientation orientation);
    void setOnPositionChanged(OnPositionChangedHandler handler);
    
    float getThickness() const { return m_thickness; }
    void setThickness(float thickness);
    
    float getMinValue() const { return m_minValue; }
    float getMaxValue() const { return m_maxValue; }
    float getPageSize() const { return m_pageSize; }
    ScrollBarOrientation getOrientation() const { return m_orientation; }
    bool isDragging() const { return m_dragging; }
    bool shouldShow() const;
};
```

### 3.2 ScrollBarBuilder 类

```cpp
class ScrollBarBuilder {
private:
    shared_ptr<ScrollBar> m_scrollBar;
public:
    ScrollBarBuilder(Control *parent, SRect rect, 
                     ScrollBarOrientation orientation = ScrollBarOrientation::Vertical,
                     float xScale = 1.0f, float yScale = 1.0f);
    
    ScrollBarBuilder& setBackgroundStateColor(StateColor stateColor);
    ScrollBarBuilder& setBorderStateColor(StateColor stateColor);
    ScrollBarBuilder& setValue(float value);
    ScrollBarBuilder& setRange(float minValue, float maxValue);
    ScrollBarBuilder& setPageSize(float pageSize);
    ScrollBarBuilder& setStepSize(float stepSize);
    ScrollBarBuilder& setThickness(float thickness);
    ScrollBarBuilder& setOnPositionChanged(ScrollBar::OnPositionChangedHandler handler);
    ScrollBarBuilder& setId(int id);
    
    shared_ptr<ScrollBar> build(void);
};
```

## 4. 实现细节

### 4.1 滑块位置计算

根据当前值计算滑块位置：

```cpp
float ScrollBar::valueToPosition(float value) const {
    if (m_maxValue == m_minValue) return 0;
    
    float trackLength = (m_orientation == ScrollBarOrientation::Vertical) 
        ? m_trackRect.height - m_thumbRect.height
        : m_trackRect.width - m_thumbRect.width;
    
    return (value - m_minValue) / (m_maxValue - m_minValue) * trackLength;
}
```

### 4.2 滑块大小计算

根据页大小和总范围计算滑块大小：

```cpp
void ScrollBar::calculateThumbRect() {
    float totalRange = m_maxValue - m_minValue;
    if (totalRange <= 0) {
        m_thumbRect = m_trackRect;
        return;
    }
    
    float thumbLength;
    if (m_orientation == ScrollBarOrientation::Vertical) {
        float ratio = m_pageSize / totalRange;
        thumbLength = std::max(m_minThumbLength, m_trackRect.height * ratio);
    } else {
        float ratio = m_pageSize / totalRange;
        thumbLength = std::max(m_minThumbLength, m_trackRect.width * ratio);
    }
    
    // 计算滑块位置...
}
```

### 4.3 拖拽交互

使用新的 union-based Event API 处理鼠标交互：

```cpp
bool ScrollBar::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;
    
    SRect drawRect = getDrawRect();
    float scale = getScaleXX();
    
    // MOUSE_DOWN: 启动拖拽或点击轨道翻页
    if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        if (isPointInThumb(event->mouseButton.x, event->mouseButton.y)) {
            m_dragging = true;
            m_thumbPressed = true;
            float localX = (event->mouseButton.x - drawRect.left) / scale;
            float localY = (event->mouseButton.y - drawRect.top) / scale;
            if (m_orientation == ScrollBarOrientation::Vertical) {
                m_dragOffset = localY - m_thumbRect.top;
            } else {
                m_dragOffset = localX - m_thumbRect.left;
            }
            return true;
        }
        
        if (isPointInTrack(event->mouseButton.x, event->mouseButton.y)) {
            // 点击轨道：翻一页
            float localX = (event->mouseButton.x - drawRect.left) / scale;
            float localY = (event->mouseButton.y - drawRect.top) / scale;
            float clickPos = (m_orientation == ScrollBarOrientation::Vertical) ? localY : localX;
            float thumbPos = (m_orientation == ScrollBarOrientation::Vertical)
                ? m_thumbRect.top : m_thumbRect.left;
            float newValue = (clickPos < thumbPos)
                ? m_value - m_pageSize
                : m_value + m_pageSize;
            setValue(newValue);
            return true;
        }
    }
    
    // MOUSE_UP: 结束拖拽
    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        m_dragging = false;
        m_thumbPressed = false;
    }
    
    // MOUSE_MOVE: 拖拽中更新滑块位置
    if (event->m_type == EventType::MouseMove) {
        if (!m_dragging) return false;
        float localX = (event->mousePos.x - drawRect.left) / scale;
        float localY = (event->mousePos.y - drawRect.top) / scale;
        
        float trackLength = (m_orientation == ScrollBarOrientation::Vertical)
            ? m_trackRect.height : m_trackRect.width;
        float thumbLength = (m_orientation == ScrollBarOrientation::Vertical)
            ? m_thumbRect.height : m_thumbRect.width;
        float thumbTravel = trackLength - thumbLength;
        
        float newPos = (m_orientation == ScrollBarOrientation::Vertical)
            ? localY - m_dragOffset
            : localX - m_dragOffset;
        newPos = std::max(0.0f, std::min(newPos, thumbTravel));
        
        float newValue = positionToValue(newPos);
        setValue(newValue);
        return true;
    }
    
    return false;
}
```

关键变更说明：
- 使用 `event->m_type == EventType::MouseDown/Up/Move` 替代旧的 `EventQueue::isPositionEvent(event->m_eventName)` 判断
- `event->mouseButton.x/y` 和 `event->mouseButton.button` 获取鼠标位置和按键（MouseDown/Up）
- `event->mousePos.x/y` 获取鼠标位置（MouseMove）
- 不再使用 `std::any_cast<shared_ptr<SPoint>>` 和 `try/catch` 异常处理
- 不再使用 `EventName::MOUSE_LBUTTON_DOWN/MOUSE_MOVING/MOUSE_LBUTTON_UP` 枚举

### 4.4 显示条件

滚动条根据内容和视口大小决定是否显示：

```cpp
bool ScrollBar::shouldShow() const {
    float trackLength = (m_orientation == ScrollBarOrientation::Vertical)
        ? m_trackRect.height : m_trackRect.width;
    float range = m_maxValue - m_minValue;
    if (range <= 0) return false;
    float ratio = m_pageSize / (range + m_pageSize);
    float thumbLength = trackLength * ratio;
    return thumbLength >= m_minThumbLength;
}
```

### 4.5 悬停/离开事件

```cpp
void ScrollBar::onMouseEnter(float x, float y) {
    m_thumbHovered = isPointInThumb(x, y);
}

void ScrollBar::onMouseLeave(float x, float y) {
    m_thumbHovered = false;
}
```

## 5. 常量定义

在 `ConstDef.h` 中定义滚动条相关常量。

## 6. 文件结构

```
UICornerstone/
├── include/
│   └── ScrollBar.h
├── src/
│   └── ScrollBar.cpp
└── CMakeLists.txt
```

## 7. 使用示例

```cpp
auto vScrollBar = ScrollBarBuilder(nullptr, SRect(380, 100, 20, 200), ScrollBarOrientation::Vertical)
    .setRange(0, 1000)
    .setPageSize(200)
    .setValue(0)
    .setOnPositionChanged([](float value, float pageSize, float maxValue) {
        cout << "Scroll position: " << value << endl;
    })
    .build();
BENCH->addControl(vScrollBar);
```
