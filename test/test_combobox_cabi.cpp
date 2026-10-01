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
typedef int  (*UISetStringFn)(UIInstance,void*,const char*,const char*);
typedef int  (*UIGetStringFn)(UIInstance,void*,const char*,char*,int);
typedef int  (*UIGetIntFn)(UIInstance,void*,const char*,int*);
typedef void (*UIPushUIEventFn)(UIInstance,const UIEvent*);
typedef int  (*UISetIntFn)(UIInstance,void*,const char*,int);
typedef int  (*UISetCallbackFn)(UIInstance,void*,const char*,UIEventCallback,void*);

static UICreateInstanceFn  uiCreateInstance      = nullptr;
static UIDestroyInstanceFn uiDestroyInstance     = nullptr;
static UISetViewportFn     uiSetViewport         = nullptr;
static UIProcessEventsFn   uiProcessEvents       = nullptr;
static UIUpdateFn          uiUpdate              = nullptr;
static UIClearFn           uiClear               = nullptr;
static UIRenderFn          uiRender              = nullptr;
static UIPresentFn         uiPresent             = nullptr;
static UIIsQuitFn          uiIsQuitRequested     = nullptr;
static UILoadLayoutFn      uiLoadLayout          = nullptr;
static UIFindControlFn     uiFindControl         = nullptr;
static UIRegisterActionFn  uiRegisterAction      = nullptr;
static UISetStringFn       uiSetString           = nullptr;
static UIGetStringFn       uiGetString           = nullptr;
static UIGetIntFn          uiGetInt              = nullptr;
static UIPushUIEventFn     uiPushUIEvent         = nullptr;
static UISetIntFn          uiSetInt              = nullptr;
static UISetCallbackFn     uiSetCallback          = nullptr;

static HMODULE g_uiDll = nullptr;
static UIInstance g_inst = nullptr;

// ===== 注入模拟（--sim-inject）：进程内事件注入（走 queuedEvents 通路），
// 不依赖窗口焦点，用于验证 comboEditable 打字 + 回车匹配 =====
static int g_directFired = 0;
static int g_directIdx = -99;
static void onDirectSelectionCB(void* ctl, const UIEventData* ev, void* user) {
    (void)ctl; (void)user;
    g_directFired++;
    g_directIdx = ev ? ev->data.selection.idx : -99;
}

static bool g_simInject = false;
static int g_autoSec = 0;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）
static int g_simPhase = 0;
static ULONGLONG g_simNextTick = 0;

static void injectMouseClick(float x, float y) {
    UIEvent down; memset(&down, 0, sizeof(down));
    down.type = UI_EVENT_MOUSE_DOWN;
    *(float*)down.data = x; *(float*)(down.data + 4) = y; *(int*)(down.data + 8) = 1; // Left
    uiPushUIEvent(g_inst, &down);
    UIEvent up; memset(&up, 0, sizeof(up));
    up.type = UI_EVENT_MOUSE_UP;
    *(float*)up.data = x; *(float*)(up.data + 4) = y; *(int*)(up.data + 8) = 1;
    uiPushUIEvent(g_inst, &up);
}

static void injectText(const char* s) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_TEXT_INPUT;
    strncpy((char*)ev.data, s, UI_TEXT_MAX);
    uiPushUIEvent(g_inst, &ev);
}

static void injectKeyDownUp(int code) {
    UIEvent down; memset(&down, 0, sizeof(down));
    down.type = UI_EVENT_KEY_DOWN;
    *(int*)down.data = code; *(uint16_t*)(down.data + 4) = 0;
    uiPushUIEvent(g_inst, &down);
    UIEvent up; memset(&up, 0, sizeof(up));
    up.type = UI_EVENT_KEY_UP;
    *(int*)up.data = code; *(uint16_t*)(up.data + 4) = 0;
    uiPushUIEvent(g_inst, &up);
}

static void injectClose() {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_WINDOW_CLOSE;
    uiPushUIEvent(g_inst, &ev);
}

