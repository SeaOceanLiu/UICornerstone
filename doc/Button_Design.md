# Button 按钮设计文档

## 1. 概述

Button（按钮）是一种常见的 UI 控件，用于触发动作或事件。支持多种状态（普通、悬停、按下、禁用），可通过 Actor 自定义外观，支持标题文本和动画效果。

## 2. 设计规则

> 参见 [AGENTS.md](./AGENTS.md) 中的设计规则章节。

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 绘制时才对缩放进行处理

2. **控件生命周期**：
   - `create()`: 初始创建控件
   - `recreate()`: 重新创建控件（当属性变化时）

## 3. 功能规格

### 3.1 核心功能

- **多状态外观**：支持 Normal、Hover、Pressed、Disabled 四种状态，每种状态可设置独立的 Actor
- **标题文本**：内部使用 Label 显示文本，支持文本颜色和阴影
- **点击事件**：支持点击回调函数
- **洛蒂动画支持**：支持洛蒂动画（LuotiAni）
- **子控件支持**：可添加子控件

### 3.2 状态定义

Button 继承自 `ControlState` 枚举：

```cpp
enum class ControlState {
    Normal,     // 普通状态
    Hover,      // 鼠标悬停
    Pressed,    // 按下状态
    Disabled    // 禁用状态
};
```

## 4. 接口设计

### 4.1 Button 类

```cpp
class Button: public ControlImpl {
    friend class ButtonBuilder;
public:
    using OnClickHandler = std::function<void (shared_ptr<Button>)>;

private:
    shared_ptr<Actor> m_actor;          // 普通状态外观
    shared_ptr<Actor> m_hoverActor;     // 悬停状态外观
    shared_ptr<Actor> m_pressedActor;   // 按下状态外观
    shared_ptr<Actor> m_disabledActor;  // 禁用状态外观

    shared_ptr<Label> m_caption;         // 标题文本（内部Label）
    bool m_enableTextShadow;
    shared_ptr<LuotiAni> m_luotiAni;    // 洛蒂动画

    string m_captionText;              // 标题文本内容
    float m_captionSize;                // 标题字体大小

    OnClickHandler m_onClick;           // 点击回调

public:
    Button(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    // 鼠标进入/退出处理
    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    // 状态外观设置
    void setNormalStateActor(shared_ptr<Actor> actor);
    void setHoverStateActor(shared_ptr<Actor> actor);
    void setPressedStateActor(shared_ptr<Actor> actor);
    void setDisabledStateActor(shared_ptr<Actor> actor);

    // 重载字体颜色设置相关函数，以同步调用Caption相关设置接口
    void setTextStateColor(StateColor stateColor) override;
    void setTextShadowStateColor(StateColor stateColor) override;
    void setTextShadowEnable(bool enable);

    // 标题相关
    void setCaption(string caption);
    string getCaption(void) const;
    void setCaptionSize(float size);
    uint32_t getCaptionSize(float size) const;
    SRect getCaptionRect(void) const;

    // 动画和回调
    void setLuotiAni(shared_ptr<LuotiAni> luotiAni);
    void setOnClick(OnClickHandler onClick);
};
```

### 4.2 ButtonBuilder 类

```cpp
class ButtonBuilder {
private:
    shared_ptr<Button> m_button;

public:
    ButtonBuilder(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

    // 状态外观
    ButtonBuilder& setNormalStateActor(shared_ptr<Actor> actor);
    ButtonBuilder& setHoverStateActor(shared_ptr<Actor> actor);
    ButtonBuilder& setPressedStateActor(shared_ptr<Actor> actor);
    ButtonBuilder& setDisabledStateActor(shared_ptr<Actor> actor);

    // 颜色设置
    ButtonBuilder& setBackgroundStateColor(StateColor stateColor);
    ButtonBuilder& setBorderStateColor(StateColor stateColor);
    ButtonBuilder& setTextStateColor(StateColor stateColor);
    ButtonBuilder& setTextShadowStateColor(StateColor stateColor);

    // 标题和动画
    ButtonBuilder& setCaption(string caption);
    ButtonBuilder& setCaptionSize(float size);
    ButtonBuilder& setLuotiAni(shared_ptr<LuotiAni> luotiAni);

    // 子控件和回调
    ButtonBuilder& addControl(shared_ptr<Control> child);
    ButtonBuilder& setOnClick(Button::OnClickHandler onClick);
    ButtonBuilder& setTransparent(bool isTransparent);
    ButtonBuilder& setId(int id);

    shared_ptr<Button> build(void);
};
```

## 5. 实现细节

### 5.1 构造函数

```cpp
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
```

### 5.2 绘制流程

