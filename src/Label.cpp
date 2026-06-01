#include "Label.h"

Label::Label(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale)
    , m_font(nullptr)
    , m_textEngin(nullptr)
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
    m_margin = ConstDef::LABEL_CAPTION_MARGIN; // 默认边距
    setVisible(false);    // 默认不可见，待创建成功后自动置为可见
    setTransparent(true);
    setBorderVisible(false);

    m_hoverCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    if (m_hoverCursor == nullptr) {
        SDL_Log("Label::Label: Failed to create cursor: %s", SDL_GetError());
    }
    m_defaultCursor = SDL_GetCursor();
    if(m_defaultCursor == nullptr) {
        SDL_Log("Label::Label: Failed to get default cursor: %s", SDL_GetError());
    }

    setFont(m_fontName); // 初始化字体和文本引擎
}

Label::~Label(void){
    releaseTextObjects();
    releaseTextEngin(); // 释放文本引擎资源，注意必须保证在TTF_Quit()之前执行关闭文本引擎的操作，否则会报错
    releaseFont(); // 释放字体资源，注意必须保证在TTF_Quit()之前执行关闭字体的操作，否则会报错

    if (m_hoverCursor != nullptr) {
        SDL_DestroyCursor(m_hoverCursor);
        m_hoverCursor = nullptr;
    }
}

void Label::releaseFont(void) {
    if (m_font != nullptr) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
}
void Label::releaseTextEngin(void) {
    if (m_textEngin != nullptr) {
        TTF_DestroyRendererTextEngine(m_textEngin);
        m_textEngin = nullptr;
    }
}
void Label::releaseTextObjects(void) {
    for (auto text : m_lineTexts) {
        if (text != nullptr) {
            TTF_DestroyText(text);
        }
    }
    m_lineTexts.clear();
}
void Label::releaseMultilineTexts(void) {
    m_lines.clear();
}

