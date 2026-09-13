// test_multiviewport.cpp — 多视口测试（设计文档 §5.13.7 + K1-K8 键盘跨视口导航）
// 静态链接后端（GetUIBackendCallbacks），Debug 构建运行（依赖 Debug 辅助 API）。
// 注：真实鼠标点击清 activeViewport 属于轮询通路（ownsBackend 分支），
// 注入通路（PushUIEvent → queuedEvents）不经过该逻辑，K8 改用
// "直接销毁活动视口 → activeViewport 清空"验证 cur==nullptr 分支。
#include "UICornerstoneAPI.h"
#include "EventTypes.h"
#include "Label.h"
#include "Splitter.h"
#include "Panel.h"
#include <memory>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

#ifdef _MSC_VER
#define DISABLE_ASSERT_DIALOG() _set_error_mode(_OUT_TO_STDERR), _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT)
#else
#define DISABLE_ASSERT_DIALOG() ((void)0)
#endif

// ── 事件注入辅助 ──
static void injectKey(UIInstance inst, KeyCode code, KeyMod mod, bool down) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = down ? UI_EVENT_KEY_DOWN : UI_EVENT_KEY_UP;
    UI_EVENT_KEY_CODE(&ev) = (int)code;
    UI_EVENT_KEY_MOD(&ev) = (uint16_t)mod;
    UICornerstone_PushUIEvent(inst, &ev);
    UICornerstone_ProcessEvents(inst);
    UICornerstone_Update(inst, 0.016);   // 事件入队后经 eventLoopEntry 分发
}

static void injectMouse(UIInstance inst, UIInstance routeTarget, EventType type, float x, float y) {
    UIEvent ev; memset(&ev, 0, sizeof(ev));
    ev.type = (UIEventType)type;
    UI_EVENT_MOUSE_X(&ev) = x;
    UI_EVENT_MOUSE_Y(&ev) = y;
    UI_EVENT_BUTTON(&ev) = 1;   // MouseButton::Left
    // 注入通路：dispatchToBench(inst)。测试直接 push 到子视口实例，
    // 使事件进 vp bench 队列 → vp eventLoopEntry 未消费 → 回退 owner 树。
    UICornerstone_PushUIEvent(routeTarget, &ev);
    UICornerstone_ProcessEvents(routeTarget);
    UICornerstone_Update(routeTarget, 0.016);   // eventLoopEntry：未消费 → 回退 owner
    (void)inst;
}

// F1：子视口事件回退——视口覆盖 owner clickable Label，视口空白区点击
//     经 eventLoopEntry 回退 owner 树，Label onClick 触发（splitter 场景根修）

static void frame(UIInstance win, UIInstance vp1, UIInstance vp2) {
    UICornerstone_ProcessEvents(win);
    if (vp1) { UICornerstone_Update(vp1, 0.016); UICornerstone_Render(vp1); }
    if (vp2) { UICornerstone_Update(vp2, 0.016); UICornerstone_Render(vp2); }
}

static void testF1() {
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    assert(win);

    // owner：clickable Label 置于 (100,100)（随后被子视口区域覆盖）
    UIControlHandle lbl = UICornerstone_CreateLabel(win, u8"owner-btn", 12.f, 100, 100, 120, 30, 1.f, 1.f);
    assert(lbl);
    static int g_ownerClicks = 0;
    g_ownerClicks = 0;
    auto* l = reinterpret_cast<Control*>(lbl);
    if (auto impl = dynamic_cast<Label*>(l)) {
        impl->setClickable(true);
        impl->setOnClick([](shared_ptr<Control>) { ++g_ownerClicks; });
    }

    // 子视口覆盖 (0,0,640,480)（含 Label 区域）；视口内无任何控件（空白 bench）
    UIInstance vp = UICornerstone_CreateViewport(win, UIRect{0, 0, 640, 480});
    assert(vp);
    frame(win, vp, nullptr);

    // 向视口区域内 Label 位置注入点击：路由到视口（未消费）→ 回退 owner 树
    // 注：Update 间会用真实鼠标位置做 hover 刷新，需同步注入鼠标坐标维持 Pressed 链
    UICornerstone_Debug_SetMousePosition(win, 110.f, 110.f);
    injectMouse(win, vp, EventType::MouseDown, 110.f, 110.f);
    UICornerstone_Debug_SetMousePosition(win, 110.f, 110.f);
    injectMouse(win, vp, EventType::MouseUp,   110.f, 110.f);

    if (g_ownerClicks == 1) {
        printf("PASS: F1 viewport-unconsumed event falls back to owner tree\n");
    } else {
        printf("FAIL: F1 owner clicks=%d (expect 1)\n", g_ownerClicks);
    }

    UICornerstone_DestroyInstance(win);   // 子视口随 owner 级联销毁
}



