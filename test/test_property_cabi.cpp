// =========================================================================
// test_property_cabi.cpp -- 缁熶竴灞炴€х郴缁?C ABI 闆嗘垚娴嬭瘯
// 楠岃瘉 SetColor/SetStateColor/SetBool/SetInt/SetFloat/SetString/SetEnum/SetCallback
// + 瀵瑰簲 Getter锛岃鐩?Phase 1~5 鍏ㄩ儴鎺т欢
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
typedef void  (*UIPushUIEventFn)(UIInstance,const UIEvent*);
typedef void  (*UIRegisterActionFn)(UIInstance,const char*,void(*)(void*,void*),void*);
typedef int   (*UISetColorFn)(UIInstance,void*,const char*,UIColor);
typedef int   (*UIGetColorFn)(UIInstance,void*,const char*,UIColor*);
typedef int   (*UISetStateColorFn)(UIInstance,void*,const char*,UIStateColor);
typedef int   (*UIGetStateColorFn)(UIInstance,void*,const char*,UIStateColor*);
typedef int   (*UISetBoolFn)(UIInstance,void*,const char*,int);
typedef int   (*UIGetBoolFn)(UIInstance,void*,const char*,int*);
typedef int   (*UISetIntFn)(UIInstance,void*,const char*,int);
typedef int   (*UIGetIntFn)(UIInstance,void*,const char*,int*);
typedef int   (*UISetFloatFn)(UIInstance,void*,const char*,float);
typedef int   (*UIGetFloatFn)(UIInstance,void*,const char*,float*);
typedef int   (*UISetStringFn)(UIInstance,void*,const char*,const char*);
typedef int   (*UIGetStringFn)(UIInstance,void*,const char*,char*,int);
typedef int   (*UISetEnumFn)(UIInstance,void*,const char*,const char*);
typedef int   (*UIGetEnumFn)(UIInstance,void*,const char*,char*,int);
typedef int   (*UISetCallbackFn)(UIInstance,void*,const char*,UIEventCallback,void*);
typedef int   (*UIGetFontCountFn)(void);
typedef int   (*UIGetFontNameFn)(int,char*,int);
typedef void  (*UIAddChildFn)(UIInstance,void*,void*);
typedef int   (*UIRemoveChildFn)(UIInstance,void*,void*);
typedef int   (*UIGetPtrFn)(UIInstance,void*,const char*,void**);
typedef void  (*UIGetRectFn)(UIInstance,void*,float*,float*,float*,float*);
typedef void* (*UIGetRootFn)(UIInstance);
typedef int   (*UISetCanvasSizeFn)(UIInstance,float,float);

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
static UISetColorFn         uiSetColor             = nullptr;
static UIGetColorFn         uiGetColor             = nullptr;
static UISetStateColorFn    uiSetStateColor        = nullptr;
static UIGetStateColorFn    uiGetStateColor        = nullptr;
static UISetBoolFn          uiSetBool              = nullptr;
static UIGetBoolFn          uiGetBool              = nullptr;
static UISetIntFn           uiSetInt               = nullptr;
static UIGetIntFn           uiGetInt               = nullptr;
static UISetFloatFn         uiSetFloat             = nullptr;
static UIGetFloatFn         uiGetFloat             = nullptr;
static UISetStringFn        uiSetString            = nullptr;
static UIGetStringFn        uiGetString            = nullptr;
static UISetEnumFn          uiSetEnum              = nullptr;
static UIGetEnumFn          uiGetEnum              = nullptr;
static UISetCallbackFn      uiSetCallback          = nullptr;
static UIGetFontCountFn     uiGetFontCount         = nullptr;
static UIGetFontNameFn      uiGetFontName          = nullptr;
static UIAddChildFn         uiAddChildControl      = nullptr;
static UIRemoveChildFn      uiRemoveChild          = nullptr;
static UIGetPtrFn           uiGetPtr               = nullptr;
static UIGetRectFn          uiGetRect              = nullptr;
static UIGetRootFn          uiGetRoot              = nullptr;
static UISetCanvasSizeFn    uiSetCanvasSize        = nullptr;

static HMODULE g_uiDll = nullptr;
static UIInstance g_inst = nullptr;
static int g_autoSec = 0;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）

// ===== 娴嬭瘯缁撴灉缁熻 =====
static int g_passCount = 0;
static int g_failCount = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); g_failCount++; } \
    else         { printf("  PASS: %s\n", msg); g_passCount++; } \
} while(0)

#define CHECK_RET(expr, expected, msg) do { \
    int _ret = (expr); \
    if (_ret != (expected)) { printf("  FAIL: %s (returned %d, expected %d)\n", msg, _ret, (expected)); g_failCount++; } \
    else                    { printf("  PASS: %s\n", msg); g_passCount++; } \
} while(0)

// ===== P0-48：ComboBox 选择回调统计 =====
static int g_comboSelFired = 0;
static int g_comboSelIdx = -1;
static void onComboSelChanged(void* ctl, const UIEventData* ev, void* user) {
    (void)ctl; (void)user;
    g_comboSelFired++;
    g_comboSelIdx = ev ? ev->data.selection.idx : -99;
}

