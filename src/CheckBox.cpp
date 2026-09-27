// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#include "CheckBox.h"
#include "GraphTool.h"
#include "PlatformUtils.h"
#include "PropertyNames.h"

CheckBox::CheckBox(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale),
    m_checkState(CheckState::Unchecked),
    m_style(CheckBoxStyle::Classic),
    m_layout(CheckBoxLayout::TextRight),
    // m_labelAlignmentMode(),
    // m_labelFontName(),
    // m_labelShadowEnabled(false),
    // m_labelShadowOffset({2, 2}),
    m_verticalAlign(CheckBoxVerticalAlign::Center),
    m_caption(nullptr),
    m_onCheckChanged(nullptr),
    m_sizeRatio(ConstDef::CHECKBOX_SIZE_RATIO),
    m_captionSize(0),  // 0 = 使用 ConstDef::CHECKBOX_DEFAULT_CAPTION_SIZE
    m_boxRect({0, 0, 0, 0}),
    m_boxMargin(ConstDef::CHECKBOX_BOX_MARGIN),
    m_triStateEnabled(true),
    m_checkStateColor(StateColor::Type::Text),
    m_crossStateColor(StateColor::Type::Text),
    m_indeterminateStateColor(StateColor::Type::Text)
{
    m_ctlType = ControlType::CheckBox;
    // P0-30：历史缺省态显式化（保持既有 hover/pressed 视觉）
    setHoverStateBGColor(ConstDef::DEFAULT_HOVER_COLOR);
    setPressedStateBGColor(ConstDef::DEFAULT_DOWN_COLOR);
    setHoverStateBDColor(ConstDef::DEFAULT_BORDER_HOVER_COLOR);
    setPressedStateBDColor(ConstDef::DEFAULT_BORDER_DOWN_COLOR);
    m_textColor.setHover(ConstDef::DEFAULT_TEXT_HOVER_COLOR);
    m_textColor.setPressed(ConstDef::DEFAULT_TEXT_DOWN_COLOR);
    m_textShadowColor.setHover(ConstDef::DEFAULT_TEXT_SHADOW_HOVER_COLOR);
    m_textShadowColor.setPressed(ConstDef::DEFAULT_TEXT_SHADOW_DOWN_COLOR);
    m_checkStateColor.setNormal(ConstDef::CHECKBOX_CHECK_COLOR);
    m_crossStateColor.setNormal(ConstDef::CHECKBOX_CROSS_COLOR);
    m_indeterminateStateColor.setNormal(ConstDef::CHECKBOX_INDETERMINATE_COLOR);
    m_boxBorderStateColor.setNormal(ConstDef::DEFAULT_BORDER_NORMAL_COLOR);

    m_rect = rect;
    setTransparent(true);
    setBorderVisible(false);
    setFocusable(true);

    createCaption();
}

void CheckBox::releaseCaption(void){
    if (m_caption != nullptr) {
        // #15：removeControl 须在 reset 前（旧实现先 reset 后传 nullptr → 无效调用 → 旧 Label 残留渲染树）
        auto old = m_caption;
        m_caption = nullptr;
        removeControl(old);
    }
}