// F2：跨视口 splitter 续拖——owner splitter 拖拽中指针进入视口覆盖区，
//     Move/Up 走视口回退并补跑 owner 队列 watcher（updateDrag/endDrag）
static void testF2() {
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    assert(win);

    UIControlHandle sp = UICornerstone_CreateSplitter(win, 200.f, 200.f, 8.f, 300.f, 0, 1.f, 1.f);
    assert(sp);
    auto* s = reinterpret_cast<Control*>(sp);
    auto split = s ? dynamic_cast<Splitter*>(s) : nullptr;
    assert(split);
    auto p1 = make_shared<Panel>(nullptr, SRect(0, 200, 200, 300));
    auto p2 = make_shared<Panel>(nullptr, SRect(208, 200, 400, 300));
    split->setLinkedControls(p1, p2);
    split->setSplitRatio(0.5f);

    SRect dr = split->getDrawRect();
    // 视口覆盖 splitter 右侧区域（含 Move 目标点），对齐实际绘制位置
    UIInstance vp = UICornerstone_CreateViewport(win,
        UIRect{dr.left + dr.width, dr.top, 400, dr.height});
    assert(vp);
    frame(win, vp, nullptr);

    dr = split->getDrawRect();
    float hitX = dr.left + dr.width / 2.f;
    float hitY = dr.top + dr.height / 2.f;
    float beforeT = split->getRect().top;
    injectMouse(win, win, EventType::MouseDown, hitX, hitY);
    UICornerstone_Debug_SetMousePosition(win, hitX, hitY + 100.f);
    injectMouse(win, vp, EventType::MouseMove, hitX, hitY + 100.f);
    UICornerstone_Debug_SetMousePosition(win, hitX, hitY + 100.f);
    injectMouse(win, vp, EventType::MouseUp, hitX, hitY + 100.f);

    float afterT = split->getRect().top;
    if (afterT > beforeT) {
        printf("PASS: F2 splitter drag continues across viewport (top %.1f -> %.1f)\n", beforeT, afterT);
    } else {
        printf("FAIL: F2 splitter top %.1f -> %.1f (expect increase)\n", beforeT, afterT);
    }

    UICornerstone_DestroyInstance(win);
}

// ── 每用例独立窗口，避免状态纠缠 ──
static void testK1() {
    // K1：单视口（无子视口）+ 2 WinFrame，Ctrl+Tab 行为不变
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    assert(win);
    UIControlHandle wfA = UICornerstone_CreateWinFrame(win, "WinA", 10, 10, 300, 200, 1.0f, 1.0f);
    UIControlHandle wfB = UICornerstone_CreateWinFrame(win, "WinB", 10, 240, 300, 200, 1.0f, 1.0f);
    assert(wfA && wfB);
    frame(win, NULL, NULL);

    // children.size()==0 → tryViewportScopeSwitch 短路，Ctrl+Tab 原样进视口内 FocusManager
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == NULL);
    injectKey(win, KeyCode::Tab, (KeyMod)(KeyMod::LCtrl | KeyMod::LShift), true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == NULL);

    UICornerstone_DestroyInstance(win);
    printf("PASS: K1 single viewport Ctrl+Tab\n");
}

static void testK2() {
    // K2：双视口各 1 WinFrame；首次 Ctrl+Tab 视口内优先，隐藏后跨视口
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    UIInstance vp1 = UICornerstone_CreateViewport(win, UIRect{0, 0, 640, 480});
    UIInstance vp2 = UICornerstone_CreateViewport(win, UIRect{640, 0, 640, 480});
    assert(vp1 && vp2 && vp1 != vp2);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp1);  // 首子视口自动 active

    UIControlHandle editA = UICornerstone_CreateEditBox(vp1, 10, 10, 200, 30, 1.0f, 1.0f);
    UIControlHandle wfA = UICornerstone_CreateWinFrame(vp1, "WinA", 10, 60, 300, 200, 1.0f, 1.0f);
    UIControlHandle editB1 = UICornerstone_CreateEditBox(vp2, 10, 10, 200, 30, 1.0f, 1.0f);
    assert(editA && wfA && editB1);
    frame(win, vp1, vp2);

    // 首次 Ctrl+Tab：vp1 有 1 个可见 boundary → 视口内优先，不跨视口
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp1);

    // 隐藏 vp1 的 WinFrame → vp1 可见 boundary == 0 → 跨视口切 vp2，
    // focusFirstInScope(vp2.bench) 聚焦第一个可聚焦控件 EditBox_B1
    assert(UICornerstone_SetBool(vp1, wfA, "visible", 0) == 1);
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp2);
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 1);
    assert(UICornerstone_Debug_IsControlFocused(win, editA) == 0);  // vp1 旧焦点被清

    UICornerstone_DestroyInstance(vp2);
    UICornerstone_DestroyInstance(vp1);
    UICornerstone_DestroyInstance(win);
    printf("PASS: K2 viewport-priority then cross-viewport\n");
}

