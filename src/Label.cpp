#include "Label.h"
#include "PlatformUtils.h"

Label::Label(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale)
    , m_font(nullptr)
    , m_shadowOffset({2, 2})
    , m_AlignmentMode(AlignmentMode::AM_TOP_LEFT)
    , m_fontSize(16)
    , m_caption("")
    , m_shadowEnabled(false)
    , m_fontName(FontName::HarmonyOS_Sans_SC_Regular)
    , m_fontStyle(0)
    , m_hotRect({0, 0, 0, 0})
    , m_onClick(nullptr)
    , m_onPropertyChanged(nullptr)
    , m_defaultLineHeight(0)
    , m_lineHeight(0)
    , m_enableExpand(true)
    , m_originalRect(rect)
    , m_debugDraw(false)
    , m_defaultLineSpacingRatio(0)
    , m_lineSpacing(0)
    , m_reentryCounter(0)
{
    m_rect = rect;
    m_margin = ConstDef::LABEL_CAPTION_MARGIN;
    setVisible(false);
    setTransparent(true);
    setBorderVisible(false);

    m_hoverCursor = Cursor::createSystem(SystemCursorType::Pointer);
    if (m_hoverCursor == nullptr) {
        Platform::Log("Label::Label: Failed to create cursor");
    }
    m_defaultCursor = Cursor::getDefault();

    setFont(m_fontName);
}

Label::~Label(void){
    releaseTexts();
    m_font.reset();

    delete m_hoverCursor;
    m_hoverCursor = nullptr;
}

void Label::releaseFont(void) {
    m_font.reset();
    m_fontData.reset();
}

void Label::releaseTexts(void) {
    TextRenderer* renderer = getTextRenderer();
    if (renderer) {
        for (auto* t : m_cachedTexts) {
            renderer->destroyText(t);
        }
    }
    m_cachedTexts.clear();
}

void Label::recreate() {
    if(!m_isCreated) return;

    releaseTexts();
    m_lines.clear();

    releaseFont();

    if(typeid(*this) == typeid(Label)) {
        m_isCreated = false;

        create();

        if (m_onPropertyChanged != nullptr){
            m_reentryCounter++;
            if (m_reentryCounter == 1) {
                m_onPropertyChanged(dynamic_pointer_cast<Label>(this->getThis()));
            }
            m_reentryCounter--;
        }
    }
}
void Label::create(void) {
    if (m_isCreated) return;

    loadFromResource(m_fontFile.string());
    createMultilineText();

    TextRenderer* renderer = getTextRenderer();
    if (renderer == nullptr) {
        Platform::Log("Label::create: No text renderer available");
        return;
    }

    int fontHeight = 0;
    if (m_font) {
        fontHeight = renderer->getFontHeight(m_font.get());
    }
    if (m_defaultLineHeight > 0) {
        m_lineHeight = m_defaultLineHeight;
    } else {
        m_lineHeight = static_cast<int>(fontHeight / getScaleYY());
    }
    if (m_defaultLineSpacingRatio > 0) {
        m_lineSpacing = static_cast<int>(m_lineHeight * m_defaultLineSpacingRatio);
    } else {
        m_lineSpacing = static_cast<int>(m_lineHeight * 0.2f);
    }

    for (const auto& line : m_lines) {
        void* textObj = renderer->createText(m_font.get(), line);
        m_cachedTexts.push_back(textObj);
    }

    computeLineOffsets();

    ControlImpl::create();
}

