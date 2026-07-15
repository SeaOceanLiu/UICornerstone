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
typedef void          (*UISetTextFn)(void*,const char*);
typedef const char*   (*UIGetTextFn)(void*);
typedef void  (*UIShowFn)(void*);
typedef void  (*UICloseFn)(void*);
typedef void  (*UISetBGColorFn)(void*,uint8_t,uint8_t,uint8_t,uint8_t);
typedef float         (*UIGetSliderValueFn)(void*);
typedef void          (*UISetSliderValueFn)(void*,float);
typedef const char*   (*UIGetControlIdFn)(void*);
typedef void  (*UISetDialogPositionFn)(void*,float,float,float,float);

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
static UIShowFn             uiShow                 = nullptr;
static UICloseFn            uiClose                = nullptr;
static UISetBGColorFn       uiSetBGColor           = nullptr;
static UIGetSliderValueFn       uiGetSliderValue       = nullptr;
static UISetSliderValueFn       uiSetSliderValue       = nullptr;
static UISetDialogPositionFn    uiSetDialogPosition    = nullptr;
static UIGetControlIdFn         uiGetControlId         = nullptr;

static HMODULE g_uiDll = nullptr;

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
    void* sw = uiFindControl(swatchId);
    if (sw) uiSetBGColor(sw, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
}

// ===== 更新 hex 输入框文本 =====
static void updateHexInput(int r, int g, int b, int a) {
    void* hexEb = uiFindControl("hexInput");
    if (!hexEb) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", r, g, b, a);
    g_updatingHex = true;
    uiSetText(hexEb, buf);
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
    uiSetSliderValue(uiFindControl("rSlider"), (float)g_r);
    uiSetSliderValue(uiFindControl("gSlider"), (float)g_g);
    uiSetSliderValue(uiFindControl("bSlider"), (float)g_b);
    uiSetSliderValue(uiFindControl("aSlider"), (float)g_a);
    syncColorToAll(g_r, g_g, g_b, g_a);
}

static int presetIndexFromId(void* ctl) {
    const char* id = uiGetControlId(ctl);
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
    void* rS = uiFindControl("rSlider");
    void* gS = uiFindControl("gSlider");
    void* bS = uiFindControl("bSlider");
    void* aS = uiFindControl("aSlider");
    if (!rS || !gS || !bS || !aS) return;
    int r = (int)uiGetSliderValue(rS);
    int g = (int)uiGetSliderValue(gS);
    int b = (int)uiGetSliderValue(bS);
    int a = (int)uiGetSliderValue(aS);
    setSwatchColor("dlgSwatch", r, g, b);
    setSwatchColor("btnSwatch", r, g, b);
    updateHexInput(r, g, b, a);
}

// ===== Dialog 确定 → 读取滑块值提交 globals + 主界面 Hex 标签 =====
static void onColorConfirmed(void* ctl, void* user) {
    (void)ctl; (void)user;
    void* rS = uiFindControl("rSlider");
    void* gS = uiFindControl("gSlider");
    void* bS = uiFindControl("bSlider");
    void* aS = uiFindControl("aSlider");
    if (!rS || !gS || !bS || !aS) return;
    g_r = (int)uiGetSliderValue(rS);
    g_g = (int)uiGetSliderValue(gS);
    g_b = (int)uiGetSliderValue(bS);
    g_a = (int)uiGetSliderValue(aS);
    g_savedR = g_r; g_savedG = g_g; g_savedB = g_b; g_savedA = g_a;
    char buf[32];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", g_r, g_g, g_b, g_a);
    void* lbl = uiFindControl("lblColor");
    if (lbl) uiSetText(lbl, buf);
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
    uiSetSliderValue(uiFindControl("rSlider"), (float)r);
    uiSetSliderValue(uiFindControl("gSlider"), (float)g);
    uiSetSliderValue(uiFindControl("bSlider"), (float)b);
    uiSetSliderValue(uiFindControl("aSlider"), (float)a);
    syncColorToAll(r, g, b, a);
}

static void onHexChanged(void* ctl, void* user) {
    (void)ctl; (void)user;
    if (g_updatingHex) return;
    const char* text = uiGetText(ctl);
    if (text) parseHexAndApply(text);
}

