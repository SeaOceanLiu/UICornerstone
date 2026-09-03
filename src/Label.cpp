#include "Label.h"
#include "PropertyNames.h"
#include "Utility.h"
#include "PlatformUtils.h"
#include <cstring>

Label::Label(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale)
    , m_font(nullptr)
    , m_shadowOffset({2, 2})
    , m_AlignmentMode(AlignmentMode::AM_MID_LEFT)   // 文档/schema 声明默认 mid-left（垂直中线+水平左）
    , m_fontSize(16)
    , m_caption("")
    , m_shadowEnabled(false)
    , m_fontScaleDirty(false)
    , m_fontName(FontName::HarmonyOS_Sans_SC_Regular)
    , m_fontStyle(0)
    , m_hotRect({0, 0, 0, 0})
    , m_onClick(nullptr)
    , m_onPropertyChanged(nullptr)
    , m_defaultLineHeight(0)
    , m_lineHeight(0)
    , m_enableExpand(true)
    , m_clickable(true)
    , m_originalRect(rect)
    , m_debugDraw(false)
    , m_defaultLineSpacingRatio(0)
    , m_lineSpacing(0)
    , m_reentryCounter(0)
{
    m_ctlType = ControlType::Label;
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
    // 保留 m_fontData 以便 loadFromResource 复用，避免重复磁盘 I/O
}

void Label::releaseTexts(void) {
    // 实例已销毁（静态/全局残留控件退出期析构）时 renderer 已释放，直接放弃
    // 缓存的文本资源（进程退出回收），避免对悬垂 renderer 调用 destroyText
    if (!UIContext::isActive(m_context)) return;
    TextRenderer* renderer = getTextRenderer();
    if (renderer) {
        for (auto* t : m_cachedTexts) {
            renderer->destroyText(t);
        }
    }
    m_cachedTexts.clear();
}

