// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef CheckBoxH
#define CheckBoxH
#include <functional>
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Label.h"

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
    OnCheckChangedHandler m_onCheckChanged;

    float m_sizeRatio;
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

private:
    void setBoxSize(void);
    void adjustSpaceAssignment(void);
    void adjustBoxVerticalAlign(void);

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