void Label::computeLineOffsets(void) {
    m_lineOffsets.clear();

    int lineSpacing = m_lineSpacing;
    SRect marginRect = getMarginedRect();
    float availableWidth = marginRect.width;
    float availableHeight = marginRect.height;

    float lineOffsetX = static_cast<float>(marginRect.left);
    float lineOffsetY = static_cast<float>(marginRect.top);
    float maxLineWidth = 0;
    bool isHeightTruncated = false;

    TextRenderer* renderer = getTextRenderer();

    for (size_t i = 0; i < m_lines.size(); ++i) {
        string& processedLine = m_lines[i];
        if (!m_enableExpand) {
            bool isWidthTruncated = false;
            if (availableWidth > 0 && renderer && m_cachedTexts.size() > i && m_cachedTexts[i]) {
                SSize lineSize = renderer->measureText(m_cachedTexts[i]);
                lineSize.width /= getScaleXX();
                lineSize.height /= getScaleYY();
                if (lineSize.width > availableWidth) {
                    truncateLine(processedLine, availableWidth);
                    renderer->destroyText(m_cachedTexts[i]);
                    m_cachedTexts[i] = renderer->createText(m_font.get(), processedLine);
                    isWidthTruncated = true;
                }
            }
            if (m_lines.size() > 1 && availableHeight > 0 &&
                (lineOffsetY + m_lineHeight + lineSpacing + m_lineHeight - marginRect.top) > availableHeight) {
                if (!isWidthTruncated) {
                    processedLine = "...";
                    if (renderer && m_cachedTexts.size() > i) {
                        renderer->destroyText(m_cachedTexts[i]);
                        m_cachedTexts[i] = renderer->createText(m_font.get(), processedLine);
                    }
                }
                isHeightTruncated = true;
            }
        }

        m_lineOffsets.push_back({lineOffsetX, lineOffsetY});

        SSize textSize(0, 0);
        if (renderer && m_cachedTexts.size() > i && m_cachedTexts[i]) {
            textSize = renderer->measureText(m_cachedTexts[i]);
            textSize.width /= getScaleXX();
            textSize.height /= getScaleYY();
        }
        if (textSize.width > maxLineWidth) maxLineWidth = textSize.width;

        if (isHeightTruncated) break;

        lineOffsetY = lineOffsetY + m_lineHeight + lineSpacing;
    }

    if (m_lineOffsets.empty()) return;

    float totalHeight = (m_lineHeight + lineSpacing) * static_cast<float>(m_lineOffsets.size()) - static_cast<float>(lineSpacing);

    float hotRectLeft = marginRect.right();

    for (size_t i = 0; i < m_lineOffsets.size(); ++i) {
        SSize textSize(0, 0);
        if (renderer && m_cachedTexts.size() > i && m_cachedTexts[i]) {
            textSize = renderer->measureText(m_cachedTexts[i]);
            textSize.width /= getScaleXX();
            textSize.height /= getScaleYY();
        }
        float lineWidth = textSize.width;

        switch (m_AlignmentMode) {
            case AlignmentMode::AM_TOP_RIGHT:
            case AlignmentMode::AM_MID_RIGHT:
            case AlignmentMode::AM_BOTTOM_RIGHT:
                m_lineOffsets[i].x = marginRect.left + (availableWidth - lineWidth);
                break;
            case AlignmentMode::AM_TOP_CENTER:
            case AlignmentMode::AM_CENTER:
            case AlignmentMode::AM_BOTTOM_CENTER:
                m_lineOffsets[i].x = marginRect.left + (availableWidth - lineWidth) / 2;
                break;
            default:
                break;
        }

        switch (m_AlignmentMode) {
            case AlignmentMode::AM_CENTER:
            case AlignmentMode::AM_MID_LEFT:
            case AlignmentMode::AM_MID_RIGHT:
                m_lineOffsets[i].y = marginRect.top + (availableHeight - totalHeight) / 2 + (m_lineHeight + lineSpacing) * static_cast<float>(i);
                break;
            case AlignmentMode::AM_BOTTOM_LEFT:
            case AlignmentMode::AM_BOTTOM_CENTER:
            case AlignmentMode::AM_BOTTOM_RIGHT:
                m_lineOffsets[i].y = marginRect.top + (availableHeight - totalHeight) + (m_lineHeight + lineSpacing) * static_cast<float>(i);
                break;
            default:
                break;
        }

        if (hotRectLeft > m_lineOffsets[i].x) {
            hotRectLeft = m_lineOffsets[i].x;
        }
    }

    m_hotRect = {hotRectLeft, m_lineOffsets[0].y, maxLineWidth, totalHeight};
}

