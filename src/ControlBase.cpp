#include "ControlBase.h"
ControlImpl::ControlImpl(Control *parent, float xScale, float yScale):
    // m_weakThis(this),
    // m_sharedThis(nullptr),
    m_isCreated(false),
    m_isTransparent(false),
    m_isBorderVisible(false),
    m_state(ControlState::Normal),
    m_id(INT_MAX),
    m_parent(parent),
    m_enable(true),
    m_visible(false),
    m_xScale(xScale),
    m_yScale(yScale),
    m_xxScale(parent==nullptr?xScale:xScale*parent->getScaleXX()),
    m_yyScale(parent==nullptr?yScale:yScale*parent->getScaleYY()),

    m_bgColor(StateColor()),
    m_borderColor(StateColor(StateColor::Type::Border)),
    m_textColor(StateColor(StateColor::Type::Text)),
    m_textShadowColor(StateColor(StateColor::Type::TextShadow)),

    m_surface(nullptr),
    m_renderer(nullptr),
    m_renderDevice(nullptr),
    m_texture(nullptr),
    m_rect({0, 0, 0, 0}),
    m_mouseInside(false)
{
    m_eventQueueInstance = EventQueue::getInstance();
    // if (m_parent != nullptr){
        inheritRenderer();
    // }
}

ControlImpl::ControlImpl(const ControlImpl &other):
    m_isCreated(other.m_isCreated),
    m_isTransparent(other.m_isTransparent),
    m_isBorderVisible(other.m_isBorderVisible),
    m_state(other.m_state),
    m_id(other.m_id),
    m_parent(other.m_parent),
    m_enable(other.m_enable),
    m_visible(other.m_visible),
    m_xScale(other.m_xScale),
    m_yScale(other.m_yScale),
    m_xxScale(other.m_xxScale),
    m_yyScale(other.m_yyScale),

    m_bgColor(other.m_bgColor),
    m_borderColor(other.m_borderColor),
    m_textColor(other.m_textColor),

    m_renderer(other.m_renderer),
    m_renderDevice(other.m_renderDevice),
    m_surface(other.m_surface),
    m_texture(other.m_texture),
    m_rect(other.m_rect),
    m_mouseInside(other.m_mouseInside)
{
    m_eventQueueInstance = other.m_eventQueueInstance;

    for(const auto& child : other.m_children){
        // shared_ptr<ControlImpl> newChild = make_shared<ControlImpl>(&child); // Todo: 这里需要深拷贝
        // addControl(newChild);
    }

}
ControlImpl& ControlImpl::operator=(const ControlImpl &other){
    if (this == &other) return *this;
    m_isCreated = other.m_isCreated;
    m_isTransparent = other.m_isTransparent;
    m_isBorderVisible = other.m_isBorderVisible;
    m_state = other.m_state;
    m_id = other.m_id;
    m_parent = other.m_parent;
    m_enable = other.m_enable;
    m_visible = other.m_visible;
    m_xScale = other.m_xScale;
    m_yScale = other.m_yScale;
    m_xxScale = other.m_xxScale;
    m_yyScale = other.m_yyScale;
    m_renderer = other.m_renderer;
    m_surface = other.m_surface;
    m_texture = other.m_texture;
    m_rect = other.m_rect;
    m_mouseInside = other.m_mouseInside;
    m_eventQueueInstance = other.m_eventQueueInstance;

    for(const auto& child : other.m_children){
        // shared_ptr<ControlImpl>  newChild = make_shared<ControlImpl>(child); // 这里需要深拷贝
        // addControl(newChild);
    }

    return *this;
}

void ControlImpl::recreate(void) {
    if(!m_isCreated) return;
    m_isCreated = false;

    if (typeid(*this) == typeid(ControlImpl)) {
        create();
    }
}

void ControlImpl::create(void){
    if(!m_isCreated) {
        m_isCreated = true;
        setVisible(true);
    }
}

