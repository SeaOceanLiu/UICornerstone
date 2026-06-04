#ifndef MainWindowH
#define MainWindowH
#include <SDL3/SDL.h>
#include "DebugTrace.h"
#include "ConstDef.h"
#include "EventQueue.h"
#include "Utility.h"
#include "BackendPlugin.h"
#include "ResourceProvider.h"


#define MAINWIN (MainWindow::getInstance())
#define GET_RENDERDEVICE (MAINWIN->getRenderDevice())
#define GET_TEXTRENDERER (MAINWIN->getTextRenderer())
#define GET_INPUTBACKEND (MAINWIN->getInputBackend())
#define GET_RESOURCEPROVIDER (MAINWIN->getResourceProvider())


class MainWindow {
private:
    SDL_Renderer *m_renderer;
    ResourceProvider *m_resourceProvider;
    SSize m_size;
    SPoint m_pos;

    SDL_DisplayID m_displayId;
    float m_displayWidth;
    float m_displayHeight;

    uint64_t m_nextTick;
    uint64_t m_nextRepeatTick;
    shared_ptr<Event> m_lastAction;
    unordered_map<EventName, uint64_t> m_eventJitter;

    MainWindow():
        m_renderer(nullptr),
        m_resourceProvider(nullptr),
        m_size({INITIAL_WIDTH, INITIAL_HEIGHT}),
        m_pos({INITIAL_POSX, INITIAL_POSY})
    {
        if (!BackendManager::instance()->initialize("sdl3", APP_NAME,
                INITIAL_WIDTH, INITIAL_HEIGHT, WINDOW_FLAG)) {
            SDL_Log("BackendManager initialization failed");
            return;
        }

        m_renderer = static_cast<SDL_Renderer*>(
            BackendManager::instance()->renderDevice()->getNativeHandle());

        m_resourceProvider = ResourceProvider::createFilesystem(ConstDef::pathPrefix.string());

        // 获取显示器信息
        SDL_DisplayID m_displayId = SDL_GetPrimaryDisplay();
        if (m_displayId == 0) {
            SDL_Log("SDL_GetPrimaryDisplay Error: %s\n", SDL_GetError());
            return;
        }

        const SDL_DisplayMode *displayMode = SDL_GetCurrentDisplayMode(m_displayId);
        if (displayMode == nullptr) {
            SDL_Log("SDL_GetCurrentDisplayMode Error: %s\n", SDL_GetError());
            return;
        }
        m_displayWidth = (float)displayMode->w * displayMode->pixel_density;
        m_displayHeight = (float)displayMode->h * displayMode->pixel_density;

        // Get window size
        int windowWidth = INITIAL_WIDTH;
        int windowHeight = INITIAL_HEIGHT;
        if(!SDL_GetWindowSize(
            static_cast<SDL_Window*>(
                BackendManager::instance()->window()->nativeHandle()),
            &windowWidth, &windowHeight)){
            DEBUG_STREAM << "Couldn't get window size: " << SDL_GetError() << std::endl;
        }
        m_size = SSize{(float)windowWidth, (float)windowHeight};
    }

    ~MainWindow() {
        // BackendManager shutdown is handled by its own destructor
        m_renderer = nullptr;
        delete m_resourceProvider;
    }
public:
    static MainWindow* getInstance(void){
        static MainWindow instance;
        return &instance;
    }

    static void handleWindowEvent(const SDL_WindowEvent& windowEvent) {
        switch (windowEvent.type) {
            case SDL_EVENT_WINDOW_RESIZED:
                DEBUG_STREAM << "Window size changed: " << windowEvent.data1 << " x " << windowEvent.data2 << std::endl;
                MainWindow::getInstance()->m_size = SSize{(float)windowEvent.data1, (float)windowEvent.data2};
                break;
            case SDL_EVENT_WINDOW_MOVED:
                DEBUG_STREAM << "Window position changed: (" << windowEvent.data1 << ", " << windowEvent.data2 << ")" << std::endl;
                MainWindow::getInstance()->m_pos = SPoint{(float)windowEvent.data1, (float)windowEvent.data2};
                break;
            default:
                break;
        }
    }

    SDL_Window* getWindow(void) {
        Window* w = BackendManager::instance()->window();
        return w ? static_cast<SDL_Window*>(w->nativeHandle()) : nullptr;
    }
    SDL_Renderer* getRenderer(void) { return m_renderer; }
    Window* getWindowObject(void) { return BackendManager::instance()->window(); }
    RenderDevice* getRenderDevice(void) { return BackendManager::instance()->renderDevice(); }
    TextRenderer* getTextRenderer(void) { return BackendManager::instance()->textRenderer(); }
    InputBackend* getInputBackend(void) { return BackendManager::instance()->inputBackend(); }
    ResourceProvider* getResourceProvider(void) { return m_resourceProvider; }
    SSize getWindowSize(void) { return m_size; }
    SPoint getWindowPos(void) { return m_pos; }
    float getDisplayWidth(void) { return m_displayWidth; }
    float getDisplayHeight(void) { return m_displayHeight; }
    SSize getDisplaySize(void) { return SSize{m_displayWidth, m_displayHeight}; }
};
#endif // MainWindowH
