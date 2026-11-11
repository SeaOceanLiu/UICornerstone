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
void RegisterRaylibCursorFactories(void);
#include "Cursor.h"
extern Cursor* raylibCreateSystemCursor(SystemCursorType);
extern Cursor* raylibGetDefaultCursor();
extern void raylibSetCurrentCursor(Cursor*);

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
    raylibDestroy,
    // raylib 单窗口架构（全局 CORE 只跟踪最近 InitWindow 的窗口）：多实例时
    // 所有渲染/输入 API 作用于同一窗口，无法多窗口独立显示 → 不声明 MULTI_WINDOW
    UICORN_BACKEND_CAP_RENDER_TARGET | UICORN_BACKEND_CAP_CLIP_RECT | UICORN_BACKEND_CAP_READBACK
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
    return (UIWindowHandle)raylibCreateWindow(t, w, h, f);
}
static UIRenderDeviceHandle plugin_createRenderDevice(void* nativeContext) {
    Window* win = static_cast<Window*>(nativeContext);
    return (UIRenderDeviceHandle)(win ? win->renderDevice() : nullptr);
}
static UITextRendererHandle plugin_createTextRenderer(UIRenderDeviceHandle rd) {
    if (!rd) return nullptr;
    return (UITextRendererHandle)CreateRaylibTextRenderer(static_cast<RenderDevice*>(rd));
}
static UIInputBackendHandle plugin_createInputBackend(void* nativeWindowHandle) {
    if (!nativeWindowHandle) return nullptr;
    return (UIInputBackendHandle)CreateRaylibInputBackend(static_cast<Window*>(nativeWindowHandle));
}

extern "C" BACKEND_PLUGIN_EXPORT UIBackendCallbacks* GetUIBackendCallbacks(void) {
    static UIBackendCallbacks cb;
    static bool init = false;
    if (init) return &cb;
    init = true;

    cb.version = 2;
    cb.capabilities = UICORN_BACKEND_CAP_RENDER_TARGET | UICORN_BACKEND_CAP_CLIP_RECT
                    | UICORN_BACKEND_CAP_READBACK | UICORN_BACKEND_CAP_WINDOW_SET_SIZE
                    | UICORN_BACKEND_CAP_READBACK;   // 无 MULTI_WINDOW：单窗口架构
    cb.readPixels = [](UIRenderDeviceHandle dev, void* buffer, int l, int t, int w, int h) {
        if (dev) static_cast<RenderDevice*>(dev)->readPixels(buffer, SRect((float)l, (float)t, (float)w, (float)h));
    };

    // Register surface factories so the DLL's Surface::* work
    RegisterRaylibSurfaceFactories();
    // Register cursor factories so the DLL's Cursor::createSystem()/getDefault()/setCurrent() work
    RegisterRaylibCursorFactories();

    // Also populate the callback table so BackendManager::initialize(callbacks)
    // can register cursor factories from the DLL side
    cb.createSystemCursor = [](int type) -> void* {
        return raylibCreateSystemCursor(static_cast<SystemCursorType>(type));
    };
    cb.getDefaultCursor = []() -> void* {
        return raylibGetDefaultCursor();
    };
    cb.setCurrentCursor = [](void* cursor) {
        raylibSetCurrentCursor(static_cast<Cursor*>(cursor));
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

    printf("Raylib: GetUIBackendCallbacks ready\n");
    return &cb;
}
