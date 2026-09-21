// test_handlecontrol_cabi.cpp — HandleControl 集成扩展 C ABI 测试（纯 DLL 动态加载）
// 覆盖（设计 HandleControl_CABI_Integration_Design §5 验收）：
//   H1 SetHandleTarget 切换：三次切换后 hitTest 在新 target 手柄位置命中；NULL → detach 后 hitTest 返回 0
//   H2 HandleHitTest：SE 手柄中心命中=6、Move 中心命中=1、target 本体非手柄区=0
//   H3 RectFilter：filter 内 snap(10) → 拖拽过程（MouseMove 帧）target rect 已吸附（实时生效）；
//      filter 返回 0 → 行为与现状一致
//   H4 SetHandleMoveVisible(0) → 中心 Move 手柄不命中（不绘制/不可拖）
// 依赖：CreateHandleControl/SetHandleTarget/HandleHitTest/SetHandleRectFilter/SetHandleMoveVisible
//       + PushUIEvent 鼠标注入（test_multi_instance_cabi 范式）+ CreateButton/SetFloat rect 读回
#include "UICornerstoneAPI.h"
#include "EventTypes.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _MSC_VER
#define DISABLE_ASSERT_DIALOG() _set_error_mode(_OUT_TO_STDERR), _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT)
#else
#define DISABLE_ASSERT_DIALOG() ((void)0)
#endif

static HMODULE g_dll = nullptr;

typedef UIInstance (*UICreateInstanceFn)(const char*, const UIInstanceConfig*);
typedef void       (*UIDestroyInstanceFn)(UIInstance);
typedef int        (*UIProcessEventsFn)(UIInstance);
typedef void       (*UIUpdateFn)(UIInstance, double);
typedef void       (*UIClearFn)(UIInstance);
typedef void       (*UIRenderFn)(UIInstance);
typedef void       (*UIPresentFn)(UIInstance);
typedef int        (*UIIsQuitRequestedFn)(UIInstance);
typedef void*      (*UICreateButtonFn)(UIInstance, const char*, float, float, float, float, float, float);
typedef void*      (*UICreateHandleFn)(UIInstance, void*, float, float, float, float, float, float);
typedef int        (*UISetHandleTargetFn)(UIInstance, void*, void*);
typedef int        (*UIHandleHitTestFn)(UIInstance, void*, float, float, int*);
typedef int        (*UISetHandleRectFilterFn)(UIInstance, void*, UIHandleRectFilter, void*);
typedef int        (*UISetHandleMoveVisibleFn)(UIInstance, void*, int);
typedef void       (*UIGetRectFn)(UIInstance, void*, float*, float*, float*, float*);
typedef void       (*UIPushUIEventFn)(UIInstance, const UIEvent*);

static UICreateInstanceFn         uiCreateInstanceFromPlugin = nullptr;
static UIDestroyInstanceFn        uiDestroyInstance = nullptr;
static UIProcessEventsFn          uiProcessEvents = nullptr;
static UIUpdateFn                 uiUpdate = nullptr;
static UIClearFn                  uiClear = nullptr;
static UIRenderFn                 uiRender = nullptr;
static UIPresentFn                uiPresent = nullptr;
static UIIsQuitRequestedFn        uiIsQuitRequested = nullptr;
static UICreateButtonFn           uiCreateButton = nullptr;
static UICreateHandleFn           uiCreateHandleControl = nullptr;
static UISetHandleTargetFn        uiSetHandleTarget = nullptr;
static UIHandleHitTestFn          uiHandleHitTest = nullptr;
static UISetHandleRectFilterFn    uiSetHandleRectFilter = nullptr;
static UISetHandleMoveVisibleFn   uiSetHandleMoveVisible = nullptr;
static UIGetRectFn                uiGetRect = nullptr;
static UIPushUIEventFn            uiPushUIEvent = nullptr;

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
    RESOLVE(IsQuitRequested)
    RESOLVE(CreateButton)
    RESOLVE(CreateHandleControl)
    RESOLVE(SetHandleTarget)
    RESOLVE(HandleHitTest)
    RESOLVE(SetHandleRectFilter)
    RESOLVE(SetHandleMoveVisible)
    RESOLVE(GetRect)
    RESOLVE(PushUIEvent)
