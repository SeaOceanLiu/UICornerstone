// =========================================================================
// sample_loadlibrary.cpp — UICornerstone 显式 LoadLibrary 集成模式
//
// 【集成模式】核心 DLL（LoadLibrary 显式加载）+ #include 后端源码
//
//   架构分工：
//     UICornerstone.dll  (3.2 MB)  — 控件 + C ABI 实现
//     sample_loadlibrary.exe (272 KB) — #include 后端 6 个 .cpp 源码
//
//   关键区别 vs sample_fromsource：
//     sample_fromsource：ILT 隐式加载 DLL + CMake 独立 TU 编译后端
//     sample_loadlibrary：LoadLibrary 显式加载 + GetProcAddress 函数指针
//
// 【CMake 构建命令】（仅 DLL 模式）
//   cmake --build build\sdl3_dll --config Debug --target sample_loadlibrary
//
// 【学习要点】
//   ① LoadLibrary + GetProcAddress — 完全手动管理 DLL 生命周期
//   ② #include 后端 .cpp 文件 — 后端编译入同一 TU
//   ③ 链接 UICornerstone_dll.lib 仅用于后端注册符号（非 C ABI）
//   ④ C ABI 函数全部通过函数指针调用，无 ILT
//
// 【何时使用此模式】
//   当需要：
//     - 完全掌握 DLL 加载/卸载时机（如插件热加载）
//     - 从自定义路径加载 DLL（非 exe 同目录）
//     - 兼容不支持 ILT 的工具链或跨平台场景
//     - 按需加载 DLL（启动时不加载，用户触发后才加载）
//
// 【与 test_fromsource_sdl3 的异同】
//   相同：LoadLibrary + GetProcAddress + #include 后端源码
//   区别：本示例用 main() + 帧循环，test_fromsource 用 SDL App 回调
// =========================================================================

#include <windows.h>        // LoadLibraryA, GetProcAddress, FreeLibrary
#include <cstdio>
#include <cstdint>          // uint8_t

// 包含 UICornerstoneAPI.h 获取 UIBackendCallbacks、UIControlHandle 等类型定义
#include "../../include/UICornerstoneAPI.h"

// ===== 后端源码通过 #include 编译入同一翻译单元 =====
//
// 6 个文件提供：窗口管理、渲染引擎、字体引擎、输入后端、光标、后端回调表。
// 通过这些 #include，GetUIBackendCallbacks() 可直接在本 TU 内定义，
// 无需 CMake 额外添加编译项。
//
// include 路径由 CMake 的 target_include_directories 提供。
// 如需替换后端（如 SDL3 → SFML），只需修改路径中的 sdl3/ 为 sfml/。

#include "../../src/backend/sdl3/Window.cpp"
#include "../../src/backend/sdl3/RenderDevice.cpp"
#include "../../src/backend/sdl3/TextRenderer.cpp"
#include "../../src/backend/sdl3/InputBackend.cpp"
#include "../../src/backend/sdl3/Cursor.cpp"
#include "../../src/backend/sdl3/BackendPlugin.cpp"

// ===== 零导入库：内联实现 Core 符号 =====
//
// 不定义 UICORNERSTONE_BUILD_SHARED 时，CORE_API 为空，
// Surface/Cursor/ResourceProvider 的声明不产生 dllimport。
// 以下提供定义满足链接器，无需链接 UICornerstone_dll.lib。

// --- Surface::registerFactories (no-op) ---
// 本示例不通过 Surface::create/loadFromFile/loadFromMemory 创建图像表面，
// 故工厂注册函数为空实现。如需图像加载，调用此函数注册后端工厂。
void Surface::registerFactories(SurfaceCreateFn, SurfaceLoadFromFileFn, SurfaceLoadFromMemFn) {}

// --- Cursor::registerFactories (no-op) ---
// 本示例不创建自定义光标，工厂注册为空实现。
void Cursor::registerFactories(CursorCreateSystemFn, CursorGetDefaultFn, CursorSetCurrentFn) {}

