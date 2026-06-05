#ifndef APPCALLBACKS_H
#define APPCALLBACKS_H

class Event;

class AppCallbacks {
public:
    virtual ~AppCallbacks() = default;
    virtual bool onInit() = 0;
    virtual void onEvent(const Event& event) {}  // optional override for app-specific event handling
    virtual void onUpdate() = 0;
    virtual void onRender() = 0;
    virtual void onQuit() = 0;
};

#endif