void CheckBox::captureCaptionState(void) {
    // #15/§3：重建前快照（直控/属性/工厂/parser 任意路径设置的 caption 状态均持久）
    if (!m_caption) return;
    m_captionText = m_caption->getCaption();
    if (m_caption->getFontSize() > 0) m_captionSize = (float)m_caption->getFontSize();
    m_capTextColor = m_caption->getTextStateColor();
    m_capTextShadowColor = m_caption->getTextShadowStateColor();
    m_capColorsValid = true;
    m_enableTextShadow = m_caption->isShadowEnabled();
    m_shadowOffset = m_caption->getShadowOffset();
}
void CheckBox::createCaption(void){
    if (m_caption != nullptr) {
        return;
    }

    // 先以CheckBox的rect来创建Label，获得Label的文字尺寸，再刷新Label的rect
    m_caption = LabelBuilder(this, {0, 0, getRect().width, getRect().height})
        .setFont(FontName::HarmonyOS_Sans_SC_Regular)
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setFontSize((int)effectiveCaptionSize())
        .setCaption(m_captionText)                                   // #15：宿主文本（recreate 不丢）
        .setTextStateColor(m_capColorsValid ? m_capTextColor : m_textColor)
        .setTextShadowStateColor(m_capColorsValid ? m_capTextShadowColor : m_textShadowColor)
        // .setMargin({0, 0, 0, 0})
        .setShadow(m_enableTextShadow)          // #12：宿主字段（重建/JSON 稳定）
        .setShadowOffset(m_shadowOffset)
        .setEnableExpand(false)
        // .setDebugDraw(true)
        .setOnPropertyChanged([this](shared_ptr<Label> label){  // Label的属性改变时，通过回调来触发CheckBox调整布局
            setBoxSize();
            adjustSpaceAssignment();
            adjustBoxVerticalAlign();
        })
        .build();
    if (m_caption) m_caption->setState(getState());   // (重)建时同步当前状态
}

shared_ptr<Label> CheckBox::getCaption(void) const {
    return m_caption;
}

// setBoxSize必须在createCaption之后调用，因为setBoxSize需要根据caption的行高来设置checkbox的大小
void CheckBox::setBoxSize(void) {
    if (m_caption == nullptr) {
        Platform::Log("CheckBox::setBoxSize: Caption is null");
        return;
    }
    m_boxRect.left = 0;
    m_boxRect.top = 0;
    m_boxRect.width = m_caption->getLineHeight() * m_sizeRatio;
    m_boxRect.height = m_boxRect.width;
}

// adjustSpaceAssignment必须在setBoxSize之后调用，因为adjustSpaceAssignment需要根据checkbox的大小来调整checkbox和caption的位置
void CheckBox::adjustSpaceAssignment(void) {
    SRect marginRect = getMarginedRect();

    switch(m_layout) {
        case CheckBoxLayout::TextRight:
            m_boxRect.left = marginRect.left;
            m_boxRect.top = marginRect.top;
            m_caption->setRect({m_boxRect.right(), marginRect.top,
                    marginRect.width - m_boxRect.width, marginRect.height});
            m_caption->setAlignmentMode(AlignmentMode::AM_MID_LEFT);
            break;
        case CheckBoxLayout::TextLeft:
            m_boxRect.left = marginRect.right() - m_boxRect.width;
            m_boxRect.top = marginRect.top;
            m_caption->setRect({marginRect.left, marginRect.top,
                    marginRect.width - m_boxRect.width, marginRect.height});
            m_caption->setAlignmentMode(AlignmentMode::AM_MID_RIGHT);
            break;
    }
}

void CheckBox::adjustBoxVerticalAlign(void) {
    if (m_caption == nullptr) {
        Platform::Log("CheckBox::adjustBoxVerticalAlign: Caption is null");
        return;
    }
    SRect marginRect = getMarginedRect();
    switch (m_verticalAlign)
    {
        case CheckBoxVerticalAlign::Top:
            m_boxRect.top = marginRect.top;
            break;
        case CheckBoxVerticalAlign::Center:
            m_boxRect.top = marginRect.top + (marginRect.height - m_boxRect.height) / 2;
            break;
        case CheckBoxVerticalAlign::Bottom:
            m_boxRect.top = marginRect.bottom() - m_boxRect.height;
            break;
        default:
            Platform::Log("CheckBox::adjustBoxVerticalAlign: Invalid vertical align value");
            break;
    }
}

