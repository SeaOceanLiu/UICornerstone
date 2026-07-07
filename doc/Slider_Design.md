# Slider 滑块控件设计文档

## 1. 概述

Slider（滑块）是一种允许用户通过拖动滑块手柄在一段连续范围内选择值的 UI 控件，类似于 HTML 中的 `<input type="range">`。支持水平/垂直方向、可选数值标签（跟随手柄居中）、刻度标记、键盘方向键重复、吸附（Snap-on-Release）等功能。

### 1.1 视觉结构

```
水平方向（默认）：
┌─────────────────────────────────────┐
│         ┌───┐                       │
│         │ 30│                       │  ← 数值标签，跟随手柄居中（无 clamp）
│         └───┘                       │
│  ───────●────────────────────────   │  ← 轨道 (Track) + 手柄 (Thumb)
│  |      |      |      |      |      │  ← 可选刻度标记
│  0      20     40     60     80     │
└─────────────────────────────────────┘

垂直方向：
┌──────────┐
│    0     │
│    │     │
│    │     │
│    ●     │
│  ┌───┐   │
│  │ 50│   │  ← 数值标签，跟随手柄居中
│  └───┘   │
│    │     │
│   100    │
└──────────┘
```

## 2. 功能规格

### 2.1 核心功能

- **方向支持**：水平（默认）和垂直
- **范围可配**：最小值、最大值、步长（step）
- **交互方式**：拖动手柄、点击轨道跳转、方向键（含长按重复）
- **吸附（Snap-on-Release）**：拖动时手柄自由跟随鼠标，松开时自动吸附到最近的有效 step 位置
- **回调通知**：值变化时通过 `onValueChanged` 回调（仅在提交时触发）
- **可选数值标签**：显示当前值文本，跟随手柄居中（可调间距 `m_labelGap`）
- **可选刻度标记**：显示刻度线及文字
- **缩放支持**：支持 xScale / yScale
- **反向模式**：min 在右侧/底部，max 在左侧/顶部

### 2.2 样式定义

```cpp
enum class SliderStyle {
    Horizontal,   // 水平滑块（默认）
    Vertical      // 垂直滑块
};
```

## 3. 类设计

### 3.1 Slider 类

