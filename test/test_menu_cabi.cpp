// =========================================================================
// test_menu_cabi.cpp -- C ABI integration test for Menu (MenuBar/MenuPanel/MenuItem)
// Tests: programmatic creation via C ABI, assembly (AddMenu/AddItem/AddSeparator/
// SetSubMenu), property system (item-id positioning + item-* props + caption/
// checked/shortcut), GetControlType, JSON menu-bar parsing + FindControl.
// Build from build/{backend}_dll/, output to build/{backend}_dll/test/Debug/
// =========================================================================

#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../../include/UICornerstoneAPI.h"

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

typedef UIInstance (*UICreateInstanceFn)(const UIBackendCallbacks*, const UIInstanceConfig*);
typedef void       (*UIDestroyInstanceFn)(UIInstance);
typedef void  (*UISetViewportFn)(UIInstance,float,float,float,float);
typedef void  (*UIProcessEventsFn)(UIInstance);
typedef void  (*UIUpdateFn)(UIInstance,double);
typedef void  (*UIClearFn)(UIInstance);
typedef void  (*UIRenderFn)(UIInstance);
typedef void  (*UIPresentFn)(UIInstance);
typedef int   (*UIIsQuitFn)(UIInstance);
typedef void  (*UIPushUIEventFn)(UIInstance,const UIEvent*);
typedef int   (*UILoadLayoutFn)(UIInstance,const char*);
typedef void* (*UIFindControlFn)(UIInstance,const char*);
typedef int   (*UISetStringFn)(UIInstance, void*, const char*, const char*);
typedef int   (*UIGetStringFn)(UIInstance, void*, const char*, char*, int, int*);
typedef int   (*UIGetPtrFn)(UIInstance, void*, const char*, void**);
typedef int   (*UISetFloatFn)(UIInstance, void*, const char*, float);
typedef int   (*UIGetFloatFn)(UIInstance, void*, const char*, float*);
typedef int   (*UISetIntFn)(UIInstance, void*, const char*, int);
typedef int   (*UIGetIntFn)(UIInstance, void*, const char*, int*);
typedef int   (*UISetBoolFn)(UIInstance, void*, const char*, int);
typedef int   (*UIGetBoolFn)(UIInstance, void*, const char*, int*);
typedef int   (*UIGetEnumFn)(UIInstance, void*, const char*, char*, int);
typedef void* (*UICreateMenuBarFn)(UIInstance, float, float, float, float, float, float);
typedef void* (*UICreateMenuPanelFn)(UIInstance, float, float);
typedef void* (*UICreateMenuItemFn)(UIInstance, const char*, int, float, float);
typedef void  (*UIMenuBarAddMenuFn)(UIInstance, void*, const char*, void*);
typedef void  (*UIMenuPanelAddItemFn)(UIInstance, void*, void*);
typedef void  (*UIMenuPanelAddSeparatorFn)(UIInstance, void*);
typedef void  (*UIMenuItemSetSubMenuFn)(UIInstance, void*, void*);
typedef void  (*UIAddChildControlFn)(UIInstance, void*, void*);
typedef int   (*UIGetControlTypeFn)(UIInstance, void*, char*, int);
typedef void  (*UIDestroyControlFn)(UIInstance, void*);
typedef int   (*UISetCallbackFn)(UIInstance, void*, const char*, UIEventCallback, void*);