```cpp
void Button::draw(void){
    ControlImpl::preDraw();

    if (!getVisible()) return;

    // 1. 根据状态选择对应的Actor
    auto actor = m_actor;
    switch(getState()){
        case ControlState::Disabled:
            if (m_disabledActor != nullptr) actor = m_disabledActor;
            break;
        case ControlState::Hover:
            if (m_hoverActor != nullptr) actor = m_hoverActor;
            break;
        case ControlState::Pressed:
            if (m_pressedActor != nullptr) actor = m_pressedActor;
            break;
        default:
            break;
    }

    // 2. 绘制当前状态的Actor
    if (actor != nullptr){
        actor->draw();
    }

    // 3. 绘制动画
    if (m_luotiAni != nullptr){
        m_luotiAni->draw();
    }

    // 4. 绘制标题
    if (m_caption != nullptr){
        m_caption->draw();
    }

    // 5. 绘制子控件
    ControlImpl::draw();
}
```

### 5.3 事件处理

```cpp
bool Button::handleEvent(shared_ptr<Event> event){
    if (!getEnable() || !getVisible()) return false;

    if (EventQueue::isPositionEvent(event->m_eventName)){
        if (!event->m_eventParam.has_value()) return false;

        try {
            shared_ptr<SPoint> pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;

            SRect drawRect = getDrawRect();
            if (drawRect.contains(pos->x, pos->y)){
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
                        if (m_onClick != nullptr && m_state == ControlState::Pressed){
                            m_onClick(dynamic_pointer_cast<Button>(this->getThis()));
                        }
                    case EventName::FINGER_UP:
                        setState(ControlState::Normal);
                        return true;

                    case EventName::MOUSE_MOVING:
                        setState(ControlState::Hover);
                        return true;

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
```

### 5.4 标题设置

```cpp
void Button::setCaption(string caption){
    m_captionText = caption;

    if (m_caption != nullptr){
        removeControl(m_caption);
        m_caption.reset();
    }

    if (m_captionText.length() > 0) {
        // 创建内部Label
        m_caption = LabelBuilder(this, {0, 0, m_rect.width, m_rect.height})
                            .setFont(FontName::HarmonyOS_Sans_SC_Regular)
                            .setAlignmentMode(AlignmentMode::AM_CENTER)
                            .setFontSize((int)m_captionSize)
                            .setCaption(m_captionText)
                            .setTextStateColor(m_textColor)
                            .setTextShadowStateColor(m_textShadowColor)
                            .setShadow(m_enableTextShadow)
                            .build();
        addControl(m_caption);
    }
}
```

### 5.5 状态Actor设置

```cpp
void Button::setNormalStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;

    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    m_actor = actor;
}
```

### 5.6 文本颜色同步

Button 重写了 `setTextStateColor` 和 `setTextShadowStateColor`，将颜色设置同步到内部 Label：

```cpp
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
```

## 6. 常量定义

在 `ConstDef.h` 中定义：

```cpp
// 按钮相关常量
static const float BUTTON_CAPTION_SIZE;          // 标题默认字体大小
static const SDL_Color BUTTON_NORMAL_COLOR;       // 普通状态背景色
static const SDL_Color BUTTON_HOVER_COLOR;       // 悬停状态背景色
static const SDL_Color BUTTON_DOWN_COLOR;        // 按下状态背景色
static const SDL_Color BUTTON_NORMAL_TEXT_COLOR;  // 普通状态文字色
static const SDL_Color BUTTON_HOVER_TEXT_COLOR;  // 悬停状态文字色
static const SDL_Color BUTTON_DOWN_TEXT_COLOR;    // 按下状态文字色
```

## 7. 与其他控件的关系

### 7.1 内部Label

Button 内部包含一个 `Label` 控件，用于显示标题文本。Label 的属性（字体、颜色、对齐等）由 Button 统一管理。

### 7.2 Actor

每个状态可以设置独立的 `Actor`，用于自定义按钮的外观。Actor 需要手动设置父控件为当前 Button。

### 7.3 LuotiAni

支持洛蒂动画，用于按钮的动态效果。

## 8. 使用示例

```cpp
// 基本按钮
auto button1 = ButtonBuilder(nullptr, SRect(50, 50, 120, 40))
    .setCaption("Click Me")
    .setOnClick([](shared_ptr<Button> btn) {
        cout << "Button clicked!" << endl;
    })
    .build();
BENCH->addControl(button1);

// 自定义状态的按钮
auto button2 = ButtonBuilder(nullptr, SRect(50, 100, 120, 40))
    .setCaption("Custom Button")
    .setBackgroundStateColor(StateColor::Type::Background)
    .setNormalStateColor({100, 100, 100, 255})
    .setHoverStateColor({150, 150, 150, 255})
    .setPressedStateColor({80, 80, 80, 255})
    .build();
BENCH->addControl(button2);

// 带动画的按钮
auto button3 = ButtonBuilder(nullptr, SRect(50, 150, 120, 40))
    .setCaption("Animated")
    .setLuotiAni(animation)
    .build();
BENCH->addControl(button3);
```

## 9. 文件结构

```
UIControls/
├── include/
│   └── Button.h        # 头文件
├── src/
│   └── Button.cpp      # 实现文件
└── CMakeLists.txt

test/
└── (暂无独立测试文件)

doc/
└── Button_Design.md   # 本文档
```