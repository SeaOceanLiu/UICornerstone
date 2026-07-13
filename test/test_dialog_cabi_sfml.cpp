// =========================================================================
// test_dialog_cabi_sfml.cpp — SFML 后端 · Dialog C ABI 测试
// =========================================================================

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../../include/UICornerstoneAPI.h"

// ===== 后端源码通过 #include 编译入同一翻译单元 =====
#include "../../src/backend/sfml/Window.cpp"
#include "../../src/backend/sfml/RenderDevice.cpp"
#include "../../src/backend/sfml/TextRenderer.cpp"
#include "../../src/backend/sfml/InputBackend.cpp"
#include "../../src/backend/sfml/Cursor.cpp"
#include "../../src/backend/sfml/BackendPlugin.cpp"

// ===== 三后端共享测试逻辑 =====
#include "test_dialog_cabi_shared.h"

