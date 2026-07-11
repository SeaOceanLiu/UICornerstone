#define NOMINMAX
#include "Slider.h"
#include "GraphTool.h"
#include "Cursor.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "PlatformUtils.h"

Slider::Slider(Control* parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale),
    m_minValue(0.0f),
    m_maxValue(100.0f),
    m_step(1.0f),
    m_value(0.0f),
    m_committedValue(0.0f),
    m_style(SliderStyle::Horizontal),
    m_reverse(false),
    m_dragging(false),
    m_thumbHovered(false),
    m_focusWatcherRegistered(false),
    m_trackThickness(ConstDef::SLIDER_TRACK_THICKNESS),
    m_thumbSize(ConstDef::SLIDER_THUMB_SIZE),
    m_trackColor(ConstDef::SLIDER_TRACK_COLOR),
    m_trackFillColor(ConstDef::SLIDER_TRACK_FILL_COLOR),
    m_thumbColor(ConstDef::SLIDER_THUMB_COLOR),
    m_thumbBorderColor(ConstDef::SLIDER_THUMB_BORDER_COLOR),
    m_thumbHoverColor(ConstDef::SLIDER_THUMB_HOVER_COLOR),
    m_tickInterval(ConstDef::SLIDER_TICK_INTERVAL),
    m_tickLength(ConstDef::SLIDER_TICK_LENGTH),
    m_tickColor(ConstDef::SLIDER_TICK_COLOR),
    m_tickFont(nullptr),
    m_tickFontData(nullptr),
    m_tickLabelFontSize(10),
    m_tickFontAttempted(false),
    m_showValueLabel(false),
    m_valueLabel(nullptr),
    m_labelFont(FontName::HarmonyOS_Sans_SC_Regular),
    m_labelFontSize(14),
    m_labelColor(ConstDef::SLIDER_LABEL_COLOR),
    m_labelFormat("%.0f"),
    m_labelGap(4.0f),
    m_repeatKey(0),
    m_repeatStartTime(0.0),
    m_repeatNextTime(0.0),
    m_lastRect(),
    m_onValueChanged(nullptr)
{
    m_rect = rect;
    setTransparent(true);
    setBorderVisible(false);
    setFocusable(true);
}

Slider::~Slider() {
    destroyCachedTickTexts();
}

void Slider::create(void)
{
    if (m_isCreated) return;
    ControlImpl::create();
    if (m_showValueLabel && !m_valueLabel) {
        m_valueLabel = make_shared<Label>(this, SRect());
        m_valueLabel->setFont(m_labelFont);
        m_valueLabel->setFontSize(m_labelFontSize);
        m_valueLabel->setTextNormalStateColor(m_labelColor);
        m_valueLabel->setEnableExpand(false);
        addControl(m_valueLabel);
        m_valueLabel->create();
        m_valueLabel->setVisible(true);
        // Set caption AFTER create() so font is ready for measurement
        updateValueLabel();
    } else {
        updateValueLabel();
    }
    // Re-apply rect to position the newly created value label
    setRect(m_rect);
    // Build tick text cache if applicable
    if (m_tickInterval > 0.0f && m_tickFont)
        rebuildTickTexts();
}

float Slider::snapToStep(float value) const
{
    if (m_step <= 0.0f) return value;
    float snapped = std::round((value - m_minValue) / m_step) * m_step + m_minValue;
    return std::clamp(snapped, m_minValue, m_maxValue);
}

float Slider::getContentLength()
{
    SRect dr = getDrawRect();
    if (m_style == SliderStyle::Horizontal)
        return dr.width - m_thumbSize * getScaleXX();
    else
        return dr.height - m_thumbSize * getScaleYY();
}

float Slider::valueToOffset(float value)
{
    float contentLen = getContentLength();
    if (contentLen <= 0.0f) return 0.0f;
    float p = (value - m_minValue) / (m_maxValue - m_minValue);
    p = std::clamp(p, 0.0f, 1.0f);
    if (m_reverse) p = 1.0f - p;
    return p * contentLen;
}

float Slider::offsetToValue(float offset)
{
    float contentLen = getContentLength();
    if (contentLen <= 0.0f) return m_minValue;
    float p = offset / contentLen;
    if (m_reverse) p = 1.0f - p;
    float rawValue = m_minValue + p * (m_maxValue - m_minValue);
    return std::clamp(rawValue, m_minValue, m_maxValue);
}

