// test_multiviewport_visual_cabi.cpp — 多视口视觉状态测试
// 遵循测试用例规范：C ABI 测试用 JSON 布局创建控件（LoadLayout + FindControl
// + events.onClick 绑定 RegisterAction），参照 sample_cpp_multiview 场景：
//   1. hover 跨视口隔离（Debug_SetMousePosition 注入窗口绝对坐标驱动 hover）
//   2. 点击聚焦 + 视口焦点独立 → FocusLost 清除活动视口焦点
//   3. 按钮点击 → 读取本视口 EditBox 内容 → Dialog 弹窗显示该内容
//      （弹窗内容 Label 显示自己视口 EditBox 输入的内容；右下视口 Popup
//       不显示 bug 回归：弹窗必须正确显示在自身视口内居中）
//   4. 多视口渲染冒烟
// 运行模式：无参数 = 人工模式（窗口驻留，真实鼠标输入 EditBox → 点击按钮 →
// 观察弹窗显示自己视口输入的内容，关闭窗口退出，无断言）；auto=<秒> =
// 自动断言模式（注入坐标驱动视觉状态，N 秒后退出，无人值守回归）。
// 纯 DLL 动态加载（LoadLibrary + GetProcAddress），Debug 构建运行。
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
typedef int        (*UIProcessEventsFn)(UIInstance);
typedef void       (*UIUpdateFn)(UIInstance, double);
typedef void       (*UIClearFn)(UIInstance);
typedef void       (*UIRenderFn)(UIInstance);
typedef void       (*UIPresentFn)(UIInstance);
typedef int        (*UIIsQuitRequestedFn)(UIInstance);
typedef void       (*UIPushUIEventFn)(UIInstance, const UIEvent*);
typedef UIInstance (*UICreateViewportFn)(UIInstance, UIRect);
typedef void*      (*UIFindControlFn)(UIInstance, const char*);
typedef int        (*UILoadLayoutFn)(UIInstance, const char*);
typedef void       (*UIRegisterActionFn)(UIInstance, const char*, void(*)(void*, void*), void*);
typedef void*      (*UICreateDialogFn)(UIInstance, const char*, const char*, float, float, float, float, float, float);
typedef void*      (*UICreateLabelFn)(UIInstance, const char*, float, float, float, float, float, float, float);
typedef void       (*UIAddChildControlFn)(UIInstance, void*, void*);
typedef int        (*UIGetStringFn)(UIInstance, void*, const char*, char*, int, int*);
typedef void       (*UIGetRectFn)(UIInstance, void*, float*, float*, float*, float*);
typedef int        (*UIGetBoolFn)(UIInstance, void*, const char*, int*);
typedef UIInstance (*UIDebugGetActiveViewportFn)(UIInstance);
typedef int        (*UIDebugIsControlFocusedFn)(UIInstance, void*);
typedef int        (*UIDebugIsControlHoveredFn)(UIInstance, void*);
typedef int        (*UIDebugSetMousePositionFn)(UIInstance, float, float);
typedef int        (*UIDebugGetAliveCountFn)(void);

static UIPluginCreateInstanceFn    uiCreateInstanceFromPlugin  = nullptr;
static UIDestroyInstanceFn         uiDestroyInstance           = nullptr;
static UIProcessEventsFn           uiProcessEvents             = nullptr;
static UIUpdateFn                  uiUpdate                    = nullptr;
static UIClearFn                   uiClear                     = nullptr;
static UIRenderFn                  uiRender                    = nullptr;
static UIPresentFn                 uiPresent                   = nullptr;
static UIIsQuitRequestedFn         uiIsQuitRequested           = nullptr;
static UIPushUIEventFn             uiPushUIEvent               = nullptr;
static UICreateViewportFn          uiCreateViewport            = nullptr;
static UIFindControlFn             uiFindControl               = nullptr;
static UILoadLayoutFn              uiLoadLayout                = nullptr;
static UIRegisterActionFn          uiRegisterAction            = nullptr;
static UICreateDialogFn            uiCreateDialog              = nullptr;
static UICreateLabelFn             uiCreateLabel               = nullptr;
static UIAddChildControlFn         uiAddChildControl           = nullptr;
static UIGetStringFn               uiGetString                 = nullptr;
static int g_gsLen = 0;   // P0-108：GetString 长度查询
static UIGetRectFn                 uiGetRect                   = nullptr;
static UIGetBoolFn                 uiGetBool                   = nullptr;
static UIDebugGetActiveViewportFn  uiDebug_GetActiveViewport   = nullptr;
static UIDebugIsControlFocusedFn   uiDebug_IsControlFocused    = nullptr;
static UIDebugIsControlHoveredFn   uiDebug_IsControlHovered    = nullptr;
static UIDebugSetMousePositionFn   uiDebug_SetMousePosition    = nullptr;
static UIDebugGetAliveCountFn      uiDebug_GetAliveCount       = nullptr;

