#include "ControlBase.h"
#include "PlatformUtils.h"
#include "MainWindow.h"
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
    m_renderDevice(nullptr),
    m_textRenderer(nullptr),
    m_inputBackend(nullptr),
    m_resourceProvider(nullptr),
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
    m_renderDevice = other.m_renderDevice;
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
        // 获取当前鼠标位置
        float mouseX = 0, mouseY = 0;
        if (MAINWIN && MAINWIN->getWindow()) {
            MAINWIN->getWindow()->getMousePosition(mouseX, mouseY);
        }

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
void ControlImpl::beforeDraw() {
    if (!getVisible()) return;

    m_frameDrawRect = getDrawRect();
    m_frameDrawRectValid = true;
    drawBackground(&m_frameDrawRect);
}

void ControlImpl::afterDraw() {
    if (!getVisible()) return;

    drawBorder(&m_frameDrawRect);
    m_frameDrawRectValid = false;
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
                break;
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
        // 提取事件坐标（仅对有位置的事件做遮挡检测）
        float mx = 0, my = 0;
        bool hasPos = false;
        if (event->m_type == EventType::MouseMove) {
            mx = event->mousePos.x; my = event->mousePos.y; hasPos = true;
        } else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
            mx = event->mouseButton.x; my = event->mouseButton.y; hasPos = true;
        } else if (event->m_type == EventType::MouseWheel) {
            mx = event->mouseWheel.x; my = event->mouseWheel.y; hasPos = true;
        } else if (event->m_type == EventType::FingerDown || event->m_type == EventType::FingerUp || event->m_type == EventType::FingerMotion) {
            mx = event->mousePos.x; my = event->mousePos.y; hasPos = true;
        }

        // 逆向遍历当前控件的所有子控件，保证后添加的控件先处理事件，因为后添加的控件在屏幕上位于上层
        // 使用副本遍历，防止子控件的 bringToFront 等操作修改 m_children 导致迭代器失效
        auto childrenCopy = m_children;
        for (auto it = childrenCopy.rbegin(); it != childrenCopy.rend(); ++it){
            // 对有位置的事件：检查当前子控件是否被更高层级的兄弟控件遮挡
            if (hasPos) {
                bool covered = false;
                for (auto coverIt = childrenCopy.rbegin(); coverIt != it; ++coverIt) {
                    if ((*coverIt)->getVisible() && (*coverIt)->getDrawRect().contains(mx, my)) {
                        covered = true;
                        break;
                    }
                }
                if (covered) continue;
            }
            if ((*it)->handleEvent(event)){
                return true;
            }
        }

    }
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
    child->setRenderDevice(getRenderDevice());
}

void ControlImpl::removeControl(shared_ptr<Control> child){
    m_children.erase(std::remove(m_children.begin(), m_children.end(), child), m_children.end());
}
// 只调用setParent的话，是不会添加到父控件的children中的，用于自行控制绘制逻辑和事件处理逻辑
// 如果要自动绘制和处理事件，需要调用addControl
//
// override 注意：
// - 可以加 if (m_parent == parent) return 阻止重复 setParent 触发的 recreate，
//   但仍需调用 ControlImpl::setParent(parent) 以确保 Renderer 继承等更新
//   （缩放传播由 updateChildScale() 处理，不走 setParent 路径）
// - 示例如 Label::setParent
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
        updateChildScale(child.get()); // 直接更新复合缩放，不触发 setParent 的开销和脏标记问题
    }
}
void ControlImpl::setScaleY(float yScale){
    m_yScale = yScale;
    for (auto& child : m_children){
        updateChildScale(child.get()); // 直接更新复合缩放，不触发 setParent 的开销和脏标记问题
    }
}

// 注：override 时建议在最前面加 if (m_rect == rect) return;
// 以防止不必要的 recreate() cascade（参见 CheckBox::setRect, Label::setRect）
void ControlImpl::setRect(SRect rect){
    if (m_rect == rect) return;
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

shared_ptr<Control> ControlImpl::getThis(void){
    return shared_from_this();
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
            Platform::Log("ControlImpl::getRenderDevice: No render device found!");
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

TextRenderer* ControlImpl::getTextRenderer(void) {
    if (m_textRenderer != nullptr) {
        return m_textRenderer;
    }
    if (m_parent != nullptr) {
        m_textRenderer = m_parent->getTextRenderer();
    } else {
        m_textRenderer = GET_RENDERDEVICE ? MAINWIN->getTextRenderer() : nullptr;
        if (m_textRenderer == nullptr) {
            Platform::Log("ControlImpl::getTextRenderer: No text renderer found!");
            return nullptr;
        }
    }
    return m_textRenderer;
}

void ControlImpl::setTextRenderer(TextRenderer* renderer) {
    if (m_textRenderer == renderer) return;

    m_textRenderer = renderer;
    for (auto& child : m_children){
        child->setTextRenderer(renderer);
    }
}

InputBackend* ControlImpl::getInputBackend(void) {
    if (m_inputBackend != nullptr) {
        return m_inputBackend;
    }
    if (m_parent != nullptr) {
        m_inputBackend = m_parent->getInputBackend();
    } else {
        m_inputBackend = GET_RENDERDEVICE ? MAINWIN->getInputBackend() : nullptr;
        if (m_inputBackend == nullptr) {
            Platform::Log("ControlImpl::getInputBackend: No input backend found!");
            return nullptr;
        }
    }
    return m_inputBackend;
}

void ControlImpl::setInputBackend(InputBackend* backend) {
    if (m_inputBackend == backend) return;

    m_inputBackend = backend;
    for (auto& child : m_children){
        child->setInputBackend(backend);
    }
}

ResourceProvider* ControlImpl::getResourceProvider(void) {
    if (m_resourceProvider != nullptr) {
        return m_resourceProvider;
    }
    if (m_parent != nullptr) {
        m_resourceProvider = m_parent->getResourceProvider();
    } else {
        m_resourceProvider = MAINWIN->getResourceProvider();
    }
    return m_resourceProvider;
}

void ControlImpl::setResourceProvider(ResourceProvider* provider) {
    if (m_resourceProvider == provider) return;

    m_resourceProvider = provider;
    for (auto& child : m_children){
        child->setResourceProvider(provider);
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
    SRect drawRect = m_frameDrawRectValid ? m_frameDrawRect : getDrawRect();
    return {rect.left * getScaleXX() + drawRect.left,
        rect.top * getScaleYY() + drawRect.top,
        rect.width * getScaleXX(),
        rect.height * getScaleYY()};
}
SPoint ControlImpl::mapToDrawPoint(SPoint point){
    SRect drawRect = m_frameDrawRectValid ? m_frameDrawRect : getDrawRect();
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
    if (m_renderDevice == nullptr) {
        m_renderDevice = GET_RENDERDEVICE;
    }
    if (m_textRenderer == nullptr) {
        m_textRenderer = MAINWIN->getTextRenderer();
    }
    if (m_inputBackend == nullptr) {
        m_inputBackend = MAINWIN->getInputBackend();
    }
    if (m_resourceProvider == nullptr) {
        m_resourceProvider = MAINWIN->getResourceProvider();
    }
}
