#ifndef ControlBaseH
#define ControlBaseH
#include <memory>
#include <vector>
#include <typeinfo>
#include "SColor.h"
#include "MainWindow.h"
#include "Utility.h"
#include "EventQueue.h"
#include "TextRenderer.h"
#include "InputBackend.h"
#include "ResourceProvider.h"
#include "FocusManager.h"

using namespace std;

enum class ControlState {
    Disabled,
    Normal,
    Hover,
    Pressed
};
class StateColor{
protected:
    SColor normal;
    SColor hover;
    SColor pressed;
    SColor disabled;
public:
    enum class Type{
        Background,
        Border,
        Text,
        TextShadow
    };
    StateColor(SColor n, SColor h, SColor a, SColor d):normal(n), hover(h), pressed(a), disabled(d){}
    StateColor(StateColor::Type colorType=StateColor::Type::Background):
        normal(colorType == StateColor::Type::Background ? ConstDef::DEFAULT_NORMAL_COLOR :
                colorType == StateColor::Type::Border ? ConstDef::DEFAULT_BORDER_NORMAL_COLOR :
                colorType == StateColor::Type::Text ? ConstDef::DEFAULT_TEXT_NORMAL_COLOR :
                    ConstDef::DEFAULT_TEXT_SHADOW_NORMAL_COLOR),
        hover(colorType == StateColor::Type::Background ? ConstDef::DEFAULT_HOVER_COLOR :
                colorType == StateColor::Type::Border ? ConstDef::DEFAULT_BORDER_HOVER_COLOR :
                colorType == StateColor::Type::Text ? ConstDef::DEFAULT_TEXT_HOVER_COLOR :
                    ConstDef::DEFAULT_TEXT_SHADOW_HOVER_COLOR),
        pressed(colorType == StateColor::Type::Background ? ConstDef::DEFAULT_DOWN_COLOR :
                colorType == StateColor::Type::Border ? ConstDef::DEFAULT_BORDER_DOWN_COLOR :
                colorType == StateColor::Type::Text ? ConstDef::DEFAULT_TEXT_DOWN_COLOR :
                    ConstDef::DEFAULT_TEXT_SHADOW_DOWN_COLOR),
        disabled(colorType == StateColor::Type::Background ? ConstDef::DEFAULT_DISABLED_COLOR :
                colorType == StateColor::Type::Border ? ConstDef::DEFAULT_BORDER_DISABLED_COLOR :
                colorType == StateColor::Type::Text ? ConstDef::DEFAULT_TEXT_DISABLED_COLOR :
                    ConstDef::DEFAULT_TEXT_SHADOW_DISABLED_COLOR)
    {}
    // 赋值构造函数
    StateColor(const StateColor& p):normal(p.normal), hover(p.hover), pressed(p.pressed), disabled(p.disabled){}
    StateColor(const StateColor&& p):normal(p.normal), hover(p.hover), pressed(p.pressed), disabled(p.disabled){}
    StateColor& operator=(const StateColor& p){
        normal = p.normal;
        hover = p.hover;
        pressed = p.pressed;
        disabled = p.disabled;
        return *this;
    }
    StateColor& operator=(const StateColor&& p){
        normal = p.normal;
        hover = p.hover;
        pressed = p.pressed;
        disabled = p.disabled;
        return *this;
    }

    StateColor& setNormal(SColor color){
        normal = color;
        return *this;
    }
    SColor getNormal(void) const {
        return normal;
    }
    StateColor& setHover(SColor color){
        hover = color;
        return *this;
    }
    SColor getHover(void){
        return hover;
    }
    StateColor& setPressed(SColor color){
        pressed = color;
        return *this;
    }
    SColor getPressed(void){
        return pressed;
    }
    StateColor& setDisabled(SColor color){
        disabled = color;
        return *this;
    }
    SColor getDisabled(void){
        return disabled;
    }
    StateColor& set(StateColor::Type type, SColor color){
        switch (type)
        {
        case StateColor::Type::Background:
            normal = color;
            break;
        case StateColor::Type::Border:
            normal = color;
            break;
        case StateColor::Type::Text:
            normal = color;
            break;
        default:
            break;
        }
        return *this;
    }
    SColor get(StateColor::Type type){
        switch (type)
        {
        case StateColor::Type::Background:
            return normal;
        case StateColor::Type::Border:
            return normal;
        case StateColor::Type::Text:
            return normal;
        default:
            return normal;
        }
    }
};