void CheckBox::recreate(void) {
    // 没有创建过，直接退出，待调用create方法时会重新创建相关资源
    if(!m_isCreated) {
        // 两阶段创建：context 就绪后（挂树）由 setContext 触发
        create();
        return;
    }

    // #15：释放前快照 caption 状态（文本/字号/各态色/shadow）→ 重建应用
    captureCaptionState();

    // 释放子控件
    releaseCaption();


    if (typeid(*this) == typeid(CheckBox)) {
        m_isCreated = false;  // 重置创建标志，调用create方法时会重新创建相关资源
        create();
    }
}
void CheckBox::create(void) {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建

    createCaption();
    setBoxSize();
    adjustSpaceAssignment();
    adjustBoxVerticalAlign();

    // 可以直接添加，因为addControl内部会检查是否已经添加过了，如果已经添加过了，就不会重复添加了
    addControl(m_caption);
    ControlImpl::create();
}

void CheckBox::update(void) {
    if (!getEnable()) return;
    ControlImpl::update();
}

void CheckBox::draw(void) {
    if (!getVisible()) return;

    ControlImpl::beforeDraw();

    SRect drawRect = getDrawRect();

    drawCheckBoxFrame();

    switch (m_checkState) {
        case CheckState::Checked:
            if (m_style == CheckBoxStyle::Cross) {
                drawCrossMark();
            } else {
                drawCheckMark();
            }
            break;
        case CheckState::Indeterminate:
            drawIndeterminateMark();
            break;
        case CheckState::Unchecked:
        default:
            break;
    }

    ControlImpl::draw();
    afterDraw();
}

bool CheckBox::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) { mx = event->mousePos.x; my = event->mousePos.y; gotPos = true; }
    else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }
    if (gotPos) {
        if (getDrawRect().contains(mx, my)) {
            if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
                CheckState oldState = m_checkState;
                if (m_triStateEnabled) {
                    switch (m_checkState) {
                        case CheckState::Unchecked:
                            setCheckState(CheckState::Checked);
                            break;
                        case CheckState::Checked:
                            setCheckState(CheckState::Indeterminate);
                            break;
                        case CheckState::Indeterminate:
                            setCheckState(CheckState::Unchecked);
                            break;
                    }
                } else {
                    switch (m_checkState) {
                        case CheckState::Unchecked:
                            setCheckState(CheckState::Checked);
                            break;
                        case CheckState::Checked:
                            setCheckState(CheckState::Unchecked);
                            break;
                        case CheckState::Indeterminate:
                            setCheckState(CheckState::Unchecked);
                            break;
                    }
                }
                if (m_onCheckChanged) {
                    m_onCheckChanged(dynamic_pointer_cast<CheckBox>(getThis()), oldState, m_checkState);
                }
                int state = static_cast<int>(m_checkState);
                fireCCallback(PropertyNames::kEventCheckChanged, CCallbackData::Int, &state);
                return true;
            }
            if (event->m_type == EventType::MouseMove) {
                if (getState() != ControlState::Hover) {
                    setState(ControlState::Hover);
                }
                return true;
            }
        } else {
            if (getState() == ControlState::Hover) {
                setState(ControlState::Normal);
            }
        }
    }

    // Keyboard activation: Space → toggle state
    if (event->m_type == EventType::KeyDown && getFocused()) {
        if (event->keyEvent.keycode == KeyCode::Space) {
            CheckState oldState = m_checkState;
            if (m_triStateEnabled) {
                switch (m_checkState) {
                    case CheckState::Unchecked:
                        setCheckState(CheckState::Checked);
                        break;
                    case CheckState::Checked:
                        setCheckState(CheckState::Indeterminate);
                        break;
                    case CheckState::Indeterminate:
                        setCheckState(CheckState::Unchecked);
                        break;
                }
            } else {
                setCheckState(m_checkState == CheckState::Checked ? CheckState::Unchecked : CheckState::Checked);
            }
            if (m_onCheckChanged) {
                m_onCheckChanged(dynamic_pointer_cast<CheckBox>(getThis()), oldState, m_checkState);
            }
            int state = static_cast<int>(m_checkState);
            fireCCallback(PropertyNames::kEventCheckChanged, CCallbackData::Int, &state);
            return true;
        }
    }

    return ControlImpl::handleEvent(event);
}

