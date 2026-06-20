// =========================================================================
// sample_fromsource.c — UICornerstone 混合集成模式（核心 DLL + 后端源码）
//
// 【集成模式】核心 DLL（ILT 隐式加载）+ 后端源码编译进 exe
//
//   架构分工：
//     UICornerstone.dll  (3.2 MB)  — 控件 + C ABI 实现，由 ILT 隐式加载
//     sample_fromsource.exe (272 KB) — 后端 6 个源码文件编译入 exe
//       后端源码通过 CMake 作为独立 TU 编译（非 #include）
//       GetUIBackendCallbacks() 来自 BackendPlugin.cpp
//       C ABI 函数通过 ILT（Import Library Thunk）调用 DLL
//
//   依赖：
//     运行时需要 UICornerstone.dll 同目录（Windows 加载器自动定位）
//     第三方后端 DLL（SDL3.dll / sfml-graphics-3.dll 等）
//     assets/ 字体和资源目录
//
// 【CMake 构建命令】（仅 DLL 模式）
//   cmake --build build\sdl3_dll --config Debug --target sample_fromsource
//
// 【学习要点】
//   ① GetUIBackendCallbacks() — 从编译进 exe 的后端获取回调查表
//   ② UICornerstone_Init(callbacks) — 替代 InitFromPlugin，手动传入回调
//   ③ C ABI 函数仍可直接调用（通过 ILT 链接到 UICornerstone.dll）
//   ④ 适用于需要定制后端行为（如复用已有窗口）的场景
//
// 【何时使用此模式】
//   当需要：
//     - 把核心控件放在 DLL 中（供多个程序共享）
//     - 自定义后端行为（替换窗口创建逻辑、事件处理等）
//     - 减小 exe 体积（272 KB vs 3.3 MB）
//     - 热更新核心控件（替换 DLL 即可）
// =========================================================================

#include "UICornerstoneAPI.h"
#include <stdio.h>

// ======== 后端函数声明（编译进 exe，不来自 DLL） ========
//
// GetUIBackendCallbacks 来自 BackendPlugin.cpp，
// 由 CMake 作为独立翻译单元编译进本 exe。
// 它返回一个包含 ~40 个函数指针的 UIBackendCallbacks 结构体，
// 涵盖窗口创建、渲染设备、字体引擎、输入后端、资源加载等。
// 不需要 LoadLibrary 或 GetProcAddress — 链接器直接解析。

extern UIBackendCallbacks* GetUIBackendCallbacks(void);

// ======== 回调函数 ========
//
// 与静态模式的回调完全兼容。句柄来自 UICornerstone.dll 中的工厂函数，
// 通过 ILT 调用，对调用者完全透明。

static int g_clickCount = 0;
static UIControlHandle g_statusLabel = NULL;

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;
    g_clickCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_clickCount);
    if (g_statusLabel) UICornerstone_SetText(g_statusLabel, buf);
}

// ======== main ========

int main(void) {
    // ── 初始化（混合集成模式） ──────────────────────────────────
    //
    // 与 InitFromPlugin 的区别：
    //   InitFromPlugin → 内部自动获取后端回调
    //   Init(callbacks) → 调用者显式传入回调表
    //
    // Init 调用后，DLL 内部会：
    //   ① callbacks->createWindow(...) 创建窗口（后端实现，在本 exe 内）
    //   ② callbacks->createRenderDevice(...) 创建渲染设备
    //   ③ callbacks->createTextRenderer(...) 创建字体引擎
    //   ④ callbacks->createInputBackend(...) 创建输入后端
    //   ⑤ callbacks->createResourceProvider(...) 创建资源加载器

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) return 1;

    if (!UICornerstone_Init(callbacks)) return 1;
    UICornerstone_SetViewport(0, 0, 800, 480);

    // ── 创建控件（C ABI 工厂函数，与静态模式完全一样） ──────────
    //
    // 函数实现位于 UICornerstone.dll 中，通过 ILT 调用。
    // 若 GetUIBackendCallbacks 或 Init 失败，请检查：
    //   - UICornerstone.dll 是否在 exe 同目录？
    //   - assets/ 字体和资源目录是否存在？
    //   - 后端 DLL 是否齐全？（SDL3.dll + SDL3_ttf.dll + SDL3_image.dll）

    UIControlHandle root = UICornerstone_CreatePanel(0, 0, 800, 480);

    UIControlHandle title = UICornerstone_CreateLabel(
        "UICornerstone Sample (Hybrid)", 18,
        20, 10, 760, 30);
    UICornerstone_AddChild(root, title);

    UIControlHandle btn = UICornerstone_CreateButton("Click Me",
        20, 60, 200, 80);
    UICornerstone_SetBGColor(btn, 74, 144, 217, 255);
    UICornerstone_SetOnClick(btn, onBtnClick, NULL);
    UICornerstone_AddChild(root, btn);

    g_statusLabel = UICornerstone_CreateLabel(
        "Click the button above", 14,
        20, 160, 400, 24);
    UICornerstone_AddChild(root, g_statusLabel);

    // ── 帧循环（与静态模式完全相同） ──────────────────────────
    //
    // ProcessEvents → 调用 InputBackend::pollEvent（在 exe 内实现）
    // Clear → RenderDevice::clear
    // Render → 绘制控件树 → RenderDevice 各绘制方法
    // Present → RenderDevice::present（SwapBuffers）
    //
    // 所有后端调用在 exe 内完成，不经过 DLL 桥接。

    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0f / 60.0f);
        UICornerstone_Clear();
        UICornerstone_Render();
        UICornerstone_Present();
    }

    UICornerstone_Shutdown();
    return 0;
}