SRect Slider::getThumbRect()
{
    SRect dr = getDrawRect();
    float halfThumb = (m_thumbSize * getScaleXX()) / 2.0f;
    float offset = valueToOffset(m_value);

    if (m_style == SliderStyle::Horizontal) {
        float cx = dr.left + offset + halfThumb;
        float cy = dr.top + dr.height / 2.0f;
        float thumbW = m_thumbSize * getScaleXX();
        float thumbH = m_thumbSize * getScaleYY();
        return {cx - thumbW / 2.0f, cy - thumbH / 2.0f, thumbW, thumbH};
    } else {
        float cx = dr.left + dr.width / 2.0f;
        float cy = dr.top + offset + halfThumb;
        float thumbW = m_thumbSize * getScaleXX();
        float thumbH = m_thumbSize * getScaleYY();
        return {cx - thumbW / 2.0f, cy - thumbH / 2.0f, thumbW, thumbH};
    }
}

void Slider::setValue(float value)
{
    float v = std::clamp(value, m_minValue, m_maxValue);
    float snapped = snapToStep(v);
    if (std::abs(snapped - m_committedValue) > 0.001f) {
        m_committedValue = snapped;
        m_value = snapped;
        updateValueLabel();
        if (m_onValueChanged)
            m_onValueChanged(std::static_pointer_cast<Slider>(shared_from_this()), m_committedValue);
    } else if (std::abs(snapped - m_value) > 0.001f) {
        m_value = snapped;
        updateValueLabel();
    }
}

float Slider::getPercent() const
{
    if (m_maxValue <= m_minValue) return 0.0f;
    return (m_value - m_minValue) / (m_maxValue - m_minValue);
}

void Slider::commitValue()
{
    float snapped = snapToStep(m_value);
    m_value = snapped;
    updateValueLabel();
    if (std::abs(snapped - m_committedValue) > 0.001f) {
        m_committedValue = snapped;
        if (m_onValueChanged)
            m_onValueChanged(std::static_pointer_cast<Slider>(shared_from_this()), m_committedValue);
    }
}

void Slider::setFocused(bool focused, bool byKeyboard)
{
    if (m_focused == focused) return;
    if (focused) {
        if (!m_focusWatcherRegistered) {
            EventQueue::getInstance()->addBeforeEventHandlingWatcher(EventType::MouseDown, getThis());
            m_focusWatcherRegistered = true;
        }
    }
    ControlImpl::setFocused(focused, byKeyboard);
}

void Slider::onFocusLost()
{
    m_repeatKey = 0;
}

bool Slider::beforeEventHandlingWatcher(shared_ptr<Event> event)
{
    // On MouseDown outside slider → unfocus
    if (event->m_type == EventType::MouseDown && m_focused) {
        SRect dr = getDrawRect();
        SPoint mp(event->mouseButton.x, event->mouseButton.y);
        if (!dr.contains(mp.x, mp.y)) {
            setFocused(false);
        }
    }
    return false;
}

void Slider::rebuildTickTexts()
{
    destroyCachedTickTexts();
    if (m_tickInterval <= 0.0f || !m_tickFont) return;
    for (float v = m_minValue; v <= m_maxValue + 0.001f; v += m_tickInterval) {
        int intVal = static_cast<int>(v);
        string label = std::to_string(intVal);
        void* text = getTextRenderer()->createText(m_tickFont.get(), label);
        m_cachedTickTexts.push_back(text);
    }
}

void Slider::destroyCachedTickTexts()
{
    if (!m_cachedTickTexts.empty()) {
        for (void* t : m_cachedTickTexts) {
            if (t) getTextRenderer()->destroyText(t);
        }
        m_cachedTickTexts.clear();
    }
}

void Slider::ensureTickFont()
{
    if (m_tickFont || m_tickFontAttempted || !getResourceProvider()) return;
    m_tickFontAttempted = true;
    auto it = ConstDef::fontFiles.find(m_labelFont);
    if (it == ConstDef::fontFiles.end()) return;
    auto data = getResourceProvider()->readFile(it->second);
    if (!data || data->empty()) return;
    m_tickFontData = data;
    int scaledSize = std::max(1, static_cast<int>(m_tickLabelFontSize * getScaleXX()));
    m_tickFont = getTextRenderer()->loadFontFromMemory(data->data(), data->size(), scaledSize);
}

