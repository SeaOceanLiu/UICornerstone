# EditBox 编辑框设计文档

## 1. 概述

EditBox（编辑框）是一种用于接收用户文本输入的 UI 控件，支持单行文本输入、光标定位、文本选择、密码模式等功能。

## 2. 功能规格

### 2.1 核心功能

- **文本输入**：支持单行文本输入和编辑
- **光标管理**：支持光标定位、闪烁显示
- **文本选择**：支持选中文本、复制、剪切、粘贴
- **密码模式**：支持密码输入模式（显示遮罩字符）
- **占位符**：支持占位符文本显示
- **焦点管理**：支持焦点获取和失去事件

### 2.2 事件定义

事件数据结构定义在 `EventTypes.h` 中：

```cpp
// EventTypes.h — 键盘事件
struct KeyEventData {
    int32_t keycode;
    int32_t scancode;
    uint16_t mod;
    bool repeat;
};
```

EditBox 通过 `EventType::Custom` + `customInt`/`customPtr` 实现自定义 `ON_FOCUS` 事件（`EventName::ON_FOCUS`）的焦点切换。

## 3. 接口设计

### 3.1 EditBox 类

```cpp
class EditBox: public ControlImpl {
    friend class EditBoxBuilder;
public:
    using OnTextChangedHandler = std::function<void (shared_ptr<Control>, std::string)>;
    using OnEnterHandler = std::function<void (shared_ptr<Control>)>;

protected:
    std::string m_text;
    std::string m_placeholderText;
    int m_cursorPosition;
    int m_selectionStart;
    int m_selectionEnd;
    bool m_passwordMode;
    char m_passwordChar;
    bool m_focused;
    int32_t m_cursorBlinkTime;
    bool m_cursorVisible;
    bool m_shiftPressed;
    bool m_ctrlPressed;
    bool m_isDragging;
    int m_dragStartPosition;

    float m_textOffsetX;
    float m_textOffsetY;

    Margin m_margin;

    SharedFont m_font;
    shared_ptr<vector<char>> m_fontData;
    int m_fontSize;
    FontName m_fontName;

    OnTextChangedHandler m_onTextChanged;
    OnEnterHandler m_onEnter;
    bool m_focusWatcherRegistered;
    AlignmentMode m_AlignmentMode;

protected:
    void loadFontInternal();
    std::string getDisplayText() const;
    float getTextWidth(const std::string& text);
    int getCursorFromPosition(float x);
    float getCursorX(int cursorPos);
    std::string getUtf8Substr(const std::string& str, int start, int length) const;
    void updateTextOffset();
    virtual void insertText(const std::string& text);
    static int getUtf8CharLength(unsigned char c);

public:
    EditBox(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    ~EditBox();
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    void setText(const std::string& text);
    std::string getText() const;
    int getCursorPosition() const { return m_cursorPosition; }
    void setPlaceholder(const std::string& placeholder);
    std::string getPlaceholder() const;

    void setPasswordMode(bool enable);
    bool isPasswordMode() const { return m_passwordMode; }
    void setPasswordChar(char c);

    void selectAll();
    void setSelection(int start, int end);
    void clearSelection();
    std::string getSelectedText() const;
    bool hasSelection() const { return m_selectionStart != m_selectionEnd; }

    void copy();
    void cut();
    void paste();
    void deleteSelectedText();

    void setFont(FontName fontName);
    void setFontSize(int size);
    Font* getFont() const { return m_font.get(); }

    void setAlignmentMode(AlignmentMode mode);
    AlignmentMode getAlignmentMode() const { return m_AlignmentMode; }

    void setOnTextChanged(OnTextChangedHandler handler);
    void setOnEnter(OnEnterHandler handler);

    void setFocused(bool focused);
    bool isFocused() const { return m_focused; }

    void setMargin(const Margin& margin);
    Margin getMargin() const { return m_margin; }
};
```

### 3.2 EditBoxBuilder 类

