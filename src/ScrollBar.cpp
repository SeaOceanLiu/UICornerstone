// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include "ScrollBar.h"
#include "PropertyNames.h"
#include "MainWindow.h"
#include <algorithm>
#include <cstring>

ScrollBar::ScrollBar(Control *parent, SRect rect, ScrollBarOrientation orientation, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_orientation(orientation)
    , m_minValue(0.0f)
    , m_maxValue(100.0f)
    , m_value(0.0f)
    , m_pageSize(10.0f)
    , m_stepSize(1.0f)
    , m_thickness(16.0f)
    , m_minThumbLength(16.0f)
    , m_thumbHovered(false)
    , m_thumbPressed(false)
    , m_dragging(false)
    , m_dragOffset(0.0f)
    , m_trackColor(ConstDef::SCROLLBAR_TRACK_COLOR)
    , m_thumbColor(ConstDef::SCROLLBAR_THUMB_COLOR)
    , m_thumbHoverColor(ConstDef::SCROLLBAR_THUMB_HOVER_COLOR)
    , m_thumbPressedColor(ConstDef::SCROLLBAR_THUMB_PRESSED_COLOR)
{
    m_ctlType = ControlType::ScrollBar;
    m_id = 0;
    m_visible = true;
    m_enable = true;
    m_isBorderVisible = false;
    m_isTransparent = false;
    m_state = ControlState::Normal;

    setRect(rect);
    calculateTrackRect();
    calculateThumbRect();
}

void ScrollBar::setThickness(float thickness) {
    m_thickness = thickness;
    m_minThumbLength = thickness;
    calculateTrackRect();
    calculateThumbRect();
}

void ScrollBar::calculateTrackRect() {
    SRect rect = getRect();
    m_trackRect = SRect(0.0f, 0.0f, rect.width, rect.height);
}

void ScrollBar::calculateThumbRect() {
    float trackLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_trackRect.height : m_trackRect.width;
    float range = m_maxValue - m_minValue;

    float thumbLength;
    if (range <= 0) {
        thumbLength = trackLength;
    } else {
        float ratio = m_pageSize / (range + m_pageSize);
        thumbLength = trackLength * ratio;
        thumbLength = std::max(m_minThumbLength, thumbLength);
        thumbLength = std::min(thumbLength, trackLength);
    }

    float thumbTravel = trackLength - thumbLength;
    float thumbPos = 0;
    if (thumbTravel > 0 && range > 0) {
        thumbPos = (m_value - m_minValue) / range * thumbTravel;
    }
    thumbPos = std::max(0.0f, std::min(thumbPos, thumbTravel));

    if (m_orientation == ScrollBarOrientation::Vertical) {
        m_thumbRect = SRect(0, thumbPos, m_trackRect.width, thumbLength);
    } else {
        m_thumbRect = SRect(thumbPos, 0, thumbLength, m_trackRect.height);
    }
}

float ScrollBar::valueToPosition(float value) const {
    float trackLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_trackRect.height : m_trackRect.width;
    float range = m_maxValue - m_minValue;
    float thumbLength = m_thumbRect.height;
    if (m_orientation == ScrollBarOrientation::Horizontal) {
        thumbLength = m_thumbRect.width;
    }
    float thumbTravel = trackLength - thumbLength;

    if (range <= 0 || thumbTravel <= 0) return 0;

    return (value - m_minValue) / range * thumbTravel;
}

float ScrollBar::positionToValue(float position) const {
    float trackLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_trackRect.height : m_trackRect.width;
    float range = m_maxValue - m_minValue;
    float thumbLength = m_thumbRect.height;
    if (m_orientation == ScrollBarOrientation::Horizontal) {
        thumbLength = m_thumbRect.width;
    }
    float thumbTravel = trackLength - thumbLength;

    if (thumbTravel <= 0 || range <= 0) return m_minValue;

    float ratio = position / thumbTravel;
    return m_minValue + ratio * range;
}

bool ScrollBar::isPointInThumb(float x, float y) {
    SRect drawRect = getDrawRect();
    bool isVertical = (m_orientation == ScrollBarOrientation::Vertical);
    float localX = (x - drawRect.left) / getScaleXX();
    float localY = (y - drawRect.top) / (isVertical ? getScaleYY() : getScaleXX());
    return localX >= m_thumbRect.left && localX <= m_thumbRect.left + m_thumbRect.width &&
           localY >= m_thumbRect.top && localY <= m_thumbRect.top + m_thumbRect.height;
}

bool ScrollBar::isPointInTrack(float x, float y) {
    SRect drawRect = getDrawRect();
    bool isVertical = (m_orientation == ScrollBarOrientation::Vertical);
    float localX = (x - drawRect.left) / getScaleXX();
    float localY = (y - drawRect.top) / (isVertical ? getScaleYY() : getScaleXX());
    return localX >= 0 && localX <= m_trackRect.width &&
           localY >= 0 && localY <= m_trackRect.height;
}

void ScrollBar::notifyPositionChanged(float oldValue) {
    if (m_onPositionChanged) {
        m_onPositionChanged(dynamic_pointer_cast<ScrollBar>(getThis()), oldValue, m_value, m_minValue, m_maxValue);
    }
    fireCCallback(PropertyNames::kEventPositionChanged, CCallbackData::Float, &m_value);
}

bool ScrollBar::shouldShow() const {
    float trackLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_trackRect.height : m_trackRect.width;
    float range = m_maxValue - m_minValue;
    if (range <= 0) return false;
    float ratio = m_pageSize / (range + m_pageSize);
    float thumbLength = trackLength * ratio;
    return thumbLength >= m_minThumbLength;
}

void ScrollBar::update(void) {
}

void ScrollBar::draw(void) {
    if (!m_visible) return;

    ControlImpl::beforeDraw();

    SRect drawRect = getDrawRect();

    GET_RENDERDEVICE->setDrawColor(m_trackColor);
    GET_RENDERDEVICE->fillRect(drawRect);

    SColor thumbColor = m_thumbColor;
    if (m_dragging) {
        thumbColor = m_thumbPressedColor;
    } else if (m_thumbHovered) {
        thumbColor = m_thumbHoverColor;
    }

    float scaleX = getScaleXX();
    float scaleY = getScaleYY();
    SRect thumbDrawRect(
        drawRect.left + m_thumbRect.left * scaleX,
        drawRect.top + m_thumbRect.top * scaleY,
        m_thumbRect.width * scaleX,
        m_thumbRect.height * scaleY
    );

    GET_RENDERDEVICE->setDrawColor(thumbColor);
    GET_RENDERDEVICE->fillRect(thumbDrawRect);

    afterDraw();
}

bool ScrollBar::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    SRect drawRect = getDrawRect();
    float scaleX = getScaleXX();
    bool isVertical = (m_orientation == ScrollBarOrientation::Vertical);
    float scaleY = isVertical ? getScaleYY() : getScaleXX();

    // 滚轮：命中滚动条 → 步进 value（向上滚 scrollY=+1 → value 减；setValue 内部 clamp）
    if (event->m_type == EventType::MouseWheel) {
        SRect r = getDrawRect();
        float wx = event->mouseWheel.x, wy = event->mouseWheel.y;
        if (wx >= r.left && wx <= r.right() && wy >= r.top && wy <= r.bottom()) {
            float dir = (event->mouseWheel.scrollY > 0.f) ? -1.f : 1.f;
            setValue(m_value + dir * m_stepSize);
            return true;
        }
        return false;   // 不在滚动条上：不消费（Panel 容器回调可接手）
    }

    if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        float localX = (event->mouseButton.x - drawRect.left) / scaleX;
        float localY = (event->mouseButton.y - drawRect.top) / scaleY;

        if (isPointInThumb(event->mouseButton.x, event->mouseButton.y)) {
            m_dragging = true;
            m_thumbPressed = true;
            if (m_orientation == ScrollBarOrientation::Vertical) {
                m_dragOffset = localY - m_thumbRect.top;
            } else {
                m_dragOffset = localX - m_thumbRect.left;
            }
            return true;
        }

        if (isPointInTrack(event->mouseButton.x, event->mouseButton.y)) {
            float trackLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_trackRect.height : m_trackRect.width;
            float thumbLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_thumbRect.height : m_thumbRect.width;
            float thumbPos = (m_orientation == ScrollBarOrientation::Vertical) ? m_thumbRect.top : m_thumbRect.left;
            float clickPos = (m_orientation == ScrollBarOrientation::Vertical) ? localY : localX;

            float newValue;
            if (clickPos < thumbPos) {
                newValue = m_value - m_pageSize;
            } else {
                newValue = m_value + m_pageSize;
            }
            setValue(newValue);
            return true;
        }
    }

    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        m_dragging = false;
        m_thumbPressed = false;
    }

    if (event->m_type == EventType::MouseMove) {
        if (!m_dragging) return false;
        float localX = (event->mousePos.x - drawRect.left) / scaleX;
        float localY = (event->mousePos.y - drawRect.top) / scaleY;

        float trackLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_trackRect.height : m_trackRect.width;
        float thumbLength = (m_orientation == ScrollBarOrientation::Vertical) ? m_thumbRect.height : m_thumbRect.width;
        float thumbTravel = trackLength - thumbLength;

        float newPos;
        if (m_orientation == ScrollBarOrientation::Vertical) {
            newPos = localY - m_dragOffset;
        } else {
            newPos = localX - m_dragOffset;
        }
        newPos = std::max(0.0f, std::min(newPos, thumbTravel));

        float newValue = positionToValue(newPos);
        setValue(newValue);
        return true;
    }

    return false;
}

