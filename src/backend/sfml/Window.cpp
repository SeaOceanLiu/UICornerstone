#include "Window.h"
#include "RenderDevice.h"
#include <SFML/Graphics.hpp>
#include <windows.h>

extern RenderDevice* CreateSFMLRenderDevice(sf::RenderWindow* window);
extern void SFMLSetCursorWindow(sf::Window* window);

class SFMLWindow : public Window {
public:
    SFMLWindow(sf::RenderWindow* window)
        : m_window(window)
        , m_renderDevice(CreateSFMLRenderDevice(window))
    {
        SFMLSetCursorWindow(window);
        auto mode = sf::VideoMode::getDesktopMode();
        m_displayWidth = static_cast<float>(mode.size.x);
        m_displayHeight = static_cast<float>(mode.size.y);
        auto s = window->getSize();
        window->setView(sf::View(sf::FloatRect({0, 0}, {(float)s.x, (float)s.y})));
    }

    ~SFMLWindow() override {
        // Render device is owned and deleted by BackendManager
        // (SFMLWindow 内部的 m_renderDevice 与 BackendManager 持有的是同一对象，
        // 此处仅清指针，避免 double delete；与 SDL3 后端所有权约定一致)
        m_renderDevice = nullptr;
        delete m_window;
    }

    SSize getSize() const override {
        if (!m_window) return SSize(0, 0);
        auto s = m_window->getSize();
        return SSize(static_cast<float>(s.x), static_cast<float>(s.y));
    }

    SPoint getPosition() const override {
        if (!m_window) return SPoint(0, 0);
        auto pos = m_window->getPosition();
        return SPoint(static_cast<float>(pos.x), static_cast<float>(pos.y));
    }

    float getDisplayWidth() const override { return m_displayWidth; }
    float getDisplayHeight() const override { return m_displayHeight; }

    float getDpiScale() const override {
        HWND hwnd = m_window->getNativeHandle();
        if (!hwnd) return 1.0f;
        UINT dpi = GetDpiForWindow(hwnd);
        return (dpi > 0) ? (static_cast<float>(dpi) / 96.0f) : 1.0f;
    }

    void setTitle(const std::string& title) override {
        if (m_window) m_window->setTitle(sf::String::fromUtf8(title.begin(), title.end()));
    }

    void setSize(int width, int height) override {
        if (m_window) m_window->setSize(sf::Vector2u(
            width > 0 ? (unsigned)width : 0u,
            height > 0 ? (unsigned)height : 0u));
    }
    bool setVisible(bool visible) override {
        if (!m_window) return false;
        m_window->setVisible(visible);
        m_visible = visible;
        return true;
    }
    bool isVisible() const override { return m_visible; }
    bool setAlwaysOnTop(bool) override { return false; }
    bool isAlwaysOnTop() const override { return false; }
    bool setForeground() override { if (!m_window) return false; m_window->requestFocus(); return true; }

    void* nativeHandle() override { return m_window; }

    RenderDevice* renderDevice() override { return m_renderDevice; }

    bool getMousePosition(float& x, float& y) override {
        if (!m_window) return false;
        auto pos = sf::Mouse::getPosition(*m_window);
        x = static_cast<float>(pos.x);
        y = static_cast<float>(pos.y);
        return true;
    }

    void onResized(int width, int height) override {
        if (m_window) {
            m_window->setView(sf::View(sf::FloatRect({0, 0}, {(float)width, (float)height})));
        }
    }

    sf::RenderWindow* getSFMLWindow() const { return m_window; }

private:
    sf::RenderWindow* m_window;
    RenderDevice* m_renderDevice;
    float m_displayWidth = 0;
    float m_displayHeight = 0;
    bool m_visible = true;
};

Window* CreateSFMLWindow(const char* title, int width, int height, uint32_t flags) {
    if (flags & UIWindowFlags::NoShadow)
        printf("SFML NoShadow unsupported (ignored)\n");
    if (flags & UIWindowFlags::Utility)
        printf("SFML Utility unsupported (ignored)\n");
    flags &= ~(UIWindowFlags::NoShadow | UIWindowFlags::Utility);   // 应用层位不得传给 SFML
    // 跨后端统一窗口标志（UIWindowFlags）：显式组合样式，
    // 无 Resizable 标志时不带可调边框（sf::Style::Default 恒含 Resize）
    std::uint32_t style = (flags & UIWindowFlags::Borderless)
        ? sf::Style::None
        : (sf::Style::Titlebar | sf::Style::Close);
    if (!(flags & UIWindowFlags::Borderless) && (flags & UIWindowFlags::Resizable)) style |= sf::Style::Resize;
    sf::State winState = sf::State::Windowed;
    if (flags & UIWindowFlags::Fullscreen) winState = sf::State::Fullscreen;

    sf::ContextSettings ctxSettings;
    ctxSettings.stencilBits = 8;
    auto* window = new sf::RenderWindow(
        sf::VideoMode(sf::Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height))),
        title ? sf::String::fromUtf8(title, title + strlen(title)) : sf::String("UICornerstone"),
        style,
        winState,
        ctxSettings
    );
    window->setVerticalSyncEnabled(false);
    window->setActive(true);  // Keep OpenGL context active for this thread
    auto* result = new SFMLWindow(window);
    if (flags & UIWindowFlags::Hidden) result->setVisible(false);   // SFML 无创建期 style：构造后立即隐藏
    return result;
}
