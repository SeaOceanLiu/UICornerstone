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
//   ② 后端 .cpp 文件由 CMake 按后端编译为独立 TU — 三后端通用
//   ③ 不链接 UICornerstone_dll.lib — CORE 符号由本文件的 stub 提供
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

// stub 与 FilesystemResourceProvider 需要这些核心类声明
#include "../../include/Surface.h"
#include "../../include/Cursor.h"
#include "../../include/ResourceProvider.h"

// ===== 后端源码通过 CMake 编译为独立 TU =====
//
// 6 个文件（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）
// 由 CMake 按当前后端（sdl3/sfml/raylib）选择后编译入本可执行文件，
// 提供窗口管理、渲染引擎、字体引擎、输入后端、光标、后端回调表。
// 因此本文件只需声明 GetUIBackendCallbacks()，无需 #include 后端 .cpp。

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

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

// --- MemoryResourceProvider (完整实现) ---
// 内存资源注册表（C ABI 四函数由核心 DLL 经 GetProcAddress 解析，
// 但 BackendBridge.h 的 bridge_* 直接 new MemoryResourceProvider，
// 本文件须提供完整实现以满足链接；与核心 src/ResourceProvider.cpp 保持同步）。
#include <cstring>
#include <cstdlib>
#include <unordered_map>
struct MemoryResourceProvider::Entry {
    std::shared_ptr<std::vector<char>> data;   // register 条目：拷贝后即最终存储
    const void* rawPtr = nullptr;              // adopt 条目：零拷贝持有调用方缓冲
    size_t rawLen = 0;
    void (*freeFn)(void*) = nullptr;
};
struct MemoryResourceProvider::Impl {
    std::unordered_map<std::string, Entry> entries;
    std::unordered_map<std::string, std::string> paths;   // mountPath 懒加载条目
    std::unordered_map<std::string, std::shared_ptr<std::vector<char>>> lazy;
    std::unique_ptr<ResourceProvider> delegate;           // 文件系统兜底（懒创建）
};
MemoryResourceProvider::MemoryResourceProvider() : m_impl(new Impl()) {}
MemoryResourceProvider::~MemoryResourceProvider() {
    for (auto& [name, e] : m_impl->entries) {
        if (e.freeFn && e.rawPtr) e.freeFn(const_cast<void*>(e.rawPtr));
    }
    delete m_impl;
}
bool MemoryResourceProvider::registerMemory(const std::string& name, const void* data, size_t len) {
    if (name.empty() || !data || len == 0) return false;
    auto& e = m_impl->entries[name];
    if (e.freeFn && e.rawPtr) e.freeFn(const_cast<void*>(e.rawPtr));
    e.data = std::make_shared<std::vector<char>>(static_cast<const char*>(data),
                                                 static_cast<const char*>(data) + len);
    e.rawPtr = nullptr; e.rawLen = 0; e.freeFn = nullptr;
    return true;
}
bool MemoryResourceProvider::adoptMemory(const std::string& name, void* data, size_t len,
                                         void (*freeFn)(void*)) {
    if (name.empty() || !data || len == 0) return false;
    auto& e = m_impl->entries[name];
    if (e.freeFn && e.rawPtr) e.freeFn(const_cast<void*>(e.rawPtr));
    e.data = nullptr;
    e.rawPtr = data; e.rawLen = len;
    e.freeFn = freeFn ? freeFn : std::free;
    m_impl->lazy.erase(name);
    return true;
}
bool MemoryResourceProvider::hasFilesystemFallback() const {
    return m_impl->delegate != nullptr;
}
void MemoryResourceProvider::setFilesystemFallback(const std::string& basePath) {
    if (!m_impl->delegate) m_impl->delegate.reset(ResourceProvider::createFilesystem(basePath));
}
void MemoryResourceProvider::inheritFilesystemFallback(const std::string& resourceRoot) {
    if (m_impl->delegate) return;   // 幂等（P0-80；空 resourceRoot → "assets"，与 pathPrefix 语义一致）
    m_impl->delegate.reset(ResourceProvider::createFilesystem(resourceRoot.empty() ? "assets" : resourceRoot));
}
void MemoryResourceProvider::mountPath(const std::string& name, const std::string& path,
                                       const std::string& basePath) {
    if (name.empty() || path.empty()) return;
    if (!m_impl->delegate) m_impl->delegate.reset(ResourceProvider::createFilesystem(basePath));
    m_impl->paths[name] = path;
}
static std::string stripProviderPrefix(const std::string& s) {
    if (s.rfind("provider:", 0) == 0) return s.substr(8);
    return s;
}
std::shared_ptr<std::vector<char>> MemoryResourceProvider::readFile(const std::string& path) {
    const std::string name = stripProviderPrefix(path);
    auto it = m_impl->entries.find(name);
    if (it != m_impl->entries.end()) {
        const Entry& e = it->second;
        if (e.data) return e.data;
        auto cached = m_impl->lazy.find(name);
        if (cached != m_impl->lazy.end()) return cached->second;
        auto buf = std::make_shared<std::vector<char>>(
            static_cast<const char*>(e.rawPtr), static_cast<const char*>(e.rawPtr) + e.rawLen);
        m_impl->lazy[name] = buf;
        return buf;
    }
    auto p = m_impl->paths.find(name);
    if (p != m_impl->paths.end()) {
        auto cached = m_impl->lazy.find(name);
        if (cached != m_impl->lazy.end()) return cached->second;
        if (!m_impl->delegate) return nullptr;
        auto buf = m_impl->delegate->readFile(p->second);
        if (buf) m_impl->lazy[name] = buf;
        return buf;
    }
    // P0-78：文件系统回退（未注册/未挂载键按 delegate basePath 读盘）
    if (m_impl->delegate) return m_impl->delegate->readFile(name);
    return nullptr;
}
bool MemoryResourceProvider::exists(const std::string& path) {
    const std::string name = stripProviderPrefix(path);
    if (m_impl->entries.find(name) != m_impl->entries.end()) return true;
    if (m_impl->paths.find(name) != m_impl->paths.end()) return true;
    // P0-78：文件系统回退
    if (m_impl->delegate) return m_impl->delegate->exists(name);
    return false;
}

