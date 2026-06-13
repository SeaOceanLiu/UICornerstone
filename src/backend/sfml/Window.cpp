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
        delete m_renderDevice;
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
};

Window* CreateSFMLWindow(const char* title, int width, int height, uint32_t flags) {
    std::uint32_t style = sf::Style::Default;
    sf::State winState = sf::State::Windowed;
    if (flags & 0x01) winState = sf::State::Fullscreen;

    sf::ContextSettings ctxSettings;
    ctxSettings.stencilBits = 8;
    auto* window = new sf::RenderWindow(
        sf::VideoMode(sf::Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height))),
        title ? sf::String::fromUtf8(title, title + strlen(title)) : sf::String("UICornerstone"),
        style,
        winState,
        ctxSettings
    );
    return new SFMLWindow(window);
}