static UICreateInstanceFn      uiCreateInstance       = nullptr;
static UIDestroyInstanceFn     uiDestroyInstance      = nullptr;
static UISetViewportFn         uiSetViewport          = nullptr;
static UIProcessEventsFn       uiProcessEvents        = nullptr;
static UIUpdateFn              uiUpdate               = nullptr;
static UIClearFn               uiClear                = nullptr;
static UIRenderFn              uiRender               = nullptr;
static UIPresentFn             uiPresent              = nullptr;
static UIIsQuitFn              uiIsQuit               = nullptr;
static UIPushUIEventFn         uiPushUIEvent          = nullptr;
static UILoadLayoutFn          uiLoadLayout           = nullptr;
static UIFindControlFn         uiFindControl          = nullptr;
static UISetStringFn           uiSetString            = nullptr;
static UIGetStringFn           uiGetString            = nullptr;
static int g_gsLen = 0;   // P0-108：GetString 长度查询
static UIGetPtrFn              uiGetPtr               = nullptr;
static UISetFloatFn            uiSetFloat             = nullptr;
static UIGetFloatFn            uiGetFloat             = nullptr;
static UISetIntFn              uiSetInt               = nullptr;
static UIGetIntFn              uiGetInt               = nullptr;
static UISetBoolFn             uiSetBool              = nullptr;
static UIGetBoolFn             uiGetBool              = nullptr;
static UIGetEnumFn             uiGetEnum              = nullptr;
static UICreateMenuBarFn       uiCreateMenuBar        = nullptr;
static UICreateMenuPanelFn     uiCreateMenuPanel      = nullptr;
static UICreateMenuItemFn      uiCreateMenuItem       = nullptr;
static UIMenuBarAddMenuFn      uiMenuBarAddMenu       = nullptr;
static UIMenuPanelAddItemFn    uiMenuPanelAddItem     = nullptr;
static UIMenuPanelAddSeparatorFn uiMenuPanelAddSeparator = nullptr;
static UIMenuItemSetSubMenuFn  uiMenuItemSetSubMenu   = nullptr;
static UIAddChildControlFn     uiAddChildControl      = nullptr;
static UIGetControlTypeFn      uiGetControlType       = nullptr;
static UIDestroyControlFn      uiDestroyControl       = nullptr;
static UISetCallbackFn         uiSetCallback          = nullptr;

static HMODULE g_uiDll = nullptr;
static UIInstance g_inst = nullptr;
static int g_autoSec = 0;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）
static int g_menuClicks = 0;

// ---- JSON layout: root panel + JSON menu-bar (menus/items + item-id) ----
static const char* LAYOUT_JSON =
    "{"
    "  \"controls\": ["
    "    { \"type\": \"panel\", \"id\": \"rootPanel\", \"rect\": { \"x\": 10, \"y\": 10, \"w\": 760, \"h\": 560 },"
    "      \"children\": ["
    "        { \"type\": \"label\", \"id\": \"lblStatus\", \"rect\": { \"x\": 10, \"y\": 400, \"w\": 740, \"h\": 120 },"
    "          \"caption\": \"(menu ready)\", \"fontSize\": 16, \"textColor\": [180, 200, 220] }"
    "      ]"
    "    },"
    "    { \"type\": \"menu-bar\", \"id\": \"jsonMenu\","
    "      \"font\": { \"size\": 16 }, \"barHeight\": 30,"
    "      \"menus\": ["
    "        { \"caption\": \"File\", \"items\": ["
    "          { \"id\": \"mOpen\", \"caption\": \"Open\", \"shortcut\": \"Ctrl+O\" },"
    "          { \"id\": \"mChk\", \"caption\": \"Toggle\", \"checked\": true,"
    "            \"leadingControl\": { \"type\": \"check-box\", \"checkState\": \"checked\" } },"
    "          { \"id\": \"mSub\", \"caption\": \"Recent\", \"items\": ["
    "            { \"id\": \"mR1\", \"caption\": \"r1.txt\" } ] },"
    "          { \"type\": \"separator\" },"
    "          { \"id\": \"mQuit\", \"caption\": \"Quit\" }"
    "        ] }"
    "      ]"
    "    }"
    "  ]"
    "}";

static void onMenuClick(UIControlHandle ctl, const UIEventData* event, void* userData) {
    (void)ctl;
    (void)event;
    g_menuClicks++;
    const char* tag = static_cast<const char*>(userData);
    printf("[menu] click: %s\n", tag ? tag : "?");
}