void Label::recreate() {
    // 没有创建过，直接退出，待调用create方法时会创建相关资源
    if(!m_isCreated) return;

    // 释放多行文本对像实例
    releaseTextObjects();

    // 释放多行文本
    releaseMultilineTexts();

    // 释放TextEngin
    releaseTextEngin();

    // 释放字体资源
    releaseFont();

    if(typeid(*this) == typeid(Label)) {
        m_isCreated = false;  // 重置创建标志，调用create方法时会重新创建相关资源

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
    // Implementation for creating the label
    if (m_isCreated) return;

    // 先load字体资源
    loadFromResource(m_fontFile.string());
    // 创建TextEngin
    createTextEngine();
    // 多行文本分割
    createMultilineText();
    // 创建多行文本实例
    createLineTexts();

    // //设置已创建标志并显示控件
    ControlImpl::create();
}

void Label::createMultilineText(void) {
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

SSize Label::getTextSize(TTF_Text* text) {
    if (text == nullptr) return SSize(0, 0);

    int w, h;
    if (!TTF_GetTextSize(text, &w, &h)) {
        SDL_Log("Label::getTextSize: Failed to get text size: %s", SDL_GetError());
        return SSize(0, 0);
    }
    // 注意这里需要除以缩放比例，因为TTF_GetTextSize返回的是实际像素尺寸，而我们在绘制时会根据缩放进行调整
    return SSize(static_cast<float>(w) / getScaleXX(), static_cast<float>(h) / getScaleYY());
}

void Label::createLineTexts() {
    if (m_textEngin == nullptr || m_font == nullptr) {
        SDL_Log("Label::createLineTexts: m_textEngin or m_font is nullptr");
        return;
    }
    for (auto text : m_lineTexts) {
        if (text != nullptr) {
            TTF_DestroyText(text);
        }
    }
    m_lineTexts.clear();
    m_lineOffsets.clear();

    int lineSpacing = m_lineSpacing;

    SRect marginRect = getMarginedRect();
    float availableWidth = marginRect.width;
    float availableHeight = marginRect.height;

    // 首先创建文本对象以获取每行的尺寸信息，获得最大行宽，并按TOP_LEFT对齐计算初始偏移
    float lineOffsetX = static_cast<float>(marginRect.left);
    float lineOffsetY = static_cast<float>(marginRect.top);
    float maxLineWidth = 0;
    bool isHeightTruncated = false;
    for (size_t i = 0; i < m_lines.size(); ++i) {
        string processedLine = m_lines[i];
        // 按行进行截断处理，如果不允许扩展且行宽超过可用宽度，则进行截断
        if (!m_enableExpand) {
            bool isWidthTruncated = false;
            if (availableWidth > 0 && getStringWidth(processedLine) > availableWidth) {
                truncateLine(processedLine, availableWidth);
                isWidthTruncated = true;
            }
            // 对超过总高度的后续行进行截断处理，判断如果下一行会超过总高，就交本行直接显示为"..."
            if (m_lines.size() > 1 && availableHeight > 0 && (lineOffsetY + m_lineHeight + lineSpacing + m_lineHeight - marginRect.top) > availableHeight) {
                if (!isWidthTruncated){
                    processedLine = "...";
                }
                isHeightTruncated = true;
            }
        }

        TTF_Text* textObj = TTF_CreateText(m_textEngin, m_font, processedLine.c_str(), processedLine.length());
        if (textObj == nullptr) {
            SDL_Log("Label::createLineTexts: Failed to create text object for line %zu: %s", i, SDL_GetError());
            return;
        }
        m_lineTexts.push_back(textObj);
        m_lineOffsets.push_back({lineOffsetX, lineOffsetY});

        SSize textSize = getTextSize(textObj);
        if (textSize.width > maxLineWidth) maxLineWidth = textSize.width;

        if (isHeightTruncated) {
            break;
        }
        // 计算下一行的偏移，默认按照TOP_LEFT对齐, 所以只需要在Y方向增加行高和行间距
        lineOffsetY = lineOffsetY + m_lineHeight + lineSpacing;
    }

    if (m_lineTexts.empty()) return;

    // 计算出总行高
    float totalHeight = (m_lineHeight + lineSpacing) * m_lineTexts.size() - lineSpacing;

    float hotRectLeft = marginRect.right();

    for (size_t i = 0; i < m_lineOffsets.size(); ++i) {
        SSize textSize = getTextSize(m_lineTexts[i]);
        float lineWidth = textSize.width;

        // 先根据对齐方式计算水平偏移
        switch (m_AlignmentMode) {
            case AlignmentMode::AM_TOP_RIGHT:
            case AlignmentMode::AM_MID_RIGHT:
            case AlignmentMode::AM_BOTTOM_RIGHT:
                // 右对齐时，偏移 = 左边距 + 可用宽度 - 行宽
                m_lineOffsets[i].x = marginRect.left + (availableWidth - lineWidth);
                break;
            case AlignmentMode::AM_TOP_CENTER:
            case AlignmentMode::AM_CENTER:
            case AlignmentMode::AM_BOTTOM_CENTER:
                // 居中时，偏移 = 左边距 + (可用宽度 - 行宽) / 2
                m_lineOffsets[i].x = marginRect.left + (availableWidth - lineWidth) / 2;
                break;
            default:
                break;
        }

        // 再根据对齐方式计算垂直偏移
        switch (m_AlignmentMode) {
            case AlignmentMode::AM_CENTER:
            case AlignmentMode::AM_MID_LEFT:
            case AlignmentMode::AM_MID_RIGHT:
                // 垂直居中时，偏移 = 上边距 + (可用高度 - 总行高) / 2 + (行高 + 行间距) * 行索引
                m_lineOffsets[i].y = marginRect.top + (availableHeight - totalHeight) / 2 + (m_lineHeight + lineSpacing) * i;
                break;
            case AlignmentMode::AM_BOTTOM_LEFT:
            case AlignmentMode::AM_BOTTOM_CENTER:
            case AlignmentMode::AM_BOTTOM_RIGHT:
                // 底部对齐时，偏移 = 上边距 + 可用高度 - 总行高 + (行高 + 行间距) * 行索引
                m_lineOffsets[i].y = marginRect.top + (availableHeight - totalHeight) + (m_lineHeight + lineSpacing) * i;
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

float Label::getStringWidth(const string& text) {
    if (m_font == nullptr || text.empty()) return 0;

    TTF_Text* tempText = TTF_CreateText(m_textEngin, m_font, text.c_str(), text.length());
    if (tempText == nullptr) return 0;

    SSize stringSize = getTextSize(tempText);
    TTF_DestroyText(tempText);

    return stringSize.width;
}
void Label::createTextEngine(void){
    m_textEngin = TTF_CreateRendererTextEngine(getRenderer());
    if (m_textEngin == nullptr) {
        SDL_Log("Label::createTextEngine: Failed to create text engine: %s", SDL_GetError());
        return;
    }

    if (m_defaultLineHeight > 0) {
        m_lineHeight = m_defaultLineHeight;
    } else {
        int fontHeight = TTF_GetFontHeight(m_font);
        m_lineHeight = fontHeight / getScaleYY();
    }
    if (m_defaultLineSpacingRatio > 0) {
        m_lineSpacing = static_cast<int>(m_lineHeight * m_defaultLineSpacingRatio);
    } else {
        m_lineSpacing = static_cast<int>(m_lineHeight * 0.2f);
    }
}

void Label::loadFromResource(string resourceId){
    shared_ptr<Resource> resource = ResourceLoader::getInstance()->getResource(resourceId);
    if (resource == nullptr || resource->resourceType != ResourceLoader::RT_FONTS
        || resource->pMem == nullptr) {

        SDL_Log("Label::loadFromResource: Error: '%s' is not a font\n", resourceId.c_str());
        return;
    }
    SDL_IOStream *resourceStream = SDL_IOFromConstMem(resource->pMem.get(), resource->resourceSize);
    if (resourceStream == nullptr) {
        SDL_Log("Label::loadFromResource: Failed to create IO stream for: %s", resourceId.c_str());
        return;
    }
    m_font = TTF_OpenFontIO(resourceStream, true, m_fontSize * getScaleXX());
    if (m_font == nullptr) {
        SDL_Log("Label::loadFromResource: Failed to load font: %s", SDL_GetError());
        return;
    }
}

void Label::update(void){
    if(!getEnable()) return;

    ControlImpl::update();
}

void Label::draw(void){
    if(!getVisible()) return;

    ControlImpl::preDraw();

    if (m_lineTexts.empty()) {
        return;
    }

    // SRect rect = getRect();
    // SRect drawRect = getDrawRect();

    // drawBackground(&drawRect);
    // drawBorder(&drawRect);

    if (m_debugDraw) {
        SDL_Renderer* renderer = getRenderer();

        SRect marginRectScaled = mapToDrawRect(getMarginedRect());

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderRect(renderer, marginRectScaled.toSDLFRect());

        SRect hotRectScaled = mapToDrawRect(m_hotRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderRect(renderer, hotRectScaled.toSDLFRect());
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

    for (size_t i = 0; i < m_lineTexts.size(); ++i) {
        if (m_lineTexts[i] == nullptr) continue;

        SPoint drawPoint = mapToDrawPoint(m_lineOffsets[i]);

        if (m_shadowEnabled) {
            SPoint shadowDrawOffset = mapToDrawPoint(m_lineOffsets[i] + m_shadowOffset);

            if(!TTF_SetTextColor(m_lineTexts[i], shadowColor.redByte(), shadowColor.greenByte(), shadowColor.blueByte(), shadowColor.alphaByte())) {
                SDL_Log("Label::draw: Failed to set shadow text color: %s", SDL_GetError());
            }
            if (!TTF_DrawRendererText(m_lineTexts[i],
                                        shadowDrawOffset.x,
                                        shadowDrawOffset.y)) {
                SDL_Log("Label::draw: Failed to render shadow text: %s", SDL_GetError());
            }
        }

        if(!TTF_SetTextColor(m_lineTexts[i], textColor.redByte(), textColor.greenByte(), textColor.blueByte(), textColor.alphaByte())) {
            SDL_Log("Label::draw: Failed to set text color: %s", SDL_GetError());
        }

        if (!TTF_DrawRendererText(m_lineTexts[i], drawPoint.x, drawPoint.y)) {
            SDL_Log("Label::draw: Failed to render text: %s", SDL_GetError());
        }
    }

    ControlImpl::draw();
}

bool Label::handleEvent(shared_ptr<Event> event){
    if(!getEnable() || !getVisible()) return false;

    if (ControlImpl::handleEvent(event)) return true;

    if (EventQueue::isPositionEvent(event->m_eventName)){
        if (!event->m_eventParam.has_value()) return false;
        try {
            shared_ptr<SPoint> pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            SRect detectRect = mapToDrawRect(m_hotRect);
            if (detectRect.contains(pos->x, pos->y)){
                switch(event->m_eventName){
                    case EventName::FINGER_DOWN:
                    case EventName::FINGER_MOTION:
                        if (m_onClick != nullptr){
                            m_onClick(dynamic_pointer_cast<Label>(this->getThis()));
                        }
                        setState(ControlState::Pressed);
                        return true;
                    case EventName::MOUSE_LBUTTON_DOWN:
                        setState(ControlState::Pressed);
                        if(m_hoverCursor != nullptr){
                            SDL_SetCursor(m_hoverCursor);
                        }
                        return true;
                    case EventName::MOUSE_LBUTTON_UP:
                        if (m_onClick != nullptr && m_state == ControlState::Pressed){
                            m_onClick(dynamic_pointer_cast<Label>(this->getThis()));
                        }
                        setState(ControlState::Hover);
                        if(m_hoverCursor != nullptr){
                            SDL_SetCursor(m_hoverCursor);
                        }
                        return true;
                    case EventName::MOUSE_MOVING:
                        setState(ControlState::Hover);
                        if(m_hoverCursor != nullptr){
                            SDL_SetCursor(m_hoverCursor);
                        }
                        return true;
                    case EventName::MOUSE_WHEEL:
                        return false;
                    default:
                        break;
                }
                return true;
            } else {
                setState(ControlState::Normal);
                if(m_defaultCursor){
                    SDL_SetCursor(m_defaultCursor);
                }
            }
        } catch (...) {
            return false;
        }
    }
    return false;
}

void Label::setRect(SRect rect){
    ControlImpl::setRect(rect);

    recreate();
}
void Label::setParent(Control *parent) {
    ControlImpl::setParent(parent);

    // 父控件改变可能导致缩放比例发生变化，因此需要重新创建以适应新的缩放
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
    m_fontFile = fs::path(ResourceLoader::m_fontFiles[fontName]);

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

void Label::SetFontStyle(TTF_FontStyleFlags fontStyle){
    m_fontStyle = fontStyle;

    if (m_font == nullptr) return;

    TTF_SetFontStyle(m_font, fontStyle);

    create();
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

LabelBuilder& LabelBuilder::SetFontStyle(TTF_FontStyleFlags fontStyle){
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