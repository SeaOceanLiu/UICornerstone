#pragma once
#include "UICornerstoneAPI.h"
#include "Window.h"
#include "RenderDevice.h"
#include "Texture.h"
#include "Font.h"
#include "TextRenderer.h"
#include "InputBackend.h"
#include "ResourceProvider.h"
#include "SColor.h"
#include "Utility.h"
#include "StateMachine.h"
#include <cstring>

// ============================================================
// Bridge: C ABI callbacks → C++ abstract interface
//
// Each bridge function casts the opaque handle to the
// corresponding C++ abstract type and calls its virtual method.
// Used by all backend plugins to implement GetUIBackendCallbacks.
//
// Native handles (UIRenderDeviceHandle, etc.) are simply
// C++ object pointers reinterpreted to void*.
// ============================================================

// ============ Window bridge ============

inline void bridge_destroyWindow(UIWindowHandle h) {
    delete static_cast<Window*>(h);
}
inline void bridge_getWindowSize(UIWindowHandle h, float* w, float* hh) {
    SSize sz = static_cast<Window*>(h)->getSize();
    if (w) *w = sz.width; if (hh) *hh = sz.height;
}
inline void bridge_getWindowPosition(UIWindowHandle h, float* x, float* y) {
    SPoint p = static_cast<Window*>(h)->getPosition();
    if (x) *x = p.x; if (y) *y = p.y;
}
inline float bridge_getDisplayWidth(UIWindowHandle h) {
    return static_cast<Window*>(h)->getDisplayWidth();
}
inline float bridge_getDisplayHeight(UIWindowHandle h) {
    return static_cast<Window*>(h)->getDisplayHeight();
}
inline float bridge_getDpiScale(UIWindowHandle h) {
    return static_cast<Window*>(h)->getDpiScale();
}
inline void bridge_setWindowTitle(UIWindowHandle h, const char* t) {
    static_cast<Window*>(h)->setTitle(t ? t : "");
}
inline int bridge_getMousePosition(UIWindowHandle h, float* x, float* y) {
    return static_cast<Window*>(h)->getMousePosition(*x, *y) ? 1 : 0;
}

// ============ RenderDevice bridge ============

inline void bridge_destroyRenderDevice(UIRenderDeviceHandle h) {
    delete static_cast<RenderDevice*>(h);
}
inline void bridge_setDrawColor(UIRenderDeviceHandle h, UIColor c) {
    static_cast<RenderDevice*>(h)->setDrawColor(SColor((int)c.r, (int)c.g, (int)c.b, (int)c.a));
}
inline void bridge_setBlendMode(UIRenderDeviceHandle h, int mode) {
    static_cast<RenderDevice*>(h)->setBlendMode(static_cast<BlendMode>(mode));
}
inline void bridge_setClipRect(UIRenderDeviceHandle h, float x, float y, float w, float hh) {
    static_cast<RenderDevice*>(h)->setClipRect(SRect(x, y, w, hh));
}
inline void bridge_clearClipRect(UIRenderDeviceHandle h) {
    static_cast<RenderDevice*>(h)->clearClipRect();
}
inline void bridge_fillRect(UIRenderDeviceHandle h, float x, float y, float w, float hh) {
    static_cast<RenderDevice*>(h)->fillRect(SRect(x, y, w, hh));
}
inline void bridge_drawRect(UIRenderDeviceHandle h, float x, float y, float w, float hh) {
    static_cast<RenderDevice*>(h)->drawRect(SRect(x, y, w, hh));
}
inline void bridge_drawLine(UIRenderDeviceHandle h, float x1, float y1, float x2, float y2) {
    static_cast<RenderDevice*>(h)->drawLine(x1, y1, x2, y2);
}
inline void bridge_drawPoint(UIRenderDeviceHandle h, float x, float y) {
    static_cast<RenderDevice*>(h)->drawPoint(x, y);
}
inline void bridge_clear(UIRenderDeviceHandle h, UIColor c) {
    static_cast<RenderDevice*>(h)->clear();
}
inline void bridge_present(UIRenderDeviceHandle h) {
    static_cast<RenderDevice*>(h)->present();
}
inline void bridge_flush(UIRenderDeviceHandle h) {
    static_cast<RenderDevice*>(h)->flush();
}
inline void* bridge_getNativeHandle(UIRenderDeviceHandle h) {
    return static_cast<RenderDevice*>(h)->getNativeHandle();
}
inline UITextureHandle bridge_createTextureFromFile(UIRenderDeviceHandle h, const char* path) {
    auto tex = static_cast<RenderDevice*>(h)->createTextureFromFile(path ? path : "");
    if (!tex) return nullptr;
    // Note: the Texture is owned by the shared_ptr, so we keep one alive
    // This leaks a shared_ptr — for proper ownership, the plugin should
    // maintain a texture registry. For now, it works for simple cases.
    auto* sp = new std::shared_ptr<Texture>(tex);
    return reinterpret_cast<UITextureHandle>(sp);
}
inline void bridge_destroyTexture(UIRenderDeviceHandle h, UITextureHandle tex) {
    delete static_cast<std::shared_ptr<Texture>*>(tex);
}
inline void bridge_drawTexture(UIRenderDeviceHandle h, UITextureHandle tex, const UIRect* src, const UIRect* dst) {
    auto* sp = static_cast<std::shared_ptr<Texture>*>(tex);
    SRect s = src ? SRect(src->x, src->y, src->w, src->h) : SRect();
    SRect d = dst ? SRect(dst->x, dst->y, dst->w, dst->h) : SRect();
    static_cast<RenderDevice*>(h)->drawTexture(sp->get(), &s, &d);
}
inline void bridge_getTextureSize(UITextureHandle tex, int* w, int* h) {
    auto* sp = static_cast<std::shared_ptr<Texture>*>(tex);
    if (w) *w = (*sp)->width(); if (h) *h = (*sp)->height();
}

