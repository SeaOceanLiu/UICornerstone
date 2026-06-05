#include <SDL3/SDL.h>
#include <cstring>

#include "InputBackend.h"
#include "Window.h"
#include "StateMachine.h"
#include "EventQueue.h"
#include "EditBox.h"
#include "TextArea.h"

// ============================================================
// SDL Keycode → Canonical KeyCode mapping
// ============================================================
KeyCode SDLKeycodeToKeyCode(int sdlKeycode) {
    // ASCII printable range (0x20-0x7E) maps directly
    if (sdlKeycode >= 0x20 && sdlKeycode <= 0x7F) {
        // SDL3 uses lowercase for letter keys (e.g. SDLK_C = 'c' = 0x63),
        // our canonical KeyCode uses uppercase (KeyCode::C = 0x43)
        if (sdlKeycode >= 'a' && sdlKeycode <= 'z') {
            return static_cast<KeyCode>(sdlKeycode - 32);
        }
        return static_cast<KeyCode>(sdlKeycode);
    }

    switch (sdlKeycode) {
        case SDLK_RETURN:    return KeyCode::Return;
        case SDLK_ESCAPE:    return KeyCode::Escape;
        case SDLK_BACKSPACE: return KeyCode::Backspace;
        case SDLK_TAB:       return KeyCode::Tab;
        case SDLK_DELETE:    return KeyCode::Del;

        // Cursor
        case SDLK_LEFT:  return KeyCode::Left;
        case SDLK_RIGHT: return KeyCode::Right;
        case SDLK_UP:    return KeyCode::Up;
        case SDLK_DOWN:  return KeyCode::Down;
        case SDLK_HOME:  return KeyCode::Home;
        case SDLK_END:   return KeyCode::End;
        case SDLK_PAGEUP:   return KeyCode::PageUp;
        case SDLK_PAGEDOWN: return KeyCode::PageDown;
        case SDLK_INSERT:   return KeyCode::Insert;

        // Function keys
        case SDLK_F1:  return KeyCode::F1;
        case SDLK_F2:  return KeyCode::F2;
        case SDLK_F3:  return KeyCode::F3;
        case SDLK_F4:  return KeyCode::F4;
        case SDLK_F5:  return KeyCode::F5;
        case SDLK_F6:  return KeyCode::F6;
        case SDLK_F7:  return KeyCode::F7;
        case SDLK_F8:  return KeyCode::F8;
        case SDLK_F9:  return KeyCode::F9;
        case SDLK_F10: return KeyCode::F10;
        case SDLK_F11: return KeyCode::F11;
        case SDLK_F12: return KeyCode::F12;
        case SDLK_F13: return KeyCode::F13;
        case SDLK_F14: return KeyCode::F14;
        case SDLK_F15: return KeyCode::F15;
        case SDLK_F16: return KeyCode::F16;
        case SDLK_F17: return KeyCode::F17;
        case SDLK_F18: return KeyCode::F18;
        case SDLK_F19: return KeyCode::F19;
        case SDLK_F20: return KeyCode::F20;
        case SDLK_F21: return KeyCode::F21;
        case SDLK_F22: return KeyCode::F22;
        case SDLK_F23: return KeyCode::F23;
        case SDLK_F24: return KeyCode::F24;

        // Modifiers
        case SDLK_LSHIFT: return KeyCode::LShift;
        case SDLK_RSHIFT: return KeyCode::RShift;
        case SDLK_LCTRL:  return KeyCode::LCtrl;
        case SDLK_RCTRL:  return KeyCode::RCtrl;
        case SDLK_LALT:   return KeyCode::LAlt;
        case SDLK_RALT:   return KeyCode::RAlt;
        case SDLK_LGUI:   return KeyCode::LGui;
        case SDLK_RGUI:   return KeyCode::RGui;

        // Lock keys
        case SDLK_CAPSLOCK:    return KeyCode::CapsLock;
        case SDLK_NUMLOCKCLEAR: return KeyCode::NumLock;
        case SDLK_SCROLLLOCK:  return KeyCode::ScrollLock;

        // Keypad
        case SDLK_KP_0:        return KeyCode::KP0;
        case SDLK_KP_1:        return KeyCode::KP1;
        case SDLK_KP_2:        return KeyCode::KP2;
        case SDLK_KP_3:        return KeyCode::KP3;
        case SDLK_KP_4:        return KeyCode::KP4;
        case SDLK_KP_5:        return KeyCode::KP5;
        case SDLK_KP_6:        return KeyCode::KP6;
        case SDLK_KP_7:        return KeyCode::KP7;
        case SDLK_KP_8:        return KeyCode::KP8;
        case SDLK_KP_9:        return KeyCode::KP9;
        case SDLK_KP_DECIMAL:  return KeyCode::KPDecimal;
        case SDLK_KP_DIVIDE:   return KeyCode::KPDivide;
        case SDLK_KP_MULTIPLY: return KeyCode::KPMultiply;
        case SDLK_KP_MINUS:    return KeyCode::KPMinus;
        case SDLK_KP_PLUS:     return KeyCode::KPPlus;
        case SDLK_KP_ENTER:    return KeyCode::KPEnter;
        case SDLK_KP_EQUALS:   return KeyCode::KPEquals;

        // Print / Pause
        case SDLK_PRINTSCREEN: return KeyCode::PrintScreen;
        case SDLK_PAUSE:       return KeyCode::Pause;

        // Application
        case SDLK_APPLICATION: return KeyCode::Application;
        case SDLK_MENU:        return KeyCode::Menu;

        // International (not available as SDLK macros in SDL3)

        // Media
        case SDLK_MUTE:            return KeyCode::AudioMute;
        case SDLK_VOLUMEDOWN:      return KeyCode::AudioVolDown;
        case SDLK_VOLUMEUP:        return KeyCode::AudioVolUp;
        case SDLK_MEDIA_PLAY:      return KeyCode::AudioPlay;
        case SDLK_MEDIA_STOP:      return KeyCode::AudioStop;
        case SDLK_MEDIA_PREVIOUS_TRACK: return KeyCode::AudioPrev;
        case SDLK_MEDIA_NEXT_TRACK:     return KeyCode::AudioNext;

        // Browser
        case SDLK_AC_BACK:    return KeyCode::BrowserBack;
        case SDLK_AC_FORWARD: return KeyCode::BrowserForward;
        case SDLK_AC_REFRESH: return KeyCode::BrowserRefresh;
        case SDLK_AC_STOP:    return KeyCode::BrowserStop;
        case SDLK_AC_SEARCH:  return KeyCode::BrowserSearch;
        case SDLK_AC_BOOKMARKS: return KeyCode::BrowserFavorites;
        case SDLK_AC_HOME:    return KeyCode::BrowserHome;

        // System
        case SDLK_POWER:  return KeyCode::Power;
        case SDLK_SLEEP:  return KeyCode::Sleep;
        case SDLK_WAKE: return KeyCode::WakeUp;

        // Misc
        case SDLK_HELP:  return KeyCode::Help;
        case SDLK_UNDO:  return KeyCode::Undo;
        case SDLK_CUT:   return KeyCode::Cut;
        case SDLK_COPY:  return KeyCode::Copy;
        case SDLK_PASTE: return KeyCode::Paste;
        case SDLK_FIND:  return KeyCode::Find;
        case SDLK_MEDIA_SELECT: return KeyCode::MediaSelect;

        default:
            return KeyCode::Unknown;
    }
}