void ControlImpl::update(void){
    if(!getEnable()) return;

    // 检测鼠标进入/退出状态
    if (getVisible() && getEnable()) {
        // 获取当前鼠标位置（需要从SDL获取）
        float mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        SRect drawRect = getDrawRect();
        bool isInside = drawRect.contains(mouseX, mouseY);

        // 检测鼠标进入/退出状态变化
        if (isInside && !m_mouseInside) {
            // 鼠标进入控件区域
            m_mouseInside = true;
            // 转换为控件内坐标系
            float localX = mouseX - drawRect.left;
            float localY = mouseY - drawRect.top;
            onMouseEnter(localX, localY);
        } else if (!isInside && m_mouseInside) {
            // 鼠标退出控件区域
            m_mouseInside = false;
            // 转换为控件内坐标系
            float localX = mouseX - drawRect.left;
            float localY = mouseY - drawRect.top;
            onMouseLeave(localX, localY);
        }
    }

    for (auto& child : m_children){
        child->update();
    }
}

void ControlImpl::draw(void){
    if (!getVisible()) return;

    inheritRenderer();

    // draw the control
    // ...
    // draw the children
    for (auto& child : m_children){
        child->draw();
    }
}
void ControlImpl::preDraw() {
    if (!getVisible()) return;

    SRect drawRect = getDrawRect();
    drawBackground(&drawRect);
    drawBorder(&drawRect);
}
void ControlImpl::drawBackground(const SRect *pDrawRect){
    SRect drawRect;

    if(!getTransparent()) {
        if (pDrawRect == nullptr){
            drawRect = getDrawRect();
        } else {
            drawRect = *pDrawRect;
        }

        // 背景色
        SColor bgColor;
        switch (m_state){
            case ControlState::Disabled:
                bgColor = m_bgColor.getDisabled();
            case ControlState::Hover:
                bgColor = m_bgColor.getHover();
                break;
            case ControlState::Pressed:
                bgColor = m_bgColor.getPressed();
                break;
            case ControlState::Normal:
            default:
                bgColor = m_bgColor.getNormal();
                break;
        }
        getRenderDevice()->setDrawColor(bgColor);
        getRenderDevice()->fillRect(drawRect);
    }
}

void ControlImpl::drawBorder(const SRect *pDrawRect){
    SRect drawRect;

    if(getBorderVisible()) {
        if (pDrawRect == nullptr){
            drawRect = getDrawRect();
        } else {
            drawRect = *pDrawRect;
        }

        SColor borderColor;
        switch (m_state){
            case ControlState::Disabled:
                borderColor = m_borderColor.getDisabled();
                break;
            case ControlState::Hover:
                borderColor = m_borderColor.getHover();
                break;
            case ControlState::Pressed:
                borderColor = m_borderColor.getPressed();
                break;
            case ControlState::Normal:
            default:
                borderColor = m_borderColor.getNormal();
                break;
        }
        getRenderDevice()->setDrawColor(borderColor);
        getRenderDevice()->drawRect(drawRect);
    }
}

void ControlImpl::resized(SRect newRect){
    m_rect.width = newRect.width;
    m_rect.height = newRect.height;
}
void ControlImpl::moved(SRect newRect){
    m_rect.left = newRect.left;
    m_rect.top = newRect.top;
}
//事件处理，返回值表示是否处理了该事件，true表示处理了，false表示未处理
bool ControlImpl::handleEvent(shared_ptr<Event> event){
    // 检查当前控件是否可见且启用
    if (getVisible() && getEnable()){
        // 逆向遍历当前控件的所有子控件，保证后添加的控件先处理事件，因为后添加的控件在屏幕上位于上层
        // 使用副本遍历，防止子控件的 bringToFront 等操作修改 m_children 导致迭代器失效
        auto childrenCopy = m_children;
        for (auto it = childrenCopy.rbegin(); it != childrenCopy.rend(); ++it){
            // 调用子控件的handleEvent函数处理事件
            if ((*it)->handleEvent(event)){
                // 如果子控件处理了事件，则返回true
                return true;
            }
        }

    }
    // 如果当前控件及其子控件均未处理事件，则返回false
    return false;
}

bool ControlImpl::beforeEventHandlingWatcher(shared_ptr<Event> event){
    return false;
}

bool ControlImpl::afterEventHandlingWatcher(shared_ptr<Event> event){
    return false;
}

void ControlImpl::addControl(shared_ptr<Control> child){
    if (child == nullptr) return;

    // 如果控件已经存在，则直接返回
    if (std::find(m_children.begin(), m_children.end(), child) != m_children.end()){
        return;
    }
    m_children.push_back(child);

    child->setParent(this);
    child->setRenderer(getRenderer());
}