// ===== 打开 Dialog → 保存当前色 + 同步控件 + 锚定 =====
static void showColorDlg(void*, void*) {
    g_savedR = g_r; g_savedG = g_g; g_savedB = g_b; g_savedA = g_a;
    uiSetSliderValue(uiFindControl("rSlider"), (float)g_r);
    uiSetSliderValue(uiFindControl("gSlider"), (float)g_g);
    uiSetSliderValue(uiFindControl("bSlider"), (float)g_b);
    uiSetSliderValue(uiFindControl("aSlider"), (float)g_a);
    syncColorToAll(g_r, g_g, g_b, g_a);
    void* dlg = uiFindControl("colorDlg");
    if (!dlg) return;
    uiSetDialogPosition(dlg, 100, 30, 296, 440);
    uiShow(dlg);
}

static void restoreFromSaved() {
    g_r = g_savedR; g_g = g_savedG; g_b = g_savedB; g_a = g_savedA;
    setSwatchColor("btnSwatch", g_r, g_g, g_b, g_a);
    char buf[32];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", g_r, g_g, g_b, g_a);
    void* lbl = uiFindControl("lblColor");
    if (lbl) uiSetText(lbl, buf);
}

static void onColorCancelled(void*, void*) { restoreFromSaved(); }
static void onColorClose(void*, void*) { restoreFromSaved(); }

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
    RESOLVE(Show);
    RESOLVE(Close);
    RESOLVE(SetBGColor);
    RESOLVE(GetSliderValue);
    RESOLVE(SetSliderValue);
    RESOLVE(SetDialogPosition);
    RESOLVE(GetControlId);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_dialog_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiInit) { printf("FAIL: GetProcAddress(Init)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    if (!uiInit(callbacks)) { printf("FAIL: Init\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(0, 0, 800, 480);
    printf("OK: initialized\n");

    uiRegisterAction("showColorDlg",     showColorDlg,     nullptr);
    uiRegisterAction("onColorChange",    onColorChange,    nullptr);
    uiRegisterAction("onColorConfirmed", onColorConfirmed, nullptr);
    uiRegisterAction("onColorCancelled", onColorCancelled, nullptr);
    uiRegisterAction("onColorClose",     onColorClose,     nullptr);
    uiRegisterAction("onPreset", onPreset, nullptr);
    uiRegisterAction("onHexChanged", onHexChanged, nullptr);

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "Panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 800, "h": 480 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "btnSwatch",
                        "type": "Button",
                        "rect": { "x": 30, "y": 40, "w": 60, "h": 32 },
                        "colors": { "background": { "normal": "#FF6600FF" } },
                        "borderVisible": false,
                        "events": { "onClick": "showColorDlg" }
                    },
                    {
                        "id": "lblColor",
                        "type": "Label",
                        "rect": { "x": 100, "y": 44, "w": 240, "h": 24 },
                        "caption": "#FF6600FF",
                        "fontSize": 14,
                        "textColor": [200, 200, 200]
                    }
                ]
            }
        ],
        "dialogs": [
            {
                "type": "Dialog",
                "id": "colorDlg",
                "centered": true,
                "rect": { "x": 0, "y": 0, "w": 296, "h": 440 },
                "confirmButton": { "text": "OK" },
                "cancelButton": { "text": "Cancel" },
                "events": {
                    "onConfirm": "onColorConfirmed",
                    "onCancel": "onColorCancelled",
                    "onClose": "onColorClose"
                },
                "children": [
                    {"id":"cp_00","type":"Button","rect":{"x":10,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#FF0000FF","hover":"#FF0000FF","pressed":"#FF0000FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_01","type":"Button","rect":{"x":66,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#00FF00FF","hover":"#00FF00FF","pressed":"#00FF00FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_02","type":"Button","rect":{"x":122,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#0000FFFF","hover":"#0000FFFF","pressed":"#0000FFFF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_03","type":"Button","rect":{"x":178,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#FFFF00FF","hover":"#FFFF00FF","pressed":"#FFFF00FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_04","type":"Button","rect":{"x":234,"y":10,"w":52,"h":32},"colors":{"background":{"normal":"#FF00FFFF","hover":"#FF00FFFF","pressed":"#FF00FFFF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_05","type":"Button","rect":{"x":10,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#00FFFFFF","hover":"#00FFFFFF","pressed":"#00FFFFFF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_06","type":"Button","rect":{"x":66,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#FFFFFFFF","hover":"#FFFFFFFF","pressed":"#FFFFFFFF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_07","type":"Button","rect":{"x":122,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#000000FF","hover":"#000000FF","pressed":"#000000FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_08","type":"Button","rect":{"x":178,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#808080FF","hover":"#808080FF","pressed":"#808080FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_09","type":"Button","rect":{"x":234,"y":48,"w":52,"h":32},"colors":{"background":{"normal":"#FFA500FF","hover":"#FFA500FF","pressed":"#FFA500FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_10","type":"Button","rect":{"x":10,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#800000FF","hover":"#800000FF","pressed":"#800000FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_11","type":"Button","rect":{"x":66,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#008000FF","hover":"#008000FF","pressed":"#008000FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_12","type":"Button","rect":{"x":122,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#000080FF","hover":"#000080FF","pressed":"#000080FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_13","type":"Button","rect":{"x":178,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#808000FF","hover":"#808000FF","pressed":"#808000FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_14","type":"Button","rect":{"x":234,"y":86,"w":52,"h":32},"colors":{"background":{"normal":"#800080FF","hover":"#800080FF","pressed":"#800080FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_15","type":"Button","rect":{"x":10,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#008080FF","hover":"#008080FF","pressed":"#008080FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_16","type":"Button","rect":{"x":66,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#C0C0C0FF","hover":"#C0C0C0FF","pressed":"#C0C0C0FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_17","type":"Button","rect":{"x":122,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#E0E0E0FF","hover":"#E0E0E0FF","pressed":"#E0E0E0FF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_18","type":"Button","rect":{"x":178,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#FFC0CBFF","hover":"#FFC0CBFF","pressed":"#FFC0CBFF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {"id":"cp_19","type":"Button","rect":{"x":234,"y":124,"w":52,"h":32},"colors":{"background":{"normal":"#A52A2AFF","hover":"#A52A2AFF","pressed":"#A52A2AFF"}},"borderVisible":false,"events":{"onClick":"onPreset"}},
                    {
                        "id": "dlgSwatch",
                        "type": "Button",
                        "rect": { "x": 10, "y": 166, "w": 52, "h": 32 },
                        "colors": { "background": { "normal": "#FF6600FF", "hover": "#FF6600FF", "pressed": "#FF6600FF" } },
                        "borderVisible": false
                    },
                    {
                        "id": "hexInput",
                        "type": "EditBox",
                        "rect": { "x": 72, "y": 168, "w": 130, "h": 28 },
                        "fontSize": 14,
                        "text": "#FF6600FF",
                        "textColor": [200, 200, 200],
                        "events": { "onTextChanged": "onHexChanged" }
                    },
                    {
                        "id": "lblR",
                        "type": "Label",
                        "rect": { "x": 10, "y": 226, "w": 14, "h": 16 },
                        "caption": "R",
                        "fontSize": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "rSlider",
                        "type": "Slider",
                        "rect": { "x": 29, "y": 224, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 255,
                        "showValueLabel": true,
                        "labelGap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    },
                    {
                        "id": "lblG",
                        "type": "Label",
                        "rect": { "x": 10, "y": 268, "w": 14, "h": 16 },
                        "caption": "G",
                        "fontSize": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "gSlider",
                        "type": "Slider",
                        "rect": { "x": 29, "y": 266, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 102,
                        "showValueLabel": true,
                        "labelGap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    },
                    {
                        "id": "lblB",
                        "type": "Label",
                        "rect": { "x": 10, "y": 310, "w": 14, "h": 16 },
                        "caption": "B",
                        "fontSize": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "bSlider",
                        "type": "Slider",
                        "rect": { "x": 29, "y": 308, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 0,
                        "showValueLabel": true,
                        "labelGap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    },
                    {
                        "id": "lblA",
                        "type": "Label",
                        "rect": { "x": 10, "y": 352, "w": 14, "h": 16 },
                        "caption": "A",
                        "fontSize": 12,
                        "colors": { "text": { "normal": "#C8C8C8FF" } }
                    },
                    {
                        "id": "aSlider",
                        "type": "Slider",
                        "rect": { "x": 29, "y": 350, "w": 257, "h": 20 },
                        "range": { "min": 0, "max": 255 },
                        "value": 255,
                        "showValueLabel": true,
                        "labelGap": -8,
                        "events": { "onValueChanged": "onColorChange" }
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(layoutJson)) { printf("FAIL: LoadLayout\n"); uiShutdown(); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");

    printf("Frame loop... (click color swatch or close the window)\n");
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
    printf("test_dialog_cabi_%s: done\n", shortName);
    return 0;
}

int main() { return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME); }
