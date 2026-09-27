// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef ProgressBarH
#define ProgressBarH

#include <functional>
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Label.h"
#include "PropertyNames.h"

enum class ProgressBarStyle {
    Horizontal,
    Vertical
};

enum class ProgressBarTextMode {
    None,
    Percent,
    Custom
};

inline ProgressBarStyle ProgressBarStyleFromString(const char* s) {
    if (_stricmp(s, PropertyNames::kOrientHorizontal) == 0) return ProgressBarStyle::Horizontal;
    if (_stricmp(s, PropertyNames::kOrientVertical)   == 0) return ProgressBarStyle::Vertical;
    return ProgressBarStyle::Horizontal;
}

inline ProgressBarTextMode ProgressBarTextModeFromString(const char* s) {
    if (_stricmp(s, PropertyNames::kTextModeNone)    == 0) return ProgressBarTextMode::None;
    if (_stricmp(s, PropertyNames::kTextModePercent) == 0) return ProgressBarTextMode::Percent;
    if (_stricmp(s, PropertyNames::kTextModeCustom)  == 0) return ProgressBarTextMode::Custom;
    return ProgressBarTextMode::None;
}

class ProgressBar : public ControlImpl {
    friend class ProgressBarBuilder;
public:
    using OnValueChangedHandler = std::function<void (shared_ptr<ProgressBar>, float, float)>;

private:
    float m_minValue;
    float m_maxValue;
    float m_value;
    float m_animatedValue;

    ProgressBarStyle m_style;
    ProgressBarTextMode m_textMode;
    string m_customText;

    SColor m_progressColor;
    SColor m_backgroundColor;
    SColor m_textColor;
    float m_animationSpeed;

    FontName m_fontName;
    int m_fontSize;
    AlignmentMode m_alignmentMode;

    shared_ptr<Label> m_textLabel;
    OnValueChangedHandler m_onValueChanged;

private:
    void createTextLabel();
    void updateTextLabel();

public:
    ProgressBar(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    void update(void) override;
    shared_ptr<Label> getTextLabel(void) const;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void setValue(float value);
    float getValue() const;
    float getPercent() const;

    void setRange(float minValue, float maxValue);
    void setStyle(ProgressBarStyle style);
    ProgressBarStyle getStyle() const { return m_style; }
    void setTextMode(ProgressBarTextMode mode);
    ProgressBarTextMode getTextMode() const { return m_textMode; }
    void setCustomText(string text);
    string getCustomText() const { return m_customText; }

    void setProgressColor(SColor color);
    SColor getProgressColor() const { return m_progressColor; }
    void setBackgroundColor(SColor color);
    SColor getBackgroundColor() const { return m_backgroundColor; }
    void setTextColor(SColor color);
    SColor getTextColor() const { return m_textColor; }
    void setAnimationSpeed(float speed);
    float getAnimationSpeed() const { return m_animationSpeed; }

    void setFont(FontName fontName);
    void setFontSize(int fontSize);
    void setAlignmentMode(AlignmentMode mode);

    void setOnValueChanged(OnValueChangedHandler handler);

    // ── Property system overrides ──
    void setTextStateColor(StateColor stateColor) override;        // P0-26：转发 textLabel（四态）
    void setTextShadowStateColor(StateColor stateColor) override;  // P0-26
    int setColorProperty(const char* prop, SColor color) override;
    int setBoolProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setIntProperty(const char* prop, int value) override;
    int setStringProperty(const char* prop, const char* value) override;
    int setEnumProperty(const char* prop, const char* value) override;

    int getColorProperty(const char* prop, SColor& out) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getIntProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int getEnumProperty(const char* prop, const char*& out) override;
};

class ProgressBarBuilder {
private:
    shared_ptr<ProgressBar> m_progressBar;
public:
    ProgressBarBuilder(Control *parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    ProgressBarBuilder& setBackgroundStateColor(StateColor stateColor);
    ProgressBarBuilder& setBorderStateColor(StateColor stateColor);
    ProgressBarBuilder& setValue(float value);
    ProgressBarBuilder& setRange(float minValue, float maxValue);
    ProgressBarBuilder& setStyle(ProgressBarStyle style);
    ProgressBarBuilder& setTextMode(ProgressBarTextMode mode);
    ProgressBarBuilder& setCustomText(string text);
    ProgressBarBuilder& setProgressColor(SColor color);
    ProgressBarBuilder& setBackgroundColor(SColor color);
    ProgressBarBuilder& setTextColor(SColor color);
    ProgressBarBuilder& setAnimationSpeed(float speed);
    ProgressBarBuilder& setFont(FontName fontName);
    ProgressBarBuilder& setFontSize(int fontSize);
    ProgressBarBuilder& setAlignmentMode(AlignmentMode mode);
    ProgressBarBuilder& setOnValueChanged(ProgressBar::OnValueChangedHandler handler);
    ProgressBarBuilder& setId(int id);

    shared_ptr<ProgressBar> build(void);
};

#endif