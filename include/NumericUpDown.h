#ifndef NumericUpDownH
#define NumericUpDownH

#include <functional>
#include "SColor.h"
#include "EditBox.h"

class NumericUpDown : public EditBox {
    friend class NumericUpDownBuilder;
public:
    using OnValueChangedHandler = std::function<void(shared_ptr<NumericUpDown>, double newValue)>;

private:
    double m_minValue;
    double m_maxValue;
    double m_step;
    double m_pageStep;
    double m_value;
    double m_committedValue;
    int    m_decimals;
    bool   m_readOnly;

    bool   m_btnUpPressed;
    bool   m_btnDownPressed;
    double m_pressStartTime;
    double m_lastRepeatTime;

    bool   m_arrowUpHovered;
    bool   m_arrowDownHovered;

    float  m_buttonWidth;
    SColor m_arrowColor;
    SColor m_arrowHoverColor;
    SColor m_arrowPressColor;

    SRect  m_lastRect;

    OnValueChangedHandler m_onValueChanged;

public:
    NumericUpDown(Control* parent, const SRect& rect,
                  float xScale = 1.0f, float yScale = 1.0f);
    ~NumericUpDown() override;

    void create() override;
    void update() override;
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void   setValue(double val);
    double getValue() const { return m_value; }
    void   setRange(double minVal, double maxVal);
    std::pair<double, double> getRange() const { return {m_minValue, m_maxValue}; }
    void   setStep(double step);
    double getStep() const { return m_step; }
    void   setPageStep(double ps);
    double getPageStep() const { return m_pageStep; }
    void   setDecimals(int n);
    int    getDecimals() const { return m_decimals; }
    void   setReadOnly(bool ro);
    bool   isReadOnly() const { return m_readOnly; }
    void   setButtonWidth(float w);
    float  getButtonWidth() const { return m_buttonWidth; }
    void   setArrowColor(SColor normal, SColor hover, SColor press);
    SColor getArrowColor() const { return m_arrowColor; }
    void   setOnValueChanged(OnValueChangedHandler handler);
    void   stepValue(int dir);

private:
    bool isInArrowArea(float x) ;
    bool isInUpArrow(float x, float y) ;
    bool isInDownArrow(float x, float y) ;
    void handleArrowPress(bool up);
    void handleArrowRelease();
    void handleRepeat();
    void setValueInternal(double val, bool fireCallback);
    void commitEditBoxText();
    double clampAndSnap(double val) const;
    string formatValue(double v) const;
    void onFocusLost() override;
    void onFocusGained(bool byKeyboard) override;
};

class NumericUpDownBuilder {
private:
    shared_ptr<NumericUpDown> m_ctl;
public:
    NumericUpDownBuilder(Control* parent, const SRect& rect,
                         float xScale = 1.0f, float yScale = 1.0f);
    NumericUpDownBuilder& setValue(double val);
    NumericUpDownBuilder& setRange(double minVal, double maxVal);
    NumericUpDownBuilder& setStep(double step);
    NumericUpDownBuilder& setPageStep(double ps);
    NumericUpDownBuilder& setDecimals(int n);
    NumericUpDownBuilder& setPlaceholder(const string& p);
    NumericUpDownBuilder& setReadOnly(bool ro);
    NumericUpDownBuilder& setButtonWidth(float w);
    NumericUpDownBuilder& setArrowColor(SColor normal, SColor hover, SColor press);
    NumericUpDownBuilder& setBackgroundStateColor(StateColor sc);
    NumericUpDownBuilder& setBorderStateColor(StateColor sc);
    NumericUpDownBuilder& setTextStateColor(StateColor sc);
    NumericUpDownBuilder& setFocusable(bool focusable);
    NumericUpDownBuilder& setFocusRingAlwaysVisible(bool visible);
    NumericUpDownBuilder& setFocusRingColor(SColor color);
    NumericUpDownBuilder& setId(int id);
    NumericUpDownBuilder& setOnValueChanged(NumericUpDown::OnValueChangedHandler cb);
    shared_ptr<NumericUpDown> build();
    operator shared_ptr<NumericUpDown>() { return build(); }
};

#endif