#undef RESOLVE
    return true;
}


// ── 鼠标注入（test_multi_instance_cabi 范式）──
static void mouseEvent(UIInstance inst, int type, float x, float y) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = static_cast<UIEventType>(type);
    UI_EVENT_MOUSE_X(&ev) = x;
    UI_EVENT_MOUSE_Y(&ev) = y;
    if (type == UI_EVENT_MOUSE_DOWN || type == UI_EVENT_MOUSE_UP)
        UI_EVENT_BUTTON(&ev) = static_cast<int>(MouseButton::Left);   // memset 全零 = None(0)，非左键会被忽略
    uiPushUIEvent(inst, &ev);
}

static void frame(UIInstance inst) {
    uiProcessEvents(inst);
    uiClear(inst);
    uiRender(inst);
    uiPresent(inst);
    uiUpdate(inst, 0.016);
}

// H3 snap filter：吸附到 10px 网格（返回 1 = 用修正值）
static int g_filterCalls = 0;
static int snapFilter(UIControlHandle target, float* x, float* y, float* w, float* h, void* ud) {
    (void)target; (void)ud;
    ++g_filterCalls;
    float* io[4] = {x, y, w, h};
    for (int i = 0; i < 4; ++i)
        *io[i] = (*io[i] < 0) ? -((int)(-*io[i] / 10 + 0.5f) * 10) : (int)(*io[i] / 10 + 0.5f) * 10;
    return 1;
}
// H3b 透传 filter：返回 0 = 用原值
static int passFilter(UIControlHandle, float*, float*, float*, float*, void*) { ++g_filterCalls; return 0; }

static int g_pass = 1;
#define CHECK(cond, name) do { \
    if (cond) printf("PASS: %s\n", name); \
    else { printf("FAIL: %s\n", name); g_pass = 0; } } while (0)