void CheckBox::setRect(SRect rect) {
    if (m_rect == rect) return;
    ControlImpl::setRect(rect);

    recreate();
}

void CheckBox::onMouseEnter(float x, float y) {
    setState(ControlState::Hover);
}

void CheckBox::onMouseLeave(float x, float y) {
    setState(ControlState::Normal);
}

void CheckBox::setState(ControlState state) {
    ControlImpl::setState(state);
    // 状态联动：caption 的 hover/pressed/disabled 各态色经此可达（Label::draw 按自身 state 取色）
    if (m_caption) m_caption->setState(state);
}

int CheckBox::getPtrProperty(const char* prop, void*& out) {
    // 暴露内部 caption Label 句柄（键与 Button 统一：caption-label）：
    // 应用经句柄直控 Label 标准属性（颜色/字体/对齐），各态色随状态联动（setState 已同步）
    if (strcmp(prop, PropertyNames::kCaptionLabel) == 0) {
        // 句柄约定：Control* 基地址（ControlImpl 虚继承 Control，须经基类转换修正偏移）
        out = m_caption ? static_cast<Control*>(m_caption.get()) : nullptr;
        return m_caption ? 1 : 0;
    }
    return ControlImpl::getPtrProperty(prop, out);
}

int CheckBox::setStringProperty(const char* prop, const char* value) {
    // #9：caption 文本运行时设置（与 Button 对齐）；Label onPropertyChanged → setBoxSize/adjustSpaceAssignment 自动刷新布局
    if (strcmp(prop, PropertyNames::kCaption) == 0) {
        m_captionText = value ? value : "";                          // #15：宿主持久化
        if (m_caption) { m_caption->setCaption(m_captionText); return 1; }
        return 0;
    }
    return ControlImpl::setStringProperty(prop, value);
}

int CheckBox::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) {
        if (m_caption) { m_captionText = m_caption->getCaption(); out = m_captionText.c_str(); return 1; }
        return 0;
    }
    return ControlImpl::getStringProperty(prop, out);
}

void CheckBox::setCheckState(CheckState state) {
    if (state == CheckState::Indeterminate && !m_triStateEnabled) {
        m_checkState = CheckState::Unchecked;
    } else {
        m_checkState = state;
    }
}

CheckState CheckBox::getCheckState() const {
    return m_checkState;
}

void CheckBox::setTriStateEnabled(bool enabled) {
    m_triStateEnabled = enabled;
    if (!enabled && m_checkState == CheckState::Indeterminate) {
        m_checkState = CheckState::Unchecked;
    }
}

bool CheckBox::isTriStateEnabled() const {
    return m_triStateEnabled;
}

void CheckBox::setStyle(CheckBoxStyle style) {
    m_style = style;
}

CheckBoxStyle CheckBox::getStyle() const {
    return m_style;
}

void CheckBox::setLayout(CheckBoxLayout layout) {
    m_layout = layout;

    recreate();
}

CheckBoxLayout CheckBox::getLayout() const {
    return m_layout;
}

void CheckBox::setVerticalAlign(CheckBoxVerticalAlign align) {
    m_verticalAlign = align;

    adjustBoxVerticalAlign();
}

CheckBoxVerticalAlign CheckBox::getVerticalAlign() const {
    return m_verticalAlign;
}

void CheckBox::setSizeRatio(float ratio) {
    m_sizeRatio = ratio;

    recreate();
}

float CheckBox::getSizeRatio() const {
    return m_sizeRatio;
}

// 生效的 caption 字号：0 = 使用 ConstDef 默认常量
float CheckBox::effectiveCaptionSize() const {
    return m_captionSize > 0 ? m_captionSize : ConstDef::CHECKBOX_DEFAULT_CAPTION_SIZE;
}