void Slider::handleKeyRepeat()
{
    if (m_repeatKey == 0 || !m_focused) return;
    double now = Platform::GetTicks();
    if (now < m_repeatNextTime) return;

    float range = m_maxValue - m_minValue;
    float step = m_step > 0.0f ? m_step : 1.0f;
    float pageStep = range * 0.1f;
    float newValue = m_value;
    bool handled = false;

    switch (m_repeatKey) {
        case static_cast<int>(KeyCode::Left):
        case static_cast<int>(KeyCode::Up):
            newValue = m_reverse ? m_value + step : m_value - step;
            handled = true;
            break;
        case static_cast<int>(KeyCode::Right):
        case static_cast<int>(KeyCode::Down):
            newValue = m_reverse ? m_value - step : m_value + step;
            handled = true;
            break;
        case static_cast<int>(KeyCode::PageUp):
            newValue = m_reverse ? m_value + pageStep : m_value - pageStep;
            handled = true;
            break;
        case static_cast<int>(KeyCode::PageDown):
            newValue = m_reverse ? m_value - pageStep : m_value + pageStep;
            handled = true;
            break;
    }

    if (handled) {
        newValue = std::clamp(newValue, m_minValue, m_maxValue);
        setValue(newValue);
        m_repeatNextTime = now + 50.0; // 50ms repeat interval
    }
}

void Slider::setRange(float minValue, float maxValue)
{
    m_minValue = std::min(minValue, maxValue);
    m_maxValue = std::max(minValue, maxValue);
    setValue(m_value);
    destroyCachedTickTexts();
}

void Slider::setStep(float step)
{
    m_step = std::max(0.0f, step);
    setValue(m_value);
}

void Slider::setStyle(SliderStyle style)
{
    m_style = style;
}

void Slider::setReverse(bool reverse)
{
    m_reverse = reverse;
}

void Slider::setTrackThickness(float thickness)
{
    m_trackThickness = std::max(1.0f, thickness);
}

void Slider::setThumbSize(float size)
{
    m_thumbSize = std::max(4.0f, size);
}

void Slider::setTrackColor(SColor color) { m_trackColor = color; }
void Slider::setTrackFillColor(SColor color) { m_trackFillColor = color; }
void Slider::setThumbColor(SColor color) { m_thumbColor = color; }
void Slider::setThumbBorderColor(SColor color) { m_thumbBorderColor = color; }
void Slider::setThumbHoverColor(SColor color) { m_thumbHoverColor = color; }

void Slider::setTickInterval(float interval)
{
    m_tickInterval = std::max(0.0f, interval);
    destroyCachedTickTexts();
}
void Slider::setTickLength(float length) { m_tickLength = std::max(1.0f, length); }
void Slider::setTickColor(SColor color) { m_tickColor = color; }

void Slider::setShowValueLabel(bool show)
{
    m_showValueLabel = show;
    if (show && !m_valueLabel && m_isCreated) {
        m_valueLabel = make_shared<Label>(this, SRect());
        m_valueLabel->setFont(m_labelFont);
        m_valueLabel->setFontSize(m_labelFontSize);
        m_valueLabel->setTextNormalStateColor(m_labelColor);
        m_valueLabel->setEnableExpand(false);
        addControl(m_valueLabel);
        m_valueLabel->create();
        m_valueLabel->setVisible(true);
        updateValueLabel();
    }
    if (m_valueLabel) m_valueLabel->setVisible(show);
}

void Slider::setLabelFont(FontName font)
{
    m_labelFont = font;
    if (m_valueLabel) m_valueLabel->setFont(font);
}

void Slider::setLabelFontSize(int size)
{
    m_labelFontSize = size;
    if (m_valueLabel) m_valueLabel->setFontSize(size);
}

void Slider::setLabelColor(SColor color)
{
    m_labelColor = color;
    if (m_valueLabel) m_valueLabel->setTextNormalStateColor(color);
}

void Slider::setLabelFormat(const string& format)
{
    m_labelFormat = format;
    updateValueLabel();
}

void Slider::updateValueLabel()
{
    if (!m_valueLabel) return;
    char buf[64];
    snprintf(buf, sizeof(buf), m_labelFormat.c_str(), m_value);
    m_valueLabel->setCaption(string(buf));
    repositionValueLabel();
}