class Control{
protected:
    // 事件队列
    EventQueue *m_eventQueueInstance;

    virtual void recreate() = 0; //重新创建控件，主要用于在一些属性改变时需要重新创建控件的情况，比如大小改变，位置改变等

public:
    virtual ~Control() = default;

    // === Focus API ===
    virtual void setFocused(bool focused, bool byKeyboard = false) = 0;
    virtual bool getFocused() const = 0;
    virtual bool isFocusable() const = 0;
    virtual int  getTabIndex() const = 0;
    virtual void setTabIndex(int index) = 0;
    virtual void setFocusable(bool focusable) = 0;
    virtual void onFocusGained(bool byKeyboard) = 0;
    virtual void onFocusLost() = 0;
    virtual void setShowFocusRing(bool show) = 0;
    virtual void setFocusRingAlwaysVisible(bool always) = 0;
    virtual void setFocusRingColor(SColor color) = 0;
    virtual void setFocusRingStyle(FocusRingStyle style) = 0;
    virtual bool isFocusBoundary() const = 0;
    virtual void setFocusBoundary(bool boundary) = 0;

    virtual void create(void) = 0;  // 初始创建控件，缺省情况下只有初始创建控件后，才能显示和处理事件
    virtual void setId(int id) = 0;
    virtual int getId(void) const = 0;
    virtual void update(void) = 0;
    virtual void beforeDraw() = 0;
    virtual void afterDraw() = 0;
    virtual void draw(void) = 0;
    virtual void resized(SRect newRect) = 0;
    virtual void moved(SRect newRect) = 0;
    virtual bool handleEvent(shared_ptr<Event> event) = 0;  //事件处理，返回值表示是否处理了该事件，true表示处理了，false表示未处理
    virtual bool beforeEventHandlingWatcher(shared_ptr<Event> event) = 0;  //事件通知，返回值表示是否吃掉该事件，true表示吃掉，即不再传递给后续控件，false表示未吃掉
    virtual bool afterEventHandlingWatcher(shared_ptr<Event> event) = 0;  //事件通知，返回值表示是否吃掉该事件，true表示吃掉，即不再传递给后续控件，false表示未吃掉
    // virtual shared_ptr<Control> addControl(shared_ptr<Control> child) = 0;
    virtual void addControl(shared_ptr<Control> child) = 0;
    virtual void removeControl(shared_ptr<Control> child) = 0;
    virtual void setParent(Control *parent) = 0;
    virtual Control* getParent(void) = 0;
    virtual void setRect(SRect rect) = 0;
    virtual SRect getRect(void) = 0;
    virtual float getScaleXX(void) = 0;
    virtual float getScaleYY(void) = 0;
    virtual void setScaleX(float xScale=1.0f) = 0;
    virtual void setScaleY(float yScale=1.0f) = 0;
    virtual void show(void) = 0;
    virtual void hide(void) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual bool getVisible(void) = 0;
    virtual void setEnable(bool enable) = 0;
    virtual bool getEnable(void) = 0;
    virtual RenderDevice* getRenderDevice(void) = 0;
    virtual void setRenderDevice(RenderDevice* device) = 0;
    virtual TextRenderer* getTextRenderer(void) = 0;
    virtual void setTextRenderer(TextRenderer* renderer) = 0;
    virtual InputBackend* getInputBackend(void) = 0;
    virtual void setInputBackend(InputBackend* backend) = 0;
    virtual ResourceProvider* getResourceProvider(void) = 0;
    virtual void setResourceProvider(ResourceProvider* provider) = 0;
    virtual shared_ptr<Control> getThis(void) = 0;
    virtual SRect getDrawRect(void) = 0;
    virtual SRect mapToDrawRect(SRect rect) = 0;
    virtual SPoint mapToDrawPoint(SPoint point) = 0;
    virtual bool isContainsPoint(float x, float y) = 0; //判断点是否在控件内

