#include "BackendPlugin.h"
#include <cstdio>

// ============================================================
// Static SDL3 Backend Registration
// ============================================================

static Window* sdl3CreateWindow(const char* title, int w, int h, uint32_t flags) {
    return CreateSDL3Window(title, w, h, flags);
}

static RenderDevice* sdl3CreateRenderDevice(Window* window) {
    return window->renderDevice();
}

static TextRenderer* sdl3CreateTextRenderer(RenderDevice* device) {
    return CreateSDL3TextRenderer(device);
}

static InputBackend* sdl3CreateInputBackend(Window* window) {
    return CreateSDL3InputBackend(window);
}

static void sdl3Destroy() {
}

static BackendAPI g_sdl3Backend = {
    1,
    sdl3CreateWindow,
    sdl3CreateRenderDevice,
    sdl3CreateTextRenderer,
    sdl3CreateInputBackend,
    sdl3Destroy
};

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
    if (backendName == "sdl3") {
        api = g_sdl3Backend;
    } else if (s_registeredAPI.version > 0) {
        api = s_registeredAPI;
    } else {
        printf("BackendManager: unknown backend \"%s\"\n", backendName.c_str());
        return false;
    }

    m_window = api.createWindow(title, width, height, flags);
    if (!m_window) {
        printf("BackendManager: failed to create window\n");
        return false;
    }

    m_renderDevice = api.createRenderDevice(m_window);
    m_textRenderer = api.createTextRenderer(m_renderDevice);
    m_inputBackend = api.createInputBackend(m_window);

    m_api = api;
    s_initialized = true;
    return true;
}

void BackendManager::shutdown() {
    if (!s_initialized) return;

    delete m_textRenderer;
    m_textRenderer = nullptr;
    delete m_inputBackend;
    m_inputBackend = nullptr;
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
