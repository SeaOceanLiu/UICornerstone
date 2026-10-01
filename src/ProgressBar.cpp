// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include "ProgressBar.h"
#include "GraphTool.h"
#include "PropertyNames.h"
#include <algorithm>
#include "PlatformUtils.h"

ProgressBar::ProgressBar(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale),
    m_minValue(0.0f),
    m_maxValue(100.0f),
    m_value(0.0f),
    m_animatedValue(0.0f),
    m_style(ProgressBarStyle::Horizontal),
    m_textMode(ProgressBarTextMode::Percent),
    m_customText(""),
    m_progressColor(ConstDef::PROGRESSBAR_PROGRESS_COLOR),
    m_backgroundColor(ConstDef::PROGRESSBAR_BACKGROUND_COLOR),
    m_textColor(ConstDef::PROGRESSBAR_TEXT_COLOR),
    m_animationSpeed(ConstDef::PROGRESSBAR_ANIMATION_SPEED),
    m_fontName(FontName::HarmonyOS_Sans_SC_Regular),
    m_fontSize(ConstDef::PROGRESSBAR_DEFAULT_FONT_SIZE),
    m_alignmentMode(AlignmentMode::AM_CENTER),
    m_textLabel(nullptr),
    m_onValueChanged(nullptr)
{
    m_ctlType = ControlType::ProgressBar;
    m_rect = rect;
    setTransparent(true);
    setBorderVisible(false);
}

void ProgressBar::update(void) {
    if (m_textLabel) m_textLabel->setState(getState());   // P0-38：每帧持续同步（label 非交互，不再随自身 hotRect 抖动）
    if (!getEnable()) return;
    ControlImpl::update();

    if (std::abs(m_animatedValue - m_value) > 0.1f) {
        float diff = m_value - m_animatedValue;
        m_animatedValue += diff * m_animationSpeed;
        if (std::abs(m_value - m_animatedValue) < 0.1f) {
            m_animatedValue = m_value;
        }
        updateTextLabel();
    }
}
shared_ptr<Label> ProgressBar::getTextLabel(void) const {
    return m_textLabel;
}

void ProgressBar::draw(void) {
    if (!getVisible()) return;

    ControlImpl::beforeDraw();

    SRect drawRect = getDrawRect();

    GET_RENDERDEVICE->setDrawColor(getEnable() ? m_backgroundColor : ConstDef::DEFAULT_BORDER_DISABLED_COLOR);
    GET_RENDERDEVICE->fillRect(drawRect);

    float progressPercent = (m_animatedValue - m_minValue) / (m_maxValue - m_minValue);
    progressPercent = std::max(0.0f, std::min(1.0f, progressPercent));

    if (progressPercent > 0.0f) {
        SRect progressRect = drawRect;
        if (m_style == ProgressBarStyle::Horizontal) {
            progressRect.width = drawRect.width * progressPercent;
        } else {
            progressRect.height = drawRect.height * progressPercent;
            progressRect.top = drawRect.top + drawRect.height - progressRect.height;
        }

        GET_RENDERDEVICE->setDrawColor(getEnable() ? m_progressColor : ConstDef::DEFAULT_TEXT_DISABLED_COLOR);
        GET_RENDERDEVICE->fillRect(progressRect);
    }

    ControlImpl::draw();
    afterDraw();
}

bool ProgressBar::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;
    return ControlImpl::handleEvent(event);
}

void ProgressBar::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    updateTextLabel();
}

void ProgressBar::setValue(float value) {
    float oldValue = m_value;
    m_value = std::max(m_minValue, std::min(m_maxValue, value));
    if (oldValue != m_value) {
        if (m_onValueChanged)
            m_onValueChanged(dynamic_pointer_cast<ProgressBar>(getThis()), oldValue, m_value);
        fireCCallback(PropertyNames::kEventValueChanged, CCallbackData::Float, &m_value);
    }
}

float ProgressBar::getValue() const {
    return m_value;
}

float ProgressBar::getPercent() const {
    if (m_maxValue - m_minValue == 0.0f) return 0.0f;
    return (m_value - m_minValue) / (m_maxValue - m_minValue) * 100.0f;
}

void ProgressBar::setRange(float minValue, float maxValue) {
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_value = std::max(m_minValue, std::min(m_maxValue, m_value));
    updateTextLabel();
}

void ProgressBar::setStyle(ProgressBarStyle style) {
    m_style = style;
}

void ProgressBar::setTextMode(ProgressBarTextMode mode) {
    m_textMode = mode;
    if (mode == ProgressBarTextMode::None) {
        m_textLabel.reset();
    } else {
        createTextLabel();
    }
}