```cpp
class Slider : public ControlImpl {
    friend class SliderBuilder;
public:
    using OnValueChangedHandler = std::function<void(shared_ptr<Slider>, float value)>;

private:
    // ── 范围 ──
    float m_minValue;
    float m_maxValue;
    float m_step;
    float m_value;
    float m_committedValue;       // 最后提交的（已吸附）值，供回调检测

    // ── 样式 ──
    SliderStyle m_style;
    bool        m_reverse;        // true: min在右/底部，max在左/顶部

    // ── 拖拽状态 ──
    bool   m_dragging;
    bool   m_thumbHovered;
    SPoint m_dragStartMouse;

    // ── 焦点 / 键盘 ──
    // m_focused 已迁移到基类 ControlImpl
    bool m_focusWatcherRegistered;

    // ── 视觉配置 ──
    float  m_trackThickness;
    float  m_thumbSize;
    SColor m_trackColor;
    SColor m_trackFillColor;
    SColor m_thumbColor;
    SColor m_thumbBorderColor;
    SColor m_thumbHoverColor;

    // ── 刻度标记 ──
    float  m_tickInterval;
    float  m_tickLength;
    SColor m_tickColor;
    SharedFont m_tickFont;
    shared_ptr<vector<char>> m_tickFontData;
    int    m_tickLabelFontSize;
    vector<void*> m_cachedTickTexts;
    bool   m_tickFontAttempted;

    // ── 数值标签 ──
    bool          m_showValueLabel;
    shared_ptr<Label> m_valueLabel;
    FontName      m_labelFont;
    int           m_labelFontSize;
    SColor        m_labelColor;
    string        m_labelFormat;
    float         m_labelGap;          // 标签与滑块间距（默认 4px）

    // ── 键盘重复 ──
    int    m_repeatKey;
    double m_repeatStartTime;
    double m_repeatNextTime;

    // ── 脏矩形追踪 ──
    SRect m_lastRect;

    // ── 回调 ──
    OnValueChangedHandler m_onValueChanged;

private:
    // ── 坐标工具 ──
    float snapToStep(float value) const;
    float valueToOffset(float value);       // 值 → 轨道偏移（已缩放像素）
    float offsetToValue(float offset);      // 轨道偏移 → 值
    float getContentLength();               // 有效轨道长度（已缩放像素）
    SRect getThumbRect();                   // 手柄在屏幕坐标下的矩形

    // ── 数值标签 ──
    void updateValueLabel();
    void repositionValueLabel();            // 标签跟随手柄居中

    // ── 刻度 ──
    void rebuildTickTexts();
    void destroyCachedTickTexts();
    void ensureTickFont();                  // 懒加载刻度字体

    // ── 键盘重复 ──
    void handleKeyRepeat();                 // 每帧检测长按重复

    // ── 焦点 ──
    void setFocused(bool focused);
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void onFocusLost() override;            // 停止键盘重复

public:
    Slider(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    ~Slider();
    void create(void) override;
    void update(void) override;             // 含 handleKeyRepeat()
    void draw(void) override;               // 含刻度绘制 + afterDraw()
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;      // 含脏矩形检查

    // ── 范围 ──
    void  setRange(float minValue, float maxValue);
    float getMinValue() const;
    float getMaxValue() const;
    void  setStep(float step);
    float getStep() const;
    void  setValue(float value);
    float getValue() const;
    float getPercent() const;

    // ── 样式 ──
    void setStyle(SliderStyle style);
    SliderStyle getStyle() const;
    void setReverse(bool reverse);
    bool getReverse() const;

    // ── 焦点 ──
    bool isFocused() const override;        // 基类 ControlImpl::getFocused()

    // ── 视觉配置 ──
    void setTrackThickness(float thickness);
    void setThumbSize(float size);
    void setTrackColor(SColor color);
    void setTrackFillColor(SColor color);
    void setThumbColor(SColor color);
    void setThumbBorderColor(SColor color);
    void setThumbHoverColor(SColor color);

    // ── 刻度标记 ──
    void setTickInterval(float interval);
    void setTickLength(float length);
    void setTickColor(SColor color);

    // ── 数值标签 ──
    void setShowValueLabel(bool show);
    bool isShowValueLabel() const;
    shared_ptr<Label> getValueLabel() const;
    void setLabelFont(FontName font);
    void setLabelFontSize(int size);
    void setLabelColor(SColor color);
    void setLabelFormat(const string& format);
    void setLabelGap(float gap);            // 标签与滑块间距

    // ── 回调 ──
    void setOnValueChanged(OnValueChangedHandler handler);
};
```

### 3.2 SliderBuilder 类

```cpp
class SliderBuilder {
private:
    shared_ptr<Slider> m_slider;
public:
    SliderBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    SliderBuilder& setRange(float minValue, float maxValue);
    SliderBuilder& setValue(float value);
    SliderBuilder& setStep(float step);
    SliderBuilder& setStyle(SliderStyle style);
    SliderBuilder& setReverse(bool reverse);
    SliderBuilder& setTrackThickness(float thickness);
    SliderBuilder& setThumbSize(float size);
    SliderBuilder& setTrackColor(SColor color);
    SliderBuilder& setTrackFillColor(SColor color);
    SliderBuilder& setThumbColor(SColor color);
    SliderBuilder& setThumbBorderColor(SColor color);
    SliderBuilder& setThumbHoverColor(SColor color);
    SliderBuilder& setTickInterval(float interval);
    SliderBuilder& setTickLength(float length);
    SliderBuilder& setTickColor(SColor color);
    SliderBuilder& setShowValueLabel(bool show);
    SliderBuilder& setLabelFont(FontName font);
    SliderBuilder& setLabelFontSize(int size);
    SliderBuilder& setLabelColor(SColor color);
    SliderBuilder& setLabelFormat(const string& format);
    SliderBuilder& setLabelGap(float gap);
    SliderBuilder& setOnValueChanged(Slider::OnValueChangedHandler handler);
    SliderBuilder& setBackgroundStateColor(StateColor stateColor);
    SliderBuilder& setBorderStateColor(StateColor stateColor);
    SliderBuilder& setId(int id);

    shared_ptr<Slider> build(void);
};
```