int main(int argc, char** argv) {
    DISABLE_ASSERT_DIALOG();
    setvbuf(stdout, NULL, _IONBF, 0);
    int autoSec = 6;
    for (int i = 1; i < argc; ++i)
        if (strncmp(argv[i], "auto=", 5) == 0) autoSec = atoi(argv[i] + 5);
    (void)autoSec;

    g_dll = LoadLibraryA("UICornerstone.dll");
    if (!g_dll) { printf("FAIL: LoadLibrary\n"); return 1; }
    if (!loadAllProcs()) return 1;

    UIInstance inst = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    if (!inst) { printf("FAIL: CreateInstance\n"); return 1; }

    // 目标 A(80,60,200,80) B(80,300,150,60)；手柄附加 A
    void* btnA = uiCreateButton(inst, "A", 80, 60, 200, 80, 1.0f, 1.0f);
    void* btnB = uiCreateButton(inst, "B", 80, 300, 150, 60, 1.0f, 1.0f);
    void* handle = uiCreateHandleControl(inst, btnA, 0, 0, 0, 0, 1.0f, 1.0f);
    frame(inst);

    // ── H2 命中：SE 手柄中心 (target 右下角 280,140) → 6；Move 中心 (180,100) → 1；本体非手柄 (150,120) → 0
    int ht = -1;
    int hitSE = uiHandleHitTest(inst, handle, 280.f, 140.f, &ht);
    CHECK(hitSE == 1 && ht == 6, "H2 SE handle hit type=SE(6)");
    int hitMove = uiHandleHitTest(inst, handle, 180.f, 100.f, &ht);
    CHECK(hitMove == 1 && ht == 1, "H2 Move handle hit type=Move(1)");
    int hitBody = uiHandleHitTest(inst, handle, 150.f, 120.f, &ht);
    CHECK(hitBody == 0 && ht == 0, "H2 body non-handle miss");

    // ── H1 切换：setTarget(B) → SE 手柄位置跟随 B(右下角 230,360)；切回 A；再切 B；最后 NULL detach
    uiSetHandleTarget(inst, handle, btnB);
    frame(inst);
    int hitB = uiHandleHitTest(inst, handle, 230.f, 360.f, &ht);
    CHECK(hitB == 1 && ht == 6, "H1 target switched to B (SE follows B)");

    uiSetHandleTarget(inst, handle, btnA);
    frame(inst);
    int hitA = uiHandleHitTest(inst, handle, 280.f, 140.f, &ht);
    CHECK(hitA == 1 && ht == 6, "H1 target switched back to A");

    uiSetHandleTarget(inst, handle, NULL);
    frame(inst);
    int hitDet = uiHandleHitTest(inst, handle, 280.f, 140.f, &ht);
    CHECK(hitDet == 0 && ht == 0, "H1 detach → no handle hit");

    // 重新附加 A（供 H3/H4）
    uiSetHandleTarget(inst, handle, btnA);
    frame(inst);

    // ── H3 filter：snap(10) 注入 SE 拖拽 (280,140) 按下 → 移动到 (305,165) → snap 后 target 宽高 = 210/90→? 
    //    startRect 200x80，拖 delta(25,25) → 无 filter 宽高 225x105；snap 后 230x110 → 实时生效
    uiSetHandleRectFilter(inst, handle, snapFilter, NULL);
    mouseEvent(inst, UI_EVENT_MOUSE_DOWN, 280.f, 140.f);  frame(inst);
    mouseEvent(inst, UI_EVENT_MOUSE_MOVE, 290.f, 150.f);  frame(inst);
    mouseEvent(inst, UI_EVENT_MOUSE_MOVE, 305.f, 165.f);  frame(inst);
    float rx = 0, ry = 0, w = 0, h = 0;
    uiGetRect(inst, btnA, &rx, &ry, &w, &h);
    CHECK(g_filterCalls >= 2, "H3 filter invoked during drag");
    CHECK(w == 230.f && h == 110.f, "H3 snap applied live during drag (w=230 h=110)");
    mouseEvent(inst, UI_EVENT_MOUSE_UP, 305.f, 165.f);  frame(inst);

    // H3b pass filter（返回 0）→ 原值生效。H3 后 rect=(80,60,230,110) → SE=(310,170)
    uiSetHandleRectFilter(inst, handle, passFilter, NULL);
    mouseEvent(inst, UI_EVENT_MOUSE_DOWN, 310.f, 170.f);  frame(inst);
    mouseEvent(inst, UI_EVENT_MOUSE_MOVE, 335.f, 195.f);  frame(inst);   // delta(25,25)
    mouseEvent(inst, UI_EVENT_MOUSE_UP, 335.f, 195.f);  frame(inst);
    CHECK(g_filterCalls >= 3, "H3b pass-filter invoked");
    // 返回 0 → 未 snap：宽高 230+25=255 / 110+25=135（若 snap 则 260/140）
    float rx2 = 0, ry2 = 0, w2 = 0, h2 = 0;
    uiGetRect(inst, btnA, &rx2, &ry2, &w2, &h2);
    CHECK(w2 == 255.f && h2 == 135.f, "H3b filter=0 keeps raw value (w=255 h=135)");

    // ── H4 Move 手柄关闭：SetHandleMoveVisible(0) → Move 中心 (80+230/2, 60+135/2)=(195,127.5) 不命中
    uiSetHandleMoveVisible(inst, handle, 0);
    frame(inst);
    int hitMoveOff = uiHandleHitTest(inst, handle, 195.f, 127.f, &ht);
    CHECK(hitMoveOff == 0, "H4 Move handle hidden → no hit at center");
    uiSetHandleMoveVisible(inst, handle, 1);

    uiDestroyInstance(inst);
    FreeLibrary(g_dll);
    if (g_pass) { printf("=== PASS: test_handlecontrol_cabi ===\n"); return 0; }
    printf("=== FAIL: test_handlecontrol_cabi ===\n");
    return 1;
}