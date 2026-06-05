#ifndef APPCALLBACKS_H
#define APPCALLBACKS_H

class AppCallbacks {
public:
    virtual ~AppCallbacks() = default;
    virtual bool onInit() = 0;
    virtual void onUpdate() = 0;
    virtual void onRender() = 0;
    virtual void onQuit() = 0;
};

#endif