## 4. 交互逻辑

### 4.1 核心行为：Snap-on-Release

Slider 采用"自由拖动，松手吸附"策略：

- **拖拽中**（MouseMove + m_dragging）：`m_value` 设为鼠标位置的 raw（未吸附）值，数值标签实时更新，但 `onValueChanged` 不触发。
- **松开鼠标**：`commitValue()` → `snapToStep(m_value)` → 若与 `m_committedValue` 不同则触发 `onValueChanged`。
- **点击轨道**：`setValue(rawValue)` → snap + trig + startDrag。
- **外部调用 setValue()**：直接吸附并触发回调（与松手行为一致）。
- **方向键**：`setValue(m_value ± step)` → snap + trig。

### 4.2 值 ↔ 屏幕偏移

```cpp
// 有效轨道长度（已缩放）：
// contentLen = dr.width - m_thumbSize * getScaleXX()
//            = (m_rect.width - m_thumbSize) * getScaleXX()

// 值 → 轨道偏移（已缩放像素）
float Slider::valueToOffset(float value) {
    float p = (value - m_minValue) / (m_maxValue - m_minValue);
    p = std::clamp(p, 0.0f, 1.0f);
    if (m_reverse) p = 1.0f - p;
    return p * getContentLength();
}

// 轨道偏移 → 值（未吸附原始值）
float Slider::offsetToValue(float offset) {
    float p = offset / getContentLength();
    if (m_reverse) p = 1.0f - p;
    return m_minValue + p * (m_maxValue - m_minValue);
}
```

### 4.3 手柄矩形（屏幕坐标）

```cpp
SRect Slider::getThumbRect() {
    SRect dr = getDrawRect();
    float halfThumb = (m_thumbSize * getScaleXX()) / 2.0f;
    float offset = valueToOffset(m_value);

    if (m_style == SliderStyle::Horizontal) {
        float cx = dr.left + offset + halfThumb;
        float cy = dr.top + dr.height / 2.0f;
        return {cx - halfThumb, cy - halfThumb,
                m_thumbSize * getScaleXX(), m_thumbSize * getScaleYY()};
    } else {
        float cx = dr.left + dr.width / 2.0f;
        float cy = dr.top + offset + halfThumb;
        return {cx - halfThumb, cy - halfThumb,
                m_thumbSize * getScaleXX(), m_thumbSize * getScaleYY()};
    }
}
```

注意 `valueToOffset()` 返回已缩放的偏移量，直接加到 `dr.left` 上即可。

### 4.4 数值标签定位

`repositionValueLabel()` 是核心定位逻辑：

```
1. 从 m_valueLabel->getMargin() 获取 Label 的 Caption Margin（默认 5,5,5,5）
2. 用 measureText() 测量文本实际宽度/高度（已缩放 → 除以 scale → 未缩放）
3. labelW = textW + margin.left + margin.right + 2  （确保不截断）
4. labelH = textH + margin.top + margin.bottom + 2

水平：
   thumbCX_unscaled = (thumbR.left + thumbR.width/2 - dr.left) / scaleXX
   lx = thumbCX_unscaled - margin.left - textW/2      ← 文本中心 = 手柄中心
   ly = -labelH - m_labelGap                           ← 间距可调

垂直：
   thumbCY_unscaled = (thumbR.top + thumbR.height/2 - dr.top) / scaleYY
   ly = thumbCY_unscaled - margin.top - textH/2        ← 文本中心 Y = 手柄中心 Y
   lx = m_rect.width + m_labelGap                      ← 间距可调
```

关键设计决策：**不对 lx/ly 做 clamp**。当手柄处于极值时，标签可能超出 slider 边界，但文本中心始终与手柄中心对齐，两端体验一致。

### 4.5 事件处理

