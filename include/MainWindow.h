#ifndef MainWindowH
#define MainWindowH
#include "ConstDef.h"
#include "EventQueue.h"
#include "Utility.h"
#include "BackendPlugin.h"
#include "ResourceProvider.h"
#include "AppCallbacks.h"
#include "FocusManager.h"


#define MAINWIN (MainWindow::getInstance())
#define GET_RENDERDEVICE (MAINWIN->getRenderDevice())
#define GET_TEXTRENDERER (MAINWIN->getTextRenderer())
#define GET_INPUTBACKEND (MAINWIN->getInputBackend())
#define GET_RESOURCEPROVIDER (MAINWIN->getResourceProvider())
#define GET_FOCUSMANAGER (MAINWIN->getFocusManager())


class MainWindow {
private:
    ResourceProvider *m_resourceProvider;
    FocusManager m_focusManager;
    SSize m_size;
    SPoint m_pos;
    float m_displayWidth;
    float m_displayHeight;

    bool m_quitRequested;
    uint64_t m_nextTick;
    uint64_t m_nextRepeatTick;
    shared_ptr<Event> m_lastAction;
    unordered_map<EventType, uint64_t> m_eventJitter;
    int m_pendingResizeW = -1, m_pendingResizeH = -1;
    uint64_t m_lastResizeArrival = 0;

    MainWindow():
        m_quitRequested(false),
        m_resourceProvider(nullptr),
        m_size({INITIAL_WIDTH, INITIAL_HEIGHT}),
        m_pos({INITIAL_POSX, INITIAL_POSY}),
        m_displayWidth(0),
        m_displayHeight(0)
    {
    #if defined(UICORNERSTONE_BACKEND_SDL3)
        const char* backendName = "sdl3";
    #elif defined(UICORNERSTONE_BACKEND_SFML)
        const char* backendName = "sfml";
    #elif defined(UICORNERSTONE_BACKEND_RAYLIB)
        const char* backendName = "raylib";
    #else
        const char* backendName = "sdl3";
    #endif
        if (!BackendManager::instance()->initialize(backendName, APP_NAME,
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
        m_size = SSize{(float)w, (float)h};
    }

    void onWindowMoved(int x, int y) {
        m_pos = SPoint{(float)x, (float)y};
    }

    Window* getWindow(void) { return BackendManager::instance()->window(); }
    RenderDevice* getRenderDevice(void) { return BackendManager::instance()->renderDevice(); }
    TextRenderer* getTextRenderer(void) { return BackendManager::instance()->textRenderer(); }
    InputBackend* getInputBackend(void) { return BackendManager::instance()->inputBackend(); }
    ResourceProvider* getResourceProvider(void) { return m_resourceProvider; }
    FocusManager* getFocusManager() { return &m_focusManager; }
    SSize getWindowSize(void) { return m_size; }
    SPoint getWindowPos(void) { return m_pos; }
    float getDisplayWidth(void) { return m_displayWidth; }
    float getDisplayHeight(void) { return m_displayHeight; }
    SSize getDisplaySize(void) { return SSize{m_displayWidth, m_displayHeight}; }

    // Set window title
    void setTitle(const std::string& title) {
        Window* w = BackendManager::instance()->window();
        if (w) w->setTitle(title);
    }

    // Request graceful quit from within callbacks (e.g. "Exit" menu item)
    void quit() { m_quitRequested = true; }

    // === Mode 1: Owned loop ===
    // Runs the entire main loop internally.
    // Returns 0 on normal exit, 1 on init failure.
    int run(AppCallbacks* app);

    // === Mode 2: Tick-based API ===
    // For users who want to integrate with their own main loop
    // (e.g. third-party engine loop).
    //
    // Usage:
    //   if (!MAINWIN->init(app)) return 1;
    //   while (running) {
    //       running = MAINWIN->processEvents();  // false on WindowClose
    //       MAINWIN->update(app);
    //       MAINWIN->render(app);
    //   }
    //   MAINWIN->shutdown(app);

    // Initialize the app. Must be called before the loop.
    bool init(AppCallbacks* app);

    // Poll and dispatch all pending events.
    // Handles WindowResize/WindowMoved internally.
    // Dispatches other events to the control tree via old-style dispatch.
    // Calls app->onEvent() for the new-style event notification.
    // Returns false if WindowClose was received (caller should stop).
    bool processEvents(AppCallbacks* app);

    // Single frame update (calls app->onUpdate()).
    void update(AppCallbacks* app);

    // Single frame render + present (calls app->onRender() + present).
    void render(AppCallbacks* app);

    // Shutdown (calls app->onQuit()).
    void shutdown(AppCallbacks* app);
};
#endif // MainWindowH
