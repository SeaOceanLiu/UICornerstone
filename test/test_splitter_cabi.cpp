// =========================================================================
// test_splitter_cabi.cpp -- single fromsource C ABI test for Splitter (all backends)
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
typedef UIInstance (*UICreateInstanceFn)(const UIBackendCallbacks*, const UIInstanceConfig*);
typedef void  (*UISetViewportFn)(UIInstance,float,float,float,float);
typedef void  (*UIProcessEventsFn)(UIInstance);
typedef void  (*UIUpdateFn)(UIInstance,double);
typedef void  (*UIClearFn)(UIInstance);
typedef void  (*UIRenderFn)(UIInstance);
typedef void  (*UIPresentFn)(UIInstance);
typedef int   (*UIIsQuitFn)(UIInstance);
typedef void  (*UIDestroyInstanceFn)(UIInstance);
typedef int   (*UILoadLayoutFn)(UIInstance,const char*);
typedef void* (*UIFindControlFn)(UIInstance,const char*);
typedef void  (*UIPushUIEventFn)(UIInstance,const UIEvent*);
typedef void  (*UIRegisterActionFn)(UIInstance,const char*,void(*)(void*,void*),void*);
typedef int  (*UISetStringFn)(UIInstance,void*,const char*,const char*);
typedef int  (*UIGetFloatFn)(UIInstance,void*,const char*,float*);

static UICreateInstanceFn   uiCreateInstance       = nullptr;
static UISetViewportFn      uiSetViewport          = nullptr;
static UIProcessEventsFn    uiProcessEvents        = nullptr;
static UIUpdateFn           uiUpdate               = nullptr;
static UIClearFn            uiClear                = nullptr;
static UIRenderFn           uiRender               = nullptr;
static UIPresentFn          uiPresent              = nullptr;
static UIIsQuitFn           uiIsQuitRequested      = nullptr;
static UIPushUIEventFn     uiPushUIEvent          = nullptr;
static UIDestroyInstanceFn  uiDestroyInstance      = nullptr;
static UILoadLayoutFn       uiLoadLayout           = nullptr;
static UIFindControlFn      uiFindControl          = nullptr;
static UIRegisterActionFn   uiRegisterAction       = nullptr;
static UISetStringFn uiSetString = nullptr;
static UIGetFloatFn  uiGetFloat  = nullptr;

static HMODULE  g_uiDll = nullptr;
static UIInstance g_inst = nullptr;
static int g_autoSec = 0;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）

static char g_ratioInfo[128] = "Ratio: 0.400";

static void onSplitterMoved(void* ctl, void* user) {
    (void)ctl; (void)user;
    float r = 0.0f;
    uiGetFloat(g_inst, uiFindControl(g_inst, "mySplitter"), "ratio", &r);
    snprintf(g_ratioInfo, sizeof(g_ratioInfo), "Ratio: %.3f", r);
    void* lbl = uiFindControl(g_inst, "lblStatus");
    if (lbl) uiSetString(g_inst, lbl, "caption", g_ratioInfo);
    printf("%s\n", g_ratioInfo);
}

static void loadAllProcs(HMODULE dll) {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(dll, "UICornerstone_" #name)

    RESOLVE(CreateInstance);
    RESOLVE(SetViewport);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(PushUIEvent);
    RESOLVE(DestroyInstance);
    RESOLVE(LoadLayout);
    RESOLVE(FindControl);
    RESOLVE(RegisterAction);
    RESOLVE(SetString);
    RESOLVE(GetFloat);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_splitter_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiCreateInstance) { printf("FAIL: GetProcAddress(CreateInstance)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_splitter_cabi";
    cfg.windowWidth = 700;
    cfg.windowHeight = 420;
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 700, 420);
    printf("OK: initialized\n");

    uiRegisterAction(g_inst, "onSplitterMoved", onSplitterMoved, nullptr);

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 700, "h": 420 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "label",
                        "rect": { "x": 20, "y": 16, "w": 560, "h": 28 },
                        "caption": "Splitter C ABI Test",
                        "font-size": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "panelFirst",
                        "type": "panel",
                        "rect": { "x": 20, "y": 60, "w": 224, "h": 200 },
                        "colors": { "background": { "normal": "#3A3A3AFF" } },
                        "children": [
                            {
                                "id": "lblFirst",
                                "type": "label",
                                "rect": { "x": 10, "y": 10, "w": 200, "h": 24 },
                                "caption": "Left Panel",
                                "font-size": 14,
                                "textColor": [200, 200, 200]
                            }
                        ]
                    },
                    {
                        "id": "panelSecond",
                        "type": "panel",
                        "rect": { "x": 250, "y": 60, "w": 330, "h": 200 },
                        "colors": { "background": { "normal": "#3A3A3AFF" } },
                        "children": [
                            {
                                "id": "lblSecond",
                                "type": "label",
                                "rect": { "x": 10, "y": 10, "w": 300, "h": 24 },
                                "caption": "Right Panel",
                                "font-size": 14,
                                "textColor": [200, 200, 200]
                            }
                        ]
                    },
                    {
                        "id": "mySplitter",
                        "type": "splitter",
                        "rect": { "x": 244, "y": 60, "w": 6, "h": 200 },
                        "orientation": "vertical",
                        "first-linked": "panelFirst",
                        "second-linked": "panelSecond",
                        "thickness": 6,
                        "ratio": 0.4,
                        "first-min": 50,
                        "second-min": 50,
                        "events": { "onSplitterMoved": "onSplitterMoved" }
                    },
                    {
                        "id": "lblStatus",
                        "type": "label",
                        "rect": { "x": 20, "y": 280, "w": 560, "h": 24 },
                        "caption": "Ratio: 0.400",
                        "font-size": 14,
                        "textColor": [180, 200, 220]
                    },
                    {
                        "id": "lblHint",
                        "type": "label",
                        "rect": { "x": 20, "y": 300, "w": 560, "h": 20 },
                        "caption": "Drag the splitter bar to resize panels. Press close button to exit.",
                        "font-size": 11,
                        "textColor": [140, 140, 160]
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(g_inst, layoutJson)) { printf("FAIL: LoadLayout\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");

    void* sp = uiFindControl(g_inst, "mySplitter");
    if (!sp) { printf("FAIL: FindControl(mySplitter)\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    float r0 = 0.0f;
    uiGetFloat(g_inst, sp, "ratio", &r0);
    printf("OK: Splitter found, initial ratio=%.3f\n", r0);

    printf("Frame loop... (interact with the Splitter or close the window)\n");
    ULONGLONG autoT0 = GetTickCount64();
    while (!uiIsQuitRequested(g_inst)) {
        if (g_autoSec > 0 && (GetTickCount64() - autoT0) >= (ULONGLONG)g_autoSec * 1000) {
            UIEvent ue; memset(&ue, 0, sizeof(ue)); ue.type = UI_EVENT_WINDOW_CLOSE;
            uiPushUIEvent(g_inst, &ue);
        }
        uiProcessEvents(g_inst);
        uiUpdate(g_inst, 1.0 / 60.0);
        uiClear(g_inst);
        uiRender(g_inst);
        uiPresent(g_inst);
    }

    uiDestroyInstance(g_inst);
    g_inst = nullptr;
    FreeLibrary(g_uiDll);
    g_uiDll = nullptr;
    printf("test_splitter_cabi_%s: done\n", shortName);
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}
