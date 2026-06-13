# Label 标签设计文档

## 1. 概述

Label（标签）是一种用于显示文本的 UI 控件，支持单行和多行文本渲染、阴影效果、对齐方式等功能。

## 2. 设计规则

> 参见 [AGENTS.md](../AGENTS.md) 中的设计规则章节。

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 生成字体时，使用缩放后的字体大小（即和字体大小相关的数据都是缩放后的）
   - 绘制时才对缩放进行处理，而字体因为是已缩放的，所以可以直接绘制

2. **控件生命周期**：
   - `create()`: 初始创建控件，缺省情况下只有初始创建控件后，才能显示和处理事件
   - `recreate()`: 重新创建控件，主要用于在一些属性改变时需要重新创建控件的情况
   - `preDraw()`: 在绘制之前调用，负责绘制背景和边框

## 3. 功能规格

### 3.1 核心功能

- **文本渲染**：支持单行和多行文本显示
- **对齐方式**：支持 9 种对齐模式
- **阴影效果**：支持文本阴影及偏移设置
- **自动扩展**：支持根据文本内容自动扩展尺寸
- **点击事件**：支持点击回调
- **属性变更回调**：支持属性变更回调，用于父控件响应属性变化
- **调试绘制**：支持绘制热区矩形用于调试

### 3.2 对齐模式定义

```cpp
enum class AlignmentMode: int{
    AM_TOP_LEFT,      // 顶部左对齐
    AM_MID_LEFT,      // 中部左对齐
    AM_BOTTOM_LEFT,   // 底部左对齐

    AM_TOP_RIGHT,     // 顶部右对齐
    AM_MID_RIGHT,     // 中部右对齐
    AM_BOTTOM_RIGHT,  // 底部右对齐

    AM_TOP_CENTER,    // 顶部居中
    AM_CENTER,        // 完全居中
    AM_BOTTOM_CENTER  // 底部居中
};
```

## 4. 接口设计

### 4.1 Label 类

```cpp
class Label: public ControlImpl {
    friend class LabelBuilder;
    using OnClickHandler = std::function<void (shared_ptr<Label>)>;
    using OnPropertyChangedHandler = std::function<void (shared_ptr<Label>)>;

private:
    TTF_Font *m_font;
    TTF_TextEngine *m_textEngin;

    SPoint m_shadowOffset;
    AlignmentMode m_AlignmentMode;
    int m_fontSize;
    string m_caption;
    bool m_shadowEnabled;
    FontName m_fontName;
    fs::path m_fontFile;

    OnClickHandler m_onClick;
    OnPropertyChangedHandler m_onPropertyChanged;
    std::vector<std::string> m_lines;
    std::vector<TTF_Text*> m_lineTexts;
    std::vector<SPoint> m_lineOffsets;  // 每行文本的偏移位置（未缩放）

    int m_lineHeight;                     // 行高（未缩放）
    int m_lineSpacing;                    // 行间距（未缩放）
    int m_defaultLineHeight;              // 用户设置的行高
    float m_defaultLineSpacingRatio;     // 用户设置的行间距比例
    int m_reentryCounter;                 // 防止回调重入
    bool m_enableExpand;
    bool m_debugDraw;

    SRect m_hotRect;                      // 文本热区（未缩放）

protected:
    void recreate(void) override;
public:
    Label(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    ~Label(void);
    void create(void) override;
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    void setParent(Control *parent) override;

    void setCaption(string caption);
    string getCaption(void) const;
    void setFont(FontName fontName);
    void setAlignmentMode(AlignmentMode Alignment);
    AlignmentMode getAlignmentMode(void) const;
    void setFontSize(int fontSize);
    void setShadow(bool enabled);
    void setShadowOffset(SPoint offset);
    void setOnClick(OnClickHandler handler);
    void setOnPropertyChanged(OnPropertyChangedHandler handler);

    void setLineHeight(int height);
    int getLineHeight() const;
    void setLineSpacingRatio(float spacingRatio);
    void setDebugDraw(bool enabled);

    void setEnableExpand(bool enable);
    bool getEnableExpand() const;

private:
    void releaseFont(void);
    void releaseTextEngin(void);
    void releaseTextObjects(void);
    void releaseMultilineTexts(void);

    void createTextEngine(void);
    void createMultilineText(void);
    void createLineTexts(void);
    SSize getTextSize(TTF_Text* text);
    float getStringWidth(const string& text);
    void truncateLine(string& line, float maxWidth);
    SRect getHotRect(void);
};
```

### 4.2 LabelBuilder 类

```cpp
class LabelBuilder {
private:
    shared_ptr<Label> m_label;
public:
    LabelBuilder(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

    LabelBuilder& setTextStateColor(StateColor stateColor);
    LabelBuilder& setTextShadowStateColor(StateColor stateColor);
    LabelBuilder& setCaption(string caption);
    LabelBuilder& setFont(FontName fontName);
    LabelBuilder& setAlignmentMode(AlignmentMode Alignment);
    LabelBuilder& setFontSize(int fontSize);
    LabelBuilder& setMargin(Margin margin);
    LabelBuilder& setShadow(bool enabled);
    LabelBuilder& setShadowOffset(SPoint offset);
    LabelBuilder& setOnClick(Label::OnClickHandler handler);
    LabelBuilder& setOnPropertyChanged(Label::OnPropertyChangedHandler handler);
    LabelBuilder& setLineHeight(int height);
    LabelBuilder& setLineSpacingRatio(float spacingRatio);
    LabelBuilder& setEnableExpand(bool enable);
    LabelBuilder& setBorderStateColor(StateColor stateColor);
    LabelBuilder& SetFontStyle(TTF_FontStyleFlags fontStyle);
    LabelBuilder& setId(int id);
    LabelBuilder& setDebugDraw(bool enabled);

    shared_ptr<Label> build(void);
};
```

