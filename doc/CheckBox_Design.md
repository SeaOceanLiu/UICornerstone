# CheckBox 复选框设计文档

## 1. 概述

复选框（CheckBox）是一种常见的 UI 控件，用于表示布尔状态或从多个选项中选择多个选项。本设计文档描述了复选框的功能规格、接口定义和实现细节。

## 2. 设计规则

> 参见 [AGENTS.md](../AGENTS.md) 中的设计规则章节。

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 生成字体时，使用缩放后的字体大小
   - 绘制时才对缩放进行处理

2. **控件生命周期**：
   - `create()`: 初始创建控件，创建内部 Label 并设置布局
   - `recreate()`: 重新创建控件，用于属性变化时重新布局

## 3. 功能规格

### 3.1 核心功能

- **状态切换**：支持两种模式：
  - 三态模式（默认）：未选中 → 选中 → 三态 → 未选中
  - 两态模式：仅在未选中 ↔ 选中之间切换
- **样式支持**：支持三种视觉样式（Classic、Cross、Circle）
- **布局支持**：支持文字在左侧或右侧两种布局
- **垂直对齐**：支持三种垂直对齐方式（居中、顶部、底部）
- **尺寸比例**：复选框尺寸基于字体大小的比例计算
- **颜色自定义**：支持自定义勾选符号、叉号、三态、边框颜色
- **Label 暴露**：通过 `getCaption()` 方法暴露内部 Label，支持直接配置

### 3.2 状态定义

```cpp
enum class CheckState {
    Unchecked,     // 未选中
    Checked,       // 选中
    Indeterminate  // 三态（部分选中）
};
```

### 3.3 样式定义

```cpp
enum class CheckBoxStyle {
    Classic,   // 方框 + 勾选符号
    Cross,     // 方框 + X 符号
    Circle     // 圆形 + 勾选符号
};
```

### 3.4 布局定义

```cpp
enum class CheckBoxLayout {
    TextRight,  // 文字在右（默认）
    TextLeft    // 文字在左
};

enum class CheckBoxVerticalAlign {
    Center,      // 垂直居中（默认）
    Top,         // 与第一行顶部对齐
    Bottom       // 与最后一行底部对齐
};
```

## 4. 尺寸计算规则

### 4.1 复选框尺寸

复选框尺寸根据字体大小和缩放比例计算：

```
复选框尺寸 = 行高 × 尺寸倍率
```

- **默认倍率**：`1.0`（复选框尺寸等于行高）
- **用户可调整**：通过 `setSizeRatio(float ratio)` 设置倍率
- 复选框为正方形，宽度等于高度

### 4.2 布局计算

#### TextRight 布局
```
+------------------------------------------+
| [复选框] [文字区域]                      |
+------------------------------------------+
```

- 复选框区域：左侧，尺寸为 `m_caption->getLineHeight() * m_sizeRatio`
- 间距：`CHECKBOX_BOX_MARGIN`（Margin 类型）
- 文字区域：剩余宽度，Label 使用 `AM_MID_LEFT` 对齐

#### TextLeft 布局
```
+------------------------------------------+
| [文字区域] [复选框]                      |
+------------------------------------------+
```

- 复选框区域：右侧
- 文字区域：剩余宽度，Label 使用 `AM_MID_RIGHT` 对齐

### 4.3 多行文字垂直对齐

当文字有多行时，复选框的垂直位置根据 `m_verticalAlign` 确定：

| 对齐方式 | 说明 |
|---------|------|
| Center | 复选框垂直居中于整体文字区域 |
| Top | 复选框与第一行顶部对齐 |
| Bottom | 复选框与最后一行底部对齐 |

### 4.4 内部 Label 交互

CheckBox 使用内部 Label 作为 Caption，通过以下方式交互：

1. **创建时**：在 `createCaption()` 中创建 Label，设置回调监听属性变化
2. **属性变化监听**：Label 的属性变化时，通过回调触发 CheckBox 重新布局

```cpp
m_caption->setOnPropertyChanged([this](shared_ptr<Label> label){
    setBoxSize();
    adjustSpaceAssignment();
    adjustBoxVerticalAlign();
})
```

3. **直接访问**：通过 `getCaption()` 方法获取内部 Label，可直接配置字体、大小、对齐等

## 5. 接口设计

### 5.1 CheckBox 类