void ControlImpl::removeControl(shared_ptr<Control> child){
    m_children.erase(std::remove(m_children.begin(), m_children.end(), child), m_children.end());
}
// 只调用setParent的话，是不会添加到父控件的children中的，用于自行控制绘制逻辑和事件处理逻辑
// 如果要自动绘制和处理事件，需要调用addControl
void ControlImpl::setParent(Control *parent){
    m_parent = parent;
    inheritRenderer();
    m_xxScale = (parent==nullptr?m_xScale:m_xScale*parent->getScaleXX());
    m_yyScale = (parent==nullptr?m_yScale:m_yScale*parent->getScaleYY());
}

Control* ControlImpl::getParent(void){
    return m_parent;
}

float ControlImpl::getScaleXX(void){
    return m_xxScale;
}
float ControlImpl::getScaleYY(void){
    return m_yyScale;
}
void ControlImpl::setScaleX(float xScale){
    m_xScale = xScale;
    for (auto& child : m_children){
        child->setParent(this); // 通过重新设置父控件来更新子控件的实际缩放值
    }
}
void ControlImpl::setScaleY(float yScale){
    m_yScale = yScale;
    for (auto& child : m_children){
        child->setParent(this); // 通过重新设置父控件来更新子控件的实际缩放值
    }
}

void ControlImpl::setRect(SRect rect){
    m_rect = rect;
}

SRect ControlImpl::getRect(void){
    return m_rect;
}
void ControlImpl::setMargin(Margin margin){
    m_margin = margin;

    recreate();
}
Margin ControlImpl::getMargin(void) const{
    return m_margin;
}
SRect ControlImpl::getMarginedRect(void) {
    Margin margin = getMargin();
    SRect marginRect = {
        margin.left,
        margin.top,
        getRect().width - margin.left - margin.right,
        getRect().height - margin.top - margin.bottom
    };
    return marginRect.normalize();
}

void ControlImpl::show(void){
    m_visible = true;
}

void ControlImpl::hide(void){
    m_visible = false;
}

void ControlImpl::setVisible(bool visible) {
    m_visible = visible;
}

bool ControlImpl::getVisible(void){
    return m_visible;
}

void ControlImpl::setEnable(bool enable){
    m_enable = enable;
    setState(enable ? ControlState::Normal : ControlState::Disabled);
}
bool ControlImpl::getEnable(void){
    return m_enable;
}

SDL_Renderer* ControlImpl::getRenderer(void){
    if (m_renderer != nullptr){
        return m_renderer;
    }
    if (m_parent != nullptr){
        m_renderer = m_parent->getRenderer();
    } else {
        m_renderer = GET_RENDERER;
        if (m_renderer == nullptr) {
            SDL_Log("ControlImpl::getRenderer: No renderer found!");
            return nullptr;
        }
    }
    return m_renderer;
}
shared_ptr<Control> ControlImpl::getThis(void){
    return shared_from_this();
}
void ControlImpl::setRenderer(SDL_Renderer* renderer){
    if (m_renderer == renderer) return;

    m_renderer = renderer;
    for (auto& child : m_children){
        child->setRenderer(renderer);
    }
}

RenderDevice* ControlImpl::getRenderDevice(void) {
    if (m_renderDevice != nullptr) {
        return m_renderDevice;
    }
    if (m_parent != nullptr) {
        m_renderDevice = m_parent->getRenderDevice();
    } else {
        m_renderDevice = GET_RENDERDEVICE;
        if (m_renderDevice == nullptr) {
            SDL_Log("ControlImpl::getRenderDevice: No render device found!");
            return nullptr;
        }
    }
    return m_renderDevice;
}

void ControlImpl::setRenderDevice(RenderDevice* device) {
    if (m_renderDevice == device) return;

    m_renderDevice = device;
    for (auto& child : m_children){
        child->setRenderDevice(device);
    }
}

SRect ControlImpl::getDrawRect(void){
    Control *parent = getParent();
    SRect parentDrawRect;
    if (parent != nullptr){
        parentDrawRect = parent->getDrawRect();
        return {m_rect.left * parent->getScaleXX() + parentDrawRect.left,
            m_rect.top * parent->getScaleYY() + parentDrawRect.top,
            m_rect.width * getScaleXX(),
            m_rect.height * getScaleYY()};
    }
    return {m_rect.left, m_rect.top, m_rect.width * getScaleXX(), m_rect.height * getScaleYY()};
}