void CheckBox::setCaptionSize(float size) {
    m_captionSize = size;
    if (m_caption != nullptr) {
        m_caption->setFontSize((int)effectiveCaptionSize());  // 回调触发盒子/布局重排
    }
}

float CheckBox::getCaptionSize() const {
    return m_captionSize;
}

void CheckBox::setOnCheckChanged(OnCheckChangedHandler handler) {
    m_onCheckChanged = handler;
}

void CheckBox::setCheckColor(SColor color) {
    m_checkStateColor.setNormal(color);
}

SColor CheckBox::getCheckColor() {
    return m_checkStateColor.getNormal();
}

void CheckBox::setCrossColor(SColor color) {
    m_crossStateColor.setNormal(color);
}

SColor CheckBox::getCrossColor() {
    return m_crossStateColor.getNormal();
}

void CheckBox::setIndeterminateColor(SColor color) {
    m_indeterminateStateColor.setNormal(color);
}

SColor CheckBox::getIndeterminateColor() {
    return m_indeterminateStateColor.getNormal();
}

void CheckBox::setBoxBorderColor(SColor color) {
    m_boxBorderStateColor.setNormal(color);
}

SColor CheckBox::getBoxBorderColor() {
    return m_boxBorderStateColor.getNormal();
}

// float CheckBox::calculateCheckBoxSize() {
//     float fontSize = m_captionSize > 0 ? m_captionSize : ConstDef::CHECKBOX_DEFAULT_CAPTION_SIZE;
//     return fontSize * getScaleXX() * m_sizeRatio;
// }

// SRect CheckBox::calculateCheckBoxRect() {
//     SRect drawRect = getDrawRect();
//     float boxSize = calculateCheckBoxSize();

//     float boxX = (m_layout == CheckBoxLayout::TextRight)
//         ? drawRect.left
//         : drawRect.left + drawRect.width - boxSize;

//     float boxY;
//     switch (m_verticalAlign) {
//         case CheckBoxVerticalAlign::Top:
//             boxY = drawRect.top;
//             break;
//         case CheckBoxVerticalAlign::Bottom:
//             boxY = drawRect.top + drawRect.height - boxSize;
//             break;
//         case CheckBoxVerticalAlign::Center:
//         default:
//             boxY = drawRect.top + (drawRect.height - boxSize) / 2;
//             break;
//     }

//     return {boxX, boxY, boxSize, boxSize};
// }

SRect CheckBox::getBoxDrawRect(){
    SRect boxFrameRect = {m_boxRect.left + m_boxMargin.left, m_boxRect.top + m_boxMargin.top,
                    m_boxRect.width - m_boxMargin.left - m_boxMargin.right,
                    m_boxRect.height - m_boxMargin.top - m_boxMargin.bottom};
    SRect boxDrawRect = mapToDrawRect(boxFrameRect);

    return boxDrawRect;
}

