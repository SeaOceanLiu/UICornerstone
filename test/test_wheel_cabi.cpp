// test_wheel_cabi.cpp — Panel 容器级滚轮事件 + ScrollBar 滚轮 C ABI 测试（纯 DLL 动态加载）
// 覆盖（设计 Wheel_Support_Design §5 验收）：
//   W1 Panel wheel 回调：面板内注入 wheel → "mouse-wheel" 回调触发，floatVal == 注入 scrollY
//   W2 子控件消费优先：wheel 落在 Panel 内 ScrollBar 上 → ScrollBar value 变化，Panel 回调不触发（单次消费）
//   W3 ScrollBar 方向：scrollY=+1（向上滚）→ value - step；scrollY=-1 → value + step；越界 clamp
//   W4 面板外不触发：wheel 坐标在面板外 → 无回调
// 注入：UI_EVENT_WHEEL_DELTA(data+0)=scrollY / WHEEL_MOUSE_X(data+4) / WHEEL_MOUSE_Y(data+8)
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
typedef void*      (*UICreateScrollBarFn)(UIInstance, float, float, float, float, int, float, float);
typedef int        (*UISetFloatFn)(UIInstance, void*, const char*, float);
typedef int        (*UIGetFloatFn)(UIInstance, void*, const char*, float*);
typedef int        (*UISetCallbackFn)(UIInstance, void*, const char*, UIEventCallback, void*);
typedef void       (*UIAddChildFn)(UIInstance, void*, void*);
typedef void*      (*UICreateTextAreaFn)(UIInstance, float, float, float, float, float, float);
typedef void*      (*UICreateListViewFn)(UIInstance, float, float, float, float, float, float);
typedef void*      (*UICreateNumericUpDownFn)(UIInstance, float, float, float, float, float, float);
typedef void       (*UIPushUIEventFn)(UIInstance, const UIEvent*);

static UICreateInstanceFn   uiCreateInstanceFromPlugin = nullptr;
static UIDestroyInstanceFn  uiDestroyInstance = nullptr;
static UIProcessEventsFn    uiProcessEvents = nullptr;
static UIUpdateFn           uiUpdate = nullptr;
static UIClearFn            uiClear = nullptr;
static UIRenderFn           uiRender = nullptr;
static UIPresentFn          uiPresent = nullptr;
static UICreatePanelFn      uiCreatePanel = nullptr;
static UICreateScrollBarFn  uiCreateScrollBar = nullptr;
static UISetFloatFn         uiSetFloat = nullptr;
static UIGetFloatFn         uiGetFloat = nullptr;
static UISetCallbackFn      uiSetCallback = nullptr;
static UIPushUIEventFn      uiPushUIEvent = nullptr;
static UIAddChildFn         uiAddChildControl = nullptr;
static UICreateTextAreaFn   uiCreateTextArea = nullptr;
static UICreateListViewFn   uiCreateListView = nullptr;
static UICreateNumericUpDownFn uiCreateNumericUpDown = nullptr;
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
    RESOLVE(CreateScrollBar)
    RESOLVE(SetFloat)
    RESOLVE(GetFloat)
    RESOLVE(SetCallback)
    RESOLVE(PushUIEvent)
    RESOLVE(AddChildControl)
    RESOLVE(CreateTextArea)
    RESOLVE(CreateListView)
    RESOLVE(CreateNumericUpDown)
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

static void wheelEvent(UIInstance inst, float x, float y, float scrollY) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_MOUSE_WHEEL;
    UI_EVENT_WHEEL_DELTA(&ev) = scrollY;
    UI_EVENT_WHEEL_MOUSE_X(&ev) = x;
    UI_EVENT_WHEEL_MOUSE_Y(&ev) = y;
    uiPushUIEvent(inst, &ev);
}

// 回调记录
static int   g_panelWheelCalls = 0;
static float g_lastWheelY = 0.f;
static void onPanelWheel(UIControlHandle, const UIEventData* ev, void*) {
    ++g_panelWheelCalls;
    g_lastWheelY = ev->data.floatVal;
}

static int g_pass = 1;
#define CHECK(cond, name) do { \
    if (cond) printf("PASS: %s\n", name); \
    else { printf("FAIL: %s\n", name); g_pass = 0; } } while (0)

