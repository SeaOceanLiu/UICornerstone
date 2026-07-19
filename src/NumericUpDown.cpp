#define NOMINMAX
#include "NumericUpDown.h"
#include "GraphTool.h"
#include "InputBackend.h"
#include "PlatformUtils.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

NumericUpDown::NumericUpDown(Control* parent, const SRect& rect,
                             float xScale, float yScale)
    : EditBox(parent, rect, xScale, yScale)
    , m_minValue(0.0), m_maxValue(100.0), m_step(ConstDef::NUMERICUPDOWN_DEFAULT_STEP)
    , m_pageStep(ConstDef::NUMERICUPDOWN_DEFAULT_PAGESTEP)
    , m_value(0.0), m_committedValue(0.0)
    , m_decimals(ConstDef::NUMERICUPDOWN_DEFAULT_DECIMALS), m_readOnly(false)
    , m_btnUpPressed(false), m_btnDownPressed(false)
    , m_pressStartTime(0.0), m_lastRepeatTime(0.0)
    , m_arrowUpHovered(false), m_arrowDownHovered(false)
    , m_buttonWidth(ConstDef::NUMERICUPDOWN_BUTTON_WIDTH)
    , m_arrowColor(ConstDef::NUMERICUPDOWN_ARROW_COLOR)
    , m_arrowHoverColor(ConstDef::NUMERICUPDOWN_ARROW_HOVER_COLOR)
    , m_arrowPressColor(ConstDef::NUMERICUPDOWN_ARROW_PRESS_COLOR)
    , m_lastRect(), m_onValueChanged(nullptr)
{
    m_fontSize = ConstDef::NUMERICUPDOWN_FONT_SIZE;
    m_margin.right += m_buttonWidth;
    setNormalStateBGColor(ConstDef::NUMERICUPDOWN_BG_COLOR);
    setFocusable(true);
}

NumericUpDown::~NumericUpDown() {}

void NumericUpDown::create() {
    EditBox::create();
    EditBox::setText(formatValue(m_value));
    InputBackend* ib = getInputBackend();
    if (ib) ib->startTextInput();
}

void NumericUpDown::update() {
    EditBox::update();
    handleRepeat();
}

void NumericUpDown::draw() {
    EditBox::draw();
    auto* dev = GET_RENDERDEVICE;
    if (!dev) return;

    SRect dr = getDrawRect();
    float sx = getScaleXX();
    float aw = m_buttonWidth * sx;
    float arrowLeft = dr.right() - aw;
    float cx = dr.right() - aw * 0.5f;
    float cy = dr.top + dr.height / 2.0f;

    // Arrow area background (inset from edges to avoid covering focus ring)
    SColor arrowBg = m_btnUpPressed || m_btnDownPressed
        ? ConstDef::NUMERICUPDOWN_ARROW_BG_PRESS
        : (m_arrowUpHovered || m_arrowDownHovered
            ? ConstDef::NUMERICUPDOWN_ARROW_BG_HOVER
            : ConstDef::NUMERICUPDOWN_ARROW_BG);
    dev->setDrawColor(arrowBg);
    dev->fillRect({arrowLeft + 2, dr.top + 3, aw - 5, dr.height - 6});

    // Vertical separator
    dev->setDrawColor(ConstDef::NUMERICUPDOWN_SEPARATOR_COLOR);
    dev->drawLine(arrowLeft, dr.top + 3, arrowLeft, dr.bottom() - 3);

    // Triangles with gap and horizontal separator
    float triH = std::min(aw, dr.height) * ConstDef::NUMERICUPDOWN_TRIANGLE_HEIGHT_RATIO;
    float halfW = triH * ConstDef::NUMERICUPDOWN_TRIANGLE_WIDTH_RATIO;
    float gap = ConstDef::NUMERICUPDOWN_ARROW_GAP * sx;

    SColor upCol = m_btnUpPressed
        ? ConstDef::NUMERICUPDOWN_ARROW_PRESS_COLOR
        : (m_arrowUpHovered
            ? ConstDef::NUMERICUPDOWN_ARROW_HOVER_COLOR
            : ConstDef::NUMERICUPDOWN_ARROW_COLOR);
    dev->drawTriangle(cx, cy - gap - triH, cx - halfW, cy - gap, cx + halfW, cy - gap, upCol);

    SColor dnCol = m_btnDownPressed
        ? ConstDef::NUMERICUPDOWN_ARROW_PRESS_COLOR
        : (m_arrowDownHovered
            ? ConstDef::NUMERICUPDOWN_ARROW_HOVER_COLOR
            : ConstDef::NUMERICUPDOWN_ARROW_COLOR);
    dev->drawTriangle(cx - halfW, cy + gap, cx + halfW, cy + gap, cx, cy + gap + triH, dnCol);

    // Horizontal separator between triangles
    dev->setDrawColor(ConstDef::NUMERICUPDOWN_SEPARATOR_COLOR);
    dev->drawLine(arrowLeft + 4, cy, dr.right() - 4, cy);
}

