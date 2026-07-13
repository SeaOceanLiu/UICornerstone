// =========================================================================
// test_dialog_cabi_raylib.cpp — Raylib 后端 · Dialog C ABI 测试
// =========================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../../include/UICornerstoneAPI.h"

// ===== 后端源码通过 #include 编译入同一翻译单元 =====
#include "../../src/backend/raylib/Window.cpp"
#include "../../src/backend/raylib/RenderDevice.cpp"
#include "../../src/backend/raylib/TextRenderer.cpp"
#include "../../src/backend/raylib/InputBackend.cpp"
#include "../../src/backend/raylib/Cursor.cpp"
#include "../../src/backend/raylib/BackendPlugin.cpp"

// ===== 三后端共享测试逻辑 =====
#include "test_dialog_cabi_shared.h"

