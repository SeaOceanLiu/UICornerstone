// =========================================================================
// test_combobox_cabi.cpp -- single fromsource C ABI test for ComboBox (all backends)
// Backend name provided via -DBACKEND_SHORT_NAME / -DBACKEND_DISPLAY_NAME
// =========================================================================

#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../../include/UICornerstoneAPI.h"

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

// ===== C ABI function pointer types =====
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

static void loadAllProcs(HMODULE dll) {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(dll, "UICornerstone_" #name)

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
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_combobox_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiInit) { printf("FAIL: GetProcAddress(Init)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    if (!uiInit(callbacks)) { printf("FAIL: Init\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(0, 0, 540, 320);
    printf("OK: initialized\n");

    uiRegisterAction("onSelectionChanged", onSelectionChanged, nullptr);

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
    printf("test_combobox_cabi_%s: done\n", shortName);
    return 0;
}

int main() { return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME); }
