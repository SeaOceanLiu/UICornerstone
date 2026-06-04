#ifndef LabelH
#define LabelH

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include "ConstDef.h"
#include "ControlBase.h"
#include "Font.h"

struct SDL_Cursor;

enum class LabelState {
    Normal,
    Hover,
    Pressed
};

enum class AlignmentMode: int{
    AM_TOP_LEFT,
    AM_MID_LEFT,
    AM_BOTTOM_LEFT,

    AM_TOP_RIGHT,
    AM_MID_RIGHT,
    AM_BOTTOM_RIGHT,

    AM_TOP_CENTER,
    AM_CENTER,
    AM_BOTTOM_CENTER
};

enum class LabelDirection: int{
    LD_LEFT_TO_RIGHT = 4,
    LD_RIGHT_TO_LEFT,
    LD_TOP_TO_BOTTOM,
    LD_BOTTOM_TO_TOP
};

class Label: public ControlImpl {
    friend class LabelBuilder;
    using OnClickHandler = std::function<void (shared_ptr<Label>)>;
    using OnPropertyChangedHandler = std::function<void (shared_ptr<Label>)>;
private:
    SharedFont m_font;
    shared_ptr<vector<char>> m_fontData;
    SPoint m_translatedPos;
    SDL_Cursor *m_hoverCursor;
    SDL_Cursor *m_defaultCursor;

    SPoint m_shadowOffset;
    AlignmentMode m_AlignmentMode;
    int m_fontSize;
    string m_caption;
    bool m_shadowEnabled;
    FontName m_fontName;
    fs::path m_fontFile;
    int m_fontStyle;
    SRect m_hotRect;

    OnClickHandler m_onClick;
    OnPropertyChangedHandler m_onPropertyChanged;

    std::vector<std::string> m_lines;
    std::vector<SPoint> m_lineOffsets;
    int m_defaultLineHeight;
    int m_lineHeight;
    bool m_enableExpand;
    SRect m_originalRect;
    bool m_debugDraw;
    float m_defaultLineSpacingRatio;
    int m_lineSpacing;

    int m_reentryCounter;

    std::vector<void*> m_cachedTexts;

private:
    void recreate() override;
    void releaseFont(void);
    void releaseTexts(void);
    void loadFromFile(void);
    void loadFromResource(string resourceId);
    void createMultilineText(void);
    void computeLineOffsets(void);
    void truncateLine(string& line, float maxWidth);
    SSize getTextSize(const string& text);
    float getStringWidth(const string& text);

public:
    Label(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    ~Label(void);
    void create(void) override;
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    void setParent(Control *parent) override;
    SRect getHotRect(void);

    void setCaption(string caption);
    string getCaption(void) const;
    void setFont(FontName fontName);
    void setAlignmentMode(AlignmentMode Alignment);
    AlignmentMode getAlignmentMode(void) const;
    void setMargin(Margin margin) override;
    void setFontSize(int fontSize);
    int getFontSize(void) const { return m_fontSize; }
    void setShadow(bool enabled);
    void setShadowOffset(SPoint offset);
    void setOnClick(OnClickHandler handler);
    void setOnPropertyChanged(OnPropertyChangedHandler handler);

    void setLineHeight(int height);
    int getLineHeight() const;

    void setLineSpacingRatio(float spacingRatio);
    float getLineSpacingRatio() const { return m_defaultLineSpacingRatio; }

    void setDebugDraw(bool enabled);
    bool getDebugDraw() const { return m_debugDraw; }

    void setEnableExpand(bool enable);
    bool getEnableExpand() const;

    void SetFontStyle(int fontStyle);
};

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
    LabelBuilder& setId(int id);

    LabelBuilder& setLineHeight(int height);

    LabelBuilder& setEnableExpand(bool enable);

    LabelBuilder& setBorderStateColor(StateColor stateColor);

    LabelBuilder& setDebugDraw(bool enabled);

    LabelBuilder& SetFontStyle(int fontStyle);

    shared_ptr<Label> build(void);
};
#endif  // LabelH