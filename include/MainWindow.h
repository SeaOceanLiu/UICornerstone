#ifndef MainWindowH
#define MainWindowH
#include <SDL3/SDL.h>
#include "DebugTrace.h"
#include "ConstDef.h"
#include "EventQueue.h"
#include "Utility.h"
#include "RenderDevice.h"
#include "TextRenderer.h"
#include "Window.h"
#include "InputBackend.h"


#define MAINWIN (MainWindow::getInstance())
#define GET_RENDERER (MAINWIN->getRenderer())
#define GET_RENDERDEVICE (MAINWIN->getRenderDevice())
#define GET_TEXTRENDERER (MAINWIN->getTextRenderer())
#define GET_INPUTBACKEND (MAINWIN->getInputBackend())


class MainWindow {
private:
    Window *m_window;
    SDL_Renderer *m_renderer;
    RenderDevice *m_renderDevice;
    TextRenderer *m_textRenderer;
    InputBackend *m_inputBackend;
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
        m_window(nullptr),
        m_renderer(nullptr),
        m_renderDevice(nullptr),
        m_textRenderer(nullptr),
        m_inputBackend(nullptr),
        m_size({INITIAL_WIDTH, INITIAL_HEIGHT}),
        m_pos({INITIAL_POSX, INITIAL_POSY})
    {
        m_window = CreateSDL3Window(APP_NAME, INITIAL_WIDTH, INITIAL_HEIGHT, WINDOW_FLAG);
        if (!m_window) {
            SDL_Log("Couldn't create window");
            return;
        }

        m_renderDevice = m_window->renderDevice();
        m_renderer = static_cast<SDL_Renderer*>(m_renderDevice->getNativeHandle());
        m_textRenderer = CreateSDL3TextRenderer(m_renderDevice);
        m_inputBackend = CreateSDL3InputBackend(m_window);

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
        if(!SDL_GetWindowSize(m_window ? static_cast<SDL_Window*>(m_window->nativeHandle()) : nullptr, &windowWidth, &windowHeight)){
            DEBUG_STREAM << "Couldn't get window size: " << SDL_GetError() << std::endl;
        }
        m_size = SSize{(float)windowWidth, (float)windowHeight};
    }

    ~MainWindow() {
        delete m_textRenderer;
        delete m_inputBackend;
        delete m_window;
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
        return m_window ? static_cast<SDL_Window*>(m_window->nativeHandle()) : nullptr;
    }
    SDL_Renderer* getRenderer(void) { return m_renderer; }
    Window* getWindowObject(void) { return m_window; }
    RenderDevice* getRenderDevice(void) { return m_renderDevice; }
    TextRenderer* getTextRenderer(void) { return m_textRenderer; }
    InputBackend* getInputBackend(void) { return m_inputBackend; }
    SSize getWindowSize(void) { return m_size; }
    SPoint getWindowPos(void) { return m_pos; }
    float getDisplayWidth(void) { return m_displayWidth; }
    float getDisplayHeight(void) { return m_displayHeight; }
    SSize getDisplaySize(void) { return SSize{m_displayWidth, m_displayHeight}; }
};
#endif // MainWindowH
