#include "Button.h"
#include "PropertyNames.h"
#include "PlatformUtils.h"
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
    m_ctlType = ControlType::Button;
    m_rect = rect;
    setFocusable(true);
    // P0-30：历史缺省态显式化（保持既有 hover/pressed 视觉）
    setHoverStateBGColor(ConstDef::DEFAULT_HOVER_COLOR);
    setPressedStateBGColor(ConstDef::DEFAULT_DOWN_COLOR);
    setHoverStateBDColor(ConstDef::DEFAULT_BORDER_HOVER_COLOR);
    setPressedStateBDColor(ConstDef::DEFAULT_BORDER_DOWN_COLOR);
    m_textColor.setHover(ConstDef::DEFAULT_TEXT_HOVER_COLOR);
    m_textColor.setPressed(ConstDef::DEFAULT_TEXT_DOWN_COLOR);
    m_textShadowColor.setHover(ConstDef::DEFAULT_TEXT_SHADOW_HOVER_COLOR);
    m_textShadowColor.setPressed(ConstDef::DEFAULT_TEXT_SHADOW_DOWN_COLOR);
}

void Button::create(void){
    if (m_isCreated) return;

    ControlImpl::create();
    // 状态 Actor 不在 m_children（setParent 仅挂渲染/事件链），
    // setContext 的子树传播到达不了它们，这里在 context 就绪后补建
    auto ensureActor = [this](shared_ptr<Actor>& actor){
        if (actor != nullptr && !actor->isCreated()){
            actor->setParent(this);
            actor->create();
        }
    };
    ensureActor(m_actor);
    ensureActor(m_hoverActor);
    ensureActor(m_pressedActor);
    ensureActor(m_disabledActor);
}

// 状态 Actor 与内嵌 LuotiAni 不在 m_children（树内传播到不了），
// 父链缩放变更时在此收口刷新非树成员（drawRect 仍经 setParent 的父级基准派生）
void Button::refreshScaleWith(float parentXX, float parentYY){
    ControlImpl::refreshScaleWith(parentXX, parentYY);
    auto refreshActor = [this](shared_ptr<Actor>& child){
        if (child) child->refreshScaleWith(m_xxScale, m_yyScale);
    };
    refreshActor(m_actor);
    refreshActor(m_hoverActor);
    refreshActor(m_pressedActor);
    refreshActor(m_disabledActor);
    if (m_luotiAni) m_luotiAni->refreshScaleWith(m_xxScale, m_yyScale);
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

    ControlImpl::beforeDraw();

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

    // 5. 最后绘制边框
    afterDraw();

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
        SRect drawRect = getDrawRect();
        if (drawRect.contains(mx, my)){
            if (event->m_type == EventType::FingerDown || event->m_type == EventType::FingerMotion) {
                if (m_onClick != nullptr){
                    m_onClick(dynamic_pointer_cast<Button>(this->getThis()));
                }
                fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
                setState(ControlState::Pressed);
                return true;
            }
            if (event->m_type == EventType::FingerUp) {
                setState(ControlState::Normal);
                return true;
            }
            if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
                setState(ControlState::Pressed);
                return true;
            }
            if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
                if (m_onClick != nullptr && m_state == ControlState::Pressed){
                    m_onClick(dynamic_pointer_cast<Button>(this->getThis()));
                }
                fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
                setState(ControlState::Normal);
                return true;
            }
            if (event->m_type == EventType::MouseMove) {
                setState(ControlState::Hover);
                return true;
            }
            return true;
        } else {
            setState(ControlState::Normal);
        }
    }
    // Keyboard activation: Enter / Space
    if (event->m_type == EventType::KeyDown && getFocused()) {
        if (event->keyEvent.keycode == KeyCode::Return ||
            event->keyEvent.keycode == KeyCode::Space) {
            setState(ControlState::Pressed);
            if (m_onClick)
                m_onClick(dynamic_pointer_cast<Button>(this->getThis()));
            fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
            return true;
        }
    }
    if (event->m_type == EventType::KeyUp && getFocused()) {
        if (event->keyEvent.keycode == KeyCode::Return ||
            event->keyEvent.keycode == KeyCode::Space) {
            setState(ControlState::Normal);
            return true;
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
    if (m_luotiAni != nullptr){
        m_luotiAni->setRect({0, 0, m_rect.width, m_rect.height});
    }
    // 状态 Actor 与按钮同尺寸（拉伸填满按钮），按钮尺寸变化时需同步，
    // 否则图片停留在旧尺寸矩形上（拉伸失效/错位）
    if (m_actor != nullptr){
        m_actor->setRect({0, 0, m_rect.width, m_rect.height});
    }
    if (m_hoverActor != nullptr){
        m_hoverActor->setRect({0, 0, m_rect.width, m_rect.height});
    }
    if (m_pressedActor != nullptr){
        m_pressedActor->setRect({0, 0, m_rect.width, m_rect.height});
    }
    if (m_disabledActor != nullptr){
        m_disabledActor->setRect({0, 0, m_rect.width, m_rect.height});
    }
}

/*********************************************************for Builder mode**********************************************************/

void Button::setNormalStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    actor->setVisible(true);                                  // P0-20①：运行时构造的 Actor m_visible 默认 false
    if (isCreated() && !actor->isCreated()) actor->create();  // P0-20②：ensureActor 只在 Button::create 跑一次
    m_actor = actor;
}
void Button::setHoverStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    actor->setVisible(true);                                  // P0-20①
    if (isCreated() && !actor->isCreated()) actor->create();  // P0-20②
    m_hoverActor = actor;
}

