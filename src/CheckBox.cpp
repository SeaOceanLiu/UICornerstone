#include "CheckBox.h"
#include "GraphTool.h"

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
    // m_captionSize(),
    m_boxRect({0, 0, 0, 0}),
    m_boxMargin(ConstDef::CHECKBOX_BOX_MARGIN),
    m_triStateEnabled(true),
    m_checkStateColor(StateColor::Type::Text),
    m_crossStateColor(StateColor::Type::Text),
    m_indeterminateStateColor(StateColor::Type::Text)
{
    m_checkStateColor.setNormal(ConstDef::CHECKBOX_CHECK_COLOR);
    m_crossStateColor.setNormal(ConstDef::CHECKBOX_CROSS_COLOR);
    m_indeterminateStateColor.setNormal(ConstDef::CHECKBOX_INDETERMINATE_COLOR);
    m_boxBorderStateColor.setNormal(ConstDef::DEFAULT_BORDER_NORMAL_COLOR);

    m_rect = rect;
    setTransparent(true);
    setBorderVisible(false);


    createCaption();
}

void CheckBox::releaseCaption(void){
    if (m_caption != nullptr) {
        m_caption.reset();
        m_caption = nullptr;
        removeControl(m_caption);
    }
}
void CheckBox::createCaption(void){
    if (m_caption != nullptr) {
        return;
    }

    // 先以CheckBox的rect来创建Label，获得Label的文字尺寸，再刷新Label的rect
    m_caption = LabelBuilder(this, {0, 0, getRect().width, getRect().height})
        .setFont(FontName::HarmonyOS_Sans_SC_Regular)
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setFontSize((int)ConstDef::CHECKBOX_DEFAULT_CAPTION_SIZE)
        .setCaption("")
        .setTextStateColor(m_textColor)
        // .setMargin({0, 0, 0, 0})
        .setShadow(false)
        .setShadowOffset({2, 2})
        .setEnableExpand(false)
        // .setDebugDraw(true)
        .setOnPropertyChanged([this](shared_ptr<Label> label){  // Label的属性改变时，通过回调来触发CheckBox调整布局
            setBoxSize();
            adjustSpaceAssignment();
            adjustBoxVerticalAlign();
        })
        .build();
}

Label& CheckBox::getCaption(void) const {
    if (m_caption == nullptr) {
        SDL_Log("CheckBox::getCaption: Caption is null");
        throw std::runtime_error("Caption is null");
    }
    return *m_caption;
}

// setBoxSize必须在createCaption之后调用，因为setBoxSize需要根据caption的行高来设置checkbox的大小
void CheckBox::setBoxSize(void) {
    if (m_caption == nullptr) {
        SDL_Log("CheckBox::setBoxSize: Caption is null");
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
        SDL_Log("CheckBox::adjustBoxVerticalAlign: Caption is null");
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
            SDL_Log("CheckBox::adjustBoxVerticalAlign: Invalid vertical align value");
            break;
    }
}

void CheckBox::recreate(void) {
    // 没有创建过，直接退出，待调用create方法时会创建相关资源
    if(!m_isCreated) return;

    // 释放子控件
    releaseCaption();


    if (typeid(*this) == typeid(CheckBox)) {
        m_isCreated = false;  // 重置创建标志，调用create方法时会重新创建相关资源
        create();
    }
}
void CheckBox::create(void) {
    if (m_isCreated) return;

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
    ControlImpl::preDraw();

    if (!getVisible()) return;

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
}

bool CheckBox::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;

    if (EventQueue::isPositionEvent(event->m_eventName)) {
        if (!event->m_eventParam.has_value()) return false;

        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;

            if (getDrawRect().contains(pos->x, pos->y)) {
                switch (event->m_eventName) {
                    case EventName::MOUSE_LBUTTON_UP:
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
                            m_onCheckChanged(dynamic_pointer_cast<CheckBox>(getThis()), m_checkState);
                        }
                        return true;
                    case EventName::MOUSE_MOVING:
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
        } catch (...) {
            return false;
        }
    }

    return ControlImpl::handleEvent(event);
}

void CheckBox::setRect(SRect rect) {
    ControlImpl::setRect(rect);

    recreate();
}

void CheckBox::onMouseEnter(float x, float y) {
    setState(ControlState::Hover);
}

