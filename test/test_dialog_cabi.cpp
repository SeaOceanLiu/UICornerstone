// =========================================================================
// test_dialog_cabi.cpp -- single fromsource C ABI test for Dialog (all backends)
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
typedef int   (*UISetStringFn)(UIInstance,void*,const char*,const char*);
typedef int   (*UIGetStringFn)(UIInstance,void*,const char*,char*,int);
typedef int   (*UISetColorFn)(UIInstance,void*,const char*,UIColor);
typedef int   (*UISetBoolFn)(UIInstance,void*,const char*,int);
typedef int   (*UISetFloatFn)(UIInstance,void*,const char*,float);
typedef int   (*UIGetFloatFn)(UIInstance,void*,const char*,float*);
typedef void  (*UISetRectFn)(UIInstance,void*,float,float,float,float);
typedef const char*   (*UIGetControlIdFn)(UIInstance,void*);

static UICreateInstanceFn  uiCreateInstance       = nullptr;
static UIDestroyInstanceFn uiDestroyInstance      = nullptr;
static UISetViewportFn     uiSetViewport          = nullptr;
static UIProcessEventsFn   uiProcessEvents        = nullptr;
static UIUpdateFn          uiUpdate               = nullptr;
static UIClearFn           uiClear                = nullptr;
static UIRenderFn          uiRender               = nullptr;
static UIPresentFn         uiPresent              = nullptr;
static UIIsQuitFn          uiIsQuitRequested      = nullptr;
static UIPushUIEventFn     uiPushUIEvent          = nullptr;
static UILoadLayoutFn      uiLoadLayout           = nullptr;
static UIFindControlFn     uiFindControl          = nullptr;
static UIRegisterActionFn  uiRegisterAction       = nullptr;
static UISetStringFn       uiSetString            = nullptr;
static UIGetStringFn       uiGetString            = nullptr;
static UISetColorFn        uiSetColor             = nullptr;
static UISetBoolFn         uiSetBool              = nullptr;
static UISetFloatFn        uiSetFloat             = nullptr;
static UIGetFloatFn        uiGetFloat             = nullptr;
static UISetRectFn         uiSetRect              = nullptr;
static UIGetControlIdFn    uiGetControlId         = nullptr;

static HMODULE g_uiDll = nullptr;
static UIInstance g_inst = nullptr;
static int g_autoSec = 5;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）；默认 5 秒自动关闭

// ===== 当前颜色状态 =====
static int g_r = 255, g_g = 102, g_b = 0, g_a = 255;
// ===== 备份（Cancel 恢复） =====
static int g_savedR = 255, g_savedG = 102, g_savedB = 0, g_savedA = 255;
// ===== Hex 输入防递归 =====
static bool g_updatingHex = false;

// ===== 预设色 =====
static const uint32_t kPresetColors[] = {
    0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF,
    0x00FFFF, 0xFFFFFF, 0x000000, 0x808080, 0xFFA500,
    0x800000, 0x008000, 0x000080, 0x808000, 0x800080,
    0x008080, 0xC0C0C0, 0xE0E0E0, 0xFFC0CB, 0xA52A2A
};

