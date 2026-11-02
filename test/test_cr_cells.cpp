// temp_cr_cells_probe.cpp - CR20261101001/002/003 引擎行为验证（C ABI）
#include "UICornerstoneAPI.h"
#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

static UIInstance g = nullptr;
static int evBegin = 0, evEdited = 0, evCancel = 0;
static string lastOld, lastNew;
static int lastReadOnly = -1, lastRow = -1, lastCol = -1;
static int nodeBegin = 0, nodeEdited = 0, nodeCancel = 0;
static string nodeOld, nodeNew, nodeId;
static int nodeReadOnly = -1;
static int rzBegin = 0, rzChanged = 0, rzEnded = 0;
static int rzCol = -1, rzCanceled = -1;
static float rzOld = 0, rzNew = 0;

static void onCellEv(UIControlHandle, const UIEventData* e, void*) {
    if (!e || !e->eventName) return;
    string n = e->eventName;
    if (n == "cell-edit-begin" || n == "cell-edited" || n == "cell-edit-cancel") {
        if (n == "cell-edit-begin") evBegin++;
        else if (n == "cell-edited") evEdited++;
        else evCancel++;
        lastRow = e->data.grid.row;
        lastCol = e->data.grid.col;
        lastOld = e->data.grid.oldText ? e->data.grid.oldText : "";
        lastNew = e->data.grid.newText ? e->data.grid.newText : "";
        lastReadOnly = e->data.grid.editReadOnly;
    } else if (n == "node-edit-begin" || n == "node-edited" || n == "node-edit-cancel") {
        if (n == "node-edit-begin") nodeBegin++;
        else if (n == "node-edited") nodeEdited++;
        else nodeCancel++;
        nodeId = e->data.treeNode.id ? e->data.treeNode.id : "";
        nodeOld = e->data.treeNode.oldText ? e->data.treeNode.oldText : "";
        nodeNew = e->data.treeNode.newText ? e->data.treeNode.newText : "";
        nodeReadOnly = e->data.treeNode.editReadOnly;
    } else if (n == "column-resize-begin" || n == "column-resize-changed" || n == "column-resize-ended") {
        if (n == "column-resize-begin") rzBegin++;
        else if (n == "column-resize-changed") rzChanged++;
        else rzEnded++;
        rzCol = e->data.resize.col;
        rzOld = e->data.resize.oldWidth;
        rzNew = e->data.resize.newWidth;
        rzCanceled = e->data.resize.canceled;
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
static void key(int code, int mod = 0) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = UI_EVENT_KEY_DOWN; UI_EVENT_KEY_CODE(&e) = code; UI_EVENT_KEY_MOD(&e) = (uint16_t)mod;
    UICornerstone_PushUIEvent(g, &e);
}
static void keyUp(int code) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = UI_EVENT_KEY_UP; UI_EVENT_KEY_CODE(&e) = code;
    UICornerstone_PushUIEvent(g, &e);
}
static void textInput(const char* s) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = UI_EVENT_TEXT_INPUT;
    strncpy((char*)e.data, s, sizeof(e.data) - 1);
    UICornerstone_PushUIEvent(g, &e);
}
static void wheel(float x, float y, float dy) {
    UIEvent e; memset(&e, 0, sizeof(e));
    e.type = UI_EVENT_MOUSE_WHEEL;
    UI_EVENT_WHEEL_DELTA(&e) = dy;
    UI_EVENT_WHEEL_MOUSE_X(&e) = x;
    UI_EVENT_WHEEL_MOUSE_Y(&e) = y;
    UICornerstone_PushUIEvent(g, &e);
}
static string cellText(void* lv, int r, int c) {
    char buf[256] = {0};
    if (!UICornerstone_ListViewGetCellText(g, lv, r, c, buf, (int)sizeof(buf))) return "<fail>";
    return buf;
}
static string editingId(void* tv, bool* ro = nullptr) {
    int r = 0;
    int len = UICornerstone_TreeViewGetEditingId(g, tv, nullptr, 0, &r);
    if (len <= 0) { if (ro) *ro = (r != 0); return ""; }
    string s((size_t)len, '\0');
    int got = UICornerstone_TreeViewGetEditingId(g, tv, &s[0], len + 1, &r);
    if (got < 0) s.clear();
    else s.resize((size_t)got);
    if (ro) *ro = (r != 0);
    return s;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    g = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!g) { printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(g, 0, 0, 900, 700);
    bool ok = true;

    // ===== CR-001 ListView =====
    void* lv = UICornerstone_CreateListView(g, 10, 10, 400, 160, 1, 1);
    UICornerstone_ListViewAddColumn(g, lv, "A", 100, 1);
    UICornerstone_ListViewAddColumn(g, lv, "B", 120, 0);
    UICornerstone_ListViewAddColumn(g, lv, "C", 80, 0);
    const char* r0[] = {"A0", "B0", "C0"};
    const char* r1[] = {"A1", "B1", "C1"};
    const char* r2[] = {"A2", "B2", "C2"};
    UICornerstone_ListViewAddRow(g, lv, "r0", 3, r0);
    UICornerstone_ListViewAddRow(g, lv, "r1", 3, r1);
    UICornerstone_ListViewAddRow(g, lv, "r2", 3, r2);
    UICornerstone_SetBool(g, lv, "cell-editing", 1);
    UICornerstone_SetString(g, lv, "editable-columns", "0,1");
    UICornerstone_SetString(g, lv, "read-only-columns", "1");
    UICornerstone_SetString(g, lv, "read-only-row-ids", "r2");
    UICornerstone_SetCallback(g, lv, "cell-edit-begin", onCellEv, nullptr);
    UICornerstone_SetCallback(g, lv, "cell-edited", onCellEv, nullptr);
    UICornerstone_SetCallback(g, lv, "cell-edit-cancel", onCellEv, nullptr);
    UICornerstone_SetCallback(g, lv, "column-resize-begin", onCellEv, nullptr);
    UICornerstone_SetCallback(g, lv, "column-resize-changed", onCellEv, nullptr);
    UICornerstone_SetCallback(g, lv, "column-resize-ended", onCellEv, nullptr);
    pump(2);

    // 1) 可编辑单元格：Begin → 输入 → Enter 提交
    bool b1 = UICornerstone_ListViewBeginEditCell(g, lv, 0, 0) == 1;
    bool e1 = UICornerstone_ListViewIsEditing(g, lv) == 1;
    key(0x41, 0x40); keyUp(0x41); pump(1);
    textInput("alpha"); pump(1);
    key(0x0D); keyUp(0x0D); pump(2);
    bool ok1 = b1 && e1 && evBegin == 1 && evEdited == 1 && evCancel == 0 &&
               lastRow == 0 && lastCol == 0 && lastOld == "A0" && lastNew == "alpha" &&
               lastReadOnly == 0 && cellText(lv, 0, 0) == "alpha";
    printf("1) cell commit: begin=%d editing=%d ev=%d/%d/%d old='%s' new='%s' ro=%d cell='%s' %s\n",
           (int)b1, (int)e1, evBegin, evEdited, evCancel, lastOld.c_str(), lastNew.c_str(),
           lastReadOnly, cellText(lv, 0, 0).c_str(), ok1 ? "OK" : "FAIL");
    ok &= ok1;

    // 2) 只读行：仍可进入、EditBox 只读、Enter → cancel(readOnly=1)
    int eb = evBegin, ed = evEdited, ec = evCancel;
    bool b2 = UICornerstone_ListViewBeginEditCell(g, lv, 2, 0) == 1;
    key(0x41, 0x40); keyUp(0x41); pump(1);
    textInput("zzz"); pump(1);
    key(0x0D); keyUp(0x0D); pump(2);
    bool ok2 = b2 && evBegin == eb + 1 && evEdited == ed && evCancel == ec + 1 &&
               lastReadOnly == 1 && cellText(lv, 2, 0) == "A2";
    printf("2) readonly cell cancel: begin=%d ev=%d/%d/%d ro=%d cell='%s' %s\n",
           (int)b2, evBegin, evEdited, evCancel, lastReadOnly, cellText(lv, 2, 0).c_str(),
           ok2 ? "OK" : "FAIL");
    ok &= ok2;

    // 3) editable-columns 过滤（列 2 不可编辑）
    bool ok3 = UICornerstone_ListViewBeginEditCell(g, lv, 0, 2) == 0;
    printf("3) editable-columns filter: %s\n", ok3 ? "OK" : "FAIL");
    ok &= ok3;

    // 4) 部分可见行不进入（高度 60：第 1 行仅部分可见）
    void* lvSmall = UICornerstone_CreateListView(g, 420, 10, 300, 60, 1, 1);
    UICornerstone_SetBool(g, lvSmall, "cell-editing", 1);
    UICornerstone_ListViewAddColumn(g, lvSmall, "A", 100, 0);
    const char* s0[] = {"S0"};
    const char* s1[] = {"S1"};
    UICornerstone_ListViewAddRow(g, lvSmall, "s0", 1, s0);
    UICornerstone_ListViewAddRow(g, lvSmall, "s1", 1, s1);
    pump(2);
    bool ok4 = UICornerstone_ListViewBeginEditCell(g, lvSmall, 1, 0) == 0;
    printf("4) partial row blocked: %s\n", ok4 ? "OK" : "FAIL");
    ok &= ok4;

    // 5) 滚动取消编辑
    int ec5 = evCancel;
    bool b5 = UICornerstone_ListViewBeginEditCell(g, lv, 1, 0) == 1;
    pump(1);
    wheel(100, 100, -3);
    pump(2);
    bool ok5 = b5 && UICornerstone_ListViewIsEditing(g, lv) == 0 && evCancel == ec5 + 1 &&
               cellText(lv, 1, 0) == "A1";
    printf("5) scroll cancels edit: begin=%d editing=%d cancel=%d cell='%s' %s\n",
           (int)b5, UICornerstone_ListViewIsEditing(g, lv), evCancel - ec5,
           cellText(lv, 1, 0).c_str(), ok5 ? "OK" : "FAIL");
    ok &= ok5;

    // 6) CR-003 列宽会话：命中/Begin/Update/End（commit）
    float divX = 100.0f;
    int hit = UICornerstone_ListViewHitTestColumnDivider(g, lv, divX, 10);
    int rb = rzBegin, rc = rzChanged, re = rzEnded;
    bool b6 = UICornerstone_ListViewBeginColumnResize(g, lv, 0, divX) == 1;
    float w1 = UICornerstone_ListViewUpdateColumnResize(g, lv, divX + 30);
    float w1b = UICornerstone_ListViewUpdateColumnResize(g, lv, divX + 30);   // 去重
    bool e6 = UICornerstone_ListViewEndColumnResize(g, lv, 1) == 1;
    bool ok6 = hit == 0 && b6 && w1 == 130.0f && w1b == 130.0f &&
               rzBegin == rb + 1 && rzChanged == rc + 1 && rzEnded == re + 1 &&
               rzCol == 0 && rzCanceled == 0 && rzOld == 100.0f && rzNew == 130.0f;
    printf("6) resize commit: hit=%d w=%.1f/%.1f ev=%d/%d/%d old=%.1f new=%.1f %s\n",
           hit, w1, w1b, rzBegin - rb, rzChanged - rc, rzEnded - re, rzOld, rzNew,
           ok6 ? "OK" : "FAIL");
    ok &= ok6;

    // 7) 取消恢复 + ended 恰好一次
    int re2 = rzEnded, rc2 = rzChanged;
    bool b7 = UICornerstone_ListViewBeginColumnResize(g, lv, 1, 220) == 1;
    UICornerstone_ListViewUpdateColumnResize(g, lv, 240);
    bool c7 = UICornerstone_ListViewCancelColumnResize(g, lv) == 1;
    float curW = -1;   // 无 getter；用命中位置间接：列 0 宽 130，列 1 起点 130
    bool ok7 = b7 && c7 && rzEnded == re2 + 1 && rzCanceled == 1 && rzNew == rzOld;
    printf("7) resize cancel: begin=%d cancel=%d ended=%d canceled=%d old=%.1f new=%.1f %s\n",
           (int)b7, (int)c7, rzEnded - re2, rzCanceled, rzOld, rzNew, ok7 ? "OK" : "FAIL");
    ok &= ok7;

    // 8) 无会话 Update 返回 -1 且不改宽
    float u8 = UICornerstone_ListViewUpdateColumnResize(g, lv, 500);
    bool ok8 = u8 == -1.0f && UICornerstone_ListViewEndColumnResize(g, lv, 1) == 0;
    printf("8) update without session: %.1f %s\n", u8, ok8 ? "OK" : "FAIL");
    ok &= ok8;

    // ===== CR-002 TreeView =====
    void* tv = UICornerstone_CreateTreeView(g, 10, 200, 300, 220, 1, 1);
    UICornerstone_TreeViewAddNode(g, tv, "", "n1", "Node1", 1);
    UICornerstone_TreeViewAddNode(g, tv, "", "n2", "Node2", 1);
    UICornerstone_TreeViewAddNode(g, tv, "n1", "c1", "Child1", 1);
    UICornerstone_SetBool(g, tv, "label-editing", 1);
    UICornerstone_SetString(g, tv, "read-only-ids", "n2");
    UICornerstone_SetCallback(g, tv, "node-edit-begin", onCellEv, nullptr);
    UICornerstone_SetCallback(g, tv, "node-edited", onCellEv, nullptr);
    UICornerstone_SetCallback(g, tv, "node-edit-cancel", onCellEv, nullptr);
    pump(2);

    // 9) 节点编辑提交
    bool b9 = UICornerstone_TreeViewBeginEditNode(g, tv, "n1") == 1;
    bool ed9 = UICornerstone_TreeViewIsEditing(g, tv) == 1;
    bool ro9 = false;
    string id9 = editingId(tv, &ro9);
    key(0x41, 0x40); keyUp(0x41); pump(1);
    textInput("Renamed"); pump(1);
    key(0x0D); keyUp(0x0D); pump(2);
    bool ok9 = b9 && ed9 && id9 == "n1" && ro9 == false &&
               nodeBegin == 1 && nodeEdited == 1 && nodeCancel == 0 &&
               nodeId == "n1" && nodeOld == "Node1" && nodeNew == "Renamed" && nodeReadOnly == 0;
    printf("9) node commit: begin=%d editing=%d id='%s' ev=%d/%d/%d old='%s' new='%s' ro=%d %s\n",
           (int)b9, (int)ed9, id9.c_str(), nodeBegin, nodeEdited, nodeCancel,
           nodeOld.c_str(), nodeNew.c_str(), nodeReadOnly, ok9 ? "OK" : "FAIL");
    ok &= ok9;

    // 10) 只读节点 → cancel；空 id Begin 失败
    int nb = nodeBegin, ne = nodeEdited, nc = nodeCancel;
    bool b10 = UICornerstone_TreeViewBeginEditNode(g, tv, "n2") == 1;
    key(0x41, 0x40); keyUp(0x41); pump(1);
    textInput("zzz"); pump(1);
    key(0x0D); keyUp(0x0D); pump(2);
    bool ok10 = b10 && nodeBegin == nb + 1 && nodeEdited == ne && nodeCancel == nc + 1 &&
                nodeReadOnly == 1 &&
                UICornerstone_TreeViewBeginEditNode(g, tv, "") == 0;
    printf("10) readonly node cancel + empty id: begin=%d ev=%d/%d/%d ro=%d %s\n",
           (int)b10, nodeBegin, nodeEdited, nodeCancel, nodeReadOnly, ok10 ? "OK" : "FAIL");
    ok &= ok10;

    // 11) 展开取消编辑
    int nc11 = nodeCancel;
    bool b11 = UICornerstone_TreeViewBeginEditNode(g, tv, "c1") == 1 ||
               UICornerstone_TreeViewBeginEditNode(g, tv, "n1") == 1;
    pump(1);
    UICornerstone_TreeViewCollapseNode(g, tv, "n1");
    pump(2);
    bool ok11 = b11 && UICornerstone_TreeViewIsEditing(g, tv) == 0 && nodeCancel == nc11 + 1;
    printf("11) collapse cancels edit: begin=%d editing=%d cancel=%d %s\n",
           (int)b11, UICornerstone_TreeViewIsEditing(g, tv), nodeCancel - nc11,
           ok11 ? "OK" : "FAIL");
    ok &= ok11;

    // 12) 声明式解析
    const char* layout = R"json({
      "controls": [
        { "type": "list-view", "id": "lvj", "rect": { "x": 420, "y": 300, "w": 300, "h": 120 },
          "cell-editing": true, "editable-columns": "0", "read-only-row-ids": "rj",
          "columns": [ { "title": "A", "width": 100 } ],
          "rows": [ { "id": "rj", "cells": ["x"] } ] },
        { "type": "tree-view", "id": "tvj", "rect": { "x": 420, "y": 440, "w": 300, "h": 160 },
          "label-editing": true, "read-only-ids": "nj",
          "items": [ { "id": "nj", "label": "N" } ] }
      ]
    })json";
    bool lr = UICornerstone_LoadLayout(g, layout) == 1;
    void* lvj = UICornerstone_FindControl(g, "lvj");
    void* tvj = UICornerstone_FindControl(g, "tvj");
    int ce = -1, le = -1;
    if (lvj) UICornerstone_GetBool(g, lvj, "cell-editing", &ce);
    if (tvj) UICornerstone_GetBool(g, tvj, "label-editing", &le);
    bool ok12 = lr && lvj && tvj && ce == 1 && le == 1;
    printf("12) declarative keys: load=%d cell-editing=%d label-editing=%d %s\n",
           (int)lr, ce, le, ok12 ? "OK" : "FAIL");
    ok &= ok12;

    // 13) CR-003 外部 setColumnWidth 取消会话（ended 恰好一次；最终宽=外部请求值）
    int re13 = rzEnded;
    bool b13 = UICornerstone_ListViewBeginColumnResize(g, lv, 0, 130.0f) == 1;
    UICornerstone_ListViewUpdateColumnResize(g, lv, 140.0f);
    UICornerstone_ListViewSetColumnWidth(g, lv, 0, 200.0f);
    int hit13 = UICornerstone_ListViewHitTestColumnDivider(g, lv, 200.0f, 10);
    bool ok13 = b13 && rzEnded == re13 + 1 && rzCanceled == 1 && hit13 == 0 &&
                UICornerstone_ListViewCancelColumnResize(g, lv) == 0;
    printf("13) external width cancels session: begin=%d ended=%d canceled=%d hit200=%d %s\n",
           (int)b13, rzEnded - re13, rzCanceled, hit13, ok13 ? "OK" : "FAIL");
    ok &= ok13;

    // 14) CR-003 隐藏/禁用前置校验
    UICornerstone_SetBool(g, lv, "visible", 0);
    int hitHide = UICornerstone_ListViewHitTestColumnDivider(g, lv, 200.0f, 10);
    bool begHide = UICornerstone_ListViewBeginColumnResize(g, lv, 0, 200.0f) == 1;
    UICornerstone_SetBool(g, lv, "visible", 1);
    UICornerstone_SetBool(g, lv, "enabled", 0);
    int hitDis = UICornerstone_ListViewHitTestColumnDivider(g, lv, 200.0f, 10);
    bool begDis = UICornerstone_ListViewBeginColumnResize(g, lv, 0, 200.0f) == 1;
    UICornerstone_SetBool(g, lv, "enabled", 1);
    bool ok14 = hitHide == -1 && !begHide && hitDis == -1 && !begDis;
    printf("14) hidden/disabled resize: hit=%d/%d begin=%d/%d %s\n",
           hitHide, hitDis, (int)begHide, (int)begDis, ok14 ? "OK" : "FAIL");
    ok &= ok14;

    // 15) 销毁静默（不发 cancel）
    void* lvDel = UICornerstone_CreateListView(g, 10, 520, 300, 120, 1, 1);
    UICornerstone_SetBool(g, lvDel, "cell-editing", 1);
    UICornerstone_ListViewAddColumn(g, lvDel, "A", 100, 0);
    const char* d0[] = {"D0"};
    UICornerstone_ListViewAddRow(g, lvDel, "d0", 1, d0);
    UICornerstone_SetCallback(g, lvDel, "cell-edit-cancel", onCellEv, nullptr);
    UICornerstone_SetCallback(g, lvDel, "cell-edited", onCellEv, nullptr);
    pump(1);
    int ec15 = evCancel, ee15 = evEdited;
    bool b15 = UICornerstone_ListViewBeginEditCell(g, lvDel, 0, 0) == 1;
    UICornerstone_DestroyControl(g, lvDel);
    pump(2);
    bool ok15 = b15 && evCancel == ec15 && evEdited == ee15;
    printf("15) destroy silent: begin=%d cancelDelta=%d editedDelta=%d %s\n",
           (int)b15, evCancel - ec15, evEdited - ee15, ok15 ? "OK" : "FAIL");
    ok &= ok15;

    // 16) TreeView 销毁静默（不发 cancel/edited）
    void* tvDel = UICornerstone_CreateTreeView(g, 10, 560, 300, 120, 1, 1);
    UICornerstone_TreeViewAddNode(g, tvDel, "", "nd", "ND", 1);
    UICornerstone_SetBool(g, tvDel, "label-editing", 1);
    UICornerstone_SetCallback(g, tvDel, "node-edit-cancel", onCellEv, nullptr);
    UICornerstone_SetCallback(g, tvDel, "node-edited", onCellEv, nullptr);
    pump(1);
    int nc16 = nodeCancel, ne16 = nodeEdited;
    bool b16 = UICornerstone_TreeViewBeginEditNode(g, tvDel, "nd") == 1;
    UICornerstone_DestroyControl(g, tvDel);
    pump(2);
    bool ok16 = b16 && nodeCancel == nc16 && nodeEdited == ne16;
    printf("16) tree destroy silent: begin=%d cancelDelta=%d editedDelta=%d %s\n",
           (int)b16, nodeCancel - nc16, nodeEdited - ne16, ok16 ? "OK" : "FAIL");
    ok &= ok16;

    printf("RESULT: %s\n", ok ? "ALL OK" : "FAILED");
    UICornerstone_DestroyInstance(g);
    printf("auto=0\n");
    return ok ? 0 : 2;
}