```cpp
class CheckBox : public ControlImpl {
    friend class CheckBoxBuilder;
public:
    using OnCheckChangedHandler = std::function<void (shared_ptr<CheckBox>, CheckState)>;

private:
    CheckState m_checkState;
    CheckBoxStyle m_style;
    CheckBoxLayout m_layout;
    CheckBoxVerticalAlign m_verticalAlign;

    shared_ptr<Label> m_caption;
    OnCheckChangedHandler m_onCheckChanged;

    float m_sizeRatio;
    bool m_triStateEnabled;

    SRect m_boxRect;            // 复选框区域（未缩放）
    Margin m_boxMargin;         // 复选框内边距

    StateColor m_checkStateColor;
    StateColor m_crossStateColor;
    StateColor m_indeterminateStateColor;
    StateColor m_boxBorderStateColor;

protected:
    void recreate(void) override;
public:
    CheckBox(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    void releaseCaption(void);
    void createCaption(void);
    Label& getCaption(void) const;    // 暴露内部 Label
    void create(void) override;

    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    void setCheckState(CheckState state);
    CheckState getCheckState() const;

    void setStyle(CheckBoxStyle style);
    CheckBoxStyle getStyle() const;

    void setLayout(CheckBoxLayout layout);
    CheckBoxLayout getLayout() const;

    void setVerticalAlign(CheckBoxVerticalAlign align);
    CheckBoxVerticalAlign getVerticalAlign() const;

    void setSizeRatio(float ratio);
    float getSizeRatio() const;

    void setTriStateEnabled(bool enabled);
    bool isTriStateEnabled() const;

    void setOnCheckChanged(OnCheckChangedHandler handler);

    void setCheckColor(SDL_Color color);
    SDL_Color getCheckColor();
    void setCrossColor(SDL_Color color);
    SDL_Color getCrossColor();
    void setIndeterminateColor(SDL_Color color);
    SDL_Color getIndeterminateColor();

    void setBoxBorderColor(SDL_Color color);
    SDL_Color getBoxBorderColor();

private:
    void setBoxSize(void);
    void adjustSpaceAssignment(void);
    void adjustBoxVerticalAlign(void);

    SRect getBoxDrawRect(); // for drawing
    void drawCheckBoxFrame();
    void drawCheckMark();
    void drawCrossMark();
    void drawIndeterminateMark();
};
```

### 5.2 CheckBoxBuilder 类

```cpp
class CheckBoxBuilder {
private:
    shared_ptr<CheckBox> m_checkBox;

public:
    CheckBoxBuilder(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    CheckBoxBuilder& setStyle(CheckBoxStyle style);
    CheckBoxBuilder& setLayout(CheckBoxLayout layout);
    CheckBoxBuilder& setVerticalAlign(CheckBoxVerticalAlign align);
    CheckBoxBuilder& setCheckState(CheckState state);
    CheckBoxBuilder& setSizeRatio(float ratio);
    CheckBoxBuilder& setCaptionText(string caption);    // 改为设置内部 Label 文本
    CheckBoxBuilder& setCaptionSize(float size);        // 实际上是设置内部 Label 字体大小
    CheckBoxBuilder& setTriStateEnabled(bool enabled);
    CheckBoxBuilder& setOnCheckChanged(CheckBox::OnCheckChangedHandler handler);
    CheckBoxBuilder& setCheckColor(SDL_Color color);
    CheckBoxBuilder& setCrossColor(SDL_Color color);
    CheckBoxBuilder& setIndeterminateColor(SDL_Color color);
    CheckBoxBuilder& setBoxBorderColor(SDL_Color color);
    CheckBoxBuilder& setBackgroundStateColor(StateColor stateColor);
    CheckBoxBuilder& setBorderStateColor(StateColor stateColor);
    CheckBoxBuilder& setTextStateColor(StateColor stateColor);
    CheckBoxBuilder& setId(int id);
    CheckBoxBuilder& setEnable(bool enable);

    shared_ptr<CheckBox> build(void);
};
```

## 6. 常量定义

在 `ConstDef.h` 中添加：

```cpp
// 复选框相关常量
static const float BOX_PEN_WIDTH;                 // 边框线宽（2.0f）
static const float MARK_PEN_WIDTH;                // 标记线宽（2.5f）
static const Margin CHECKBOX_MARGIN;             // 复选框整体外边距
static const float CHECKBOX_SIZE_RATIO;           // 默认尺寸倍率（1.0）
static const Margin CHECKBOX_BOX_MARGIN;          // 复选框与文字的间距
static const float CHECKBOX_DEFAULT_CAPTION_SIZE; // 文字默认大小
static const SDL_Color CHECKBOX_CHECK_COLOR;      // 勾选符号颜色
static const SDL_Color CHECKBOX_CROSS_COLOR;      // X 符号颜色
static const SDL_Color CHECKBOX_INDETERMINATE_COLOR; // 三态颜色
```

