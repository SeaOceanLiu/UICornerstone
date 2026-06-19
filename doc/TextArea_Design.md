# TextArea 文本域设计文档

## 1. 概述

TextArea（文本域）是一种用于接收多行文本输入的 UI 控件，继承自 EditBox，支持滚动、自动换行、文本域等功能。

## 2. 功能规格

### 2.1 核心功能

- **多行文本**：支持多行文本输入和显示
- **自动滚动**：支持滚动到光标位置
- **自动换行**：支持自动换行功能
- **滚动条**：内置垂直和水平滚动条
- **行高设置**：支持自定义行高

## 3. 接口设计

### 3.1 TextArea 类

- 继承自 `EditBox`（见 `include/EditBox.h`），通过继承的 `getTextRenderer()` 访问 `TextRenderer*`（由 `ControlImpl` 提供，非独立成员）
- 不再持有 `TTF_TextEngine*` 或 `SDL_Renderer*`
- 事件处理使用 union 风格的 `Event` API（Phase 12），通过 `event->keyEvent`、`event->textInput.text`、`event->mousePos`、`event->mouseWheel`、`event->mouseButton` 字段访问

```cpp
class TextArea: public EditBox {
    friend class TextAreaBuilder;
private:
    std::vector<std::string> m_lines;
    std::vector<int> m_lineStartPositions;
    int m_scrollY;
    int m_scrollX;
    bool m_wordWrap;
    int m_lineHeight;
    shared_ptr<ScrollBar> m_vScrollBar;
    shared_ptr<ScrollBar> m_hScrollBar;
    bool m_autoScroll;
    bool m_updatingScrollBar;

    std::string m_lastTextForRebuild;

private:
    void rebuildLines();
    void updateVScrollBar();
    void updateHScrollBar();
    void ensureCursorHorizontalVisible();
    void ensureCursorVisible();
    int getVisibleLines();
    int getTotalLines() const { return static_cast<int>(m_lines.size()); }
    int getMaxLinePixelWidth();
    float getCharWidth(const std::string& text, int byteIndex);
    int getLinePixelWidth(const std::string& line);
    int getByteIndexFromPixelX(const std::string& text, float pixelX);

public:
    TextArea(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void setText(const std::string& text);
    void insertTextAtCursor(const std::string& text);
    void insertText(const std::string& text) override;
    void deleteSelectedText() override;

    void setScrollY(int y);
    int getScrollY() const { return m_scrollY; }
    void setScrollX(int x);
    int getScrollX() const { return m_scrollX; }
    void scrollToBottom();
    void setWordWrap(bool enable);
    bool isWordWrap() const { return m_wordWrap; }
    void setLineHeight(int height);
    int getLineHeight() const { return m_lineHeight; }
    void setScrollBarThickness(float thickness);
    float getScrollBarThickness() const;

    void setOnTextChangedHandler(OnTextChangedHandler handler);
};
```

### 3.2 TextAreaBuilder 类

```cpp
class TextAreaBuilder {
private:
    shared_ptr<TextArea> m_textArea;
public:
    TextAreaBuilder(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    TextAreaBuilder& setBackgroundStateColor(StateColor stateColor);
    TextAreaBuilder& setBorderStateColor(StateColor stateColor);
    TextAreaBuilder& setTextStateColor(StateColor stateColor);

    TextAreaBuilder& setText(const std::string& text);
    TextAreaBuilder& setPlaceholder(const std::string& placeholder);
    TextAreaBuilder& setPasswordMode(bool enable);
    TextAreaBuilder& setPasswordChar(char c);
    TextAreaBuilder& setFont(FontName fontName);
    TextAreaBuilder& setFontSize(int size);
    TextAreaBuilder& setWordWrap(bool enable);
    TextAreaBuilder& setLineHeight(int height);
    TextAreaBuilder& setOnTextChanged(TextArea::OnTextChangedHandler handler);
    TextAreaBuilder& setOnEnter(EditBox::OnEnterHandler handler);
    TextAreaBuilder& setId(int id);
    TextAreaBuilder& setTransparent(bool isTransparent);

    shared_ptr<TextArea> build(void);
};
```

