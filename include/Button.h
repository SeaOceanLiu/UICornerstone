#ifndef ButtonH
#define ButtonH
#include <functional>
#include "ConstDef.h"
#include "Actor.h"
#include "Label.h"
#include "LuotiAni.h"

// enum class ButtonState {
//     Normal,
//     Hover,
//     Pressed
// };

class Button: public ControlImpl {
    friend class ButtonBuilder;
public:
    using OnClickHandler = std::function<void (shared_ptr<Button>)>;
private:
    shared_ptr<Actor> m_actor;
    shared_ptr<Actor> m_hoverActor;
    shared_ptr<Actor> m_pressedActor;
    shared_ptr<Actor> m_disabledActor;

    shared_ptr<Label> m_caption;
    bool m_enableTextShadow;
    SPoint m_shadowOffset{1.0f, 1.0f};   // 宿主存储：caption 重建/替换后仍生效（#12）
    shared_ptr<LuotiAni>m_luotiAni;

    string m_captionText;
    float m_captionSize;

    OnClickHandler m_onClick;
public:
    Button(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    void update(void) override;
    void draw(void) override;
    void create(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    // 鼠标进入/退出处理
    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    void setNormalStateActor(shared_ptr<Actor> actor);
    void setHoverStateActor(shared_ptr<Actor> actor);
    void setPressedStateActor(shared_ptr<Actor> actor);
    void setDisabledStateActor(shared_ptr<Actor> actor);
    // 重载字体颜色设置相关函数，以同步调用Caption相关设置接口
    void setTextStateColor(StateColor stateColor) override;
    void setTextShadowStateColor(StateColor stateColor) override;
    void setTextShadowEnable(bool enable);

    void setCaption(string caption);
    void setCaptionLabel(shared_ptr<Label> label);
    shared_ptr<Label> getCaptionLabel(void) const;
    string getCaption(void) const;
    void setCaptionSize(float size);
    float getCaptionSize() const;
    SRect getCaptionRect(void) const;

    void setLuotiAni(shared_ptr<LuotiAni>luotiAni);
    void setOnClick(OnClickHandler onClick);
    void setRenderDevice(RenderDevice* device) override;
    void refreshScaleWith(float parentXX, float parentYY) override;

    // ── Property system overrides ──
    int setBoolProperty(const char* prop, int value) override;
    int setStringProperty(const char* prop, const char* value) override;
    int setPtrProperty(const char* prop, void* value) override;
    int getPtrProperty(const char* prop, void*& out) override;   // caption-label → 内部 caption Label 句柄
    // 状态联动：Button 状态变化时同步内部 caption Label（Label::draw 按自身 state 取色）
    void setState(ControlState state) override;
    int setIntProperty(const char* prop, int value) override;
    int getIntProperty(const char* prop, int& out) override;
    int setFloatProperty(const char* prop, float value) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override { return ControlImpl::setCallbackProperty(event, cb, userData); }
};

class ButtonBuilder {
private:
    shared_ptr<Button> m_button;
public:
    ButtonBuilder(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    ButtonBuilder& setNormalStateActor(shared_ptr<Actor> actor);
    ButtonBuilder& setHoverStateActor(shared_ptr<Actor> actor);
    ButtonBuilder& setPressedStateActor(shared_ptr<Actor> actor);
    ButtonBuilder& setDisabledStateActor(shared_ptr<Actor> actor);

    ButtonBuilder& setBackgroundStateColor(StateColor stateColor);
    ButtonBuilder& setBorderStateColor(StateColor stateColor);
    ButtonBuilder& setTextStateColor(StateColor stateColor);
    ButtonBuilder& setTextShadowStateColor(StateColor stateColor);

    ButtonBuilder& setCaption(string caption);
    ButtonBuilder& setCaptionSize(float size);
    ButtonBuilder& setLuotiAni(shared_ptr<LuotiAni> luotiAni);
    ButtonBuilder& addControl(shared_ptr<Control> child);
    ButtonBuilder& setOnClick(Button::OnClickHandler onClick);
    ButtonBuilder& setTransparent(bool isTransparent);
    ButtonBuilder& setId(int id);
    shared_ptr<Button> build(void);
};
#endif