bool NumericUpDown::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (m_readOnly) {
        if (event->m_type == EventType::MouseDown ||
            event->m_type == EventType::KeyDown ||
            event->m_type == EventType::TextInput)
            return false;
    }

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        if (isContainsPoint(event->mouseButton.x, event->mouseButton.y) &&
            isInArrowArea(event->mouseButton.x)) {
            if (isInUpArrow(event->mouseButton.x, event->mouseButton.y)) {
                handleArrowPress(true); setFocused(true); return true;
            }
            if (isInDownArrow(event->mouseButton.x, event->mouseButton.y)) {
                handleArrowPress(false); setFocused(true); return true;
            }
        }
    }

    if (event->m_type == EventType::MouseUp &&
        event->mouseButton.button == MouseButton::Left) {
        if (m_btnUpPressed || m_btnDownPressed) {
            handleArrowRelease(); return true;
        }
    }

    if (event->m_type == EventType::MouseMove) {
        if (isContainsPoint(event->mousePos.x, event->mousePos.y)) {
            m_arrowUpHovered = isInUpArrow(event->mousePos.x, event->mousePos.y);
            m_arrowDownHovered = isInDownArrow(event->mousePos.x, event->mousePos.y);
        } else {
            m_arrowUpHovered = m_arrowDownHovered = false;
        }
    }

    if (event->m_type == EventType::KeyDown && getFocused()) {
        if (m_readOnly) return true;
        switch (event->keyEvent.keycode) {
            case KeyCode::Up: case KeyCode::Plus: stepValue(+1); return true;
            case KeyCode::Down: case KeyCode::Minus: stepValue(-1); return true;
            case KeyCode::PageUp: setValue(m_value + m_pageStep); return true;
            case KeyCode::PageDown: setValue(m_value - m_pageStep); return true;
            case KeyCode::Home: setValue(m_minValue); return true;
            case KeyCode::End: setValue(m_maxValue); return true;
            case KeyCode::Return: commitEditBoxText(); return true;
            default: break;
        }
    }

    return EditBox::handleEvent(event);
}

void NumericUpDown::setRect(SRect rect) {
    if (rect.width < m_buttonWidth + ConstDef::NUMERICUPDOWN_MIN_WIDTH_PADDING)
        rect.width = m_buttonWidth + ConstDef::NUMERICUPDOWN_MIN_WIDTH_PADDING;
    if (rect == m_lastRect) return;
    m_lastRect = rect;
    EditBox::setRect(rect);
}

bool NumericUpDown::isInArrowArea(float x)  {
    SRect dr = getDrawRect();
    return x >= dr.right() - m_buttonWidth * getScaleXX() && x <= dr.right();
}

bool NumericUpDown::isInUpArrow(float x, float y)  {
    if (!isInArrowArea(x)) return false;
    SRect dr = getDrawRect();
    return y < dr.top + dr.height / 2.0f;
}

bool NumericUpDown::isInDownArrow(float x, float y)  {
    if (!isInArrowArea(x)) return false;
    SRect dr = getDrawRect();
    return y >= dr.top + dr.height / 2.0f;
}

void NumericUpDown::handleArrowPress(bool up) {
    if (up) { m_btnUpPressed = true; stepValue(+1); }
    else { m_btnDownPressed = true; stepValue(-1); }
    m_pressStartTime = m_lastRepeatTime = Platform::GetTicks() / 1000.0;
}

void NumericUpDown::handleArrowRelease() {
    m_btnUpPressed = false; m_btnDownPressed = false;
}

void NumericUpDown::handleRepeat() {
    if (!m_btnUpPressed && !m_btnDownPressed) return;
    double now = Platform::GetTicks() / 1000.0;
    if (now - m_pressStartTime < ConstDef::NUMERICUPDOWN_REPEAT_DELAY_SEC) return;
    if (now - m_lastRepeatTime >= ConstDef::NUMERICUPDOWN_REPEAT_INTERVAL_SEC) {
        stepValue(m_btnUpPressed ? +1 : -1);
        m_lastRepeatTime = now;
    }
}

void NumericUpDown::stepValue(int dir) { setValueInternal(m_value + dir * m_step, true); }
void NumericUpDown::setValue(double val) { setValueInternal(val, true); }

void NumericUpDown::setValueInternal(double val, bool fireCallback) {
    double oldCommitted = m_committedValue;
    m_value = clampAndSnap(val);
    EditBox::setText(formatValue(m_value));
    if (fireCallback) {
        m_committedValue = m_value;
        if (m_onValueChanged && m_committedValue != oldCommitted)
            m_onValueChanged(std::static_pointer_cast<NumericUpDown>(shared_from_this()), m_committedValue);
    }
}

void NumericUpDown::commitEditBoxText() {
    string text = EditBox::getText();
    if (text.empty()) { setValueInternal(m_minValue, true); return; }
    try {
        size_t pos = 0;
        double val = std::stod(text, &pos);
        if (pos != text.size() || std::isnan(val) || std::isinf(val)) throw std::invalid_argument("");
        setValueInternal(val, true);
    } catch (...) {
        EditBox::setText(formatValue(m_value));
    }
}

