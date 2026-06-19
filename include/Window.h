#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include "Utility.h"

class RenderDevice;

class Window {
public:
    virtual ~Window() = default;

    virtual SSize getSize() const = 0;
    virtual SPoint getPosition() const = 0;
    virtual float getDisplayWidth() const = 0;
    virtual float getDisplayHeight() const = 0;
    virtual float getDpiScale() const = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void* nativeHandle() = 0;
    virtual RenderDevice* renderDevice() = 0;
    virtual bool getMousePosition(float& x, float& y) = 0;
    virtual void setResizable(bool resizable) { (void)resizable; }
    virtual void onResized(int width, int height) {}
};

Window* CreateSDL3Window(const char* title, int width, int height, uint32_t flags);
Window* CreateSDL3WindowFromExisting(void* nativeWindow, void* nativeRenderer);

#endif
