#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include "Utility.h"
#include "UICornerstoneAPI.h"   // UICORN_WINDOW_FLAG_*（UIWindowFlags 唯一值来源）

class RenderDevice;

// 跨后端统一窗口标志（值对齐 SDL_WINDOW_*，各后端自行映射：
// raylib 按 SDL_WINDOW_RESIZABLE=0x20 约定，sfml 按 0x01=Fullscreen 约定）
namespace UIWindowFlags {
    constexpr uint32_t None       = UICORN_WINDOW_FLAG_NONE;
    constexpr uint32_t Fullscreen = UICORN_WINDOW_FLAG_FULLSCREEN;
    constexpr uint32_t Hidden     = UICORN_WINDOW_FLAG_HIDDEN;
    constexpr uint32_t Borderless = UICORN_WINDOW_FLAG_BORDERLESS;
    constexpr uint32_t Utility    = UICORN_WINDOW_FLAG_UTILITY;
    constexpr uint32_t NoShadow   = UICORN_WINDOW_FLAG_NO_SHADOW;
    constexpr uint32_t Resizable  = UICORN_WINDOW_FLAG_RESIZABLE;
    constexpr uint32_t Vsync      = UICORN_WINDOW_FLAG_VSYNC;
}   // 值引用 UICornerstoneAPI.h 宏（单一事实来源）

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
    // 请求窗口 resize（后端实现：sdl3=SDL_SetWindowSize、sfml=setSize、
    // raylib=SetWindowSize（headless 实例空操作，由调用方按
    // UICORN_BACKEND_CAP_WINDOW_SET_SIZE 守卫））。尺寸变化经后端事件
    // 回流 WindowResize 见 §21（运行期窗口 API）。
    virtual void setSize(int width, int height) { (void)width; (void)height; }
    virtual void onResized(int width, int height) {}
    virtual bool setVisible(bool visible) { (void)visible; return false; } // 默认失败；各后端显式覆盖
    virtual bool isVisible() const { return !isHeadless(); }
    virtual bool setAlwaysOnTop(bool onTop) { (void)onTop; return false; }
    virtual bool isAlwaysOnTop() const { return false; }
    virtual bool setForeground() { return false; }
    // 无原生窗口（headless）查询：单窗口架构后端（raylib）的多实例中，
    // 非首个实例不创建真实窗口（防止覆盖全局窗口状态），据此跳过
    // 渲染/输入等窗口相关操作。默认有窗口；原生 GPU 后端（GLFW 等）
    // 多窗口天然支持，无需覆写。
    virtual bool isHeadless() const { return false; }
};

Window* CreateSDL3Window(const char* title, int width, int height, uint32_t flags);
Window* CreateSDL3WindowFromExisting(void* nativeWindow, void* nativeRenderer);

#endif