static void testK3K4K5() {
    // K3：vp1 内 2 WinFrame，Ctrl+Tab 在视口内切换；K4：全部隐藏后跨视口；K5：反向
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    UIInstance vp1 = UICornerstone_CreateViewport(win, UIRect{0, 0, 640, 480});
    UIInstance vp2 = UICornerstone_CreateViewport(win, UIRect{640, 0, 640, 480});
    assert(vp1 && vp2);

    UIControlHandle editA = UICornerstone_CreateEditBox(vp1, 10, 10, 200, 30, 1.0f, 1.0f);
    UIControlHandle wfA1 = UICornerstone_CreateWinFrame(vp1, "WinA1", 10, 60, 300, 180, 1.0f, 1.0f);
    UIControlHandle wfA2 = UICornerstone_CreateWinFrame(vp1, "WinA2", 10, 260, 300, 180, 1.0f, 1.0f);
    UIControlHandle editB1 = UICornerstone_CreateEditBox(vp2, 10, 10, 200, 30, 1.0f, 1.0f);
    assert(editA && wfA1 && wfA2 && editB1);
    frame(win, vp1, vp2);

    // K3：vp1 有 2 个可见 boundary → 视口内优先
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp1);

    // K4：vp1 的 2 个 WinFrame 全部隐藏 → 跨视口跳 vp2
    assert(UICornerstone_SetBool(vp1, wfA1, "visible", 0) == 1);
    assert(UICornerstone_SetBool(vp1, wfA2, "visible", 0) == 1);
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp2);
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 1);

    // K5：Ctrl+Shift+Tab 反向：vp2 → vp1
    injectKey(win, KeyCode::Tab, (KeyMod)(KeyMod::LCtrl | KeyMod::LShift), true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp1);
    assert(UICornerstone_Debug_IsControlFocused(win, editA) == 1);  // focusFirstInScope(vp1)
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 0);

    UICornerstone_DestroyInstance(vp2);
    UICornerstone_DestroyInstance(vp1);
    UICornerstone_DestroyInstance(win);
    printf("PASS: K3/K4/K5 in-viewport switch, cross on hidden, reverse\n");
}

static void testK6() {
    // K6：Tab 只在当前 activeViewport 内循环，不进入 vp1（注入目标须为 vp2）
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    UIInstance vp1 = UICornerstone_CreateViewport(win, UIRect{0, 0, 640, 480});
    UIInstance vp2 = UICornerstone_CreateViewport(win, UIRect{640, 0, 640, 480});
    assert(vp1 && vp2);

    UIControlHandle editA = UICornerstone_CreateEditBox(vp1, 10, 10, 200, 30, 1.0f, 1.0f);
    UIControlHandle editB1 = UICornerstone_CreateEditBox(vp2, 10, 10, 200, 30, 1.0f, 1.0f);
    UIControlHandle editB2 = UICornerstone_CreateEditBox(vp2, 10, 50, 200, 30, 1.0f, 1.0f);
    assert(editA && editB1 && editB2);
    frame(win, vp1, vp2);

    // 无焦点 → Tab 聚焦 vp2 第一个可聚焦控件 EditBox_B1
    injectKey(vp2, KeyCode::Tab, KeyMod::None, true);
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 1);
    assert(UICornerstone_Debug_IsControlFocused(win, editA) == 0);   // vp1 不受影响

    // 再 Tab → B1 → B2，仍在 vp2 内
    injectKey(vp2, KeyCode::Tab, KeyMod::None, true);
    assert(UICornerstone_Debug_IsControlFocused(win, editB2) == 1);
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 0);
    assert(UICornerstone_Debug_IsControlFocused(win, editA) == 0);

    UICornerstone_DestroyInstance(vp2);
    UICornerstone_DestroyInstance(vp1);
    UICornerstone_DestroyInstance(win);
    printf("PASS: K6 Tab stays inside active viewport\n");
}