void Slider::repositionValueLabel()
{
    if (!m_valueLabel) return;
    SRect dr = getDrawRect();
    SRect thumbR = getThumbRect();
    Margin m = m_valueLabel->getMargin();

    // Measure text (falls back to estimate if font not ready)
    float textW = 40.0f, textH = (float)m_labelFontSize;
    Font* font = m_valueLabel->getFont().get();
    string caption = m_valueLabel->getCaption();
    if (font && !caption.empty()) {
        SSize sz = getTextRenderer()->measureText(font, caption);
        textW = sz.width / getScaleXX();
        textH = sz.height / getScaleYY();
    }

    if (m_style == SliderStyle::Horizontal) {
        // Label rect must be wide/tall enough to avoid truncation
        float labelW = textW + m.left + m.right + 2.0f;
        float labelH = textH + m.top + m.bottom + 2.0f;
        // Thumb center (scaled absolute) → unscaled relative
        float thumbCX_scaled = thumbR.left + thumbR.width / 2.0f;
        float thumbCX_rel = (thumbCX_scaled - dr.left) / getScaleXX();
        // Text center = lx + m.left + textW/2, must equal thumbCX_rel
        float lx = thumbCX_rel - m.left - textW / 2.0f;
        // No clamp: label extends beyond slider boundaries when needed,
        // keeping text always centered on thumb.
        float ly = -labelH - m_labelGap;
        m_valueLabel->setRect({lx, ly, labelW, labelH});
    } else {
        // Label rect must be wide/tall enough to avoid truncation
        float labelW = textW + m.left + m.right + 2.0f;
        float labelH = textH + m.top + m.bottom + 2.0f;
        // Thumb center (scaled absolute) → unscaled relative
        float thumbCY_scaled = thumbR.top + thumbR.height / 2.0f;
        float thumbCY_rel = (thumbCY_scaled - dr.top) / getScaleYY();
        // Text center Y = ly + m.top + textH/2, must equal thumbCY_rel
        float ly = thumbCY_rel - m.top - textH / 2.0f;
        // No clamp: label extends beyond slider boundaries when needed,
        // keeping text always centered on thumb.
        float lx = m_rect.width + m_labelGap;
        m_valueLabel->setRect({lx, ly, labelW, labelH});
    }
}

void Slider::update(void)
{
    if (!getEnable()) return;
    ControlImpl::update();
    handleKeyRepeat();
}

