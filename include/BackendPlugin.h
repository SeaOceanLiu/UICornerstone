#ifndef BackendPluginH
#define BackendPluginH

#include <string>
#include "Window.h"
#include "RenderDevice.h"
#include "TextRenderer.h"
#include "InputBackend.h"

// BackendAPI - C ABI compatible function table for backend plugin DLLs.
// Each backend DLL exports a GetBackendAPI() function returning a pointer
// to a static BackendAPI instance.
struct BackendAPI {
    unsigned version;
    bool (*init)();
    Window* (*createWindow)(const char* title, int w, int h, uint32_t flags);
    RenderDevice* (*createRenderDevice)(Window* window);
    TextRenderer* (*createTextRenderer)(RenderDevice* device);
    InputBackend* (*createInputBackend)(Window* window);
    void (*destroy)();
};

// BackendManager - singleton that owns the backend lifecycle.
// In this phase the SDL3 backend is linked statically; dynamic DLL loading
// will be added in a later phase.
class BackendManager {
public:
    static BackendManager* instance();

    bool initialize(const std::string& backendName = "sdl3",
                    const char* title = "UICornerstone",
                    int width = 1024, int height = 768, uint32_t flags = 0);
    void shutdown();

    Window* window() const { return m_window; }
    RenderDevice* renderDevice() const { return m_renderDevice; }
    TextRenderer* textRenderer() const { return m_textRenderer; }
    InputBackend* inputBackend() const { return m_inputBackend; }

    // Register a backend statically (called from backend init functions)
    static void registerBackend(const BackendAPI& api);

private:
    BackendManager() = default;
    ~BackendManager();

    BackendAPI m_api = {};
    Window* m_window = nullptr;
    RenderDevice* m_renderDevice = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    InputBackend* m_inputBackend = nullptr;

    static BackendAPI s_registeredAPI;
    static bool s_initialized;
};

#endif // BackendPluginH
