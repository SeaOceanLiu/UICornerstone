// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef EditBoxH
#define EditBoxH

#include <functional>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_clipboard.h>
#include "ConstDef.h"
#include "ControlBase.h"
#include "Label.h"
#include "Utility.h"

struct TextInputEventData {
    std::string text;
    int32_t start;
    int32_t length;
};

struct KeyEventData {
    int32_t keycode;
    int32_t scancode;
    uint16_t mod;
    bool repeat;
};

struct FocusEventData {
    void* controlPtr;
    bool focused;
};

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

    TTF_Font *m_font;
    TTF_TextEngine *m_textEngine;
    TTF_Text *m_textObj;
    TTF_Text *m_placeholderTextObj;
    int m_fontSize;
    FontName m_fontName;

    TTF_TextEngine* getTextEngine() const { return m_textEngine; }

    OnTextChangedHandler m_onTextChanged;
    OnEnterHandler m_onEnter;
    bool m_focusWatcherRegistered;
    AlignmentMode m_AlignmentMode;

protected:
    void createTextEngine();
    void createTextObjects();
    void destroyTextObjects();
    std::string getDisplayText() const;
    float getTextWidth(const std::string& text) const;
    int getCursorFromPosition(float x) const;
    float getCursorX(int cursorPos) const;
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
    void setRenderer(SDL_Renderer *renderer) override;

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

    virtual void copy() const;
    virtual void cut();
    virtual void paste();
    virtual void deleteSelectedText();

    void setFont(FontName fontName);
    void setFontSize(int size);
    TTF_Font* getFont() const { return m_font; }

    void setAlignmentMode(AlignmentMode mode);
    AlignmentMode getAlignmentMode() const { return m_AlignmentMode; }

    void setOnTextChanged(OnTextChangedHandler handler);
    void setOnEnter(OnEnterHandler handler);

    void setFocused(bool focused);
    bool isFocused() const { return m_focused; }

    void setMargin(const Margin& margin);
    Margin getMargin() const { return m_margin; }

    void recreateTextObjects();
};

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

#endif