```cpp
class EditBoxBuilder {
private:
    shared_ptr<EditBox> m_editBox;
public:
    EditBoxBuilder(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    EditBoxBuilder& setBackgroundStateColor(StateColor stateColor);
    EditBoxBuilder& setBorderStateColor(StateColor stateColor);
    EditBoxBuilder& setTextStateColor(StateColor stateColor);

    EditBoxBuilder& setText(const std::string& text);
    EditBoxBuilder& setPlaceholder(const std::string& placeholder);
    EditBoxBuilder& setPasswordMode(bool enable);
    EditBoxBuilder& setPasswordChar(char c);
    EditBoxBuilder& setFont(FontName fontName);
    EditBoxBuilder& setFontSize(int size);
    EditBoxBuilder& setAlignmentMode(AlignmentMode mode);
    EditBoxBuilder& setOnTextChanged(EditBox::OnTextChangedHandler handler);
    EditBoxBuilder& setOnEnter(EditBox::OnEnterHandler handler);
    EditBoxBuilder& setId(int id);
    EditBoxBuilder& setTransparent(bool isTransparent);

    shared_ptr<EditBox> build(void);
};
```

## 4. 实现细节

### 4.1 光标闪烁

使用 `m_cursorBlinkTime` 记录上次光标显示时间，在 `update()` 中每帧递增固定步长（16ms），达到 500ms 间隔后切换光标可见性：

```cpp
void EditBox::update(void) {
    if (m_focused) {
        m_cursorBlinkTime += 16;
        if (m_cursorBlinkTime >= ConstDef::EDITBOX_CURSOR_BLINK_INTERVAL) {
            m_cursorVisible = !m_cursorVisible;
            m_cursorBlinkTime = 0;
        }
    }
}
```

### 4.2 密码模式

在密码模式下，显示密码字符而非实际文本：

```cpp
std::string EditBox::getDisplayText() const {
    if (m_passwordMode && !m_text.empty()) {
        return std::string(m_text.length(), m_passwordChar);
    }
    return m_text;
}
```

### 4.3 UTF-8 处理

支持 UTF-8 编码的文本输入，需要正确处理多字节字符：

```cpp
int EditBox::getUtf8CharLength(unsigned char c) {
    if ((c & 0x80) == 0) return 1;      // 1 字节字符
    if ((c & 0xE0) == 0xC0) return 2;  // 2 字节字符
    if ((c & 0xF0) == 0xE0) return 3;  // 3 字节字符
    if ((c & 0xF8) == 0xF0) return 4;    // 4 字节字符
    return 1;
}
```

### 4.4 事件处理（handleEvent）

使用基于 `EventType` 枚举 + union 字段的 Event API（Phase 12），通过 `event->m_type` 和 `event->mouseButton`/`event->mousePos`/`event->keyEvent`/`event->textInput` 等 union 成员访问事件数据，无需 `std::any_cast`：

```cpp
// MouseDown: 焦点获取、光标定位、选区拖拽起始
if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
    setFocused(true);
    // 通过 getCursorFromPosition() 将像素坐标转为文本偏移
    int newCursor = getCursorFromPosition(event->mouseButton.x - getDrawRect().left);
    ...
}

// MouseMove: 拖拽选区扩展
if (event->m_type == EventType::MouseMove) {
    if (m_focused && m_isDragging) {
        int newCursor = getCursorFromPosition(event->mousePos.x - getDrawRect().left);
        ...
    }
}

// TextInput: 过滤控制字符后调用 insertText()
if (event->m_type == EventType::TextInput) {
    std::string data(event->textInput.text);
    ...
    insertText(filtered);
}

// KeyDown: 方向键导航、编辑操作、快捷键
if (event->m_type == EventType::KeyDown) {
    const auto& key = event->keyEvent;
    // Ctrl+A/C/V/X, Backspace, Del, Left/Right, Home/End, Enter
    ...
}
```

