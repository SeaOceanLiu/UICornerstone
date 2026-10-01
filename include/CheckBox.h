// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef CheckBoxH
#define CheckBoxH
#include <functional>
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Label.h"
#include "PropertyNames.h"

enum class CheckBoxStyle {
    Classic,
    Cross,
    Circle
};

enum class CheckState {
    Unchecked,
    Checked,
    Indeterminate
};

enum class CheckBoxLayout {
    TextRight,
    TextLeft
};

enum class CheckBoxVerticalAlign {
    Center,
    Top,
    Bottom
};

inline CheckBoxStyle CheckBoxStyleFromString(const char* s) {
    if (_stricmp(s, PropertyNames::kStyleClassic) == 0) return CheckBoxStyle::Classic;
    if (_stricmp(s, PropertyNames::kStyleCross)   == 0) return CheckBoxStyle::Cross;
    if (_stricmp(s, PropertyNames::kStyleCircle)  == 0) return CheckBoxStyle::Circle;
    return CheckBoxStyle::Classic;
}

inline CheckState CheckStateFromString(const char* s) {
    if (_stricmp(s, PropertyNames::kCheckUnchecked)     == 0) return CheckState::Unchecked;
    if (_stricmp(s, PropertyNames::kCheckChecked)       == 0) return CheckState::Checked;
    if (_stricmp(s, PropertyNames::kCheckIndeterminate) == 0) return CheckState::Indeterminate;
    return CheckState::Unchecked;
}

class CheckBox : public ControlImpl {
    friend class CheckBoxBuilder;
public:
    using OnCheckChangedHandler = std::function<void (shared_ptr<CheckBox>, CheckState, CheckState)>;

private:
    CheckState m_checkState;
    CheckBoxStyle m_style;
    CheckBoxLayout m_layout;
    CheckBoxVerticalAlign m_verticalAlign;

    shared_ptr<Label> m_caption;
    std::string m_captionText;   // caption 字符串读回稳定存储（getStringProperty）
    bool   m_enableTextShadow = false;            // 宿主存储（#12）：caption 重建后仍生效
    // #15/§3：caption 各态色宿主持久化（recreate 前快照、重建时应用；覆盖直控路径）
    StateColor m_capTextColor;
    StateColor m_capTextShadowColor;
    bool m_capColorsValid = false;
    SPoint m_shadowOffset{2.0f, 2.0f};            // 宿主存储（#12）：默认与既有 createCaption 一致
    OnCheckChangedHandler m_onCheckChanged;

    float m_sizeRatio;
    float m_captionSize;
    FontName m_fontName = FontName::HarmonyOS_Sans_SC_Regular;   // P032：标题字体名（读回）
    bool m_triStateEnabled;

    SRect m_boxRect;
    Margin m_boxMargin;

    StateColor m_checkStateColor;
    StateColor m_crossStateColor;
    StateColor m_indeterminateStateColor;
    StateColor m_boxBorderStateColor;
protected:
    void recreate(void) override;
public:
    CheckBox(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    void releaseCaption(void);
    void createCaption(void);
    shared_ptr<Label> getCaption(void) const;
    // #15：recreate 前快照 caption 状态（文本/字号/各态色/shadow），供 createCaption 重建应用
    void captureCaptionState(void);
    void create(void) override;
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    void setCheckState(CheckState state);
    CheckState getCheckState() const;
    void setStyle(CheckBoxStyle style);
    CheckBoxStyle getStyle() const;
    void setLayout(CheckBoxLayout layout);
    CheckBoxLayout getLayout() const;
    void setVerticalAlign(CheckBoxVerticalAlign align);
    CheckBoxVerticalAlign getVerticalAlign() const;

    void setSizeRatio(float ratio);
    float getSizeRatio() const;

    void setCaptionSize(float size);
    float getCaptionSize() const;

    void setTriStateEnabled(bool enabled);
    bool isTriStateEnabled() const;

    void setOnCheckChanged(OnCheckChangedHandler handler);

    void setCheckColor(SColor color);
    SColor getCheckColor();
    void setCrossColor(SColor color);
    SColor getCrossColor();
    void setIndeterminateColor(SColor color);
    SColor getIndeterminateColor();

    void setBoxBorderColor(SColor color);
    SColor getBoxBorderColor();

    // ── Property system overrides ──
    int setColorProperty(const char* prop, SColor color) override;
    int setBoolProperty(const char* prop, int value) override;
    int setIntProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setStringProperty(const char* prop, const char* value) override;   // caption（#9）
    void setFont(FontName font);   // P032
    int setEnumProperty(const char* prop, const char* value) override;

    int getColorProperty(const char* prop, SColor& out) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getIntProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getStringProperty(const char* prop, const char*& out) override;    // caption（#9）
    // 内部 caption Label 句柄暴露（caption-label → m_caption；与 Button 同键同约定）
    int getPtrProperty(const char* prop, void*& out) override;
    // 状态联动：CheckBox 状态变化时同步内部 caption Label
    void setState(ControlState state) override;
    int getEnumProperty(const char* prop, const char*& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;

private:
    void setBoxSize(void);
    void adjustSpaceAssignment(void);
    void adjustBoxVerticalAlign(void);
    float effectiveCaptionSize() const;

    // float calculateCheckBoxSize();
    // SRect calculateCheckBoxRect();
    // void calculateBoxAndCaptionRect();
    // void updateCaptionPosition();

    SRect getBoxDrawRect(); // for drawing
    void drawCheckBoxFrame();
    void drawCheckMark();
    void drawCrossMark();
    void drawIndeterminateMark();
};

class CheckBoxBuilder {
private:
    shared_ptr<CheckBox> m_checkBox;
public:
    CheckBoxBuilder(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    CheckBoxBuilder& setStyle(CheckBoxStyle style);
    CheckBoxBuilder& setLayout(CheckBoxLayout layout);
    CheckBoxBuilder& setVerticalAlign(CheckBoxVerticalAlign align);
    CheckBoxBuilder& setCheckState(CheckState state);
    CheckBoxBuilder& setSizeRatio(float ratio);
    CheckBoxBuilder& setCaptionText(string caption);
    CheckBoxBuilder& setCaptionSize(float size);
    CheckBoxBuilder& setTriStateEnabled(bool enabled);
    CheckBoxBuilder& setOnCheckChanged(CheckBox::OnCheckChangedHandler handler);
    CheckBoxBuilder& setCheckColor(SColor color);
    CheckBoxBuilder& setCrossColor(SColor color);
    CheckBoxBuilder& setIndeterminateColor(SColor color);
    CheckBoxBuilder& setBoxBorderColor(SColor color);
    CheckBoxBuilder& setBackgroundStateColor(StateColor stateColor);
    CheckBoxBuilder& setBorderStateColor(StateColor stateColor);
    CheckBoxBuilder& setTextStateColor(StateColor stateColor);
    CheckBoxBuilder& setId(int id);
    CheckBoxBuilder& setEnable(bool enable);
    shared_ptr<CheckBox> build(void);
};
#endif