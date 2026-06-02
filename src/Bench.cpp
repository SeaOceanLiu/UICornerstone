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
    // 将资源加载到内存中
    ResourceLoader::getInstance()->loadConfig();
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
        if(ResourceLoader::getInstance()->getLoadingProgress() == 1.0f){
            ResourceLoader::getInstance()->detachLoadingThread();
            m_isLoading = false;
            initial();
        }
    }else {
        if (m_lastAction != nullptr){
            repeatTrigger();
        }
        Panel::update();
    }
}
void Bench::draw(void){
    if(m_isLoading){
        ControlImpl::preDraw();

        SRect rect = {0, m_rect.height / 2 - 50, m_rect.width, 100};
        SRect percentRect = {0, m_rect.height / 2 - 50, m_rect.width * ResourceLoader::getInstance()->getLoadingProgress(), 100};

        GET_RENDERDEVICE->setDrawColor(SColor(255, 165, 0, 255));
        GET_RENDERDEVICE->fillRect(percentRect);
        GET_RENDERDEVICE->setDrawColor(SColor(128, 128, 128, 255));
        GET_RENDERDEVICE->drawRect(rect);
        return;
    }

    if (!m_visible) return;
    // 绘制子控件
    Panel::draw();

    // drawCenteredRectangle(getRenderer(), (int)m_rect.width, (int)m_rect.height);
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

// Draw a centered rectangle
void Bench::drawCenteredRectangle(SDL_Renderer* renderer, int windowWidth, int windowHeight) {
    // Calculate rectangle size (60% of window size)
    int rectWidth = static_cast<int>(windowWidth * 0.6f);
    int rectHeight = static_cast<int>(windowHeight * 0.6f);

    // Calculate rectangle position (centered)
    int rectX = (windowWidth - rectWidth) / 2;
    int rectY = (windowHeight - rectHeight) / 2;

    // Set draw color to dark blue
    SDL_SetRenderDrawColor(renderer, 0, 0, 139, 255); // Dark blue

    // Draw filled rectangle
    SDL_FRect rect = { static_cast<float>(rectX), static_cast<float>(rectY),
                        static_cast<float>(rectWidth), static_cast<float>(rectHeight) };
    SDL_RenderFillRect(renderer, &rect);

    // Set border color to white
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White

    // Draw rectangle border
    SDL_RenderRect(renderer, &rect);
}