void Label::createMultilineText(void) {
    m_lines.clear();
    size_t start = 0;
    while (true) {
        size_t pos = m_caption.find('\n', start);
        if (pos == string::npos) {
            m_lines.push_back(m_caption.substr(start));
            break;
        } else {
            m_lines.push_back(m_caption.substr(start, pos - start));
            start = pos + 1;
        }
    }
}

void Label::truncateLine(string& line, float maxWidth) {
    if (maxWidth <= 0) {
        line = "";
        return;
    }

    const string ellipsis3 = "...";
    const string ellipsis2 = "..";
    const string ellipsis1 = ".";

    float w3 = getStringWidth(ellipsis3);
    float w2 = getStringWidth(ellipsis2);
    float w1 = getStringWidth(ellipsis1);

    if (maxWidth >= w3) {
        int low = 0;
        int high = static_cast<int>(line.length());
        while (low < high) {
            int mid = (low + high + 1) / 2;
            string test = line.substr(0, mid) + ellipsis3;
            if (getStringWidth(test) <= maxWidth) {
                low = mid;
            } else {
                high = mid - 1;
            }
        }
        line = line.substr(0, low) + ellipsis3;
    } else if (maxWidth >= w2) {
        line = ellipsis2;
    } else if (maxWidth >= w1) {
        line = ellipsis1;
    } else {
        line = "";
    }
}

SSize Label::getTextSize(const string& text) {
    if (m_font == nullptr || text.empty()) return SSize(0, 0);

    TextRenderer* renderer = getTextRenderer();
    if (renderer == nullptr) return SSize(0, 0);

    SSize size = renderer->measureText(m_font.get(), text);
    return SSize(size.width / getScaleXX(), size.height / getScaleYY());
}

float Label::getStringWidth(const string& text) {
    return getTextSize(text).width;
}

void Label::loadFromResource(string resourceId){
    ResourceProvider* provider = getResourceProvider();
    if (provider == nullptr) {
        Platform::Log("Label::loadFromResource: No resource provider available");
        return;
    }

    m_fontData = provider->readFile(resourceId);
    if (m_fontData == nullptr || m_fontData->empty()) {
        Platform::Log("Label::loadFromResource: Error: '%s' not found\n", resourceId.c_str());
        return;
    }

    TextRenderer* renderer = getTextRenderer();
    if (renderer == nullptr) {
        Platform::Log("Label::loadFromResource: No text renderer available");
        return;
    }

    m_font = renderer->loadFontFromMemoryWithText(m_fontData->data(), m_fontData->size(),
                                           static_cast<int>(m_fontSize * getScaleXX()),
                                           m_caption);
    if (m_font == nullptr) {
        Platform::Log("Label::loadFromResource: Failed to load font");
    }
}

void Label::update(void){
    if(!getEnable()) return;

    ControlImpl::update();
}

void Label::draw(void){
    if(!getVisible()) return;

    ControlImpl::preDraw();

    if (m_lines.empty()) {
        return;
    }

    if (m_debugDraw) {
        SRect marginRectScaled = mapToDrawRect(getMarginedRect());
        GET_RENDERDEVICE->setDrawColor(SColor(0, 1, 0, 1));
        GET_RENDERDEVICE->drawRect(marginRectScaled);

        SRect hotRectScaled = mapToDrawRect(m_hotRect);
        GET_RENDERDEVICE->setDrawColor(SColor(1, 1, 0, 1));
        GET_RENDERDEVICE->drawRect(hotRectScaled);
    }

    SColor shadowColor;
    switch(getState()) {
        case ControlState::Disabled:
            shadowColor = m_textShadowColor.getDisabled();
            break;
        case ControlState::Hover:
            shadowColor = m_textShadowColor.getHover();
            break;
        case ControlState::Pressed:
            shadowColor = m_textShadowColor.getPressed();
            break;
        default:
            shadowColor = m_textShadowColor.getNormal();
            break;
    }

    SColor textColor;
    switch(getState()) {
        case ControlState::Disabled:
            textColor = m_textColor.getDisabled();
            break;
        case ControlState::Hover:
            textColor = m_textColor.getHover();
            break;
        case ControlState::Pressed:
            textColor = m_textColor.getPressed();
            break;
        default:
            textColor = m_textColor.getNormal();
            break;
    }

    TextRenderer* renderer = getTextRenderer();
    if (renderer == nullptr) return;

    for (size_t i = 0; i < m_lines.size(); ++i) {
        if (i >= m_lineOffsets.size()) break;
        if (i >= m_cachedTexts.size() || m_cachedTexts[i] == nullptr) continue;

        SPoint drawPoint = mapToDrawPoint(m_lineOffsets[i]);

        if (m_shadowEnabled) {
            SPoint shadowDrawPoint = mapToDrawPoint(m_lineOffsets[i] + m_shadowOffset);
            renderer->drawText(m_cachedTexts[i],
                               shadowDrawPoint.x, shadowDrawPoint.y, shadowColor);
        }

        renderer->drawText(m_cachedTexts[i],
                           drawPoint.x, drawPoint.y, textColor);
    }

    ControlImpl::draw();
}

