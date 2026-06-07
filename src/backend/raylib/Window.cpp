#include "Window.h"
#include "RenderDevice.h"
#include "RaylibCompat.h"
#include <cstdio>

// ============================================================
// RaylibWindow
// ============================================================
class RaylibWindow : public Window {
public:
    RaylibWindow(const char* title, int w, int h, uint32_t flags)
        : m_renderDevice(nullptr)
    {
        // Apply config flags
        unsigned int rlFlags = 0;
        if (flags & 0x0001) rlFlags |= FLAG_WINDOW_RESIZABLE;
        if (flags & 0x0002) rlFlags |= FLAG_FULLSCREEN_MODE;
        if (flags & 0x0004) rlFlags |= FLAG_WINDOW_HIGHDPI;
        if (flags & 0x0008) rlFlags |= FLAG_VSYNC_HINT;
        if (rlFlags) SetConfigFlags(rlFlags);

        InitWindow(w, h, title);
        SetTargetFPS(60);
    }

    ~RaylibWindow() override {
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
