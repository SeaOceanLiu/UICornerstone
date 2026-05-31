// 由A(MinMax V2.5)I生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include "ProgressBar.h"
#include "GraphTool.h"
#include <algorithm>

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
    m_rect = rect;
    setTransparent(true);
    setBorderVisible(false);
}

void ProgressBar::update(void) {
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

    ControlImpl::preDraw();

    SRect drawRect = getDrawRect();

    SDL_Renderer *renderer = getRenderer();
    if (!renderer) return;

    SDL_Color bgColor = getEnable() ? m_backgroundColor : ConstDef::DEFAULT_BORDER_DISABLED_COLOR;
    if (!SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a)) {
        return;
    }
    SDL_RenderFillRect(renderer, drawRect.toSDLFRect());

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

        SDL_Color progColor = getEnable() ? m_progressColor : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;
        if (!SDL_SetRenderDrawColor(renderer, progColor.r, progColor.g, progColor.b, progColor.a)) {
            return;
        }
        SDL_RenderFillRect(renderer, progressRect.toSDLFRect());
    }

    ControlImpl::draw();
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
    if (oldValue != m_value && m_onValueChanged) {
        m_onValueChanged(dynamic_pointer_cast<ProgressBar>(getThis()), oldValue, m_value);
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

void ProgressBar::setProgressColor(SDL_Color color) {
    m_progressColor = color;
}

void ProgressBar::setBackgroundColor(SDL_Color color) {
    m_backgroundColor = color;
}

void ProgressBar::setTextColor(SDL_Color color) {
    m_textColor = color;
    if (m_textLabel != nullptr) {
        StateColor sc;
        sc.setNormal(color);
        m_textLabel->setTextStateColor(sc);
    }
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
        SDL_Log("ProgressBar::createTextLabel: Text label already exists, reuse it");
        return;

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
        .setTextStateColor([&]() { StateColor sc; sc.setNormal(m_textColor); return sc; }())
        .build();

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

ProgressBarBuilder& ProgressBarBuilder::setProgressColor(SDL_Color color) {
    m_progressBar->setProgressColor(color);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setBackgroundColor(SDL_Color color) {
    m_progressBar->setBackgroundColor(color);
    return *this;
}

ProgressBarBuilder& ProgressBarBuilder::setTextColor(SDL_Color color) {
    m_progressBar->setTextColor(color);
    return *this;
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