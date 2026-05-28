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
    shared_ptr<LuotiAni>m_luotiAni;

    string m_captionText;
    float m_captionSize;

    OnClickHandler m_onClick;
public:
    Button(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    void update(void) override;
    void draw(void) override;
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
    string getCaption(void) const;
    void setCaptionSize(float size);
    uint32_t getCaptionSize(float size) const;
    SRect getCaptionRect(void) const;

    void setLuotiAni(shared_ptr<LuotiAni>luotiAni);
    void setOnClick(OnClickHandler onClick);
    void setRenderer(SDL_Renderer* renderer) override;
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
