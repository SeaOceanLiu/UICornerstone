// =========================================================================
// test_numericupdown_cabi.cpp -- single fromsource C ABI test for NumericUpDown (all backends)
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

// ── NumericUpDown ──
typedef void   (*UISetNumericUpDownValueFn)(void*, double);
typedef double (*UIGetNumericUpDownValueFn)(void*);
typedef void   (*UISetNumericUpDownRangeFn)(void*, double, double);
typedef void   (*UISetNumericUpDownStepFn)(void*, double);
typedef void   (*UISetNumericUpDownPageStepFn)(void*, double);
typedef void   (*UISetNumericUpDownDecimalsFn)(void*, int);
typedef void   (*UISetNumericUpDownReadOnlyFn)(void*, int);
typedef void   (*UISetOnNumericUpDownValueChangedFn)(void*, void (*)(void*, double), void*);

static UIInitFn                        uiInit                      = nullptr;
static UISetViewportFn                 uiSetViewport               = nullptr;
static UIProcessEventsFn               uiProcessEvents             = nullptr;
static UIUpdateFn                      uiUpdate                    = nullptr;
static UIClearFn                       uiClear                     = nullptr;
static UIRenderFn                      uiRender                    = nullptr;
static UIPresentFn                     uiPresent                   = nullptr;
static UIIsQuitFn                      uiIsQuitRequested           = nullptr;
static UIShutdownFn                    uiShutdown                  = nullptr;
static UILoadLayoutFn                  uiLoadLayout                = nullptr;
static UIFindControlFn                 uiFindControl               = nullptr;
static UIRegisterActionFn              uiRegisterAction            = nullptr;

static UISetNumericUpDownValueFn       uiSetNumericUpDownValue     = nullptr;
static UIGetNumericUpDownValueFn       uiGetNumericUpDownValue     = nullptr;
static UISetNumericUpDownRangeFn       uiSetNumericUpDownRange     = nullptr;
static UISetNumericUpDownStepFn        uiSetNumericUpDownStep      = nullptr;
static UISetNumericUpDownPageStepFn    uiSetNumericUpDownPageStep  = nullptr;
static UISetNumericUpDownDecimalsFn    uiSetNumericUpDownDecimals  = nullptr;
static UISetNumericUpDownReadOnlyFn    uiSetNumericUpDownReadOnly  = nullptr;
static UISetOnNumericUpDownValueChangedFn uiSetOnNumericUpDownValueChanged = nullptr;

static HMODULE g_uiDll = nullptr;

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

    RESOLVE(SetNumericUpDownValue);
    RESOLVE(GetNumericUpDownValue);
    RESOLVE(SetNumericUpDownRange);
    RESOLVE(SetNumericUpDownStep);
    RESOLVE(SetNumericUpDownPageStep);
    RESOLVE(SetNumericUpDownDecimals);
    RESOLVE(SetNumericUpDownReadOnly);
    RESOLVE(SetOnNumericUpDownValueChanged);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    setbuf(stdout, NULL);
    printf("=== test_numericupdown_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiInit) { printf("FAIL: GetProcAddress(Init)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    if (!uiInit(callbacks)) { printf("FAIL: Init\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(0, 0, 600, 480);
    printf("OK: initialized\n");

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "Panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 600, "h": 480 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "Label",
                        "rect": { "x": 20, "y": 16, "w": 560, "h": 28 },
                        "caption": "NumericUpDown C ABI Test",
                        "fontSize": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "lblHint1",
                        "type": "Label",
                        "rect": { "x": 20, "y": 56, "w": 200, "h": 20 },
                        "caption": "nudInteger (step=1, range 0~100)",
                        "fontSize": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudInteger",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 78, "w": 180, "h": 32 },
                        "value": 50,
                        "range": { "min": 0, "max": 100 },
                        "step": 1
                    },
                    {
                        "id": "lblHint2",
                        "type": "Label",
                        "rect": { "x": 20, "y": 120, "w": 200, "h": 20 },
                        "caption": "nudFloat (step=0.2, decimals=2)",
                        "fontSize": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudFloat",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 142, "w": 180, "h": 32 },
                        "value": 0.6,
                        "range": { "min": 0.0, "max": 1.0 },
                        "step": 0.2,
                        "decimals": 2
                    },
                    {
                        "id": "lblHint3",
                        "type": "Label",
                        "rect": { "x": 20, "y": 184, "w": 200, "h": 20 },
                        "caption": "nudReadOnly (42)",
                        "fontSize": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudReadOnly",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 206, "w": 180, "h": 32 },
                        "value": 42,
                        "range": { "min": 0, "max": 100 },
                        "readOnly": true
                    },
                    {
                        "id": "lblHint4",
                        "type": "Label",
                        "rect": { "x": 20, "y": 248, "w": 200, "h": 20 },
                        "caption": "nudBigStep (step=50, range 0~1000)",
                        "fontSize": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudBigStep",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 270, "w": 180, "h": 32 },
                        "value": 100,
                        "range": { "min": 0, "max": 1000 },
                        "step": 50
                    },
                    {
                        "id": "lblHint5",
                        "type": "Label",
                        "rect": { "x": 20, "y": 312, "w": 200, "h": 20 },
                        "caption": "nudPageStep (pageStep=25)",
                        "fontSize": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudPageStep",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 334, "w": 180, "h": 32 },
                        "value": 50,
                        "range": { "min": 0, "max": 1000 },
                        "step": 1,
                        "pageStep": 25
                    },
                    {
                        "id": "lblStatus",
                        "type": "Label",
                        "rect": { "x": 20, "y": 390, "w": 560, "h": 24 },
                        "caption": "C ABI test: interact with NumericUpDown controls",
                        "fontSize": 14,
                        "textColor": [180, 200, 220]
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(layoutJson)) {
        printf("FAIL: LoadLayout\n");
        uiShutdown();
        FreeLibrary(g_uiDll);
        return 1;
    }
    printf("OK: layout loaded (5 NumericUpDown + labels)\n");

    // 通过 C ABI 设置回调（比 JSON 事件更精确，能传递 double 值）
    if (uiSetOnNumericUpDownValueChanged) {
        void* nudInt = uiFindControl("nudInteger");
        if (nudInt) {
            uiSetOnNumericUpDownValueChanged(nudInt, [](void*, double v) {
                printf("Value: %.2f\n", v);
            }, nullptr);
        }
    }

    // ── 运行时通过 C ABI 修改属性 ──
    void* nudFloat = uiFindControl("nudFloat");
    if (nudFloat && uiSetNumericUpDownValue) {
        uiSetNumericUpDownValue(nudFloat, 0.8);
        double v = uiGetNumericUpDownValue(nudFloat);
        printf("OK: nudFloat set to %.2f, get=%.2f\n", 0.8, v);
    }

    void* nudPageStep = uiFindControl("nudPageStep");
    if (nudPageStep && uiSetNumericUpDownPageStep) {
        uiSetNumericUpDownPageStep(nudPageStep, 50.0);
        printf("OK: nudPageStep pageStep set to 50\n");
    }

    printf("Frame loop... (interact with NumericUpDown controls or close the window)\n");
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
    printf("test_numericupdown_cabi_%s: done\n", shortName);
    return 0;
}

int main() { return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME); }
