// test_image.cpp — Image 图片控件测试（Image_Design.md §7 T1-T8）
// 纯 DLL 动态加载方式：LoadLibrary("UICornerstone.dll") + GetProcAddress 解析全部
// C ABI 函数，经 CreateInstanceFromPlugin → 核心 DLL 内部 LoadLibrary(UIBackend_xxx.dll)
// + GetUIBackendCallbacks 回调表创建实例（结构仿 test_multi_instance_cabi.cpp）。
// 非 DLL 构建树（无 UICornerstone.dll）下跳过运行（返回 0）。
#include "UICornerstoneAPI.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#ifdef _MSC_VER
#define DISABLE_ASSERT_DIALOG() _set_error_mode(_OUT_TO_STDERR), _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT)
#else
#define DISABLE_ASSERT_DIALOG() ((void)0)
#endif

// ===== C ABI 函数指针（动态加载） =====
typedef UIInstance (*UIPluginCreateInstanceFn)(const char*, const UIInstanceConfig*);
typedef void       (*UIDestroyInstanceFn)(UIInstance);
typedef void       (*UIProcessEventsFn)(UIInstance);
typedef void       (*UIUpdateFn)(UIInstance, double);
typedef void       (*UIRenderFn)(UIInstance);
typedef void       (*UIPushUIEventFn)(UIInstance, const UIEvent*);
typedef int        (*UIIsQuitRequestedFn)(UIInstance);
typedef void*      (*UICreateImageFn)(UIInstance, const char*, float, float, float, float, float, float);
typedef void*      (*UICreateButtonFn)(UIInstance, const char*, float, float, float, float, float, float);
typedef void*      (*UICreatePanelFn)(UIInstance, float, float, float, float, float, float);
typedef void       (*UIAddChildControlFn)(UIInstance, void*, void*);
typedef void       (*UIDestroyControlFn)(UIInstance, void*);
typedef void       (*UISetRectFn)(UIInstance, void*, float, float, float, float);
typedef void       (*UIGetRectFn)(UIInstance, void*, float*, float*, float*, float*);
typedef int        (*UISetCallbackFn)(UIInstance, void*, const char*, void(*)(void*, const UIEventData*, void*), void*);
typedef int        (*UISetEnumFn)(UIInstance, void*, const char*, const char*);
typedef int        (*UIGetEnumFn)(UIInstance, void*, const char*, char*, int);
typedef int        (*UISetBoolFn)(UIInstance, void*, const char*, int);
typedef int        (*UIGetBoolFn)(UIInstance, void*, const char*, int*);
typedef int        (*UISetIntFn)(UIInstance, void*, const char*, int);
typedef int        (*UIGetIntFn)(UIInstance, void*, const char*, int*);
typedef int        (*UISetStringFn)(UIInstance, void*, const char*, const char*);
typedef int        (*UIActorSetSourceRectFn)(UIInstance, void*, float, float, float, float);
typedef int        (*UIGetStringFn)(UIInstance, void*, const char*, char*, int);
typedef int        (*UIDebugGetAliveCountFn)(void);

static UIPluginCreateInstanceFn uiCreateInstanceFromPlugin = nullptr;
static UIDestroyInstanceFn      uiDestroyInstance          = nullptr;
static UIProcessEventsFn        uiProcessEvents            = nullptr;
static UIUpdateFn               uiUpdate                   = nullptr;
static UIRenderFn               uiRender                   = nullptr;
static UIPushUIEventFn          uiPushUIEvent              = nullptr;
static UIIsQuitRequestedFn      uiIsQuitRequested          = nullptr;
static UICreateImageFn          uiCreateImage              = nullptr;
static UICreateButtonFn         uiCreateButton             = nullptr;
static UICreatePanelFn          uiCreatePanel              = nullptr;
static UIAddChildControlFn      uiAddChildControl          = nullptr;
static UIDestroyControlFn       uiDestroyControl           = nullptr;
static UISetRectFn              uiSetRect                  = nullptr;
static UIGetRectFn              uiGetRect                  = nullptr;
static UISetCallbackFn          uiSetCallback              = nullptr;
static UISetEnumFn              uiSetEnum                  = nullptr;
static UIGetEnumFn              uiGetEnum                  = nullptr;
static UISetBoolFn              uiSetBool                  = nullptr;
static UIGetBoolFn              uiGetBool                  = nullptr;
static UISetIntFn               uiSetInt                   = nullptr;
static UIGetIntFn               uiGetInt                   = nullptr;
static UISetStringFn            uiSetString                = nullptr;
static UIActorSetSourceRectFn   uiActorSetSourceRect       = nullptr;
static UIGetStringFn            uiGetString                = nullptr;
static UIDebugGetAliveCountFn   uiDebug_GetAliveCount      = nullptr;

