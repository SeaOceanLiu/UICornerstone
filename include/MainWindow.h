#ifndef MainWindowH
#define MainWindowH
#include <SDL3/SDL.h>
#include "DebugTrace.h"
#include "ConstDef.h"
#include "EventQueue.h"
#include "Utility.h"
#include "RenderDevice.h"


#define MAINWIN (MainWindow::getInstance())
#define GET_RENDERER (MAINWIN->getRenderer())
#define GET_RENDERDEVICE (MAINWIN->getRenderDevice())

class MainWindow {
private:
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    RenderDevice *m_renderDevice;
    SSize m_size;
    SPoint m_pos;

    SDL_DisplayID m_displayId;
    float m_displayWidth;
    float m_displayHeight;

    uint64_t m_nextTick;
    uint64_t m_nextRepeatTick;
    shared_ptr<Event> m_lastAction;
    unordered_map<EventName, uint64_t> m_eventJitter; // jitter for each event

    MainWindow():
        m_window(nullptr),
        m_renderer(nullptr),
        m_renderDevice(nullptr),
        m_size({INITIAL_WIDTH, INITIAL_HEIGHT}),
        m_pos({INITIAL_POSX, INITIAL_POSY})
    {
        // 创建SDL3窗口和渲染器
        if (!SDL_CreateWindowAndRenderer(APP_NAME, INITIAL_WIDTH, INITIAL_HEIGHT, WINDOW_FLAG, &m_window, &m_renderer)) {
            SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
            return;
        }

        m_renderDevice = CreateSDL3RenderDevice(m_renderer);

        //设置窗口位置
        if(!SDL_SetWindowPosition(m_window, INITIAL_POSX, INITIAL_POSY)){
            SDL_Log("Couldn't set window position: %s", SDL_GetError());
        }

        // 是否开启垂直同步
        if(!SDL_SetRenderVSync(m_renderer, VSYNC_FLAG)){
            SDL_Log("Couldn't set vsync: %s", SDL_GetError());
        }

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
        if(!SDL_GetWindowSize(m_window, &windowWidth, &windowHeight)){
            DEBUG_STREAM << "Couldn't get window size: " << SDL_GetError() << std::endl;
        }
        m_size = SSize{(float)windowWidth, (float)windowHeight};
    }

    ~MainWindow() {
        delete m_renderDevice;
        if (m_renderer) SDL_DestroyRenderer(m_renderer);
        if (m_window) SDL_DestroyWindow(m_window);
    }
public:
    static MainWindow* getInstance(void){
        static MainWindow instance; // 静态局部变量，程序运行期间只会被初始化一次
        return &instance;
    }

     // Window resize event callback function
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

    SDL_Window* getWindow(void) { return m_window; }
    SDL_Renderer* getRenderer(void) { return m_renderer; }
    RenderDevice* getRenderDevice(void) { return m_renderDevice; }
    SSize getWindowSize(void) { return m_size; }
    SPoint getWindowPos(void) { return m_pos; }
    // SDL_PixelFormat getPixelFormat(void) { return m_pixelFormat; }
    float getDisplayWidth(void) { return m_displayWidth; }
    float getDisplayHeight(void) { return m_displayHeight; }
    SSize getDisplaySize(void) { return SSize{m_displayWidth, m_displayHeight}; }
};
#endif // MainWindowH
