#include "BackendPlugin.h"
#include <SFML/Graphics.hpp>
#include <cstdio>

// Forward declarations of factory functions
Window* CreateSFMLWindow(const char* title, int w, int h, uint32_t flags);
RenderDevice* CreateSFMLRenderDevice(sf::RenderWindow* window);
TextRenderer* CreateSFMLTextRenderer(RenderDevice* device);
InputBackend* CreateSFMLInputBackend(Window* window);

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
    sfmlDestroy
};