void CheckBox::onMouseLeave(float x, float y) {
    setState(ControlState::Normal);
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

void CheckBox::setOnCheckChanged(OnCheckChangedHandler handler) {
    m_onCheckChanged = handler;
}

void CheckBox::setCheckColor(SDL_Color color) {
    m_checkStateColor.setNormal(color);
}

SDL_Color CheckBox::getCheckColor() {
    return m_checkStateColor.getNormal();
}

void CheckBox::setCrossColor(SDL_Color color) {
    m_crossStateColor.setNormal(color);
}

SDL_Color CheckBox::getCrossColor() {
    return m_crossStateColor.getNormal();
}

void CheckBox::setIndeterminateColor(SDL_Color color) {
    m_indeterminateStateColor.setNormal(color);
}

SDL_Color CheckBox::getIndeterminateColor() {
    return m_indeterminateStateColor.getNormal();
}

void CheckBox::setBoxBorderColor(SDL_Color color) {
    m_boxBorderStateColor.setNormal(color);
}

SDL_Color CheckBox::getBoxBorderColor() {
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
    SDL_Renderer *renderer = getRenderer();
    if (!renderer) return;

    SRect boxDrawRect = getBoxDrawRect();

    SDL_Color borderColor = getEnable() ? m_boxBorderStateColor.getNormal() : ConstDef::DEFAULT_BORDER_DISABLED_COLOR;

    if (!SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a)) {
        return;
    }

    float penWidth = ConstDef::BOX_PEN_WIDTH * getScaleXX();  // 根据X轴缩放比例调整线宽

    switch (m_style) {
        case CheckBoxStyle::Classic:
        case CheckBoxStyle::Cross: {
            SPoint topLeft(boxDrawRect.left + penWidth / 2, boxDrawRect.top + penWidth / 2);
            SPoint topRight(boxDrawRect.left + boxDrawRect.width - penWidth / 2, boxDrawRect.top + penWidth / 2);
            SPoint bottomRight(boxDrawRect.left + boxDrawRect.width - penWidth / 2, boxDrawRect.top + boxDrawRect.height - penWidth / 2);
            SPoint bottomLeft(boxDrawRect.left + penWidth / 2, boxDrawRect.top + boxDrawRect.height - penWidth / 2);

            GraphTool::DrawingContext dc(renderer);
            dc.setPenColor(GraphTool::SColor(borderColor.r / 255.0f, borderColor.g / 255.0f, borderColor.b / 255.0f, borderColor.a / 255.0f));
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
            GraphTool::DrawingContext dc(renderer);
            dc.setPenColor(GraphTool::SColor(borderColor.r / 255.0f, borderColor.g / 255.0f, borderColor.b / 255.0f, borderColor.a / 255.0f));
            dc.setPenWidth(penWidth);

            dc.drawCircle(boxCenter, radius);

            break;
        }
    }
}

void CheckBox::drawCheckMark() {
    SDL_Renderer *renderer = getRenderer();
    if (!renderer) return;

    SDL_Color checkColor = getEnable() ? m_checkStateColor.getNormal() : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;

    GraphTool::DrawingContext dc(renderer);
    dc.setPenColor(GraphTool::SColor(checkColor.r / 255.0f, checkColor.g / 255.0f, checkColor.b / 255.0f, checkColor.a / 255.0f));
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
    SDL_Renderer *renderer = getRenderer();
    if (!renderer) return;

    SDL_Color crossColor = getEnable() ? m_crossStateColor.getNormal() : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;

    GraphTool::DrawingContext dc(renderer);
    dc.setPenColor(GraphTool::SColor(crossColor.r / 255.0f, crossColor.g / 255.0f, crossColor.b / 255.0f, crossColor.a / 255.0f));
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
    SDL_Renderer *renderer = getRenderer();
    if (!renderer) return;

    SDL_Color indColor = getEnable() ? m_indeterminateStateColor.getNormal() : ConstDef::DEFAULT_TEXT_DISABLED_COLOR;

    GraphTool::DrawingContext dc(renderer);
    dc.setPenColor(GraphTool::SColor(indColor.r / 255.0f, indColor.g / 255.0f, indColor.b / 255.0f, indColor.a / 255.0f));
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
    m_checkBox->getCaption().setCaption(caption);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCaptionSize(float size) {
    m_checkBox->getCaption().setFontSize(size);
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

CheckBoxBuilder& CheckBoxBuilder::setCheckColor(SDL_Color color) {
    m_checkBox->setCheckColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setCrossColor(SDL_Color color) {
    m_checkBox->setCrossColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setIndeterminateColor(SDL_Color color) {
    m_checkBox->setIndeterminateColor(color);
    return *this;
}

CheckBoxBuilder& CheckBoxBuilder::setBoxBorderColor(SDL_Color color) {
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