// ===== 工具函数：设置色块颜色 =====
static void setSwatchColor(const char* swatchId, int r, int g, int b, int a = 255) {
    void* sw = uiFindControl(g_inst, swatchId);
    if (sw) uiSetColor(g_inst, sw, "background", UIColor{(uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a});
}

// ===== 更新 hex 输入框文本 =====
static void updateHexInput(int r, int g, int b, int a) {
    void* hexEb = uiFindControl(g_inst, "hexInput");
    if (!hexEb) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", r, g, b, a);
    g_updatingHex = true;
    uiSetString(g_inst, hexEb, "text", buf);
    g_updatingHex = false;
}

// ===== 实时同步 btnSwatch + 设置 dlgSwatch + 更新 hex =====
static void syncColorToAll(int r, int g, int b, int a) {
    setSwatchColor("dlgSwatch", r, g, b);
    setSwatchColor("btnSwatch", r, g, b);
    updateHexInput(r, g, b, a);
}

// ===== 从预设色更新 =====
static void setColorFromPreset(uint32_t color) {
    g_r = (int)((color >> 16) & 0xFF);
    g_g = (int)((color >> 8) & 0xFF);
    g_b = (int)(color & 0xFF);
    g_a = 255;
    uiSetFloat(g_inst, uiFindControl(g_inst, "rSlider"), "value", (float)g_r);
    uiSetFloat(g_inst, uiFindControl(g_inst, "gSlider"), "value", (float)g_g);
    uiSetFloat(g_inst, uiFindControl(g_inst, "bSlider"), "value", (float)g_b);
    uiSetFloat(g_inst, uiFindControl(g_inst, "aSlider"), "value", (float)g_a);
    syncColorToAll(g_r, g_g, g_b, g_a);
}

static int presetIndexFromId(void* ctl) {
    const char* id = uiGetControlId(g_inst, ctl);
    if (!id || id[0] == '\0') return -1;
    if (strncmp(id, "cp_", 3) != 0) return -1;
    int idx = atoi(id + 3);
    return (idx >= 0 && idx < 20) ? idx : -1;
}

static void onPreset(void* ctl, void* user) {
    (void)user;
    int idx = presetIndexFromId(ctl);
    if (idx >= 0) setColorFromPreset(kPresetColors[idx]);
}

// ===== 滑块变化 → 同步所有 UI =====
static void onColorChange(void* ctl, void* user) {
    (void)ctl; (void)user;
    void* rS = uiFindControl(g_inst, "rSlider");
    void* gS = uiFindControl(g_inst, "gSlider");
    void* bS = uiFindControl(g_inst, "bSlider");
    void* aS = uiFindControl(g_inst, "aSlider");
    if (!rS || !gS || !bS || !aS) return;
    float rv, gv, bv, av;
    uiGetFloat(g_inst, rS, "value", &rv);
    uiGetFloat(g_inst, gS, "value", &gv);
    uiGetFloat(g_inst, bS, "value", &bv);
    uiGetFloat(g_inst, aS, "value", &av);
    int r = (int)rv, g = (int)gv, b = (int)bv, a = (int)av;
    setSwatchColor("dlgSwatch", r, g, b);
    setSwatchColor("btnSwatch", r, g, b);
    updateHexInput(r, g, b, a);
}

// ===== Dialog 确定 → 读取滑块值提交 globals + 主界面 Hex 标签 =====
static void onColorConfirmed(void* ctl, void* user) {
    (void)ctl; (void)user;
    void* rS = uiFindControl(g_inst, "rSlider");
    void* gS = uiFindControl(g_inst, "gSlider");
    void* bS = uiFindControl(g_inst, "bSlider");
    void* aS = uiFindControl(g_inst, "aSlider");
    if (!rS || !gS || !bS || !aS) return;
    float rv, gv, bv, av;
    uiGetFloat(g_inst, rS, "value", &rv);
    uiGetFloat(g_inst, gS, "value", &gv);
    uiGetFloat(g_inst, bS, "value", &bv);
    uiGetFloat(g_inst, aS, "value", &av);
    g_r = (int)rv; g_g = (int)gv; g_b = (int)bv; g_a = (int)av;
    g_savedR = g_r; g_savedG = g_g; g_savedB = g_b; g_savedA = g_a;
    char buf[32];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", g_r, g_g, g_b, g_a);
    void* lbl = uiFindControl(g_inst, "lblColor");
    if (lbl) uiSetString(g_inst, lbl, "caption", buf);
}

// ===== 解析 Hex 字符串 → 更新滑块 + swatch =====
static void parseHexAndApply(const char* hex) {
    if (!hex || hex[0] == '\0') return;
    if (hex[0] == '#') hex++;
    int len = (int)strlen(hex);
    int r=-1,g=-1,b=-1,a=255;
    if (len == 6) {
        sscanf_s(hex, "%02x%02x%02x", &r, &g, &b);
    } else if (len == 8) {
        sscanf_s(hex, "%02x%02x%02x%02x", &r, &g, &b, &a);
    }
    if (r<0||g<0||b<0) return;
    g_r = r; g_g = g; g_b = b; g_a = a;
    uiSetFloat(g_inst, uiFindControl(g_inst, "rSlider"), "value", (float)r);
    uiSetFloat(g_inst, uiFindControl(g_inst, "gSlider"), "value", (float)g);
    uiSetFloat(g_inst, uiFindControl(g_inst, "bSlider"), "value", (float)b);
    uiSetFloat(g_inst, uiFindControl(g_inst, "aSlider"), "value", (float)a);
    syncColorToAll(r, g, b, a);
}

static void onHexChanged(void* ctl, void* user) {
    (void)ctl; (void)user;
    if (g_updatingHex) return;
    char textBuf[256] = "";
    uiGetString(g_inst, ctl, "text", textBuf, sizeof(textBuf));
    const char* text = textBuf;
    if (text) parseHexAndApply(text);
}

// ===== 打开 Dialog → 保存当前色 + 同步控件 + 锚定 =====
static void showColorDlg(void*, void*) {
    g_savedR = g_r; g_savedG = g_g; g_savedB = g_b; g_savedA = g_a;
    uiSetFloat(g_inst, uiFindControl(g_inst, "rSlider"), "value", (float)g_r);
    uiSetFloat(g_inst, uiFindControl(g_inst, "gSlider"), "value", (float)g_g);
    uiSetFloat(g_inst, uiFindControl(g_inst, "bSlider"), "value", (float)g_b);
    uiSetFloat(g_inst, uiFindControl(g_inst, "aSlider"), "value", (float)g_a);
    syncColorToAll(g_r, g_g, g_b, g_a);
    void* dlg = uiFindControl(g_inst, "colorDlg");
    if (!dlg) return;
    uiSetRect(g_inst, dlg, 100, 30, 296, 440);
    uiSetBool(g_inst, dlg, "visible", 1);
}

static void restoreFromSaved() {
    g_r = g_savedR; g_g = g_savedG; g_b = g_savedB; g_a = g_savedA;
    setSwatchColor("btnSwatch", g_r, g_g, g_b, g_a);
    char buf[32];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", g_r, g_g, g_b, g_a);
    void* lbl = uiFindControl(g_inst, "lblColor");
    if (lbl) uiSetString(g_inst, lbl, "caption", buf);
}

static void onColorCancelled(void*, void*) { restoreFromSaved(); }
static void onColorClose(void*, void*) { restoreFromSaved(); }

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
    RESOLVE(GetString);
    RESOLVE(SetColor);
    RESOLVE(SetBool);
    RESOLVE(SetFloat);
    RESOLVE(GetFloat);
    RESOLVE(SetRect);
    RESOLVE(GetControlId);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_dialog_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiCreateInstance) { printf("FAIL: GetProcAddress(CreateInstance)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_dialog_cabi";
    cfg.windowWidth = 800;
    cfg.windowHeight = 480;
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 800, 480);
    printf("OK: initialized\n");

    uiRegisterAction(g_inst, "showColorDlg",     showColorDlg,     nullptr);
    uiRegisterAction(g_inst, "onColorChange",    onColorChange,    nullptr);
    uiRegisterAction(g_inst, "onColorConfirmed", onColorConfirmed, nullptr);
    uiRegisterAction(g_inst, "onColorCancelled", onColorCancelled, nullptr);
    uiRegisterAction(g_inst, "onColorClose",     onColorClose,     nullptr);
    uiRegisterAction(g_inst, "onPreset", onPreset, nullptr);
    uiRegisterAction(g_inst, "onHexChanged", onHexChanged, nullptr);

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 800, "h": 480 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "btnSwatch",
                        "type": "button",
                        "rect": { "x": 30, "y": 40, "w": 60, "h": 32 },
                        "colors": { "background": { "normal": "#FF6600FF" } },
                        "border-visible": false,
                        "events": { "onClick": "showColorDlg" }
                    },
                    {
                        "id": "lblColor",
                        "type": "label",
                        "rect": { "x": 100, "y": 44, "w": 240, "h": 24 },
                        "caption": "#FF6600FF",
                        "font-size": 14,
                        "textColor": [200, 200, 200]
                    }
                ]
            }
        ],
        "dialogs": [
            {
                "type": "dialog",
                "id": "colorDlg",
                "centered": true,
                "rect": { "x": 0, "y": 0, "w": 296, "h": 440 },
                "colors": { "background": { "normal": "#E0E0E0FF" } },
                "border-visible": true,
                "confirm-button": { "text": "OK" },
                "cancel-button": { "text": "Cancel" },
                "events": {
                    "onConfirm": "onColorConfirmed",
                    "onCancel": "onColorCancelled",
                    "onClose": "onColorClose"
                },
                "children": [
                    {"id":"cp_00","type":"button","rect":{"x":10,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#FF0000FF","hover":"#FF0000FF","pressed":"#FF0000FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_01","type":"button","rect":{"x":66,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#00FF00FF","hover":"#00FF00FF","pressed":"#00FF00FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_02","type":"button","rect":{"x":122,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#0000FFFF","hover":"#0000FFFF","pressed":"#0000FFFF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_03","type":"button","rect":{"x":178,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#FFFF00FF","hover":"#FFFF00FF","pressed":"#FFFF00FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_04","type":"button","rect":{"x":234,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#FF00FFFF","hover":"#FF00FFFF","pressed":"#FF00FFFF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_05","type":"button","rect":{"x":10,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#00FFFFFF","hover":"#00FFFFFF","pressed":"#00FFFFFF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_06","type":"button","rect":{"x":66,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#FFFFFFFF","hover":"#FFFFFFFF","pressed":"#FFFFFFFF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_07","type":"button","rect":{"x":122,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#000000FF","hover":"#000000FF","pressed":"#000000FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_08","type":"button","rect":{"x":178,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#808080FF","hover":"#808080FF","pressed":"#808080FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_09","type":"button","rect":{"x":234,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#FFA500FF","hover":"#FFA500FF","pressed":"#FFA500FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_10","type":"button","rect":{"x":10,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#800000FF","hover":"#800000FF","pressed":"#800000FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_11","type":"button","rect":{"x":66,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#008000FF","hover":"#008000FF","pressed":"#008000FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_12","type":"button","rect":{"x":122,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#000080FF","hover":"#000080FF","pressed":"#000080FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_13","type":"button","rect":{"x":178,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#808000FF","hover":"#808000FF","pressed":"#808000FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_14","type":"button","rect":{"x":234,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#800080FF","hover":"#800080FF","pressed":"#800080FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_15","type":"button","rect":{"x":10,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#008080FF","hover":"#008080FF","pressed":"#008080FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_16","type":"button","rect":{"x":66,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#C0C0C0FF","hover":"#C0C0C0FF","pressed":"#C0C0C0FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_17","type":"button","rect":{"x":122,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#E0E0E0FF","hover":"#E0E0E0FF","pressed":"#E0E0E0FF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_18","type":"button","rect":{"x":178,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#FFC0CBFF","hover":"#FFC0CBFF","pressed":"#FFC0CBFF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_19","type":"button","rect":{"x":234,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#A52A2AFF","hover":"#A52A2AFF","pressed":"#A52A2AFF"}},"border-visible":false,"events":{"onClick":"onPreset"}},
                    {
                        "id": "dlgSwatch",
                        "type": "button",
                        "rect": { "x": 10, "y": 166, "w": 52, "h": 32 },
                        "colors": { "background": { "normal": "#FF6600FF", "hover": "#FF6600FF", "pressed": "#FF6600FF" } },
                        "border-visible": false
                    },
                    {
                        "id": "hexInput",
                        "type": "edit-box",
                        "rect": { "x": 72, "y": 168, "w": 130, "h": 28 },
                        "font-size": 14,
                        "text": "#FF6600FF",
                        "textColor": [200, 200, 200],
                        "events": { "onTextChanged": "onHexChanged" }
                    },
                    {
                        "id": "lblR",
                        "type": "label",
                        "rect": { "x": 10, "y": 226, "w": 14, "h": 16 },
                        "caption": "R",
                        "font-size": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "rSlider",
                        "type": "slider",
                        "rect": { "x": 29, "y": 224, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 255,
                        "show-value-label": true,
                        "label-gap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    },
                    {
                        "id": "lblG",
                        "type": "label",
                        "rect": { "x": 10, "y": 268, "w": 14, "h": 16 },
                        "caption": "G",
                        "font-size": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "gSlider",
                        "type": "slider",
                        "rect": { "x": 29, "y": 266, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 102,
                        "show-value-label": true,
                        "label-gap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    },
                    {
                        "id": "lblB",
                        "type": "label",
                        "rect": { "x": 10, "y": 310, "w": 14, "h": 16 },
                        "caption": "B",
                        "font-size": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "bSlider",
                        "type": "slider",
                        "rect": { "x": 29, "y": 308, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 0,
                        "show-value-label": true,
                        "label-gap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    },
                    {
                        "id": "lblA",
                        "type": "label",
                        "rect": { "x": 10, "y": 352, "w": 14, "h": 16 },
                        "caption": "A",
                        "font-size": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "aSlider",
                        "type": "slider",
                        "rect": { "x": 29, "y": 350, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 255,
                        "show-value-label": true,
                        "label-gap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(g_inst, layoutJson)) { printf("FAIL: LoadLayout\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");

    printf("Frame loop... (click color swatch or close the window)\n");
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
    printf("test_dialog_cabi_%s: done\n", shortName);
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}