static HMODULE g_uiDll = nullptr;

static bool loadAllProcs() {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(g_uiDll, "UICornerstone_" #name); \
    if (!ui##name) { printf("FAIL: GetProcAddress(UICornerstone_" #name ")\n"); return false; }
    RESOLVE(CreateInstanceFromPlugin)
    RESOLVE(DestroyInstance)
    RESOLVE(ProcessEvents)
    RESOLVE(Update)
    RESOLVE(Clear)
    RESOLVE(Render)
    RESOLVE(Present)
    RESOLVE(IsQuitRequested)
    RESOLVE(PushUIEvent)
    RESOLVE(CreateViewport)
    RESOLVE(FindControl)
    RESOLVE(LoadLayout)
    RESOLVE(RegisterAction)
    RESOLVE(CreateDialog)
    RESOLVE(CreateLabel)
    RESOLVE(AddChildControl)
    RESOLVE(GetString)
    RESOLVE(GetRect)
    RESOLVE(GetBool)
    RESOLVE(Debug_GetActiveViewport)
    RESOLVE(Debug_IsControlFocused)
    RESOLVE(Debug_IsControlHovered)
    RESOLVE(Debug_SetMousePosition)
    RESOLVE(Debug_GetAliveCount)
#undef RESOLVE
    return true;
}

// ── 按钮回调（JSON events.onClick → RegisterAction）：读取本视口 EditBox
//    内容 → Dialog 弹窗内 Label 显示（弹窗显示自己视口输入的内容；人工模式
//    可真实输入观察） ──
struct ClickCtx {
    UIInstance vp;              // 本视口实例
    void* edit;                 // 本视口 EditBox
    void* lastDialog;           // 最近一次回调创建的弹窗句柄（auto 模式断言用）
    void* lastContentLabel;     // 弹窗内内容 Label（auto 模式断言用）
};
static void onShow(UIControlHandle ctl, void* userData) {
    (void)ctl;
    ClickCtx* ctx = (ClickCtx*)userData;
    char buf[128] = {0};
    if (!uiGetString(ctx->vp, ctx->edit, "text", buf, sizeof(buf), &g_gsLen)) {
        printf("[show] FAIL: cannot read edit text\n");
        return;
    }
    ctx->lastDialog = uiCreateDialog(ctx->vp, "OK", "", 0, 0, 280, 120, 1.0f, 1.0f);   // 视口内居中弹窗
    // 内容文本：Label 坐标相对 Dialog（弹窗 280x120，内容区居中）
    ctx->lastContentLabel = uiCreateLabel(ctx->vp, buf, 14.0f, 20, 30, 240, 60, 1.0f, 1.0f);
    uiAddChildControl(ctx->vp, ctx->lastDialog, ctx->lastContentLabel);
    printf("[show] popup displays: \"%s\"\n", buf);
}

// ── 事件注入辅助（仅 auto 模式使用） ──
static void injectClick(UIInstance inst, float x, float y) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_MOUSE_DOWN;
    UI_EVENT_MOUSE_X(&ev) = x;
    UI_EVENT_MOUSE_Y(&ev) = y;
    UI_EVENT_BUTTON(&ev) = 1;   // MouseButton::Left
    uiPushUIEvent(inst, &ev);
    UIEvent up; memset(&up, 0, sizeof(up));
    up.type = UI_EVENT_MOUSE_UP;
    UI_EVENT_MOUSE_X(&up) = x;
    UI_EVENT_MOUSE_Y(&up) = y;
    UI_EVENT_BUTTON(&up) = 1;   // MouseButton::Left
    uiPushUIEvent(inst, &up);
    uiProcessEvents(inst);
    uiUpdate(inst, 0.016);      // 事件入队后经 eventLoopEntry 分发（click 在 MouseUp 触发）
}

static void injectFocusLost(UIInstance inst) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_FOCUS_LOST;
    uiPushUIEvent(inst, &ev);
    uiProcessEvents(inst);
    uiUpdate(inst, 0.016);
}

static void printUsage(const char* argv0) {
    printf("用法: %s [auto=<秒>]\n", argv0);
    printf("  auto=0（缺省）= 人工模式：窗口驻留，真实鼠标输入 EditBox → 点击按钮 → 观察弹窗显示自己视口输入的内容，关闭窗口退出（无断言）\n");
    printf("  auto=N = 自动断言模式：注入坐标驱动视觉状态并断言，N 秒后退出（无人值守回归）\n");
}