// ===== C ABI 函数指针 =====
//
// 每个指针对应 UICornerstone.dll 中的一个导出函数。
// 全部在 main 中通过 GetProcAddress 解析。

typedef UIInstance (*UICreateInstanceFn)(void*, const void*);
typedef void  (*UIDestroyInstanceFn)(void*);
typedef void  (*UISetViewportFn)(void*,float,float,float,float);
typedef void  (*UIProcessEventsFn)(void*);
typedef void  (*UIUpdateFn)(void*,double);
typedef void  (*UIClearFn)(void*);
typedef void  (*UIRenderFn)(void*);
typedef void  (*UIPresentFn)(void*);
typedef int   (*UIIsQuitFn)(void*);
typedef void* (*UICreateButtonFn)(void*,const char*,float,float,float,float,float,float);
typedef void* (*UICreateLabelFn)(void*,const char*,float,float,float,float,float,float,float);
typedef void* (*UICreatePanelFn)(void*,float,float,float,float,float,float);
typedef int   (*UISetColorFn)(void*,void*,const char*,UIColor);
typedef int   (*UISetStringFn)(void*,void*,const char*,const char*);
typedef int   (*UISetCallbackFn)(void*,void*,const char*,void(*)(void*,const void*,void*),void*);
typedef void  (*UIAddChildFn)(void*,void*,void*);

static UICreateInstanceFn  uiCreateInstance = nullptr;
static UIDestroyInstanceFn uiDestroyInstance= nullptr;
static UISetViewportFn     uiSetViewport  = nullptr;
static UIProcessEventsFn   uiProcessEvents= nullptr;
static UIUpdateFn          uiUpdate       = nullptr;
static UIClearFn           uiClear        = nullptr;
static UIRenderFn          uiRender       = nullptr;
static UIPresentFn         uiPresent      = nullptr;
static UIIsQuitFn          uiIsQuitRequested = nullptr;
static UICreateButtonFn    uiCreateButton  = nullptr;
static UICreateLabelFn     uiCreateLabel   = nullptr;
static UICreatePanelFn     uiCreatePanel   = nullptr;
static UISetColorFn        uiSetColor      = nullptr;
static UISetStringFn       uiSetString     = nullptr;
static UISetCallbackFn     uiSetCallback   = nullptr;
static UIAddChildFn        uiAddChild      = nullptr;

