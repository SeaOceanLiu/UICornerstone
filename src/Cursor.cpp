#include "Cursor.h"
#include <cstdio>

// ============================================================
// Factory registration — set by backend plugin DLL during init
// ============================================================

static CursorCreateSystemFn g_createSystemFn = nullptr;
static CursorGetDefaultFn   g_getDefaultFn   = nullptr;
static CursorSetCurrentFn   g_setCurrentFn   = nullptr;

void Cursor::registerFactories(CursorCreateSystemFn c, CursorGetDefaultFn g, CursorSetCurrentFn s) {
    g_createSystemFn = c;
    g_getDefaultFn = g;
    g_setCurrentFn = s;
}

Cursor* Cursor::createSystem(SystemCursorType type) {
    if (!g_createSystemFn) {
        printf("Cursor::createSystem: no backend factory registered\n");
        return nullptr;
    }
    return g_createSystemFn(type);
}

Cursor* Cursor::getDefault() {
    if (!g_getDefaultFn) {
        printf("Cursor::getDefault: no backend factory registered\n");
        return nullptr;
    }
    return g_getDefaultFn();
}

void Cursor::setCurrent(Cursor* cursor) {
    if (!g_setCurrentFn) {
        printf("Cursor::setCurrent: no backend factory registered\n");
        return;
    }
    g_setCurrentFn(cursor);
}
