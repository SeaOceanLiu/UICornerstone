#include "BackendPlugin.h"
#include "../BackendBridge.h"
#include "RaylibCompat.h"
#include <cstdio>

// Forward declarations of factory functions
Window* CreateRaylibWindow(const char* title, int w, int h, uint32_t flags);
RenderDevice* CreateRaylibRenderDevice();
TextRenderer* CreateRaylibTextRenderer(RenderDevice* device);
InputBackend* CreateRaylibInputBackend(Window* window);
void RegisterRaylibSurfaceFactories(void);

// ============================================================
// Raylib Backend Registration
// ============================================================

static Window* raylibCreateWindow(const char* title, int w, int h, uint32_t flags) {
    return CreateRaylibWindow(title, w, h, flags);
}

static RenderDevice* raylibCreateRenderDevice(Window* window) {
    return window->renderDevice();
}

static TextRenderer* raylibCreateTextRenderer(RenderDevice* device) {
    return CreateRaylibTextRenderer(device);
}

static InputBackend* raylibCreateInputBackend(Window* window) {
    return CreateRaylibInputBackend(window);
}

static bool raylibInit() {
    return true;
}

static void raylibDestroy() {
}

BackendAPI g_raylibBackend = {
    1,
    raylibInit,
    raylibCreateWindow,
    raylibCreateRenderDevice,
    raylibCreateTextRenderer,
    raylibCreateInputBackend,
    raylibDestroy
};

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
    g_pluginWin = raylibCreateWindow(t, w, h, f);
    return (UIWindowHandle)g_pluginWin;
}
static UIRenderDeviceHandle plugin_createRenderDevice(void*) {
    if (!g_pluginRD && g_pluginWin) g_pluginRD = g_pluginWin->renderDevice();
    return (UIRenderDeviceHandle)g_pluginRD;
}
static UITextRendererHandle plugin_createTextRenderer(UIRenderDeviceHandle) {
    if (!g_pluginTR && g_pluginRD) g_pluginTR = CreateRaylibTextRenderer(g_pluginRD);
    return (UITextRendererHandle)g_pluginTR;
}
static UIInputBackendHandle plugin_createInputBackend(void*) {
    if (!g_pluginIB && g_pluginWin) g_pluginIB = CreateRaylibInputBackend(g_pluginWin);
    return (UIInputBackendHandle)g_pluginIB;
}

extern "C" BACKEND_PLUGIN_EXPORT UIBackendCallbacks* GetUIBackendCallbacks(void) {
    static UIBackendCallbacks cb;
    static bool init = false;
    if (init) return &cb;
    init = true;

    cb.version = 1;

    // Register surface factories so the DLL's Surface::* work
    RegisterRaylibSurfaceFactories();

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
    cb.newFrame             = bridge_newFrame;
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

    printf("Raylib: GetUIBackendCallbacks ready\n");
    return &cb;
}
