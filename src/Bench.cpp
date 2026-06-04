#include "Bench.h"

void Bench::initial(void){
    // setBGColor(INITIAL_BG_COLOR);
    // setBorderColor(INITIAL_BORDER_COLOR);
    setTransparent(false);

    m_isInitialed = true;
    SDL_Log("Loading finished, waiting user starting game................................");
    if (m_onInitial != nullptr){
        m_onInitial();
    }
}

Bench::Bench(Control *parent, SRect rect, float xScale, float yScale):
    TopControl(),
    Panel(parent, rect, xScale, yScale),
    m_isLoading(true),
    m_isInitialed(false),
    m_nextTick(0),
    m_nextRepeatTick(0),
    m_isExiting(SDL_APP_CONTINUE),
    m_onInitial(nullptr)
{
    setTransparent(true);

    SDL_Log("Loading resources.....................................");
    m_isLoading = false;
    initial();
}

void Bench::inputControl(shared_ptr<Event> event) {
    // if (m_eventJitter.find(event->m_eventName) != m_eventJitter.end()){
    //     if (SDL_GetTicks() < m_eventJitter[event->m_eventName]){
    //         SDL_Log("Event %d is jittering", event->m_eventName);
    //         return;
    //     }
    //     m_eventJitter[event->m_eventName] = SDL_GetTicks() + DEFAULT_BTN_MS_INTERVAL;
    // }

    // if(EventQueue::isPositionEvent(event->m_eventName)){
    //     m_lastAction = event;
    //     m_nextTick = SDL_GetTicks() + DEFAULT_BTN_MS_INTERVAL;
    // }

    triggerEvent(event);
}

void Bench::repeatTrigger(void){
    if (m_lastAction != nullptr){
        uint64_t currentTick = SDL_GetTicks();
        if (currentTick < m_nextRepeatTick || currentTick < m_eventJitter[m_lastAction->m_eventName]){
            return;
        }

        switch(m_lastAction->m_eventName){
            case EventName::FINGER_DOWN:
            case EventName::FINGER_MOTION:
            case EventName::MOUSE_LBUTTON_DOWN:
            case EventName::MOUSE_MBUTTON_DOWN:
            case EventName::MOUSE_RBUTTON_DOWN:
                triggerEvent(m_lastAction);
                break;
            default:
                break;
        }
        m_nextRepeatTick = SDL_GetTicks() + DEFAULT_BTN_MS_REPEAT;
    }
}
void Bench::update() {
    if (m_isLoading){
        m_isLoading = false;
        initial();
    }else {
        if (m_lastAction != nullptr){
            repeatTrigger();
        }
        Panel::update();
    }
}
void Bench::draw(void){
    if (!m_visible) return;

    Panel::draw();
}

SDL_AppResult Bench::isExiting(void) {
    return m_isExiting;
}

void Bench::setOnInitial(OnInitialHandler handler) {
    if (m_isInitialed) {
        handler();
    } else {
        m_onInitial = handler;
    }
}

void Bench::resized(SRect newRect) {
    Panel::resized(newRect);
    for (auto& child : m_children) {
        if (!child->getVisible()) continue;
        auto panel = dynamic_pointer_cast<Panel>(child);
        if (panel) {
            panel->resolveChildPercentages();
            if (panel->getLayoutEngine()) {
                panel->reflowChildren();
            }
        }
    }
}

void Bench::addControl(shared_ptr<Control> control) {
    Panel::addControl(control);
    resolveChildPercentages();
    auto panel = dynamic_pointer_cast<Panel>(control);
    if (panel) {
        panel->resolveChildPercentages();
        if (panel->getLayoutEngine()) {
            panel->reflowChildren();
        }
    }
}
