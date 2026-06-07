#include "BackendPlugin.h"
#include "RaylibCompat.h"
#include <cstdio>

// Forward declarations of factory functions
Window* CreateRaylibWindow(const char* title, int w, int h, uint32_t flags);
RenderDevice* CreateRaylibRenderDevice();
TextRenderer* CreateRaylibTextRenderer(RenderDevice* device);
InputBackend* CreateRaylibInputBackend(Window* window);

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
