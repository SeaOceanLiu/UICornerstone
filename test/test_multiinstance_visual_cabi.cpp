// test_multiinstance_visual_cabi.cpp — 多实例（多窗口）视觉状态测试
// 遵循测试用例规范：C ABI 测试用 JSON 布局创建控件（LoadLayout + FindControl
// + events.onClick 绑定 RegisterAction），参照 sample_cpp_multiinstance 场景：
//   1. hover 跨窗口隔离（Debug_SetMousePosition 注入窗口内坐标驱动 hover）
//   2. 点击聚焦 + 焦点环并存（FocusManager 独立）→ FocusLost 清除本实例焦点
//   3. 按钮点击 → 读取本窗口 EditBox 内容 → 显示到对方窗口 Label
//      （跨实例内容传递：另一个 instance 实际显示出发送的内容）
//   4. 双窗口渲染冒烟
// 运行模式：无参数 = 人工模式（窗口驻留，真实鼠标输入 EditBox → 点击按钮 →
// 观察对方窗口显示内容，关闭窗口退出，无断言）；auto=<秒> = 自动断言模式
// （注入坐标驱动视觉状态，N 秒后退出，无人值守回归）。
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
typedef void*      (*UIFindControlFn)(UIInstance, const char*);
typedef int        (*UILoadLayoutFn)(UIInstance, const char*);
typedef void       (*UIRegisterActionFn)(UIInstance, const char*, void(*)(void*, void*), void*);
typedef int        (*UIGetStringFn)(UIInstance, void*, const char*, char*, int, int*);
typedef int        (*UISetStringFn)(UIInstance, void*, const char*, const char*);
typedef void       (*UIGetRectFn)(UIInstance, void*, float*, float*, float*, float*);
typedef int        (*UIGetBoolFn)(UIInstance, void*, const char*, int*);
typedef uint32_t   (*UIGetBackendCapabilitiesFn)(UIInstance);
typedef int        (*UIDebugIsControlFocusedFn)(UIInstance, void*);
typedef int        (*UIDebugIsControlHoveredFn)(UIInstance, void*);
typedef int        (*UIDebugSetMousePositionFn)(UIInstance, float, float);
typedef int        (*UIDebugClearMousePositionFn)(UIInstance);
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
static UIFindControlFn             uiFindControl               = nullptr;
static UILoadLayoutFn              uiLoadLayout                = nullptr;
static UIRegisterActionFn          uiRegisterAction            = nullptr;
static UIGetStringFn               uiGetString                 = nullptr;
static int g_gsLen = 0;   // P0-108：GetString 长度查询
static UISetStringFn               uiSetString                 = nullptr;
static UIGetRectFn                 uiGetRect                   = nullptr;
static UIGetBoolFn                 uiGetBool                   = nullptr;
static UIGetBackendCapabilitiesFn  uiGetBackendCapabilities    = nullptr;
static UIDebugIsControlFocusedFn   uiDebug_IsControlFocused    = nullptr;
static UIDebugIsControlHoveredFn   uiDebug_IsControlHovered    = nullptr;
static UIDebugSetMousePositionFn   uiDebug_SetMousePosition    = nullptr;
static UIDebugClearMousePositionFn uiDebug_ClearMousePosition  = nullptr;
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
    RESOLVE(FindControl)
    RESOLVE(LoadLayout)
    RESOLVE(RegisterAction)
    RESOLVE(GetString)
    RESOLVE(SetString)
    RESOLVE(GetRect)
    RESOLVE(GetBool)
    RESOLVE(GetBackendCapabilities)
    RESOLVE(Debug_IsControlFocused)
    RESOLVE(Debug_IsControlHovered)
    RESOLVE(Debug_SetMousePosition)
    RESOLVE(Debug_ClearMousePosition)
    RESOLVE(Debug_GetAliveCount)
#undef RESOLVE
    return true;
}