void ScrollBar::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    calculateTrackRect();
    calculateThumbRect();
}

void ScrollBar::onMouseEnter(float x, float y) {
    m_thumbHovered = isPointInThumb(x, y);
}

void ScrollBar::onMouseLeave(float x, float y) {
    m_thumbHovered = false;
}

void ScrollBar::setValue(float value) {
    float oldValue = m_value;
    m_value = std::max(m_minValue, std::min(value, m_maxValue));
    calculateThumbRect();
    notifyPositionChanged(oldValue);
}

float ScrollBar::getValue() const {
    return m_value;
}

void ScrollBar::setRange(float minValue, float maxValue) {
    float oldValue = m_value;
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_value = std::max(m_minValue, std::min(m_value, m_maxValue));
    calculateThumbRect();
    notifyPositionChanged(oldValue);
}

void ScrollBar::setPageSize(float pageSize) {
    float oldValue = m_value;
    m_pageSize = pageSize;
    calculateThumbRect();
    notifyPositionChanged(oldValue);
}

void ScrollBar::setStepSize(float stepSize) {
    m_stepSize = stepSize;
}

void ScrollBar::setOrientation(ScrollBarOrientation orientation) {
    m_orientation = orientation;
    calculateTrackRect();
    calculateThumbRect();
}