void Slider::draw(void)
{
    if (!getVisible()) return;
    ControlImpl::beforeDraw();

    SRect dr = getDrawRect();
    auto* rd = GET_RENDERDEVICE;
    GraphTool::DrawingContext ctx(rd);

    float scaleX = getScaleXX();
    float scaleY = getScaleYY();

    float trackThick = m_trackThickness * std::min(scaleX, scaleY);
    float thumbS = m_thumbSize * std::min(scaleX, scaleY);

    SColor thumbCol = m_thumbHovered ? m_thumbHoverColor : m_thumbColor;

    if (m_style == SliderStyle::Horizontal) {
        float trackY = dr.top + dr.height / 2.0f - trackThick / 2.0f;
        SRect trackRect = {dr.left, trackY, dr.width, trackThick};

        // Background track
        rd->setDrawColor(m_trackColor);
        rd->fillRect(trackRect);

        // Tick marks + labels
        if (m_tickInterval > 0.0f) {
            ensureTickFont();
            float tickLen = m_tickLength * std::min(scaleX, scaleY);
            float tickTop = trackRect.top + trackRect.height;
            float tickBottom = tickTop + tickLen;
            rd->setDrawColor(m_tickColor);
            if (m_cachedTickTexts.empty() && m_tickFont)
                rebuildTickTexts();
            size_t idx = 0;
            for (float v = m_minValue; v <= m_maxValue; v += m_tickInterval) {
                float offset = valueToOffset(v);
                float tickX = dr.left + offset + thumbS / 2.0f;
                rd->drawLine(tickX, tickTop, tickX, tickBottom);
                if (idx < m_cachedTickTexts.size() && m_cachedTickTexts[idx]) {
                    SSize sz = getTextRenderer()->measureText(m_cachedTickTexts[idx]);
                    float lx = tickX - sz.width / 2.0f;
                    float ly = tickBottom + 2;
                    getTextRenderer()->drawText(m_cachedTickTexts[idx], lx, ly, m_tickColor);
                }
                ++idx;
            }
        }

        // Filled track
        SRect thumbR = getThumbRect();
        float thumbCx = thumbR.left + thumbR.width / 2.0f;
        rd->setDrawColor(m_trackFillColor);
        if (m_reverse) {
            // Fill from thumb center to right edge (min is on the right)
            float rightEdge = dr.left + dr.width;
            if (thumbCx < rightEdge) {
                rd->fillRect({thumbCx, trackY, rightEdge - thumbCx, trackThick});
            }
        } else {
            // Fill from left edge to thumb center (min is on the left)
            if (thumbCx > dr.left) {
                rd->fillRect({dr.left, trackY, thumbCx - dr.left, trackThick});
            }
        }

        // Thumb
        rd->setDrawColor(thumbCol);
        rd->fillRect(thumbR);
        rd->setDrawColor(m_thumbBorderColor);
        rd->drawRect(thumbR);
    } else {
        float trackX = dr.left + dr.width / 2.0f - trackThick / 2.0f;
        SRect trackRect = {trackX, dr.top, trackThick, dr.height};

        // Background track
        rd->setDrawColor(m_trackColor);
        rd->fillRect(trackRect);

        // Tick marks + labels
        if (m_tickInterval > 0.0f) {
            ensureTickFont();
            float tickLen = m_tickLength * std::min(scaleX, scaleY);
            float tickRight = trackRect.left;
            float tickLeft = tickRight - tickLen;
            rd->setDrawColor(m_tickColor);
            if (m_cachedTickTexts.empty() && m_tickFont)
                rebuildTickTexts();
            size_t idx = 0;
            for (float v = m_minValue; v <= m_maxValue; v += m_tickInterval) {
                float offset = valueToOffset(v);
                float tickY = dr.top + offset + thumbS / 2.0f;
                rd->drawLine(tickLeft, tickY, tickRight, tickY);
                if (idx < m_cachedTickTexts.size() && m_cachedTickTexts[idx]) {
                    SSize sz = getTextRenderer()->measureText(m_cachedTickTexts[idx]);
                    float lx = tickLeft - sz.width - 2;
                    float ly = tickY - sz.height / 2.0f;
                    getTextRenderer()->drawText(m_cachedTickTexts[idx], lx, ly, m_tickColor);
                }
                ++idx;
            }
        }

        // Filled track
        SRect thumbR = getThumbRect();
        float thumbCy = thumbR.top + thumbR.height / 2.0f;
        rd->setDrawColor(m_trackFillColor);
        if (m_reverse) {
            // Fill from thumb center to bottom edge (min is on the bottom)
            float bottomEdge = dr.top + dr.height;
            if (thumbCy < bottomEdge) {
                rd->fillRect({trackX, thumbCy, trackThick, bottomEdge - thumbCy});
            }
        } else {
            // Fill from top edge to thumb center (min is on the top)
            if (thumbCy > dr.top) {
                rd->fillRect({trackX, dr.top, trackThick, thumbCy - dr.top});
            }
        }

        // Thumb
        rd->setDrawColor(thumbCol);
        rd->fillRect(thumbR);
        rd->setDrawColor(m_thumbBorderColor);
        rd->drawRect(thumbR);
    }

    // Draw children (value label)
    ControlImpl::draw();

    ControlImpl::afterDraw();
}