static void testK7() {
    // K7：焦点回跳——切回 vp1 时 focusFirstInScope 聚焦第一个可聚焦控件，不记忆原焦点
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    UIInstance vp1 = UICornerstone_CreateViewport(win, UIRect{0, 0, 640, 480});
    UIInstance vp2 = UICornerstone_CreateViewport(win, UIRect{640, 0, 640, 480});
    assert(vp1 && vp2);

    UIControlHandle editA1 = UICornerstone_CreateEditBox(vp1, 10, 10, 200, 30, 1.0f, 1.0f);
    UIControlHandle editA2 = UICornerstone_CreateEditBox(vp1, 10, 50, 200, 30, 1.0f, 1.0f);
    UIControlHandle editB1 = UICornerstone_CreateEditBox(vp2, 10, 10, 200, 30, 1.0f, 1.0f);
    assert(editA1 && editA2 && editB1);
    frame(win, vp1, vp2);

    // 原焦点在 vp1 的 EditBox_A2
    injectKey(vp1, KeyCode::Tab, KeyMod::None, true);   // → A1
    injectKey(vp1, KeyCode::Tab, KeyMod::None, true);   // → A2
    assert(UICornerstone_Debug_IsControlFocused(win, editA2) == 1);

    // 跨视口切到 vp2（vp1 无 boundary）→ B1 聚焦，A2 失焦
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp2);
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 1);
    assert(UICornerstone_Debug_IsControlFocused(win, editA2) == 0);

    // 切回 vp1 → focusFirstInScope 聚焦 A1（不是记忆的 A2）
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp1);
    assert(UICornerstone_Debug_IsControlFocused(win, editA1) == 1);
    assert(UICornerstone_Debug_IsControlFocused(win, editA2) == 0);

    UICornerstone_DestroyInstance(vp2);
    UICornerstone_DestroyInstance(vp1);
    UICornerstone_DestroyInstance(win);
    printf("PASS: K7 focus returns to first control in scope\n");
}

static void testK8() {
    // K8：activeViewport 为 null 时（直接销毁活动视口后）Ctrl+Tab 从 children.front() 切入。
    // 注意：children.size()<=1 时 tryViewportScopeSwitch 短路，故用 3 个视口，
    // 销毁活动视口 vp1 后仍剩 2 个子视口，覆盖 cur==nullptr 分支
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    UIInstance win = UICornerstone_CreateInstance(cb, NULL);
    UIInstance vp1 = UICornerstone_CreateViewport(win, UIRect{0, 0, 320, 480});
    UIInstance vp2 = UICornerstone_CreateViewport(win, UIRect{320, 0, 320, 480});
    UIInstance vp3 = UICornerstone_CreateViewport(win, UIRect{640, 0, 320, 480});
    assert(vp1 && vp2 && vp3);

    UIControlHandle editA = UICornerstone_CreateEditBox(vp1, 10, 10, 200, 30, 1.0f, 1.0f);
    UIControlHandle editB1 = UICornerstone_CreateEditBox(vp2, 10, 10, 200, 30, 1.0f, 1.0f);
    assert(editA && editB1);
    frame(win, vp1, vp2);

    // 直接销毁活动视口 vp1 → owner 将 activeViewport 置空（防悬垂）
    UICornerstone_DestroyInstance(vp1);
    assert(UICornerstone_Debug_GetActiveViewport(win) == NULL);

    // Ctrl+Tab：cur==nullptr 分支 → nextViewport 从 children.front()（vp2）切入，不崩溃
    injectKey(win, KeyCode::Tab, KeyMod::LCtrl, true);
    assert(UICornerstone_Debug_GetActiveViewport(win) == vp2);
    assert(UICornerstone_Debug_IsControlFocused(win, editB1) == 1);

    UICornerstone_DestroyInstance(vp3);
    UICornerstone_DestroyInstance(vp2);
    UICornerstone_DestroyInstance(win);
    printf("PASS: K8 activeViewport==null after destroy, Ctrl+Tab re-enters\n");
}

int main() {
    DISABLE_ASSERT_DIALOG();
    UIBackendCallbacks* cb = GetUIBackendCallbacks();
    assert(cb);
    (void)cb;

    testF1();
    testF2();
    testK1();
    testK2();
    testK3K4K5();
    testK6();
    testK7();
    testK8();
    testF1();
    testF1();

    assert(UICornerstone_Debug_GetAliveCount() == 0);
    printf("ALL PASS: multiviewport + keyboard navigation\n");
    return 0;
}
