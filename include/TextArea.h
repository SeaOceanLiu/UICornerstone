// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef TextAreaH
#define TextAreaH

#include <vector>
#include <float.h>
#include "EditBox.h"
#include "ScrollBar.h"

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
    bool m_lineHeightCustom;
    Font* m_lastFontForLineHeight;

    std::string m_lastTextForRebuild;

private:
    void rebuildLines();
    void refreshLineHeightFromFont();
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
    void refreshScaleWith(float parentXX, float parentYY) override;

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
    void setAutoScroll(bool enable);
    bool isAutoScroll() const { return m_autoScroll; }
    void setLineHeight(int height);
    int getLineHeight() const { return m_lineHeight; }
    void setScrollBarThickness(float thickness);
    float getScrollBarThickness() const;

    void setOnTextChangedHandler(OnTextChangedHandler handler);

    // ── Property system overrides ──
    int setBoolProperty(const char* prop, int value) override;
    int setIntProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getIntProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;
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