// ============ InputBackend bridge ============

inline void bridge_destroyInputBackend(UIInputBackendHandle h) {
    delete static_cast<InputBackend*>(h);
}
inline void bridge_startTextInput(UIInputBackendHandle h) {
    static_cast<InputBackend*>(h)->startTextInput();
}
inline void bridge_stopTextInput(UIInputBackendHandle h) {
    static_cast<InputBackend*>(h)->stopTextInput();
}
inline int bridge_getModState(UIInputBackendHandle h) {
    return static_cast<int>(static_cast<InputBackend*>(h)->getModState());
}

// Event conversion: C++ Event → UIEvent
inline int bridge_pollEvent(UIInputBackendHandle h, UIEvent* ue) {
    if (!ue) return 0;
    memset(ue, 0, sizeof(*ue));

    Event event;
    if (!static_cast<InputBackend*>(h)->pollEvent(event)) return 0;

    switch (event.m_type) {
    case EventType::MouseMove:
        ue->type = UI_EVENT_MOUSE_MOVE;
        memcpy(ue->data, &event.mousePos.x, sizeof(float));
        memcpy(ue->data + 4, &event.mousePos.y, sizeof(float));
        break;
    case EventType::MouseDown:
        ue->type = UI_EVENT_MOUSE_DOWN;
        memcpy(ue->data, &event.mouseButton.x, sizeof(float));
        memcpy(ue->data + 4, &event.mouseButton.y, sizeof(float));
        { int btn = (int)event.mouseButton.button; memcpy(ue->data + 8, &btn, sizeof(int)); }
        break;
    case EventType::MouseUp:
        ue->type = UI_EVENT_MOUSE_UP;
        memcpy(ue->data, &event.mouseButton.x, sizeof(float));
        memcpy(ue->data + 4, &event.mouseButton.y, sizeof(float));
        { int btn = (int)event.mouseButton.button; memcpy(ue->data + 8, &btn, sizeof(int)); }
        break;
    case EventType::MouseWheel:
        ue->type = UI_EVENT_MOUSE_WHEEL;
        memcpy(ue->data, &event.mouseWheel.scrollY, sizeof(float));
        break;
    case EventType::KeyDown:
        ue->type = UI_EVENT_KEY_DOWN;
        { int kc = (int)event.keyEvent.keycode; memcpy(ue->data, &kc, sizeof(int)); }
        break;
    case EventType::KeyUp:
        ue->type = UI_EVENT_KEY_UP;
        { int kc = (int)event.keyEvent.keycode; memcpy(ue->data, &kc, sizeof(int)); }
        break;
    case EventType::TextInput:
        ue->type = UI_EVENT_TEXT_INPUT;
        strncpy((char*)ue->data, event.textInput.text, 31);
        break;
    case EventType::WindowResize:
        ue->type = UI_EVENT_WINDOW_RESIZE;
        memcpy(ue->data, &event.resizeEvent.width, sizeof(int));
        memcpy(ue->data + 4, &event.resizeEvent.height, sizeof(int));
        break;
    case EventType::WindowClose:
        ue->type = UI_EVENT_WINDOW_CLOSE;
        break;
    case EventType::FocusGained:
        ue->type = UI_EVENT_FOCUS_GAINED;
        break;
    case EventType::FocusLost:
        ue->type = UI_EVENT_FOCUS_LOST;
        break;
    default:
        return 0;
    }
    return 1;
}