SRect ControlImpl::mapToDrawRect(SRect rect){
    SRect drawRect = getDrawRect();
    return {rect.left * getScaleXX() + drawRect.left,
        rect.top * getScaleYY() + drawRect.top,
        rect.width * getScaleXX(),
        rect.height * getScaleYY()};
}
SPoint ControlImpl::mapToDrawPoint(SPoint point){
    SRect drawRect = getDrawRect();
    return {point.x * getScaleXX() + drawRect.left,
        point.y * getScaleYY() + drawRect.top};
}
bool ControlImpl::isContainsPoint(float x, float y){
    SRect drawRect = getDrawRect();
    return drawRect.contains(x, y);
}

void ControlImpl::onMouseEnter(float x, float y){
    // 默认不做任何处理，子类可重写此方法
}

void ControlImpl::onMouseLeave(float x, float y){
    // 默认不做任何处理，子类可重写此方法
}

void ControlImpl::setTransparent(bool isTransparent){
    m_isTransparent = isTransparent;
}

void ControlImpl::setState(ControlState state){
    m_state = state;
}

void ControlImpl::setBackgroundStateColor(StateColor stateColor){
    m_bgColor = stateColor;
}
void ControlImpl::setBorderStateColor(StateColor stateColor){
    m_borderColor = stateColor;
    setBorderVisible(true);
}
void ControlImpl::setTextStateColor(StateColor stateColor){
    m_textColor = stateColor;
}
void ControlImpl::setTextShadowStateColor(StateColor stateColor){
    m_textShadowColor = stateColor;
}
StateColor ControlImpl::getBackgroundStateColor(void){
    return m_bgColor;
}
StateColor ControlImpl::getBorderStateColor(void){
    return m_borderColor;
}
StateColor ControlImpl::getTextStateColor(void){
    return m_textColor;
}
StateColor ControlImpl::getTextShadowStateColor(void){
    return m_textShadowColor;
}

void ControlImpl::setNormalStateBGColor(SColor color){
    m_bgColor.setNormal(color);
}
void ControlImpl::setHoverStateBGColor(SColor color){
    m_bgColor.setHover(color);
}
void ControlImpl::setPressedStateBGColor(SColor color){
    m_bgColor.setPressed(color);
}
void ControlImpl::setDisabledStateBGColor(SColor color){
    m_bgColor.setDisabled(color);
}
void ControlImpl::setNormalStateBDColor(SColor color){
    m_borderColor.setNormal(color);
}
void ControlImpl::setHoverStateBDColor(SColor color){
    m_borderColor.setHover(color);
}
void ControlImpl::setPressedStateBDColor(SColor color){
    m_borderColor.setPressed(color);
}
void ControlImpl::setDisabledStateBDColor(SColor color){
    m_borderColor.setDisabled(color);
}
void ControlImpl::setTextNormalStateColor(SColor color){
    m_textColor.setNormal(color);
}
void ControlImpl::setTextHoverStateColor(SColor color){
    m_textColor.setHover(color);
}
void ControlImpl::setTextPressedStateColor(SColor color){
    m_textColor.setPressed(color);
}
void ControlImpl::setTextDisabledStateColor(SColor color){
    m_textColor.setDisabled(color);
}
void ControlImpl::setTextShadowNormalStateColor(SColor color){
    m_textShadowColor.setNormal(color);
}
void ControlImpl::setTextShadowHoverStateColor(SColor color){
    m_textShadowColor.setHover(color);
}
void ControlImpl::setTextShadowPressedStateColor(SColor color){
    m_textShadowColor.setPressed(color);
}
void ControlImpl::setTextShadowDisabledStateColor(SColor color){
    m_textShadowColor.setDisabled(color);
}

void ControlImpl::setBorderVisible(bool isVisible){
    m_isBorderVisible = isVisible;
}
bool ControlImpl::getBorderVisible(void){
    return m_isBorderVisible;
}

void ControlImpl::triggerEvent(shared_ptr<Event> event){
    m_eventQueueInstance->pushEventIntoQueue(event);
}

void ControlImpl::inheritRenderer(void) {
    if (m_renderer == nullptr){
        m_renderer = GET_RENDERER;
    }
    if (m_renderDevice == nullptr) {
        m_renderDevice = GET_RENDERDEVICE;
    }
}
