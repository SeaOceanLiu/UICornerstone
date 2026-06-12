#ifndef StateMachineH
#define StateMachineH

#include <functional>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cstdio>
#include "EventTypes.h"

using namespace std;

enum class State: int;    // 使用时，需要重新定义该枚举值
enum class EventName: int; // 自定义事件路由类型

class Event{
public:
    EventType m_type;

    union {
        EventMousePos    mousePos;
        EventMouseButton mouseButton;
        EventMouseWheel  mouseWheel;
        EventKey         keyEvent;
        EventTextInput   textInput;
        EventTextEditing textEditing;
        EventResize      resizeEvent;
        EventWindowMoved windowMoved;
        EventFocus       focusEvent;
        char _pad;
    };
    int   customInt;
    void* customPtr;

    Event():
        m_type(EventType::None),
        customInt(0),
        customPtr(nullptr)
    {
    }

    Event(EventType type):
        m_type(type),
        customInt(0),
        customPtr(nullptr)
    {
    }

    Event(const Event& event):
        m_type(event.m_type),
        customInt(event.customInt),
        customPtr(event.customPtr)
    {
        copyUnion(event);
    }
    Event(Event&& event):
        m_type(event.m_type),
        customInt(event.customInt),
        customPtr(event.customPtr)
    {
        copyUnion(event);
    }
    Event& operator=(const Event& event){
        m_type = event.m_type;
        customInt = event.customInt;
        customPtr = event.customPtr;
        copyUnion(event);
        return *this;
    }
    Event& operator=(Event&& event){
        m_type = event.m_type;
        customInt = event.customInt;
        customPtr = event.customPtr;
        copyUnion(event);
        return *this;
    }

    ~Event(){};

private:
    void copyUnion(const Event& event) {
        switch (event.m_type) {
            case EventType::MouseMove:
                mousePos = event.mousePos; break;
            case EventType::MouseDown:
            case EventType::MouseUp:
                mouseButton = event.mouseButton; break;
            case EventType::MouseWheel:
                mouseWheel = event.mouseWheel; break;
            case EventType::KeyDown:
            case EventType::KeyUp:
                keyEvent = event.keyEvent; break;
            case EventType::TextInput:
                textInput = event.textInput; break;
            case EventType::TextEditing:
                textEditing = event.textEditing; break;
            case EventType::WindowResize:
                resizeEvent = event.resizeEvent; break;
            case EventType::WindowMoved:
                windowMoved = event.windowMoved; break;
            case EventType::FocusGained:
            case EventType::FocusLost:
                focusEvent = event.focusEvent; break;
            case EventType::FingerDown:
            case EventType::FingerUp:
            case EventType::FingerMotion:
                mousePos = event.mousePos; break;
            default:
                _pad = 0; break;
        }
    }
};

// StateMachine 模板类
class StateMachine
{
    using EnterLeaveHandler = std::function<void (State)>;
    using EventHandler = std::function<bool(shared_ptr<Event>)>;

protected:
    State m_currentState;
    std::unordered_map<State, EnterLeaveHandler> m_enterStateHandlers;
    std::unordered_map<State, EnterLeaveHandler> m_leaveStateHandlers;
    std::unordered_map<State, EventHandler> m_stateEventHandlers;
public:
    StateMachine(State initialState):
        m_currentState(initialState)
    {
    }
    virtual ~StateMachine(){};

    void registerEnterStateHandler(State state, EnterLeaveHandler handler){
        m_enterStateHandlers[state] = handler;
    }
    void registerLeaveStateHandler(State state, EnterLeaveHandler handler){
        m_leaveStateHandlers[state] = handler;
    }
    // 注册状态事件处理程序
    void registerStateEventHandler(State state, EventHandler handler) {
        m_stateEventHandlers[state] = handler;
    }

    State getState(void){
        return m_currentState;
    }

    void setState(State state){
        if (m_currentState == state){
            return;
        }
        // SDL_Log("Leave state %d", static_cast<int>(m_currentState));
        auto it = m_leaveStateHandlers.find(m_currentState);
        if (it != m_leaveStateHandlers.end()) {
            it->second(state);
        }
        auto lastState = m_currentState;
        m_currentState = state;
        // SDL_Log("Enter state %d", static_cast<int>(m_currentState));
        it = m_enterStateHandlers.find(m_currentState);
        if (it != m_enterStateHandlers.end()) {
            it->second(lastState);
        }
    }

    // 触发状态事件
    bool stateEvent(shared_ptr<Event> event) {
        auto it = m_stateEventHandlers.find(m_currentState);
        if (it != m_stateEventHandlers.end()) {
            return it->second(event);
        } else {
            printf("No handler for event %d in state %d\n", static_cast<int>(event->m_type), static_cast<int>(m_currentState));
            return false;
        }
    }
};

#endif // StateMachineH