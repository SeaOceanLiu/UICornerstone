// 由AI生成，可能不完整或有错误，请自行检查和修改
#ifndef TextAreaH
#define TextAreaH

#include <vector>
#include <float.h>
#include "EditBox.h"
#include "ScrollBar.h"

struct MouseWheelEventData {
    float x;
    float y;
    float mouseX;
    float mouseY;
};

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

#endif