bool Slider::handleEvent(shared_ptr<Event> event)
{
    if (!getVisible() || !getEnable()) return false;

    SPoint mousePos;
    bool hasPos = false;
    if (event->m_type == EventType::MouseMove) {
        mousePos = SPoint(event->mousePos.x, event->mousePos.y);
        hasPos = true;
    } else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mousePos = SPoint(event->mouseButton.x, event->mouseButton.y);
        hasPos = true;
    }

    // Refresh cursor on every MouseMove to survive WM_SETCURSOR reset
    if (hasPos && event->m_type == EventType::MouseMove) {
        SRect dr = getDrawRect();
        if (m_dragging) {
            // Dragging — update position and keep pointer cursor
            float halfThumb = (m_thumbSize * getScaleXX()) / 2.0f;
            float offset;
            if (m_style == SliderStyle::Horizontal) {
                offset = (mousePos.x - dr.left) - halfThumb;
            } else {
                offset = (mousePos.y - dr.top) - halfThumb;
            }
            float rawValue = offsetToValue(offset);
            m_value = rawValue;
            updateValueLabel();
            Cursor::setCurrent(Cursor::createSystem(SystemCursorType::Pointer));
            return true;
        }

        // Not dragging — hover detection
        SRect thumbR = getThumbRect();
        bool onThumb = thumbR.contains(mousePos.x, mousePos.y);
        m_thumbHovered = onThumb;
        Cursor::setCurrent(onThumb
            ? Cursor::createSystem(SystemCursorType::Pointer)
            : Cursor::getDefault());
        return false; // Don't consume — siblings may also handle hover
    }

    // MouseUp: commit drag value
    if (m_dragging && event->m_type == EventType::MouseUp) {
        m_dragging = false;
        m_thumbHovered = false;
        commitValue();
        SRect dr = getDrawRect();
        // After release, refresh cursor based on whether mouse is still on thumb
        if (hasPos && dr.contains(mousePos.x, mousePos.y)) {
            SRect thumbR = getThumbRect();
            if (thumbR.contains(mousePos.x, mousePos.y))
                Cursor::setCurrent(Cursor::createSystem(SystemCursorType::Pointer));
            else
                Cursor::setCurrent(Cursor::getDefault());
        } else {
            Cursor::setCurrent(Cursor::getDefault());
        }
        return true;
    }

    // MouseDown: start drag on thumb or click on track
    if (!m_dragging && hasPos && event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        SRect dr = getDrawRect();
        if (!dr.contains(mousePos.x, mousePos.y)) return false;

        setFocused(true);
        SRect thumbR = getThumbRect();
        if (thumbR.contains(mousePos.x, mousePos.y)) {
            m_dragging = true;
            m_dragStartMouse = mousePos;
            Cursor::setCurrent(Cursor::createSystem(SystemCursorType::Pointer));
            return true;
        }
        // Click on track — jump to position
        float halfThumb = (m_thumbSize * getScaleXX()) / 2.0f;
        float offset;
        if (m_style == SliderStyle::Horizontal) {
            offset = (mousePos.x - dr.left) - halfThumb;
        } else {
            offset = (mousePos.y - dr.top) - halfThumb;
        }
        float rawValue = offsetToValue(offset);
        setValue(rawValue);
        m_dragging = true;
        m_dragStartMouse = mousePos;
        Cursor::setCurrent(Cursor::createSystem(SystemCursorType::Pointer));
        return true;
    }

    // Keyboard: arrow keys, home/end, page up/down
    if (event->m_type == EventType::KeyDown && m_focused) {
        float range = m_maxValue - m_minValue;
        float step = m_step > 0.0f ? m_step : 1.0f;
        float pageStep = range * 0.1f;
        float newValue = m_value;
        bool handled = false;

        switch (event->keyEvent.keycode) {
            case KeyCode::Left:
            case KeyCode::Up:
                newValue = m_reverse ? m_value + step : m_value - step;
                handled = true;
                break;
            case KeyCode::Right:
            case KeyCode::Down:
                newValue = m_reverse ? m_value - step : m_value + step;
                handled = true;
                break;
            case KeyCode::Home:
                newValue = m_reverse ? m_maxValue : m_minValue;
                handled = true;
                break;
            case KeyCode::End:
                newValue = m_reverse ? m_minValue : m_maxValue;
                handled = true;
                break;
            case KeyCode::PageUp:
                newValue = m_reverse ? m_value + pageStep : m_value - pageStep;
                handled = true;
                break;
            case KeyCode::PageDown:
                newValue = m_reverse ? m_value - pageStep : m_value + pageStep;
                handled = true;
                break;
        }

        if (handled) {
            newValue = std::clamp(newValue, m_minValue, m_maxValue);
            setValue(newValue);
            // Start key repeat tracking
            m_repeatKey = static_cast<int>(event->keyEvent.keycode);
            double now = Platform::GetTicks();
            m_repeatStartTime = now;
            m_repeatNextTime = now + 350.0; // 350ms initial delay
            return true;
        }
    }

    // KeyUp: stop key repeat
    if (event->m_type == EventType::KeyUp && m_focused) {
        if (static_cast<int>(event->keyEvent.keycode) == m_repeatKey)
            m_repeatKey = 0;
    }

    // On loss of focus, stop repeat
    if (event->m_type == EventType::FocusLost && m_focused) {
        m_repeatKey = 0;
    }

    return false;
}