// ── 布局 JSON（JSON 创建控件：验证 LayoutParser 完整链路 + 事件绑定） ──
// 各视口内：Label(标题) + EditBox(12,42,220,28) + Button(12,82,100,30)
// vpA 内按钮绝对中心 (62,97)、EditBox 绝对中心 (122,56)；
// vpB 内按钮绝对中心 (512+62,384+97)=(574,481)、EditBox (512+122,384+56)=(634,440)
static const char* kLayoutJsonA = R"json({
    "controls": [
        {"type": "label",   "id": "lblTitle", "caption": "Bench A",          "rect": {"x": 12, "y": 10, "w": 200, "h": 24}},
        {"type": "edit-box", "id": "edit",     "text": "Hello from Bench A",  "rect": {"x": 12, "y": 42, "w": 220, "h": 28}},
        {"type": "button",  "id": "btnShow",  "caption": "Show",             "rect": {"x": 12, "y": 82, "w": 100, "h": 30}, "events": {"onClick": "show"}}
    ]
})json";
static const char* kLayoutJsonB = R"json({
    "controls": [
        {"type": "label",   "id": "lblTitle", "caption": "Bench B",          "rect": {"x": 12, "y": 10, "w": 200, "h": 24}},
        {"type": "edit-box", "id": "edit",     "text": "Hello from Bench B",  "rect": {"x": 12, "y": 42, "w": 220, "h": 28}},
        {"type": "button",  "id": "btnShow",  "caption": "Show",             "rect": {"x": 12, "y": 82, "w": 100, "h": 30}, "events": {"onClick": "show"}}
    ]
})json";

