#include "Window.h"
#include "RenderDevice.h"
#include "RaylibCompat.h"
#include <cstdio>

// Forward declaration of the factory
RenderDevice* CreateRaylibRenderDevice();

// ============================================================
// RaylibWindow
// ============================================================
class RaylibWindow : public Window {
public:
    RaylibWindow(const char* title, int w, int h, uint32_t flags)
    {
        // Apply config flags
        unsigned int rlFlags = 0;
        // 0x00000020 = SDL_WINDOW_RESIZABLE → FLAG_WINDOW_RESIZABLE
        if (flags & 0x00000020) rlFlags |= FLAG_WINDOW_RESIZABLE;
        if (rlFlags) SetConfigFlags(rlFlags);

        InitWindow(w, h, title);
        SetTraceLogLevel(LOG_WARNING);  // Suppress raylib's INFO spam (font/texture load logs)
        // SetTargetFPS is NOT set — present() does its own 60 Hz timing.

        m_renderDevice = CreateRaylibRenderDevice();
    }

    ~RaylibWindow() override {
        delete m_renderDevice;
        CloseWindow();
    }

    SSize getSize() const override {
        return SSize(static_cast<float>(GetScreenWidth()),
                     static_cast<float>(GetScreenHeight()));
    }

    SPoint getPosition() const override {
        Vector2 pos = GetWindowPosition();
        return SPoint(pos.x, pos.y);
    }

    float getDisplayWidth() const override {
        int monitor = GetCurrentMonitor();
        return static_cast<float>(GetMonitorWidth(monitor));
    }

    float getDisplayHeight() const override {
        int monitor = GetCurrentMonitor();
        return static_cast<float>(GetMonitorHeight(monitor));
    }

    float getDpiScale() const override {
        Vector2 dpi = GetWindowScaleDPI();
        return dpi.x;
    }

    void setTitle(const std::string& title) override {
        SetWindowTitle(title.c_str());
    }

    void* nativeHandle() override {
        return nullptr;
    }

    RenderDevice* renderDevice() override {
        return m_renderDevice;
    }

    bool getMousePosition(float& x, float& y) override {
        Vector2 pos = GetMousePosition();
        x = pos.x;
        y = pos.y;
        return true;
    }

    void setResizable(bool resizable) override {
        if (resizable)
            SetWindowState(FLAG_WINDOW_RESIZABLE);
        else
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
    }

    void onResized(int width, int height) override {
        // raylib handles internal resize automatically via GLFW callback
        (void)width;
        (void)height;
    }

private:
    RenderDevice* m_renderDevice;
};

// ============================================================
// Factory entry point
// ============================================================
Window* CreateRaylibWindow(const char* title, int w, int h, uint32_t flags) {
    return new RaylibWindow(title, w, h, flags);
}
