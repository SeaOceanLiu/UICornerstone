#include "Surface.h"
#include <cstdio>

// ============================================================
// Factory registration — set by backend plugin DLL during init
// ============================================================

static SurfaceCreateFn      g_createFn     = nullptr;
static SurfaceLoadFromFileFn g_loadFileFn  = nullptr;
static SurfaceLoadFromMemFn  g_loadMemFn   = nullptr;

void Surface::registerFactories(SurfaceCreateFn c, SurfaceLoadFromFileFn lf, SurfaceLoadFromMemFn lm) {
    g_createFn = c;
    g_loadFileFn = lf;
    g_loadMemFn = lm;
}

SharedSurface Surface::create(int width, int height) {
    if (!g_createFn) {
        printf("Surface::create: no backend factory registered\n");
        return nullptr;
    }
    return g_createFn(width, height);
}

SharedSurface Surface::loadFromFile(const std::string& path) {
    if (!g_loadFileFn) {
        printf("Surface::loadFromFile: no backend factory registered\n");
        return nullptr;
    }
    return g_loadFileFn(path);
}

SharedSurface Surface::loadFromMemory(const void* data, size_t len) {
    if (!g_loadMemFn) {
        printf("Surface::loadFromMemory: no backend factory registered\n");
        return nullptr;
    }
    return g_loadMemFn(data, len);
}
