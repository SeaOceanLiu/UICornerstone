// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include "ScrollBar.h"
#include "MainWindow.h"
#include <algorithm>

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
{
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
    float localX = (x - drawRect.left) / getScaleXX();
    float localY = (y - drawRect.top) / getScaleXX();
    return localX >= m_thumbRect.left && localX <= m_thumbRect.left + m_thumbRect.width &&
           localY >= m_thumbRect.top && localY <= m_thumbRect.top + m_thumbRect.height;
}

bool ScrollBar::isPointInTrack(float x, float y) {
    SRect drawRect = getDrawRect();
    float localX = (x - drawRect.left) / getScaleXX();
    float localY = (y - drawRect.top) / getScaleXX();
    return localX >= 0 && localX <= m_trackRect.width &&
           localY >= 0 && localY <= m_trackRect.height;
}

void ScrollBar::notifyPositionChanged(float oldValue) {
    if (m_onPositionChanged) {
        m_onPositionChanged(dynamic_pointer_cast<ScrollBar>(getThis()), oldValue, m_value, m_minValue, m_maxValue);
    }
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

    ControlImpl::preDraw();

    SRect drawRect = getDrawRect();

    GET_RENDERDEVICE->setDrawColor(ConstDef::SCROLLBAR_TRACK_COLOR);
    GET_RENDERDEVICE->fillRect(drawRect);

    SColor thumbColor = ConstDef::SCROLLBAR_THUMB_COLOR;
    if (m_dragging) {
        thumbColor = ConstDef::SCROLLBAR_THUMB_PRESSED_COLOR;
    } else if (m_thumbHovered) {
        thumbColor = ConstDef::SCROLLBAR_THUMB_HOVER_COLOR;
    }

    float scale = getScaleXX();
    SRect thumbDrawRect(
        drawRect.left + m_thumbRect.left * scale,
        drawRect.top + m_thumbRect.top * scale,
        m_thumbRect.width * scale,
        m_thumbRect.height * scale
    );

    GET_RENDERDEVICE->setDrawColor(thumbColor);
    GET_RENDERDEVICE->fillRect(thumbDrawRect);
}

bool ScrollBar::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    SRect drawRect = getDrawRect();
    float scale = getScaleXX();

    if (event->m_eventName == EventName::MOUSE_LBUTTON_DOWN) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            float localX = (pos->x - drawRect.left) / scale;
            float localY = (pos->y - drawRect.top) / scale;

            if (isPointInThumb(pos->x, pos->y)) {
                m_dragging = true;
                m_thumbPressed = true;
                if (m_orientation == ScrollBarOrientation::Vertical) {
                    m_dragOffset = localY - m_thumbRect.top;
                } else {
                    m_dragOffset = localX - m_thumbRect.left;
                }
                return true;
            }

            if (isPointInTrack(pos->x, pos->y)) {
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
        } catch (...) {
            return false;
        }
    }

    if (event->m_eventName == EventName::MOUSE_LBUTTON_UP) {
        m_dragging = false;
        m_thumbPressed = false;
    }

    if (event->m_eventName == EventName::MOUSE_MOVING) {
        if (!m_dragging) return false;
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            float localX = (pos->x - drawRect.left) / scale;
            float localY = (pos->y - drawRect.top) / scale;

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
        } catch (...) {
            return false;
        }
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