void CheckBox::drawCheckBoxFrame() {
    SRect boxDrawRect = getBoxDrawRect();

    SColor borderColor = getEnable() ? m_boxBorderStateColor.getNormal() : ConstDef::DEFAULT_BORDER_DISABLED_COLOR;

    getRenderDevice()->setDrawColor(borderColor);

    float penWidth = ConstDef::BOX_PEN_WIDTH * getScaleXX();  // 根据X轴缩放比例调整线宽

    switch (m_style) {
        case CheckBoxStyle::Classic:
        case CheckBoxStyle::Cross: {
            SPoint topLeft(boxDrawRect.left + penWidth / 2, boxDrawRect.top + penWidth / 2);
            SPoint topRight(boxDrawRect.left + boxDrawRect.width - penWidth / 2, boxDrawRect.top + penWidth / 2);
            SPoint bottomRight(boxDrawRect.left + boxDrawRect.width - penWidth / 2, boxDrawRect.top + boxDrawRect.height - penWidth / 2);
            SPoint bottomLeft(boxDrawRect.left + penWidth / 2, boxDrawRect.top + boxDrawRect.height - penWidth / 2);

            GraphTool::DrawingContext dc(getRenderDevice());
            dc.setPenColor(GraphTool::SColor(borderColor.red(), borderColor.green(), borderColor.blue(), borderColor.alpha()));
            dc.setPenWidth(penWidth);

            dc.drawLine(topLeft, topRight);
            dc.drawLine(topRight, bottomRight);
            dc.drawLine(bottomRight, bottomLeft);
            dc.drawLine(bottomLeft, topLeft);
            break;
        }
        case CheckBoxStyle::Circle: {
            SPoint boxCenter = boxDrawRect.center();
            float radius = boxDrawRect.width / 2 - penWidth / 2;

            const int numPoints = 36;
            GraphTool::DrawingContext dc(getRenderDevice());
            dc.setPenColor(GraphTool::SColor(borderColor.red(), borderColor.green(), borderColor.blue(), borderColor.alpha()));
            // dc.setPenColor(GraphTool::SColor(1.0f, 1.0f, 1.0f, 1.0f));
            dc.setPenWidth(penWidth);

            dc.drawCircle(boxCenter, radius);

            break;
        }
    }
}

void CheckBox::drawCheckMark() {
    SColor checkColor = getEnable() ? m_checkStateColor.getNormal() : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;

    GraphTool::DrawingContext dc(getRenderDevice());
    dc.setPenColor(GraphTool::SColor(checkColor.red(), checkColor.green(), checkColor.blue(), checkColor.alpha()));
    float penWidth = ConstDef::MARK_PEN_WIDTH * getScaleXX();  // 根据X轴缩放比例调整线宽
    dc.setPenWidth(penWidth);

    SRect boxDrawRect = getBoxDrawRect();

    float padding = boxDrawRect.width * 0.2f;
    float startX = boxDrawRect.left + padding;
    float endX = boxDrawRect.left + boxDrawRect.width - padding;
    float startY = boxDrawRect.top + boxDrawRect.height / 2;
    float midX = boxDrawRect.left + boxDrawRect.width * 0.4f;
    float midY = boxDrawRect.top + boxDrawRect.height * 0.7f;

    dc.drawLine(startX, startY, midX + penWidth / 2 , midY + penWidth / 2);
    dc.drawLine(midX, midY, endX, boxDrawRect.top + padding);
}

void CheckBox::drawCrossMark() {
    SColor crossColor = getEnable() ? m_crossStateColor.getNormal() : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;

    GraphTool::DrawingContext dc(getRenderDevice());
    dc.setPenColor(GraphTool::SColor(crossColor.red(), crossColor.green(), crossColor.blue(), crossColor.alpha()));
    float penWidth = ConstDef::MARK_PEN_WIDTH * getScaleXX();  // 根据X轴缩放比例调整线宽
    dc.setPenWidth(penWidth);

    SRect boxDrawRect = getBoxDrawRect();

    float padding = boxDrawRect.width * 0.2f;
    float startX = boxDrawRect.left + padding;
    float endX = boxDrawRect.left + boxDrawRect.width - padding;
    float startY = boxDrawRect.top + padding;
    float endY = boxDrawRect.top + boxDrawRect.height - padding;

    dc.drawLine(startX, startY, endX, endY);
    dc.drawLine(endX, startY, startX, endY);
}

void CheckBox::drawIndeterminateMark() {
    SColor indColor = getEnable() ? m_indeterminateStateColor.getNormal() : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;

    GraphTool::DrawingContext dc(getRenderDevice());
    dc.setPenColor(GraphTool::SColor(indColor.red(), indColor.green(), indColor.blue(), indColor.alpha()));
    float penWidth = ConstDef::MARK_PEN_WIDTH * getScaleXX();  // 根据X轴缩放比例调整线宽
    dc.setPenWidth(penWidth);

    SRect boxDrawRect = getBoxDrawRect();

    float padding = boxDrawRect.width * 0.25f;
    float lineStartX = boxDrawRect.left + padding;
    float lineEndX = boxDrawRect.left + boxDrawRect.width - padding;
    float lineY = boxDrawRect.top + boxDrawRect.height / 2;

    dc.drawLine(lineStartX, lineY, lineEndX, lineY);
}

