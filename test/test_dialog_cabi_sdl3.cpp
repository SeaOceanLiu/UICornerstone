// =========================================================================
// test_dialog_cabi_sdl3.cpp — SDL3 后端 · Dialog C ABI 测试
// =========================================================================

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../../include/UICornerstoneAPI.h"

// ===== 后端源码通过 #include 编译入同一翻译单元 =====
#include "../../src/backend/sdl3/Window.cpp"
#include "../../src/backend/sdl3/RenderDevice.cpp"
#include "../../src/backend/sdl3/TextRenderer.cpp"
#include "../../src/backend/sdl3/InputBackend.cpp"
#include "../../src/backend/sdl3/Cursor.cpp"
#include "../../src/backend/sdl3/BackendPlugin.cpp"

// ===== 三后端共享测试逻辑 =====
#include "test_dialog_cabi_shared.h"