// ===== 浜嬩欢鍥炶皟 =====
static void onEventCallback(void* ctl, const UIEventData* event, void* user) {
    (void)ctl; (void)user;
    printf("  C ABI Callback fired: event=%s\n", event ? event->eventName : "null");
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
    RESOLVE(SetColor);
    RESOLVE(GetColor);
    RESOLVE(SetStateColor);
    RESOLVE(GetStateColor);
    RESOLVE(SetBool);
    RESOLVE(GetBool);
    RESOLVE(SetInt);
    RESOLVE(GetInt);
    RESOLVE(SetFloat);
    RESOLVE(GetFloat);
    RESOLVE(SetString);
    RESOLVE(GetString);
    RESOLVE(SetEnum);
    RESOLVE(GetEnum);
    RESOLVE(SetCallback);
    RESOLVE(GetFontCount);
    RESOLVE(GetFontName);
    RESOLVE(AddChildControl);
    RESOLVE(RemoveChild);
    RESOLVE(GetPtr);
    RESOLVE(GetRect);
    RESOLVE(GetRoot);
    RESOLVE(SetCanvasSize);
#undef RESOLVE
}

static int runPropertyTests(void) {
    printf("\n--- Color property tests ---\n");

    void* slider = uiFindControl(g_inst, "sliderProp");
    void* check  = uiFindControl(g_inst, "checkProp");
    void* prog   = uiFindControl(g_inst, "progProp");
    void* scroll = uiFindControl(g_inst, "scrollProp");
    void* combo  = uiFindControl(g_inst, "comboProp");
    void* nud    = uiFindControl(g_inst, "nudProp");

    CHECK(slider != NULL, "FindControl(sliderProp)");
    CHECK(check  != NULL, "FindControl(checkProp)");
    CHECK(prog   != NULL, "FindControl(progProp)");
    CHECK(scroll != NULL, "FindControl(scrollProp)");
    CHECK(combo  != NULL, "FindControl(comboProp)");
    CHECK(nud    != NULL, "FindControl(nudProp)");

    if (!slider) return 1;

    // 鈹€鈹€ SetColor 鈹€鈹€
    UIColor red   = {255,0,0,255};
    UIColor green = {0,255,0,255};
    UIColor blue  = {0,0,255,255};

    CHECK_RET(uiSetColor(g_inst, slider, "track",         red),   1, "Slider: SetColor(track, red)");
    CHECK_RET(uiSetColor(g_inst, slider, "thumb",         green), 1, "Slider: SetColor(thumb, green)");
    CHECK_RET(uiSetColor(g_inst, slider, "thumb-hover",   blue),  1, "Slider: SetColor(thumb-hover, blue)");
    CHECK_RET(uiSetColor(g_inst, slider, "tick",          red),   1, "Slider: SetColor(tick, red)");
    CHECK_RET(uiSetColor(g_inst, slider, "label",         green), 1, "Slider: SetColor(label, green)");
    CHECK_RET(uiSetColor(g_inst, slider, "invalid-prop",  red),   0, "Slider: SetColor(invalid) -> 0");

    CHECK_RET(uiSetColor(g_inst, check, "check-color",    green), 1, "CheckBox: SetColor(check-color, green)");
    CHECK_RET(uiSetColor(g_inst, check, "cross-color",    red),   1, "CheckBox: SetColor(cross-color, red)");
    CHECK_RET(uiSetColor(g_inst, check, "box-border-color", blue), 1, "CheckBox: SetColor(box-border-color, blue)");

    CHECK_RET(uiSetColor(g_inst, prog,  "background",     red),   1, "ProgressBar: SetColor(background, red)");
    CHECK_RET(uiSetColor(g_inst, prog,  "progress",       green), 1, "ProgressBar: SetColor(progress, green)");

    CHECK_RET(uiSetColor(g_inst, scroll,"track",          red),   1, "ScrollBar: SetColor(track, red)");
    CHECK_RET(uiSetColor(g_inst, scroll,"thumb",          green), 1, "ScrollBar: SetColor(thumb, green)");
    CHECK_RET(uiSetColor(g_inst, scroll,"thumb-hover",    blue),  1, "ScrollBar: SetColor(thumb-hover, blue)");
    CHECK_RET(uiSetColor(g_inst, scroll,"thumb-pressed",  red),   1, "ScrollBar: SetColor(thumb-pressed, red)");

    // 鈹€鈹€ SetStateColor 鈹€鈹€
    UIStateColor sbg;
    sbg.normal     = {40,40,40,255};
    sbg.hover      = {60,60,60,255};
    sbg.pressed    = {80,80,80,255};
    sbg.disabled   = {20,20,20,255};

    CHECK_RET(uiSetStateColor(g_inst, slider, "background", sbg), 1, "Slider: SetStateColor(background)");
    CHECK_RET(uiSetStateColor(g_inst, slider, "border",     sbg), 1, "Slider: SetStateColor(border)");

    // 鈹€鈹€ GetStateColor (閫氱敤 StateColor 灞炴€э紝鐢?ControlImpl 瀹炵幇) 鈹€鈹€
    UIStateColor got;
    memset(&got, 0, sizeof(got));
    CHECK_RET(uiGetStateColor(g_inst, slider, "background", &got), 1, "Slider: GetStateColor(background) -> 1");
    CHECK(got.normal.r == 40 && got.normal.g == 40, "  -> normal.r = 40, normal.g = 40");

    memset(&got, 0, sizeof(got));
    CHECK_RET(uiGetStateColor(g_inst, slider, "invalid", &got), 0, "Slider: GetStateColor(invalid) -> 0");

    printf("\n--- Bool property tests ---\n");

    // 鈹€鈹€ SetBool 鈹€鈹€
    CHECK_RET(uiSetBool(g_inst, slider, "show-value-label", 1), 1, "Slider: SetBool(show-value-label, 1)");
    CHECK_RET(uiSetBool(g_inst, slider, "reverse",          1), 1, "Slider: SetBool(reverse, 1)");
    CHECK_RET(uiSetBool(g_inst, slider, "invalid-bool",     1), 0, "Slider: SetBool(invalid) -> 0");

    // CheckBox 没有 "checked" 独立 bool 属性，checked 状态由 SetEnum(check-state) 控制
    CHECK_RET(uiSetBool(g_inst, check,  "tri-state",        0), 1, "CheckBox: SetBool(tri-state, 0)");

    // 鈹€鈹€ GetBool (閫氱敤 Bool 灞炴€х敱 ControlImpl 澶勭悊) 鈹€鈹€
    int bval = -1;
    CHECK_RET(uiGetBool(g_inst, slider, "visible", &bval), 1, "Slider: GetBool(visible) -> 1");
    CHECK(bval == 1, "  -> visible = 1");

    bval = -1;
    CHECK_RET(uiGetBool(g_inst, slider, "invalid", &bval), 0, "Slider: GetBool(invalid) -> 0");

    printf("\n--- Int property tests ---\n");

    // 鈹€鈹€ SetInt 鈹€鈹€
    CHECK_RET(uiSetInt(g_inst, combo, "selected-index", 2),  1, "ComboBox: SetInt(selected-index, 2)");
    CHECK_RET(uiSetInt(g_inst, combo, "max-visible-items", 5), 1, "ComboBox: SetInt(max-visible-items, 5)");
    CHECK_RET(uiSetInt(g_inst, combo, "invalid-int", 0),      0, "ComboBox: SetInt(invalid) -> 0");

    // NumericUpDown 只接受 "decimals" 作为 int 属性
    CHECK_RET(uiSetInt(g_inst, nud,   "decimals", 2),          1, "NumericUpDown: SetInt(decimals, 2)");

    // 控件特有 getter 现在已实现
    int ival = -1;
    CHECK_RET(uiGetInt(g_inst, combo, "selected-index", &ival), 1, "ComboBox: GetInt(selected-index) -> 1");
    CHECK(ival == 2, "  -> selected-index = 2");

    ival = -1;
    CHECK_RET(uiGetInt(g_inst, combo, "invalid", &ival), 0, "ComboBox: GetInt(invalid) -> 0");

    printf("\n--- Float property tests ---\n");

    // 鈹€鈹€ SetFloat 鈹€鈹€
    CHECK_RET(uiSetFloat(g_inst, slider, "step",             5.0f),  1, "Slider: SetFloat(step, 5.0)");
    CHECK_RET(uiSetFloat(g_inst, slider, "track-thickness",  8.0f),  1, "Slider: SetFloat(track-thickness, 8.0)");
    CHECK_RET(uiSetFloat(g_inst, slider, "value",            50.0f), 1, "Slider: SetFloat(value, 50.0)");
    CHECK_RET(uiSetFloat(g_inst, slider, "invalid-float",    0.0f),  0, "Slider: SetFloat(invalid) -> 0");

    CHECK_RET(uiSetFloat(g_inst, scroll, "value",    42.0f), 1, "ScrollBar: SetFloat(value, 42.0)");
    CHECK_RET(uiSetFloat(g_inst, scroll, "page-size",20.0f), 1, "ScrollBar: SetFloat(page-size, 20.0)");
    CHECK_RET(uiSetFloat(g_inst, scroll, "range-min",0.0f),  1, "ScrollBar: SetFloat(range-min, 0.0)");
    CHECK_RET(uiSetFloat(g_inst, scroll, "range-max",200.0f),1, "ScrollBar: SetFloat(range-max, 200.0)");
    CHECK_RET(uiSetFloat(g_inst, scroll, "step-size", 5.0f), 1, "ScrollBar: SetFloat(step-size, 5.0)");

    CHECK_RET(uiSetFloat(g_inst, prog,   "value", 75.0f), 1, "ProgressBar: SetFloat(value, 75.0)");
    CHECK_RET(uiSetFloat(g_inst, nud,    "step",  2.0f),  1, "NumericUpDown: SetFloat(step, 2.0)");

    // 鈹€鈹€ GetFloat 鈹€鈹€
    // 控件特有 getter 现在已实现
    float fval = -1.0f;
    CHECK_RET(uiGetFloat(g_inst, slider, "value", &fval), 1, "Slider: GetFloat(value) -> 1");
    CHECK(fval > 49.0f && fval < 51.0f, "  -> value ~ 50.0");
    CHECK_RET(uiGetFloat(g_inst, slider, "invalid", &fval), 0, "Slider: GetFloat(invalid) -> 0");

    printf("\n--- String property tests ---\n");

    // 鈹€鈹€ SetString 鈹€鈹€
    CHECK_RET(uiSetString(g_inst, slider, "label-format", "%.0f px"), 1, "Slider: SetString(label-format, '%.0f px')");
    CHECK_RET(uiSetString(g_inst, slider, "invalid-str",   "x"),       0, "Slider: SetString(invalid) -> 0");

    CHECK_RET(uiSetString(g_inst, prog,   "custom-text", "%.0f%%"),   1, "ProgressBar: SetString(custom-text, '%.0f%%')");

    // 控件特有 getter 现在已实现
    char sbuf[128];
    memset(sbuf, 0, sizeof(sbuf));
    CHECK_RET(uiGetString(g_inst, slider, "label-format", sbuf, (int)sizeof(sbuf)), 1, "Slider: GetString(label-format) -> 1");
    CHECK(strcmp(sbuf, "%.0f px") == 0, "  -> label-format = '%.0f px'");
    memset(sbuf, 0, sizeof(sbuf));
    CHECK_RET(uiGetString(g_inst, slider, "invalid", sbuf, (int)sizeof(sbuf)), 0, "Slider: GetString(invalid) -> 0");

    printf("\n--- Enum property tests ---\n");

    // 鈹€鈹€ SetEnum 鈹€鈹€
    CHECK_RET(uiSetEnum(g_inst, slider, "style", "horizontal"), 1, "Slider: SetEnum(style, horizontal)");
    CHECK_RET(uiSetEnum(g_inst, slider, "style", "vertical"),   1, "Slider: SetEnum(style, vertical)");
    CHECK_RET(uiSetEnum(g_inst, slider, "style", "invalid"),    0, "Slider: SetEnum(style, invalid) -> 0");
    CHECK_RET(uiSetEnum(g_inst, slider, "invalid-enum", "x"),   0, "Slider: SetEnum(invalid-enum, ...) -> 0");

    CHECK_RET(uiSetEnum(g_inst, check,  "style", "auto-check"), 1, "CheckBox: SetEnum(style, auto-check)");
    CHECK_RET(uiSetEnum(g_inst, check,  "check-state", "mixed"), 1, "CheckBox: SetEnum(check-state, mixed)");

    CHECK_RET(uiSetEnum(g_inst, prog,   "style", "mario"),      1, "ProgressBar: SetEnum(style, mario)");
    CHECK_RET(uiSetEnum(g_inst, prog,   "text-mode", "value"),  1, "ProgressBar: SetEnum(text-mode, value)");

    CHECK_RET(uiSetEnum(g_inst, scroll, "orientation", "horizontal"), 1, "ScrollBar: SetEnum(orientation, horizontal)");
    CHECK_RET(uiSetEnum(g_inst, scroll, "orientation", "vertical"),   1, "ScrollBar: SetEnum(orientation, vertical)");
    CHECK_RET(uiSetEnum(g_inst, scroll, "orientation", "invalid"),    0, "ScrollBar: SetEnum(orientation, invalid) -> 0");

    // 控件特有 enum getter 尚未实现 (Phase 4 待完成)
    char ebuf[64];
    memset(ebuf, 0, sizeof(ebuf));
    CHECK_RET(uiGetEnum(g_inst, slider, "style", ebuf, (int)sizeof(ebuf)), 1, "Slider: GetEnum(style) -> 1");
    CHECK(strcmp(ebuf, "vertical") == 0, "  -> style = 'vertical'");

    memset(ebuf, 0, sizeof(ebuf));
    CHECK_RET(uiGetEnum(g_inst, slider, "invalid", ebuf, (int)sizeof(ebuf)), 0, "Slider: GetEnum(invalid) -> 0");

    printf("\n--- Callback property tests ---\n");

    // setCallbackProperty 现已在 ControlImpl 中实现通用存储
    CHECK_RET(uiSetCallback(g_inst, slider, "value-changed", onEventCallback, NULL), 1, "Slider: SetCallback(value-changed) -> 1");
    CHECK_RET(uiSetCallback(g_inst, slider, "invalid-event", onEventCallback, NULL), 1, "Slider: SetCallback(invalid-event) -> 1");

    // 鈹€鈹€ P0-48 鎵╁睍鈶? 瀛椾綋鏋氫妇娓呭崟 API 鈹€鈹€
    // P0-49: text-shadow per-state read-back (label/button four states)
    printf("\n--- text-shadow per-state read-back tests ---\n");
    {
        void* shLbl = uiFindControl(g_inst, "lblTitle");
        void* shBtn = uiFindControl(g_inst, "btnProp");
        CHECK(shLbl != NULL && shBtn != NULL, "FindControl(label/button) for shadow");
        const char* shKeys[4] = {"text-shadow", "text-shadow.hover", "text-shadow.pressed", "text-shadow.disabled"};
        UIColor shCols[4] = {{40,41,42,255}, {50,51,52,255}, {60,61,62,255}, {70,71,72,255}};
        for (int i = 0; i < 4; ++i) {
            char msg[96];
            if (shLbl) {
                snprintf(msg, sizeof(msg), "label SetColor(%s)", shKeys[i]);
                CHECK_RET(uiSetColor(g_inst, shLbl, shKeys[i], shCols[i]), 1, msg);
                UIColor o = {0,0,0,0};
                int rg = uiGetColor(g_inst, shLbl, shKeys[i], &o);
                snprintf(msg, sizeof(msg), "label GetColor(%s) round-trip", shKeys[i]);
                CHECK(rg == 1 && o.r == shCols[i].r && o.g == shCols[i].g && o.b == shCols[i].b, msg);
            }
            if (shBtn) {
                char msg[96];
                snprintf(msg, sizeof(msg), "button SetColor(%s)", shKeys[i]);
                CHECK_RET(uiSetColor(g_inst, shBtn, shKeys[i], shCols[i]), 1, msg);
                UIColor o = {0,0,0,0};
                int rg = uiGetColor(g_inst, shBtn, shKeys[i], &o);
                snprintf(msg, sizeof(msg), "button GetColor(%s) round-trip", shKeys[i]);
                CHECK(rg == 1 && o.r == shCols[i].r && o.g == shCols[i].g && o.b == shCols[i].b, msg);
            }
        }
    }

    // P0-49: WinFrame background per-state round-trip (single keys 1:1 + object path)
    printf("\n--- WinFrame background per-state tests ---\n");
    {
        void* shWf = uiFindControl(g_inst, "wfProp");
        CHECK(shWf != NULL, "FindControl(wfProp)");
        if (shWf) {
            UIColor c1 = {11,22,33,255}; UIColor o1 = {0,0,0,0};
            CHECK_RET(uiSetColor(g_inst, shWf, "background.pressed", c1), 1, "WF SetColor(background.pressed)");
            CHECK_RET(uiGetColor(g_inst, shWf, "background.pressed", &o1), 1, "WF GetColor(background.pressed)");
            CHECK(o1.r == 11 && o1.g == 22 && o1.b == 33, "WF background.pressed round-trip");
            UIColor c2 = {44,55,66,255}; UIColor o2 = {0,0,0,0};
            CHECK_RET(uiSetColor(g_inst, shWf, "background.disabled", c2), 1, "WF SetColor(background.disabled)");
            CHECK_RET(uiGetColor(g_inst, shWf, "background.disabled", &o2), 1, "WF GetColor(background.disabled)");
            CHECK(o2.r == 44 && o2.g == 55 && o2.b == 66, "WF background.disabled round-trip");

            UIStateColor in;
            in.normal = {100,1,1,255}; in.hover = {120,2,2,255};
            in.pressed = {130,3,3,255}; in.disabled = {140,4,4,255};
            CHECK_RET(uiSetStateColor(g_inst, shWf, "background", in), 1, "WF SetStateColor(background)");
            UIStateColor out; memset(&out, 0, sizeof(out));
            CHECK_RET(uiGetStateColor(g_inst, shWf, "background", &out), 1, "WF GetStateColor(background)");
            CHECK(out.normal.r == 100 && out.hover.r == 120 && out.pressed.r == 100 && out.disabled.r == 100,
                  "WF object path round-trip (pressed/disabled collapsed to normal)");
        }
    }

    // P0-50: container child mount (reparent / remove-keepalive / guards)
    printf("\n--- Container child mount tests ---\n");
    {
        void* root = uiFindControl(g_inst, "rootPanel");
        void* mlb  = uiFindControl(g_inst, "lblTitle");
        void* wfC  = uiFindControl(g_inst, "wfProp");
        void* client = NULL;
        if (wfC && uiGetPtr) uiGetPtr(g_inst, wfC, "client-panel", &client);
        CHECK(root != NULL && mlb != NULL && wfC != NULL && client != NULL, "FindControl(root/label/wf/client)");
        if (root && mlb && wfC && client && uiAddChildControl && uiRemoveChild) {
            CHECK_RET(uiRemoveChild(g_inst, root, mlb), 1, "RemoveChild(root,label) -> 1");
            CHECK_RET(uiRemoveChild(g_inst, root, mlb), 0, "RemoveChild again -> 0 (detached)");
            uiAddChildControl(g_inst, root, mlb);
            CHECK_RET(uiRemoveChild(g_inst, root, mlb), 1, "re-add then RemoveChild -> 1 (keep-alive)");

            uiAddChildControl(g_inst, client, mlb);
            CHECK_RET(uiRemoveChild(g_inst, root, mlb), 0, "cross-container: old parent released (C1)");
            CHECK_RET(uiRemoveChild(g_inst, client, mlb), 1, "cross-container: new container owns child");

            uiAddChildControl(g_inst, root, mlb);
            CHECK_RET(uiRemoveChild(g_inst, root, mlb), 1, "restore to root");
            uiAddChildControl(g_inst, root, mlb);   // 复位（后续帧循环视觉不变）

            CHECK_RET(uiRemoveChild(g_inst, root, root), 0, "RemoveChild(self) -> 0");
            CHECK_RET(uiRemoveChild(g_inst, mlb, root), 0, "RemoveChild(non-parent) -> 0");
        }
    }

    // P0-59: runtime layout mode switch + anchor read-back
    printf("\n--- Runtime layout & anchor tests ---\n");
    {
        void* rt = uiFindControl(g_inst, "rootPanel");
        void* la = uiFindControl(g_inst, "lblTitle");
        CHECK(rt != NULL && la != NULL, "FindControl(root/label) for layout tests");
        if (rt && la && uiGetRect) {
            float x0 = 0.f, y0 = 0.f, w0 = 0.f, h0 = 0.f;
            uiGetRect(g_inst, la, &x0, &y0, &w0, &h0);
            char abuf[32] = {0};
            CHECK_RET(uiSetString(g_inst, rt, "child-id", "lblTitle"), 1, "Set child-id");
            CHECK_RET(uiSetEnum(g_inst, rt, "anchor", "top-right"), 1, "Set child anchor");
            memset(abuf, 0, sizeof(abuf));
            CHECK_RET(uiGetEnum(g_inst, rt, "anchor", abuf, (int)sizeof(abuf)), 1, "Get child anchor");
            CHECK(strcmp(abuf, "top-right") == 0, "anchor read-back == top-right");
            CHECK_RET(uiSetFloat(g_inst, rt, "anchor-offset-x", 12.0f), 1, "Set anchor-offset-x");
            float ox = 0.f;
            CHECK_RET(uiGetFloat(g_inst, rt, "anchor-offset-x", &ox), 1, "Get anchor-offset-x");
            CHECK(ox == 12.0f, "anchor-offset-x read-back");

            char lbuf[32] = {0};
            CHECK_RET(uiGetEnum(g_inst, rt, "layout", lbuf, (int)sizeof(lbuf)), 1, "Get layout (before)");
            CHECK(strcmp(lbuf, "absolute") == 0, "layout default == absolute");
            CHECK_RET(uiSetEnum(g_inst, rt, "layout", "anchor"), 1, "Switch to anchor layout");
            memset(lbuf, 0, sizeof(lbuf));
            uiGetEnum(g_inst, rt, "layout", lbuf, (int)sizeof(lbuf));
            CHECK(strcmp(lbuf, "anchor") == 0, "layout read-back == anchor");
            float x1 = 0.f, y1 = 0.f, w1 = 0.f, h1 = 0.f;
            uiGetRect(g_inst, la, &x1, &y1, &w1, &h1);
            CHECK(x1 != x0 || y1 != y0, "anchor layout repositions child");
            CHECK_RET(uiSetEnum(g_inst, rt, "layout", "absolute"), 1, "Switch back to absolute");
            memset(lbuf, 0, sizeof(lbuf));
            uiGetEnum(g_inst, rt, "layout", lbuf, (int)sizeof(lbuf));
            CHECK(strcmp(lbuf, "absolute") == 0, "layout read-back == absolute (engine cleared)");

            // ---- P0-60: anchor setter immediate reflow + sparse + offset-only no activation ----
            void* bt = uiFindControl(g_inst, "btnProp");
            CHECK(bt != NULL, "FindControl(btnProp) for P0-60");
            if (bt) {
                float bx0 = 0.f, by0 = 0.f, bw0 = 0.f, bh0 = 0.f;
                float bx1 = 0.f, by1 = 0.f, bw1 = 0.f, bh1 = 0.f;
                // sparse: entering anchor layout keeps unanchored child rect
                uiGetRect(g_inst, bt, &bx0, &by0, &bw0, &bh0);
                CHECK_RET(uiSetEnum(g_inst, rt, "layout", "anchor"), 1, "P0-60 re-enter anchor layout");
                uiGetRect(g_inst, bt, &bx1, &by1, &bw1, &bh1);
                CHECK(bx0 == bx1 && by0 == by1, "P0-60 sparse: unanchored child keeps rect");

                // anchor change reflows immediately (lblTitle anchored top-right -> bottom-stretch)
                float lx0 = 0.f, ly0 = 0.f, lw0 = 0.f, lh0 = 0.f;
                float lx1 = 0.f, ly1 = 0.f, lw1 = 0.f, lh1 = 0.f;
                uiGetRect(g_inst, la, &lx0, &ly0, &lw0, &lh0);
                CHECK_RET(uiSetEnum(g_inst, rt, "anchor", "bottom-stretch"), 1, "P0-60 set anchor bottom-stretch");
                uiGetRect(g_inst, la, &lx1, &ly1, &lw1, &lh1);
                CHECK(lx1 != lx0 || ly1 != ly0 || lw1 != lw0 || lh1 != lh0, "P0-60 anchor change reflows immediately");
                // offset change reflows immediately
                uiGetRect(g_inst, la, &lx0, &ly0, &lw0, &lh0);
                CHECK_RET(uiSetFloat(g_inst, rt, "anchor-offset-x", 6.0f), 1, "P0-60 set offset-x");
                uiGetRect(g_inst, la, &lx1, &ly1, &lw1, &lh1);
                CHECK(lx1 != lx0 || ly1 != ly0 || lw1 != lw0 || lh1 != lh0, "P0-60 offset change reflows immediately");

                // offset-only does not activate btnProp
                uiGetRect(g_inst, bt, &bx0, &by0, &bw0, &bh0);
                CHECK_RET(uiSetString(g_inst, rt, "child-id", "btnProp"), 1, "P0-60 target btnProp");
                CHECK_RET(uiSetFloat(g_inst, rt, "anchor-offset-x", 25.0f), 1, "P0-60 offset-only set");
                uiGetRect(g_inst, bt, &bx1, &by1, &bw1, &bh1);
                CHECK(bx0 == bx1 && by0 == by1, "P0-60 offset-only does not activate anchor");
                char ab2[32] = {0};
                CHECK_RET(uiGetEnum(g_inst, rt, "anchor", ab2, (int)sizeof(ab2)), 0, "P0-60 anchor read-back unset for offset-only");
                CHECK_RET(uiSetEnum(g_inst, rt, "anchor", "bottom-right"), 1, "P0-60 activate btnProp anchor");
                uiGetRect(g_inst, bt, &bx0, &by0, &bw0, &bh0);
                CHECK(bx0 != bx1 || by0 != by1, "P0-60 anchor activation reflows immediately");

                // restore
                uiSetEnum(g_inst, rt, "layout", "absolute");
                uiSetString(g_inst, rt, "child-id", "lblTitle");
            }
        }
    }

    // P0-61: root handle + immediate canvas apply + root anchoring
    printf("\n--- Root anchor tests ---\n");
    {
        void* root = (uiGetRoot) ? uiGetRoot(g_inst) : NULL;
        CHECK(root != NULL, "P0-61 GetRoot");
        if (root && uiGetRect && uiSetCanvasSize) {
            float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
            CHECK_RET(uiSetCanvasSize(g_inst, 400.0f, 300.0f), 1, "P0-61 SetCanvasSize immediate");
            uiGetRect(g_inst, root, &x, &y, &w, &h);
            CHECK(x == 0.f && y == 0.f && w == 400.f && h == 300.f, "P0-61 root rect == canvas (immediate)");
            CHECK_RET(uiSetEnum(g_inst, root, "layout", "anchor"), 1, "P0-61 root layout=anchor");
            CHECK_RET(uiSetString(g_inst, root, "child-id", "rootPanel"), 1, "P0-61 root child-id");
            CHECK_RET(uiSetEnum(g_inst, root, "anchor", "fill"), 1, "P0-61 rootPanel anchor=fill");
            void* rp = uiFindControl(g_inst, "rootPanel");
            CHECK(rp != NULL, "P0-61 find rootPanel");
            uiGetRect(g_inst, rp, &x, &y, &w, &h);
            CHECK(w == 400.f && h == 300.f, "P0-61 rootPanel follows canvas (fill)");
            CHECK_RET(uiSetCanvasSize(g_inst, 420.0f, 310.0f), 1, "P0-61 canvas resize");
            uiGetRect(g_inst, rp, &x, &y, &w, &h);
            CHECK(w == 420.f && h == 310.f, "P0-61 anchored rootPanel follows immediately");
            uiSetCanvasSize(g_inst, 540.0f, 520.0f);
            uiSetEnum(g_inst, root, "layout", "absolute");
        }
    }

    printf("\n--- Font list API tests ---\n");
    CHECK(uiGetFontCount && uiGetFontCount() == 6, "GetFontCount() == 6");
    char fname[64] = {0};
    CHECK_RET(uiGetFontName ? uiGetFontName(0, fname, (int)sizeof(fname)) : 0, 1, "GetFontName(0)");
    CHECK(strcmp(fname, "asul-bold") == 0, "GetFontName(0) == asul-bold");
    {
        bool allOk = true;
        int fc = uiGetFontCount ? uiGetFontCount() : 0;
        for (int i = 0; i < fc; ++i) {
            char n[64] = {0};
            if (!uiGetFontName || !uiGetFontName(i, n, (int)sizeof(n)) || n[0] == 0) allOk = false;
        }
        CHECK(allOk, "GetFontName(0..n) all valid");
    }
    CHECK_RET(uiGetFontName ? uiGetFontName(-1, fname, (int)sizeof(fname)) : 0, 0, "GetFontName(-1) -> 0");
    CHECK_RET(uiGetFontName ? uiGetFontName(99, fname, (int)sizeof(fname)) : 0, 0, "GetFontName(out of range) -> 0");

    // 鈹€鈹€ P0-47鈶? Slider font 璇诲洖锛堢己鐪?label 瀛椾綋鍚嶏級 鈹€鈹€
    printf("\n--- Slider font read-back tests ---\n");
    {
        char sf[64] = {0};
        CHECK_RET(uiGetEnum(g_inst, slider, "font", sf, (int)sizeof(sf)), 1, "Slider GetEnum(font) default");
        CHECK(strcmp(sf, "harmonyos-sans-sc-regular") == 0, "Slider default font == harmonyos-sans-sc-regular");
        CHECK_RET(uiSetString(g_inst, slider, "font", "muyao-softbrush"), 1, "Slider SetString(font, muyao-softbrush)");
        memset(sf, 0, sizeof(sf));
        CHECK_RET(uiGetString(g_inst, slider, "font", sf, (int)sizeof(sf)), 1, "Slider GetString(font)");
        CHECK(strcmp(sf, "muyao-softbrush") == 0, "Slider font read-back matches");
    }

    // 鈹€鈹€ P0-48锛歳elected-index 绋嬪簭鍖栬Е鍙戯紙鍙樻洿瀹堝崼锛? 鈹€鈹€
    printf("\n--- ComboBox selection trigger tests ---\n");
    g_comboSelFired = 0; g_comboSelIdx = -1;
    CHECK_RET(uiSetCallback(g_inst, combo, "selection-changed", onComboSelChanged, NULL), 1, "ComboBox SetCallback(selection-changed)");
    CHECK_RET(uiSetInt(g_inst, combo, "selected-index", 1), 1, "ComboBox SetInt(selected-index,1)");
    CHECK(g_comboSelFired == 1 && g_comboSelIdx == 1, "selection-changed fired on change");
    CHECK_RET(uiSetInt(g_inst, combo, "selected-index", 1), 1, "ComboBox SetInt(same value)");
    CHECK(g_comboSelFired == 1, "same value does not re-fire (guard)");
    CHECK_RET(uiSetInt(g_inst, combo, "selected-index", 2), 1, "ComboBox SetInt(selected-index,2)");
    CHECK(g_comboSelFired == 2 && g_comboSelIdx == 2, "selection-changed fired again on change");

    printf("\n========================================\n");
    printf("Property test results: %d passed, %d failed\n", g_passCount, g_failCount);
    printf("========================================\n");

    return (g_failCount > 0) ? 1 : 0;
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_property_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiCreateInstance) { printf("FAIL: GetProcAddress(CreateInstance)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_property_cabi";
    cfg.windowWidth = 540;
    cfg.windowHeight = 520;
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 540, 520);
    printf("OK: initialized\n");
    fflush(stdout);

    // JSON 甯冨眬鍖呭惈鍏ㄩ儴寰呮祴鎺т欢
    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 540, "h": 520 },
                "colors": { "background": { "normal": "#1E1E2EFF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "label",
                        "rect": { "x": 20, "y": 12, "w": 500, "h": 28 },
                        "caption": "Property System C ABI Test",
                        "font-size": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "sliderProp",
                        "type": "slider",
                        "rect": { "x": 20, "y": 60, "w": 200, "h": 40 }
                    },
                    {
                        "id": "checkProp",
                        "type": "check-box",
                        "rect": { "x": 20, "y": 120, "w": 160, "h": 28 },
                        "caption": "Test checkbox"
                    },
                    {
                        "id": "progProp",
                        "type": "progress-bar",
                        "rect": { "x": 20, "y": 168, "w": 200, "h": 24 }
                    },
                    {
                        "id": "scrollProp",
                        "type": "scroll-bar",
                        "rect": { "x": 20, "y": 212, "w": 200, "h": 18 }
                    },
                    {
                        "id": "comboProp",
                        "type": "combo-box",
                        "rect": { "x": 20, "y": 256, "w": 200, "h": 32 },
                        "items": [
                            { "label": "A", "value": "a" },
                            { "label": "B", "value": "b" },
                            { "label": "C", "value": "c" }
                        ]
                    },
                    {
                        "id": "nudProp",
                        "type": "numeric-up-down",
                        "rect": { "x": 20, "y": 308, "w": 160, "h": 32 }
                    },
                    {
                        "id": "btnProp",
                        "type": "button",
                        "rect": { "x": 240, "y": 60, "w": 160, "h": 32 },
                        "caption": "Shadow test"
                    },
                    {
                        "id": "wfProp",
                        "type": "win-frame",
                        "rect": { "x": 240, "y": 110, "w": 260, "h": 150 },
                        "title": "WF shadow test"
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(g_inst, layoutJson)) { printf("FAIL: LoadLayout\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");
    fflush(stdout);

    int ret = runPropertyTests();
    fflush(stdout);

    printf("\nProperty test results: %d passed, %d failed\n", g_passCount, g_failCount);
    fflush(stdout);

    printf("Frame loop... (interact with the controls or close the window)\n");
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
    printf("test_property_cabi_%s: done (return %d)\n", shortName, ret);
    return ret;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}