/*********************************************************for Builder mode**********************************************************/

CheckBoxBuilder::CheckBoxBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_checkBox(nullptr)
{
    m_checkBox = make_shared<CheckBox>(parent, rect, xScale, yScale);
}

CheckBoxBuilder& CheckBoxBuilder::setStyle(CheckBoxStyle style) {
    m_checkBox->setStyle(style);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setLayout(CheckBoxLayout layout) {
    m_checkBox->setLayout(layout);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setVerticalAlign(CheckBoxVerticalAlign align) {
    m_checkBox->setVerticalAlign(align);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCheckState(CheckState state) {
    m_checkBox->setCheckState(state);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setSizeRatio(float ratio) {
    m_checkBox->setSizeRatio(ratio);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCaptionText(string caption) {
    m_checkBox->setStringProperty(PropertyNames::kCaption, caption.c_str());   // #15：经宿主字段
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCaptionSize(float size) {
    m_checkBox->setCaptionSize(size);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setTriStateEnabled(bool enabled) {
    m_checkBox->setTriStateEnabled(enabled);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setOnCheckChanged(CheckBox::OnCheckChangedHandler handler) {
    m_checkBox->setOnCheckChanged(handler);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCheckColor(SColor color) {
    m_checkBox->setCheckColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCrossColor(SColor color) {
    m_checkBox->setCrossColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setIndeterminateColor(SColor color) {
    m_checkBox->setIndeterminateColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setBoxBorderColor(SColor color) {
    m_checkBox->setBoxBorderColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_checkBox->setBackgroundStateColor(stateColor);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setBorderStateColor(StateColor stateColor) {
    m_checkBox->setBorderStateColor(stateColor);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setTextStateColor(StateColor stateColor) {
    m_checkBox->setTextStateColor(stateColor);
    return *this;
}

// ── Property system overrides ──

int CheckBox::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kCheck) == 0)         { setCheckColor(color);         return 1; }
    if (strcmp(prop, PropertyNames::kCross) == 0)         { setCrossColor(color);         return 1; }
    if (strcmp(prop, PropertyNames::kIndeterminate) == 0) { setIndeterminateColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kBoxBorder) == 0)     { setBoxBorderColor(color);     return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

int CheckBox::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kTriState) == 0) { setTriStateEnabled(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kChecked) == 0)  { setCheckState(value ? CheckState::Checked : CheckState::Unchecked); return 1; }
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // #12：宿主字段存储 + 同步 caption（重建不丢失）
        m_enableTextShadow = (value != 0);
        if (m_caption) m_caption->setShadow(m_enableTextShadow);
        return 1;
    }
    return ControlImpl::setBoolProperty(prop, value);
}

int CheckBox::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kSizeRatio) == 0) { setSizeRatio(value); return 1; }
    // #12：阴影偏移（宿主字段存储 + 同步 caption）
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) {
        m_shadowOffset.x = value;
        if (m_caption) m_caption->setShadowOffset(m_shadowOffset);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
        m_shadowOffset.y = value;
        if (m_caption) m_caption->setShadowOffset(m_shadowOffset);
        return 1;
    }
    return ControlImpl::setFloatProperty(prop, value);
}

int CheckBox::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { setCaptionSize((float)value); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}

int CheckBox::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kCheckBoxStyle) == 0) {
        setStyle(CheckBoxStyleFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kCheckState) == 0) {
        setCheckState(CheckStateFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kLayout) == 0) {
        if (_stricmp(value, PropertyNames::kLayoutTextRight) == 0) { setLayout(CheckBoxLayout::TextRight); return 1; }
        if (_stricmp(value, PropertyNames::kLayoutTextLeft)  == 0) { setLayout(CheckBoxLayout::TextLeft);  return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kVerticalAlign) == 0) {
        if (_stricmp(value, PropertyNames::kVAlignCenter) == 0) { setVerticalAlign(CheckBoxVerticalAlign::Center); return 1; }
        if (_stricmp(value, PropertyNames::kVAlignTop)    == 0) { setVerticalAlign(CheckBoxVerticalAlign::Top);    return 1; }
        if (_stricmp(value, PropertyNames::kVAlignBottom) == 0) { setVerticalAlign(CheckBoxVerticalAlign::Bottom); return 1; }
        return 0;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

int CheckBox::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kCheck) == 0)         { out = getCheckColor();         return 1; }
    if (strcmp(prop, PropertyNames::kCross) == 0)         { out = getCrossColor();         return 1; }
    if (strcmp(prop, PropertyNames::kIndeterminate) == 0) { out = getIndeterminateColor(); return 1; }
    if (strcmp(prop, PropertyNames::kBoxBorder) == 0)     { out = getBoxBorderColor();     return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int CheckBox::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kTriState) == 0) { out = m_triStateEnabled ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kChecked) == 0)  { out = (m_checkState == CheckState::Checked) ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // #12：读宿主字段
        out = m_enableTextShadow ? 1 : 0;
        return 1;
    }
    return ControlImpl::getBoolProperty(prop, out);
}

