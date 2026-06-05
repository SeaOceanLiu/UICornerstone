#include <SDL3/SDL.h>
#include <cstring>

#include "InputBackend.h"
#include "Window.h"
#include "StateMachine.h"
#include "EventQueue.h"
#include "EditBox.h"
#include "TextArea.h"

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
                keyData.keycode = sdlEvent.key.key;
                keyData.scancode = sdlEvent.key.scancode;
                keyData.mod = sdlEvent.key.mod;
                keyData.repeat = sdlEvent.key.repeat != 0;

                event.m_type = EventType::KeyDown;
                event.keyEvent.keycode = sdlEvent.key.key;
                event.keyEvent.mod = sdlEvent.key.mod;
                event.keyEvent.repeat = sdlEvent.key.repeat != 0;
                event.m_eventName = EventName::KEY_DOWN;
                event.m_eventParam = keyData;
                break;
            }

            case SDL_EVENT_KEY_UP:
            {
                KeyEventData keyData;
                keyData.keycode = sdlEvent.key.key;
                keyData.scancode = sdlEvent.key.scancode;
                keyData.mod = sdlEvent.key.mod;
                keyData.repeat = sdlEvent.key.repeat != 0;

                event.m_type = EventType::KeyUp;
                event.keyEvent.keycode = sdlEvent.key.key;
                event.keyEvent.mod = sdlEvent.key.mod;
                event.keyEvent.repeat = sdlEvent.key.repeat != 0;
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
                event.mousePos.x = sdlEvent.button.x;
                event.mousePos.y = sdlEvent.button.y;
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
                event.mousePos.x = sdlEvent.button.x;
                event.mousePos.y = sdlEvent.button.y;
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
