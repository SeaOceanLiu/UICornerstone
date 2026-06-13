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

### 2.2 事件定义

```cpp
struct MouseWheelEventData {
    float x;
    float y;
    float mouseX;
    float mouseY;
};
```

## 3. 接口设计

### 3.1 TextArea 类

```cpp
class TextArea: public EditBox {
    friend class TextAreaBuilder;
private:
    std::vector<std::string> m_lines;
    std::vector<int> m_lineStartPositions;
    std::vector<int> m_linePixelWidths;
    int m_scrollY;
    int m_scrollX;
    bool m_wordWrap;
    int m_lineHeight;
    shared_ptr<ScrollBar> m_vScrollBar;
    shared_ptr<ScrollBar> m_hScrollBar;
    bool m_autoScroll;
    bool m_updatingScrollBar;
    
    TTF_TextEngine *m_textEngine;
    std::string m_lastTextForRebuild;
    
private:
    void rebuildLines();
    void updateVScrollBar();
    void updateHScrollBar();
    void ensureCursorHorizontalVisible();
    void ensureCursorVisible();
    int getVisibleLines();
    int getTotalLines() const { return static_cast<int>(m_lines.size()); }
    int getMaxLinePixelWidth() const;
    float getCharWidth(const std::string& text, int byteIndex);
    int getLinePixelWidth(const std::string& line);
    int getByteIndexFromPixelX(const std::string& text, float pixelX);
    
public:
    TextArea(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    void setRenderer(SDL_Renderer *renderer) override;
    
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

### 4.1 行解析

TextArea 将文本解析为多行：

```cpp
void TextArea::rebuildLines() {
    m_lines.clear();
    std::istringstream stream(m_text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        m_lines.push_back(line);
    }
}
```

### 4.2 自动换行

启用自动换行时，根据控件宽度分割行：

```cpp
if (m_wordWrap) {
    int maxWidth = getWidth() - m_hScrollBar->getThickness();
    // 按像素宽度分割长行
}
```

### 4.3 滚动条同步

TextArea 管理垂直和水平滚动条：

```cpp
void TextArea::updateVScrollBar() {
    if (!m_vScrollBar) return;
    
    int totalHeight = getTotalLines() * m_lineHeight;
    int visibleHeight = getHeight();
    
    m_vScrollBar->setRange(0, totalHeight - visibleHeight);
    m_vScrollBar->setPageSize(visibleHeight);
    m_vScrollBar->setValue(m_scrollY);
}
```

### 4.4 滚动到光标

确保光标始终可见：

```cpp
void TextArea::ensureCursorVisible() {
    int cursorLine = getCursorLine();
    int cursorY = cursorLine * m_lineHeight;
    
    if (cursorY < m_scrollY) {
        setScrollY(cursorY);
    } else if (cursorY + m_lineHeight > m_scrollY + getVisibleHeight()) {
        setScrollY(cursorY + m_lineHeight - getVisibleHeight());
    }
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
    .setOnTextChanged([](string text) {
        cout << "Text changed: " << text.length() << " chars" << endl;
    })
    .build();
BENCH->addControl(textArea);
```