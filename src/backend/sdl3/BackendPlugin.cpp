#include "BackendPlugin.h"
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
