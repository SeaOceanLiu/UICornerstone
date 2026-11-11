#include "Window.h"
#include "RenderDevice.h"
#include <SDL3/SDL.h>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

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
        m_visible = window && (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0;
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

    void setResizable(bool resizable) override {
        if (m_window) SDL_SetWindowResizable(m_window, resizable);
    }
    bool setVisible(bool visible) override {
        if (!m_window) return false;
        if (visible) SDL_ShowWindow(m_window); else SDL_HideWindow(m_window);
        m_visible = visible;
        return true;
    }

    bool isVisible() const override { return m_visible; }
    bool setAlwaysOnTop(bool onTop) override {
        if (!m_window) return false;
        SDL_SetWindowAlwaysOnTop(m_window, onTop);
        m_alwaysOnTop = onTop;
        return true;
    }
    bool isAlwaysOnTop() const override { return m_alwaysOnTop; }
    bool setForeground() override {
        if (!m_window) return false;
        SDL_RaiseWindow(m_window);
        return true;
    }

    void setSize(int width, int height) override {
        if (m_window) SDL_SetWindowSize(m_window, width, height);
    }

    bool getMousePosition(float& x, float& y) override {
        if (!m_window) return false;
        // SDL_GetMouseState 返回的是鼠标相对"当前鼠标焦点窗口"的坐标：
        // 鼠标在另一窗口（多实例）时拿到的坐标属于别的窗口，会造成 hover
        // 串扰。须用全局坐标判断鼠标是否在本窗口内，不在则返回 false
        // （调用方按"鼠标不在本窗口"处理，清除 hover 状态）。
        float gx, gy;
        SDL_GetGlobalMouseState(&gx, &gy);
        int wx, wy;
        SDL_GetWindowPosition(m_window, &wx, &wy);
        int ww, wh;
        SDL_GetWindowSize(m_window, &ww, &wh);
        if (gx < wx || gx >= wx + ww || gy < wy || gy >= wy + wh)
            return false;
        x = gx - wx;
        y = gy - wy;
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
    bool m_visible = true;
    bool m_alwaysOnTop = false;
};

// CR20261109001：SDL3 Windows Utility fallback（EXSTYLE）。必须在首次 Show 前调用。
// 返回 true=已具备/成功，false=失败但调用方仍继续创建/显示。
static bool applyUtilityFallback(SDL_Window* window) {
#ifdef _WIN32
    if (!window) return false;
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hwnd) {
        fprintf(stderr, "SDL3 Utility: HWND unavailable (best-effort skipped)\n");
        return false;
    }
    SetLastError(0);
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    DWORD getErr = GetLastError();
    if (style == 0 && getErr != 0) {
        fprintf(stderr, "SDL3 Utility: GetWindowLongPtr failed (err=%lu, best-effort)\n", (unsigned long)getErr);
        return false;
    }
    if (style & WS_EX_TOOLWINDOW) {
        fprintf(stderr, "SDL3 Utility: WS_EX_TOOLWINDOW already set (fallback skipped)\n");
        return true;
    }
    SetLastError(0);
    LONG_PTR previous = SetWindowLongPtrW(hwnd, GWL_EXSTYLE, style | WS_EX_TOOLWINDOW);
    DWORD setErr = GetLastError();
    if (previous == 0 && setErr != 0) {
        fprintf(stderr, "SDL3 Utility: SetWindowLongPtr failed (err=%lu, best-effort)\n", (unsigned long)setErr);
        return false;
    }
    if (!SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                      SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        fprintf(stderr, "SDL3 Utility: SetWindowPos refresh failed (err=%lu, style applied)\n", (unsigned long)GetLastError());
        return false;
    }
    fprintf(stderr, "SDL3 Utility: WS_EX_TOOLWINDOW applied (EXSTYLE 0x%llx -> 0x%llx)\n",
            (unsigned long long)style, (unsigned long long)(style | WS_EX_TOOLWINDOW));
    return true;
#else
    (void)window;
    return false;
#endif
}

// CR20261108001：SDL3 Windows best-effort 无阴影；必须在首次显示前调用。
static void applyNoShadow(SDL_Window* window) {
#ifdef _WIN32
    if (!window) return;
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hwnd) {
        fprintf(stderr, "SDL3 NoShadow: HWND unavailable (best-effort skipped)\n");
        return;
    }
    DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy))))
        fprintf(stderr, "SDL3 NoShadow: DwmSetWindowAttribute failed (best-effort)\n");
    MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    LONG_PTR style = GetClassLongPtr(hwnd, GCL_STYLE);
    if (style & CS_DROPSHADOW) SetClassLongPtr(hwnd, GCL_STYLE, style & ~CS_DROPSHADOW);
#else
    (void)window;
#endif
}

// ============================================================
// Factory Function
// ============================================================
Window* CreateSDL3Window(const char* title, int width, int height, uint32_t flags) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    // flags 为跨后端统一窗口标志（UIWindowFlags，值对齐 SDL_WINDOW_*，可直接透传）。
    // 注意：Vsync 是应用层保留位（raylib 创建期专用），不属 SDL 窗口标志，须掩掉。
    const bool noShadow = (flags & UIWindowFlags::NoShadow) != 0;
    const bool utility = (flags & UIWindowFlags::Utility) != 0;
    const bool explicitlyHidden = (flags & UIWindowFlags::Hidden) != 0;
    // NoShadow 是应用层位，绝不传给 SDL；Utility 是 SDL 原生位，保留透传。
    // Utility/NoShadow 需要首次显示前处理：无显式 Hidden 时先隐藏创建，处理后再显示。
    const bool needDeferredShow = (utility || noShadow) && !explicitlyHidden;
    uint32_t sdlFlags = flags & ~(UIWindowFlags::Vsync | UIWindowFlags::NoShadow);
    if (needDeferredShow) sdlFlags |= UIWindowFlags::Hidden;
    if (!SDL_CreateWindowAndRenderer(title, width, height, sdlFlags, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return nullptr;
    }
    // VSync 退出默认关闭（开启与否由后端配置入口控制，不硬编码）
    SDL_SetRenderVSync(renderer, 0);
    if (utility) applyUtilityFallback(window);   // 先 EXSTYLE
    if (noShadow) applyNoShadow(window);          // 后 DWM/region
    if (needDeferredShow) SDL_ShowWindow(window);
    return new SDL3Window(window, renderer);
}

Window* CreateSDL3WindowFromExisting(void* nativeWindow, void* nativeRenderer) {
    return new SDL3Window(static_cast<SDL_Window*>(nativeWindow),
                          static_cast<SDL_Renderer*>(nativeRenderer));
}
