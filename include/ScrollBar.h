// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#ifndef ScrollBarH
#define ScrollBarH

#include <functional>
#include "ConstDef.h"
#include "ControlBase.h"

enum class ScrollBarOrientation {
    Vertical,
    Horizontal
};

class ScrollBar: public ControlImpl {
    friend class ScrollBarBuilder;
public:
    using OnPositionChangedHandler = std::function<void (shared_ptr<ScrollBar>, float, float, float, float)>;
private:
    ScrollBarOrientation m_orientation;
    float m_minValue;
    float m_maxValue;
    float m_value;
    float m_pageSize;
    float m_stepSize;
    float m_thickness;
    float m_minThumbLength;

    bool m_thumbHovered;
    bool m_thumbPressed;
    bool m_dragging;
    float m_dragOffset;

    SRect m_thumbRect;
    SRect m_trackRect;

    OnPositionChangedHandler m_onPositionChanged;

private:
    void calculateThumbRect();
    void calculateTrackRect();
    float valueToPosition(float value) const;
    float positionToValue(float position) const;
    bool isPointInThumb(float x, float y);
    bool isPointInTrack(float x, float y);
    void notifyPositionChanged(float oldValue = 0);

public:
    ScrollBar(Control *parent, SRect rect, ScrollBarOrientation orientation = ScrollBarOrientation::Vertical,
               float xScale = 1.0f, float yScale = 1.0f);
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    void setValue(float value);
    float getValue() const;
    void setRange(float minValue, float maxValue);
    void setPageSize(float pageSize);
    void setStepSize(float stepSize);
    void setOrientation(ScrollBarOrientation orientation);
    void setOnPositionChanged(OnPositionChangedHandler handler);

    float getThickness() const { return m_thickness; }
    void setThickness(float thickness);

    float getMinValue() const { return m_minValue; }
    float getMaxValue() const { return m_maxValue; }
    float getPageSize() const { return m_pageSize; }
    ScrollBarOrientation getOrientation() const { return m_orientation; }
    bool isDragging() const { return m_dragging; }
    bool shouldShow() const;
};

class ScrollBarBuilder {
private:
    shared_ptr<ScrollBar> m_scrollBar;
public:
    ScrollBarBuilder(Control *parent, SRect rect,
                     ScrollBarOrientation orientation = ScrollBarOrientation::Vertical,
                     float xScale = 1.0f, float yScale = 1.0f);

    ScrollBarBuilder& setBackgroundStateColor(StateColor stateColor);
    ScrollBarBuilder& setBorderStateColor(StateColor stateColor);
    ScrollBarBuilder& setValue(float value);
    ScrollBarBuilder& setRange(float minValue, float maxValue);
    ScrollBarBuilder& setPageSize(float pageSize);
    ScrollBarBuilder& setStepSize(float stepSize);
    ScrollBarBuilder& setThickness(float thickness);
    ScrollBarBuilder& setOnPositionChanged(ScrollBar::OnPositionChangedHandler handler);
    ScrollBarBuilder& setId(int id);

    shared_ptr<ScrollBar> build(void);
};

#endif