bool Label::handleEvent(shared_ptr<Event> event){
    if(!getEnable() || !getVisible()) return false;

    if (ControlImpl::handleEvent(event)) return true;

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) { mx = event->mousePos.x; my = event->mousePos.y; gotPos = true; }
    else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }
    if (!gotPos && EventQueue::isPositionEvent(event->m_eventName) && event->m_eventParam.has_value()) {
        try {
            auto p = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (p) { mx = p->x; my = p->y; gotPos = true; }
        } catch (...) { }
    }
    if (gotPos) {
        SRect detectRect = mapToDrawRect(m_hotRect);
        if (detectRect.contains(mx, my)){
            switch(event->m_eventName){
                case EventName::FINGER_DOWN:
                case EventName::FINGER_MOTION:
                    if (m_onClick != nullptr){
                        m_onClick(dynamic_pointer_cast<Label>(this->getThis()));
                    }
                    setState(ControlState::Pressed);
                    return true;
            }
            if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
                setState(ControlState::Pressed);
                if(m_hoverCursor != nullptr){
                    Cursor::setCurrent(m_hoverCursor);
                }
                return true;
            }
            if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
                if (m_onClick != nullptr && m_state == ControlState::Pressed){
                    m_onClick(dynamic_pointer_cast<Label>(this->getThis()));
                }
                setState(ControlState::Hover);
                if(m_hoverCursor != nullptr){
                    Cursor::setCurrent(m_hoverCursor);
                }
                return true;
            }
            if (event->m_type == EventType::MouseMove) {
                setState(ControlState::Hover);
                if(m_hoverCursor != nullptr){
                    Cursor::setCurrent(m_hoverCursor);
                }
                return true;
            }
            return true;
        } else {
            setState(ControlState::Normal);
            if(m_defaultCursor){
                Cursor::setCurrent(m_defaultCursor);
            }
        }
    }
    return false;
}

void Label::setRect(SRect rect){
    if (m_rect == rect) return;
    ControlImpl::setRect(rect);

    recreate();
}
void Label::setParent(Control *parent) {
    if (m_parent == parent) {
        // 父控件未变，但仍需更新缩放（setScaleX/setScaleY 依赖此路径）
        ControlImpl::setParent(parent);
        return;
    }
    ControlImpl::setParent(parent);

    recreate();
}
SRect Label::getHotRect(void){
    return m_hotRect;
}

/*********************************************************for Builder mode**********************************************************/
void Label::setCaption(string caption){
    if (caption == m_caption) return;

    m_caption = caption;

    recreate();
}
string Label::getCaption(void) const{
    return m_caption;
}
void Label::setFont(FontName fontName){
    m_fontName = fontName;
    m_fontFile = fs::path(ConstDef::fontFiles.at(fontName));

    recreate();
}
void Label::setAlignmentMode(AlignmentMode Alignment){
    m_AlignmentMode = Alignment;

    recreate();
}
AlignmentMode Label::getAlignmentMode(void) const{
    return m_AlignmentMode;
}
void Label::setMargin(Margin margin){
    m_margin = margin;

    recreate();
}
void Label::setFontSize(int fontSize){
    if (fontSize == m_fontSize) return;
    m_fontSize = fontSize;

    recreate();
}

