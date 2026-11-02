// temp_cr_ui_probe.cpp - CR-001/002 双击进入编辑 / Tab 失焦提交 / 只读与可见性
#include "UICornerstoneAPI.h"
#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

static UIInstance g = nullptr;
static int evBegin = 0, evEdited = 0, evCancel = 0;
static string lastOld, lastNew;
static int lastRO = -1;

static void onEdit(UIControlHandle, const UIEventData* e, void*) {
    if (!e || !e->eventName) return;
    string n = e->eventName;
    if (n == "cell-edit-begin" || n == "node-edit-begin") evBegin++;
    else if (n == "cell-edited" || n == "node-edited") evEdited++;
    else if (n == "cell-edit-cancel" || n == "node-edit-cancel") evCancel++;
    if (n.find("cell") != string::npos) {
        lastOld = e->data.grid.oldText ? e->data.grid.oldText : "";
        lastNew = e->data.grid.newText ? e->data.grid.newText : "";
        lastRO = e->data.grid.editReadOnly;
    } else {
        lastOld = e->data.treeNode.oldText ? e->data.treeNode.oldText : "";
        lastNew = e->data.treeNode.newText ? e->data.treeNode.newText : "";
        lastRO = e->data.treeNode.editReadOnly;
    }
}
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
    e.type = t;
    UI_EVENT_MOUSE_X(&e) = x; UI_EVENT_MOUSE_Y(&e) = y; UI_EVENT_BUTTON(&e) = 1;
    UICornerstone_PushUIEvent(g, &e);
}
static void click(float x, float y) { mouse(UI_EVENT_MOUSE_DOWN, x, y); mouse(UI_EVENT_MOUSE_UP, x, y); }
static void key(int code, int mod = 0) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = UI_EVENT_KEY_DOWN; UI_EVENT_KEY_CODE(&e) = code; UI_EVENT_KEY_MOD(&e) = (uint16_t)mod;
    UICornerstone_PushUIEvent(g, &e);
    UIEvent u; memset(&u, 0, sizeof(u));
    u.type = UI_EVENT_KEY_UP; UI_EVENT_KEY_CODE(&u) = code;
    UICornerstone_PushUIEvent(g, &u);
}
static void textInput(const char* s) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = UI_EVENT_TEXT_INPUT;
    strncpy((char*)e.data, s, sizeof(e.data) - 1);
    UICornerstone_PushUIEvent(g, &e);
}
static string cellText(void* lv, int r, int c) {
    char b[256] = {0};
    UICornerstone_ListViewGetCellText(g, lv, r, c, b, (int)sizeof(b));
    return b;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    g = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!g) { printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(g, 0, 0, 900, 700);
    bool ok = true;

    // ListView 双击进入编辑
    void* lv = UICornerstone_CreateListView(g, 10, 10, 400, 160, 1, 1);
    UICornerstone_ListViewAddColumn(g, lv, "A", 120, 0);
    const char* r0[] = {"LV0"};
    UICornerstone_ListViewAddRow(g, lv, "r0", 1, r0);
    UICornerstone_SetBool(g, lv, "cell-editing", 1);
    UICornerstone_SetCallback(g, lv, "cell-edit-begin", onEdit, nullptr);
    UICornerstone_SetCallback(g, lv, "cell-edited", onEdit, nullptr);
    UICornerstone_SetCallback(g, lv, "cell-edit-cancel", onEdit, nullptr);
    pump(2);

    // 双击单元格（row0 col0 中心：x=10+60, y=10+28+12）
    click(70, 50);
    pump(1);
    click(70, 50);
    pump(2);
    bool ok1 = UICornerstone_ListViewIsEditing(g, lv) == 1 && evBegin == 1;
    printf("1) double-click enters edit: editing=%d begin=%d %s\n",
           UICornerstone_ListViewIsEditing(g, lv), evBegin, ok1 ? "OK" : "FAIL");
    ok &= ok1;

    // 输入 + Tab（失焦提交）
    key(0x41, 0x40); pump(1);
    textInput("viaTab"); pump(1);
    key(0x09); pump(2);
    bool ok2 = UICornerstone_ListViewIsEditing(g, lv) == 0 && evEdited == 1 &&
               lastOld == "LV0" && lastNew == "viaTab" && cellText(lv, 0, 0) == "viaTab";
    printf("2) Tab commits: editing=%d edited=%d old='%s' new='%s' cell='%s' %s\n",
           UICornerstone_ListViewIsEditing(g, lv), evEdited, lastOld.c_str(), lastNew.c_str(),
           cellText(lv, 0, 0).c_str(), ok2 ? "OK" : "FAIL");
    ok &= ok2;

    // TreeView 双击标签进入编辑
    void* tv = UICornerstone_CreateTreeView(g, 10, 300, 300, 200, 1, 1);
    UICornerstone_TreeViewAddNode(g, tv, "", "n1", "Node1", 1);
    UICornerstone_SetBool(g, tv, "label-editing", 1);
    UICornerstone_SetCallback(g, tv, "node-edit-begin", onEdit, nullptr);
    UICornerstone_SetCallback(g, tv, "node-edited", onEdit, nullptr);
    UICornerstone_SetCallback(g, tv, "node-edit-cancel", onEdit, nullptr);
    pump(2);
    // 标签 rect：local x≈20..（4+16+文本），行中心 y≈11
    click(10 + 25, 300 + 11);
    pump(1);
    click(10 + 25, 300 + 11);
    pump(2);
    bool ok3 = UICornerstone_TreeViewIsEditing(g, tv) == 1 && evBegin == 2;
    printf("3) tree double-click enters edit: editing=%d begin=%d %s\n",
           UICornerstone_TreeViewIsEditing(g, tv), evBegin, ok3 ? "OK" : "FAIL");
    ok &= ok3;

    // Esc 取消（不写回）
    int nc = evCancel;
    key(0x1B); pump(2);
    bool ok4 = UICornerstone_TreeViewIsEditing(g, tv) == 0 && evCancel == nc + 1 &&
               lastOld == "Node1" && lastNew.empty() &&
               UICornerstone_Debug_IsControlFocused(g, tv) == 1;
    printf("4) Esc cancels node edit + focus restore: editing=%d cancel=%d focus=%d %s\n",
           UICornerstone_TreeViewIsEditing(g, tv), evCancel - nc,
           UICornerstone_Debug_IsControlFocused(g, tv), ok4 ? "OK" : "FAIL");
    printf("   focus lv=%d tv=%d\n", UICornerstone_Debug_IsControlFocused(g, lv), UICornerstone_Debug_IsControlFocused(g, tv));
    ok &= ok4;
    // 4b) 程序化 Commit 后焦点
    bool b4b = UICornerstone_TreeViewBeginEditNode(g, tv, "n1") == 1;
    pump(1);
    bool c4b = UICornerstone_TreeViewCommitEdit(g, tv) == 1;
    pump(1);
    printf("   4b commit focus=%d begin=%d commit=%d\n",
           UICornerstone_Debug_IsControlFocused(g, tv), (int)b4b, (int)c4b);

    // 只读列：双击进入但 EditBox 只读；输入+Enter → cancel(RO)
    UICornerstone_SetString(g, lv, "read-only-columns", "0");
    pump(1);
    int b0 = evBegin, c0 = evCancel, e0 = evEdited;
    click(70, 50);
    pump(1);
    click(70, 50);
    pump(2);
    bool entered = UICornerstone_ListViewIsEditing(g, lv) == 1;
    key(0x41, 0x40); pump(1);
    textInput("blocked"); pump(1);
    key(0x0D); pump(2);
    bool ok5 = entered && evBegin == b0 + 1 && evEdited == e0 && evCancel == c0 + 1 &&
               lastRO == 1 && cellText(lv, 0, 0) == "viaTab";
    printf("5) readonly double-click + Enter=cancel: entered=%d ev=%d/%d/%d ro=%d cell='%s' %s\n",
           (int)entered, evBegin - b0, evEdited - e0, evCancel - c0, lastRO,
           cellText(lv, 0, 0).c_str(), ok5 ? "OK" : "FAIL");
    ok &= ok5;

    // 6) CR-003：运行时拖拽与 API session 互斥
    void* lvR = UICornerstone_CreateListView(g, 10, 480, 300, 120, 1, 1);
    UICornerstone_ListViewAddColumn(g, lvR, "A", 120, 0);
    const char* rr[] = {"x"};
    UICornerstone_ListViewAddRow(g, lvR, "rx", 1, rr);
    pump(2);
    mouse(UI_EVENT_MOUSE_DOWN, 10 + 120, 480 + 10);
    mouse(UI_EVENT_MOUSE_MOVE, 10 + 140, 480 + 10);
    mouse(UI_EVENT_MOUSE_UP, 10 + 140, 480 + 10);
    pump(2);
    int hit140 = UICornerstone_ListViewHitTestColumnDivider(g, lvR, 140, 10);
    mouse(UI_EVENT_MOUSE_DOWN, 10 + 140, 480 + 10);
    pump(1);   // 让运行时进入 m_dragCol 后再尝试 Begin
    bool begDuringDrag = UICornerstone_ListViewBeginColumnResize(g, lvR, 0, 140) == 1;
    mouse(UI_EVENT_MOUSE_UP, 10 + 140, 480 + 10);
    pump(1);
    bool apiBeg = UICornerstone_ListViewBeginColumnResize(g, lvR, 0, 140) == 1;
    mouse(UI_EVENT_MOUSE_DOWN, 10 + 140, 480 + 10);
    mouse(UI_EVENT_MOUSE_MOVE, 10 + 180, 480 + 10);
    mouse(UI_EVENT_MOUSE_UP, 10 + 180, 480 + 10);
    pump(1);
    float wApi = UICornerstone_ListViewUpdateColumnResize(g, lvR, 140);
    bool apiCancel = UICornerstone_ListViewCancelColumnResize(g, lvR) == 1;
    pump(1);
    int hitBack = UICornerstone_ListViewHitTestColumnDivider(g, lvR, 140, 10);
    bool ok6 = hit140 == 0 && !begDuringDrag && apiBeg && wApi == 140.0f && apiCancel && hitBack == 0;
    printf("6) resize drag/session mutex: hit140=%d beginDuringDrag=%d api=%d w=%.1f cancel=%d %s\n",
           hit140, (int)begDuringDrag, (int)apiBeg, wApi, (int)apiCancel, ok6 ? "OK" : "FAIL");
    ok &= ok6;

    printf("RESULT: %s\n", ok ? "ALL OK" : "FAILED");
    UICornerstone_DestroyInstance(g);
    printf("auto=0\n");
    return ok ? 0 : 2;
}