static void loadFunctions() {
    uiCreateInstance = (UICreateInstanceFn)    GetProcAddress(g_uiDll, "UICornerstone_CreateInstance");
    uiSetViewport    = (UISetViewportFn)       GetProcAddress(g_uiDll, "UICornerstone_SetViewport");
    uiProcessEvents  = (UIProcessEventsFn)     GetProcAddress(g_uiDll, "UICornerstone_ProcessEvents");
    uiUpdate         = (UIUpdateFn)            GetProcAddress(g_uiDll, "UICornerstone_Update");
    uiClear          = (UIClearFn)             GetProcAddress(g_uiDll, "UICornerstone_Clear");
    uiRender         = (UIRenderFn)            GetProcAddress(g_uiDll, "UICornerstone_Render");
    uiPresent        = (UIPresentFn)           GetProcAddress(g_uiDll, "UICornerstone_Present");
    uiIsQuit         = (UIIsQuitFn)            GetProcAddress(g_uiDll, "UICornerstone_IsQuitRequested");
    uiPushUIEvent    = (UIPushUIEventFn)       GetProcAddress(g_uiDll, "UICornerstone_PushUIEvent");
    uiDestroyInstance= (UIDestroyInstanceFn)   GetProcAddress(g_uiDll, "UICornerstone_DestroyInstance");
    uiLoadLayout     = (UILoadLayoutFn)        GetProcAddress(g_uiDll, "UICornerstone_LoadLayout");
    uiFindControl    = (UIFindControlFn)       GetProcAddress(g_uiDll, "UICornerstone_FindControl");
    uiSetString      = (UISetStringFn)         GetProcAddress(g_uiDll, "UICornerstone_SetString");
    uiGetString      = (UIGetStringFn)         GetProcAddress(g_uiDll, "UICornerstone_GetString");
    uiGetPtr         = (UIGetPtrFn)            GetProcAddress(g_uiDll, "UICornerstone_GetPtr");
    uiSetFloat       = (UISetFloatFn)          GetProcAddress(g_uiDll, "UICornerstone_SetFloat");
    uiGetFloat       = (UIGetFloatFn)          GetProcAddress(g_uiDll, "UICornerstone_GetFloat");
    uiSetInt         = (UISetIntFn)            GetProcAddress(g_uiDll, "UICornerstone_SetInt");
    uiGetInt         = (UIGetIntFn)            GetProcAddress(g_uiDll, "UICornerstone_GetInt");
    uiSetBool        = (UISetBoolFn)           GetProcAddress(g_uiDll, "UICornerstone_SetBool");
    uiGetBool        = (UIGetBoolFn)           GetProcAddress(g_uiDll, "UICornerstone_GetBool");
    uiGetEnum        = (UIGetEnumFn)           GetProcAddress(g_uiDll, "UICornerstone_GetEnum");
    uiCreateMenuBar  = (UICreateMenuBarFn)     GetProcAddress(g_uiDll, "UICornerstone_CreateMenuBar");
    uiCreateMenuPanel= (UICreateMenuPanelFn)   GetProcAddress(g_uiDll, "UICornerstone_CreateMenuPanel");
    uiCreateMenuItem = (UICreateMenuItemFn)    GetProcAddress(g_uiDll, "UICornerstone_CreateMenuItem");
    uiMenuBarAddMenu = (UIMenuBarAddMenuFn)    GetProcAddress(g_uiDll, "UICornerstone_MenuBarAddMenu");
    uiMenuPanelAddItem = (UIMenuPanelAddItemFn)GetProcAddress(g_uiDll, "UICornerstone_MenuPanelAddItem");
    uiMenuPanelAddSeparator = (UIMenuPanelAddSeparatorFn) GetProcAddress(g_uiDll, "UICornerstone_MenuPanelAddSeparator");
    uiMenuItemSetSubMenu = (UIMenuItemSetSubMenuFn) GetProcAddress(g_uiDll, "UICornerstone_MenuItemSetSubMenu");
    uiAddChildControl= (UIAddChildControlFn)   GetProcAddress(g_uiDll, "UICornerstone_AddChildControl");
    uiGetControlType = (UIGetControlTypeFn)      GetProcAddress(g_uiDll, "UICornerstone_GetControlType");
    uiDestroyControl = (UIDestroyControlFn)      GetProcAddress(g_uiDll, "UICornerstone_DestroyControl");
    uiSetCallback    = (UISetCallbackFn)       GetProcAddress(g_uiDll, "UICornerstone_SetCallback");
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    setvbuf(stdout, NULL, _IONBF, 0);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }

    loadFunctions();
    if (!uiCreateInstance || !uiIsQuit || !uiPushUIEvent || !uiLoadLayout || !uiFindControl ||
        !uiCreateMenuBar || !uiCreateMenuPanel || !uiCreateMenuItem ||
        !uiMenuBarAddMenu || !uiMenuPanelAddItem || !uiMenuPanelAddSeparator ||
        !uiMenuItemSetSubMenu || !uiAddChildControl) {
        printf("FAIL: GetProcAddress (menu factory set)\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_menu_cabi";
    cfg.windowWidth = 800;
    cfg.windowHeight = 600;
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 800, 600);
    printf("Init OK\n");

    // ---- 1. JSON menu-bar: parse + FindControl + item-id positioning ----
    if (uiLoadLayout(g_inst, LAYOUT_JSON) == 0) { printf("FAIL: LoadLayout\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("LoadLayout OK\n");

    void* jsonBar = uiFindControl(g_inst, "jsonMenu");
    if (!jsonBar) { printf("FAIL: FindControl(jsonMenu)\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("FindControl(jsonMenu) OK\n");

    // barHeight 30 from JSON; font.size 16 → default auto 16*1.6=25.6 overridden by barHeight
    float barH = 0;
    if (!uiGetFloat || !uiGetFloat(g_inst, jsonBar, "bar-height", &barH) || barH != 30.0f) {
        printf("FAIL: GetFloat(bar-height) got %.1f\n", barH); return 1;
    }
    printf("GetFloat(bar-height)=30 OK\n");

    // GetControlType on JSON bar
    char typeBuf[64] = {0};
    if (!uiGetControlType || !uiGetControlType(g_inst, jsonBar, typeBuf, (int)sizeof(typeBuf)) || strcmp(typeBuf, "menu-bar") != 0) {
        printf("FAIL: GetControlType(jsonMenu) got \"%s\"\n", typeBuf); return 1;
    }
    printf("GetControlType(menu-bar) OK\n");

    // 移除 JSON 菜单栏（Menu 缺省全宽占据视口顶部，视口只保留一个菜单栏）
    uiDestroyControl(g_inst, jsonBar);
    printf("DestroyControl(jsonMenu) OK\n");

    // ---- 2. Programmatic creation: full C ABI assembly chain ----
    void* bar = uiCreateMenuBar(g_inst, 0, 40, 400, 32, 1.0f, 1.0f);
    if (!bar) { printf("FAIL: CreateMenuBar\n"); return 1; }
    void* panel = uiCreateMenuPanel(g_inst, 1.0f, 1.0f);
    if (!panel) { printf("FAIL: CreateMenuPanel\n"); return 1; }
    void* itemOpen = uiCreateMenuItem(g_inst, "Open", 0, 1.0f, 1.0f);
    if (!itemOpen) { printf("FAIL: CreateMenuItem(Open)\n"); return 1; }
    // item-id set on the item itself (CABI): required for panel item-* positioning
    if (uiSetString(g_inst, itemOpen, "item-id", "mOpen") != 1) { printf("FAIL: SetString(item item-id)\n"); return 1; }
    char idBuf[64] = {0};
    if (!uiGetString(g_inst, itemOpen, "item-id", idBuf, (int)sizeof(idBuf), &g_gsLen) || strcmp(idBuf, "mOpen") != 0) {
        printf("FAIL: GetString(item item-id) got \"%s\"\n", idBuf); return 1;
    }
    uiMenuPanelAddItem(g_inst, panel, itemOpen);
    void* itemSep = uiCreateMenuItem(g_inst, "", 1, 1.0f, 1.0f);   // type 1 = Separator
    if (!itemSep) { printf("FAIL: CreateMenuItem(Separator)\n"); return 1; }
    uiMenuPanelAddItem(g_inst, panel, itemSep);
    void* subPanel = uiCreateMenuPanel(g_inst, 1.0f, 1.0f);
    void* itemSub = uiCreateMenuItem(g_inst, "Recent", 2, 1.0f, 1.0f);  // type 2 = SubMenu
    if (!subPanel || !itemSub) { printf("FAIL: CreateMenuItem(SubMenu)/panel\n"); return 1; }
    uiMenuPanelAddItem(g_inst, panel, itemSub);
    uiMenuItemSetSubMenu(g_inst, itemSub, subPanel);
    uiMenuBarAddMenu(g_inst, bar, "File", panel);

    // caption / checked / shortcut on programmatic item
    if (uiSetString(g_inst, itemOpen, "caption", "Open File") != 1) { printf("FAIL: SetString(caption)\n"); return 1; }
    char cbuf[64] = {0};
    if (!uiGetString(g_inst, itemOpen, "caption", cbuf, (int)sizeof(cbuf), &g_gsLen) || strcmp(cbuf, "Open File") != 0) {
        printf("FAIL: GetString(caption) got \"%s\"\n", cbuf); return 1;
    }
    if (uiSetString(g_inst, itemOpen, "shortcut", "Ctrl+O") != 1) { printf("FAIL: SetString(shortcut)\n"); return 1; }
    if (uiSetBool(g_inst, itemOpen, "checked", 1) != 1) { printf("FAIL: SetBool(checked)\n"); return 1; }
    int chk = 0;
    if (!uiGetBool(g_inst, itemOpen, "checked", &chk) || chk != 1) { printf("FAIL: GetBool(checked)\n"); return 1; }
    printf("programmatic item caption/shortcut/checked OK\n");

    // item-id positioning on programmatic panel: item-* props resolve against item-id target
    if (uiSetString(g_inst, panel, "item-id", "mOpen") != 1) { printf("FAIL: SetString(panel item-id)\n"); return 1; }
    // item-font-size 0 (inherit) → 18; item-leading-gap 12 → 10 via panel item-* props
    int fs = 0;
    if (!uiGetInt || !uiGetInt(g_inst, panel, "item-font-size", &fs) || fs != 0) { printf("FAIL: GetInt(item-font-size) default\n"); return 1; }
    if (!uiSetInt || uiSetInt(g_inst, panel, "item-font-size", 18) != 1 ||
        !uiGetInt(g_inst, panel, "item-font-size", &fs) || fs != 18) { printf("FAIL: SetInt(item-font-size)\n"); return 1; }
    float gap = 0;
    if (!uiGetFloat(g_inst, panel, "item-leading-gap", &gap) || gap != 8.0f) { printf("FAIL: GetFloat(item-leading-gap) default got %.1f\n", gap); return 1; }
    if (!uiSetFloat || uiSetFloat(g_inst, panel, "item-leading-gap", 10.0f) != 1 ||
        !uiGetFloat(g_inst, panel, "item-leading-gap", &gap) || gap != 10.0f) { printf("FAIL: SetFloat(item-leading-gap)\n"); return 1; }
    printf("panel item-id positioning + item-* props OK\n");

    // click callback on programmatic item
    if (!uiSetCallback || uiSetCallback(g_inst, itemOpen, "click", onMenuClick, (void*)"Open File") != 1) {
        printf("FAIL: SetCallback(click)\n"); return 1;
    }

    // GetControlType for panel / item / separator
    if (!uiGetControlType(g_inst, panel, typeBuf, (int)sizeof(typeBuf)) || strcmp(typeBuf, "menu-panel") != 0) {
        printf("FAIL: GetControlType(panel) got \"%s\"\n", typeBuf); return 1;
    }
    if (!uiGetControlType(g_inst, itemOpen, typeBuf, (int)sizeof(typeBuf)) || strcmp(typeBuf, "menu-item") != 0) {
        printf("FAIL: GetControlType(item) got \"%s\"\n", typeBuf); return 1;
    }
    if (!uiGetControlType(g_inst, itemSep, typeBuf, (int)sizeof(typeBuf)) || strcmp(typeBuf, "menu-item") != 0) {
        printf("FAIL: GetControlType(separator) got \"%s\"\n", typeBuf); return 1;
    }
    printf("GetControlType: menu-bar / menu-panel / menu-item OK\n");

    // Attach programmatic bar under the root panel
    void* root = uiFindControl(g_inst, "rootPanel");
    if (!root) { printf("FAIL: FindControl(rootPanel)\n"); return 1; }
    uiAddChildControl(g_inst, root, bar);

    printf("Menu C ABI assertions PASSED\n");

    printf("Frame loop running - open menus, close window to exit\n");
    ULONGLONG autoT0 = GetTickCount64();
    while (!uiIsQuit(g_inst)) {
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
    printf("=== Menu C ABI Test PASSED ===\n");
    return 0;
}
