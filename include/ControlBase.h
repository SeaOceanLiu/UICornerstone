#ifndef ControlBaseH
#define ControlBaseH
#include <memory>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <string>
#include "SColor.h"
#include "ConstDef.h"
#include "UIContext.h"
#include "Utility.h"
#include "EventQueue.h"
#include "TextRenderer.h"
#include "InputBackend.h"
#include "ResourceProvider.h"
#include "FocusManager.h"
#include "RenderDevice.h"

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

// 控件类型枚举：子类构造函数设置 m_type，基类 getControlType() 返回。
// 与 JSON "type" 值一一对应（小写 kebab-case），需字符串时经枚举转常量。
enum class ControlType {
    None,
    Label, Button, EditBox, ComboBox, TextArea, CheckBox,
    ProgressBar, Slider, ScrollBar, Panel, WinFrame, ColorPicker,
    Splitter, TreeView, NumericUpDown, Popup, ConfirmPopup, Dialog,
    MenuItem, MenuPanel, MenuBar, Image, Animation, HandleControl, Shape,
    ListView, StatusBar, TabControl
};

class Control{
protected:
    // 事件队列
    EventQueue *m_eventQueueInstance = nullptr;
    // 所属实例上下文（多实例：每个控件指向自己的 UIContext）
    UIContext* m_context = nullptr;

    virtual void recreate() = 0; //重新创建控件，主要用于在一些属性改变时需要重新创建控件的情况，比如大小改变，位置改变等

public:
    explicit Control(UIContext* ctx = nullptr) : m_context(ctx) {}
    virtual ~Control() = default;

    UIContext* getContext() const { return m_context; }
    // 同步 m_eventQueueInstance：事件投递必须指向所属实例的队列。
    // 多实例两阶段创建：控件先以 null context 创建（字体等延迟加载），
    // 挂入控件树（addControl/setContext）后由 ControlImpl::setContext
    // 传播 context 并触发 recreate() 重建资源。
    virtual void setContext(UIContext* ctx) {
        m_context = ctx;
        m_eventQueueInstance = ctx ? ctx->eventQueue : nullptr;
    }

    // === Focus API ===
    virtual void setFocused(bool focused, bool byKeyboard = false) = 0;
    virtual bool getFocused() const = 0;
    virtual bool isMouseInside() const = 0;
    virtual bool isFocusable() const = 0;
    virtual int  getTabIndex() const = 0;
    virtual void setTabIndex(int index) = 0;
    virtual void setFocusable(bool focusable) = 0;
    virtual void onFocusGained(bool byKeyboard) = 0;
    virtual void onFocusLost() = 0;
    virtual void onFocusScopeActivated() = 0;  // 键盘切换（CTRL+Tab）将焦点 scope 激活时调用，如 WinFrame 提升到顶层
    virtual void setShowFocusRing(bool show) = 0;
    virtual void setFocusRingAlwaysVisible(bool always) = 0;
    virtual void setFocusRingColor(SColor color) = 0;
    virtual void setFocusRingStyle(FocusRingStyle style) = 0;
    virtual bool getShowFocusRing() const = 0;
    virtual bool getFocusRingAlwaysVisible() const = 0;
    virtual SColor getFocusRingColor() const = 0;
    virtual FocusRingStyle getFocusRingStyle() const = 0;
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
    // 父链缩放变更后整棵子树复合缩放刷新（child 视角，虚分派到被刷新的节点，
    // 非树成员子控件经覆写收口）。parentXX/YY = 父当前复合值。
    virtual void refreshScaleWith(float parentXX, float parentYY) {}
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
    virtual SPoint mapViewportToCanvas(SPoint point) = 0;
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