static void runSimInject() {
    ULONGLONG now = GetTickCount64();
    if (now < g_simNextTick) return;
    printf("[inject] phase %d\n", g_simPhase);
    switch (g_simPhase) {
    case 0: injectMouseClick(170, 156); break; // 点击 comboEditable 正文（可编辑模式 → 聚焦）
    case 1: injectText("b"); break;
    case 2: injectText("e"); break;
    case 3: injectKeyDownUp(0x0D); break;      // Return → 精确匹配 "be" → Beijing
    case 4: injectMouseClick(170, 72); break;  // 点击 comboMain（只读 → 打开下拉）
    case 5: injectText("x"); break;            // 只读模式 TextInput → 应被拦截
    case 6: injectKeyDownUp(0x1B); break;      // Escape → 关闭下拉
    case 7: injectMouseClick(170, 156); break; // 重新聚焦 comboEditable
    case 8:
        // 清空编辑框（set "text" 属性）后输入无匹配内容
        uiSetString(g_inst, uiFindControl(g_inst, "comboEditable"), "text", "");
        injectText("zzz");
        break;
    case 9: injectKeyDownUp(0x0D); break;      // Return → 无匹配 → 保留输入、index=-1
    case 10: injectMouseClick(425, 156); break; // 点击 Dump 按钮 → 属性 CAPI 读取（视觉验证）
    case 11: injectMouseClick(310, 156); break; // 点 comboEditable 三角（打开下拉）
    case 12: injectMouseClick(310, 156); break; // 再点三角 → 应收起（修复前会关闭后重开）
    case 13: injectMouseClick(170, 187); break; // 点列表第一项位置：若已收起则无选中；若重开则 Selected #0
    case 14: injectMouseClick(425, 156); break; // 再次 Dump → 验证 text 仍为 'zzz'（收起不清空）
    default:
        {
            char buf[256] = "";
            char val[256] = "";
            int idx = -2;
            void* ctl = uiFindControl(g_inst, "comboEditable");
            if (ctl) {
                uiGetString(g_inst, ctl, "text", buf, sizeof(buf));
                uiGetString(g_inst, ctl, "selected-value", val, sizeof(val));
                uiGetInt(g_inst, ctl, "selected-index", &idx);
            }
            printf("[inject] comboEditable text='%s' selected-value='%s' selected-index=%d (expect text=zzz, index=-1)\n", buf, val, idx);
            g_simInject = false;
            injectClose();
            return;
        }
    }
    g_simPhase++;
    g_simNextTick = now + 350;
}

// ===== 选中回调 =====
static char g_selectionInfo[128] = "Selected: (none)";

static void onSelectionChanged(void* ctl, void* user) {
    (void)user;
    int idx = -1;
    uiGetInt(g_inst, ctl, "selected-index", &idx);
    char labelBuf[256] = "";
    uiGetString(g_inst, ctl, "text", labelBuf, sizeof(labelBuf));
    const char* label = labelBuf;
    snprintf(g_selectionInfo, sizeof(g_selectionInfo), "Selected: #%d = %s", idx, label ? label : "(null)");
    void* lbl = uiFindControl(g_inst, "lblStatus");
    if (lbl) uiSetString(g_inst, lbl, "caption", g_selectionInfo);
    printf("%s\n", g_selectionInfo);
}

// ===== Dump 按钮回调：通过属性 CAPI 读取两个 ComboBox 的输入内容/选中状态 =====
static void onDumpCombo(void* ctl, void* user) {
    (void)user;
    printf("=== Dump Combo State (via property CAPI) ===\n");
    char editableInfo[256] = "";
    const char* ids[] = { "comboMain", "comboEditable" };
    for (int i = 0; i < 2; i++) {
        void* c = uiFindControl(g_inst, ids[i]);
        if (!c) continue;
        char text[256] = "";
        char val[256] = "";
        int idx = -2;
        uiGetString(g_inst, c, "text", text, sizeof(text));
        uiGetString(g_inst, c, "selected-value", val, sizeof(val));
        uiGetInt(g_inst, c, "selected-index", &idx);
        printf("[dump] %s: text='%s' selected-value='%s' selected-index=%d\n", ids[i], text, val, idx);
        if (strcmp(ids[i], "comboEditable") == 0) {
            snprintf(editableInfo, sizeof(editableInfo),
                     "Editable: text='%s' val='%s' idx=%d", text, val, idx);
        }
    }
    void* lbl = uiFindControl(g_inst, "lblStatus");
    if (lbl && editableInfo[0]) uiSetString(g_inst, lbl, "caption", editableInfo);
}

