#include "BackendPlugin.h"
#include "../BackendBridge.h"
#include <SFML/Graphics.hpp>
#include <cstdio>

// Forward declarations of factory functions
Window* CreateSFMLWindow(const char* title, int w, int h, uint32_t flags);
RenderDevice* CreateSFMLRenderDevice(sf::RenderWindow* window);
TextRenderer* CreateSFMLTextRenderer(RenderDevice* device);
InputBackend* CreateSFMLInputBackend(Window* window);
void RegisterSFMLSurfaceFactories(void);
void RegisterSFMLCursorFactories(void);
#include "Cursor.h"
extern Cursor* sfmlCreateSystemCursor(SystemCursorType);
extern Cursor* sfmlGetDefaultCursor();
extern void sfmlSetCurrentCursor(Cursor*);

// ============================================================
// SFML Backend Registration
// ============================================================

static Window* sfmlCreateWindow(const char* title, int w, int h, uint32_t flags) {
    return CreateSFMLWindow(title, w, h, flags);
}

static RenderDevice* sfmlCreateRenderDevice(Window* window) {
    return window->renderDevice();
}

static TextRenderer* sfmlCreateTextRenderer(RenderDevice* device) {
    return CreateSFMLTextRenderer(device);
}

static InputBackend* sfmlCreateInputBackend(Window* window) {
    return CreateSFMLInputBackend(window);
}

static bool sfmlInit() {
    return true;
}

static void sfmlDestroy() {
}

BackendAPI g_sfmlBackend = {
    1,
    sfmlInit,
    sfmlCreateWindow,
    sfmlCreateRenderDevice,
    sfmlCreateTextRenderer,
    sfmlCreateInputBackend,
    sfmlDestroy,
    UICORN_BACKEND_CAP_MULTI_WINDOW | UICORN_BACKEND_CAP_RENDER_TARGET
        | UICORN_BACKEND_CAP_CLIP_RECT | UICORN_BACKEND_CAP_READBACK
        | UICORN_BACKEND_CAP_WINDOW_SET_SIZE
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

static UIWindowHandle plugin_createWindow(const char* t, int w, int h, uint32_t f) {
    return (UIWindowHandle)sfmlCreateWindow(t, w, h, f);
}
static UIRenderDeviceHandle plugin_createRenderDevice(void* nativeContext) {
    Window* win = static_cast<Window*>(nativeContext);
    return (UIRenderDeviceHandle)(win ? win->renderDevice() : nullptr);
}
static UITextRendererHandle plugin_createTextRenderer(UIRenderDeviceHandle rd) {
    if (!rd) return nullptr;
    return (UITextRendererHandle)CreateSFMLTextRenderer(static_cast<RenderDevice*>(rd));
}
static UIInputBackendHandle plugin_createInputBackend(void* nativeWindowHandle) {
    if (!nativeWindowHandle) return nullptr;
    return (UIInputBackendHandle)CreateSFMLInputBackend(static_cast<Window*>(nativeWindowHandle));
}

extern "C" BACKEND_PLUGIN_EXPORT UIBackendCallbacks* GetUIBackendCallbacks(void) {
    static UIBackendCallbacks cb;
    static bool init = false;
    if (init) return &cb;
    init = true;

    cb.version = 2;
    cb.capabilities = UICORN_BACKEND_CAP_MULTI_WINDOW | UICORN_BACKEND_CAP_RENDER_TARGET
                    | UICORN_BACKEND_CAP_CLIP_RECT | UICORN_BACKEND_CAP_READBACK
                    | UICORN_BACKEND_CAP_WINDOW_SET_SIZE;
    cb.readPixels = [](UIRenderDeviceHandle dev, void* buffer, int l, int t, int w, int h) {
        if (dev) static_cast<RenderDevice*>(dev)->readPixels(buffer, SRect((float)l, (float)t, (float)w, (float)h));
    };

    // Register surface factories so the DLL's Surface::loadFromFile/Surface::create/Surface::loadFromMemory work
    RegisterSFMLSurfaceFactories();
    // Register cursor factories so the DLL's Cursor::createSystem()/getDefault()/setCurrent() work
    RegisterSFMLCursorFactories();

    // Also populate the callback table so BackendManager::initialize(callbacks)
    // can register cursor factories from the DLL side
    cb.createSystemCursor = [](int type) -> void* {
        return sfmlCreateSystemCursor(static_cast<SystemCursorType>(type));
    };
    cb.getDefaultCursor = []() -> void* {
        return sfmlGetDefaultCursor();
    };
    cb.setCurrentCursor = [](void* cursor) {
        sfmlSetCurrentCursor(static_cast<Cursor*>(cursor));
    };

    // Window
    cb.createWindow         = plugin_createWindow;
    cb.destroyWindow        = bridge_destroyWindow;
    cb.getWindowSize        = bridge_getWindowSize;
    cb.getWindowPosition    = bridge_getWindowPosition;
    cb.getDisplayWidth      = bridge_getDisplayWidth;
    cb.getDisplayHeight     = bridge_getDisplayHeight;
    cb.getDpiScale          = bridge_getDpiScale;
    cb.setWindowTitle       = bridge_setWindowTitle;
    cb.setWindowSize        = bridge_setWindowSize;
    cb.getMousePosition     = bridge_getMousePosition;
    cb.setWindowVisible      = [](UIWindowHandle h, int visible) -> int {
        return h ? (static_cast<Window*>(h)->setVisible(visible != 0) ? 1 : 0) : 0;
    };
    cb.getWindowVisible      = [](UIWindowHandle h) -> int {
        return h ? (static_cast<Window*>(h)->isVisible() ? 1 : 0) : -1;
    };
    cb.setWindowAlwaysOnTop  = [](UIWindowHandle h, int onTop) -> int {
        return h ? (static_cast<Window*>(h)->setAlwaysOnTop(onTop != 0) ? 1 : 0) : 0;
    };
    cb.getWindowAlwaysOnTop  = [](UIWindowHandle h) -> int {
        return h ? (static_cast<Window*>(h)->isAlwaysOnTop() ? 1 : 0) : -1;
    };
    cb.setWindowForeground   = [](UIWindowHandle h) -> int {
        return h ? (static_cast<Window*>(h)->setForeground() ? 1 : 0) : 0;
    };

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
    cb.setBackendConfig     = bridge_setBackendConfig;
    cb.getBackendConfig     = bridge_getBackendConfig;
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
    cb.createMemoryResourceProvider = bridge_createMemoryResourceProvider;
    cb.memoryProviderRegister      = bridge_memoryProviderRegister;
    cb.memoryProviderAdopt         = bridge_memoryProviderAdopt;
    cb.setResourceProvider         = bridge_setResourceProvider;

    printf("SFML: GetUIBackendCallbacks ready\n");
    return &cb;
}
