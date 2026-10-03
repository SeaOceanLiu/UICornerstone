// =========================================================================
// test_overlay_viewport_cabi.cpp -- P0-52：浮层恒在子视口之上 + 浮层优先事件路由
// 动态加载 UICornerstone.dll；覆盖：
//   1) 无浮层：子视口内坐标路由到子视口（Debug_RouteMouseTarget=0）
//   2) 浮层可见：owner 独占（=1，含视口区域外坐标）
//   3) 像素级：RenderOverlays 后浮层覆盖子视口（前后截图差异）
//   4) 关闭浮层：路由恢复（=0）；空浮层 RenderOverlays 无副作用
// =========================================================================

#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "../include/UICornerstoneAPI.h"

// ===== 动态函数指针 =====
typedef UIInstance (*UIPluginCreateInstanceFn)(const char*, const UIInstanceConfig*);
typedef void       (*UIDestroyInstanceFn)(UIInstance);
typedef int        (*UIProcessEventsFn)(UIInstance);
typedef void       (*UIUpdateFn)(UIInstance, double);
typedef void       (*UIClearFn)(UIInstance);
typedef void       (*UIRenderFn)(UIInstance);
typedef void       (*UIRenderOverlaysFn)(UIInstance);
typedef void       (*UIPresentFn)(UIInstance);
typedef UIInstance (*UICreateViewportFn)(UIInstance, UIRect);
typedef void*      (*UICreateContextMenuFn)(UIInstance, float, float, float, float, float, float);
typedef int        (*UIContextMenuAddItemFn)(UIInstance, void*, const char*, const char*);
typedef int        (*UIContextMenuShowFn)(UIInstance, void*, float, float);
typedef int        (*UIContextMenuCloseFn)(UIInstance, void*);
typedef int        (*UIDebugRouteMouseTargetFn)(UIInstance, float, float);
typedef int        (*UICaptureRectFn)(UIInstance, float, float, float, float, uint8_t*, int*, int*);

static UIPluginCreateInstanceFn uiCreateInstanceFromPlugin = nullptr;
static UIDestroyInstanceFn      uiDestroyInstance = nullptr;
static UIProcessEventsFn        uiProcessEvents = nullptr;
static UIUpdateFn               uiUpdate = nullptr;
static UIClearFn                uiClear = nullptr;
static UIRenderFn               uiRender = nullptr;
static UIRenderOverlaysFn       uiRenderOverlays = nullptr;
static UIPresentFn              uiPresent = nullptr;
static UICreateViewportFn       uiCreateViewport = nullptr;
static UICreateContextMenuFn    uiCreateContextMenu = nullptr;
static UIContextMenuAddItemFn   uiContextMenuAddItem = nullptr;
static UIContextMenuShowFn      uiContextMenuShow = nullptr;
static UIContextMenuCloseFn     uiContextMenuClose = nullptr;
static UIDebugRouteMouseTargetFn uiDebug_RouteMouseTarget = nullptr;
static UICaptureRectFn          uiCaptureRect = nullptr;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("PASS: %s\n", msg); g_pass++; } \
    else      { printf("FAIL: %s\n", msg); g_fail++; } \
} while (0)

static HMODULE g_dll = nullptr;

static void loadProcs(HMODULE dll) {
#define RESOLVE(name) *(void**)&ui##name = GetProcAddress(dll, "UICornerstone_" #name)
    RESOLVE(CreateInstanceFromPlugin);
    RESOLVE(DestroyInstance);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(RenderOverlays);
    RESOLVE(Present);
    RESOLVE(CreateViewport);
    RESOLVE(CreateContextMenu);
    RESOLVE(ContextMenuAddItem);
    RESOLVE(ContextMenuShow);
    RESOLVE(ContextMenuClose);
    RESOLVE(Debug_RouteMouseTarget);
    RESOLVE(CaptureRect);
#undef RESOLVE
}

