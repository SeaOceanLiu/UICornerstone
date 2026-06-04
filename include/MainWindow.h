#ifndef MainWindowH
#define MainWindowH
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
    ResourceProvider *m_resourceProvider;
    SSize m_size;
    SPoint m_pos;
    float m_displayWidth;
    float m_displayHeight;

    uint64_t m_nextTick;
    uint64_t m_nextRepeatTick;
    shared_ptr<Event> m_lastAction;
    unordered_map<EventName, uint64_t> m_eventJitter;

    MainWindow():
        m_resourceProvider(nullptr),
        m_size({INITIAL_WIDTH, INITIAL_HEIGHT}),
        m_pos({INITIAL_POSX, INITIAL_POSY}),
        m_displayWidth(0),
        m_displayHeight(0)
    {
        if (!BackendManager::instance()->initialize("sdl3", APP_NAME,
                INITIAL_WIDTH, INITIAL_HEIGHT, WINDOW_FLAG)) {
            printf("BackendManager initialization failed\n");
            return;
        }

        m_resourceProvider = ResourceProvider::createFilesystem(ConstDef::pathPrefix.string());

        Window* w = BackendManager::instance()->window();
        if (w) {
            m_size = w->getSize();
            m_pos = w->getPosition();
            m_displayWidth = w->getDisplayWidth();
            m_displayHeight = w->getDisplayHeight();
        }
    }

    ~MainWindow() {
        delete m_resourceProvider;
    }
public:
    static MainWindow* getInstance(void){
        static MainWindow instance;
        return &instance;
    }

    void onWindowResized(int w, int h) {
        DEBUG_STREAM << "Window size changed: " << w << " x " << h << std::endl;
        m_size = SSize{(float)w, (float)h};
    }

    void onWindowMoved(int x, int y) {
        DEBUG_STREAM << "Window position changed: (" << x << ", " << y << ")" << std::endl;
        m_pos = SPoint{(float)x, (float)y};
    }

    Window* getWindow(void) { return BackendManager::instance()->window(); }
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
