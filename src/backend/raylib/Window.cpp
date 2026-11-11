#include "Window.h"
#include "RenderDevice.h"
#include "RaylibCompat.h"
#include <cstdio>

// Forward declaration of the factory
RenderDevice* CreateRaylibRenderDevice();

// ============================================================
// RaylibWindow
// ============================================================
// raylib 单窗口架构（全局 CORE 只跟踪最近一次 InitWindow 的窗口）：
// 进程内仅允许一个真实窗口（第一个实例）。后续实例不 InitWindow——
// 否则新 InitWindow 覆盖 CORE 当前窗口，所有实例的渲染/输入 API 全部
// 串扰到同一窗口（多窗口内容交替闪动、先创建窗口失效）。
// 无窗口实例为"离屏逻辑态"：逻辑断言（注入式 hover/焦点/内容传递）
// 正常；渲染/输入按能力位由调用方（测试/应用）跳过。
class RaylibWindow : public Window {
public:
    RaylibWindow(const char* title, int w, int h, uint32_t flags)
        : m_hasOwnWindow(false)
    {
        if (s_windowCount == 0) {
            // Apply config flags（跨后端统一标志，值对齐 SDL_WINDOW_*）
            unsigned int rlFlags = 0;
            if (flags & UIWindowFlags::Resizable) rlFlags |= FLAG_WINDOW_RESIZABLE;
            if (flags & UIWindowFlags::Fullscreen) rlFlags |= FLAG_FULLSCREEN_MODE;
            if (flags & UIWindowFlags::Hidden) rlFlags |= FLAG_WINDOW_HIDDEN;
            if (flags & UIWindowFlags::Borderless) rlFlags |= FLAG_WINDOW_UNDECORATED;
            if (flags & UIWindowFlags::Vsync) rlFlags |= FLAG_VSYNC_HINT;
            if (rlFlags) SetConfigFlags(rlFlags);

            InitWindow(w, h, title);
            SetTraceLogLevel(LOG_WARNING);  // Suppress raylib's INFO spam (font/texture load logs)
            SetExitKey(0);                  // Don't let ESC close the window — UICornerstone handles it
            // SetTargetFPS is NOT set — present() does its own 60 Hz timing.
            m_hasOwnWindow = true;
        }
        s_windowCount++;

        m_renderDevice = CreateRaylibRenderDevice();
        if (m_hasOwnWindow) m_visible = !IsWindowHidden();
    }

    ~RaylibWindow() override {
        // Render device is owned and deleted by BackendManager
        // (与 SFML/SDL3 所有权约定一致，避免 double delete)
        m_renderDevice = nullptr;
        // raylib 是单窗口架构：全局 CORE 只跟踪最近一次 InitWindow 的窗口。
        // 用 IsWindowReady() 守卫避免窗口已关闭时二次 CloseWindow 崩溃。
        if (m_hasOwnWindow && IsWindowReady()) CloseWindow();
        if (s_windowCount > 0) s_windowCount--;
    }

    SSize getSize() const override {
        return SSize(static_cast<float>(m_hasOwnWindow ? GetScreenWidth() : 0),
                     static_cast<float>(m_hasOwnWindow ? GetScreenHeight() : 0));
    }

    SPoint getPosition() const override {
        if (!m_hasOwnWindow) return SPoint(0.0f, 0.0f);
        Vector2 pos = GetWindowPosition();
        return SPoint(pos.x, pos.y);
    }

    float getDisplayWidth() const override {
        if (!m_hasOwnWindow) return 0.0f;
        int monitor = GetCurrentMonitor();
        return static_cast<float>(GetMonitorWidth(monitor));
    }

    float getDisplayHeight() const override {
        if (!m_hasOwnWindow) return 0.0f;
        int monitor = GetCurrentMonitor();
        return static_cast<float>(GetMonitorHeight(monitor));
    }

    float getDpiScale() const override {
        if (!m_hasOwnWindow) return 1.0f;
        Vector2 dpi = GetWindowScaleDPI();
        return dpi.x;
    }

    void setTitle(const std::string& title) override {
        if (m_hasOwnWindow) SetWindowTitle(title.c_str());
    }

    void* nativeHandle() override {
        return nullptr;
    }

    RenderDevice* renderDevice() override {
        return m_renderDevice;
    }

    bool isHeadless() const override { return !m_hasOwnWindow; }

    bool getMousePosition(float& x, float& y) override {
        // 无窗口实例（非主实例）不读全局鼠标状态——CORE 只反映主实例
        // 窗口，读取会串扰主实例输入。
        if (!m_hasOwnWindow) return false;
        Vector2 pos = GetMousePosition();
        x = pos.x;
        y = pos.y;
        return true;
    }

    bool setVisible(bool visible) override {
        if (!m_hasOwnWindow) return false;
        if (visible) ClearWindowState(FLAG_WINDOW_HIDDEN);
        else SetWindowState(FLAG_WINDOW_HIDDEN);
        m_visible = !IsWindowHidden();
        return m_visible == visible;
    }
    bool isVisible() const override { return m_hasOwnWindow ? m_visible : false; }
    bool setAlwaysOnTop(bool onTop) override {
        if (!m_hasOwnWindow) return false;
        if (onTop) SetWindowState(FLAG_WINDOW_TOPMOST); else ClearWindowState(FLAG_WINDOW_TOPMOST);
        m_alwaysOnTop = IsWindowState(FLAG_WINDOW_TOPMOST);
        return m_alwaysOnTop == onTop;
    }
    bool isAlwaysOnTop() const override { return m_hasOwnWindow ? m_alwaysOnTop : false; }
    bool setForeground() override { if (!m_hasOwnWindow) return false; SetWindowFocused(); return true; }

    void setResizable(bool resizable) override {
        if (!m_hasOwnWindow) return;
        if (resizable)
            SetWindowState(FLAG_WINDOW_RESIZABLE);
        else
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
    }

    void setSize(int width, int height) override {
        // headless 实例（多窗口后端的多实例非首实例）无真实窗口：空操作
        if (!m_hasOwnWindow) return;
        SetWindowSize(width, height);
    }

    void onResized(int width, int height) override {
        // raylib handles internal resize automatically via GLFW callback
        (void)width;
        (void)height;
    }

private:
    bool m_visible = true;
    bool m_alwaysOnTop = false;
    static int s_windowCount;   // 进程级真实窗口计数（单线程创建）
    bool m_hasOwnWindow;
    RenderDevice* m_renderDevice;
};

int RaylibWindow::s_windowCount = 0;

// ============================================================
// Factory entry point
// ============================================================
Window* CreateRaylibWindow(const char* title, int w, int h, uint32_t flags) {
    if (flags & UIWindowFlags::NoShadow)
        printf("raylib NoShadow unsupported (ignored)\n");
    if (flags & UIWindowFlags::Utility)
        printf("raylib Utility unsupported (ignored)\n");
    flags &= ~(UIWindowFlags::NoShadow | UIWindowFlags::Utility);   // 应用层位不得传给 raylib
    return new RaylibWindow(title, w, h, flags);
}
