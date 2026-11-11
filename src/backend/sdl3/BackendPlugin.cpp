#include "BackendPlugin.h"
#include "../BackendBridge.h"
#include <SDL3/SDL.h>
#include <cstdio>

// ============================================================
// SDL3 Backend Registration
// ============================================================

// 窗口复用模式：若用户通过 SDL3Backend_SetReuseWindow 传入了已有的
// SDL 窗口/渲染器句柄，则复用它们（不创建新窗口，sdl3Init/sdl3Destroy 为空操作）
static SDL_Window*   g_reuseWindow   = nullptr;
static SDL_Renderer* g_reuseRenderer = nullptr;

static bool g_sdl3Active = false;

bool SDL3BackendIsActive(void) {
    return g_sdl3Active;
}

void SDL3Backend_SetReuseWindow(SDL_Window* w, SDL_Renderer* r) {
    g_reuseWindow = w;
    g_reuseRenderer = r;
}

static Window* sdl3CreateWindow(const char* title, int w, int h, uint32_t flags) {
    if (g_reuseWindow && g_reuseRenderer)
        return CreateSDL3WindowFromExisting(g_reuseWindow, g_reuseRenderer);
    return CreateSDL3Window(title, w, h, flags);
}
static bool sdl3Init() {
    if (g_reuseWindow && g_reuseRenderer) return true;
    SDL_SetAppMetadata("UICornerstone", "1.2.0", "com.uicornerstone.app");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    g_sdl3Active = true;
    return true;
}
static void sdl3Destroy() {
    if (g_reuseWindow && g_reuseRenderer) return;
    SDL_Quit();
    g_sdl3Active = false;
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

BackendAPI g_sdl3Backend = {
    1,
    sdl3Init,
    sdl3CreateWindow,
    sdl3CreateRenderDevice,
    sdl3CreateTextRenderer,
    sdl3CreateInputBackend,
    sdl3Destroy,
    UICORN_BACKEND_CAP_MULTI_WINDOW | UICORN_BACKEND_CAP_RENDER_TARGET
        | UICORN_BACKEND_CAP_CLIP_RECT | UICORN_BACKEND_CAP_READBACK
        | UICORN_BACKEND_CAP_WINDOW_SET_SIZE
};

// ============================================================
// Factory registration — forward declarations from other
// backend source files (compiled into this same DLL).
// ============================================================

extern void RegisterSDL3SurfaceFactories(void);
extern void RegisterSDL3CursorFactories(void);
#include "Cursor.h"
extern Cursor* sdl3CreateSystemCursor(SystemCursorType);
extern Cursor* sdl3GetDefaultCursor();
extern void sdl3SetCurrentCursor(Cursor*);

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
    return (UIWindowHandle)sdl3CreateWindow(t, w, h, f);
}
static UIRenderDeviceHandle plugin_createRenderDevice(void* nativeContext) {
    Window* win = static_cast<Window*>(nativeContext);
    return (UIRenderDeviceHandle)(win ? win->renderDevice() : nullptr);
}
static UITextRendererHandle plugin_createTextRenderer(UIRenderDeviceHandle rd) {
    if (!rd) return nullptr;
    return (UITextRendererHandle)CreateSDL3TextRenderer(static_cast<RenderDevice*>(rd));
}
static UIInputBackendHandle plugin_createInputBackend(void* nativeWindowHandle) {
    if (!nativeWindowHandle) return nullptr;
    return (UIInputBackendHandle)CreateSDL3InputBackend(static_cast<Window*>(nativeWindowHandle));
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

    // Also populate the callback table so BackendManager::initialize(callbacks)
    // can register cursor factories from the DLL side (for fromsource/DLL bridge path)
    cb.createSystemCursor = [](int type) -> void* {
        return sdl3CreateSystemCursor(static_cast<SystemCursorType>(type));
    };
    cb.getDefaultCursor = []() -> void* {
        return sdl3GetDefaultCursor();
    };
    cb.setCurrentCursor = [](void* cursor) {
        sdl3SetCurrentCursor(static_cast<Cursor*>(cursor));
    };

    cb.version = 2;
    cb.capabilities = UICORN_BACKEND_CAP_MULTI_WINDOW | UICORN_BACKEND_CAP_RENDER_TARGET
                    | UICORN_BACKEND_CAP_CLIP_RECT | UICORN_BACKEND_CAP_READBACK
                    | UICORN_BACKEND_CAP_WINDOW_SET_SIZE;
    cb.readPixels = [](UIRenderDeviceHandle dev, void* buffer, int l, int t, int w, int h) {
        if (dev) static_cast<RenderDevice*>(dev)->readPixels(buffer, SRect((float)l, (float)t, (float)w, (float)h));
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

    printf("SDL3: GetUIBackendCallbacks ready\n");
    return &cb;
}