void Slider::setRect(SRect rect)
{
    if (rect == m_lastRect) return;
    m_lastRect = rect;
    ControlImpl::setRect(rect);
    repositionValueLabel();
}

void Slider::setLabelGap(float gap)
{
    m_labelGap = gap;
    repositionValueLabel();
}

void Slider::setOnValueChanged(OnValueChangedHandler handler)
{
    m_onValueChanged = handler;
}

// ==================== SliderBuilder ====================

SliderBuilder::SliderBuilder(Control* parent, SRect rect, float xScale, float yScale):
    m_slider(nullptr)
{
    m_slider = make_shared<Slider>(parent, rect, xScale, yScale);
}

SliderBuilder& SliderBuilder::setRange(float minValue, float maxValue)
{ m_slider->setRange(minValue, maxValue); return *this; }

SliderBuilder& SliderBuilder::setValue(float value)
{ m_slider->setValue(value); return *this; }

SliderBuilder& SliderBuilder::setStep(float step)
{ m_slider->setStep(step); return *this; }

SliderBuilder& SliderBuilder::setStyle(SliderStyle style)
{ m_slider->setStyle(style); return *this; }

SliderBuilder& SliderBuilder::setReverse(bool reverse)
{ m_slider->setReverse(reverse); return *this; }

SliderBuilder& SliderBuilder::setTrackThickness(float thickness)
{ m_slider->setTrackThickness(thickness); return *this; }

SliderBuilder& SliderBuilder::setThumbSize(float size)
{ m_slider->setThumbSize(size); return *this; }

SliderBuilder& SliderBuilder::setTrackColor(SColor color)
{ m_slider->setTrackColor(color); return *this; }

SliderBuilder& SliderBuilder::setTrackFillColor(SColor color)
{ m_slider->setTrackFillColor(color); return *this; }

SliderBuilder& SliderBuilder::setThumbColor(SColor color)
{ m_slider->setThumbColor(color); return *this; }

SliderBuilder& SliderBuilder::setThumbBorderColor(SColor color)
{ m_slider->setThumbBorderColor(color); return *this; }

SliderBuilder& SliderBuilder::setThumbHoverColor(SColor color)
{ m_slider->setThumbHoverColor(color); return *this; }

SliderBuilder& SliderBuilder::setTickInterval(float interval)
{ m_slider->setTickInterval(interval); return *this; }

SliderBuilder& SliderBuilder::setTickLength(float length)
{ m_slider->setTickLength(length); return *this; }

SliderBuilder& SliderBuilder::setTickColor(SColor color)
{ m_slider->setTickColor(color); return *this; }

SliderBuilder& SliderBuilder::setShowValueLabel(bool show)
{ m_slider->setShowValueLabel(show); return *this; }

SliderBuilder& SliderBuilder::setLabelFont(FontName font)
{ m_slider->setLabelFont(font); return *this; }

SliderBuilder& SliderBuilder::setLabelFontSize(int size)
{ m_slider->setLabelFontSize(size); return *this; }

SliderBuilder& SliderBuilder::setLabelColor(SColor color)
{ m_slider->setLabelColor(color); return *this; }

SliderBuilder& SliderBuilder::setLabelFormat(const string& format)
{ m_slider->setLabelFormat(format); return *this; }

SliderBuilder& SliderBuilder::setLabelGap(float gap)
{ m_slider->setLabelGap(gap); return *this; }

SliderBuilder& SliderBuilder::setOnValueChanged(Slider::OnValueChangedHandler handler)
{ m_slider->setOnValueChanged(handler); return *this; }

SliderBuilder& SliderBuilder::setBackgroundStateColor(StateColor stateColor)
{ m_slider->setBackgroundStateColor(stateColor); return *this; }

SliderBuilder& SliderBuilder::setBorderStateColor(StateColor stateColor)
{ m_slider->setBorderStateColor(stateColor); return *this; }

SliderBuilder& SliderBuilder::setId(int id)
{ m_slider->setId(id); return *this; }

shared_ptr<Slider> SliderBuilder::build(void)
{
    m_slider->create();
    if (!m_slider->getVisible())
        m_slider->setVisible(true);
    return m_slider;
}