    // 鼠标进入/退出回调函数
    virtual void onMouseEnter(float x, float y) = 0;
    virtual void onMouseLeave(float x, float y) = 0;

    virtual void setTransparent(bool isTransparent) = 0; //设置控件背景是否透明
    virtual bool getTransparent(void) = 0;
    virtual void setState(ControlState state) = 0;  //设置控件相关状态，需要控件自行处理状态变化
    virtual ControlState getState(void) = 0;  //获取控件相关状态
    // 状态相关设置接口
    virtual void setBackgroundStateColor(StateColor stateColor) = 0;
    virtual void setBorderStateColor(StateColor stateColor) = 0;
    virtual void setTextStateColor(StateColor stateColor) = 0;
    virtual void setTextShadowStateColor(StateColor stateColor) = 0;
    virtual StateColor getBackgroundStateColor(void) = 0;
    virtual StateColor getBorderStateColor(void) = 0;
    virtual StateColor getTextStateColor(void) = 0;
    virtual StateColor getTextShadowStateColor(void) = 0;

    virtual void setNormalStateBGColor(SColor color) = 0;
    virtual void setHoverStateBGColor(SColor color) = 0;
    virtual void setPressedStateBGColor(SColor color) = 0;
    virtual void setDisabledStateBGColor(SColor color) = 0;
    virtual void setNormalStateBDColor(SColor color) = 0;
    virtual void setHoverStateBDColor(SColor color) = 0;
    virtual void setPressedStateBDColor(SColor color) = 0;
    virtual void setDisabledStateBDColor(SColor color) = 0;
    virtual void setTextNormalStateColor(SColor color) = 0;
    virtual void setTextHoverStateColor(SColor color) = 0;
    virtual void setTextPressedStateColor(SColor color) = 0;
    virtual void setTextDisabledStateColor(SColor color) = 0;
    virtual void setTextShadowNormalStateColor(SColor color) = 0;
    virtual void setTextShadowHoverStateColor(SColor color) = 0;
    virtual void setTextShadowPressedStateColor(SColor color) = 0;
    virtual void setTextShadowDisabledStateColor(SColor color) = 0;

    virtual SColor getBGColor(void) = 0;
    virtual SColor getBorderColor(void) = 0;
    virtual void setBorderVisible(bool isVisible) = 0;
    virtual bool getBorderVisible(void) = 0;
    virtual Margin getMargin(void) const = 0;
};

class ControlImpl: virtual public Control, public enable_shared_from_this<ControlImpl>{
protected:
    bool m_isCreated;
    int m_id;
    bool m_visible;
    bool m_enable;
    float m_xScale;
    float m_yScale;
    float m_xxScale;
    float m_yyScale;

    StateColor m_bgColor; //背景颜色
    StateColor m_borderColor; //边框颜色
    StateColor m_textColor; //文字颜色
    StateColor m_textShadowColor; //文字阴影颜色

    SharedSurface m_surface;
    RenderDevice *m_renderDevice;
    TextRenderer *m_textRenderer;
    InputBackend *m_inputBackend;
    ResourceProvider *m_resourceProvider;
    SharedTexture m_texture;

    SRect m_rect;
    Margin m_margin;
    Control *m_parent;
    vector<shared_ptr<Control>> m_children; //子控件

    bool m_isTransparent;
    bool m_isBorderVisible;
    bool m_alwaysOnTop = false;
    ControlState m_state;
    // 鼠标进入/退出状态跟踪
    bool m_mouseInside;

    SRect m_frameDrawRect;
    bool m_frameDrawRectValid = false;

    // === Focus state ===
    bool m_focused = false;
    bool m_focusable = false;
    int  m_tabIndex = -1;
    bool m_focusByKeyboard = false;