    // ── Property system (string-based, multi-type) ──
    virtual int setColorProperty(const char* prop, SColor color) { return 0; }
    virtual int setStateColorProperty(const char* prop, StateColor stateColor) { return 0; }
    virtual int setBoolProperty(const char* prop, int value) { return 0; }
    virtual int setIntProperty(const char* prop, int value) { return 0; }
    virtual int setFloatProperty(const char* prop, float value) { return 0; }
    virtual int setStringProperty(const char* prop, const char* value) { return 0; }
    virtual int setEnumProperty(const char* prop, const char* value) { return 0; }
    virtual int setPtrProperty(const char* prop, void* value) { return 0; }

    // ── Getter Property system ──
    virtual int getColorProperty(const char* prop, SColor& out) { return 0; }
    virtual int getStateColorProperty(const char* prop, StateColor& out) { return 0; }
    virtual int getBoolProperty(const char* prop, int& out) { return 0; }
    virtual int getIntProperty(const char* prop, int& out) { return 0; }
    virtual int getFloatProperty(const char* prop, float& out) { return 0; }
    virtual int getStringProperty(const char* prop, const char*& out) { return 0; }
    virtual int getEnumProperty(const char* prop, const char*& out) { return 0; }
    virtual int getPtrProperty(const char* prop, void*& out) { return 0; }

    // ── 控件类型（枚举，构造时由子类设置；需字符串时经枚举转常量）──
    ControlType getControlType() const { return m_ctlType; }

protected:
    ControlType m_ctlType = ControlType::None;

public:
    // ── Callback system ──
    // event: event name (e.g. "click", "value-changed"), not "on" prefixed
    virtual int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) { return 0; }
};

class ControlImpl: virtual public Control, public enable_shared_from_this<ControlImpl>{
protected:
    bool m_isCreated;
    int m_id;
    shared_ptr<class ContextMenu> m_contextMenu;   // 右键上下文菜单（决策点 2-B）
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
    // ── 字体上下文（JSON 字体声明 + 父链继承；LayoutParser 使用）──
    void setFontContext(FontName name, float size, bool isExplicit, bool nameExplicit) {
        m_fontContextName = name; m_fontContextSize = size;
        m_fontContextExplicit = isExplicit; m_fontContextNameExplicit = nameExplicit;
    }
    FontName getFontContextName() const { return m_fontContextName; }
    float getFontContextSize() const { return m_fontContextSize; }
    bool hasExplicitFont() const { return m_fontContextExplicit; }
    bool hasExplicitFontName() const { return m_fontContextNameExplicit; }
    FontName m_fontContextName = FontName::Asul_Regular;
    float m_fontContextSize = 0.0f;         // 0 = 未声明
    bool m_fontContextExplicit = false;
    bool m_fontContextNameExplicit = false; // font.name 是否显式声明（仅显式时覆盖子默认字体名）

