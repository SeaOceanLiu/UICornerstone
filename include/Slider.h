#ifndef SliderH
#define SliderH

#include <functional>
#include <vector>
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Label.h"
#include "EventQueue.h"

enum class SliderStyle {
    Horizontal,
    Vertical
};

class Slider : public ControlImpl {
    friend class SliderBuilder;
public:
    using OnValueChangedHandler = std::function<void(shared_ptr<Slider>, float value)>;

private:
    // Range
    float m_minValue;
    float m_maxValue;
    float m_step;
    float m_value;
    float m_committedValue;

    // Style
    SliderStyle m_style;
    bool        m_reverse;

    // Drag state
    bool   m_dragging;
    bool   m_thumbHovered;
    SPoint m_dragStartMouse;

    // Focus / Keyboard
    bool m_focused;
    bool m_focusWatcherRegistered;

    // Visual config
    float  m_trackThickness;
    float  m_thumbSize;
    SColor m_trackColor;
    SColor m_trackFillColor;
    SColor m_thumbColor;
    SColor m_thumbBorderColor;
    SColor m_thumbHoverColor;

    // Tick marks
    float  m_tickInterval;
    float  m_tickLength;
    SColor m_tickColor;
    SharedFont m_tickFont;
    shared_ptr<vector<char>> m_tickFontData;
    int    m_tickLabelFontSize;
    vector<void*> m_cachedTickTexts;
    bool   m_tickFontAttempted;

    // Value label
    bool          m_showValueLabel;
    shared_ptr<Label> m_valueLabel;
    FontName      m_labelFont;
    int           m_labelFontSize;
    SColor        m_labelColor;
    string        m_labelFormat;
    float         m_labelGap;

    // Keyboard repeat
    int    m_repeatKey;
    double m_repeatStartTime;
    double m_repeatNextTime;

    // Dirty-rect tracking
    SRect m_lastRect;

    // Callback
    OnValueChangedHandler m_onValueChanged;

private:
    float snapToStep(float value) const;
    float valueToOffset(float value);
    float offsetToValue(float offset);
    SRect getThumbRect();
    float getContentLength();
    void updateValueLabel();
    void commitValue();
    void setFocused(bool focused);
    void rebuildTickTexts();
    void destroyCachedTickTexts();
    void ensureTickFont();
    void repositionValueLabel();
    void handleKeyRepeat();
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;

public:
    Slider(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    ~Slider();
    void create(void) override;
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    // Range
    void  setRange(float minValue, float maxValue);
    float getMinValue() const { return m_minValue; }
    float getMaxValue() const { return m_maxValue; }

    void  setStep(float step);
    float getStep() const { return m_step; }

    void  setValue(float value);
    float getValue() const { return m_value; }
    float getPercent() const;

    // Style
    void setStyle(SliderStyle style);
    SliderStyle getStyle() const { return m_style; }
    void setReverse(bool reverse);
    bool getReverse() const { return m_reverse; }

    // Focus
    bool isFocused() const { return m_focused; }

    // Visual config
    void setTrackThickness(float thickness);
    float getTrackThickness() const { return m_trackThickness; }
    void setThumbSize(float size);
    float getThumbSize() const { return m_thumbSize; }
    void setTrackColor(SColor color);
    SColor getTrackColor() const { return m_trackColor; }
    void setTrackFillColor(SColor color);
    SColor getTrackFillColor() const { return m_trackFillColor; }
    void setThumbColor(SColor color);
    SColor getThumbColor() const { return m_thumbColor; }
    void setThumbBorderColor(SColor color);
    SColor getThumbBorderColor() const { return m_thumbBorderColor; }
    void setThumbHoverColor(SColor color);
    SColor getThumbHoverColor() const { return m_thumbHoverColor; }

    // Tick marks
    void setTickInterval(float interval);
    float getTickInterval() const { return m_tickInterval; }
    void setTickLength(float length);
    float getTickLength() const { return m_tickLength; }
    void setTickColor(SColor color);
    SColor getTickColor() const { return m_tickColor; }

    // Value label
    void setShowValueLabel(bool show);
    bool isShowValueLabel() const { return m_showValueLabel; }
    shared_ptr<Label> getValueLabel() const { return m_valueLabel; }
    void setLabelFont(FontName font);
    void setLabelFontSize(int size);
    void setLabelColor(SColor color);
    void setLabelFormat(const string& format);
    void setLabelGap(float gap);

    // Callback
    void setOnValueChanged(OnValueChangedHandler handler);
};

class SliderBuilder {
private:
    shared_ptr<Slider> m_slider;
public:
    SliderBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    SliderBuilder& setRange(float minValue, float maxValue);
    SliderBuilder& setValue(float value);
    SliderBuilder& setStep(float step);
    SliderBuilder& setStyle(SliderStyle style);
    SliderBuilder& setReverse(bool reverse);
    SliderBuilder& setTrackThickness(float thickness);
    SliderBuilder& setThumbSize(float size);
    SliderBuilder& setTrackColor(SColor color);
    SliderBuilder& setTrackFillColor(SColor color);
    SliderBuilder& setThumbColor(SColor color);
    SliderBuilder& setThumbBorderColor(SColor color);
    SliderBuilder& setThumbHoverColor(SColor color);
    SliderBuilder& setTickInterval(float interval);
    SliderBuilder& setTickLength(float length);
    SliderBuilder& setTickColor(SColor color);
    SliderBuilder& setShowValueLabel(bool show);
    SliderBuilder& setLabelFont(FontName font);
    SliderBuilder& setLabelFontSize(int size);
    SliderBuilder& setLabelColor(SColor color);
    SliderBuilder& setLabelFormat(const string& format);
    SliderBuilder& setLabelGap(float gap);
    SliderBuilder& setOnValueChanged(Slider::OnValueChangedHandler handler);
    SliderBuilder& setBackgroundStateColor(StateColor stateColor);
    SliderBuilder& setBorderStateColor(StateColor stateColor);
    SliderBuilder& setId(int id);

    shared_ptr<Slider> build(void);
};

#endif