// ── 按钮回调（JSON events.onClick → RegisterAction）：读取本窗口 EditBox
//    内容 → 显示到对方窗口 Label（跨实例内容传递；人工模式可真实输入观察）──
struct SendCtx {
    UIInstance self;      // 本窗口实例
    UIInstance other;     // 对方窗口实例
    void* edit;           // 本窗口 EditBox
    void* otherMsg;       // 对方窗口消息 Label
    void* selfStatus;     // 本窗口状态 Label
    int sendCount;        // 发送次数（auto 模式断言用）
};
static void onSend(UIControlHandle ctl, void* userData) {
    (void)ctl;
    SendCtx* ctx = (SendCtx*)userData;
    char buf[128] = {0};
    if (!uiGetString(ctx->self, ctx->edit, "text", buf, sizeof(buf), &g_gsLen)) {
        printf("[send] FAIL: cannot read edit text\n");
        return;
    }
    char msg[160];
    snprintf(msg, sizeof(msg), "Message: %s", buf);
    assert(uiSetString(ctx->other, ctx->otherMsg, "caption", msg) == 1);
    assert(uiSetString(ctx->self, ctx->selfStatus, "caption", "Status: sent") == 1);
    ctx->sendCount++;
    printf("[send] -> other window shows: \"%s\"\n", buf);
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

static void runFrame(UIInstance inst) {
    uiProcessEvents(inst);
    uiUpdate(inst, 0.016);
}

static void printUsage(const char* argv0) {
    printf("用法: %s [auto=<秒>]\n", argv0);
    printf("  auto=0（缺省）= 人工模式：窗口驻留，真实鼠标输入 EditBox → 点击按钮 → 观察对方窗口显示内容，关闭窗口退出（无断言）\n");
    printf("  auto=N = 自动断言模式：注入坐标驱动视觉状态并断言，N 秒后退出（无人值守回归）\n");
}

// ── 布局 JSON（JSON 创建控件：验证 LayoutParser 完整链路 + 事件绑定） ──
// Window A / B：Label(标题) + EditBox(20,60,380,32) + Button(20,110,140,36)
// + Label(消息接收区) + Label(状态区)；按钮中心 (90,128)、EditBox 中心 (210,76)
static const char* kLayoutJsonA = R"json({
    "controls": [
        {"type": "label",   "id": "lblTitle",  "caption": "Window A (independent instance)", "rect": {"x": 20, "y": 12,  "w": 420, "h": 30}},
        {"type": "edit-box", "id": "edit",      "text": "hello from A",                      "rect": {"x": 20, "y": 60,  "w": 380, "h": 32}},
        {"type": "button",  "id": "btnSend",   "caption": "Send to B",                      "rect": {"x": 20, "y": 110, "w": 140, "h": 36}, "events": {"onClick": "send"}},
        {"type": "label",   "id": "lblMsg",    "caption": "Message: (none)",                "rect": {"x": 20, "y": 160, "w": 420, "h": 60}},
        {"type": "label",   "id": "lblStatus", "caption": "Status: idle",                   "rect": {"x": 20, "y": 230, "w": 380, "h": 28}}
    ]
})json";
static const char* kLayoutJsonB = R"json({
    "controls": [
        {"type": "label",   "id": "lblTitle",  "caption": "Window B (independent instance)", "rect": {"x": 20, "y": 12,  "w": 420, "h": 30}},
        {"type": "edit-box", "id": "edit",      "text": "hello from B",                      "rect": {"x": 20, "y": 60,  "w": 380, "h": 32}},
        {"type": "button",  "id": "btnSend",   "caption": "Send to A",                      "rect": {"x": 20, "y": 110, "w": 140, "h": 36}, "events": {"onClick": "send"}},
        {"type": "label",   "id": "lblMsg",    "caption": "Message: (none)",                "rect": {"x": 20, "y": 160, "w": 420, "h": 60}},
        {"type": "label",   "id": "lblStatus", "caption": "Status: idle",                   "rect": {"x": 20, "y": 230, "w": 380, "h": 28}}
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

    // ── 双窗口实例（默认 1024×768），布局参照 sample_cpp_multiinstance ──
    UIInstance winA = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    UIInstance winB = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    assert(winA && winB && winA != winB);

    // 后端能力：MULTI_WINDOW 才做双窗口渲染；单窗口架构后端（raylib）
    // 非首个实例为 headless（无真实窗口），人工模式仅见主实例窗口
    uint32_t caps = uiGetBackendCapabilities(winA);
    bool multiWindow = (caps & UICORN_BACKEND_CAP_MULTI_WINDOW) != 0;
    printf("后端能力: %s\n", multiWindow ? "MULTI_WINDOW（双窗口渲染）"
                                        : "单窗口（多实例仅首个实例有窗口，渲染冒烟跳过）");

    // 跨窗口通信上下文：A 按钮 → 显示到 B 的消息 Label；B 按钮 → 显示到 A
    SendCtx ctxA = { winA, winB, NULL, NULL, NULL, 0 };
    SendCtx ctxB = { winB, winA, NULL, NULL, NULL, 0 };
    uiRegisterAction(winA, "send", onSend, &ctxA);
    uiRegisterAction(winB, "send", onSend, &ctxB);

    // JSON 布局创建控件（先注册 action，LoadLayout 时按 events.onClick 绑定）
    assert(uiLoadLayout(winA, kLayoutJsonA) == 1);
    assert(uiLoadLayout(winB, kLayoutJsonB) == 1);
    void* editA = uiFindControl(winA, "edit");
    void* btnA  = uiFindControl(winA, "btnSend");
    void* msgA  = uiFindControl(winA, "lblMsg");
    void* editB = uiFindControl(winB, "edit");
    void* btnB  = uiFindControl(winB, "btnSend");
    void* msgB  = uiFindControl(winB, "lblMsg");
    ctxA.edit = editA; ctxA.otherMsg = msgB; ctxA.selfStatus = uiFindControl(winA, "lblStatus");
    ctxB.edit = editB; ctxB.otherMsg = msgA; ctxB.selfStatus = uiFindControl(winB, "lblStatus");
    assert(editA && btnA && msgA && editB && btnB && msgB);
    runFrame(winA);
    runFrame(winB);

    // ── auto 模式：自动断言视觉状态 ──
    if (autoSec > 0) {
        // 视觉 1：hover 跨窗口隔离
        // 鼠标"在 A 窗口按钮上"（注入坐标，替代真实鼠标）→ A 按钮 hover、B 按钮不 hover
        assert(uiDebug_SetMousePosition(winA, 90.0f, 128.0f) == 1);
        assert(uiDebug_SetMousePosition(winB, 5000.0f, 5000.0f) == 1);  // B 窗口外
        runFrame(winA);
        runFrame(winB);
        assert(uiDebug_IsControlHovered(winA, btnA) == 1);
        assert(uiDebug_IsControlHovered(winB, btnB) == 0);   // 鼠标不在 B 窗口 → B 无 hover
        assert(uiDebug_IsControlHovered(winA, editA) == 0);  // 不在 EditBox 上

        // 反向：鼠标在 B 按钮上 → B hover、A 清除
        assert(uiDebug_SetMousePosition(winA, 5000.0f, 5000.0f) == 1);
        assert(uiDebug_SetMousePosition(winB, 90.0f, 128.0f) == 1);
        runFrame(winA);
        runFrame(winB);
        assert(uiDebug_IsControlHovered(winB, btnB) == 1);
        assert(uiDebug_IsControlHovered(winA, btnA) == 0);

        // 窗口内但不在控件上 → 清除 hover
        assert(uiDebug_SetMousePosition(winA, 50.0f, 50.0f) == 1);
        runFrame(winA);
        assert(uiDebug_IsControlHovered(winA, btnA) == 0);

        assert(uiDebug_ClearMousePosition(winA) == 1);
        assert(uiDebug_ClearMousePosition(winB) == 1);
        printf("PASS: hover cross-window isolation\n");

        // 视觉 2：点击聚焦 + 焦点环并存 → FocusLost 清除
        injectClick(winA, 210.0f, 76.0f);   // 点击 A 的 EditBox
        injectClick(winB, 210.0f, 76.0f);   // 点击 B 的 EditBox
        // FocusManager 相互独立：两个窗口同时各有一个焦点环（原 bug 场景）
        assert(uiDebug_IsControlFocused(winA, editA) == 1);
        assert(uiDebug_IsControlFocused(winB, editB) == 1);

        // A 窗口失去系统焦点 → A 焦点环消失，B 保留
        injectFocusLost(winA);
        assert(uiDebug_IsControlFocused(winA, editA) == 0);
        assert(uiDebug_IsControlFocused(winB, editB) == 1);
        printf("PASS: dual focus rings then FocusLost clears window A\n");

        // 视觉 3：按钮点击 → 跨实例内容传递（对方窗口显示本窗口 EditBox 内容）
        injectClick(winA, 90.0f, 128.0f);   // 点击 A 的按钮
        assert(ctxA.sendCount == 1);        // 回调已触发（JSON events.onClick 绑定生效）
        char got[160] = {0};
        assert(uiGetString(winB, msgB, "caption", got, sizeof(got), &g_gsLen) == 1, &g_gsLen);
        assert(strcmp(got, "Message: hello from A") == 0);   // B 窗口显示 A 发送的内容
        printf("PASS: button click -> content shown in other window (\"%s\")\n", got);

        // 反向：B 按钮 → A 窗口显示 B 的内容
        injectClick(winB, 90.0f, 128.0f);
        assert(ctxB.sendCount == 1);
        assert(uiGetString(winA, msgA, "caption", got, sizeof(got), &g_gsLen) == 1, &g_gsLen);
        assert(strcmp(got, "Message: hello from B") == 0);
        printf("PASS: reverse direction content shown in other window (\"%s\")\n", got);

        // 渲染冒烟：双窗口渲染不崩溃。
        // 仅在后端声明 MULTI_WINDOW 能力时执行——单窗口架构后端（raylib：
        // 全局 CORE 只跟踪最近 InitWindow 的窗口，多实例渲染全画到同一窗口）
        // 下"双窗口独立渲染"无视觉意义，按能力位跳过（逻辑断言均已执行）。
        if (uiGetBackendCapabilities(winA) & UICORN_BACKEND_CAP_MULTI_WINDOW) {
            uiRender(winA);
            uiRender(winB);
            printf("PASS: dual-window render smoke\n");
        } else {
            printf("SKIP: dual-window render smoke (backend without MULTI_WINDOW capability)\n");
        }
    }

    // ── 帧循环：auto 模式超时退出；人工模式等窗口关闭 ──
    // 仅当后端声明 MULTI_WINDOW 能力时对第二个实例做渲染/交换——单窗口
    // 架构后端（raylib）的第二实例无真实窗口（headless），渲染会串扰到
    // 主实例窗口（内容闪动）。
    uint64_t t0 = GetTickCount64();
    while (!uiIsQuitRequested(winA) && !uiIsQuitRequested(winB)) {
        if (autoSec && GetTickCount64() - t0 >= (uint64_t)autoSec * 1000) break;
        uiProcessEvents(winA);
        uiProcessEvents(winB);
        uiUpdate(winA, 1.0 / 60.0);
        uiUpdate(winB, 1.0 / 60.0);
        uiClear(winA);
        uiRender(winA);
        uiPresent(winA);
        if (multiWindow) {
            uiClear(winB);
            uiRender(winB);
            uiPresent(winB);
        }
    }
    printf("窗口已关闭（%s）\n", autoSec ? "自动超时" : "人工");

    // ── 逆序销毁 + 泄漏检查（auto 模式断言） ──
    uiDestroyInstance(winB);
    uiDestroyInstance(winA);
    if (autoSec > 0) {
        assert(uiDebug_GetAliveCount() == 0);
        printf("PASS: reverse-order destroy, alive=%d\n", uiDebug_GetAliveCount());
    }

    FreeLibrary(g_uiDll);
    if (autoSec > 0) {
        printf("ALL PASS: multi-instance visual states (CABI dynamic DLL)\n");
        return 0;
    }
    printf("MANUAL MODE EXIT\n");
    return 0;
}
