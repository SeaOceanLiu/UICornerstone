// temp_cr4_probe.cpp - CR-004 通用单元格控件 + CR-005 命中/光标
#include "UICornerstoneAPI.h"
#include "Cursor.h"
#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

static UIInstance g = nullptr;
static int g_btn = 0, g_sel = 0, g_begin = 0;
static void onBtn(UIControlHandle, const UIEventData*, void*) { g_btn++; }
static void onSel(UIControlHandle, const UIEventData*, void*) { g_sel++; }
static void onBegin(UIControlHandle, const UIEventData*, void*) { g_begin++; }

static void pump(int n) {
    for (int i = 0; i < n; ++i) {
        UICornerstone_ProcessEvents(g);
        UICornerstone_Update(g, 1.0 / 60.0);
        UICornerstone_Render(g);
        UICornerstone_Present(g);
    }
}
static void mouse(UIEventType t, float x, float y) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = t; UI_EVENT_MOUSE_X(&e) = x; UI_EVENT_MOUSE_Y(&e) = y; UI_EVENT_BUTTON(&e) = 1;
    UICornerstone_PushUIEvent(g, &e);
}
static void click(float x, float y) { mouse(UI_EVENT_MOUSE_DOWN, x, y); mouse(UI_EVENT_MOUSE_UP, x, y); }

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    g = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!g) { printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(g, 0, 0, 900, 700);
    bool ok = true;

    void* lv = UICornerstone_CreateListView(g, 10, 10, 400, 160, 1, 1);
    UICornerstone_ListViewAddColumn(g, lv, "A", 120, 1);
    UICornerstone_ListViewAddColumn(g, lv, "B", 120, 0);
    const char* r0[] = {"a0", "b0"};
    const char* r1[] = {"a1", "b1"};
    UICornerstone_ListViewAddRow(g, lv, "r0", 2, r0);
    UICornerstone_ListViewAddRow(g, lv, "r1", 2, r1);
    UICornerstone_SetBool(g, lv, "cell-editing", 1);
    UICornerstone_SetCallback(g, lv, "selection-changed", onSel, nullptr);
    UICornerstone_SetCallback(g, lv, "item-click", onSel, nullptr);
    UICornerstone_SetCallback(g, lv, "cell-edit-begin", onBegin, nullptr);
    pump(2);

    // Panel + Button → FILL cell(0,0)
    void* panel = UICornerstone_CreatePanel(g, 0, 0, 100, 24, 1, 1);
    void* btn = UICornerstone_CreateButton(g, "B", 2, 2, 60, 20, 1, 1);
    UICornerstone_SetCallback(g, btn, "click", onBtn, nullptr);
    UICornerstone_AddChildControl(g, panel, btn);
    void* root = UICornerstone_GetRoot(g);
    bool rm = UICornerstone_RemoveChild(g, root, panel) != 0;
    int setc = UICornerstone_ListViewSetCellControl(g, lv, 0, 0, panel, UI_LIST_CELL_LAYOUT_FILL);
    void* got = UICornerstone_ListViewGetCellControl(g, lv, 0, 0);
    pump(2);
    bool ok1 = rm && setc == 1 && got == panel;
    printf("1) set FILL cell control: rm=%d set=%d got=%p %s\n", (int)rm, setc, got, ok1 ? "OK" : "FAIL");
    ok &= ok1;

    // 点击按钮：按钮计数，不选行
    int b0 = g_btn, s0 = g_sel;
    click(10 + 2 + 2 + 10, 10 + 28 + 1 + 2 + 10);   // cell(0,0)+panel+button 内
    pump(2);
    bool ok2 = g_btn == b0 + 1 && g_sel == s0;
    printf("2) button click consumes: btn=%d selDelta=%d %s\n", g_btn - b0, g_sel - s0, ok2 ? "OK" : "FAIL");
    ok &= ok2;

    // 点击 FILL 空白（按钮右侧）：回落行选择
    click(10 + 2 + 90, 10 + 28 + 1 + 12);
    pump(2);
    bool ok3 = g_sel == s0 + 1;
    printf("3) FILL blank falls through selection: selDelta=%d %s\n", g_sel - s0, ok3 ? "OK" : "FAIL");
    ok &= ok3;

    // FILL 双击空白：编辑屏蔽
    int eb = g_begin;
    click(10 + 2 + 90, 10 + 28 + 1 + 12);
    pump(1);
    click(10 + 2 + 90, 10 + 28 + 1 + 12);
    pump(2);
    bool ok4 = g_begin == eb && UICornerstone_ListViewIsEditing(g, lv) == 0;
    printf("4) FILL double-click shield: beginDelta=%d editing=%d %s\n",
           g_begin - eb, UICornerstone_ListViewIsEditing(g, lv), ok4 ? "OK" : "FAIL");
    ok &= ok4;

    // captured：按钮按下 → 移出 → 抬起：不点击、不选行
    int b1 = g_btn, s1 = g_sel;
    mouse(UI_EVENT_MOUSE_DOWN, 10 + 2 + 2 + 10, 10 + 28 + 1 + 2 + 10);
    mouse(UI_EVENT_MOUSE_MOVE, 500, 500);
    mouse(UI_EVENT_MOUSE_UP, 500, 500);
    pump(2);
    bool ok5 = g_btn == b1 && g_sel == s1;
    printf("5) captured drag-out release: btnDelta=%d selDelta=%d %s\n",
           g_btn - b1, g_sel - s1, ok5 ? "OK" : "FAIL");
    ok &= ok5;

    // LEADING 兼容：单元格控件 Button（旧 API）
    void* lb = UICornerstone_CreateButton(g, "L", 0, 0, 16, 16, 1, 1);
    UICornerstone_SetCallback(g, lb, "click", onBtn, nullptr);
    UICornerstone_RemoveChild(g, root, lb);
    int setl = UICornerstone_ListViewSetCellLeadingControl(g, lv, 1, 0, lb);
    pump(2);
    int b2 = g_btn;
    click(10 + 4 + 8, 10 + 28 + 24 + 4 + 8);
    pump(2);
    bool ok6 = setl == 1 && g_btn == b2 + 1;
    printf("6) LEADING compat click: set=%d btnDelta=%d %s\n", setl, g_btn - b2, ok6 ? "OK" : "FAIL");
    ok &= ok6;

    // 行删除后 Get 返回 NULL
    UICornerstone_ListViewRemoveRow(g, lv, 0);
    pump(1);
    void* gone = UICornerstone_ListViewGetCellControl(g, lv, 0, 0);
    void* outOfRange = UICornerstone_ListViewGetCellControl(g, lv, 99, 0);
    bool ok7 = gone == lb && outOfRange == nullptr;   // 原 row1 LEADING 上移；FILL 随 row0 移除
    printf("7) row delete: got=%p panel=%p lb=%p out=%p %s\n",
           gone, panel, lb, outOfRange, ok7 ? "OK" : "FAIL");
    ok &= ok7;

    // CR-005：8px 命中 / 最后一列可见边界 / 光标显式 API
    void* lv2 = UICornerstone_CreateListView(g, 430, 10, 240, 160, 1, 1);
    UICornerstone_ListViewAddColumn(g, lv2, "A", 100, 0);
    UICornerstone_ListViewAddColumn(g, lv2, "B", 100, 0);
    UICornerstone_ListViewAddColumn(g, lv2, "C", 100, 0);
    const char* rr[] = {"x", "y", "z"};
    UICornerstone_ListViewAddRow(g, lv2, "q", 3, rr);
    pump(2);
    int hit5 = UICornerstone_ListViewHitTestColumnDivider(g, lv2, 100 + 5, 10);
    int hitLast = UICornerstone_ListViewHitTestColumnDivider(g, lv2, 239 - 4, 10);   // 最后一列可见右边界附近
    bool curOff = UICornerstone_ListViewSetColumnResizeCursor(g, lv2, 0) == 1;
    UICornerstone_SetBool(g, lv2, "visible", 0);
    bool offHidden = UICornerstone_ListViewSetColumnResizeCursor(g, lv2, 0) == 1;
    bool onHidden = UICornerstone_ListViewSetColumnResizeCursor(g, lv2, 1) == 0;
    bool hitHidden = UICornerstone_ListViewHitTestColumnDivider(g, lv2, 100, 10) == -1;
    UICornerstone_SetBool(g, lv2, "visible", 1);
    bool ok8 = hit5 == 0 && hitLast == 2 && curOff && offHidden && onHidden && hitHidden;
    printf("8) CR005 hit/cursor: hit5=%d hitLast=%d off=%d offHidden=%d onHidden=%d %s\n",
           hit5, hitLast, (int)curOff, (int)offHidden, (int)onHidden, ok8 ? "OK" : "FAIL");
    ok &= ok8;

    // 9) 全局光标 owner 仲裁：隐藏非 owner 的 ListView 不应重置当前 EW
    bool curA = UICornerstone_ListViewSetColumnResizeCursor(g, lv2, 1) == 1;
    const void* ownerA = Cursor::getCurrentOwner();
    UICornerstone_SetBool(g, lv, "visible", 0);   // 非 owner 隐藏
    const void* ownerAfterHide = Cursor::getCurrentOwner();
    bool ownerKept = (ownerAfterHide == reinterpret_cast<const void*>(lv2));
    UICornerstone_SetBool(g, lv, "visible", 1);
    UICornerstone_SetBool(g, lv2, "visible", 0);  // owner 隐藏 → 恢复默认
    bool ownerCleared = (Cursor::getCurrentOwner() == nullptr);
    UICornerstone_SetBool(g, lv2, "visible", 1);
    bool ok9 = curA && ownerA == reinterpret_cast<const void*>(lv2) && ownerKept && ownerCleared;
    printf("9) cursor owner arbitration: set=%d owner=%p afterHide=%p lv=%p lv2=%p kept=%d cleared=%d %s\n",
           (int)curA, ownerA, ownerAfterHide, lv, lv2, (int)ownerKept, (int)ownerCleared, ok9 ? "OK" : "FAIL");
    ok &= ok9;

    // 10) 非 owner FocusLost 不影响当前 owner
    UICornerstone_ListViewSetColumnResizeCursor(g, lv2, 1);
    UIEvent fe{}; fe.type = UI_EVENT_FOCUS_LOST;
    UICornerstone_PushUIEvent(g, &fe);
    pump(1);
    bool ownerAfterFocus = (Cursor::getCurrentOwner() == reinterpret_cast<const void*>(lv2));
    UICornerstone_ListViewSetColumnResizeCursor(g, lv2, 0);
    bool ok10 = ownerAfterFocus && Cursor::getCurrentOwner() == nullptr;
    printf("10) non-owner FocusLost keeps owner: focus=%d %s\n",
           (int)ownerAfterFocus, ok10 ? "OK" : "FAIL");
    ok &= ok10;

    printf("RESULT: %s\n", ok ? "ALL OK" : "FAILED");
    UICornerstone_DestroyInstance(g);
    printf("auto=0\n");
    return ok ? 0 : 2;
}
