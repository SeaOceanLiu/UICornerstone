// test_wheel_nested_cabi.cpp — P0-3 调查：Panel mouse-wheel 在深层嵌套/遮挡场景的可达性
// 场景（第三批需求 §3 定位建议）：
//   N1 单层 Panel（bench 直接子）挂回调 → wheel 应触发
//   N2 深层嵌套 Panel×5 挂回调 → wheel 应触发
//   N3 经 TabControl 中间层（panel 挂 tc 下）→ wheel 应触发（验证中间层分发）
//   N4 兄弟遮挡：后添加的全幅兄弟覆盖 → 内层回调被遮挡跳过（机制证明）
#include "UICornerstoneAPI.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _MSC_VER
#define DISABLE_ASSERT_DIALOG() _set_error_mode(_OUT_TO_STDERR), _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT)
#else
#define DISABLE_ASSERT_DIALOG() ((void)0)
#endif

typedef UIInstance (*UICreateInstanceFn)(const char*, const UIInstanceConfig*);
typedef void       (*UIDestroyInstanceFn)(UIInstance);
typedef int        (*UIProcessEventsFn)(UIInstance);
typedef void       (*UIUpdateFn)(UIInstance, double);
typedef void       (*UIClearFn)(UIInstance);
typedef void       (*UIRenderFn)(UIInstance);
typedef void       (*UIPresentFn)(UIInstance);
typedef void*      (*UICreatePanelFn)(UIInstance, float, float, float, float, float, float);
typedef void       (*UIAddChildFn)(UIInstance, void*, void*);
typedef int        (*UISetCallbackFn)(UIInstance, void*, const char*, UIEventCallback, void*);
typedef void       (*UIPushUIEventFn)(UIInstance, const UIEvent*);
typedef int        (*UITabAddPageFn)(UIInstance, void*, const char*);
typedef void*      (*UICreateTabControlFn)(UIInstance, float, float, float, float, float, float);

static UICreateInstanceFn   uiCreateInstanceFromPlugin = nullptr;
static UIDestroyInstanceFn  uiDestroyInstance = nullptr;
static UIProcessEventsFn    uiProcessEvents = nullptr;
static UIUpdateFn           uiUpdate = nullptr;
static UIClearFn            uiClear = nullptr;
static UIRenderFn           uiRender = nullptr;
static UIPresentFn          uiPresent = nullptr;
static UICreatePanelFn      uiCreatePanel = nullptr;
static UIAddChildFn         uiAddChildControl = nullptr;
static UISetCallbackFn      uiSetCallback = nullptr;
static UIPushUIEventFn      uiPushUIEvent = nullptr;
static UITabAddPageFn       uiTabAddPage = nullptr;
static UICreateTabControlFn uiCreateTabControl = nullptr;
static HMODULE g_dll = nullptr;

