#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <cstring>

#include "InputBackend.h"
#include "Window.h"
#include "StateMachine.h"
#include "EventQueue.h"
#include "EditBox.h"
#include "TextArea.h"

static KeyCode SFMLKeycodeToKeyCode(sf::Keyboard::Key sfmlKey) {
    // A-Z (consecutive in SFML enum)
    int sfmlVal = static_cast<int>(sfmlKey);
    int aVal = static_cast<int>(sf::Keyboard::Key::A);
    int zVal = static_cast<int>(sf::Keyboard::Key::Z);
    if (sfmlVal >= aVal && sfmlVal <= zVal) {
        return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (sfmlVal - aVal));
    }

    // Num0-Num9 (consecutive in SFML enum)
    int num0Val = static_cast<int>(sf::Keyboard::Key::Num0);
    int num9Val = static_cast<int>(sf::Keyboard::Key::Num9);
    if (sfmlVal >= num0Val && sfmlVal <= num9Val) {
        return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (sfmlVal - num0Val));
    }

    switch (sfmlKey) {
        case sf::Keyboard::Key::Enter:     return KeyCode::Return;
        case sf::Keyboard::Key::Escape:    return KeyCode::Escape;
        case sf::Keyboard::Key::Backspace: return KeyCode::Backspace;
        case sf::Keyboard::Key::Tab:       return KeyCode::Tab;
        case sf::Keyboard::Key::Delete:    return KeyCode::Del;
        case sf::Keyboard::Key::Space:     return KeyCode::Space;

        case sf::Keyboard::Key::Left:  return KeyCode::Left;
        case sf::Keyboard::Key::Right: return KeyCode::Right;
        case sf::Keyboard::Key::Up:    return KeyCode::Up;
        case sf::Keyboard::Key::Down:  return KeyCode::Down;
        case sf::Keyboard::Key::Home:  return KeyCode::Home;
        case sf::Keyboard::Key::End:   return KeyCode::End;
        case sf::Keyboard::Key::PageUp:   return KeyCode::PageUp;
        case sf::Keyboard::Key::PageDown: return KeyCode::PageDown;
        case sf::Keyboard::Key::Insert:   return KeyCode::Insert;

        case sf::Keyboard::Key::F1:  return KeyCode::F1;
        case sf::Keyboard::Key::F2:  return KeyCode::F2;
        case sf::Keyboard::Key::F3:  return KeyCode::F3;
        case sf::Keyboard::Key::F4:  return KeyCode::F4;
        case sf::Keyboard::Key::F5:  return KeyCode::F5;
        case sf::Keyboard::Key::F6:  return KeyCode::F6;
        case sf::Keyboard::Key::F7:  return KeyCode::F7;
        case sf::Keyboard::Key::F8:  return KeyCode::F8;
        case sf::Keyboard::Key::F9:  return KeyCode::F9;
        case sf::Keyboard::Key::F10: return KeyCode::F10;
        case sf::Keyboard::Key::F11: return KeyCode::F11;
        case sf::Keyboard::Key::F12: return KeyCode::F12;
        case sf::Keyboard::Key::F13: return KeyCode::F13;
        case sf::Keyboard::Key::F14: return KeyCode::F14;
        case sf::Keyboard::Key::F15: return KeyCode::F15;

        case sf::Keyboard::Key::LShift: return KeyCode::LShift;
        case sf::Keyboard::Key::RShift: return KeyCode::RShift;
        case sf::Keyboard::Key::LControl:  return KeyCode::LCtrl;
        case sf::Keyboard::Key::RControl:  return KeyCode::RCtrl;
        case sf::Keyboard::Key::LAlt:   return KeyCode::LAlt;
        case sf::Keyboard::Key::RAlt:   return KeyCode::RAlt;
        case sf::Keyboard::Key::LSystem:  return KeyCode::LGui;
        case sf::Keyboard::Key::RSystem:  return KeyCode::RGui;

        case sf::Keyboard::Key::Comma:     return KeyCode::Comma;
        case sf::Keyboard::Key::Period:    return KeyCode::Period;
        case sf::Keyboard::Key::Slash:     return KeyCode::Slash;
        case sf::Keyboard::Key::Semicolon: return KeyCode::Semicolon;
        case sf::Keyboard::Key::Apostrophe: return KeyCode::Apostrophe;
        case sf::Keyboard::Key::LBracket:  return KeyCode::LeftBracket;
        case sf::Keyboard::Key::RBracket:  return KeyCode::RightBracket;
        case sf::Keyboard::Key::Hyphen:    return KeyCode::Minus;
        case sf::Keyboard::Key::Equal:     return KeyCode::Equals;

        case sf::Keyboard::Key::Add:          return KeyCode::KPPlus;
        case sf::Keyboard::Key::Subtract:     return KeyCode::KPMinus;
        case sf::Keyboard::Key::Multiply:     return KeyCode::KPMultiply;
        case sf::Keyboard::Key::Divide:       return KeyCode::KPDivide;
        case sf::Keyboard::Key::Pause:        return KeyCode::Pause;

        default: return KeyCode::Unknown;
    }
}

