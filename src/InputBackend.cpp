#include "InputBackend.h"
#include "Window.h"
#include <SDL3/SDL.h>

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

private:
    SDL_Window* m_window;
};

InputBackend* CreateSDL3InputBackend(Window* window) {
    return new SDL3InputBackend(window->nativeHandle());
}
