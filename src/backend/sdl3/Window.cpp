#include "Window.h"
#include "RenderDevice.h"
#include <SDL3/SDL.h>

// ============================================================
// SDL3Window
// ============================================================
class SDL3Window : public Window {
public:
    SDL3Window(SDL_Window* window, SDL_Renderer* renderer)
        : m_window(window)
        , m_renderer(renderer)
        , m_renderDevice(CreateSDL3RenderDevice(renderer))
    {
        SDL_DisplayID displayId = SDL_GetPrimaryDisplay();
        if (displayId != 0) {
            const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayId);
            if (mode) {
                m_displayWidth = (float)mode->w * mode->pixel_density;
                m_displayHeight = (float)mode->h * mode->pixel_density;
            }
        }
    }

    ~SDL3Window() override {
        // Render device is owned and deleted by BackendManager
        m_renderDevice = nullptr;
        if (m_renderer) SDL_DestroyRenderer(m_renderer);
        if (m_window) SDL_DestroyWindow(m_window);
    }

    SSize getSize() const override {
        int w = 0, h = 0;
        if (m_window) SDL_GetWindowSize(m_window, &w, &h);
        return SSize{(float)w, (float)h};
    }

    SPoint getPosition() const override {
        int x = 0, y = 0;
        if (m_window) SDL_GetWindowPosition(m_window, &x, &y);
        return SPoint{(float)x, (float)y};
    }

    float getDisplayWidth() const override { return m_displayWidth; }
    float getDisplayHeight() const override { return m_displayHeight; }

    float getDpiScale() const override {
        if (!m_window) return 1.0f;
        SDL_DisplayID displayId = SDL_GetDisplayForWindow(m_window);
        if (displayId == 0) return 1.0f;
        float scale = SDL_GetDisplayContentScale(displayId);
        return (scale > 0.0f) ? scale : 1.0f;
    }

    void setTitle(const std::string& title) override {
        if (m_window) SDL_SetWindowTitle(m_window, title.c_str());
    }

    void* nativeHandle() override { return (void*)m_window; }

    RenderDevice* renderDevice() override { return m_renderDevice; }

    bool getMousePosition(float& x, float& y) override {
        if (!m_window) return false;
        SDL_GetMouseState(&x, &y);
        return true;
    }

    SDL_Window* getSDLWindow() const { return m_window; }
    SDL_Renderer* getSDLRenderer() const { return m_renderer; }

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    RenderDevice* m_renderDevice;
    float m_displayWidth = 0;
    float m_displayHeight = 0;
};

// ============================================================
// Factory Function
// ============================================================
Window* CreateSDL3Window(const char* title, int width, int height, uint32_t flags) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer(title, width, height, flags, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return nullptr;
    }
    SDL_SetRenderVSync(renderer, 0);
    return new SDL3Window(window, renderer);
}