void Label::recreate() {
    if(!m_isCreated) {
        // 两阶段创建：context 就绪后（挂树）由 setContext 触发，重试字体加载
        create();
        return;
    }

    releaseTexts();
    m_lines.clear();

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
        fireCCallback(PropertyNames::kEventPropertyChanged, CCallbackData::None, nullptr);
    }
}
void Label::create(void) {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建（字体依赖 context）

    if (!m_fontResourceId.empty()) loadFromResource(m_fontResourceId);
    else loadFromResource(m_fontFile.string());
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

    if (m_font) {
        for (const auto& line : m_lines) {
            void* textObj = renderer->createText(m_font.get(), line);
            m_cachedTexts.push_back(textObj);
        }
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
    line = ::truncateText(line, maxWidth,
        [this](const string& s) { return getStringWidth(s); });
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

void Label::loadFromFile(fs::path filePath){
    if (GET_CONTEXT == nullptr) return;  // 两阶段：挂树后 create() 经 m_fontFile 加载
    loadFromResource(filePath.string());
}

void Label::loadFromResource(string resourceId){
    if (m_font) return;
    if (GET_CONTEXT == nullptr) return;  // 两阶段：挂树后由 create() 加载

    // 若字体数据已加载（setFontSize 后 releaseFont 释放了 m_font 但保留了数据），直接复用
    if (!m_fontData) {
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
    }

    TextRenderer* renderer = getTextRenderer();
    if (renderer == nullptr) {
        Platform::Log("Label::loadFromResource: No text renderer available");
        return;
    }

    int scaledSize = static_cast<int>(m_fontSize * getScaleXX());
    m_font = renderer->loadFontFromMemoryWithText(m_fontData->data(), m_fontData->size(),
                                           scaledSize, m_caption);
    if (m_fontStyle != 0 && m_font) renderer->setFontStyle(m_font.get(), m_fontStyle);
    if (m_font == nullptr) {
        Platform::Log("Label::loadFromResource: Failed to load font");
    }
}

void Label::update(void){
    if(!getEnable()) return;

    if (m_fontScaleDirty && m_visible) {
        m_fontScaleDirty = false;
        releaseFont();
        recreate();
    }

    ControlImpl::update();
}

void Label::draw(void){
    if(!getVisible()) return;

    ControlImpl::beforeDraw();

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
    afterDraw();
}

bool Label::handleEvent(shared_ptr<Event> event){
    if(!getEnable() || !getVisible()) return false;

    if (ControlImpl::handleEvent(event)) return true;

    if (!m_clickable) return false;

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) { mx = event->mousePos.x; my = event->mousePos.y; gotPos = true; }
    else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }
    else if (event->m_type == EventType::FingerDown || event->m_type == EventType::FingerUp || event->m_type == EventType::FingerMotion) {
        mx = event->mousePos.x; my = event->mousePos.y; gotPos = true;
    }
    if (gotPos) {
        SRect detectRect = mapToDrawRect(m_hotRect);
        if (detectRect.contains(mx, my)){
            if (event->m_type == EventType::FingerDown || event->m_type == EventType::FingerMotion) {
                if (m_onClick != nullptr){
                    m_onClick(dynamic_pointer_cast<Label>(this->getThis()));
                }
                fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
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
                fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
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
    float oldScaleX = getScaleXX();
    float oldScaleY = getScaleYY();
    ControlImpl::setParent(parent);
    if (oldScaleX != getScaleXX() || oldScaleY != getScaleYY()) {
        releaseFont();
    }

    recreate();
}
SRect Label::getHotRect(void){
    return m_hotRect;
}

// 父链缩放变更时重建文本：字号随复合缩放（loadFromResource 用 getScaleXX）。
// 无变化时不重建（拖动不动时零开销）；setParent 已有同逻辑，此处补运行期缩放。
// 不可见控件延后到可见帧（update）再重建，避免布局变更时重建不可见文本。
void Label::refreshScaleWith(float parentXX, float parentYY){
    float oldScaleX = getScaleXX();
    float oldScaleY = getScaleYY();
    ControlImpl::refreshScaleWith(parentXX, parentYY);
    if (oldScaleX != getScaleXX() || oldScaleY != getScaleYY()) {
        if (m_visible) {
            releaseFont();
            recreate();
        } else {
            m_fontScaleDirty = true;
        }
    }
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

    releaseFont();
    m_fontData.reset();
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

    releaseFont();
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

void Label::setClickable(bool clickable){
    m_clickable = clickable;
}

// ── Property system overrides ──
int Label::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0)    { setShadow(value != 0);      return 1; }
    if (strcmp(prop, PropertyNames::kExpand) == 0)    { setEnableExpand(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kClickable) == 0)  { setClickable(value != 0);   return 1; }
    if (strcmp(prop, PropertyNames::kDebugDraw) == 0)  { setDebugDraw(value != 0);   return 1; }
    // 字体样式分位开关（可组合）：font-bold/italic/underline/strikethrough → 位 1/2/4/8
    if (strcmp(prop, PropertyNames::kFontBold) == 0)          { SetFontStyle(value ? (m_fontStyle | 0x01) : (m_fontStyle & ~0x01)); return 1; }
    if (strcmp(prop, PropertyNames::kFontItalic) == 0)        { SetFontStyle(value ? (m_fontStyle | 0x02) : (m_fontStyle & ~0x02)); return 1; }
    if (strcmp(prop, PropertyNames::kFontUnderline) == 0)     { SetFontStyle(value ? (m_fontStyle | 0x04) : (m_fontStyle & ~0x04)); return 1; }
    if (strcmp(prop, PropertyNames::kFontStrikethrough) == 0) { SetFontStyle(value ? (m_fontStyle | 0x08) : (m_fontStyle & ~0x08)); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int Label::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0)   { setFontSize(value);   return 1; }
    if (strcmp(prop, PropertyNames::kLineHeight) == 0) { setLineHeight(value); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}
int Label::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kLineSpacingRatio) == 0) { setLineSpacingRatio(value); return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { SPoint o = m_shadowOffset; o.x = value; setShadowOffset(o); return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { SPoint o = m_shadowOffset; o.y = value; setShadowOffset(o); return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}
int Label::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) { setCaption(value); return 1; }
    if (strcmp(prop, PropertyNames::kFontResource) == 0) {
        if (value && value[0]) {           // 强制换用内存字体：释放旧字体与旧数据
            m_fontResourceId = value;      // 两阶段记忆：parse 阶段 provider 未就绪时由 create() 补读
            m_fontFile = fs::path();       // 与 font-file 互斥：清除路径形态
            releaseFont();
            m_fontData = nullptr;
            loadFromResource(value);
        }
        return 1;
    }
    if (strcmp(prop, PropertyNames::kFontFile) == 0) {
        if (value && value[0]) {           // 任意字体文件路径：覆盖枚举默认 m_fontFile（两阶段：create() 加载）
            m_fontFile = fs::path(value);
            m_fontResourceId.clear();
            releaseFont();
            m_fontData = nullptr;
            loadFromFile(fs::path(value));
        }
        return 1;
    }
    return ControlImpl::setStringProperty(prop, value);
}
int Label::setEnumProperty(const char* prop, const char* value) {
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
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        setFont(FontNameFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kFontStyle) == 0) {
        if (_stricmp(value, PropertyNames::kFontStyleNormal) == 0)        { SetFontStyle(0); return 1; }
        if (_stricmp(value, PropertyNames::kFontStyleBold) == 0)          { SetFontStyle(1); return 1; }
        if (_stricmp(value, PropertyNames::kFontStyleItalic) == 0)        { SetFontStyle(2); return 1; }
        if (_stricmp(value, PropertyNames::kFontStyleUnderline) == 0)     { SetFontStyle(4); return 1; }
        if (_stricmp(value, PropertyNames::kFontStyleStrikethrough) == 0) { SetFontStyle(8); return 1; }
        return 0;
    }
    return ControlImpl::setEnumProperty(prop, value);
}
int Label::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0)    { out = m_shadowEnabled ? 1 : 0;      return 1; }
    if (strcmp(prop, PropertyNames::kExpand) == 0)    { out = m_enableExpand ? 1 : 0;       return 1; }
    if (strcmp(prop, PropertyNames::kClickable) == 0)  { out = m_clickable ? 1 : 0;          return 1; }
    if (strcmp(prop, PropertyNames::kDebugDraw) == 0) { out = m_debugDraw ? 1 : 0;          return 1; }
    if (strcmp(prop, PropertyNames::kFontBold) == 0)          { out = (m_fontStyle & 0x01) ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kFontItalic) == 0)        { out = (m_fontStyle & 0x02) ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kFontUnderline) == 0)     { out = (m_fontStyle & 0x04) ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kFontStrikethrough) == 0) { out = (m_fontStyle & 0x08) ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}
