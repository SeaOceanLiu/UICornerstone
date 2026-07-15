// =========================================================================
// test_combobox_cabi_shared.h — 三后端共享的 ComboBox C ABI 测试逻辑
//
// 包含：Core 符号 stub、C ABI 函数指针、JSON 布局、回调、main()
// Backend 源码（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）
// 必须在 include 本文件之前通过 #include 编译入同一 TU。
// =========================================================================
#ifndef TEST_COMBOBOX_CABI_SHARED_H
#define TEST_COMBOBOX_CABI_SHARED_H

#ifndef _WINDOWS_
extern "C" {
    __declspec(dllimport) void* __stdcall LoadLibraryA(const char* lpLibFileName);
    __declspec(dllimport) void* __stdcall GetProcAddress(void* hModule, const char* lpProcName);
    __declspec(dllimport) int   __stdcall FreeLibrary(void* hLibModule);
}
using HMODULE = void*;
#else
#include <windows.h>
#endif
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "../../include/UICornerstoneAPI.h"

// ===== 零导入库：内联实现 Core 符号 =====
void Surface::registerFactories(SurfaceCreateFn, SurfaceLoadFromFileFn, SurfaceLoadFromMemFn) {}
void Cursor::registerFactories(CursorCreateSystemFn, CursorGetDefaultFn, CursorSetCurrentFn) {}

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
typedef int   (*UIInitFn)(void*);
typedef void  (*UISetViewportFn)(float,float,float,float);
typedef void  (*UIProcessEventsFn)(void);
typedef void  (*UIUpdateFn)(double);
typedef void  (*UIClearFn)(void);
typedef void  (*UIRenderFn)(void);
typedef void  (*UIPresentFn)(void);
typedef int   (*UIIsQuitFn)(void);
typedef void  (*UIShutdownFn)(void);
typedef int   (*UILoadLayoutFn)(const char*);
typedef void* (*UIFindControlFn)(const char*);
typedef void  (*UIRegisterActionFn)(const char*,void(*)(void*,void*),void*);
typedef void  (*UISetTextFn)(void*,const char*);
typedef const char*   (*UIGetTextFn)(void*);
typedef void          (*UISetComboItemsFn)(void*,const char*);
typedef int           (*UIGetSelectedIndexFn)(void*);
typedef const char*   (*UIGetSelectedLabelFn)(void*);

static UIInitFn             uiInit                 = nullptr;
static UISetViewportFn      uiSetViewport          = nullptr;
static UIProcessEventsFn    uiProcessEvents        = nullptr;
static UIUpdateFn           uiUpdate               = nullptr;
static UIClearFn            uiClear                = nullptr;
static UIRenderFn           uiRender               = nullptr;
static UIPresentFn          uiPresent              = nullptr;
static UIIsQuitFn           uiIsQuitRequested      = nullptr;
static UIShutdownFn         uiShutdown             = nullptr;
static UILoadLayoutFn       uiLoadLayout           = nullptr;
static UIFindControlFn      uiFindControl          = nullptr;
static UIRegisterActionFn   uiRegisterAction       = nullptr;
static UISetTextFn          uiSetText              = nullptr;
static UIGetTextFn          uiGetText              = nullptr;
static UISetComboItemsFn    uiSetComboItems        = nullptr;
static UIGetSelectedIndexFn uiGetSelectedIndex     = nullptr;
static UIGetSelectedLabelFn uiGetSelectedLabel     = nullptr;

static HMODULE g_uiDll = nullptr;

// ===== 选中回调 =====
static char g_selectionInfo[128] = "Selected: (none)";

static void onSelectionChanged(void* ctl, void* user) {
    (void)ctl; (void)user;
    int idx = uiGetSelectedIndex(uiFindControl("comboMain"));
    const char* label = uiGetSelectedLabel(uiFindControl("comboMain"));
    snprintf(g_selectionInfo, sizeof(g_selectionInfo), "Selected: #%d = %s", idx, label ? label : "(null)");
    void* lbl = uiFindControl("lblStatus");
    if (lbl) uiSetText(lbl, g_selectionInfo);
    printf("%s\n", g_selectionInfo);
}

// ===== main =====
int main(void) {
    // ==== 加载 DLL ====
    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    // ==== 解析 C ABI ====
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
    RESOLVE(LoadLayout);
    RESOLVE(FindControl);
    RESOLVE(RegisterAction);
    RESOLVE(SetText);
    RESOLVE(GetText);
    RESOLVE(SetComboItems);
    RESOLVE(GetSelectedIndex);
    RESOLVE(GetSelectedLabel);
#undef RESOLVE

    if (!uiInit) { printf("FAIL: GetProcAddress(Init)\n"); FreeLibrary(g_uiDll); return 1; }

    // ==== 获取后端回调表 ====
    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    // ==== 初始化 ====
    if (!uiInit(callbacks)) { printf("FAIL: Init\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(0, 0, 540, 320);
    printf("OK: initialized\n");

    // ==== 注册 Action ====
    uiRegisterAction("onSelectionChanged", onSelectionChanged, nullptr);

    // ==== JSON 布局 ====
    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "Panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 540, "h": 320 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "Label",
                        "rect": { "x": 20, "y": 16, "w": 500, "h": 28 },
                        "caption": "ComboBox C ABI Test",
                        "fontSize": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "comboMain",
                        "type": "ComboBox",
                        "rect": { "x": 20, "y": 56, "w": 300, "h": 32 },
                        "fontSize": 16,
                        "placeholder": "Select a city...",
                        "items": [
                            { "label": "Beijing",   "value": "beijing" },
                            { "label": "Shanghai",  "value": "shanghai" },
                            { "label": "Guangzhou", "value": "guangzhou" },
                            { "label": "Shenzhen",  "value": "shenzhen" },
                            { "label": "Chengdu",   "value": "chengdu" },
                            { "label": "Wuhan",     "value": "wuhan", "disabled": true },
                            { "label": "Xi'an",     "value": "xian" },
                            { "label": "Hangzhou",  "value": "hangzhou" },
                            { "label": "Nanjing",   "value": "nanjing" },
                            { "label": "Chongqing", "value": "chongqing" }
                        ],
                        "events": { "onSelectionChanged": "onSelectionChanged" }
                    },
                    {
                        "id": "lblStatus",
                        "type": "Label",
                        "rect": { "x": 20, "y": 100, "w": 500, "h": 24 },
                        "caption": "Selected: (none)",
                        "fontSize": 14,
                        "textColor": [180, 200, 220]
                    },
                    {
                        "id": "lblHint",
                        "type": "Label",
                        "rect": { "x": 20, "y": 140, "w": 500, "h": 140 },
                        "caption": "Click the ComboBox to open the dropdown.\nSelect an item to see its index and label.\n\nItems that are disabled (e.g. Wuhan)\ncannot be selected.\n\nPress the close button to exit.",
                        "fontSize": 12,
                        "textColor": [140, 140, 160]
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(layoutJson)) { printf("FAIL: LoadLayout\n"); uiShutdown(); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");

    // ==== 帧循环 ====
    printf("Frame loop... (interact with the ComboBox or close the window)\n");
    while (!uiIsQuitRequested()) {
        uiProcessEvents();
        uiUpdate(1.0 / 60.0);
        uiClear();
        uiRender();
        uiPresent();
    }

    uiShutdown();
    FreeLibrary(g_uiDll);
    g_uiDll = nullptr;
    return 0;
}

#endif // TEST_COMBOBOX_CABI_SHARED_H