static bool loadAllProcs() {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(g_dll, "UICornerstone_" #name); \
    if (!ui##name) { printf("FAIL: GetProcAddress(UICornerstone_" #name ")\n"); return false; }
    RESOLVE(CreateInstanceFromPlugin)
    RESOLVE(DestroyInstance)
    RESOLVE(ProcessEvents)
    RESOLVE(Update)
    RESOLVE(Clear)
    RESOLVE(Render)
    RESOLVE(Present)
    RESOLVE(CreatePanel)
    RESOLVE(AddChildControl)
    RESOLVE(SetCallback)
    RESOLVE(PushUIEvent)
    RESOLVE(TabAddPage)
    RESOLVE(CreateTabControl)
#undef RESOLVE
    return true;
}

static void frame(UIInstance inst) {
    uiProcessEvents(inst);
    uiClear(inst);
    uiRender(inst);
    uiPresent(inst);
    uiUpdate(inst, 0.016);
}
static void wheelEvent(UIInstance inst, float x, float y, float sy) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_MOUSE_WHEEL;
    UI_EVENT_WHEEL_DELTA(&ev) = sy;
    UI_EVENT_WHEEL_MOUSE_X(&ev) = x;
    UI_EVENT_WHEEL_MOUSE_Y(&ev) = y;
    uiPushUIEvent(inst, &ev);
}

#define MKCb(varName) \
    static int varName##Calls = 0; \
    static void varName(UIControlHandle, const UIEventData* ev, void*) { (void)ev; ++varName##Calls; }
MKCb(cbN1) MKCb(cbN2) MKCb(cbN3) MKCb(cbN4Inner) MKCb(cbN6aInner) MKCb(cbN6bMid)

static int g_pass = 1;
#define CHECK(cond, name) do { \
    if (cond) printf("PASS: %s\n", name); \
    else { printf("FAIL: %s\n", name); g_pass = 0; } } while (0)

int main(void) {
    DISABLE_ASSERT_DIALOG();
    setvbuf(stdout, NULL, _IONBF, 0);

    g_dll = LoadLibraryA("UICornerstone.dll");
    if (!g_dll) { printf("FAIL: LoadLibrary\n"); return 1; }
    if (!loadAllProcs()) return 1;

    UIInstance inst = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    if (!inst) { printf("FAIL: CreateInstance\n"); return 1; }

    // ── N1 单层 Panel（bench 直接子）(40,40,200,200)
    void* p1 = uiCreatePanel(inst, 40, 40, 200, 200, 1.0f, 1.0f);
    uiSetCallback(inst, p1, "mouse-wheel", cbN1, NULL);
    frame(inst);
    wheelEvent(inst, 100.f, 100.f, 1.f); frame(inst);
    printf("[N1] calls=%d\n", cbN1Calls);
    CHECK(cbN1Calls == 1, "N1 single-level panel fires");

    // ── N2 深层嵌套 Panel×5（500,40 起每层偏移 10）
    void* cur = uiCreatePanel(inst, 500, 40, 220, 220, 1.0f, 1.0f);
    for (int i = 0; i < 4; ++i) {
        void* next = uiCreatePanel(inst, 10.f + i * 10.f, 10.f + i * 10.f, 180.f - i * 20.f, 180.f - i * 20.f, 1.0f, 1.0f);
        uiAddChildControl(inst, cur, next);
        cur = next;
    }
    uiSetCallback(inst, cur, "mouse-wheel", cbN2, NULL);
    frame(inst);
    wheelEvent(inst, 650.f, 190.f, 1.f); frame(inst);  // 最内层屏幕 rect=(600,140,100,100) 中心
    printf("[N2] calls=%d\n", cbN2Calls);
    CHECK(cbN2Calls == 1, "N2 deep-nested panel(5) fires");

    // ── N3 经 TabControl 中间层：tc(500,300,220,220)；空页 + panel 挂 tc（普通子）
    void* tc = uiCreateTabControl(inst, 500, 300, 220, 220, 1.0f, 1.0f);
    uiTabAddPage(inst, tc, "P0");
    void* page = uiCreatePanel(inst, 510, 340, 200, 150, 1.0f, 1.0f);  // 先挂 bench
    uiAddChildControl(inst, tc, page);                                  // reparent 到 tc
    uiSetCallback(inst, page, "mouse-wheel", cbN3, NULL);
    frame(inst);
    wheelEvent(inst, 610.f, 400.f, 1.f); frame(inst);
    printf("[N3] calls=%d\n", cbN3Calls);
    CHECK(cbN3Calls == 1, "N3 panel under tab-control fires");

    // ── N4 遮挡机制证明：outer(750,40,200,200) 含 inner(局部 50,40,80,80)；
    //    再创建全幅兄弟 cover(750,40,200,200)（后添加→更顶层）。
    //    wheel 在 inner 中心 → ControlImpl 遮挡检查（cover isContainsPoint）→ inner 整树被跳过
    void* outer = uiCreatePanel(inst, 750, 40, 200, 200, 1.0f, 1.0f);
    void* inner = uiCreatePanel(inst, 50, 40, 80, 80, 1.0f, 1.0f);
    uiAddChildControl(inst, outer, inner);
    uiSetCallback(inst, inner, "mouse-wheel", cbN4Inner, NULL);
    void* cover = uiCreatePanel(inst, 750, 40, 200, 200, 1.0f, 1.0f);
    (void)cover;
    frame(inst);
    wheelEvent(inst, 840.f, 120.f, 1.f); frame(inst);
    printf("[N4] inner=%d\n", cbN4InnerCalls);
    CHECK(cbN4InnerCalls == 0, "N4 covered inner skipped (mechanism proof)");

    // ── N6a（复核放行条件）：三层嵌套——outer 无订阅 / mid 无订阅 / inner 有订阅 / 鼠标在 inner 内
    //     期望：修复前后均触发（内层自身 fire）
    void* o6a = uiCreatePanel(inst, 100, 500, 240, 240, 1.0f, 1.0f);
    void* m6a = uiCreatePanel(inst, 20, 20, 200, 200, 1.0f, 1.0f);
    uiAddChildControl(inst, o6a, m6a);
    void* i6a = uiCreatePanel(inst, 20, 20, 150, 150, 1.0f, 1.0f);
    uiAddChildControl(inst, m6a, i6a);
    uiSetCallback(inst, i6a, "mouse-wheel", cbN6aInner, NULL);
    frame(inst);
    wheelEvent(inst, 215.f, 615.f, 1.f); frame(inst);   // inner 屏幕 (140,540,150,150) 中心
    printf("[N6a] inner=%d\n", cbN6aInnerCalls);
    CHECK(cbN6aInnerCalls == 1, "N6a inner-subscribed fires (mouse inside inner)");

    // ── N6b（空转消费修复验证）：三层嵌套——outer 无订阅 / mid 有订阅 / inner 无订阅 / 鼠标在 inner 内
    //     修复前：inner（无订阅）空转吞 → mid 不触发；修复后：inner 透传 → mid 触发
    void* o6b = uiCreatePanel(inst, 400, 500, 240, 240, 1.0f, 1.0f);
    void* m6b = uiCreatePanel(inst, 20, 20, 200, 200, 1.0f, 1.0f);
    uiAddChildControl(inst, o6b, m6b);
    void* i6b = uiCreatePanel(inst, 20, 20, 150, 150, 1.0f, 1.0f);
    uiAddChildControl(inst, m6b, i6b);
    uiSetCallback(inst, m6b, "mouse-wheel", cbN6bMid, NULL);
    frame(inst);
    wheelEvent(inst, 515.f, 615.f, 1.f); frame(inst);   // inner6b 屏幕 (440,540,150,150) 中心
    printf("[N6b] mid=%d\n", cbN6bMidCalls);
    CHECK(cbN6bMidCalls == 1, "N6b outer-subscribed mid receives (inner unsubscribed passthrough)");

    uiDestroyInstance(inst);
    FreeLibrary(g_dll);
    if (g_pass) { printf("=== PASS: test_wheel_nested_cabi ===\n"); return 0; }
    printf("=== FAIL: test_wheel_nested_cabi ===\n");
    return 1;
}