#include "BackendPlugin.h"
#include "UICornerstoneAPI.h"
#include "CallbackAdapters.h"
#include <cstdio>

// ============================================================
// BackendManager implementation
// ============================================================

BackendAPI BackendManager::s_registeredAPI = {};
bool BackendManager::s_initialized = false;

BackendManager* BackendManager::instance() {
    static BackendManager mgr;
    return &mgr;
}

void BackendManager::registerBackend(const BackendAPI& api) {
    s_registeredAPI = api;
}

bool BackendManager::initialize(const std::string& backendName,
                                const char* title,
                                int width, int height, uint32_t flags) {
    if (s_initialized) return true;

    BackendAPI api = {};
    bool found = false;
#if !defined(UICORNERSTONE_BUILD_SHARED)
    // DLL plugin mode: backend lives in UIBackend_xxx.dll loaded via LoadLibrary,
    // so these symbols are not available in UICornerstone.dll.
#if defined(UICORNERSTONE_BACKEND_SDL3)
    if (backendName == "sdl3") {
        extern BackendAPI g_sdl3Backend;
        extern void RegisterSDL3SurfaceFactories(void);
        extern void RegisterSDL3CursorFactories(void);
        RegisterSDL3SurfaceFactories();
        RegisterSDL3CursorFactories();
        api = g_sdl3Backend;
        found = true;
    }
#endif
#if defined(UICORNERSTONE_BACKEND_SFML)
    if (backendName == "sfml") {
        extern BackendAPI g_sfmlBackend;
        api = g_sfmlBackend;
        found = true;
    }
#endif
#if defined(UICORNERSTONE_BACKEND_RAYLIB)
    if (backendName == "raylib") {
        extern BackendAPI g_raylibBackend;
        api = g_raylibBackend;
        found = true;
    }
#endif
#endif // !UICORNERSTONE_BUILD_SHARED
    if (!found) {
        if (s_registeredAPI.version > 0) {
            api = s_registeredAPI;
        } else {
            printf("BackendManager: unknown backend \"%s\"\n", backendName.c_str());
            return false;
        }
    }

    if (api.init && !api.init()) {
        printf("BackendManager: backend init failed\n");
        return false;
    }

    printf("BackendManager: using \"%s\" backend\n", backendName.c_str());
    m_window = api.createWindow(title, width, height, flags);
    if (!m_window) {
        printf("BackendManager: failed to create window\n");
        if (api.destroy) api.destroy();
        return false;
    }

    m_renderDevice = api.createRenderDevice(m_window);
    m_textRenderer = api.createTextRenderer(m_renderDevice);
    m_inputBackend = api.createInputBackend(m_window);

    m_api = api;
    s_initialized = true;
    return true;
}

bool BackendManager::initialize(const UIBackendCallbacks* callbacks) {
    if (s_initialized) return true;
    if (!callbacks || callbacks->version != 1) {
        printf("BackendManager: invalid callback table\n");
        return false;
    }

    UIWindowHandle winHandle = nullptr;
    if (callbacks->createWindow) {
        winHandle = callbacks->createWindow("UICornerstone", 1024, 768, 0);
        if (!winHandle) {
            printf("BackendManager: callback createWindow failed\n");
            return false;
        }
    }
    m_window = new CallbackWindow(callbacks, winHandle);

    UIRenderDeviceHandle rdHandle = nullptr;
    if (callbacks->createRenderDevice) {
        rdHandle = callbacks->createRenderDevice(nullptr);
        if (!rdHandle) {
            printf("BackendManager: callback createRenderDevice failed\n");
            delete m_window; m_window = nullptr;
            return false;
        }
    }
    m_renderDevice = new CallbackRenderDevice(callbacks, rdHandle);

    UITextRendererHandle trHandle = nullptr;
    if (callbacks->createTextRenderer) {
        trHandle = callbacks->createTextRenderer(rdHandle);
        if (!trHandle) {
            printf("BackendManager: callback createTextRenderer failed\n");
            delete m_window; m_window = nullptr;
            delete m_renderDevice; m_renderDevice = nullptr;
            return false;
        }
    }
    m_textRenderer = new CallbackTextRenderer(callbacks, trHandle, rdHandle);

    UIInputBackendHandle ibHandle = nullptr;
    if (callbacks->createInputBackend) {
        ibHandle = callbacks->createInputBackend(winHandle);
    }
    m_inputBackend = new CallbackInputBackend(callbacks, ibHandle);

    printf("BackendManager: initialized from callback table\n");
    s_initialized = true;
    return true;
}

void BackendManager::shutdown() {
    if (!s_initialized) return;

    delete m_textRenderer;
    m_textRenderer = nullptr;
    delete m_inputBackend;
    m_inputBackend = nullptr;

    delete m_renderDevice;
    m_renderDevice = nullptr;

    delete m_window;
    m_window = nullptr;

    if (m_api.destroy) m_api.destroy();
    m_api = {};
    s_initialized = false;
}

BackendManager::~BackendManager() {
    shutdown();
}