static HMODULE g_uiDll = nullptr;

static bool loadAllProcs() {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(g_uiDll, "UICornerstone_" #name); \
    if (!ui##name) { printf("FAIL: GetProcAddress(UICornerstone_" #name ")\n"); return false; }
    RESOLVE(CreateInstanceFromPlugin)
    RESOLVE(DestroyInstance)
    RESOLVE(ProcessEvents)
    RESOLVE(Update)
    RESOLVE(Render)
    RESOLVE(PushUIEvent)
    RESOLVE(IsQuitRequested)
    RESOLVE(CreateImage)
    RESOLVE(CreateButton)
    RESOLVE(CreatePanel)
    RESOLVE(AddChildControl)
    RESOLVE(DestroyControl)
    RESOLVE(SetRect)
    RESOLVE(GetRect)
    RESOLVE(SetCallback)
    RESOLVE(SetEnum)
    RESOLVE(GetEnum)
    RESOLVE(SetBool)
    RESOLVE(GetBool)
    RESOLVE(SetInt)
    RESOLVE(GetInt)
    RESOLVE(SetString)
    RESOLVE(ActorSetSourceRect)
    RESOLVE(GetString)
    RESOLVE(Debug_GetAliveCount)
#undef RESOLVE
    return true;
}

static void clickCb(UIControlHandle ctl, const UIEventData* event, void* userData) {
    (void)ctl; (void)event; (*(int*)userData)++;
}

static void injectMouseClick(UIInstance inst, float x, float y) {
    UIEvent down; memset(&down, 0, sizeof(down));
    down.type = UI_EVENT_MOUSE_DOWN;
    UI_EVENT_MOUSE_X(&down) = x;
    UI_EVENT_MOUSE_Y(&down) = y;
    UI_EVENT_BUTTON(&down) = 1;
    uiPushUIEvent(inst, &down);
    UIEvent up; memset(&up, 0, sizeof(up));
    up.type = UI_EVENT_MOUSE_UP;
    UI_EVENT_MOUSE_X(&up) = x;
    UI_EVENT_MOUSE_Y(&up) = y;
    UI_EVENT_BUTTON(&up) = 1;
    uiPushUIEvent(inst, &up);
    uiProcessEvents(inst);
    uiUpdate(inst, 0.016);
}

static void runFrame(UIInstance inst) {
    uiProcessEvents(inst);
    uiUpdate(inst, 0.016);
    uiRender(inst);
}

static void getRectOf(UIInstance inst, UIControlHandle ctl, float* x, float* y, float* w, float* h) {
    uiGetRect(inst, ctl, x, y, w, h);
}

static bool rectEq(float x1, float y1, float w1, float h1,
                   float x2, float y2, float w2, float h2) {
    return x1 == x2 && y1 == y2 && w1 == w2 && h1 == h2;
}

int main(int argc, char* argv[]) {
    DISABLE_ASSERT_DIALOG();
    // 命令行参数任意顺序：识别 auto=<秒>（本测试为有限步骤逻辑测试，
    // 无窗口循环，接受参数但不改变行为）；无法识别参数 WARN 后忽略
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) {
            printf("auto= 已接受（本测试无窗口循环，直接执行）\n");
        } else {
            printf("WARN: 忽略无法识别的参数: %s\n", argv[i]);
        }
    }

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) {
        printf("SKIP: UICornerstone.dll not present (non-DLL build tree)\n");
        return 0;
    }
    if (!loadAllProcs()) { FreeLibrary(g_uiDll); return 1; }
    printf("OK: dynamically loaded UICornerstone.dll\n");

    UIInstance inst = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    assert(inst);

    // ── T1 工厂创建（显式 w/h → rect 保留，Image_Design §6.1） ──
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                            10.0f, 20.0f, 100.0f, 50.0f, 1.0f, 1.0f);
        assert(img);
        float x, y, w, h;
        getRectOf(inst, img, &x, &y, &w, &h);
        assert(rectEq(x, y, w, h, 10, 20, 100, 50));
        printf("PASS: T1 explicit rect preserved (%.0f,%.0f,%.0f,%.0f)\n", x, y, w, h);
        uiDestroyControl(inst, img);
    }

    // ── T2 自然尺寸（w/h=0 → 纹理 32x32） ──
    float naturalW = 0, naturalH = 0;
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png", 0, 0, 0, 0, 1.0f, 1.0f);
        assert(img);
        float x, y, w, h;
        getRectOf(inst, img, &x, &y, &w, &h);
        assert(rectEq(x, y, w, h, 0, 0, 32, 32));
        naturalW = w; naturalH = h;
        printf("PASS: T2 natural size (%.0f x %.0f)\n", w, h);
        uiDestroyControl(inst, img);
    }

    // ── T3 属性往返 + 只写不读 ──
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                            0, 0, 64, 64, 1.0f, 1.0f);
        assert(img);
        char buf[64];

        assert(uiSetEnum(inst, img, "scale-type", "fit-center") == 1);
        assert(uiGetEnum(inst, img, "scale-type", buf, sizeof(buf)) == 1);
        assert(strcmp(buf, "fit-center") == 0);

        assert(uiSetEnum(inst, img, "anchor", "center") == 1);
        assert(uiGetEnum(inst, img, "anchor", buf, sizeof(buf)) == 1);
        assert(strcmp(buf, "center") == 0);

        assert(uiSetBool(inst, img, "match-parent-rect", 1) == 1);
        int b = 0;
        assert(uiGetBool(inst, img, "match-parent-rect", &b) == 1 && b == 1);

        assert(uiSetInt(inst, img, "alpha", 128) == 1);
        int v = 0;
        assert(uiGetInt(inst, img, "alpha", &v) == 1 && v == 128);

        // image 只写不读（fs::path 悬垂语义）
        assert(uiGetString(inst, img, "image", buf, sizeof(buf)) == 0);
        assert(uiSetString(inst, img, "image", "assets/images/cross_down.png") == 1);
        printf("PASS: T3 property round-trip + write-only image\n");
        uiDestroyControl(inst, img);
    }

    // ── T8 scale-type=tile 枚举回环（v1.1.1 平铺）──
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                            0, 0, 64, 64, 1.0f, 1.0f);
        assert(img);
        char buf[64];
        assert(uiSetEnum(inst, img, "scale-type", "tile") == 1);
        assert(uiGetEnum(inst, img, "scale-type", buf, sizeof(buf)) == 1);
        assert(strcmp(buf, "tile") == 0);
        // 非法枚举仍拒绝
        assert(uiSetEnum(inst, img, "scale-type", "mosaic") == 0);
        printf("PASS: T8 scale-type tile round-trip + invalid rejected\n");
        uiDestroyControl(inst, img);
    }

    // ── T9 ActorSetSourceRect：设置/清除语义 + 无效句柄拒绝 ──
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                            0, 0, 64, 64, 1.0f, 1.0f);
        assert(img);
        assert(uiActorSetSourceRect(inst, img, 10.f, 10.f, 20.f, 20.f) == 1);   // 设置
        assert(uiActorSetSourceRect(inst, img, -1.f, 0.f, 20.f, 20.f) == 1);    // x<0 清除
        assert(uiActorSetSourceRect(inst, img, 0.f, 0.f, 0.f, 20.f) == 1);      // w<=0 清除
        assert(uiActorSetSourceRect(inst, img, 0.f, 0.f, 20.f, 0.f) == 1);      // h<=0 清除
        assert(uiActorSetSourceRect(inst, img, 5.f, 5.f, 16.f, 16.f) == 1);     // 重设
        assert(uiActorSetSourceRect(inst, img, -1.f, -1.f, 0.f, 0.f) == 1);     // 清除
        assert(uiActorSetSourceRect(inst, NULL, 0.f, 0.f, 10.f, 10.f) == 0);    // 无效句柄
        printf("PASS: T9 ActorSetSourceRect set/clear/invalid-handle\n");
        uiDestroyControl(inst, img);
    }

    // ── T4 matchParentRect：挂 Panel 下开启 → 跟随父矩形 ──
    {
        UIControlHandle pnl = uiCreatePanel(inst, 0, 0, 200.0f, 100.0f, 1.0f, 1.0f);
        assert(pnl);
        UIControlHandle img = uiCreateImage(inst, NULL, 5, 5, 50, 50, 1.0f, 1.0f);
        assert(img);
        assert(uiSetBool(inst, img, "match-parent-rect", 1) == 1);
        uiAddChildControl(inst, pnl, img);
        float x, y, w, h;
        getRectOf(inst, img, &x, &y, &w, &h);
        assert(rectEq(x, y, w, h, 5, 5, 200, 100));
        printf("PASS: T4 match-parent-rect (%.0f x %.0f)\n", w, h);
        uiDestroyControl(inst, pnl);
    }

    // ── T5 事件遮挡修正：Button 与 Image 重叠，两种叠加顺序 ──
    {
        // A：先 Button 后 Image（Image 在上层，isContainsPoint=false → 不遮挡）
        {
            UIControlHandle btn = uiCreateButton(inst, "BTN", 50, 50, 100, 40, 1.0f, 1.0f);
            UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                                50, 50, 100, 40, 1.0f, 1.0f);
            assert(btn && img);
            int fired = 0;
            assert(uiSetCallback(inst, btn, "click", clickCb, &fired) == 1);
            runFrame(inst);
            injectMouseClick(inst, 100, 70);
            assert(fired == 1);
            printf("PASS: T5A image-on-top does not occlude button (fired=%d)\n", fired);
            uiDestroyControl(inst, btn);
            uiDestroyControl(inst, img);
        }
        // B：先 Image 后 Button（Button 在上层，直接命中）
        {
            UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                                50, 50, 100, 40, 1.0f, 1.0f);
            UIControlHandle btn = uiCreateButton(inst, "BTN", 50, 50, 100, 40, 1.0f, 1.0f);
            assert(img && btn);
            int fired = 0;
            assert(uiSetCallback(inst, btn, "click", clickCb, &fired) == 1);
            runFrame(inst);
            injectMouseClick(inst, 100, 70);
            assert(fired == 1);
            printf("PASS: T5B button-on-top (fired=%d)\n", fired);
            uiDestroyControl(inst, img);
            uiDestroyControl(inst, btn);
        }
    }

    // ── T6 锚点：各值枚举往返（渲染冒烟部分并入 T7） ──
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png", 0, 0, 64, 64, 1.0f, 1.0f);
        assert(img);
        const char* anchors[] = { "top-left", "mid-left", "bottom-left", "top-right",
                                  "mid-right", "bottom-right", "top-center", "center",
                                  "bottom-center" };
        char buf[64];
        for (int i = 0; i < 9; i++) {
            assert(uiSetEnum(inst, img, "anchor", anchors[i]) == 1);
            assert(uiGetEnum(inst, img, "anchor", buf, sizeof(buf)) == 1);
            assert(strcmp(buf, anchors[i]) == 0);
        }
        printf("PASS: T6 anchor enum 9 values\n");
        uiDestroyControl(inst, img);
    }

    // ── T7 渲染冒烟：60 帧循环 ──
    {
        UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png", 10, 10, 80, 80, 1.0f, 1.0f);
        assert(img);
        assert(uiSetEnum(inst, img, "scale-type", "center-crop") == 1);
        assert(uiSetInt(inst, img, "alpha", 200) == 1);
        for (int i = 0; i < 60; i++) {
            runFrame(inst);
        }
        assert(uiIsQuitRequested(inst) == 0);
        printf("PASS: T7 render smoke 60 frames\n");
        uiDestroyControl(inst, img);
    }

    // ── T8 挂树后换图：显式 rect 保留；自然尺寸跟随新图 ──
    {
        // a 显式 rect
        {
            UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png",
                                                5, 5, 64, 64, 1.0f, 1.0f);
            assert(img);
            runFrame(inst);
            assert(uiSetString(inst, img, "image", "assets/images/down.png") == 1);
            runFrame(inst);
            float x, y, w, h;
            getRectOf(inst, img, &x, &y, &w, &h);
            assert(rectEq(x, y, w, h, 5, 5, 64, 64));
            printf("PASS: T8a explicit rect kept after reload (%.0f x %.0f)\n", w, h);
            uiDestroyControl(inst, img);
        }
        // b 自然尺寸跟随新图（cross_up 32x32 → down 48x48）
        {
            UIControlHandle img = uiCreateImage(inst, "assets/images/cross_up.png", 0, 0, 0, 0, 1.0f, 1.0f);
            assert(img);
            runFrame(inst);
            assert(uiSetString(inst, img, "image", "assets/images/down.png") == 1);
            runFrame(inst);
            float x, y, w, h;
            getRectOf(inst, img, &x, &y, &w, &h);
            assert(rectEq(x, y, w, h, 0, 0, 48, 48));
            printf("PASS: T8b natural size follows new image (%.0f x %.0f)\n", w, h);
            uiDestroyControl(inst, img);
        }
    }

    uiDestroyInstance(inst);
    assert(uiDebug_GetAliveCount() == 0);

    FreeLibrary(g_uiDll);
    printf("ALL PASS: image control T1-T8\n");
    return 0;
}