double NumericUpDown::clampAndSnap(double val) const {
    if (std::isnan(val)) return m_committedValue;
    val = std::clamp(val, m_minValue, m_maxValue);
    if (m_step > 0 && std::isfinite(m_minValue) && std::isfinite(m_maxValue)) {
        double n = std::round((val - m_minValue) / m_step);
        val = m_minValue + n * m_step;
    }
    if (m_decimals >= 0 && m_decimals <= ConstDef::NUMERICUPDOWN_DECIMALS_MAX) {
        double s = std::pow(10.0, m_decimals);
        val = std::round(val * s) / s;
    }
    return std::clamp(val, m_minValue, m_maxValue);
}

string NumericUpDown::formatValue(double v) const {
    char buf[64];
    if (m_decimals <= 0) snprintf(buf, sizeof(buf), "%lld", (long long)std::round(v));
    else snprintf(buf, sizeof(buf), "%.*f", m_decimals, v);
    return string(buf);
}

void NumericUpDown::onFocusLost() {
    EditBox::onFocusLost();
    commitEditBoxText();
    m_btnUpPressed = m_btnDownPressed = false;
}

void NumericUpDown::onFocusGained(bool byKeyboard) {
    EditBox::onFocusGained(byKeyboard);
    InputBackend* ib = getInputBackend();
    if (ib) ib->startTextInput();
}

void NumericUpDown::setRange(double minVal, double maxVal) {
    if (minVal > maxVal) std::swap(minVal, maxVal);
    m_minValue = minVal; m_maxValue = maxVal;
    setValue(m_value);
}

void NumericUpDown::setStep(double step) { m_step = step > 0 ? step : ConstDef::NUMERICUPDOWN_DEFAULT_STEP; }
void NumericUpDown::setPageStep(double ps) { m_pageStep = ps > 0 ? ps : ConstDef::NUMERICUPDOWN_DEFAULT_PAGESTEP; }
void NumericUpDown::setDecimals(int n) { m_decimals = std::clamp(n, 0, ConstDef::NUMERICUPDOWN_DECIMALS_MAX); }
void NumericUpDown::setReadOnly(bool ro) { m_readOnly = ro; }
void NumericUpDown::setButtonWidth(float w) { m_buttonWidth = w; m_margin.right = 4.0f + m_buttonWidth; }
void NumericUpDown::setArrowColor(SColor n, SColor h, SColor p) { m_arrowColor = n; m_arrowHoverColor = h; m_arrowPressColor = p; }
void NumericUpDown::setOnValueChanged(OnValueChangedHandler handler) { m_onValueChanged = handler; }

// ── Builder ──

NumericUpDownBuilder::NumericUpDownBuilder(Control* parent, const SRect& rect, float xScale, float yScale)
    : m_ctl(make_shared<NumericUpDown>(parent, rect, xScale, yScale)) {}

NumericUpDownBuilder& NumericUpDownBuilder::setValue(double val) { m_ctl->m_value = val; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setRange(double mn, double mx) { m_ctl->m_minValue = mn; m_ctl->m_maxValue = mx; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setStep(double s) { m_ctl->m_step = s; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setPageStep(double ps) { m_ctl->m_pageStep = ps; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setDecimals(int n) { m_ctl->m_decimals = std::clamp(n, 0, ConstDef::NUMERICUPDOWN_DECIMALS_MAX); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setPlaceholder(const string& p) { m_ctl->setPlaceholder(p); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setReadOnly(bool ro) { m_ctl->m_readOnly = ro; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setButtonWidth(float w) { m_ctl->m_buttonWidth = w; m_ctl->m_margin.right = 4.0f + w; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setArrowColor(SColor n, SColor h, SColor p) { m_ctl->m_arrowColor = n; m_ctl->m_arrowHoverColor = h; m_ctl->m_arrowPressColor = p; return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setBackgroundStateColor(StateColor sc) { m_ctl->setBackgroundStateColor(sc); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setBorderStateColor(StateColor sc) { m_ctl->setBorderStateColor(sc); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setTextStateColor(StateColor sc) { m_ctl->setTextStateColor(sc); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setFocusable(bool f) { m_ctl->setFocusable(f); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setFocusRingAlwaysVisible(bool v) { m_ctl->setFocusRingAlwaysVisible(v); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setFocusRingColor(SColor c) { m_ctl->setFocusRingColor(c); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setId(int id) { m_ctl->setId(id); return *this; }
NumericUpDownBuilder& NumericUpDownBuilder::setOnValueChanged(NumericUpDown::OnValueChangedHandler cb) { m_ctl->m_onValueChanged = cb; return *this; }

shared_ptr<NumericUpDown> NumericUpDownBuilder::build() { m_ctl->create(); return m_ctl; }