void Button::setPressedStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    actor->setVisible(true);                                  // P0-20①
    if (isCreated() && !actor->isCreated()) actor->create();  // P0-20②
    m_pressedActor = actor;
}
void Button::setDisabledStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    actor->setVisible(true);                                  // P0-20①
    if (isCreated() && !actor->isCreated()) actor->create();  // P0-20②
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

void Button::setState(ControlState state){
    ControlImpl::setState(state);
    // 状态联动：caption 的 hover/pressed/disabled 各态色经此可达（Label::draw 按自身 state 取色）
    if (m_caption != nullptr){
        m_caption->setState(state);
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
                            .setShadowOffset(m_shadowOffset)
                            .build();
        m_caption->setTransparent(true);
        addControl(m_caption);
        m_caption->setState(getState());   // (重)建时同步当前状态
    }
}

void Button::setCaptionLabel(shared_ptr<Label> label){
    if (m_caption != nullptr){
        removeControl(m_caption);
        m_caption.reset();
    }
    m_caption = label;
    if (m_caption != nullptr){
        m_caption->setParent(this);
        m_caption->setRect({0, 0, m_rect.width, m_rect.height});
        m_captionText = m_caption->getCaption();
        addControl(m_caption);
        m_caption->setState(getState());   // 替换时同步当前状态
        // #12：宿主 shadow 状态应用到新 caption（button 级配置优先于 caption-label 内嵌配置）
        m_caption->setShadow(m_enableTextShadow);
        m_caption->setShadowOffset(m_shadowOffset);
    }
}