## 5. 实现细节

### 5.1 生命周期管理

Label 使用 `create()` 和 `recreate()` 模式管理资源：

```cpp
void Label::create(void) {
    if (m_isCreated) return;

    // 1. 加载字体资源
    loadFromResource(m_fontFile.string());
    // 2. 创建文本引擎
    createTextEngine();
    // 3. 多行文本分割
    createMultilineText();
    // 4. 创建多行文本实例
    createLineTexts();

    // 调用基类 create
    ControlImpl::create();
}

void Label::recreate(void) {
    if(!m_isCreated) return;

    // 释放所有资源
    releaseTextObjects();
    releaseMultilineTexts();
    releaseTextEngin();
    releaseFont();

    // 重置创建标志并重新创建
    m_isCreated = false;
    create();

    // 触发属性变更回调
    if (m_onPropertyChanged != nullptr) {
        m_reentryCounter++;
        if (m_reentryCounter == 1) {
            m_onPropertyChanged(dynamic_pointer_cast<Label>(this->getThis()));
        }
        m_reentryCounter--;
    }
}
```

### 5.2 文本解析与创建

Label 支持多行文本，通过 `\n` 分割，然后创建每行的 TTF_Text 对象：

```cpp
void Label::createMultilineText(void) {
    m_lines.clear();
    size_t start = 0;
    while (true) {
        size_t pos = m_caption.find('\n', start);
        if (pos == string::npos) {
            m_lines.push_back(m_caption.substr(start));
            break;
        } else {
            m_lines.push_back(m_caption.substr(start, pos - start));
            start = pos + 1;
        }
    }
}

void Label::createLineTexts(void) {
    // 为每一行创建 TTF_Text 对象
    // 计算每行的偏移位置
    // 处理文本截断和对齐
}
```

### 5.3 缩放处理

根据设计规则：
1. **存储位置**：所有位置数据存储为未缩放的值
2. **字体生成**：使用缩放后的字体大小
3. **绘制**：使用 `mapToDrawPoint()` 和 `mapToDrawRect()` 进行坐标转换

```cpp
SPoint ControlImpl::mapToDrawPoint(SPoint point){
    SRect drawRect = getDrawRect();
    return {point.x * getScaleXX() + drawRect.left,
        point.y * getScaleYY() + drawRect.top};
}

SRect ControlImpl::mapToDrawRect(SRect rect){
    SRect drawRect = getDrawRect();
    return {rect.left * getScaleXX() + drawRect.left,
        rect.top * getScaleYY() + drawRect.top,
        rect.width * getScaleXX(),
        rect.height * getScaleYY()};
}
```

### 5.4 对齐计算

根据不同的对齐模式计算每行文本的偏移位置：

```cpp
// 水平对齐计算
switch (m_AlignmentMode) {
    case AlignmentMode::AM_TOP_RIGHT:
    case AlignmentMode::AM_MID_RIGHT:
    case AlignmentMode::AM_BOTTOM_RIGHT:
        m_lineOffsets[i].x = marginRect.left + (availableWidth - lineWidth);
        break;
    case AlignmentMode::AM_TOP_CENTER:
    case AlignmentMode::AM_CENTER:
    case AlignmentMode::AM_BOTTOM_CENTER:
        m_lineOffsets[i].x = marginRect.left + (availableWidth - lineWidth) / 2;
        break;
}

// 垂直对齐计算
switch (m_AlignmentMode) {
    case AlignmentMode::AM_CENTER:
    case AlignmentMode::AM_MID_LEFT:
    case AlignmentMode::AM_MID_RIGHT:
        m_lineOffsets[i].y = marginRect.top + (availableHeight - totalHeight) / 2 + (m_lineHeight + lineSpacing) * i;
        break;
    case AlignmentMode::AM_BOTTOM_LEFT:
    case AlignmentMode::AM_BOTTOM_CENTER:
    case AlignmentMode::AM_BOTTOM_RIGHT:
        m_lineOffsets[i].y = marginRect.top + (availableHeight - totalHeight) + (m_lineHeight + lineSpacing) * i;
        break;
}
```

### 5.5 阴影渲染

在主文本之前渲染阴影文本，支持偏移量设置。

### 5.6 属性变更回调

Label 提供 `setOnPropertyChanged()` 回调，当属性变化时触发，通知父控件调整布局：

```cpp
// CheckBox 中使用示例
m_label->setOnPropertyChanged([this](shared_ptr<Label> label){
    setBoxSize();
    adjustSpaceAssignment();
    adjustBoxVerticalAlign();
})
```

## 6. 常量定义

在 `ConstDef.h` 中定义 Label 相关常量（无新增）。

## 7. 文件结构

```
UICornerstone/
├── include/
│   └── Label.h
├── src/
│   └── Label.cpp
└── CMakeLists.txt
```