    ControlImpl(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ControlImpl(const ControlImpl& other);
    ~ControlImpl();
    void create(void) override;  // 初始创建控件，缺省情况下只有初始创建控件后，才能显示和处理事件
    void setContext(UIContext* ctx) override;  // 传播 context 至子树并触发延迟重建
    bool isCreated(void) const { return m_isCreated; }

    // === Focus API ===
    void setFocused(bool focused, bool byKeyboard = false) override;
    bool getFocused() const override { return m_focused; }
    bool isMouseInside() const override { return m_mouseInside; }
    bool isFocusable() const override { return m_focusable; }
    int  getTabIndex() const override { return m_tabIndex; }
    void setTabIndex(int index) override;
    void setFocusable(bool focusable) override;
    void onFocusGained(bool byKeyboard) override;
    void onFocusLost() override;
    void onFocusScopeActivated() override;
    void setShowFocusRing(bool show) override { m_showFocusRing = show; }
    void setFocusRingAlwaysVisible(bool always) override { m_focusRingAlwaysVisible = always; }
    void setFocusRingColor(SColor color) override { m_focusRingColor = color; }
    void setFocusRingStyle(FocusRingStyle style) override { m_focusRingStyle = style; }
    bool getShowFocusRing() const override { return m_showFocusRing; }
    bool getFocusRingAlwaysVisible() const override { return m_focusRingAlwaysVisible; }
    SColor getFocusRingColor() const override { return m_focusRingColor; }
    FocusRingStyle getFocusRingStyle() const override { return m_focusRingStyle; }
    bool isFocusBoundary() const override { return m_isFocusBoundary; }
    void setFocusBoundary(bool boundary) override;
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
    void refreshScaleWith(float parentXX, float parentYY) override;
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
    void setContextMenu(shared_ptr<class ContextMenu> menu);
    shared_ptr<class ContextMenu> getContextMenu() const { return m_contextMenu; }
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
    SPoint mapViewportToCanvas(SPoint point) override;
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

    // ── Property system overrides ──
    int setColorProperty(const char* prop, SColor color) override;
    int setStateColorProperty(const char* prop, StateColor stateColor) override;
    int setBoolProperty(const char* prop, int value) override;
    int setIntProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setStringProperty(const char* prop, const char* value) override;
    int setEnumProperty(const char* prop, const char* value) override;
    int setPtrProperty(const char* prop, void* value) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;

    struct SelectionPayload { int idx; const char* val; };
    struct TreeNodePayload { const char* id; void* userData; };
    struct ColorPayload { uint8_t r,g,b,a; };
    struct GridPayload { int row; int col; int asc; };
    enum class CCallbackData { None, Int, Float, String, Selection, Ptr, TreeNode, Color, Grid };

    // 从子控件事件处理器中触发 C ABI 回调
    // eventName: PropertyNames 中的事件常量
    // data: 指向数据的指针（int*/float*/SelectionPayload*）
    void fireCCallback(const char* eventName, CCallbackData data, const void* ptr);

    int getColorProperty(const char* prop, SColor& out) override;
    int getStateColorProperty(const char* prop, StateColor& out) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getIntProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int getEnumProperty(const char* prop, const char*& out) override;
    int getPtrProperty(const char* prop, void*& out) override;

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

    // ── C ABI 回调存储 ──
    struct CCallbackEntry {
        void (*cb)(void*, const void*, void*);
        void* userData;
    };
    std::unordered_map<std::string, CCallbackEntry> m_cCallbacks;
};

/*主界面需要继承该类，以支持事件列队的处理入口eventLoopEntry*/
class TopControl: virtual public Control{
public:
    TopControl(UIContext* ctx = nullptr): Control(ctx){
        m_eventQueueInstance = ctx ? ctx->eventQueue : nullptr;
    }
    void eventLoopEntry(void){
        if (!m_eventQueueInstance) return;
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

// ============================================================
// 上下文宏：在 Control 派生类成员函数内展开（引用 m_context）。
// null 保护：控件在挂入控件树（addControl/setContext）之前 m_context
// 可能为 nullptr，宏返回 nullptr 而非解引用崩溃。
// 注意：不能命名为 CONTEXT——winnt.h 在 AMD64 下定义 #define CONTEXT
// CONTEXT_AMD64，同名宏会导致 Windows SDK 头文件解析崩溃。
// ============================================================
#define GET_CONTEXT (m_context)
#define BENCH (GET_CONTEXT ? (GET_CONTEXT)->bench : nullptr)
#define MAINWIN (GET_CONTEXT ? (GET_CONTEXT)->mainWindow : nullptr)
#define GET_RENDERDEVICE (GET_CONTEXT ? (GET_CONTEXT)->renderDevice : nullptr)
#define GET_TEXTRENDERER (GET_CONTEXT ? (GET_CONTEXT)->textRenderer : nullptr)
#define GET_INPUTBACKEND (GET_CONTEXT ? (GET_CONTEXT)->inputBackend : nullptr)
#define GET_RESOURCEPROVIDER (GET_CONTEXT ? (GET_CONTEXT)->resourceProvider : nullptr)
#define GET_FOCUSMANAGER (GET_CONTEXT ? (GET_CONTEXT)->focusManager : nullptr)
#endif  // ControlBaseH