void ProgressBar::setCustomText(string text) {
    m_customText = text;
    updateTextLabel();
}

void ProgressBar::setProgressColor(SColor color) {
    m_progressColor = color;
}

void ProgressBar::setBackgroundColor(SColor color) {
    m_backgroundColor = color;
}

void ProgressBar::setTextColor(SColor color) {
    m_textColor = color;
    m_textStateColor.setNormal(color);   // P0-35②：快照同步
    if (m_textLabel != nullptr) {
        m_textLabel->setTextStateColor(m_textStateColor);
    }
}

void ProgressBar::setState(ControlState state) {
    ControlImpl::setState(state);
    if (m_textLabel) m_textLabel->setState(state);   // P0-35②：label 不自行检测，跟随控件态
}

void ProgressBar::setAnimationSpeed(float speed) {
    m_animationSpeed = std::max(0.01f, std::min(1.0f, speed));
}

void ProgressBar::setFont(FontName fontName) {
    m_fontName = fontName;
    if (m_textLabel != nullptr) {
        m_textLabel->setFont(fontName);
    }
}

void ProgressBar::setFontSize(int fontSize) {
    m_fontSize = fontSize;
    if (m_textLabel != nullptr) {
        m_textLabel->setFontSize(fontSize);
    }
}

void ProgressBar::setAlignmentMode(AlignmentMode mode) {
    m_alignmentMode = mode;
    if (m_textLabel != nullptr) {
        m_textLabel->setAlignmentMode(mode);
    }
}

void ProgressBar::setOnValueChanged(OnValueChangedHandler handler) {
    m_onValueChanged = handler;
}

void ProgressBar::createTextLabel() {
    if (m_textLabel != nullptr) {
        Platform::Log("ProgressBar::createTextLabel: Text label already exists, reuse it");
        removeControl(m_textLabel);
        m_textLabel.reset();
    }

    SRect labelRect;
    labelRect.left = 0;
    labelRect.top = 0;
    labelRect.width = m_rect.width - ConstDef::PROGRESSBAR_TEXT_MARGIN * 2;
    labelRect.height = m_rect.height;

    string displayText;
    if (m_textMode == ProgressBarTextMode::Percent) {
        int percent = (int)((m_animatedValue - m_minValue) / (m_maxValue - m_minValue) * 100.0f);
        displayText = std::to_string(percent) + "%";
    } else {
        displayText = m_customText;
    }

    m_textLabel = LabelBuilder(nullptr, labelRect)
        .setFont(m_fontName)
        .setAlignmentMode(m_alignmentMode)
        .setFontSize(m_fontSize)
        .setCaption(displayText)
        .setTextStateColor(m_textStateColor)          // P0-35②：三态快照（重建不丢）
        .setTextShadowStateColor(m_textShadowStateColor)
        .setClickable(false)                          // P0-38：内部标签非交互，状态由控件统一驱动
        .build();

    m_textLabel->setState(getState());   // P0-35②：状态同步
    addControl(m_textLabel);
}

void ProgressBar::updateTextLabel() {
    if (m_textLabel == nullptr) {
        if (m_textMode != ProgressBarTextMode::None) {
            createTextLabel();
        }
        return;
    }

    string displayText;
    if (m_textMode == ProgressBarTextMode::Percent) {
        int percent = (int)((m_animatedValue - m_minValue) / (m_maxValue - m_minValue) * 100.0f);
        displayText = std::to_string(percent) + "%";
    } else {
        displayText = m_customText;
    }

    m_textLabel->setCaption(displayText);
    // P0-35①：尺寸变化后同步 label 矩形（保持居中）
    m_textLabel->setRect(SRect(0, 0, m_rect.width - ConstDef::PROGRESSBAR_TEXT_MARGIN * 2, m_rect.height));
    m_textLabel->setState(getState());
}

/****************************************************************************for Builder mode****************************************************************************/

ProgressBarBuilder::ProgressBarBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_progressBar(nullptr)
{
    m_progressBar = make_shared<ProgressBar>(parent, rect, xScale, yScale);
}