## 4. 实现细节

### 4.1 文本测量

文本测量和绘制全部通过 `getTextRenderer()` 进行，不再直接调用 TTF API：

- `getCharWidth()`：用 `getTextRenderer()->measureText(getFont(), charStr)` 获取单个字符宽度
- `getLinePixelWidth()`：用 `getTextRenderer()->measureText(getFont(), line)` 获取整行像素宽度
- 绘制文本：用 `getTextRenderer()->drawText(getFont(), displayLine, x, y, color)` 逐行绘制

### 4.2 行解析

`rebuildLines()` 将文本解析为多行，支持 `\n` 换行符分割和自动换行折叠：

```cpp
void TextArea::rebuildLines() {
    m_lines.clear();
    m_lineStartPositions.clear();
    // 按 \n 分割文本为逻辑行
    // 若启用 wordWrap，将超出 availableWidth 的逻辑行进一步折叠
    // availableWidth = getRect().width - 16.0f - 16.0f * scaleXX
    // 记录每行在原始文本中的起始字节偏移（m_lineStartPositions）
    // 最后一行若尾部是 \n，追加空行
}
```

### 4.3 滚动条同步

TextArea 管理垂直和水平滚动条。滚动条通过 `setOnPositionChanged` lambda 回调修改 `m_scrollY`/`m_scrollX`：

```cpp
void TextArea::updateVScrollBar() {
    int totalHeight = getTotalLines() * m_lineHeight;
    int availableHeight = (int)rect.height - margin.top - margin.bottom;
    if (hScrollVisible) availableHeight -= hThickness;

    if (totalHeight > availableHeight) {
        m_vScrollBar->setVisible(true);
        m_vScrollBar->setRange(0, totalHeight - availableHeight);
        m_vScrollBar->setPageSize(availableHeight);
    } else {
        m_vScrollBar->setVisible(false);
        m_scrollY = 0;
    }
}
```

### 4.4 事件处理

`handleEvent()` 使用 union 风格的 Event API，事件处理顺序：

1. **TextInput**（`event->textInput.text`）：过滤控制字符（保留 `\t`），调用 `insertText()`
2. **KeyDown** — Enter/Return：插入 `\n`，重建行，触发 `onTextChanged`
3. **Mouse** 先委托给可见的滚动条（`m_vScrollBar->handleEvent()` / `m_hScrollBar->handleEvent()`）
4. **MouseDown Left**（`event->mouseButton`）：点击定位光标行列，支持 Shift 选择
5. **MouseWheel**（`event->mouseWheel.scrollY`）：垂直滚动
6. **MouseMove**（`event->mousePos`）：拖拽选择
7. **MouseUp**（`event->mouseButton`）：结束拖拽
8. **KeyDown** — Ctrl+C/V/X：复制/粘贴/剪切
9. **KeyDown** — Up/Down/Home/End/Left/Right：光标导航，支持 Shift 扩展选择
10. 未处理的事件回退到 `EditBox::handleEvent()`

### 4.5 滚动到光标

```cpp
void TextArea::ensureCursorVisible() {
    int cursorY = cursorLine * m_lineHeight;
    int availableHeight = (int)(availableHeightScaled / scale);

    if (cursorY < m_scrollY) {
        m_scrollY = cursorY;
    } else if (cursorY + m_lineHeight > m_scrollY + availableHeight) {
        m_scrollY = cursorY + m_lineHeight - availableHeight;
    }
    // autoScroll 模式下始终滚动到底部
}
```

## 5. 文件结构

```
UICornerstone/
├── include/
│   └── TextArea.h
├── src/
│   └── TextArea.cpp
└── CMakeLists.txt
```

## 6. 使用示例

```cpp
auto textArea = TextAreaBuilder(nullptr, SRect(100, 100, 300, 200))
    .setWordWrap(true)
    .setLineHeight(24)
    .setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "Text changed: " << text.length() << " chars" << endl;
    })
    .build();
BENCH->addControl(textArea);
```
