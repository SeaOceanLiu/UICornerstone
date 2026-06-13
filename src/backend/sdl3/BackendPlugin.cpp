#include "BackendPlugin.h"
#include "../BackendBridge.h"
#include <SDL3/SDL.h>
#include <cstdio>

// ============================================================
// SDL3 Backend Registration
// ============================================================

static Window* sdl3CreateWindow(const char* title, int w, int h, uint32_t flags) {
    return CreateSDL3Window(title, w, h, flags);
}

static RenderDevice* sdl3CreateRenderDevice(Window* window) {
    return window->renderDevice();
}

static TextRenderer* sdl3CreateTextRenderer(RenderDevice* device) {
    return CreateSDL3TextRenderer(device);
}

static InputBackend* sdl3CreateInputBackend(Window* window) {
    return CreateSDL3InputBackend(window);
}

static bool sdl3Init() {
    SDL_SetAppMetadata("UICornerstone", "1.0.0", "com.uicornerstone.app");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

static void sdl3Destroy() {
    SDL_Quit();
}

BackendAPI g_sdl3Backend = {
    1,
    sdl3Init,
    sdl3CreateWindow,
    sdl3CreateRenderDevice,
    sdl3CreateTextRenderer,
    sdl3CreateInputBackend,
    sdl3Destroy
};

// ============================================================
// Factory registration — forward declarations from other
// backend source files (compiled into this same DLL).
// ============================================================

extern void RegisterSDL3SurfaceFactories(void);
extern void RegisterSDL3CursorFactories(void);

// ============================================================
// GetUIBackendCallbacks — C ABI callback table for plugin mode
// When compiled as a standalone plugin DLL, export the symbol.
// ============================================================

#ifdef UICORNERSTONE_BACKEND_PLUGIN
  #ifdef _MSC_VER
    #define BACKEND_PLUGIN_EXPORT __declspec(dllexport)
  #else
    #define BACKEND_PLUGIN_EXPORT __attribute__((visibility("default")))
  #endif
#else
  #define BACKEND_PLUGIN_EXPORT
#endif

static Window*      g_pluginWin = nullptr;
static RenderDevice* g_pluginRD = nullptr;
static TextRenderer* g_pluginTR = nullptr;
static InputBackend* g_pluginIB = nullptr;

static UIWindowHandle plugin_createWindow(const char* t, int w, int h, uint32_t f) {
    if (g_pluginWin) return (UIWindowHandle)g_pluginWin;
    g_pluginWin = sdl3CreateWindow(t, w, h, f);
    return (UIWindowHandle)g_pluginWin;
}
static UIRenderDeviceHandle plugin_createRenderDevice(void*) {
    if (!g_pluginRD && g_pluginWin) g_pluginRD = g_pluginWin->renderDevice();
    return (UIRenderDeviceHandle)g_pluginRD;
}
static UITextRendererHandle plugin_createTextRenderer(UIRenderDeviceHandle) {
    if (!g_pluginTR && g_pluginRD) g_pluginTR = CreateSDL3TextRenderer(g_pluginRD);
    return (UITextRendererHandle)g_pluginTR;
}
static UIInputBackendHandle plugin_createInputBackend(void*) {
    if (!g_pluginIB && g_pluginWin) g_pluginIB = CreateSDL3InputBackend(g_pluginWin);
    return (UIInputBackendHandle)g_pluginIB;
}

extern "C" BACKEND_PLUGIN_EXPORT UIBackendCallbacks* GetUIBackendCallbacks(void) {
    static UIBackendCallbacks cb;
    static bool init = false;
    if (init) return &cb;
    init = true;

    // Backend init
    if (!sdl3Init()) return nullptr;

    // Register surface and cursor factories so core (UICornerstone.dll)
    // can delegate to our concrete implementations
    RegisterSDL3SurfaceFactories();
    RegisterSDL3CursorFactories();

    cb.version = 1;

    // Window
    cb.createWindow         = plugin_createWindow;
    cb.destroyWindow        = bridge_destroyWindow;
    cb.getWindowSize        = bridge_getWindowSize;
    cb.getWindowPosition    = bridge_getWindowPosition;
    cb.getDisplayWidth      = bridge_getDisplayWidth;
    cb.getDisplayHeight     = bridge_getDisplayHeight;
    cb.getDpiScale          = bridge_getDpiScale;
    cb.setWindowTitle       = bridge_setWindowTitle;
    cb.getMousePosition     = bridge_getMousePosition;

    // RenderDevice
    cb.createRenderDevice   = plugin_createRenderDevice;
    cb.destroyRenderDevice  = bridge_destroyRenderDevice;
    cb.setDrawColor         = bridge_setDrawColor;
    cb.setBlendMode         = bridge_setBlendMode;
    cb.setClipRect          = bridge_setClipRect;
    cb.clearClipRect        = bridge_clearClipRect;
    cb.fillRect             = bridge_fillRect;
    cb.drawRect             = bridge_drawRect;
    cb.drawLine             = bridge_drawLine;
    cb.drawPoint            = bridge_drawPoint;
    cb.fillTriangle         = bridge_fillTriangle;
    cb.fillQuad             = bridge_fillQuad;
    cb.clear                = bridge_clear;
    cb.present              = bridge_present;
    cb.flush                = bridge_flush;
    cb.getNativeHandle      = bridge_getNativeHandle;
    cb.createTextureFromFile = bridge_createTextureFromFile;
    cb.destroyTexture       = bridge_destroyTexture;
    cb.drawTexture          = bridge_drawTexture;
    cb.getTextureSize       = bridge_getTextureSize;

    // InputBackend
    cb.createInputBackend   = plugin_createInputBackend;
    cb.destroyInputBackend  = bridge_destroyInputBackend;
    cb.pollEvent            = bridge_pollEvent;
    cb.startTextInput       = bridge_startTextInput;
    cb.stopTextInput        = bridge_stopTextInput;
    cb.getModState          = bridge_getModState;
    cb.setClipboardText     = bridge_setClipboardText;
    cb.getClipboardText     = bridge_getClipboardText;

    // TextRenderer
    cb.createTextRenderer   = plugin_createTextRenderer;
    cb.destroyTextRenderer  = bridge_destroyTextRenderer;
    cb.loadFont             = bridge_loadFont;
    cb.loadFontFromMemory   = bridge_loadFontFromMemory;
    cb.destroyFont          = bridge_destroyFont;
    cb.measureTextWidth     = bridge_measureTextWidth;
    cb.getFontHeight        = bridge_getFontHeight;
    cb.drawText             = bridge_drawText;
    cb.drawTextWrapped      = bridge_drawTextWrapped;

    // ResourceProvider
    cb.createResourceProvider   = bridge_createResourceProvider;
    cb.destroyResourceProvider  = bridge_destroyResourceProvider;
    cb.readFile                 = bridge_readFile;
    cb.fileExists               = bridge_fileExists;

    printf("SDL3: GetUIBackendCallbacks ready\n");
    return &cb;
}