int CheckBox::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kSizeRatio) == 0) { out = m_sizeRatio; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_shadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_shadowOffset.y; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

int CheckBox::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { out = (int)getCaptionSize(); return 1; }
    return ControlImpl::getIntProperty(prop, out);
}

int CheckBox::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCheckBoxStyle) == 0) {
        switch (m_style) {
            case CheckBoxStyle::Classic: out = PropertyNames::kStyleClassic; return 1;
            case CheckBoxStyle::Cross:   out = PropertyNames::kStyleCross;   return 1;
            case CheckBoxStyle::Circle:  out = PropertyNames::kStyleCircle;  return 1;
        }
    }
    if (strcmp(prop, PropertyNames::kCheckState) == 0) {
        switch (m_checkState) {
            case CheckState::Unchecked:     out = PropertyNames::kCheckUnchecked;     return 1;
            case CheckState::Checked:       out = PropertyNames::kCheckChecked;       return 1;
            case CheckState::Indeterminate: out = PropertyNames::kCheckIndeterminate; return 1;
        }
    }
    if (strcmp(prop, PropertyNames::kLayout) == 0) {
        switch (m_layout) {
            case CheckBoxLayout::TextRight: out = PropertyNames::kLayoutTextRight; return 1;
            case CheckBoxLayout::TextLeft:  out = PropertyNames::kLayoutTextLeft;  return 1;
        }
    }
    if (strcmp(prop, PropertyNames::kVerticalAlign) == 0) {
        switch (m_verticalAlign) {
            case CheckBoxVerticalAlign::Center: out = PropertyNames::kVAlignCenter; return 1;
            case CheckBoxVerticalAlign::Top:    out = PropertyNames::kVAlignTop;    return 1;
            case CheckBoxVerticalAlign::Bottom: out = PropertyNames::kVAlignBottom; return 1;
        }
    }
    return ControlImpl::getEnumProperty(prop, out);
}

int CheckBox::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventCheckChanged) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

CheckBoxBuilder& CheckBoxBuilder::setId(int id) {
    m_checkBox->setId(id);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setEnable(bool enable) {
    m_checkBox->setEnable(enable);
    return *this;
}

shared_ptr<CheckBox> CheckBoxBuilder::build(void) {
    m_checkBox->create();
    return m_checkBox;
}