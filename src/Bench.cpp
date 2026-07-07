#include "Bench.h"
#include "PlatformUtils.h"
#include "EventTypes.h"

void Bench::initial(void){
    // setBGColor(INITIAL_BG_COLOR);
    // setBorderColor(INITIAL_BORDER_COLOR);
    setTransparent(false);

    m_isInitialed = true;
    Platform::Log("Loading finished, waiting user starting game................................");
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
    m_isExiting(0),
    m_onInitial(nullptr)
{
    setTransparent(true);

    Platform::Log("Loading resources.....................................");
    m_isLoading = false;
    m_isFocusBoundary = true;
    initial();
}

void Bench::inputControl(shared_ptr<Event> event) {
    triggerEvent(event);
}

void Bench::repeatTrigger(void){
    if (m_lastAction != nullptr){
        uint64_t currentTick = Platform::GetTicks();
        if (currentTick < m_nextRepeatTick || currentTick < m_eventJitter[m_lastAction->m_type]){
            return;
        }

        switch(m_lastAction->m_type){
            case EventType::FingerDown:
            case EventType::FingerMotion:
            case EventType::MouseDown:
                triggerEvent(m_lastAction);
                break;
            default:
                break;
        }
        m_nextRepeatTick = Platform::GetTicks() + DEFAULT_BTN_MS_REPEAT;
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

bool Bench::handleEvent(shared_ptr<Event> event) {
    // Intercept Tab / Ctrl+Tab before passing to children
    if (event->m_type == EventType::KeyDown &&
        event->keyEvent.keycode == KeyCode::Tab) {
        bool ctrl = isModSet(event->keyEvent.mod, KeyMod::LCtrl) || isModSet(event->keyEvent.mod, KeyMod::RCtrl);
        bool shift = isModSet(event->keyEvent.mod, KeyMod::Shift);

        if (ctrl) {
            if (shift)
                GET_FOCUSMANAGER->focusPrevScope();
            else
                GET_FOCUSMANAGER->focusNextScope();
        } else {
            Control* current = GET_FOCUSMANAGER->getCurrentFocused();
            if (shift)
                GET_FOCUSMANAGER->focusPrev(current);
            else
                GET_FOCUSMANAGER->focusNext(current);
        }
        return true;
    }
    return Panel::handleEvent(event);
}

int Bench::isExiting(void) {
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
