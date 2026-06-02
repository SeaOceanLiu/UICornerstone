#ifndef INPUTBACKEND_H
#define INPUTBACKEND_H

#include <string>

class Window;

class InputBackend {
public:
    virtual ~InputBackend() = default;

    virtual void startTextInput() = 0;
    virtual void stopTextInput() = 0;
    virtual bool isTextInputActive() const = 0;
    virtual void setClipboardText(const std::string& text) = 0;
    virtual std::string getClipboardText() const = 0;
    virtual bool hasScreenKeyboard() const = 0;
};

InputBackend* CreateSDL3InputBackend(Window* window);

#endif
