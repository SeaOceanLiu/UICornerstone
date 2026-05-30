#include "Button.h"
Button::Button(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale),
    m_onClick(nullptr),
    m_actor(nullptr),
    m_hoverActor(nullptr),
    m_pressedActor(nullptr),
    m_disabledActor(nullptr),
    m_caption(nullptr),
    m_enableTextShadow(false),
    m_luotiAni(nullptr),
    m_captionSize(ConstDef::BUTTON_CAPTION_SIZE)
{
    m_rect = rect;
}

void Button::update(void){
    if (!getEnable()) return;

    // 更新 LuotiAni 粒子动画（如果有）
    if (m_luotiAni != nullptr) {
        m_luotiAni->update();
    }

    // 如果有子控件，这里需要更新子控件
    ControlImpl::update();
}
void Button::draw(void){
    if (!getVisible()) return;

    ControlImpl::preDraw();

    // 2. 绘制当前控件的图标
    auto actor = m_actor;
    switch(getState()){
        case ControlState::Disabled:
            if (m_disabledActor != nullptr){
                actor = m_disabledActor;
            }
            break;
        case ControlState::Hover:
            if (m_hoverActor != nullptr){
                actor = m_hoverActor;
            }
            break;
        case ControlState::Pressed:
            if (m_pressedActor != nullptr){
                actor = m_pressedActor;
            }
            break;
        case ControlState::Normal:
        default:
            break;
    }

    if (actor != nullptr){
        actor->draw();
    }

    // 4. 绘制动画
    if(m_luotiAni != nullptr){
        m_luotiAni->draw();
    }

    // 3. 绘制当前控件的标题
    if (m_caption != nullptr){
        m_caption->draw();
    }

    // 4. 接着绘制子控件
    ControlImpl::draw();

    // // 5. 最后绘制边框
    // if(!m_isTransparent && m_isBorderVisible) {
    //     SDL_Color borderColor;
    //     switch (m_state){
    //         case ControlState::Disabled:
    //             borderColor = m_borderColor.getDisabled();
    //             break;
    //         case ControlState::Hover:
    //             borderColor = m_borderColor.getHover();
    //             break;
    //         case ControlState::Pressed:
    //             borderColor = m_borderColor.getPressed();
    //             break;
    //         case ControlState::Normal:
    //         default:
    //             borderColor = m_borderColor.getNormal();
    //             break;
    //     }
    //     if(!SDL_SetRenderDrawColor(getRenderer(), borderColor.r, borderColor.g, borderColor.b, borderColor.a)){
    //         SDL_Log("Panel fFailed to set border color: %s", SDL_GetError());
    //     }
    //     if(!SDL_RenderRect(getRenderer(), drawRect.toSDLFRect())){
    //         SDL_Log("Panel failed to draw border: %s", SDL_GetError());
    //     }
    // }
}

bool Button::handleEvent(shared_ptr<Event> event){
    if (!getEnable() || !getVisible()) return false;

    if (EventQueue::isPositionEvent(event->m_eventName)){
        if (!event->m_eventParam.has_value()) return false;
        try {
            shared_ptr<SPoint> pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            SRect drawRect = getDrawRect();
            if (drawRect.contains(pos->x, pos->y)){
                SDL_Log("Button::handleEvent: %d", event->m_eventName);
                switch(event->m_eventName){
                    case EventName::FINGER_DOWN:
                    case EventName::FINGER_MOTION:
                        if (m_onClick != nullptr){
                            m_onClick(dynamic_pointer_cast<Button>(this->getThis()));
                        }
                        setState(ControlState::Pressed);
                        return true;
                    case EventName::MOUSE_LBUTTON_DOWN:
                        setState(ControlState::Pressed);
                        return true;
                    case EventName::MOUSE_LBUTTON_UP:
                        SDL_Log("Button::handleEvent m_onClick = 0x%0X, m_state = %d", m_onClick, m_state);
                        if (m_onClick != nullptr && m_state == ControlState::Pressed){
                            m_onClick(dynamic_pointer_cast<Button>(this->getThis()));
                        }
                    case EventName::FINGER_UP:
                        setState(ControlState::Normal);
                        return true;
                    case EventName::MOUSE_MOVING:
                        setState(ControlState::Hover);
                        return true;
                    case EventName::MOUSE_WHEEL:
                        return false;
                    default:
                        break;
                }
                return true;
            } else {
                setState(ControlState::Normal);
            }
        } catch (...) {
            return false;
        }
    }
    if (ControlImpl::handleEvent(event)) return true;
    return false;
}
void Button::onMouseEnter(float x, float y)
{
    setState(ControlState::Hover);
}

void Button::onMouseLeave(float x, float y)
{
    setState(ControlState::Normal);
}

void Button::setRect(SRect rect){
    ControlImpl::setRect(rect);

    if (m_caption != nullptr){
        m_caption->setRect({0, 0, m_rect.width, m_rect.height});
    }
}

/*********************************************************for Builder mode**********************************************************/