ProgressBarBuilder& ProgressBarBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_progressBar->setBackgroundStateColor(stateColor);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setBorderStateColor(StateColor stateColor) {
    m_progressBar->setBorderStateColor(stateColor);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setValue(float value) {
    m_progressBar->setValue(value);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setRange(float minValue, float maxValue) {
    m_progressBar->setRange(minValue, maxValue);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setStyle(ProgressBarStyle style) {
    m_progressBar->setStyle(style);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setTextMode(ProgressBarTextMode mode) {
    m_progressBar->setTextMode(mode);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setCustomText(string text) {
    m_progressBar->setCustomText(text);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setProgressColor(SColor color) {
    m_progressBar->setProgressColor(color);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setBackgroundColor(SColor color) {
    m_progressBar->setBackgroundColor(color);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setTextColor(SColor color) {
    m_progressBar->setTextColor(color);
    return *this;
}

// ── Property system overrides ──

int ProgressBar::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kProgress) == 0)   { setProgressColor(color);   return 1; }
    if (strcmp(prop, PropertyNames::kBackground) == 0) { setBackgroundColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kText) == 0)       { setTextColor(color);       return 1; }
    // P0-35②③：文本/阴影单态键 → 三态快照 + label
    if (strcmp(prop, PropertyNames::kTextHover) == 0)    { m_textStateColor.setHover(color);    setTextStateColor(m_textStateColor); return 1; }
    if (strcmp(prop, PropertyNames::kTextPressed) == 0)  { m_textStateColor.setPressed(color);  setTextStateColor(m_textStateColor); return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0) { m_textStateColor.setDisabled(color); setTextStateColor(m_textStateColor); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)   { m_textShadowStateColor.setNormal(color);   setTextShadowStateColor(m_textShadowStateColor); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadowHover) == 0)   { m_textShadowStateColor.setHover(color);    setTextShadowStateColor(m_textShadowStateColor); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadowPressed) == 0) { m_textShadowStateColor.setPressed(color);  setTextShadowStateColor(m_textShadowStateColor); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadowDisabled) == 0){ m_textShadowStateColor.setDisabled(color); setTextShadowStateColor(m_textShadowStateColor); return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

void ProgressBar::setTextStateColor(StateColor stateColor) {   // P0-26：文本三态转发 textLabel
    ControlImpl::setTextStateColor(stateColor);
    m_textStateColor = stateColor;   // P0-35②：快照（重建不丢）
    updateTextLabel();
    if (m_textLabel) {
        m_textLabel->setTextStateColor(stateColor);
        m_textLabel->setState(getState());
    }
}
void ProgressBar::setTextShadowStateColor(StateColor stateColor) {
    ControlImpl::setTextShadowStateColor(stateColor);
    m_textShadowStateColor = stateColor;   // P0-35③：快照
    updateTextLabel();
    if (m_textLabel) m_textLabel->setTextShadowStateColor(stateColor);
}

int ProgressBar::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // P0-26：textLabel 阴影开关（懒建后转发）
        updateTextLabel();
        if (m_textLabel) { m_textLabel->setShadow(value != 0); return 1; }
        return 0;
    }
    return ControlImpl::setBoolProperty(prop, value);
}

int ProgressBar::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) {   // P0-26：textLabel 阴影偏移（懒建后转发）
        updateTextLabel();
        if (m_textLabel) { SPoint o = m_textLabel->getShadowOffset(); o.x = value; m_textLabel->setShadowOffset(o); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
        updateTextLabel();
        if (m_textLabel) { SPoint o = m_textLabel->getShadowOffset(); o.y = value; m_textLabel->setShadowOffset(o); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kValue) == 0)          { setValue(value);                return 1; }
    if (strcmp(prop, PropertyNames::kRangeMin) == 0)       { setRange(value, m_maxValue);    return 1; }
    if (strcmp(prop, PropertyNames::kRangeMax) == 0)       { setRange(m_minValue, value);    return 1; }
    if (strcmp(prop, PropertyNames::kAnimationSpeed) == 0) { setAnimationSpeed(value);       return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}

int ProgressBar::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { setFontSize(value); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}

int ProgressBar::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kTextContent) == 0) {   // P0-27c：text 键直达（自动切 custom 模式）
        setCustomText(value ? value : "");
        setTextMode(ProgressBarTextMode::Custom);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kCustomText) == 0) { setCustomText(value); return 1; }
    return ControlImpl::setStringProperty(prop, value);
}

int ProgressBar::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kProgressBarStyle) == 0) {
        setStyle(ProgressBarStyleFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kTextMode) == 0) {
        setTextMode(ProgressBarTextModeFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        setFont(FontNameFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kAlign) == 0) {
        if (_stricmp(value, PropertyNames::kAlignLowerTopLeft) == 0)      { setAlignmentMode(AlignmentMode::AM_TOP_LEFT);      return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerMidLeft) == 0)      { setAlignmentMode(AlignmentMode::AM_MID_LEFT);      return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerBottomLeft) == 0)   { setAlignmentMode(AlignmentMode::AM_BOTTOM_LEFT);   return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerTopRight) == 0)     { setAlignmentMode(AlignmentMode::AM_TOP_RIGHT);     return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerMidRight) == 0)     { setAlignmentMode(AlignmentMode::AM_MID_RIGHT);     return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerBottomRight) == 0)  { setAlignmentMode(AlignmentMode::AM_BOTTOM_RIGHT);  return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerTopCenter) == 0)    { setAlignmentMode(AlignmentMode::AM_TOP_CENTER);    return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerCenter) == 0)        { setAlignmentMode(AlignmentMode::AM_CENTER);        return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerBottomCenter) == 0) { setAlignmentMode(AlignmentMode::AM_BOTTOM_CENTER); return 1; }
        return 0;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