// --- FilesystemResourceProvider (完整实现) ---
// ResourceProvider 负责文件 I/O，Label/EditBox 等控件依赖它加载字体。
// 这里从 ResourceProvider.cpp 直接内联实现，确保字体加载功能正常。
#include <filesystem>
namespace fs = std::filesystem;
class FilesystemResourceProvider : public ResourceProvider {
    fs::path m_basePath;
public:
    explicit FilesystemResourceProvider(const std::string& basePath) : m_basePath(basePath) {}
    std::shared_ptr<std::vector<char>> readFile(const std::string& path) override {
        fs::path fullPath = m_basePath / path;
        FILE* f = fopen(fullPath.string().c_str(), "rb");
        if (!f) return nullptr;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size <= 0) { fclose(f); return nullptr; }
        fseek(f, 0, SEEK_SET);
        auto buffer = std::make_shared<std::vector<char>>(static_cast<size_t>(size));
        size_t bytesRead = fread(buffer->data(), 1, static_cast<size_t>(size), f);
        fclose(f);
        if (bytesRead != static_cast<size_t>(size)) return nullptr;
        return buffer;
    }
    bool exists(const std::string& path) override {
        return fs::exists(m_basePath / path);
    }
};
ResourceProvider* ResourceProvider::createFilesystem(const std::string& basePath) {
    return new FilesystemResourceProvider(basePath);
}

// ===== C ABI 函数指针 =====
//
// 每个指针对应 UICornerstone.dll 中的一个导出函数。
// 全部在 main 中通过 GetProcAddress 解析。

typedef int   (*UIInitFn)(void*);
typedef void  (*UISetViewportFn)(float,float,float,float);
typedef void  (*UIProcessEventsFn)(void);
typedef void  (*UIUpdateFn)(double);
typedef void  (*UIClearFn)(void);
typedef void  (*UIRenderFn)(void);
typedef void  (*UIPresentFn)(void);
typedef int   (*UIIsQuitFn)(void);
typedef void  (*UIShutdownFn)(void);
typedef void* (*UICreateButtonFn)(const char*,float,float,float,float);
typedef void* (*UICreateLabelFn)(const char*,float,float,float,float,float);
typedef void* (*UICreatePanelFn)(float,float,float,float);
typedef void  (*UISetBGColorFn)(void*,uint8_t,uint8_t,uint8_t,uint8_t);
typedef void  (*UISetTextFn)(void*,const char*);
typedef void  (*UIAddChildFn)(void*,void*);
typedef void  (*UISetOnClickFn)(void*,void(*)(void*,void*),void*);

static UIInitFn            uiInit         = nullptr;
static UISetViewportFn     uiSetViewport  = nullptr;
static UIProcessEventsFn   uiProcessEvents= nullptr;
static UIUpdateFn          uiUpdate       = nullptr;
static UIClearFn           uiClear        = nullptr;
static UIRenderFn          uiRender       = nullptr;
static UIPresentFn         uiPresent      = nullptr;
static UIIsQuitFn          uiIsQuitRequested = nullptr;
static UIShutdownFn        uiShutdown     = nullptr;
static UICreateButtonFn    uiCreateButton  = nullptr;
static UICreateLabelFn     uiCreateLabel   = nullptr;
static UICreatePanelFn     uiCreatePanel   = nullptr;
static UISetBGColorFn      uiSetBGColor    = nullptr;
static UISetTextFn         uiSetText       = nullptr;
static UIAddChildFn        uiAddChild      = nullptr;
static UISetOnClickFn      uiSetOnClick    = nullptr;

static HMODULE      g_uiDll  = nullptr;   // LoadLibrary 返回的 DLL 句柄
static int          g_count  = 0;
static void*        g_status = nullptr;   // 状态标签句柄

// ======== 回调函数 ========