static HMODULE      g_uiDll  = nullptr;   // LoadLibrary 返回的 DLL 句柄
static UIInstance   g_inst   = nullptr;   // 当前实例
static int          g_count  = 0;
static void*        g_status = nullptr;   // 状态标签句柄

// ======== 回调函数 ========

static void onBtnClick(void* ctl, const void* evt, void* user) {
    (void)ctl; (void)evt; (void)user;
    g_count++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_count);
    if (g_status && uiSetString) uiSetString(g_inst, g_status, "caption", buf);
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

    RESOLVE(CreateInstance);
    RESOLVE(DestroyInstance);
    RESOLVE(SetViewport);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(CreateButton);
    RESOLVE(CreateLabel);
    RESOLVE(CreatePanel);
    RESOLVE(SetColor);
    RESOLVE(SetString);
    RESOLVE(SetCallback);
    // AddChild 导出名是 UICornerstone_AddChildControl（非 UICornerstone_AddChild）
    *(void**)&uiAddChild = GetProcAddress(g_uiDll, "UICornerstone_AddChildControl");
#undef RESOLVE

    if (!uiCreateInstance) {
        printf("FAIL: GetProcAddress(UICornerstone_CreateInstance)\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    if (!uiSetColor) {
        printf("FAIL: GetProcAddress(UICornerstone_SetColor)\n");
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

    // ── 第四步：创建实例 ──────────────────────────────────────
    //
    // 通过函数指针 uiCreateInstance 调用 DLL 中的 UICornerstone_CreateInstance。
    // 这是函数指针调用（不是 ILT，不是直接链接）。
    g_inst = uiCreateInstance(callbacks, NULL);
    if (!g_inst) {
        printf("FAIL: UICornerstone_CreateInstance\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    uiSetViewport(g_inst, 0, 0, 800, 480);
    printf("OK: initialized\n"); fflush(stdout);

    // ── 第五步：创建控件 ──────────────────────────────────────
    //
    // 通过函数指针调用工厂函数。
    // 注意：此处不能直接调用 UICornerstone_CreatePanel（那是 ILT 链接），
    // 必须通过 uiCreatePanel 函数指针。

    void* root = uiCreatePanel(g_inst, 0, 0, 800, 480, 1.0f, 1.0f);

    void* title = uiCreateLabel(g_inst, "LoadLibrary + #include Backend Demo", 18,
                                20, 10, 760, 30, 1.0f, 1.0f);
    uiAddChild(g_inst, root, title);

    void* btn = uiCreateButton(g_inst, "Click Me", 20, 60, 200, 80, 1.0f, 1.0f);
    UIColor btnColor = {74, 144, 217, 255};
    uiSetColor(g_inst, btn, "background", btnColor);
    uiSetCallback(g_inst, btn, "click", onBtnClick, nullptr);
    uiAddChild(g_inst, root, btn);

    g_status = uiCreateLabel(g_inst, "Click the button above", 14,
                             20, 160, 400, 24, 1.0f, 1.0f);
    uiAddChild(g_inst, root, g_status);

    printf("entering loop\n"); fflush(stdout);
    int frameCount = 0;
    while (!uiIsQuitRequested(g_inst)) {
        if (++frameCount <= 3) { printf("frame %d\n", frameCount); fflush(stdout); }
        uiProcessEvents(g_inst);
        uiUpdate(g_inst, 1.0 / 60.0);
        uiClear(g_inst);
        uiRender(g_inst);
        uiPresent(g_inst);
    }
    // ── 清理 ──────────────────────────────────────────────────
    uiDestroyInstance(g_inst);
    g_inst = nullptr;
    FreeLibrary(g_uiDll);
    g_uiDll = nullptr;
    printf("Done\n");
    return 0;
}