shared_ptr<Label> Button::getCaptionLabel(void) const {
    return m_caption;
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
float Button::getCaptionSize() const{
    return m_captionSize;
}
SRect Button::getCaptionRect(void) const{
    return m_caption != nullptr ? m_caption->getHotRect() : SRect(0, 0, 0, 0);
}
void Button::setLuotiAni(shared_ptr<LuotiAni>luotiAni){
    m_luotiAni = luotiAni;
    if (m_luotiAni != nullptr){
        m_luotiAni->setParent(this);
        m_luotiAni->setRect({0, 0, m_rect.width, m_rect.height});
        m_luotiAni->setVisible(true);
    }
}
void Button::setRenderDevice(RenderDevice* device) {
    ControlImpl::setRenderDevice(device);
    if (m_luotiAni != nullptr) m_luotiAni->setRenderDevice(device);
}
void Button::setOnClick(OnClickHandler onClick){
    m_onClick = onClick;
}

// ── Property system overrides ──
int Button::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { setTextShadowEnable(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kPlaying) == 0 && m_luotiAni) return m_luotiAni->setBoolProperty(prop, value);
    return ControlImpl::setBoolProperty(prop, value);
}
int Button::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { out = m_enableTextShadow ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kPlaying) == 0 && m_luotiAni) return m_luotiAni->getBoolProperty(prop, out);
    return ControlImpl::getBoolProperty(prop, out);
}
int Button::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { setCaptionSize((float)value); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}
int Button::setFloatProperty(const char* prop, float value) {
    // #12：阴影偏移（宿主字段存储 + 同步内部 caption；caption 重建/替换后不丢失）
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

int Button::getPtrProperty(const char* prop, void*& out) {
    // 暴露内部 caption Label 句柄：应用经句柄直控 Label 标准属性（颜色/字体/对齐）。
    // 键与 JSON 布局键同键同常量（kCaptionLabel）；句柄归属校验经 Button 子树通过。
    if (strcmp(prop, PropertyNames::kCaptionLabel) == 0) {
        // 句柄约定：存 Control* 基地址（ControlImpl 虚继承 Control，须经基类转换修正偏移）
        out = m_caption ? static_cast<Control*>(m_caption.get()) : nullptr;
        return m_caption ? 1 : 0;
    }
    return ControlImpl::getPtrProperty(prop, out);
}

int Button::setPtrProperty(const char* prop, void* value) {
    if (strcmp(prop, PropertyNames::kLuotiAni) == 0) {
        // luotiAni 借用语义（与 leadingControl 一致）：生命周期由调用方保证
        if (value) {
            setLuotiAni(shared_ptr<LuotiAni>(static_cast<LuotiAni*>(value), [](LuotiAni*){}));
        } else {
            setLuotiAni(nullptr);
        }
        return 1;
    }
    return ControlImpl::setPtrProperty(prop, value);
}

int Button::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) { setCaption(value); return 1; }
    if (strcmp(prop, PropertyNames::kAnimation) == 0) {
        if (!value || !value[0]) return 0;
        try {   // Bonus A：镜像 LuotiAni::setStringProperty——失败返回 0、保留旧动画、可重试（原无保护会 terminate）
            auto ani = make_shared<LuotiAni>(this);
            ani->loadFromFile(fs::path(value));   // 原值直传：basePath 解析与 provider: 分流在 loadFromFile（P0-21 原值读回）
            setLuotiAni(ani);                     // 加载成功才替换旧动画
            ani->prepare();
            ani->play();
        } catch (...) {
            return 0;
        }
        return 1;
    }
    // 状态图：设置后创建对应状态 Actor（matchParentRect=true 跟随按钮框体；
    // Actor 缩放传 1.0f —— 仅继承父级缩放，避免 xScale² 叠加）
    if (strcmp(prop, PropertyNames::kNormalImage) == 0 ||
        strcmp(prop, PropertyNames::kHoverImage) == 0 ||
        strcmp(prop, PropertyNames::kPressedImage) == 0 ||
        strcmp(prop, PropertyNames::kDisabledImage) == 0) {
        if (!value) return 0;
        // 原值直传：basePath 解析与 provider: 分流统一在 Actor::loadFromFile（P0-21 原值读回）
        auto actor = make_shared<Actor>(this, fs::path(value), true, 1.0f, 1.0f);
        actor->setScaleType(ScaleType::FIT_CENTER);
        if (strcmp(prop, PropertyNames::kNormalImage) == 0)         setNormalStateActor(actor);
        else if (strcmp(prop, PropertyNames::kHoverImage) == 0)    setHoverStateActor(actor);
        else if (strcmp(prop, PropertyNames::kPressedImage) == 0)  setPressedStateActor(actor);
        else                                                       setDisabledStateActor(actor);
        return 1;
    }
    return ControlImpl::setStringProperty(prop, value);
}

int Button::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { out = (int)getCaptionSize(); return 1; }
    if ((strcmp(prop, PropertyNames::kTotalFrames) == 0 || strcmp(prop, PropertyNames::kCurrentFrame) == 0) && m_luotiAni)
        return m_luotiAni->getIntProperty(prop, out);   // Bonus B：内嵌动画 frames 读回转发
    return ControlImpl::getIntProperty(prop, out);
}
int Button::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_shadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_shadowOffset.y; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

// P0-21：状态 Actor 文件路径读回（稳定存储于 Actor 成员；未设置/资源引用返回 0）
static int buttonActorFilePath(const shared_ptr<Actor>& actor, const char*& out) {
    if (!actor || actor->getFilePathStr().empty()) return 0;
    out = actor->getFilePathStr().c_str();
    return 1;
}
int Button::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) { out = m_captionText.c_str(); return 1; }
    if (strcmp(prop, PropertyNames::kNormalImage) == 0)   return buttonActorFilePath(m_actor, out);
    if (strcmp(prop, PropertyNames::kHoverImage) == 0)    return buttonActorFilePath(m_hoverActor, out);
    if (strcmp(prop, PropertyNames::kPressedImage) == 0)  return buttonActorFilePath(m_pressedActor, out);
    if (strcmp(prop, PropertyNames::kDisabledImage) == 0) return buttonActorFilePath(m_disabledActor, out);
    if (strcmp(prop, PropertyNames::kAnimation) == 0) {
        if (m_luotiAni && !m_luotiAni->getFilePathStr().empty()) { out = m_luotiAni->getFilePathStr().c_str(); return 1; }
        return 0;
    }
    return ControlImpl::getStringProperty(prop, out);
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