static void onBtnClick(void* ctl, void* user) {
    (void)ctl; (void)user;
    g_count++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_count);
    if (g_status && uiSetText) uiSetText(g_status, buf);
}

// ======== main ========

int main(void) {
    // ── 第一步：显式加载 UICornerstone.dll ──────────────────────
    //
    // LoadLibrary 返回 HMODULE，用于后续 GetProcAddress 和 FreeLibrary。
    // 如果 DLL 不在 exe 同目录，可以传完整路径：
    //   LoadLibraryA("C:\\MyApp\\UICornerstone.dll")
    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) {
        printf("FAIL: LoadLibrary(UICornerstone.dll)\n");
        return 1;
    }
    printf("OK: loaded UICornerstone.dll\n");

    // ── 第二步：解析 C ABI 函数指针 ──────────────────────────────
    //
    // GetProcAddress(hModule, "FunctionName") → void* 函数地址。
    // 若函数不存在（DLL 版本不匹配），返回 NULL。
    // 建议：至少检查 Init 是否解析成功。
    //
    // 宏展开示例：
    //   RESOLVE(Init) → uiInit = (UIInitFn)GetProcAddress(dll, "UICornerstone_Init")

#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(g_uiDll, "UICornerstone_" #name)

    RESOLVE(Init);
    RESOLVE(SetViewport);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(Shutdown);
    RESOLVE(CreateButton);
    RESOLVE(CreateLabel);
    RESOLVE(CreatePanel);
    RESOLVE(SetBGColor);
    RESOLVE(SetText);
    RESOLVE(AddChild);
    RESOLVE(SetOnClick);
#undef RESOLVE

    if (!uiInit) {
        printf("FAIL: GetProcAddress(UICornerstone_Init)\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    // ── 第三步：获取后端回调表 ──────────────────────────────────
    //
    // GetUIBackendCallbacks 定义在 BackendPlugin.cpp 中（已 #include 入本 TU）。
    // 内部已调用 sdl3Init()、RegisterSDL3SurfaceFactories()、
    // RegisterSDL3CursorFactories()，无需手动初始化。
    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) {
        printf("FAIL: GetUIBackendCallbacks\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    // ── 第四步：初始化核心控件引擎 ──────────────────────────────
    //
    // 通过函数指针 uiInit 调用 DLL 中的 UICornerstone_Init。
    // 这是函数指针调用（不是 ILT，不是直接链接）。
    if (!uiInit(callbacks)) {
        printf("FAIL: UICornerstone_Init\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    uiSetViewport(0, 0, 800, 480);
    printf("OK: initialized\n");

    // ── 第五步：创建控件 ──────────────────────────────────────
    //
    // 通过函数指针调用工厂函数。
    // 注意：此处不能直接调用 UICornerstone_CreatePanel（那是 ILT 链接），
    // 必须通过 uiCreatePanel 函数指针。

    void* root = uiCreatePanel(0, 0, 800, 480);

    void* title = uiCreateLabel("LoadLibrary + #include Backend Demo", 18,
                                20, 10, 760, 30);
    uiAddChild(root, title);

    void* btn = uiCreateButton("Click Me", 20, 60, 200, 80);
    uiSetBGColor(btn, 74, 144, 217, 255);
    uiSetOnClick(btn, onBtnClick, nullptr);
    uiAddChild(root, btn);

    g_status = uiCreateLabel("Click the button above", 14,
                             20, 160, 400, 24);
    uiAddChild(root, g_status);

    // ── 第六步：帧循环 ──────────────────────────────────────────
    while (!uiIsQuitRequested()) {
        uiProcessEvents();
        uiUpdate(1.0 / 60.0);
        uiClear();
        uiRender();
        uiPresent();
    }

    // ── 清理 ──────────────────────────────────────────────────
    uiShutdown();
    FreeLibrary(g_uiDll);
    g_uiDll = nullptr;
    printf("Done\n");
    return 0;
}