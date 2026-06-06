#ifndef INPUTBACKEND_H
#define INPUTBACKEND_H

#include <string>
#include <cstdint>
#include "EventTypes.h"

class Event;
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

    // Poll an abstract event from the backend.
    // Populates both new (EventType+union) and old (EventName+std::any) fields.
    // Returns true if an event was polled, false if no events pending.
    virtual bool pollEvent(Event& event) = 0;

    // Get current keyboard modifier state (for use outside event handlers)
    virtual KeyMod getModState() = 0;
};

// SDL3 backend factory (only available when using SDL3 backend)
InputBackend* CreateSDL3InputBackend(Window* window);

// SDL→canonical mapping utilities (for SDL3 backend)
KeyCode SDLKeycodeToKeyCode(int sdlKeycode);
KeyMod  SDLKeymodToKeyMod(uint16_t sdlMod);

#endif