```
Slider::handleEvent(event)
├── MouseMove：
│   ├── m_dragging == true：raw 值跟随鼠标，updateValueLabel()，指针光标
│   └── 悬停检测：thumbR.contains(mouse) → m_thumbHovered + 指针光标
├── MouseUp + m_dragging：commitValue()，恢复光标
├── MouseDown + Left + 在矩形内：
│   ├── 命中手柄 → 开始拖拽
│   └── 点击轨道 → setValue(raw) + 开始拖拽
│   两种都调 setFocused(true)
├── KeyDown + m_focused：
│   ├── Left/Up：m_reverse ? +step : -step
│   ├── Right/Down：m_reverse ? -step : +step
│   ├── Home：m_reverse ? max : min
│   ├── End：m_reverse ? min : max
│   ├── PageUp：↕ pageStep = range * 0.1
│   └── PageDown：↕ pageStep
│   启动键盘重复追踪（m_repeatKey, 350ms 初始延迟, 50ms 重复间隔）
├── KeyUp：停止键盘重复
└── FocusLost：停止键盘重复
```

### 4.6 键盘重复机制

`handleKeyRepeat()` 在 `update()` 中每帧调用，使用 `Platform::GetTicks()` 计时：
- 首次 KeyDown 后 350ms 初始延迟
- 后续每 50ms 重复一次
- 仅 Left/Right/Up/Down/PageUp/PageDown 支持重复（Home/End 单次跳转）
- KeyUp 或 FocusLost 停止重复

### 4.7 焦点管理

Slider 的焦点已从自有 `m_focused` 迁移到基类 `ControlImpl`：

- 构造函数调用 `setFocusable(true)` 注册到 FocusManager
- 鼠标按下（拖拽或点击轨道）时通过基类 `setFocused(true, bool)` 设置焦点
- 保留 `beforeEventHandlingWatcher` 监听全局 MouseDown，检测外部点击释放焦点
- 覆盖 `onFocusLost()` 停止键盘重复并清除 `m_repeatKey`
- `draw()` 末尾调用 `ControlImpl::afterDraw()`，触发 `drawFocusRing()`
- 焦点环：3 层（黑+白+颜色），在聚焦控件周围自动绘制

### 4.8 光标反馈

- 悬停手柄时：`SystemCursorType::Pointer`（手形指针）
- 拖拽中：保持 Pointer
- 离开手柄：`Cursor::getDefault()`
- 每帧 MouseMove 刷新（应对 `WM_SETCURSOR` 复位）

## 5. 绘制

### 5.1 绘制顺序

```
Slider::draw()
├── ControlImpl::beforeDraw()
├── 水平/垂直分支：
│   ├── 轨道背景 (trackColor) ────────── fillRect 完整轨道
│   ├── 刻度线 (tickInterval > 0) ───── drawLine + drawText
│   │   ├── 水平：轨道下方竖线 + 数值标签
│   │   └── 垂直：轨道左侧横线 + 数值标签
│   ├── 轨道填充段 (trackFillColor) ─── 从起点到手柄中心
│   │   ├── 水平：左→thumbCx（reverse: thumbCx→右）
│   │   └── 垂直：上→thumbCy（reverse: thumbCy→下）
│   └── 手柄 (thumbCol + thumbBorderColor) ─── fillRect + drawRect
└── ControlImpl::draw() ─────────────── 绘制子控件（数值标签）
```

### 5.2 刻度标记

- 启用条件：`m_tickInterval > 0`
- 刻度线位置：水平在轨道下方、垂直在轨道左侧
- 刻度标签：使用 `m_labelFont` 字体 + `m_tickLabelFontSize` 大小，通过 `ensureTickFont()` 懒加载
- 刻度文本缓存：`m_cachedTickTexts` 预创建 `TTF_Text*` 避兔每帧 create/destroy
- 缓存失效：`setRange()` 和 `setTickInterval()` 时销毁

### 5.3 脏矩形优化

`setRect()` 添加 dirty-rect check：
```cpp
if (rect == m_lastRect) return;
m_lastRect = rect;
```
避免重复重定位 value label，减少 Label::setRect 调用链。

### 5.4 刻度字体懒加载

`ensureTickFont()` 单次加载，`m_tickFontAttempted` 防止重复尝试：

```cpp
void Slider::ensureTickFont() {
    if (m_tickFont || m_tickFontAttempted || !getResourceProvider()) return;
    m_tickFontAttempted = true;
    // 读取字体文件 → loadFontFromMemory → m_tickFont
}
```