int Label::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0)   { out = m_fontSize;   return 1; }
    if (strcmp(prop, PropertyNames::kLineHeight) == 0) { out = getLineHeight(); return 1; }
    return ControlImpl::getIntProperty(prop, out);
}
int Label::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kLineSpacingRatio) == 0) { out = m_defaultLineSpacingRatio; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0)    { out = m_shadowOffset.x;  return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0)    { out = m_shadowOffset.y; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}
int Label::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) { out = m_caption.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}
int Label::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kAlign) == 0) {
        switch (m_AlignmentMode) {
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
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        out = FontNameToString(m_fontName);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kFontStyle) == 0) {
        switch (m_fontStyle) {
        case 0:  out = PropertyNames::kFontStyleNormal; return 1;
        case 1:  out = PropertyNames::kFontStyleBold; return 1;
        case 2:  out = PropertyNames::kFontStyleItalic; return 1;
        case 4:  out = PropertyNames::kFontStyleUnderline; return 1;
        case 8:  out = PropertyNames::kFontStyleStrikethrough; return 1;
        default: out = PropertyNames::kFontStyleNormal; return 1;
        }
    }
    return ControlImpl::getEnumProperty(prop, out);
}
int Label::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventClick) == 0 ||
        strcmp(event, PropertyNames::kEventPropertyChanged) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
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

LabelBuilder& LabelBuilder::setClickable(bool clickable){
    m_label->setClickable(clickable);
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
