// #include "EventQueue.h"
#include "ControlBase.h"

std::mutex EventQueue::m_mtxForEventQueue;
std::mutex EventQueue::m_mtxForBeforeEventHandlingWatcher;
std::mutex EventQueue::m_mtxForAfterEventHandlingWatcher;

void EventQueue::pushEventIntoQueue(shared_ptr<Event> event){
    m_mtxForEventQueue.lock();
    m_eventQueue.push(event);
    m_mtxForEventQueue.unlock();
}

shared_ptr<Event> EventQueue::popEventFromQueue(void){
    m_mtxForEventQueue.lock();
    if (m_eventQueue.empty()){
        m_mtxForEventQueue.unlock();
        return nullptr;
    }
    shared_ptr<Event> event = m_eventQueue.front();
    m_eventQueue.pop();
    m_mtxForEventQueue.unlock();
    return event;
}
void EventQueue::clear(void){
    shared_ptr<Event> event = popEventFromQueue();
    while(event != nullptr){
        event = popEventFromQueue();
    }

    for(auto& it : m_beforeEventHandlingWatcherMap){
        it.second.clear();
    }
    m_beforeEventHandlingWatcherMap.clear();
    for(auto& it : m_afterEventHandlingWatcherMap){
        it.second.clear();
    }
    m_afterEventHandlingWatcherMap.clear();
}

bool EventQueue::addBeforeEventHandlingWatcher(EventType eventType, shared_ptr<Control> control){
    if (control == nullptr || eventType == EventType::None) return false;

    m_mtxForBeforeEventHandlingWatcher.lock();
    if (m_beforeEventHandlingWatcherMap.find(eventType) == m_beforeEventHandlingWatcherMap.end() ||
        std::find(m_beforeEventHandlingWatcherMap[eventType].begin(), m_beforeEventHandlingWatcherMap[eventType].end(), control) ==
            m_beforeEventHandlingWatcherMap[eventType].end()){
        m_beforeEventHandlingWatcherMap[eventType].push_back(control);
    }
    m_mtxForBeforeEventHandlingWatcher.unlock();

    return true;
}

bool EventQueue::removeBeforeEventHandlingWatcher(EventType eventType, shared_ptr<Control> control){
    if (control == nullptr || eventType == EventType::None) return false;

    m_mtxForBeforeEventHandlingWatcher.lock();
    auto it = m_beforeEventHandlingWatcherMap.find(eventType);
    if (it != m_beforeEventHandlingWatcherMap.end()){
        auto it2 = std::find(it->second.begin(), it->second.end(), control);
        if (it2 != it->second.end()){
            it->second.erase(it2);
            m_mtxForBeforeEventHandlingWatcher.unlock();
            return true;
        }
    }
    m_mtxForBeforeEventHandlingWatcher.unlock();
    return false;
}

void EventQueue::notifyBeforeEventHandlingWatchers(shared_ptr<Event> event){
    if (event->m_type == EventType::None) return;

    m_mtxForBeforeEventHandlingWatcher.lock();
    auto it = m_beforeEventHandlingWatcherMap.find(event->m_type);
    if (it != m_beforeEventHandlingWatcherMap.end()){
        for (auto control : it->second){
            if(control->beforeEventHandlingWatcher(event))
                break;
        }
    }
    m_mtxForBeforeEventHandlingWatcher.unlock();
}


bool EventQueue::addAfterEventHandlingWatcher(EventType eventType, shared_ptr<Control> control){
    if (control == nullptr || eventType == EventType::None) return false;

    m_mtxForAfterEventHandlingWatcher.lock();
    if (m_afterEventHandlingWatcherMap.find(eventType) == m_afterEventHandlingWatcherMap.end() ||
        std::find(m_afterEventHandlingWatcherMap[eventType].begin(), m_afterEventHandlingWatcherMap[eventType].end(), control) ==
            m_afterEventHandlingWatcherMap[eventType].end()){
        m_afterEventHandlingWatcherMap[eventType].push_back(control);
    }
    m_mtxForAfterEventHandlingWatcher.unlock();

    return true;
}

bool EventQueue::removeAfterEventHandlingWatcher(EventType eventType, shared_ptr<Control> control){
    if (control == nullptr || eventType == EventType::None) return false;

    m_mtxForAfterEventHandlingWatcher.lock();
    auto it = m_afterEventHandlingWatcherMap.find(eventType);
    if (it != m_afterEventHandlingWatcherMap.end()){
        auto it2 = std::find(it->second.begin(), it->second.end(), control);
        if (it2 != it->second.end()){
            it->second.erase(it2);
            m_mtxForAfterEventHandlingWatcher.unlock();
            return true;
        }
    }
    m_mtxForAfterEventHandlingWatcher.unlock();
    return false;
}

void EventQueue::notifyAfterEventHandlingWatchers(shared_ptr<Event> event){
    if (event->m_type == EventType::None) return;

    m_mtxForAfterEventHandlingWatcher.lock();
    auto it = m_afterEventHandlingWatcherMap.find(event->m_type);
    if (it != m_afterEventHandlingWatcherMap.end()){
        for (auto control : it->second){
            if(control->afterEventHandlingWatcher(event))
                break;
        }
    }
    m_mtxForAfterEventHandlingWatcher.unlock();
}
