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

// ===== C ABI function pointer types（当前 API：多实例，函数均带 UIInstance 参数）=====
typedef UIInstance (*UICreateInstanceFn)(const UIBackendCallbacks*, const UIInstanceConfig*);
typedef void       (*UIDestroyInstanceFn)(UIInstance);
typedef void  (*UISetViewportFn)(UIInstance,float,float,float,float);
typedef void  (*UIProcessEventsFn)(UIInstance);
typedef void  (*UIUpdateFn)(UIInstance,double);
typedef void  (*UIClearFn)(UIInstance);
typedef void  (*UIRenderFn)(UIInstance);
typedef void  (*UIPresentFn)(UIInstance);
typedef int   (*UIIsQuitFn)(UIInstance);
typedef int   (*UILoadLayoutFn)(UIInstance,const char*);
typedef void* (*UIFindControlFn)(UIInstance,const char*);
typedef void  (*UIRegisterActionFn)(UIInstance,const char*,void(*)(void*,void*),void*);
typedef void  (*UIPushUIEventFn)(UIInstance,const UIEvent*);

typedef int  (*UISetFloatFn)(UIInstance,void*,const char*,float);
typedef int  (*UIGetFloatFn)(UIInstance,void*,const char*,float*);
typedef int  (*UISetStringFn)(UIInstance,void*,const char*,const char*);
typedef int  (*UISetCallbackFn)(UIInstance,void*,const char*,void (*)(void*, const void*, void*),void*);

static UICreateInstanceFn             uiCreateInstance          = nullptr;
static UIDestroyInstanceFn            uiDestroyInstance         = nullptr;
static UISetViewportFn                uiSetViewport             = nullptr;
static UIProcessEventsFn              uiProcessEvents           = nullptr;
static UIUpdateFn                     uiUpdate                  = nullptr;
static UIClearFn                      uiClear                   = nullptr;
static UIRenderFn                     uiRender                  = nullptr;
static UIPresentFn                    uiPresent                 = nullptr;
static UIIsQuitFn                     uiIsQuitRequested         = nullptr;
static UIPushUIEventFn                uiPushUIEvent             = nullptr;
static UILoadLayoutFn                 uiLoadLayout              = nullptr;
static UIFindControlFn                uiFindControl             = nullptr;
static UIRegisterActionFn             uiRegisterAction          = nullptr;

static UISetFloatFn    uiSetFloat    = nullptr;
static UIGetFloatFn    uiGetFloat    = nullptr;
static UISetStringFn   uiSetString   = nullptr;
static UISetCallbackFn uiSetCallback = nullptr;

static HMODULE g_uiDll = nullptr;
static UIInstance g_inst = nullptr;
static int g_autoSec = 0;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）

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

    RESOLVE(SetFloat);
    RESOLVE(GetFloat);
    RESOLVE(SetString);
    RESOLVE(SetCallback);
#undef RESOLVE
}

static void onNudValueChanged(void*, const void* evt, void* user) {
    float v = ((const UIEventData*)evt)->data.floatVal;
    printf("%s = %.2f\n", (const char*)user, (double)v);
    void* status = uiFindControl(g_inst, "lblStatus");
    if (!status) return;
    char buf[128];
    float vi=0,vf=0,vo=0,vb=0,vp=0;
    uiGetFloat(g_inst, uiFindControl(g_inst, "nudInteger"), "value", &vi);
    uiGetFloat(g_inst, uiFindControl(g_inst, "nudFloat"), "value", &vf);
    uiGetFloat(g_inst, uiFindControl(g_inst, "nudReadOnly"), "value", &vo);
    uiGetFloat(g_inst, uiFindControl(g_inst, "nudBigStep"), "value", &vb);
    uiGetFloat(g_inst, uiFindControl(g_inst, "nudPageStep"), "value", &vp);
    snprintf(buf, sizeof(buf), "int=%.0f  float=%.2f  ro=%.0f  big=%.0f  page=%.0f", vi, vf, vo, vb, vp);
    uiSetString(g_inst, status, "caption", buf);
}