### 常量变更说明

| 常量 | 旧值 | 新值 | 说明 |
|------|------|------|------|
| CHECKBOX_SIZE_RATIO | 0.8f | 1.0f | 复选框尺寸等于行高 |
| CHECKBOX_BOX_MARGIN | float (4.0f) | Margin | 支持四边不同间距 |
| BOX_PEN_WIDTH | - | 2.0f | 新增：边框线宽 |
| MARK_PEN_WIDTH | - | 2.5f | 新增：标记线宽 |

## 7. 事件处理

### 7.1 状态切换逻辑

点击复选框时，状态按以下顺序循环切换（默认三态模式）：

```
Unchecked → Checked → Indeterminate → Unchecked
```

如果禁用三态模式（`setTriStateEnabled(false)`）：
```
Unchecked → Checked → Unchecked
```

### 7.2 事件处理流程

```cpp
bool CheckBox::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;

    if (EventQueue::isPositionEvent(event->m_eventName)) {
        if (!event->m_eventParam.has_value()) return false;

        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;

            if (getDrawRect().contains(pos->x, pos->y)) {
                switch (event->m_eventName) {
                    case EventName::MOUSE_LBUTTON_UP:
                        // 循环切换状态
                        switch (m_checkState) {
                            case CheckState::Unchecked:
                                setCheckState(CheckState::Checked);
                                break;
                            case CheckState::Checked:
                                if (m_triStateEnabled) {
                                    setCheckState(CheckState::Indeterminate);
                                } else {
                                    setCheckState(CheckState::Unchecked);
                                }
                                break;
                            case CheckState::Indeterminate:
                                setCheckState(CheckState::Unchecked);
                                break;
                        }
                        if (m_onCheckChanged) {
                            m_onCheckChanged(dynamic_pointer_cast<CheckBox>(getThis()), m_checkState);
                        }
                        return true;

                    case EventName::MOUSE_MOVING:
                        if (getState() != ControlState::Hover) {
                            setState(ControlState::Hover);
                        }
                        return true;
                }
            } else {
                if (getState() == ControlState::Hover) {
                    setState(ControlState::Normal);
                }
            }
        } catch (...) {
            return false;
        }
    }

    return ControlImpl::handleEvent(event);
}
```

## 8. 实现细节

### 8.1 生命周期管理

```cpp
void CheckBox::create(void) {
    if (m_isCreated) return;

    createCaption();         // 创建内部 Label
    setBoxSize();            // 根据行高设置复选框尺寸
    adjustSpaceAssignment(); // 设置复选框和 Label 的位置
    adjustBoxVerticalAlign(); // 调整垂直对齐

    addControl(m_caption);
    ControlImpl::create();
}

void CheckBox::recreate(void) {
    if(!m_isCreated) return;

    releaseCaption();  // 释放内部 Label

    m_isCreated = false;
    create();          // 重新创建
}
```

### 8.2 布局实现

#### 设置复选框尺寸

```cpp
void CheckBox::setBoxSize(void) {
    m_boxRect.left = 0;
    m_boxRect.top = 0;
    m_boxRect.width = m_caption->getLineHeight() * m_sizeRatio;
    m_boxRect.height = m_boxRect.width;
}
```

#### 分配空间

```cpp
void CheckBox::adjustSpaceAssignment(void) {
    SRect marginRect = getMarginedRect();

    switch(m_layout) {
        case CheckBoxLayout::TextRight:
            m_boxRect.left = marginRect.left;
            m_caption->setRect({m_boxRect.right(), marginRect.top,
                    marginRect.width - m_boxRect.width, marginRect.height});
            m_caption->setAlignmentMode(AlignmentMode::AM_MID_LEFT);
            break;
        case CheckBoxLayout::TextLeft:
            m_boxRect.left = marginRect.right() - m_boxRect.width;
            m_caption->setRect({marginRect.left, marginRect.top,
                    marginRect.width - m_boxRect.width, marginRect.height});
            m_caption->setAlignmentMode(AlignmentMode::AM_MID_RIGHT);
            break;
    }
}
```

#### 垂直对齐