void Button::setNormalStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    m_actor = actor;
}
void Button::setHoverStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    m_hoverActor = actor;
}

void Button::setPressedStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    m_pressedActor = actor;
}
void Button::setDisabledStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    m_disabledActor = actor;
}

void Button::setTextStateColor(StateColor stateColor){
    ControlImpl::setTextStateColor(stateColor);
    if (m_caption != nullptr){
        m_caption->setTextStateColor(stateColor);
    }
}
void Button::setTextShadowStateColor(StateColor stateColor){
    ControlImpl::setTextShadowStateColor(stateColor);
    if (m_caption != nullptr){
        m_caption->setTextShadowStateColor(stateColor);
    }
}

void Button::setTextShadowEnable(bool enable){
    m_enableTextShadow = enable;
    if (m_caption != nullptr){
        m_caption->setShadow(enable);
    }
}
void Button::setCaption(string caption){
    m_captionText = caption;

    if (m_caption != nullptr){
        removeControl(m_caption);

        m_caption.reset();
        m_caption = nullptr;
    }
    if (m_captionText.length() > 0) {
        m_caption = LabelBuilder(this, {0, 0, m_rect.width, m_rect.height})
                            .setFont(FontName::HarmonyOS_Sans_SC_Regular)
                            .setAlignmentMode(AlignmentMode::AM_CENTER)
                            .setFontSize((int)m_captionSize)
                            .setCaption(m_captionText)
                            .setTextStateColor(m_textColor)
                            .setTextShadowStateColor(m_textShadowColor)
                            .setShadow(m_enableTextShadow)
                            .build();
        m_caption->setTransparent(true);
        addControl(m_caption);
    }
}
string Button::getCaption(void) const{
    return m_captionText;
}
void Button::setCaptionSize(float size){
    m_captionSize = size;
    if (m_caption != nullptr){
        m_caption->setFontSize((int)m_captionSize);
    }
}
uint32_t Button::getCaptionSize(float size) const{
    return static_cast<uint32_t>(m_captionSize);
}
SRect Button::getCaptionRect(void) const{
    return m_caption != nullptr ? m_caption->getHotRect() : SRect(0, 0, 0, 0);
}
void Button::setLuotiAni(shared_ptr<LuotiAni>luotiAni){
    m_luotiAni = luotiAni;
    if (m_luotiAni != nullptr){
        // 设置动画的父控件为当前控件
        m_luotiAni->setParent(this);
        m_luotiAni->setVisible(true);
    }
}
void Button::setOnClick(OnClickHandler onClick){
    m_onClick = onClick;
}

void Button::setRenderer(SDL_Renderer* renderer){
    ControlImpl::setRenderer(renderer);
    if (m_luotiAni != nullptr) {
        m_luotiAni->setRenderer(renderer);
    }
}

ButtonBuilder::ButtonBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_button(nullptr)
{
    m_button = make_shared<Button>(parent, rect, xScale, yScale);
}
ButtonBuilder& ButtonBuilder::setNormalStateActor(shared_ptr<Actor> actor){
    m_button->setNormalStateActor(actor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setHoverStateActor(shared_ptr<Actor> actor){
    m_button->setHoverStateActor(actor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setPressedStateActor(shared_ptr<Actor> actor){
    m_button->setPressedStateActor(actor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setDisabledStateActor(shared_ptr<Actor> actor){
    m_button->setDisabledStateActor(actor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setBackgroundStateColor(StateColor stateColor){
    m_button->setBackgroundStateColor(stateColor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setBorderStateColor(StateColor stateColor){
    m_button->setBorderStateColor(stateColor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setTextStateColor(StateColor stateColor){
    m_button->setTextStateColor(stateColor);
    return *this;
}
ButtonBuilder& ButtonBuilder::setTextShadowStateColor(StateColor stateColor){
    m_button->setTextShadowStateColor(stateColor);
    return *this;
}

ButtonBuilder& ButtonBuilder::setCaption(string caption){
    m_button->setCaption(caption);
    return *this;
}
ButtonBuilder& ButtonBuilder::setCaptionSize(float size){
    m_button->setCaptionSize(size);
    return *this;
}
ButtonBuilder& ButtonBuilder::setLuotiAni(shared_ptr<LuotiAni>luotiAni){
    m_button->setLuotiAni(luotiAni);
    return *this;
}
ButtonBuilder& ButtonBuilder::addControl(shared_ptr<Control> child){
    m_button->addControl(child);
    return *this;
}
ButtonBuilder& ButtonBuilder::setOnClick(Button::OnClickHandler onClick){
    m_button->setOnClick(onClick);
    return *this;
}
ButtonBuilder& ButtonBuilder::setTransparent(bool isTransparent){
    m_button->setTransparent(isTransparent);
    return *this;
}
ButtonBuilder& ButtonBuilder::setId(int id){
    m_button->setId(id);
    return *this;
}
shared_ptr<Button> ButtonBuilder::build(void){
    m_button->create();
    return m_button;
}