static KeyMod SFMLKeymodToKeyMod(bool ctrl, bool alt, bool shift, bool system) {
    KeyMod mod = KeyMod::None;
    if (shift) {
        mod = mod | KeyMod::Shift;
    }
    if (ctrl) {
        mod = mod | KeyMod::Ctrl;
    }
    if (alt) {
        mod = mod | KeyMod::Alt;
    }
    if (system) {
        mod = mod | KeyMod::Gui;
    }
    return mod;
}

class SFMLInputBackend : public InputBackend {
public:
    SFMLInputBackend(sf::Window* window)
        : m_window(window)
    {
    }

    void startTextInput() override { m_textInputActive = true; }
    void stopTextInput() override { m_textInputActive = false; }
    bool isTextInputActive() const override { return m_textInputActive; }

    void setClipboardText(const std::string& text) override {
        sf::Clipboard::setString(sf::String::fromUtf8(text.begin(), text.end()));
    }

    std::string getClipboardText() const override {
        auto str = sf::Clipboard::getString();
        auto utf8 = str.toUtf8();
        return std::string(utf8.begin(), utf8.end());
    }

    bool hasScreenKeyboard() const override { return false; }

    bool pollEvent(Event& event) override {
        if (!m_window) return false;

        auto optEvent = m_window->pollEvent();
        if (!optEvent) {
            return false;
        }

        const auto& sfmlEvent = *optEvent;

        event.m_eventName = static_cast<EventName>(0);
        event.m_eventParam = {};
        event.m_type = EventType::None;
        event._pad = 0;

        if (sfmlEvent.is<sf::Event::Closed>()) {
            event.m_type = EventType::WindowClose;
            event.m_eventName = EventName::Exit;
        }
        else if (const auto* resized = sfmlEvent.getIf<sf::Event::Resized>()) {
            event.m_type = EventType::WindowResize;
            event.resizeEvent.width = static_cast<int>(resized->size.x);
            event.resizeEvent.height = static_cast<int>(resized->size.y);
        }
        else if (const auto* keyPressed = sfmlEvent.getIf<sf::Event::KeyPressed>()) {
            KeyEventData keyData;
            keyData.keycode = SFMLKeycodeToKeyCode(keyPressed->code);
            keyData.scancode = static_cast<int32_t>(0);
            keyData.mod = SFMLKeymodToKeyMod(keyPressed->control, keyPressed->alt, keyPressed->shift, keyPressed->system);
            keyData.repeat = false;

            event.m_type = EventType::KeyDown;
            event.keyEvent.keycode = keyData.keycode;
            event.keyEvent.mod = keyData.mod;
            event.keyEvent.scancode = keyData.scancode;
            event.keyEvent.repeat = keyData.repeat;
            event.m_eventName = EventName::KEY_DOWN;
            event.m_eventParam = keyData;
        }
        else if (const auto* keyReleased = sfmlEvent.getIf<sf::Event::KeyReleased>()) {
            KeyEventData keyData;
            keyData.keycode = SFMLKeycodeToKeyCode(keyReleased->code);
            keyData.scancode = static_cast<int32_t>(0);
            keyData.mod = SFMLKeymodToKeyMod(keyReleased->control, keyReleased->alt, keyReleased->shift, keyReleased->system);
            keyData.repeat = false;

            event.m_type = EventType::KeyUp;
            event.keyEvent.keycode = keyData.keycode;
            event.keyEvent.mod = keyData.mod;
            event.keyEvent.scancode = keyData.scancode;
            event.keyEvent.repeat = keyData.repeat;
            event.m_eventName = EventName::KEY_UP;
            event.m_eventParam = keyData;
        }
        else if (const auto* textEntered = sfmlEvent.getIf<sf::Event::TextEntered>()) {
            TextInputEventData textData;
            char32_t unicode = textEntered->unicode;
            char utf8[8] = {};
            if (unicode < 0x80) {
                utf8[0] = static_cast<char>(unicode);
            } else if (unicode < 0x800) {
                utf8[0] = static_cast<char>(0xC0 | (unicode >> 6));
                utf8[1] = static_cast<char>(0x80 | (unicode & 0x3F));
            } else if (unicode < 0x10000) {
                utf8[0] = static_cast<char>(0xE0 | (unicode >> 12));
                utf8[1] = static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                utf8[2] = static_cast<char>(0x80 | (unicode & 0x3F));
            } else {
                utf8[0] = static_cast<char>(0xF0 | (unicode >> 18));
                utf8[1] = static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
                utf8[2] = static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                utf8[3] = static_cast<char>(0x80 | (unicode & 0x3F));
            }
            textData.text = utf8;
            textData.start = -1;
            textData.length = -1;

            event.m_type = EventType::TextInput;
            std::memset(event.textInput.text, 0, sizeof(event.textInput.text));
            std::strncpy(event.textInput.text, utf8, sizeof(event.textInput.text) - 1);
            event.m_eventName = EventName::TEXT_INPUT;
            event.m_eventParam = textData;
        }
        else if (const auto* scrolled = sfmlEvent.getIf<sf::Event::MouseWheelScrolled>()) {
            MouseWheelEventData wheelData;
            wheelData.x = static_cast<float>(scrolled->position.x);
            wheelData.y = static_cast<float>(scrolled->position.y);
            wheelData.mouseX = static_cast<float>(scrolled->position.x);
            wheelData.mouseY = static_cast<float>(scrolled->position.y);

            event.m_type = EventType::MouseWheel;
            event.mouseWheel.x = static_cast<float>(scrolled->position.x);
            event.mouseWheel.y = static_cast<float>(scrolled->position.y);
            event.mouseWheel.scrollX = 0;
            event.mouseWheel.scrollY = scrolled->delta * 120.0f;
            event.m_eventName = EventName::MOUSE_WHEEL;
            event.m_eventParam = wheelData;
        }
        else if (const auto* moved = sfmlEvent.getIf<sf::Event::MouseMoved>()) {
            auto mousePos = std::make_shared<SPoint>(
                static_cast<float>(moved->position.x),
                static_cast<float>(moved->position.y));

            event.m_type = EventType::MouseMove;
            event.mousePos.x = static_cast<float>(moved->position.x);
            event.mousePos.y = static_cast<float>(moved->position.y);
            event.m_eventName = EventName::MOUSE_MOVING;
            event.m_eventParam = mousePos;
        }
        else if (const auto* btnPressed = sfmlEvent.getIf<sf::Event::MouseButtonPressed>()) {
            auto mousePos = std::make_shared<SPoint>(
                static_cast<float>(btnPressed->position.x),
                static_cast<float>(btnPressed->position.y));

            MouseButton btn = MouseButton::None;
            switch (btnPressed->button) {
                case sf::Mouse::Button::Left:   btn = MouseButton::Left; break;
                case sf::Mouse::Button::Right:  btn = MouseButton::Right; break;
                case sf::Mouse::Button::Middle: btn = MouseButton::Middle; break;
                case sf::Mouse::Button::Extra1: btn = MouseButton::X1; break;
                case sf::Mouse::Button::Extra2: btn = MouseButton::X2; break;
                default: break;
            }

            event.m_type = EventType::MouseDown;
            event.mouseButton.x = static_cast<float>(btnPressed->position.x);
            event.mouseButton.y = static_cast<float>(btnPressed->position.y);
            event.mouseButton.button = btn;
            switch (btnPressed->button) {
                case sf::Mouse::Button::Left:   event.m_eventName = EventName::MOUSE_LBUTTON_DOWN; break;
                case sf::Mouse::Button::Right:  event.m_eventName = EventName::MOUSE_RBUTTON_DOWN; break;
                case sf::Mouse::Button::Middle: event.m_eventName = EventName::MOUSE_MBUTTON_DOWN; break;
                default:                         event.m_eventName = EventName::MOUSE_LBUTTON_DOWN; break;
            }
            event.m_eventParam = mousePos;
        }
        else if (const auto* btnReleased = sfmlEvent.getIf<sf::Event::MouseButtonReleased>()) {
            auto mousePos = std::make_shared<SPoint>(
                static_cast<float>(btnReleased->position.x),
                static_cast<float>(btnReleased->position.y));

            MouseButton btn = MouseButton::None;
            switch (btnReleased->button) {
                case sf::Mouse::Button::Left:   btn = MouseButton::Left; break;
                case sf::Mouse::Button::Right:  btn = MouseButton::Right; break;
                case sf::Mouse::Button::Middle: btn = MouseButton::Middle; break;
                case sf::Mouse::Button::Extra1: btn = MouseButton::X1; break;
                case sf::Mouse::Button::Extra2: btn = MouseButton::X2; break;
                default: break;
            }

            event.m_type = EventType::MouseUp;
            event.mouseButton.x = static_cast<float>(btnReleased->position.x);
            event.mouseButton.y = static_cast<float>(btnReleased->position.y);
            event.mouseButton.button = btn;
            switch (btnReleased->button) {
                case sf::Mouse::Button::Left:   event.m_eventName = EventName::MOUSE_LBUTTON_UP; break;
                case sf::Mouse::Button::Right:  event.m_eventName = EventName::MOUSE_RBUTTON_UP; break;
                case sf::Mouse::Button::Middle: event.m_eventName = EventName::MOUSE_MBUTTON_UP; break;
                default:                         event.m_eventName = EventName::MOUSE_LBUTTON_UP; break;
            }
            event.m_eventParam = mousePos;
        }

        return true;
    }

    KeyMod getModState() override {
        uint16_t mod = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) mod |= static_cast<uint16_t>(KeyMod::LShift);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) mod |= static_cast<uint16_t>(KeyMod::RShift);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) mod |= static_cast<uint16_t>(KeyMod::LCtrl);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl)) mod |= static_cast<uint16_t>(KeyMod::RCtrl);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt)) mod |= static_cast<uint16_t>(KeyMod::LAlt);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt)) mod |= static_cast<uint16_t>(KeyMod::RAlt);
        return static_cast<KeyMod>(mod);
    }

private:
    sf::Window* m_window;
    bool m_textInputActive = false;
};

InputBackend* CreateSFMLInputBackend(Window* window) {
    return new SFMLInputBackend(static_cast<sf::Window*>(window->nativeHandle()));
}