int main(int argc, char** argv) {
    DISABLE_ASSERT_DIALOG();
    int autoSec = 0;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) {
            autoSec = std::atoi(argv[i] + 5);
        } else {
            printf("WARN: 忽略无法识别的参数: %s\n", argv[i]);
        }
    }
    printf("模式: %s\n", autoSec ? "auto（自动断言）" : "人工（窗口驻留，关闭窗口退出）");

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary(UICornerstone.dll)\n"); return 1; }
    if (!loadAllProcs()) { FreeLibrary(g_uiDll); return 1; }

    // ── 单窗口双视口（默认 1024×768），布局参照 sample_cpp_multiview ──
    UIInstance win = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    assert(win);
    UIInstance vpA = uiCreateViewport(win, UIRect{0, 0, 512, 384});
    UIInstance vpB = uiCreateViewport(win, UIRect{512, 384, 512, 384});
    assert(vpA && vpB && vpA != vpB);
    assert(uiDebug_GetActiveViewport(win) == vpA);  // 首子视口自动 active

    // 各视口注册 action + JSON 布局创建控件（先注册 action，LoadLayout 时绑定）
    ClickCtx ctxA = { vpA, NULL, NULL, NULL };
    ClickCtx ctxB = { vpB, NULL, NULL, NULL };
    uiRegisterAction(vpA, "show", onShow, &ctxA);
    uiRegisterAction(vpB, "show", onShow, &ctxB);
    assert(uiLoadLayout(vpA, kLayoutJsonA) == 1);
    assert(uiLoadLayout(vpB, kLayoutJsonB) == 1);
    void* editA = uiFindControl(vpA, "edit");
    void* btnA  = uiFindControl(vpA, "btnShow");
    void* editB = uiFindControl(vpB, "edit");
    void* btnB  = uiFindControl(vpB, "btnShow");
    ctxA.edit = editA;
    ctxB.edit = editB;
    assert(editA && btnA && editB && btnB);
    uiProcessEvents(win);
    uiUpdate(vpA, 0.016);
    uiUpdate(vpB, 0.016);

    // ── auto 模式：自动断言视觉状态 ──
    if (autoSec > 0) {
        // 视觉 1：hover 跨视口隔离
        // 鼠标在 vpA 按钮上（vpB 注入窗口外坐标）→ vpA 按钮 hover、vpB 按钮不 hover
        assert(uiDebug_SetMousePosition(vpA, 62.0f, 97.0f) == 1);
        assert(uiDebug_SetMousePosition(vpB, 5000.0f, 5000.0f) == 1);
        uiUpdate(vpA, 0.016);
        uiUpdate(vpB, 0.016);
        assert(uiDebug_IsControlHovered(win, btnA) == 1);
        assert(uiDebug_IsControlHovered(win, btnB) == 0);
        assert(uiDebug_IsControlHovered(win, editA) == 0);

        // 反向：鼠标在 vpB 按钮上 → vpB hover、vpA 清除
        assert(uiDebug_SetMousePosition(vpA, 5000.0f, 5000.0f) == 1);
        assert(uiDebug_SetMousePosition(vpB, 574.0f, 481.0f) == 1);
        uiUpdate(vpA, 0.016);
        uiUpdate(vpB, 0.016);
        assert(uiDebug_IsControlHovered(win, btnB) == 1);
        assert(uiDebug_IsControlHovered(win, btnA) == 0);
        printf("PASS: hover cross-viewport isolation\n");

        // 视觉 2：点击聚焦 + 视口焦点独立 → FocusLost 清除
        injectClick(vpA, 122.0f, 56.0f);    // 点击 vpA 的 EditBox
        injectClick(vpB, 634.0f, 440.0f);   // 点击 vpB 的 EditBox
        assert(uiDebug_IsControlFocused(win, editA) == 1);
        assert(uiDebug_IsControlFocused(win, editB) == 1);   // 两个视口各自焦点环并存

        // vpA 失去焦点 → vpA 焦点清除，vpB 保留（activeViewport 分支）
        injectFocusLost(vpA);
        assert(uiDebug_IsControlFocused(win, editA) == 0);
        assert(uiDebug_IsControlFocused(win, editB) == 1);
        printf("PASS: dual viewport focus rings then FocusLost clears vpA\n");

        // 视觉 3：点击按钮 → 弹窗显示本视口 EditBox 的内容 + 视口内居中
        // （原 sample_cpp_multiview 的 Popup 不显示 bug 回归：右下视口弹窗
        //   必须正确显示在自身视口内，且内容为自身 EditBox 的输入）
        injectClick(vpB, 574.0f, 481.0f);   // 点击 vpB 的按钮
        uiProcessEvents(vpB);
        uiUpdate(vpB, 0.016);
        assert(ctxB.lastDialog != NULL);        // 回调已触发（JSON 事件绑定生效）
        assert(ctxB.lastContentLabel != NULL);  // 弹窗内容 Label 已创建
        int visB = 0;
        assert(uiGetBool(vpB, ctxB.lastDialog, "visible", &visB) == 1 && visB == 1);
        float bx = 0, by = 0, bw = 0, bh = 0;
        uiGetRect(vpB, ctxB.lastDialog, &bx, &by, &bw, &bh);
        // 视口 512×384，弹窗 280×120 在视口内居中：(512-280)/2=116, (384-120)/2=132
        // —— 父相对本地坐标，不越界
        assert(bw == 280.0f && bh == 120.0f);
        assert(bx == 116.0f && by == 132.0f);
        char content[128] = {0};
        assert(uiGetString(vpB, ctxB.lastContentLabel, "caption", content, sizeof(content), &g_gsLen) == 1, &g_gsLen);
        assert(strcmp(content, "Hello from Bench B") == 0);  // 弹窗显示本视口 EditBox 内容
        printf("PASS: button click -> dialog shows viewport edit content (\"%s\", rect=%g,%g,%g,%g)\n",
            content, bx, by, bw, bh);

        // 渲染冒烟：各视口 clip 渲染 + owner present
        uiRender(vpA);
        uiRender(vpB);
        uiPresent(win);
        printf("PASS: multiviewport render smoke\n");
    }

    // ── 帧循环：auto 模式超时退出；人工模式等窗口关闭 ──
    uint64_t t0 = GetTickCount64();
    while (!uiIsQuitRequested(win)) {
        if (autoSec && GetTickCount64() - t0 >= (uint64_t)autoSec * 1000) break;
        uiProcessEvents(win);
        uiUpdate(vpA, 1.0 / 60.0);
        uiUpdate(vpB, 1.0 / 60.0);
        uiClear(win);
        uiRender(vpA);
        uiRender(vpB);
        uiPresent(win);
    }
    printf("窗口已关闭（%s）\n", autoSec ? "自动超时" : "人工");

    // ── 销毁 + 泄漏检查（auto 模式断言） ──
    uiDestroyInstance(vpA);
    uiDestroyInstance(vpB);
    uiDestroyInstance(win);
    if (autoSec > 0) {
        assert(uiDebug_GetAliveCount() == 0);
        printf("PASS: destroy all, alive=%d\n", uiDebug_GetAliveCount());
    }

    FreeLibrary(g_uiDll);
    if (autoSec > 0) {
        printf("ALL PASS: multiviewport visual states (CABI dynamic DLL)\n");
        return 0;
    }
    printf("MANUAL MODE EXIT\n");
    return 0;
}