// ============ TextRenderer bridge ============

inline void bridge_destroyTextRenderer(UITextRendererHandle h) {
    delete static_cast<TextRenderer*>(h);
}
inline UIFontHandle bridge_loadFont(UITextRendererHandle h, const char* path, float size) {
    auto* tr = static_cast<TextRenderer*>(h);
    auto font = tr->loadFont(path ? path : "", (int)size);
    if (!font) return nullptr;
    auto* sp = new std::shared_ptr<Font>(font);
    return reinterpret_cast<UIFontHandle>(sp);
}
inline UIFontHandle bridge_loadFontFromMemory(UITextRendererHandle h, const void* data, int len, float size) {
    auto* tr = static_cast<TextRenderer*>(h);
    auto font = tr->loadFontFromMemory(data, (size_t)len, (int)size);
    if (!font) return nullptr;
    auto* sp = new std::shared_ptr<Font>(font);
    return reinterpret_cast<UIFontHandle>(sp);
}
inline void bridge_destroyFont(UITextRendererHandle h, UIFontHandle f) {
    delete static_cast<std::shared_ptr<Font>*>(f);
}
inline float bridge_measureTextWidth(UITextRendererHandle h, UIFontHandle f, const char* text) {
    auto* sp = static_cast<std::shared_ptr<Font>*>(f);
    SSize sz = static_cast<TextRenderer*>(h)->measureText(sp->get(), text ? text : "");
    return sz.width;
}
inline float bridge_getFontHeight(UITextRendererHandle h, UIFontHandle f) {
    auto* sp = static_cast<std::shared_ptr<Font>*>(f);
    return (float)static_cast<TextRenderer*>(h)->getFontHeight(sp->get());
}
inline void bridge_drawText(UITextRendererHandle h, UIFontHandle f, const char* text, float x, float y, UIColor c) {
    auto* sp = static_cast<std::shared_ptr<Font>*>(f);
    auto color = SColor((int)c.r, (int)c.g, (int)c.b, (int)c.a);
    static_cast<TextRenderer*>(h)->drawText(sp->get(), text ? text : "", x, y, color);
}
inline void bridge_drawTextWrapped(UITextRendererHandle h, UIFontHandle f, const char* text, float x, float y, float wrapWidth, UIColor c) {
    auto* sp = static_cast<std::shared_ptr<Font>*>(f);
    auto color = SColor((int)c.r, (int)c.g, (int)c.b, (int)c.a);
    static_cast<TextRenderer*>(h)->drawText(sp->get(), text ? text : "", x, y, wrapWidth, color);
}

// ============ ResourceProvider bridge ============

inline void bridge_destroyResourceProvider(UIResourceProviderHandle h) {
    delete static_cast<ResourceProvider*>(h);
}
inline int bridge_readFile(UIResourceProviderHandle h, const char* path, void* buf, int maxLen) {
    auto data = static_cast<ResourceProvider*>(h)->readFile(path ? path : "");
    if (!data || data->empty()) return 0;
    int len = (int)data->size() < maxLen ? (int)data->size() : maxLen - 1;
    memcpy(buf, data->data(), len);
    return len;
}
inline int bridge_fileExists(UIResourceProviderHandle h, const char* path) {
    return static_cast<ResourceProvider*>(h)->exists(path ? path : "") ? 1 : 0;
}