static void loadAllProcs(HMODULE dll) {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(dll, "UICornerstone_" #name)

    RESOLVE(CreateInstance);
    RESOLVE(DestroyInstance);
    RESOLVE(SetViewport);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(LoadLayout);
    RESOLVE(FindControl);
    RESOLVE(RegisterAction);
    RESOLVE(SetString);
    RESOLVE(GetString);
    RESOLVE(GetInt);
    RESOLVE(PushUIEvent);
    RESOLVE(SetInt);
    RESOLVE(SetCallback);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_combobox_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiCreateInstance) { printf("FAIL: GetProcAddress(CreateInstance)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_combobox_cabi";
    cfg.windowWidth = 540;
    cfg.windowHeight = 320;
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 540, 320);
    printf("OK: initialized\n");

    uiRegisterAction(g_inst, "onSelectionChanged", onSelectionChanged, nullptr);
    uiRegisterAction(g_inst, "onDumpCombo", onDumpCombo, nullptr);

    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 540, "h": 320 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "label",
                        "rect": { "x": 20, "y": 16, "w": 500, "h": 28 },
                        "caption": "ComboBox C ABI Test",
                        "font-size": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "comboMain",
                        "type": "combo-box",
                        "rect": { "x": 20, "y": 56, "w": 300, "h": 32 },
                        "font-size": 16,
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
                        "type": "label",
                        "rect": { "x": 20, "y": 100, "w": 500, "h": 24 },
                        "caption": "Selected: (none)",
                        "font-size": 14,
                        "textColor": [180, 200, 220]
                    },
                    {
                        "id": "comboEditable",
                        "type": "combo-box",
                        "rect": { "x": 20, "y": 140, "w": 300, "h": 32 },
                        "font-size": 16,
                        "editable": true,
                        "placeholder": "Type to filter...",
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
                        "id": "btnDump",
                        "type": "button",
                        "rect": { "x": 340, "y": 140, "w": 170, "h": 32 },
                        "caption": "Dump Combo",
                        "events": { "onClick": "onDumpCombo" }
                    },
                    {
                        "id": "lblHint",
                        "type": "label",
                        "rect": { "x": 20, "y": 196, "w": 500, "h": 110 },
                        "caption": "Mode 1 (read-only): click anywhere to open the dropdown.\nMode 2 (editable): type text + Enter to select the\nmatching item; unmatched text is kept as the content.\nDisabled items (e.g. Wuhan) cannot be selected.\n\nPress 'Dump Combo' to read text/selected-value/selected-index\nvia property CAPI (unmatched input is kept in 'text').",
                        "font-size": 12,
                        "textColor": [140, 140, 160]
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(g_inst, layoutJson)) { printf("FAIL: LoadLayout\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded\n");

    // ===== P0-48锛帰etCallback + selected-index 绋嬪簭鍖栬Е鍙?=====
    {
        void* ed = uiFindControl(g_inst, "comboEditable");
        printf("P0-48 comboEditable=%s\n", ed ? "OK" : "NULL");
        if (ed && uiSetCallback && uiSetInt) {
            uiSetCallback(g_inst, ed, "selection-changed", onDirectSelectionCB, nullptr);
            g_directFired = 0; g_directIdx = -99;
            int r1 = uiSetInt(g_inst, ed, "selected-index", 1);
            printf("P0-48 direct SetInt(1)=%d fired=%d idx=%d\n", r1, g_directFired, g_directIdx);
            int rSame = uiSetInt(g_inst, ed, "selected-index", 1);
            printf("P0-48 direct SetInt(same)=%d fired=%d (guard)\n", rSame, g_directFired);
            int r2 = uiSetInt(g_inst, ed, "selected-index", 2);
            printf("P0-48 direct SetInt(2)=%d fired=%d idx=%d\n", r2, g_directFired, g_directIdx);
        }
    }

    printf("Frame loop... (interact with the ComboBox or close the window)\n");
    ULONGLONG autoT0 = GetTickCount64();
    while (!uiIsQuitRequested(g_inst)) {
        if (g_autoSec > 0 && (GetTickCount64() - autoT0) >= (ULONGLONG)g_autoSec * 1000) {
            UIEvent ue; memset(&ue, 0, sizeof(ue)); ue.type = UI_EVENT_WINDOW_CLOSE;
            uiPushUIEvent(g_inst, &ue);
        }
        if (g_simInject) runSimInject();
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
    printf("test_combobox_cabi_%s: done\n", shortName);
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sim-inject") == 0) g_simInject = true;
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}
