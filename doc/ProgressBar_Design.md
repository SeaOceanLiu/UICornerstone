# ProgressBar 进度条设计文档

## 1. 概述

ProgressBar（进度条）是一种用于显示任务进度的 UI 控件，支持水平/垂直方向、平滑动画、可选文本显示等功能。

## 2. 功能规格

### 2.1 核心功能

- **方向支持**：支持水平（从左到右）和垂直（从上到下）两种方向
- **进度显示**：三种模式可选（无文本、百分比、自定义文本）
- **平滑动画**：进度变化时自动平滑过渡
- **颜色自定义**：支持进度条颜色、背景颜色、边框颜色、文字颜色设置
- **范围支持**：最小值和最大值可配置
- **文字属性**：支持字体、字号、对齐方式设置
- **缩放支持**：支持 xScale 和 yScale 缩放

### 2.2 样式定义

```cpp
enum class ProgressBarStyle {
    Horizontal,    // 水平进度条（默认）
    Vertical        // 垂直进度条
};

enum class ProgressBarTextMode {
    None,           // 不显示文本
    Percent,        // 显示百分比（如 "50%"）
    Custom          // 显示自定义文本
};
```

## 3. 接口设计

### 3.1 ProgressBar 类

```cpp
class ProgressBar : public ControlImpl {
    friend class ProgressBarBuilder;
public:
    using OnValueChangedHandler = std::function<void (shared_ptr<ProgressBar>, float, float)>;
    
private:
    float m_minValue;
    float m_maxValue;
    float m_value;
    float m_animatedValue;
    
    ProgressBarStyle m_style;
    ProgressBarTextMode m_textMode;
    string m_customText;
    
    SColor m_progressColor;
    SColor m_backgroundColor;
    SColor m_textColor;
    float m_animationSpeed;
    
    FontName m_fontName;
    int m_fontSize;
    AlignmentMode m_alignmentMode;
    
    shared_ptr<Label> m_textLabel;
    OnValueChangedHandler m_onValueChanged;
    
private:
    void createTextLabel();
    void updateTextLabel();
    
public:
    ProgressBar(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    void update(void) override;
    shared_ptr<Label> getTextLabel(void) const;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    
    void setValue(float value);
    float getValue() const;
    float getPercent() const;
    
    void setRange(float minValue, float maxValue);
    void setStyle(ProgressBarStyle style);
    ProgressBarStyle getStyle() const { return m_style; }
    void setTextMode(ProgressBarTextMode mode);
    ProgressBarTextMode getTextMode() const { return m_textMode; }
    void setCustomText(string text);
    string getCustomText() const { return m_customText; }
    
    void setProgressColor(SColor color);
    SColor getProgressColor() const { return m_progressColor; }
    void setBackgroundColor(SColor color);
    SColor getBackgroundColor() const { return m_backgroundColor; }
    void setTextColor(SColor color);
    SColor getTextColor() const { return m_textColor; }
    void setAnimationSpeed(float speed);
    float getAnimationSpeed() const { return m_animationSpeed; }
    
    void setFont(FontName fontName);
    void setFontSize(int fontSize);
    void setAlignmentMode(AlignmentMode mode);
    
    void setOnValueChanged(OnValueChangedHandler handler);
};
```

### 3.2 ProgressBarBuilder 类

```cpp
class ProgressBarBuilder {
private:
    shared_ptr<ProgressBar> m_progressBar;
public:
    ProgressBarBuilder(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    
    ProgressBarBuilder& setBackgroundStateColor(StateColor stateColor);
    ProgressBarBuilder& setBorderStateColor(StateColor stateColor);
    ProgressBarBuilder& setValue(float value);
    ProgressBarBuilder& setRange(float minValue, float maxValue);
    ProgressBarBuilder& setStyle(ProgressBarStyle style);
    ProgressBarBuilder& setTextMode(ProgressBarTextMode mode);
    ProgressBarBuilder& setCustomText(string text);
    ProgressBarBuilder& setProgressColor(SColor color);
    ProgressBarBuilder& setBackgroundColor(SColor color);
    ProgressBarBuilder& setTextColor(SColor color);
    ProgressBarBuilder& setAnimationSpeed(float speed);
    ProgressBarBuilder& setFont(FontName fontName);
    ProgressBarBuilder& setFontSize(int fontSize);
    ProgressBarBuilder& setAlignmentMode(AlignmentMode mode);
    ProgressBarBuilder& setOnValueChanged(ProgressBar::OnValueChangedHandler handler);
    ProgressBarBuilder& setId(int id);
    
    shared_ptr<ProgressBar> build(void);
};
```

## 4. 常量定义

在 `ConstDef.h` 中添加：

```cpp
// 进度条相关常量
static const float PROGRESSBAR_DEFAULT_HEIGHT;
static const float PROGRESSBAR_MIN_HEIGHT;
static const SColor PROGRESSBAR_DEFAULT_PROGRESS_COLOR;
static const SColor PROGRESSBAR_DEFAULT_BACKGROUND_COLOR;
static const float PROGRESSBAR_DEFAULT_ANIMATION_SPEED;
```

## 5. 实现细节

### 5.1 动画逻辑