    // === Focus ring config ===
    bool m_showFocusRing = true;
    bool m_focusRingAlwaysVisible = true;
    SColor m_focusRingColor{66, 133, 244, 255};
    FocusRingStyle m_focusRingStyle = FocusRingStyle::Solid;

    // === Focus scope ===
    bool m_isFocusBoundary = false;

    void recreate(void) override; //重新创建控件，主要用于在一些属性改变时需要重新创建控件的情况，比如大小改变，位置改变等
    // 保持 alwaysOnTop 子控件在 children 末尾
    void stabilizeTopmostChildren();
    // 直接更新子控件的复合缩放值，避免通过 setParent(this) 传播缩放（setParent 会触发
    // inheritRenderer 等不必要开销，且与 Label::setParent 的脏-父检查冲突）
    void updateChildScale(Control* child) const {
        auto* impl = dynamic_cast<ControlImpl*>(child);
        if (impl) {
            impl->m_xxScale = impl->m_xScale * m_xxScale;
            impl->m_yyScale = impl->m_yScale * m_yyScale;
        }
    }
public:
    ControlImpl(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ControlImpl(const ControlImpl& other);
    ~ControlImpl();
    void create(void) override;  // 初始创建控件，缺省情况下只有初始创建控件后，才能显示和处理事件

    // === Focus API ===
    void setFocused(bool focused, bool byKeyboard = false) override;
    bool getFocused() const override { return m_focused; }
    bool isFocusable() const override { return m_focusable; }
    int  getTabIndex() const override { return m_tabIndex; }
    void setTabIndex(int index) override;
    void setFocusable(bool focusable) override;
    void onFocusGained(bool byKeyboard) override;
    void onFocusLost() override;
    void setShowFocusRing(bool show) override { m_showFocusRing = show; }
    void setFocusRingAlwaysVisible(bool always) override { m_focusRingAlwaysVisible = always; }
    void setFocusRingColor(SColor color) override { m_focusRingColor = color; }
    void setFocusRingStyle(FocusRingStyle style) override { m_focusRingStyle = style; }
    bool isFocusBoundary() const override { return m_isFocusBoundary; }
    void setFocusBoundary(bool boundary) override { m_isFocusBoundary = boundary; }
    void drawFocusRing();

    void setId(int id) override { m_id = id; }
    int getId(void) const override { return m_id; }
    ControlImpl& operator=(const ControlImpl& other);
    void update(void) override;
    void beforeDraw() override;
    void afterDraw() override;
    void draw(void) override;
    void resized(SRect newRect) override;
    void moved(SRect newRect) override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    bool afterEventHandlingWatcher(shared_ptr<Event> event) override;
    void addControl(shared_ptr<Control> child) override;
    void removeControl(shared_ptr<Control> child) override;
    void setParent(Control *parent) override;
    Control* getParent(void) override;
    float getScaleXX(void) override;
    float getScaleYY(void) override;
    void setScaleX(float xScale=1.0f) override;
    void setScaleY(float yScale=1.0f) override;
    void setRect(SRect rect) override;
    vector<shared_ptr<Control>>& getChildren() { return m_children; }
    void setLeft(float left){
        setRect(SRect{left, getRect().top, getRect().width, getRect().height});
    }
    void setTop(float top){
        setRect(SRect{getRect().left, top, getRect().width, getRect().height});
    }
    void setWidth(float width){
        setRect(SRect{getRect().left, getRect().top, width, getRect().height});
    }
    void setHeight(float height){
        setRect(SRect{getRect().left, getRect().top, getRect().width, height});
    }
    void moveTo(float left, float top){
        setRect(SRect{left, top, getRect().width, getRect().height});
    }
    void resizeTo(float width, float height){
        setRect(SRect{getRect().left, getRect().top, width, height});
    }
    SRect getRect(void) override;
    SRect getMarginedRect(void);
    virtual void setMargin(Margin margin);
    Margin getMargin(void) const override;
    void show(void) override;
    void hide(void) override;
    void setVisible(bool visible) override;
    bool getVisible(void) override;
    void setEnable(bool enable) override;
    bool getEnable(void) override;
    RenderDevice* getRenderDevice(void) override;
    void setRenderDevice(RenderDevice* device) override;
    TextRenderer* getTextRenderer(void) override;
    void setTextRenderer(TextRenderer* renderer) override;
    InputBackend* getInputBackend(void) override;
    void setInputBackend(InputBackend* backend) override;
    ResourceProvider* getResourceProvider(void) override;
    void setResourceProvider(ResourceProvider* provider) override;
    shared_ptr<Control> getThis(void) override;
    SRect getDrawRect(void) override;
    SRect mapToDrawRect(SRect rect) override;
    SPoint mapToDrawPoint(SPoint point) override;
    bool isContainsPoint(float x, float y) override;
    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;
    void setTransparent(bool isTransparent) override;
    bool getTransparent(void) override { return m_isTransparent; }
    void setState(ControlState state) override;

    // 始终置顶标志——拥有此标志的子控件将自动保持在 children 末尾（最后绘制、最先接收事件）
    void setAlwaysOnTop(bool on) { m_alwaysOnTop = on; }
    bool isAlwaysOnTop() const { return m_alwaysOnTop; }
    ControlState getState(void) override { return m_state; }

    // 状态相关设置接口
    void setBackgroundStateColor(StateColor stateColor) override;
    void setBorderStateColor(StateColor stateColor) override;
    void setTextStateColor(StateColor stateColor) override;
    void setTextShadowStateColor(StateColor stateColor) override;
    StateColor getBackgroundStateColor(void) override;
    StateColor getBorderStateColor(void) override;
    StateColor getTextStateColor(void) override;
    StateColor getTextShadowStateColor(void) override;

    void setNormalStateBGColor(SColor color) override;
    void setHoverStateBGColor(SColor color) override;
    void setPressedStateBGColor(SColor color) override;
    void setDisabledStateBGColor(SColor color) override;
    void setNormalStateBDColor(SColor color) override;
    void setHoverStateBDColor(SColor color) override;
    void setPressedStateBDColor(SColor color) override;
    void setDisabledStateBDColor(SColor color) override;
    void setTextNormalStateColor(SColor color) override;
    void setTextHoverStateColor(SColor color) override;
    void setTextPressedStateColor(SColor color) override;
    void setTextDisabledStateColor(SColor color) override;
    void setTextShadowNormalStateColor(SColor color) override;
    void setTextShadowHoverStateColor(SColor color) override;
    void setTextShadowPressedStateColor(SColor color) override;
    void setTextShadowDisabledStateColor(SColor color) override;

    // 根据控件状态绘制背景
    void drawBackground(const SRect *pDrawRect);
    // 根据控件状态绘制边框
    void drawBorder(const SRect *pDrawRect);

    SColor getBGColor(void) override { return m_bgColor.getNormal(); }
    SColor getBorderColor(void) override { return m_borderColor.getNormal(); }
    void setBorderVisible(bool isVisible) override;
    bool getBorderVisible(void) override;

    void triggerEvent(shared_ptr<Event> event);
    void inheritRenderer(void);
};

/*主界面需要继承该类，以支持事件列队的处理入口eventLoopEntry*/
class TopControl: virtual public Control{
public:
    TopControl(void){m_eventQueueInstance = EventQueue::getInstance();}
    void eventLoopEntry(void){
        int evCount = 0;
        shared_ptr<Event> eventInQueue = m_eventQueueInstance->popEventFromQueue();
        while(eventInQueue != nullptr){
            evCount++;
            bool consumed = m_eventQueueInstance->notifyBeforeEventHandlingWatchers(eventInQueue);
            if (!consumed) {
                handleEvent(eventInQueue);
            }
            m_eventQueueInstance->notifyAfterEventHandlingWatchers(eventInQueue);

            eventInQueue = m_eventQueueInstance->popEventFromQueue();
        }
    }
};
#endif  // ControlBaseH
