#ifndef EventQueueH
#define EventQueueH

#include <queue>
#include <mutex>
#include "StateMachine.h"

using namespace std;

class Control;

enum class EventName: int{
    None,

    FINGER_DOWN,
    FINGER_UP,
    FINGER_MOTION,

    MOUSE_LBUTTON_DOWN,
    MOUSE_LBUTTON_UP,
    MOUSE_RBUTTON_DOWN,
    MOUSE_RBUTTON_UP,
    MOUSE_MBUTTON_DOWN,
    MOUSE_MBUTTON_UP,
    MOUSE_MOVING,
    MOUSE_WHEEL,

    TEXT_INPUT,
    TEXT_EDITING,
    TEXT_EDITING_CANDIDATES,

    KEY_DOWN,
    KEY_UP,

    ON_FOCUS,

    Begin,  //KeyEvent: S
    Paused, //KeyEvent: P
    GridOnOff,  //KeyEvent: Delete

    MoveDown,   //KeyEvent: Down
    MoveLeft,   //KeyEvent: Left
    MoveRight,  //KeyEvent: Right
    Rotate,     //KeyEvent: Space
    SpeedUp,    //KeyEvent: U

    Fillfull,   //ScreenEvent: Fillfull
    Draw,       //SystemEvent: Draw
    Update,     //SystemEvent: Update
    Timer,      //SystemEvent: Timer

    AnimationEnded, //Animationvent: AnimationEnded. Todo: 在事件的参数中增加id来表示是哪个动画
    LuotiAniEnded,  //LuotiAniEvent: LuotiAniEnded.
    LuotiInstanceEnded,
    AudioEnded,
    NextMusic,
    NextBackground,

    Exit
};

inline Event::Event(EventName eventName, std::any param):
    m_eventName(eventName),
    m_eventParam(param),
    m_type(EventType::None),
    _pad(0)
{
    switch (m_eventName) {
        case EventName::MOUSE_LBUTTON_DOWN:
            m_type = EventType::MouseDown;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mouseButton = {pos->x, pos->y, MouseButton::Left};
            } catch (...) {}
            break;
        case EventName::MOUSE_RBUTTON_DOWN:
            m_type = EventType::MouseDown;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mouseButton = {pos->x, pos->y, MouseButton::Right};
            } catch (...) {}
            break;
        case EventName::MOUSE_MBUTTON_DOWN:
            m_type = EventType::MouseDown;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mouseButton = {pos->x, pos->y, MouseButton::Middle};
            } catch (...) {}
            break;
        case EventName::MOUSE_LBUTTON_UP:
            m_type = EventType::MouseUp;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mouseButton = {pos->x, pos->y, MouseButton::Left};
            } catch (...) {}
            break;
        case EventName::MOUSE_RBUTTON_UP:
            m_type = EventType::MouseUp;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mouseButton = {pos->x, pos->y, MouseButton::Right};
            } catch (...) {}
            break;
        case EventName::MOUSE_MBUTTON_UP:
            m_type = EventType::MouseUp;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mouseButton = {pos->x, pos->y, MouseButton::Middle};
            } catch (...) {}
            break;
        case EventName::MOUSE_MOVING:
            m_type = EventType::MouseMove;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(m_eventParam);
                if (pos) mousePos = {pos->x, pos->y};
            } catch (...) {}
            break;
        case EventName::MOUSE_WHEEL:
            m_type = EventType::MouseWheel;
            try {
                auto wd = std::any_cast<MouseWheelEventData>(m_eventParam);
                mouseWheel = {wd.mouseX, wd.mouseY, wd.x, wd.y};
            } catch (...) {}
            break;
        case EventName::TEXT_INPUT:
            m_type = EventType::TextInput;
            try {
                auto td = std::any_cast<TextInputEventData>(m_eventParam);
                size_t len = td.text.copy(textInput.text, sizeof(textInput.text) - 1);
                textInput.text[len] = '\0';
            } catch (...) {}
            break;
        case EventName::TEXT_EDITING:
            m_type = EventType::TextEditing;
            break;
        case EventName::KEY_DOWN:
            m_type = EventType::KeyDown;
            try {
                auto kd = std::any_cast<KeyEventData>(m_eventParam);
                keyEvent = {kd.keycode, kd.mod, 0, false};
            } catch (...) {}
            break;
        case EventName::KEY_UP:
            m_type = EventType::KeyUp;
            try {
                auto kd = std::any_cast<KeyEventData>(m_eventParam);
                keyEvent = {kd.keycode, kd.mod, 0, false};
            } catch (...) {}
            break;
        default:
            break;
    }
}

class EventQueue{
private:
    static std::mutex m_mtxForEventQueue;
    static std::mutex m_mtxForBeforeEventHandlingWatcher;
    static std::mutex m_mtxForAfterEventHandlingWatcher;
    std::queue<shared_ptr<Event>> m_eventQueue;
    std::unordered_map<EventName, std::vector<shared_ptr<Control>>> m_beforeEventHandlingWatcherMap;
    std::unordered_map<EventName, std::vector<shared_ptr<Control>>> m_afterEventHandlingWatcherMap;
    EventQueue(){}
    ~EventQueue(){clear();}
public:
    static EventQueue* getInstance(void){
        static EventQueue instance; // 静态局部变量，程序运行期间只会被初始化一次
        return &instance;
    }
    static bool isPositionEvent(EventName eventName) {
        switch(eventName){
            case EventName::FINGER_DOWN         :
            case EventName::FINGER_UP           :
            case EventName::FINGER_MOTION       :
            case EventName::MOUSE_LBUTTON_DOWN  :
            case EventName::MOUSE_LBUTTON_UP    :
            case EventName::MOUSE_RBUTTON_DOWN  :
            case EventName::MOUSE_RBUTTON_UP    :
            case EventName::MOUSE_MBUTTON_DOWN  :
            case EventName::MOUSE_MBUTTON_UP    :
            case EventName::MOUSE_MOVING        :
            case EventName::MOUSE_WHEEL         :
                return true;
            default:
                return false;
        }
    }
    void pushEventIntoQueue(shared_ptr<Event> event);
    shared_ptr<Event> popEventFromQueue(void);
    void clear(void);

    bool addBeforeEventHandlingWatcher(EventName eventName, shared_ptr<Control> control);
    bool removeBeforeEventHandlingWatcher(EventName eventName, shared_ptr<Control> control);
    void notifyBeforeEventHandlingWatchers(shared_ptr<Event> event);

    bool addAfterEventHandlingWatcher(EventName eventName, shared_ptr<Control> control);
    bool removeAfterEventHandlingWatcher(EventName eventName, shared_ptr<Control> control);
    void notifyAfterEventHandlingWatchers(shared_ptr<Event> event);
};
#endif