KeyMod SDLKeymodToKeyMod(uint16_t sdlMod) {
    // Values match directly
    return static_cast<KeyMod>(sdlMod);
}

// ============================================================
// SDL3InputBackend
// ============================================================
class SDL3InputBackend : public InputBackend {
public:
    SDL3InputBackend(void* window)
        : m_window(static_cast<SDL_Window*>(window))
    {
    }

    void startTextInput() override {
        SDL_StartTextInput(m_window);
    }

    void stopTextInput() override {
        SDL_StopTextInput(m_window);
    }

    bool isTextInputActive() const override {
        return SDL_TextInputActive(m_window);
    }

    void setClipboardText(const std::string& text) override {
        SDL_SetClipboardText(text.c_str());
    }

    std::string getClipboardText() const override {
        char* text = SDL_GetClipboardText();
        if (!text) return "";
        std::string result(text);
        SDL_free(text);
        return result;
    }

    bool hasScreenKeyboard() const override {
        return SDL_HasScreenKeyboardSupport();
    }

    bool pollEvent(Event& event) override {
        SDL_Event sdlEvent;
        if (!SDL_PollEvent(&sdlEvent)) {
            return false;
        }

        event.m_eventName = static_cast<EventName>(0);
        event.m_eventParam = {};
        event.m_type = EventType::None;
        event._pad = 0;

        switch (sdlEvent.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                event.m_type = EventType::WindowClose;
                event.m_eventName = EventName::Exit;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                event.m_type = EventType::WindowResize;
                event.resizeEvent.width = sdlEvent.window.data1;
                event.resizeEvent.height = sdlEvent.window.data2;
                break;

            case SDL_EVENT_WINDOW_MOVED:
                event.m_type = EventType::WindowMoved;
                event.windowMoved.x = sdlEvent.window.data1;
                event.windowMoved.y = sdlEvent.window.data2;
                break;

            case SDL_EVENT_KEY_DOWN:
            {
                KeyEventData keyData;
                keyData.keycode = SDLKeycodeToKeyCode(sdlEvent.key.key);
                keyData.scancode = sdlEvent.key.scancode;
                keyData.mod = SDLKeymodToKeyMod(sdlEvent.key.mod);
                keyData.repeat = sdlEvent.key.repeat != 0;

                event.m_type = EventType::KeyDown;
                event.keyEvent.keycode = keyData.keycode;
                event.keyEvent.mod = keyData.mod;
                event.keyEvent.scancode = keyData.scancode;
                event.keyEvent.repeat = keyData.repeat;
                event.m_eventName = EventName::KEY_DOWN;
                event.m_eventParam = keyData;
                break;
            }

            case SDL_EVENT_KEY_UP:
            {
                KeyEventData keyData;
                keyData.keycode = SDLKeycodeToKeyCode(sdlEvent.key.key);
                keyData.scancode = sdlEvent.key.scancode;
                keyData.mod = SDLKeymodToKeyMod(sdlEvent.key.mod);
                keyData.repeat = sdlEvent.key.repeat != 0;

                event.m_type = EventType::KeyUp;
                event.keyEvent.keycode = keyData.keycode;
                event.keyEvent.mod = keyData.mod;
                event.keyEvent.scancode = keyData.scancode;
                event.keyEvent.repeat = keyData.repeat;
                event.m_eventName = EventName::KEY_UP;
                event.m_eventParam = keyData;
                break;
            }

            case SDL_EVENT_TEXT_INPUT:
            {
                TextInputEventData textData;
                textData.text = sdlEvent.text.text;
                textData.start = -1;
                textData.length = -1;

                event.m_type = EventType::TextInput;
                SDL_strlcpy(event.textInput.text, sdlEvent.text.text, sizeof(event.textInput.text));
                event.m_eventName = EventName::TEXT_INPUT;
                event.m_eventParam = textData;
                break;
            }

            case SDL_EVENT_TEXT_EDITING:
            {
                TextInputEventData textData;
                textData.text = sdlEvent.edit.text ? sdlEvent.edit.text : "";
                textData.start = sdlEvent.edit.start;
                textData.length = sdlEvent.edit.length;

                event.m_type = EventType::TextEditing;
                SDL_strlcpy(event.textEditing.text, sdlEvent.edit.text ? sdlEvent.edit.text : "", sizeof(event.textEditing.text));
                event.textEditing.start = sdlEvent.edit.start;
                event.textEditing.length = sdlEvent.edit.length;
                event.m_eventName = EventName::TEXT_EDITING;
                event.m_eventParam = textData;
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                MouseWheelEventData wheelData;
                wheelData.x = sdlEvent.wheel.x;
                wheelData.y = sdlEvent.wheel.y;
                wheelData.mouseX = sdlEvent.wheel.mouse_x;
                wheelData.mouseY = sdlEvent.wheel.mouse_y;

                event.m_type = EventType::MouseWheel;
                event.mouseWheel.x = sdlEvent.wheel.mouse_x;
                event.mouseWheel.y = sdlEvent.wheel.mouse_y;
                event.mouseWheel.scrollX = sdlEvent.wheel.x;
                event.mouseWheel.scrollY = sdlEvent.wheel.y;
                event.m_eventName = EventName::MOUSE_WHEEL;
                event.m_eventParam = wheelData;
                break;
            }

            case SDL_EVENT_MOUSE_MOTION:
            {
                auto mousePos = make_shared<SPoint>((float)sdlEvent.motion.x, (float)sdlEvent.motion.y);

                event.m_type = EventType::MouseMove;
                event.mousePos.x = sdlEvent.motion.x;
                event.mousePos.y = sdlEvent.motion.y;
                event.m_eventName = EventName::MOUSE_MOVING;
                event.m_eventParam = mousePos;
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                auto mousePos = make_shared<SPoint>((float)sdlEvent.button.x, (float)sdlEvent.button.y);

                event.m_type = EventType::MouseDown;
                event.mouseButton.x = sdlEvent.button.x;
                event.mouseButton.y = sdlEvent.button.y;
                event.mouseButton.button = static_cast<MouseButton>(sdlEvent.button.button);
                switch (sdlEvent.button.button) {
                    case SDL_BUTTON_LEFT:   event.m_eventName = EventName::MOUSE_LBUTTON_DOWN; break;
                    case SDL_BUTTON_RIGHT:  event.m_eventName = EventName::MOUSE_RBUTTON_DOWN; break;
                    case SDL_BUTTON_MIDDLE: event.m_eventName = EventName::MOUSE_MBUTTON_DOWN; break;
                    default:                event.m_eventName = EventName::MOUSE_LBUTTON_DOWN; break;
                }
                event.m_eventParam = mousePos;
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                auto mousePos = make_shared<SPoint>((float)sdlEvent.button.x, (float)sdlEvent.button.y);

                event.m_type = EventType::MouseUp;
                event.mouseButton.x = sdlEvent.button.x;
                event.mouseButton.y = sdlEvent.button.y;
                event.mouseButton.button = static_cast<MouseButton>(sdlEvent.button.button);
                switch (sdlEvent.button.button) {
                    case SDL_BUTTON_LEFT:   event.m_eventName = EventName::MOUSE_LBUTTON_UP; break;
                    case SDL_BUTTON_RIGHT:  event.m_eventName = EventName::MOUSE_RBUTTON_UP; break;
                    case SDL_BUTTON_MIDDLE: event.m_eventName = EventName::MOUSE_MBUTTON_UP; break;
                    default:                event.m_eventName = EventName::MOUSE_LBUTTON_UP; break;
                }
                event.m_eventParam = mousePos;
                break;
            }
        }

        return true;
    }

private:
    SDL_Window* m_window;
};

InputBackend* CreateSDL3InputBackend(Window* window) {
    return new SDL3InputBackend(window->nativeHandle());
}