static void frame(UIInstance inst, UIInstance vp) {
    uiProcessEvents(inst);
    uiClear(inst);
    uiRender(inst);
    if (vp) uiRender(vp);
    uiUpdate(inst, 0.016);
    uiPresent(inst);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    setvbuf(stdout, nullptr, _IONBF, 0);

    g_dll = LoadLibraryA("UICornerstone.dll");
    if (!g_dll) { printf("FAIL: LoadLibrary\n"); return 1; }
    loadProcs(g_dll);
    if (!uiCreateInstanceFromPlugin || !uiCreateViewport || !uiDebug_RouteMouseTarget || !uiRenderOverlays) {
        printf("FAIL: GetProcAddress (P0-52 exports missing)\n");
        FreeLibrary(g_dll);
        return 1;
    }

    UIInstance inst = uiCreateInstanceFromPlugin("sdl3", nullptr);
    if (!inst) { printf("FAIL: CreateInstanceFromPlugin\n"); FreeLibrary(g_dll); return 1; }
    printf("OK: instance created\n");

    UIRect vpRect = { 200.0f, 100.0f, 240.0f, 240.0f };
    UIInstance vp = uiCreateViewport(inst, vpRect);
    CHECK(vp != nullptr, "CreateViewport(200,100,240,240)");

    frame(inst, vp);
    frame(inst, vp);

    // 1) 无浮层：视口内坐标 → 子视口（0）；空白区 → owner（1）
    CHECK(uiDebug_RouteMouseTarget(inst, 300.0f, 200.0f) == 0, "no-overlay: point in vp routes to vp (0)");
    CHECK(uiDebug_RouteMouseTarget(inst, 20.0f, 20.0f) == 1, "no-overlay: blank area routes to owner (1)");

    // 2) 浮层可见：owner 独占（视口内/外均 1）
    void* menu = uiCreateContextMenu(inst, 0.0f, 0.0f, 160.0f, 80.0f, 1.0f, 1.0f);
    CHECK(menu != nullptr, "CreateContextMenu");
    uiContextMenuAddItem(inst, menu, "Item one", "Ctrl+1");
    uiContextMenuAddItem(inst, menu, "Item two", nullptr);
    CHECK(uiContextMenuShow(inst, menu, 300.0f, 200.0f) != 0, "ContextMenuShow(300,200) over vp");
    CHECK(uiDebug_RouteMouseTarget(inst, 300.0f, 200.0f) == 1, "overlay visible: point in vp routes to owner (1)");
    CHECK(uiDebug_RouteMouseTarget(inst, 20.0f, 20.0f) == 1, "overlay visible: blank area routes to owner (1)");

    // 3) 像素级：RenderOverlays 后浮层覆盖子视口（Render 内的浮层被 vp.Render 覆盖；
    //    overlay pass 重绘于其上 → 该区域像素应发生变化）
    static uint8_t bufA[256 * 256 * 4];
    static uint8_t bufB[256 * 256 * 4];
    int wa = 0, ha = 0, wb = 0, hb = 0;
    memset(bufA, 0, sizeof(bufA)); memset(bufB, 0, sizeof(bufB));
    uiClear(inst);
    uiRender(inst);
    uiRender(vp);                       // 子视口覆盖浮层（现状行为）
    int rcA = uiCaptureRect(inst, 305.0f, 205.0f, 60.0f, 24.0f, bufA, &wa, &ha);
    uiRenderOverlays(inst);             // P0-52①：浮层重绘于子视口之上
    int rcB = uiCaptureRect(inst, 305.0f, 205.0f, 60.0f, 24.0f, bufB, &wb, &hb);
    CHECK(rcA == 1 && rcB == 1 && wa == wb && ha == hb, "CaptureRect before/after overlay pass");
    bool differ = false;
    for (int i = 0; i < wa * ha * 4 && !differ; ++i) if (bufA[i] != bufB[i]) differ = true;
    CHECK(differ, "overlay drawn above child viewport (pixel diff)");
    uiPresent(inst);

    // 4) 关闭浮层：路由恢复；空浮层 overlay pass 零副作用
    int closeRc = uiContextMenuClose(inst, menu);
    CHECK(closeRc != 0, "ContextMenuClose");
    uiProcessEvents(inst);
    CHECK(uiDebug_RouteMouseTarget(inst, 300.0f, 200.0f) == 0, "overlay closed: point in vp routes to vp again (0)");

    memset(bufA, 0, sizeof(bufA)); memset(bufB, 0, sizeof(bufB));
    uiClear(inst);
    uiRender(inst);
    uiRender(vp);
    uiCaptureRect(inst, 305.0f, 205.0f, 60.0f, 24.0f, bufA, &wa, &ha);
    uiRenderOverlays(inst);             // 无浮层：无副作用
    uiCaptureRect(inst, 305.0f, 205.0f, 60.0f, 24.0f, bufB, &wb, &hb);
    bool same = (wa == wb && ha == hb) ? true : false;
    for (int i = 0; same && i < wa * ha * 4; ++i) if (bufA[i] != bufB[i]) same = false;
    CHECK(same, "no-overlay: RenderOverlays keeps frame identical");

    uiDestroyInstance(inst);
    FreeLibrary(g_dll);
    g_dll = nullptr;

    printf("\ntest_overlay_viewport_cabi: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail > 0) ? 1 : 0;
}