int main(int argc, char** argv) {
    DISABLE_ASSERT_DIALOG();
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 1; i < argc; ++i)
        if (strncmp(argv[i], "auto=", 5) == 0) { /* 帧数由用例内 frame 控制 */ }

    g_dll = LoadLibraryA("UICornerstone.dll");
    if (!g_dll) { printf("FAIL: LoadLibrary\n"); return 1; }
    if (!loadAllProcs()) return 1;

    UIInstance inst = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    if (!inst) { printf("FAIL: CreateInstance\n"); return 1; }

    // 面板 P(40,40,300,400)；滚动条 SB 在面板内 (50,50,16,200)（垂直）
    void* panel = uiCreatePanel(inst, 40, 40, 300, 400, 1.0f, 1.0f);
    void* sb = uiCreateScrollBar(inst, 50, 50, 16, 200, 1 /*vertical*/, 1.0f, 1.0f);
    uiSetFloat(inst, sb, "range-min", 0.f);
    uiSetFloat(inst, sb, "range-max", 100.f);
    uiSetFloat(inst, sb, "step-size", 10.f);
    uiSetFloat(inst, sb, "value", 50.f);
    uiSetCallback(inst, panel, "mouse-wheel", onPanelWheel, NULL);
    frame(inst);

    // ── W2+W3a 子控件消费优先：wheel 落在滚动条(50,50,16,200)上，向上滚 scrollY=+1 → value 50-10=40；Panel 回调不触发
    wheelEvent(inst, 58.f, 150.f, 1.f); frame(inst);
    float v = 0; uiGetFloat(inst, sb, "value", &v);
    CHECK(v == 40.f, "W3a wheel up on scrollbar: value 50-10=40");
    CHECK(g_panelWheelCalls == 0, "W2 scrollbar consumes: panel callback NOT fired");

    // W3b 向下滚 scrollY=-1 → value 40+10=50
    wheelEvent(inst, 58.f, 150.f, -1.f); frame(inst);
    uiGetFloat(inst, sb, "value", &v);
    CHECK(v == 50.f, "W3b wheel down: value 40+10=50");

    // ── W1 Panel 容器回调：wheel 在面板内、滚动条外(200,200) → 回调触发，floatVal=scrollY
    wheelEvent(inst, 200.f, 200.f, 1.f); frame(inst);
    CHECK(g_panelWheelCalls == 1 && g_lastWheelY == 1.f, "W1 panel wheel callback fired (floatVal=+1)");
    wheelEvent(inst, 200.f, 200.f, -1.f); frame(inst);
    CHECK(g_panelWheelCalls == 2 && g_lastWheelY == -1.f, "W1b panel callback again (floatVal=-1)");

    // ── W3c clamp：滚轮连滚 12 次（50+120=170 > max100 → clamp 100）
    for (int i = 0; i < 12; ++i) { wheelEvent(inst, 58.f, 150.f, -1.f); frame(inst); }
    uiGetFloat(inst, sb, "value", &v);
    CHECK(v == 100.f, "W3c clamp at range-max (100)");

    // ── W4 面板外不触发：wheel 在 (700,500)（面板外）→ 无回调、滚动条不变
    int callsBefore = g_panelWheelCalls;
    wheelEvent(inst, 700.f, 500.f, 1.f); frame(inst);
    CHECK(g_panelWheelCalls == callsBefore, "W4 wheel outside panel: no callback");

    // ── W5~W7 行控件门控（第三批 P0-1/P0-2 + 通则）：无余量/未聚焦 → 透传容器回调 ──
    // P2(400,40,300,400) 挂回调；内部子控件经 AddChildControl（局部坐标）
    void* p2 = uiCreatePanel(inst, 400, 40, 300, 400, 1.0f, 1.0f);
    uiSetCallback(inst, p2, "mouse-wheel", onPanelWheel, NULL);
    void* ta = uiCreateTextArea(inst, 10, 10, 200, 100, 1.0f, 1.0f);       // 空内容：无滚动余量
    uiAddChildControl(inst, p2, ta);
    void* lv = uiCreateListView(inst, 10, 150, 200, 100, 1.0f, 1.0f);      // 空列表：无滚动余量
    uiAddChildControl(inst, p2, lv);
    void* nu = uiCreateNumericUpDown(inst, 10, 300, 120, 30, 1.0f, 1.0f);  // 未聚焦
    uiAddChildControl(inst, p2, nu);
    frame(inst);

    int base = g_panelWheelCalls;
    wheelEvent(inst, 450.f, 80.f, 1.f);  frame(inst);    // TextArea(屏幕 410,50,200,100) 内
    CHECK(g_panelWheelCalls == base + 1, "W5 TextArea no-scroll-space passthrough -> container fires");
    base = g_panelWheelCalls;
    wheelEvent(inst, 450.f, 220.f, 1.f); frame(inst);    // ListView(屏幕 410,190,200,100) 内
    CHECK(g_panelWheelCalls == base + 1, "W6 ListView no-scroll-space passthrough -> container fires");
    base = g_panelWheelCalls;
    wheelEvent(inst, 450.f, 350.f, 1.f); frame(inst);    // NUD(屏幕 410,340,120,30) 内、未聚焦
    CHECK(g_panelWheelCalls == base + 1, "W7 NumericUpDown unfocused passthrough -> container fires");

    uiDestroyInstance(inst);
    FreeLibrary(g_dll);
    if (g_pass) { printf("=== PASS: test_wheel_cabi ===\n"); return 0; }
    printf("=== FAIL: test_wheel_cabi ===\n");
    return 1;
}