void ScrollBar::setOnPositionChanged(OnPositionChangedHandler handler) {
    m_onPositionChanged = handler;
}

ScrollBarBuilder::ScrollBarBuilder(Control *parent, SRect rect, ScrollBarOrientation orientation, float xScale, float yScale)
    : m_scrollBar(make_shared<ScrollBar>(parent, rect, orientation, xScale, yScale))
{
}

ScrollBarBuilder& ScrollBarBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_scrollBar->setBackgroundStateColor(stateColor);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setBorderStateColor(StateColor stateColor) {
    m_scrollBar->setBorderStateColor(stateColor);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setValue(float value) {
    m_scrollBar->setValue(value);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setRange(float minValue, float maxValue) {
    m_scrollBar->setRange(minValue, maxValue);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setPageSize(float pageSize) {
    m_scrollBar->setPageSize(pageSize);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setStepSize(float stepSize) {
    m_scrollBar->setStepSize(stepSize);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setThickness(float thickness) {
    m_scrollBar->setThickness(thickness);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setOnPositionChanged(ScrollBar::OnPositionChangedHandler handler) {
    m_scrollBar->setOnPositionChanged(handler);
    return *this;
}

ScrollBarBuilder& ScrollBarBuilder::setId(int id) {
    m_scrollBar->setId(id);
    return *this;
}

shared_ptr<ScrollBar> ScrollBarBuilder::build(void) {
    m_scrollBar->create();
    return m_scrollBar;
}

// ── Property system overrides ──

int ScrollBar::setColorProperty(const char* prop, SColor value) {
    if (strcmp(prop, PropertyNames::kTrack) == 0)          { m_trackColor = value; return 1; }
    if (strcmp(prop, PropertyNames::kThumb) == 0)          { m_thumbColor = value; return 1; }
    if (strcmp(prop, PropertyNames::kThumbHover) == 0)     { m_thumbHoverColor = value; return 1; }
    if (strcmp(prop, PropertyNames::kThumbPressed) == 0)   { m_thumbPressedColor = value; return 1; }
    return ControlImpl::setColorProperty(prop, value);
}

int ScrollBar::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kValue) == 0)              { setValue(value);           return 1; }
    if (strcmp(prop, PropertyNames::kRangeMin) == 0)           { m_minValue = value; setValue(m_value); return 1; }
    if (strcmp(prop, PropertyNames::kRangeMax) == 0)           { m_maxValue = value; setValue(m_value); return 1; }
    if (strcmp(prop, PropertyNames::kPageSize) == 0)           { setPageSize(value);        return 1; }
    if (strcmp(prop, PropertyNames::kStepSize) == 0)           { setStepSize(value);        return 1; }
    if (strcmp(prop, PropertyNames::kScrollbarThickness) == 0) { setThickness(value);       return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}

int ScrollBar::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kOrientation) == 0) {
        if (strcmp(value, PropertyNames::kOrientVertical) == 0)   { setOrientation(ScrollBarOrientation::Vertical);   return 1; }
        if (strcmp(value, PropertyNames::kOrientHorizontal) == 0) { setOrientation(ScrollBarOrientation::Horizontal); return 1; }
        return 0;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

int ScrollBar::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kTrack) == 0)          { out = m_trackColor;          return 1; }
    if (strcmp(prop, PropertyNames::kThumb) == 0)          { out = m_thumbColor;          return 1; }
    if (strcmp(prop, PropertyNames::kThumbHover) == 0)     { out = m_thumbHoverColor;     return 1; }
    if (strcmp(prop, PropertyNames::kThumbPressed) == 0)   { out = m_thumbPressedColor;   return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int ScrollBar::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kValue) == 0)              { out = m_value;    return 1; }
    if (strcmp(prop, PropertyNames::kRangeMin) == 0)           { out = m_minValue; return 1; }
    if (strcmp(prop, PropertyNames::kRangeMax) == 0)           { out = m_maxValue; return 1; }
    if (strcmp(prop, PropertyNames::kPageSize) == 0)           { out = m_pageSize; return 1; }
    if (strcmp(prop, PropertyNames::kStepSize) == 0)           { out = m_stepSize; return 1; }
    if (strcmp(prop, PropertyNames::kScrollbarThickness) == 0) { out = m_thickness; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

int ScrollBar::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kOrientation) == 0) {
        out = (m_orientation == ScrollBarOrientation::Horizontal) ? PropertyNames::kOrientHorizontal : PropertyNames::kOrientVertical;
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}

int ScrollBar::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventPositionChanged) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}