// ── Getter property overrides ──

int ProgressBar::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kProgress) == 0)    { out = m_progressColor;    return 1; }
    if (strcmp(prop, PropertyNames::kBackground) == 0)  { out = m_backgroundColor;  return 1; }
    if (strcmp(prop, PropertyNames::kText) == 0)         { out = m_textColor;        return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int ProgressBar::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { out = m_fontSize; return 1; }
    return ControlImpl::getIntProperty(prop, out);
}

int ProgressBar::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) {
        if (m_textLabel) { out = m_textLabel->isShadowEnabled() ? 1 : 0; return 1; }
        return 0;
    }
    return ControlImpl::getBoolProperty(prop, out);
}

int ProgressBar::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) {
        if (m_textLabel) { out = m_textLabel->getShadowOffset().x; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
        if (m_textLabel) { out = m_textLabel->getShadowOffset().y; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kValue) == 0)          { out = m_value;          return 1; }
    if (strcmp(prop, PropertyNames::kRangeMin) == 0)       { out = m_minValue;       return 1; }
    if (strcmp(prop, PropertyNames::kRangeMax) == 0)       { out = m_maxValue;       return 1; }
    if (strcmp(prop, PropertyNames::kAnimationSpeed) == 0) { out = m_animationSpeed; return 1; }
    if (strcmp(prop, PropertyNames::kPercent) == 0)        { out = getPercent();     return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

int ProgressBar::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCustomText) == 0 ||
        strcmp(prop, PropertyNames::kTextContent) == 0) { out = m_customText.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}

int ProgressBar::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kProgressBarStyle) == 0) {
        out = (m_style == ProgressBarStyle::Horizontal) ? PropertyNames::kOrientHorizontal : PropertyNames::kOrientVertical;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kTextMode) == 0) {
        if (m_textMode == ProgressBarTextMode::None)    out = PropertyNames::kTextModeNone;
        else if (m_textMode == ProgressBarTextMode::Percent) out = PropertyNames::kTextModePercent;
        else out = PropertyNames::kTextModeCustom;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        out = FontNameToString(m_fontName);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kAlign) == 0) {
        switch (m_alignmentMode) {
            case AlignmentMode::AM_TOP_LEFT:      out = PropertyNames::kAlignLowerTopLeft;      break;
            case AlignmentMode::AM_MID_LEFT:      out = PropertyNames::kAlignLowerMidLeft;      break;
            case AlignmentMode::AM_BOTTOM_LEFT:   out = PropertyNames::kAlignLowerBottomLeft;   break;
            case AlignmentMode::AM_TOP_RIGHT:     out = PropertyNames::kAlignLowerTopRight;     break;
            case AlignmentMode::AM_MID_RIGHT:     out = PropertyNames::kAlignLowerMidRight;     break;
            case AlignmentMode::AM_BOTTOM_RIGHT:  out = PropertyNames::kAlignLowerBottomRight;  break;
            case AlignmentMode::AM_TOP_CENTER:    out = PropertyNames::kAlignLowerTopCenter;    break;
            case AlignmentMode::AM_CENTER:        out = PropertyNames::kAlignLowerCenter;        break;
            case AlignmentMode::AM_BOTTOM_CENTER: out = PropertyNames::kAlignLowerBottomCenter; break;
            default: out = PropertyNames::kAlignLowerCenter; break;
        }
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}

ProgressBarBuilder& ProgressBarBuilder::setAnimationSpeed(float speed) {
    m_progressBar->setAnimationSpeed(speed);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setFont(FontName fontName) {
    m_progressBar->setFont(fontName);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setFontSize(int fontSize) {
    m_progressBar->setFontSize(fontSize);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setAlignmentMode(AlignmentMode mode) {
    m_progressBar->setAlignmentMode(mode);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setOnValueChanged(ProgressBar::OnValueChangedHandler handler) {
    m_progressBar->setOnValueChanged(handler);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setId(int id) {
    m_progressBar->setId(id);
    return *this;
}

shared_ptr<ProgressBar> ProgressBarBuilder::build(void) {
    m_progressBar->create();
    return m_progressBar;
}