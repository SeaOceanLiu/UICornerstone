// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef EditBoxH
#define EditBoxH

#include <string>
#include "ConstDef.h"
#include "ControlBase.h"
#include "Label.h"
#include "Utility.h"
#include "Font.h"
#include "EventTypes.h"

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
    bool m_readOnly = false;   // P0-83：只读（禁编辑/插入；保留选择/复制/滚动/光标/焦点）
    char m_passwordChar;
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
    bool m_fontScaleDirty;

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
    void create(void) override;  // 两阶段创建：挂树后加载字体
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    void refreshScaleWith(float parentXX, float parentYY) override;


    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    void setText(const std::string& text);
    std::string getText() const;
    int getCursorPosition() const { return m_cursorPosition; }
    void setPlaceholder(const std::string& placeholder);
    std::string getPlaceholder() const;

    void setPasswordMode(bool enable);
    bool isPasswordMode() const { return m_passwordMode; }
    // P0-83：只读（只读≠禁用——选择/复制/滚动/光标/焦点保留）
    void setReadOnly(bool ro) { m_readOnly = ro; }
    bool isReadOnly() const { return m_readOnly; }
    void setPasswordChar(char c);

    void selectAll();
    void setSelection(int start, int end);
    void clearSelection();
    std::string getSelectedText() const;
    bool hasSelection() const { return m_selectionStart != m_selectionEnd; }

    virtual void copy();
    virtual void cut();
    virtual void paste();
    virtual void deleteSelectedText();

    void setFont(FontName fontName);
    void setFontSize(int size);
    Font* getFont() const { return m_font.get(); }

    void setAlignmentMode(AlignmentMode mode);
    AlignmentMode getAlignmentMode() const { return m_AlignmentMode; }

    void setOnTextChanged(OnTextChangedHandler handler);
    void setOnEnter(OnEnterHandler handler);

    void setFocused(bool focused, bool byKeyboard = false) override;
    void onFocusGained(bool byKeyboard) override;
    void onFocusLost() override;
    bool isFocused() const { return getFocused(); }

    void setMargin(const Margin& margin);
    Margin getMargin() const { return m_margin; }

    // ── Property system overrides ──
    int setBoolProperty(const char* prop, int value) override;
    int setIntProperty(const char* prop, int value) override;
    int setStringProperty(const char* prop, const char* value) override;
    int setEnumProperty(const char* prop, const char* value) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getIntProperty(const char* prop, int& out) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int getEnumProperty(const char* prop, const char*& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;
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