void Label::setShadow(bool enabled){
    m_shadowEnabled = enabled;
}

void Label::setShadowOffset(SPoint offset){
    m_shadowOffset = offset;
}

void Label::setOnClick(OnClickHandler handler){
    m_onClick = handler;
}

void Label::setOnPropertyChanged(OnPropertyChangedHandler handler){
    m_onPropertyChanged = handler;
}

void Label::SetFontStyle(int fontStyle){
    m_fontStyle = fontStyle;
}

void Label::setLineHeight(int height){
    if (height <= 0) return;
    m_defaultLineHeight = height;

    recreate();
}

int Label::getLineHeight() const{
    return m_defaultLineHeight > 0 ? m_defaultLineHeight : static_cast<int>(m_lineHeight);
}

void Label::setLineSpacingRatio(float spacingRatio) {
    m_defaultLineSpacingRatio = spacingRatio;

    recreate();
}

void Label::setDebugDraw(bool enabled) {
    m_debugDraw = enabled;
}

void Label::setEnableExpand(bool enable){
    m_enableExpand = enable;

    recreate();
}

bool Label::getEnableExpand() const{
    return m_enableExpand;
}

LabelBuilder::LabelBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_label(nullptr)
{
    m_label = make_shared<Label>(parent, rect, xScale, yScale);
}
LabelBuilder& LabelBuilder::setTextStateColor(StateColor stateColor){
    m_label->setTextStateColor(stateColor);
    return *this;
}
LabelBuilder& LabelBuilder::setTextShadowStateColor(StateColor stateColor){
    m_label->setTextShadowStateColor(stateColor);
    return *this;
}

LabelBuilder& LabelBuilder::setCaption(string caption){
    m_label->setCaption(caption);
    return *this;
}
LabelBuilder& LabelBuilder::setFont(FontName fontName){
    m_label->setFont(fontName);
    m_label->loadFromResource(m_label->m_fontFile.string());
    return *this;
}
LabelBuilder& LabelBuilder::setAlignmentMode(AlignmentMode Alignment){
    m_label->setAlignmentMode(Alignment);
    return *this;
}
LabelBuilder& LabelBuilder::setFontSize(int fontSize){
    m_label->setFontSize(fontSize);
    return *this;
}
LabelBuilder& LabelBuilder::setMargin(Margin margin){
    m_label->setMargin(margin);
    return *this;
}
LabelBuilder& LabelBuilder::setShadow(bool enabled){
    m_label->setShadow(enabled);
    return *this;
}

LabelBuilder& LabelBuilder::setShadowOffset(SPoint offset){
    m_label->setShadowOffset(offset);
    return *this;
}
LabelBuilder& LabelBuilder::setOnClick(Label::OnClickHandler handler){
    m_label->setOnClick(handler);
    return *this;
}
LabelBuilder& LabelBuilder::setOnPropertyChanged(Label::OnPropertyChangedHandler handler){
    m_label->setOnPropertyChanged(handler);
    return *this;
}
LabelBuilder& LabelBuilder::setId(int id){
    m_label->setId(id);
    return *this;
}

LabelBuilder& LabelBuilder::SetFontStyle(int fontStyle){
    m_label->SetFontStyle(fontStyle);
    return *this;
}

LabelBuilder& LabelBuilder::setLineHeight(int height){
    m_label->setLineHeight(height);
    return *this;
}

LabelBuilder& LabelBuilder::setEnableExpand(bool enable){
    m_label->setEnableExpand(enable);
    return *this;
}

LabelBuilder& LabelBuilder::setBorderStateColor(StateColor stateColor){
    m_label->setBorderStateColor(stateColor);
    m_label->setBorderVisible(true);
    return *this;
}

LabelBuilder& LabelBuilder::setDebugDraw(bool enabled){
    m_label->setDebugDraw(enabled);
    return *this;
}

shared_ptr<Label> LabelBuilder::build(void){
    m_label->create();
    return m_label;
}