```cpp
void CheckBox::adjustBoxVerticalAlign(void) {
    SRect marginRect = getMarginedRect();
    switch (m_verticalAlign) {
        case CheckBoxVerticalAlign::Top:
            m_boxRect.top = marginRect.top;
            break;
        case CheckBoxVerticalAlign::Center:
            m_boxRect.top = marginRect.top + (marginRect.height - m_boxRect.height) / 2;
            break;
        case CheckBoxVerticalAlign::Bottom:
            m_boxRect.top = marginRect.bottom() - m_boxRect.height;
            break;
    }
}
```

### 8.3 缩放处理

所有位置数据（`m_boxRect`、`m_caption` 的位置）存储为未缩放的值，绘制时使用 `getBoxDrawRect()` 转换：

```cpp
SRect CheckBox::getBoxDrawRect(){
    SRect boxFrameRect = {m_boxRect.left + m_boxMargin.left, m_boxRect.top + m_boxMargin.top,
                    m_boxRect.width - m_boxMargin.left - m_boxMargin.right,
                    m_boxRect.height - m_boxMargin.top - m_boxMargin.bottom};
    SRect boxDrawRect = mapToDrawRect(boxFrameRect);
    return boxDrawRect;
}
```

### 8.4 绘制逻辑

绘制流程：
1. 调用 `preDraw()` 绘制背景和边框
2. 调用 `drawCheckBoxFrame()` 绘制复选框外框
3. 根据状态绘制标记（勾选、X、或横线）
4. 子控件（内部 Label）由 `ControlImpl::draw()` 绘制

### 8.5 样式绘制

| 样式 | 外框 | 选中状态 | 三态 |
|------|------|----------|------|
| Classic | 方框边框 | 勾选符号（√） | 横线 |
| Cross | 方框边框 | X 符号（×） | 横线 |
| Circle | 圆形边框 | 勾选符号（√） | 横线 |

### 8.6 特殊实现说明

1. **内部 Label**：CheckBox 拥有内部 Label 作为子控件，通过 `addControl()` 添加
2. **属性变化监听**：Label 属性变化时，通过回调触发 CheckBox 重新布局
3. **尺寸来源**：复选框尺寸基于内部 Label 的行高，而非字体大小

## 9. 测试用例

### 9.1 基本功能测试（使用新 API）

```cpp
void testBasicCheckBox() {
    auto checkbox = CheckBoxBuilder(nullptr, SRect(50, 50, 100, 30))
        .setCaptionText("1. Accept Terms")   // 使用 setCaptionText
        .build();
    BENCH->addControl(checkbox);
}
```

### 9.2 状态切换测试

```cpp
void testStateChange() {
    auto checkbox = CheckBoxBuilder(nullptr, SRect(50, 100, 200, 30))
        .setCaptionText("2. Enable Feature")
        .setCheckState(CheckState::Checked)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState state) {
            cout << "State changed to: " << (int)state << endl;
        })
        .build();
}
```

### 9.3 样式测试

```cpp
void testStyles() {
    auto classic = CheckBoxBuilder(nullptr, SRect(50, 150, 200, 30))
        .setStyle(CheckBoxStyle::Classic)
        .setCaptionText("3. Classic Style")
        .build();

    auto cross = CheckBoxBuilder(nullptr, SRect(50, 200, 200, 30))
        .setStyle(CheckBoxStyle::Cross)
        .setCaptionText("4. Cross Style")
        .build();

    auto circle = CheckBoxBuilder(nullptr, SRect(50, 250, 200, 30))
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionText("5. Circle Style")
        .build();
}
```

### 9.4 布局测试

```cpp
void testLayouts() {
    auto textRight = CheckBoxBuilder(nullptr, SRect(50, 300, 200, 30))
        .setLayout(CheckBoxLayout::TextRight)
        .setCaptionText("6. Text on Right")
        .build();

    auto textLeft = CheckBoxBuilder(nullptr, SRect(300, 300, 200, 30))
        .setLayout(CheckBoxLayout::TextLeft)
        .setCaptionText("7. Text on Left")
        .build();
}
```

### 9.5 使用 getCaption() 直接配置 Label

```cpp
void testCaptionAccess() {
    auto checkbox = CheckBoxBuilder(nullptr, SRect(50, 350, 300, 60), 2.0f, 2.0f)
        .setCaptionText("8. 2x缩放复选框")
        .setCheckState(CheckState::Checked)
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionSize(24)  // 设置内部 Label 字体大小
        .build();

    // 直接访问内部 Label
    StateColor sc = checkbox->getCaption().getTextStateColor();
    sc.setNormal({0, 0, 255, 255});
    checkbox->getCaption().setTextStateColor(sc);
}
```

