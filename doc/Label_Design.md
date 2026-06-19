# Label 标签设计文档

> 编制 2026-06-19 | 最后更新 2026-06-19

## 1. 概述

Label（标签）是一种用于显示文本的 UI 控件，支持单行和多行文本渲染、阴影效果、对齐方式等功能。

后端无关：通过 `TextRenderer` 抽象接口测量和绘制文本，不直接依赖任何 SDL/TTF 类型。

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

3. **字体文件缓存**：`loadFromResource()` 中 `if (m_font) return;` 避免重复 Provider 读取和桥接开销（`src/Label.cpp`，2026-06-19 优化）

## 3. 功能规格

### 3.1 核心功能

- **文本渲染**：支持单行和多行文本显示，通过 `TextRenderer::drawText()` 绘制（`void*` cached text 路径）
- **对齐方式**：支持 9 种对齐模式
- **阴影效果**：支持文本阴影及偏移设置
- **自动扩展**：支持根据文本内容自动扩展尺寸
- **点击事件**：支持点击回调（`handleEvent` 使用新 union API）
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
    SharedFont m_font;                // 抽象字体（TextRenderer 创建）
    shared_ptr<vector<char>> m_fontData;  // 字体文件数据（保持生命周期）

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
    std::vector<void*> m_cachedTexts;   // 每行缓存的 TextRenderer handle
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
    void releaseTexts(void);             // 释放 m_cachedTexts

    void loadFontInternal(void);         // 通过 TextRenderer::loadFont 加载
    void createMultilineText(void);      // \n 分割
    void createCachedTexts(void);        // TextRenderer::createText 每行
    void computeLineOffsets(void);       // 对齐排版计算
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
    LabelBuilder& setFontStyle(int fontStyle);  // int（非 TTF_FontStyleFlags）
    LabelBuilder& setId(int id);
    LabelBuilder& setDebugDraw(bool enabled);

    shared_ptr<Label> build(void);
};
```

## 5. 实现细节

### 5.1 字体与文本渲染

Label 不再直接使用 SDL3_ttf。所有字体操作通过 `TextRenderer` 抽象接口委托到后端：

| 操作 | 抽象方法 | 后端实现 |
|------|---------|---------|
| 加载字体 | `TextRenderer::loadFontFromMemory(data, len, size)` | SDL3→`TTF_OpenFontIO`, SFML→`sf::Font::loadFromMemory`, Raylib→`LoadFontFromMemory` |
| 创建文本 | `TextRenderer::createText(font, text)` → `void*` | SDL3→`TTF_CreateText`, SFML→缓存 `sf::Text` |
| 测量文本 | `TextRenderer::measureText(void* handle)` | 按后端查询 |
| 绘制文本 | `TextRenderer::drawText(void* handle, x, y, color)` | 按后端绘制 |

字体数据生命周期由 `m_fontData`（`shared_ptr<vector<char>>`）保持，防止 TTF 回调延迟读取时悬垂指针。

### 5.2 TTF_Text 缓存（Phase 7）

Label 为每行文本缓存在 `m_cachedTexts`（`vector<void*>`），避免每帧创建/销毁 `TTF_Text*`：

- `createCachedTexts()`：`recreate()` 后为每行调用 `TextRenderer::createText()`
- `releaseTexts()`：`~Label()` / `recreate()` 时调用 `TextRenderer::destroyText()`
- `computeLineOffsets()`：使用缓存的 handle 测量宽度
- 截断行（`truncateLine`）时原地替换缓存项

### 5.3 生命周期管理

```cpp
void Label::create(void) {
    if (m_isCreated) return;

    loadFontInternal();            // 1. 加载字体
    createMultilineText();         // 2. 分割文本
    createCachedTexts();           // 3. 创建每行 TTF_Text handle
    computeLineOffsets();          // 4. 排版计算

    ControlImpl::create();
}

void Label::recreate(void) {
    if(!m_isCreated) return;

    releaseTexts();                // 仅释放缓存的文本 handle 和字体
    releaseFont();                 // 字体在 setFont/setFontSize 时才释放

    m_isCreated = false;
    create();

    if (m_onPropertyChanged != nullptr) {
        m_reentryCounter++;
        if (m_reentryCounter == 1) {
            m_onPropertyChanged(dynamic_pointer_cast<Label>(this->getThis()));
        }
        m_reentryCounter--;
    }
}
```

> **优化**（2026-06-19）：`recreate()` 不再调用 `releaseFont()` — 字体在文本/对齐/边距变化时无需重载。只有 `setFont()` / `setFontSize()` 显式释放字体。

### 5.4 脏矩形优化（Phase 15）

`setRect()` 和 `setParent()` 添加脏矩形检查，避免无变化时触发 `recreate()`：

```cpp
void Label::setRect(SRect rect) {
    if (m_rect.equals(rect)) return;  // 脏矩形跳过
    ...
}
```

### 5.5 对齐计算

根据不同的对齐模式计算每行文本的偏移位置：

```cpp
// 水平对齐
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

// 垂直对齐
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

### 5.6 阴影渲染

在主文本之前渲染阴影文本，支持偏移量设置。

### 5.7 属性变更回调

Label 提供 `setOnPropertyChanged()` 回调，当属性变化时触发，通知父控件调整布局：

```cpp
// CheckBox 中使用示例
m_label->setOnPropertyChanged([this](shared_ptr<Label> label){
    setBoxSize();
    adjustSpaceAssignment();
    adjustBoxVerticalAlign();
})
```

### 5.8 事件处理（Phase 12）

`handleEvent()` 使用新 union API：

```cpp
bool Label::handleEvent(shared_ptr<Event> event) {
    // 只处理位置相关事件
    if (event->m_type == EventType::MouseDown ||
        event->m_type == EventType::MouseUp) {
        // 检查点击事件（FINGER 事件使用旧 API）
    }
    return false;
}
```

## 6. 文件结构

```
UICornerstone/
├── include/
│   ├── Label.h
│   └── TextRenderer.h           (抽象接口)
├── src/
│   ├── Label.cpp
│   └── backend/{sdl3,sfml,raylib}/TextRenderer.cpp  (后端实现)
└── CMakeLists.txt
```

## 7. 演化历史

| 日期 | 变更 |
|------|------|
| Phase 5 | TTF_Font*/TTF_TextEngine* → SharedFont + TextRenderer 抽象 |
| Phase 7 | m_lineTexts(TTF_Text*) → m_cachedTexts(void*) + releaseTexts() |
| Phase 8 | 添加 m_fontData 解决字体文件数据生命周期问题 |
| Phase 12 | handleEvent 迁移到新 union API |
| Phase 15 | setRect/setParent 脏矩形跳过；recreate() 不再释放字体 |
| 2026-06-19 | loadFromResource 缓存跳过避免 Provider 重复读取；移除 UI 帧创建/销毁字体