#ifndef EventQueueH
#define EventQueueH

#include <queue>
#include <mutex>
#include <memory>
#include "StateMachine.h"

using namespace std;

class Control;

// Custom event routing IDs (for EventType::Custom events)
enum class EventName: int{
    None,
    ON_FOCUS,
    AnimationEnded,
    LuotiInstanceEnded,
    LuotiAniEnded,
    AudioEnded,
    NextMusic,
    NextBackground,
    Exit
};

class EventQueue{
private:
    static std::mutex m_mtxForEventQueue;
    static std::mutex m_mtxForBeforeEventHandlingWatcher;
    static std::mutex m_mtxForAfterEventHandlingWatcher;
    std::queue<shared_ptr<Event>> m_eventQueue;
    std::unordered_map<EventType, std::vector<weak_ptr<Control>>> m_beforeEventHandlingWatcherMap;
    std::unordered_map<EventType, std::vector<weak_ptr<Control>>> m_afterEventHandlingWatcherMap;
    EventQueue(){}
    ~EventQueue(){clear();}
public:
    static EventQueue* getInstance(void){
        static EventQueue instance;
        return &instance;
    }
    static bool isPositionEvent(EventType type) {
        switch(type){
            case EventType::FingerDown:
            case EventType::FingerUp:
            case EventType::FingerMotion:
            case EventType::MouseDown:
            case EventType::MouseUp:
            case EventType::MouseMove:
            case EventType::MouseWheel:
                return true;
            default:
                return false;
        }
    }
    void pushEventIntoQueue(shared_ptr<Event> event);
    shared_ptr<Event> popEventFromQueue(void);
    void clear(void);

    bool addBeforeEventHandlingWatcher(EventType eventType, shared_ptr<Control> control);
    bool removeBeforeEventHandlingWatcher(EventType eventType, shared_ptr<Control> control);
    bool notifyBeforeEventHandlingWatchers(shared_ptr<Event> event);

    bool addAfterEventHandlingWatcher(EventType eventType, shared_ptr<Control> control);
    bool removeAfterEventHandlingWatcher(EventType eventType, shared_ptr<Control> control);
    void notifyAfterEventHandlingWatchers(shared_ptr<Event> event);
};
#endif