```cpp
void ProgressBar::update(void) {
    if (std::abs(m_animatedValue - m_value) > 0.1f) {
        float diff = m_value - m_animatedValue;
        m_animatedValue += diff * m_animationSpeed;
    } else {
        m_animatedValue = m_value;
    }
    updateTextLabel();
}
```

### 5.2 百分比计算

```cpp
float ProgressBar::getPercent() const {
    if (m_maxValue == m_minValue) return 0;
    float percent = (m_animatedValue - m_minValue) / (m_maxValue - m_minValue) * 100.0f;
    return std::max(0.0f, std::min(100.0f, percent));
}
```

### 5.3 文字更新逻辑

```cpp
void ProgressBar::updateTextLabel() {
    if (!m_textLabel || m_textMode == ProgressBarTextMode::None) return;
    
    string text;
    if (m_textMode == ProgressBarTextMode::Percent) {
        int percent = (int)(getPercent() + 0.5f);
        text = std::to_string(percent) + "%";
    } else if (m_textMode == ProgressBarTextMode::Custom) {
        text = m_customText;
    }
    m_textLabel->setCaption(text);
}
```

## 6. 测试用例

### 6.1 基本功能测试

```cpp
// 测试1：基本进度条
void testBasicProgressBar() {
    auto progress1 = ProgressBarBuilder(nullptr, SRect(50, 50, 300, 20))
        .setValue(50)
        .build();
    BENCH->addControl(progress1);
}
```

### 6.2 2x 缩放测试

```cpp
// 测试2：2x 缩放进度条
void testScaledProgressBar() {
    auto progress2 = ProgressBarBuilder(nullptr, SRect(50, 80, 300, 20), 2.0f, 2.0f)
        .setValue(75)
        .setProgressColor(SColor(0, 255, 0, 255))
        .build();
    BENCH->addControl(progress2);
}
```

### 6.3 百分比显示测试

```cpp
// 测试3：带百分比显示
void testPercentProgressBar() {
    auto progress3 = ProgressBarBuilder(nullptr, SRect(50, 130, 300, 30))
        .setValue(75)
        .setTextMode(ProgressBarTextMode::Percent)
        .setProgressColor(SColor(0, 200, 255, 255))
        .build();
    BENCH->addControl(progress3);
}
```

### 6.4 自定义文本测试

```cpp
// 测试4：自定义文本显示
void testCustomTextProgressBar() {
    auto progress4 = ProgressBarBuilder(nullptr, SRect(50, 170, 300, 30))
        .setValue(60)
        .setTextMode(ProgressBarTextMode::Custom)
        .setCustomText("Loading: 60%")
        .setProgressColor(SColor(255, 200, 0, 255))
        .setBackgroundColor(SColor(50, 50, 50, 255))
        .build();
    BENCH->addControl(progress4);
}
```

### 6.5 自定义文字属性测试

```cpp
// 测试5：自定义文字属性
void testCustomTextProperties() {
    auto progress5 = ProgressBarBuilder(nullptr, SRect(50, 220, 300, 30))
        .setValue(80)
        .setTextMode(ProgressBarTextMode::Percent)
        .setFont(FontName::HarmonyOS_Sans_SC_Regular)
        .setFontSize(16)
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setTextColor(SColor(255, 255, 255, 255))
        .setProgressColor(SColor(0, 255, 128, 255))
        .build();
    BENCH->addControl(progress5);
}
```

### 6.6 动画测试

```cpp
// 测试6：动画效果
void testAnimatedProgressBar() {
    auto progress6 = ProgressBarBuilder(nullptr, SRect(50, 270, 300, 25))
        .setRange(0, 100)
        .setValue(0)
        .setAnimationSpeed(0.05f)
        .setProgressColor(SColor(100, 149, 237, 255))
        .build();
    BENCH->addControl(progress6);
    
    progress6->setValue(100);
}
```

### 6.7 完整自定义测试（2x + 自定义文本 + 自定义属性）

```cpp
// 测试7：完整自定义测试
void testFullCustomProgressBar() {
    auto progress7 = ProgressBarBuilder(nullptr, SRect(50, 320, 400, 40), 2.0f, 2.0f)
        .setValue(45)
        .setTextMode(ProgressBarTextMode::Custom)
        .setCustomText("下载中 45/100")
        .setFont(FontName::HarmonyOS_Sans_SC_Regular)
        .setFontSize(20)
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setProgressColor(SColor(255, 100, 100, 255))
        .setBackgroundColor(SColor(80, 80, 80, 255))
        .setTextColor(SColor(255, 255, 255, 255))
        .build();
    BENCH->addControl(progress7);
}
```

## 7. 文件结构

```
UICornerstone/
├── include/
│   └── ProgressBar.h
├── src/
│   └── ProgressBar.cpp
└── CMakeLists.txt (更新)
```

## 8. 实现顺序

1. 在 `ConstDef.h` 添加常量声明
2. 在 `ConstDef.cpp` 添加常量定义
3. 创建 `UICornerstone/include/ProgressBar.h`
4. 创建 `UICornerstone/src/ProgressBar.cpp`
5. 更新 `UICornerstone/CMakeLists.txt`
6. 创建测试用例 `UICornerstone/test/test_progressbar.cpp`
7. 编译测试