## 6. 默认常量

在 `ConstDef.h/.cpp` 中定义：

```cpp
// 轨道
static const float SLIDER_TRACK_THICKNESS = 4.0f;
static const SColor SLIDER_TRACK_COLOR(60, 60, 60, 255);
static const SColor SLIDER_TRACK_FILL_COLOR(0, 120, 255, 255);

// 手柄
static const float SLIDER_THUMB_SIZE = 16.0f;
static const SColor SLIDER_THUMB_COLOR(255, 255, 255, 255);
static const SColor SLIDER_THUMB_BORDER_COLOR(0, 120, 255, 255);
static const SColor SLIDER_THUMB_HOVER_COLOR(200, 200, 200, 255);

// 数值标签
static const SColor SLIDER_LABEL_COLOR(255, 255, 255, 255);

// 刻度
static const float SLIDER_TICK_INTERVAL = 0.0f;  // 默认关闭
static const float SLIDER_TICK_LENGTH = 8.0f;
static const SColor SLIDER_TICK_COLOR(100, 100, 100, 255);
```

## 7. JSON 布局支持

在 `LayoutParser` 中通过 `"type": "Slider"` 解析：

```json
{
    "type": "Slider",
    "id": "volumeSlider",
    "rect": { "x": 20, "y": 10, "w": 200, "h": 30 },
    "range": { "min": 0, "max": 100 },
    "value": 50,
    "step": 1,
    "style": "Horizontal",
    "reverse": false,
    "track": {
        "thickness": 4,
        "color": { "normal": "#3C3C3CFF" },
        "fillColor": { "normal": "#0078FFFF" }
    },
    "thumb": {
        "size": 16,
        "color": { "normal": "#FFFFFFFF" },
        "borderColor": { "normal": "#0078FFFF" },
        "hoverColor": { "normal": "#C8C8C8FF" }
    },
    "showValueLabel": true,
    "labelFormat": "%.0f",
    "labelGap": 4,
    "tick": {
        "interval": 10,
        "length": 8,
        "color": { "normal": "#646464FF" }
    },
    "events": {
        "onValueChanged": "onVolumeChanged"
    }
}
```

## 8. 优化历史

### 2026-06-24: 初始刻度线 + 刻度标签

- 水平滑块刻度线在轨道下方绘制，带数值标签
- 垂直滑块刻度线在轨道左侧绘制，带数值标签
- 刻度懒加载字体，缓存 TTF_Text*

### 2026-06-30: 5 项性能/体验优化

1. **setRect 脏矩形检查**：避免重复重定位 value label
2. **键盘按键重复**：`handleKeyRepeat()` 在 `update()` 中每帧检测，350ms 初始延迟 + 50ms 重复间隔
3. **Value label 跟随 thumb**：`repositionValueLabel()` 将标签居中于 thumb 上方/右侧
4. **Tick 字体懒加载**：`ensureTickFont()` 单次加载 + `m_tickFontAttempted`
5. **draw() 去重**：水平/垂直分支共用 `ensureTickFont()`

### 2026-06-30: Value label 定位修复 + m_labelGap 参数

- **坐标 bug 修复**：`getThumbRect()` 返回已缩放绝对坐标，`setRect()` 需要未缩放相对坐标——添加坐标转换（`(thumbCenter - dr.left) / scale`）
- **文本宽度计算**：`measureText()` 结果除以 scale 转为未缩放值；考虑 Label Caption Margin 做中心对齐；labelW 足够宽避免截断
- **去掉 lx/ly clamp**：标签可超出 slider 边界，文本中心始终保持与手柄中心对齐，两端体验一致
- **`m_labelGap` 参数**：控制标签与滑块间距（默认 4px），可通过 `setLabelGap()` / Builder / JSON 配置

### 2026-07-01: test_slider 布局间距 + raylib KeyUp 修复

**test_slider.cpp 布局重算**：
- 左列 11 个水平滑块 + 1 状态标签，Y 从 40 到 800
- 为 value label（上方 ~22px）、刻度标签（下方 1x: ~22px, 2x: ~44px）留足间隙
- 右列 4 个垂直滑块 Y=10, h=250，X 间隔 110px，与左列无水平重叠
- 间距公式：`prev_track_bottom + [prev_tick_labels] + 8px + 22px(valLabel) + 4px(labelGap)`