### 4.5 文本选择和拖拽

- Shift + 方向键：扩展选区
- Ctrl + A：全选
- Ctrl + C：复制
- Ctrl + X：剪切
- Ctrl + V：粘贴
- MouseDown 后拖拽：通过 `m_isDragging` + `m_dragStartPosition` 实现连续选区

### 4.6 字体加载（loadFontInternal）

通过 `TextRenderer` 加载字体，避免直接使用 TTF API。使用 `ResourceProvider` 读取字体文件，`m_fontData` 持有 `shared_ptr<vector<char>>` 确保字体数据在 `TTF_OpenFontIO` 懒加载期间不被释放（Phase 8 修复）：

```cpp
void EditBox::loadFontInternal() {
    ResourceProvider* provider = getResourceProvider();
    m_fontData = provider->readFile(ConstDef::fontFiles.at(m_fontName));

    int scaledFontSize = (int)(m_fontSize * getScaleXX());
    m_font = getTextRenderer()->loadFontFromMemoryWithText(
        m_fontData->data(), m_fontData->size(), scaledFontSize, m_text);
}
```

在 `insertText()` 中，插入新文本后重新调用 `loadFontInternal()`，确保如 CJK 等新增码点被正确加载（适用于 raylib 后端的懒加载码点策略）。

## 5. 常量定义

在 `ConstDef.h` 中定义编辑框相关常量：

```cpp
static const float EDITBOX_DEFAULT_HEIGHT;
static const float EDITBOX_BORDER_WIDTH;
static const float EDITBOX_PADDING;
static const float EDITBOX_CURSOR_WIDTH;
static const int32_t EDITBOX_CURSOR_BLINK_INTERVAL;
static const SColor EDITBOX_SELECTION_COLOR;
static const char EDITBOX_DEFAULT_PASSWORD_CHAR;
```

## 6. 文件结构

```
UICornerstone/
├── include/
│   └── EditBox.h
├── src/
│   └── EditBox.cpp
└── CMakeLists.txt
```

## 7. 使用示例

```cpp
auto editBox = EditBoxBuilder(nullptr, SRect(100, 100, 200, 30))
    .setPlaceholder("Enter text...")
    .setOnTextChanged([](string text) {
        cout << "Text changed: " << text << endl;
    })
    .setOnEnter([]() {
        cout << "Enter pressed" << endl;
    })
    .build();
BENCH->addControl(editBox);
```

## 附录：历史重构记录

| Phase | 变更 | 说明 |
|-------|------|------|
| Phase 5 | `TTF_Font*` → `SharedFont` / `TextRenderer` | 移除直接 TTF API 依赖，通过 TextRenderer 抽象接口加载和渲染文字 |
| Phase 5 | 移除 `TTF_TextEngine*` / `TTF_Text*` | `createTextEngine()`/`createTextObjects()`/`recreateTextObjects()` 全部删除 |
| Phase 5 | 移除 `setRenderer(SDL_Renderer*)` | Renderer 职责全部移交 RenderDevice |
| Phase 8 | 新增 `m_fontData` | 使用 `shared_ptr<vector<char>>` 持有字体数据，解决 TTF_OpenFontIO 懒加载导致的内存释放崩溃 |
| Phase 8 | 新增 `loadFontInternal()` | 统一字体加载入口，ResourceProvider + TextRenderer 组合 |
| Phase 11 | 移除 EditBox.h 中 `#include <SDL3/SDL_keyboard.h>` | 头部无 SDL 类型依赖；`.cpp` 通过 MainWindow.h 间接获取 |
| Phase 12 | handleEvent 迁移至 union-based Event API | 移除全部 `std::any_cast`，使用 `EventType` + union 成员访问 |
| Phase 13 | cursor blink 使用帧计数替代 `SDL_GetTicks()` | `m_cursorBlinkTime += 16` 每帧固定步进，避免直接 SDL 时序依赖 |
