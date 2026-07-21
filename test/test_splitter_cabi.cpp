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
typedef float (*UIGetSplitterRatioFn)(void*);

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
static UIGetSplitterRatioFn uiGetSplitterRatio     = nullptr;

static HMODULE g_uiDll = nullptr;

static char g_ratioInfo[128] = "Ratio: 0.400";

static void onSplitterMoved(void* ctl, void* user) {
    (void)ctl; (void)user;
    float r = uiGetSplitterRatio(uiFindControl("mySplitter"));
    snprintf(g_ratioInfo, sizeof(g_ratioInfo), "Ratio: %.3f", r);
    void* lbl = uiFindControl("lblStatus");
    if (lbl) uiSetText(lbl, g_ratioInfo);
    printf("%s\n", g_ratioInfo);
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
    RESOLVE(GetSplitterRatio);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_splitter_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiInit) { printf("FAIL: GetProcAddress(Init)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    if (!uiInit(callbacks)) { printf("FAIL: Init\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(0, 0, 600, 320);
    printf("OK: initialized\n");

    uiRegisterAction("onSplitterMoved", onSplitterMoved, nullptr);

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "Panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 600, "h": 320 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "Label",
                        "rect": { "x": 20, "y": 16, "w": 560, "h": 28 },
                        "caption": "Splitter C ABI Test",
                        "fontSize": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "panelFirst",
                        "type": "Panel",
                        "rect": { "x": 20, "y": 60, "w": 224, "h": 200 },
                        "colors": { "background": { "normal": "#3A3A3AFF" } },
                        "children": [
                            {
                                "id": "lblFirst",
                                "type": "Label",
                                "rect": { "x": 10, "y": 10, "w": 200, "h": 24 },
                                "caption": "Left Panel",
                                "fontSize": 14,
                                "textColor": [200, 200, 200]
                            }
                        ]
                    },
                    {
                        "id": "panelSecond",
                        "type": "Panel",
                        "rect": { "x": 250, "y": 60, "w": 330, "h": 200 },
                        "colors": { "background": { "normal": "#3A3A3AFF" } },
                        "children": [
                            {
                                "id": "lblSecond",
                                "type": "Label",
                                "rect": { "x": 10, "y": 10, "w": 300, "h": 24 },
                                "caption": "Right Panel",
                                "fontSize": 14,
                                "textColor": [200, 200, 200]
                            }
                        ]
                    },
                    {
                        "id": "mySplitter",
                        "type": "Splitter",
                        "rect": { "x": 244, "y": 60, "w": 6, "h": 200 },
                        "orientation": "vertical",
                        "firstPanel": "panelFirst",
                        "secondPanel": "panelSecond",
                        "thickness": 6,
                        "ratio": 0.4,
                        "minFirst": 50,
                        "minSecond": 50,
                        "events": { "onSplitterMoved": "onSplitterMoved" }
                    },
                    {
                        "id": "lblStatus",
                        "type": "Label",
                        "rect": { "x": 20, "y": 280, "w": 560, "h": 24 },
                        "caption": "Ratio: 0.400",
                        "fontSize": 14,
                        "textColor": [180, 200, 220]
                    },
                    {
                        "id": "lblHint",
                        "type": "Label",
                        "rect": { "x": 20, "y": 300, "w": 560, "h": 20 },
                        "caption": "Drag the splitter bar to resize panels. Press close button to exit.",
                        "fontSize": 11,
                        "textColor": [140, 140, 160]
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(layoutJson)) { printf("FAIL: LoadLayout\n"); uiShutdown(); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");

    void* sp = uiFindControl("mySplitter");
    if (!sp) { printf("FAIL: FindControl(mySplitter)\n"); uiShutdown(); FreeLibrary(g_uiDll); return 1; }
    printf("OK: Splitter found, initial ratio=%.3f\n", uiGetSplitterRatio(sp));

    printf("Frame loop... (interact with the Splitter or close the window)\n");
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
    printf("test_splitter_cabi_%s: done\n", shortName);
    return 0;
}

int main() { return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME); }