**raylib InputBackend KeyUp 修复**：
- 根因：raylib 使用 `GetKeyPressed()` / `IsKeyDown()`，无对应的释放 API → 从不发送 `KeyUp` 事件
- 在 `Keyboard` phase 的 repeat 检测后添加 `IsKeyUp(m_repeatKey)` 检测
- 新增 `fillKeyUpEvent()` 方法生成 `EventType::KeyUp`
- 修复后 Slider 的 `handleEvent(KeyUp)` 能正常清空 `m_repeatKey`，`handleKeyRepeat()` 停止重复
- 三后端（SDL3/SFML/Raylib）全部编译通过

## 9. 测试计划

### 9.1 标准 C++ 测试（test/test_slider.cpp）

15 个滑块，布局 1920×1080：

- **左列**（X=50, w=250）：11 水平滑块，Y=40~695 逐行递推，间距 = 前轨道底 + 前刻度标签 + 8px间隙 + 22px(val标签) + 4px(labelGap)
- **右列**（4 垂直滑块，Y=10, h=250）：X=580/690/800/910，vTick2x(2x) visual h=500，底部 Y=510，与左列无水平重叠

| # | Y | h | 测试用例 | 验证内容 |
|---|----|----|----------|----------|
| 1 | 40 | 20 | 默认 0-100 范围 + 数值标签 + 回调 | 基础渲染 |
| 2 | 90 | 20 | Step=10, labelFormat 含额外文字 | 步长+格式 |
| 3 | 580(垂直) | 250 | 垂直方向 + 数值标签 | 垂直渲染 |
| 4 | 140 | 20 | 绿色主题 (trackFillColor, thumbBorderColor) | 颜色定制 |
| 5 | 180 | 20 | 负值范围 (-50~50), step=5 | 负值处理 |
| 6 | 230 | 20 | 窄范围 (0~3), step=1 | Snap-on-Release |
| 7 | 285 | 30 | 粗轨道 (12px) + 大手柄 (28px) | 尺寸定制 |
| 8 | 345 | 20 | 0~1 范围, step=0.1 | 浮点步长 |
| 9 | 415 | 20 | 刻度线 1x (interval=10) + 数值标签 | 刻度绘制 |
| H_Tick1x/H_Tick2x | 490/580 | 20 | 1x/2x 缩放刻度对比 | 缩放支持 |
| V_Tick1x/V_Tick2x | 690/800(垂直) | 250 | 1x/2x 垂直刻度对比 | 垂直缩放 |
| H_Reverse/V_Reverse | 695/910(垂直) | 20/250 | 水平/垂直反向模式 | Reverse 逻辑 |
| Status Label | 770 | 30 | 描述文本 | 控件共存 |

### 9.2 C ABI 集成测试

Slider 的 C ABI 用例分别添加到三后端的 fromsource 测试文件。

## 10. 关键实现注意事项

1. **坐标空间一致性**：
   - `m_rect` = 未缩放相对坐标
   - `getDrawRect()` = 已缩放绝对坐标
   - `getThumbRect()` = 已缩放绝对坐标
   - Label `setRect()` = 未缩放相对坐标
   - `measureText()` 返回已缩放像素 → 需 `/ scale` 转为未缩放

2. **Value label 必须后于 create() 调用 updateValueLabel()**：Label 的 font 在 `create()` 中加载，在此之前调用 `measureText()` 会得到空指针。

3. **Value label 不 clamp**：超出 slider 边界时，依赖父级（Bench/Panel）的 viewport clipping 裁剪。

4. **键盘重复生命周期**：由 `handleEvent(KeyDown)` 启动，`handleKeyRepeat(update)` 持续，`handleEvent(KeyUp/FocusLost)` 停止。
   - **raylib 后端依赖**：`InputBackend` 必须在 `Keyboard` phase 中检测 `IsKeyUp()` 并生成 `KeyUp` 事件，否则 Slider 的 `m_repeatKey` 永不清零，`handleKeyRepeat()` 无限重复。