static int runTest(const char* shortName, const char* displayName) {
    setbuf(stdout, NULL);
    printf("=== test_numericupdown_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiCreateInstance) { printf("FAIL: GetProcAddress(CreateInstance)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_numericupdown_cabi";
    cfg.windowWidth = 800;
    cfg.windowHeight = 560;
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 800, 560);
    printf("OK: initialized\n");

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 800, "h": 560 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "label",
                        "rect": { "x": 20, "y": 16, "w": 560, "h": 28 },
                        "caption": "NumericUpDown C ABI Test",
                        "font-size": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "lblHint1",
                        "type": "label",
                        "rect": { "x": 20, "y": 56, "w": 200, "h": 20 },
                        "caption": "nudInteger (step=1, range 0~100)",
                        "font-size": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudInteger",
                        "type": "numeric-up-down",
                        "rect": { "x": 20, "y": 78, "w": 180, "h": 32 },
                        "value": 50,
                        "range": { "min": 0, "max": 100 },
                        "step": 1
                    },
                    {
                        "id": "lblHint2",
                        "type": "label",
                        "rect": { "x": 20, "y": 120, "w": 200, "h": 20 },
                        "caption": "nudFloat (step=0.2, decimals=2)",
                        "font-size": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudFloat",
                        "type": "numeric-up-down",
                        "rect": { "x": 20, "y": 142, "w": 180, "h": 32 },
                        "value": 0.6,
                        "range": { "min": 0.0, "max": 1.0 },
                        "step": 0.2,
                        "decimals": 2
                    },
                    {
                        "id": "lblHint3",
                        "type": "label",
                        "rect": { "x": 20, "y": 184, "w": 200, "h": 20 },
                        "caption": "nudReadOnly (42)",
                        "font-size": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudReadOnly",
                        "type": "numeric-up-down",
                        "rect": { "x": 20, "y": 206, "w": 180, "h": 32 },
                        "value": 42,
                        "range": { "min": 0, "max": 100 },
                        "read-only": true
                    },
                    {
                        "id": "lblHint4",
                        "type": "label",
                        "rect": { "x": 20, "y": 248, "w": 200, "h": 20 },
                        "caption": "nudBigStep (step=50, range 0~1000)",
                        "font-size": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudBigStep",
                        "type": "numeric-up-down",
                        "rect": { "x": 20, "y": 270, "w": 180, "h": 32 },
                        "value": 100,
                        "range": { "min": 0, "max": 1000 },
                        "step": 50
                    },
                    {
                        "id": "lblHint5",
                        "type": "label",
                        "rect": { "x": 20, "y": 312, "w": 200, "h": 20 },
                        "caption": "nudPageStep (pageStep=25)",
                        "font-size": 12,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudPageStep",
                        "type": "numeric-up-down",
                        "rect": { "x": 20, "y": 334, "w": 180, "h": 32 },
                        "value": 50,
                        "range": { "min": 0, "max": 1000 },
                        "step": 1,
                        "page-step": 25
                    },
                    {
                        "id": "lblStatus",
                        "type": "label",
                        "rect": { "x": 20, "y": 390, "w": 560, "h": 24 },
                        "caption": "C ABI test: interact with NumericUpDown controls",
                        "font-size": 14,
                        "textColor": [180, 200, 220]
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(g_inst, layoutJson)) {
        printf("FAIL: LoadLayout\n");
        uiDestroyInstance(g_inst);
        FreeLibrary(g_uiDll);
        return 1;
    }
    printf("OK: layout loaded (5 NumericUpDown + labels)\n");

    static const char* nudIds[] = {"nudInteger","nudFloat","nudReadOnly","nudBigStep","nudPageStep"};
    for (int i = 0; i < 5; i++) {
        void* ctl = uiFindControl(g_inst, nudIds[i]);
        if (ctl) uiSetCallback(g_inst, ctl, "value-changed", onNudValueChanged, (void*)nudIds[i]);
    }

    void* nudFloat = uiFindControl(g_inst, "nudFloat");
    if (nudFloat && uiSetFloat) {
        uiSetFloat(g_inst, nudFloat, "value", 0.8f);
        float v = 0.0f;
        uiGetFloat(g_inst, nudFloat, "value", &v);
        printf("OK: nudFloat set to %.2f, get=%.2f\n", 0.8, (double)v);
    }

    void* nudPageStep = uiFindControl(g_inst, "nudPageStep");
    if (nudPageStep && uiSetFloat) {
        uiSetFloat(g_inst, nudPageStep, "page-step", 50.0f);
        printf("OK: nudPageStep pageStep set to 50\n");
    }

    printf("Frame loop... (interact with NumericUpDown controls or close the window)\n");
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
    printf("test_numericupdown_cabi_%s: done\n", shortName);
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}