### 9.6 缩放测试

```cpp
void testScaling() {
    auto scaled2x = CheckBoxBuilder(nullptr, SRect(50, 550, 200, 30), 2.0f, 2.0f)
        .setCaptionText("9. 2x Scaled")
        .setCheckState(CheckState::Checked)
        .build();

    auto scaled0_5x = CheckBoxBuilder(nullptr, SRect(300, 550, 200, 30), 0.5f, 0.5f)
        .setCaptionText("10. 0.5x Scaled")
        .setCheckState(CheckState::Indeterminate)
        .build();
}
```

### 9.7 尺寸倍率测试

```cpp
void testSizeRatios() {
    auto defaultRatio = CheckBoxBuilder(nullptr, SRect(50, 50, 200, 30))
        .setCaptionText("11. Default Ratio (1.0)")
        .build();

    auto largeRatio = CheckBoxBuilder(nullptr, SRect(50, 100, 200, 30))
        .setCaptionText("12. Large Ratio (1.5)")
        .setSizeRatio(1.5f)
        .build();

    auto smallRatio = CheckBoxBuilder(nullptr, SRect(50, 150, 200, 30))
        .setCaptionText("13. Small Ratio (0.5)")
        .setSizeRatio(0.5f)
        .build();
}
```

### 9.8 禁用状态测试

```cpp
void testDisabled() {
    auto disabled = CheckBoxBuilder(nullptr, SRect(50, 400, 200, 30))
        .setCaptionText("14. Disabled Checkbox")
        .setEnable(false)
        .setCheckState(CheckState::Checked)
        .build();
}
```

### 9.9 回调测试

```cpp
void testCallbacks() {
    auto checkbox = CheckBoxBuilder(nullptr, SRect(50, 650, 200, 30))
        .setCaptionText("15. Callback Test")
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState state) {
            static int count = 0;
            count++;
            cout << "Callback #" << count << ": State = " << (int)state << endl;
        })
        .build();
}
```

### 9.10 自定义颜色测试

```cpp
void testCustomColors() {
    auto customCheck = CheckBoxBuilder(nullptr, SRect(50, 50, 250, 30))
        .setCaptionText("16. Custom Check Color")
        .setCheckState(CheckState::Checked)
        .setCheckColor({255, 0, 255, 255})  // 紫色勾选
        .build();

    auto customCross = CheckBoxBuilder(nullptr, SRect(50, 100, 250, 30))
        .setStyle(CheckBoxStyle::Cross)
        .setCaptionText("17. Custom Cross Color")
        .setCheckState(CheckState::Checked)
        .setCrossColor({0, 255, 255, 255})  // 青色 X
        .build();

    auto customIndeterminate = CheckBoxBuilder(nullptr, SRect(50, 150, 250, 30))
        .setCaptionText("18. Custom Indeterminate Color")
        .setCheckState(CheckState::Indeterminate)
        .setIndeterminateColor({255, 165, 0, 255})  // 橙色横线
        .build();
}
```

## 10. 文件结构

```
UIControls/
├── include/
│   ├── CheckBox.h        # 头文件
│   ├── ControlBase.h     # 基类（含 create/recreate）
│   └── ConstDef.h        # 常量定义
├── src/
│   ├── CheckBox.cpp      # 实现文件
│   ├── ControlBase.cpp   # 基类实现
│   └── ConstDef.cpp      # 常量定义
└── CMakeLists.txt        # 构建配置

test/
└── test_checkbox.cpp     # 测试文件

doc/
└── CheckBox_Design.md   # 本文档
```

## 11. 实现顺序

1. 在 `ConstDef.h` 添加常量声明（包含新增的 BOX_PEN_WIDTH、MARK_PEN_WIDTH）
2. 在 `ConstDef.cpp` 添加常量定义
3. 更新 `ControlBase.h` 和 `ControlBase.cpp`（添加 create/recreate/preDraw）
4. 创建/更新 `UIControls/include/CheckBox.h`
5. 创建/更新 `UIControls/src/CheckBox.cpp`
6. 更新 `UIControls/CMakeLists.txt`
7. 创建/更新 `test/test_checkbox.cpp`
8. 编译测试