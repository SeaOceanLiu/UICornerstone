// test_p0_getter.c — 第七批配合修改 P0 项 + 第三批 P0-5 验证（纯 C，无渲染循环）
// 覆盖：
//   1. UICornerstone_SetControlId + FindControl（编程式控件 id 注册/移除）
//   2. Button getBoolProperty 读回 shadow（P0-1；#12 统一键）
//   3. font-size 对 button 生效（读回标题字号统一键，P0-3）
//   4. caption-label 句柄暴露（P0-5）：GetPtr → 句柄直控 Label 标准属性往返；非 Button 返回 0
// 缺省阴影色（P0-2）为编译期常量，此处不经运行断言（值见 ConstDef.cpp）。
#include "UICornerstoneAPI.h"
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

#ifdef UICORNERSTONE_BUILD_SHARED
#define CREATE_INSTANCE(inst) inst = UICornerstone_CreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL)
#else
#define CREATE_INSTANCE(inst) inst = UICornerstone_CreateInstance(GetUIBackendCallbacks(), NULL)
#endif

int main(void) {
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    setvbuf(stdout, NULL, _IONBF, 0);
    UIInstance inst;
    CREATE_INSTANCE(inst);
    if (!inst) { printf("FAIL: CreateInstance\n"); return 1; }

    int pass = 1;

    /* 1. SetControlId + FindControl（编程式创建控件注册 id） */
    UIControlHandle btn = UICornerstone_CreateButton(inst, "probe", 0, 0, 100, 40, 1.0f, 1.0f);
    if (!btn) { printf("FAIL: CreateButton\n"); UICornerstone_DestroyInstance(inst); return 1; }
    int rc = UICornerstone_SetControlId(inst, btn, "probe-btn");
    UIControlHandle found = UICornerstone_FindControl(inst, "probe-btn");
    printf("SetControlId=%d FindControl=%s\n", rc, found == btn ? "OK" : "FAIL");
    if (rc != 1 || found != btn) { printf("FAIL: SetControlId/FindControl\n"); pass = 0; }

    /* 移除（空 id）后应找不到 */
    rc = UICornerstone_SetControlId(inst, btn, "");
    found = UICornerstone_FindControl(inst, "probe-btn");
    printf("UnsetControlId=%d FindControl-after=%s\n", rc, found == NULL ? "OK(removed)" : "FAIL(仍存在)");
    if (rc != 1 || found != NULL) { printf("FAIL: SetControlId 移除\n"); pass = 0; }

    /* 2. Button getBoolProperty 读回 shadow（#12 统一键，设置后可读回） */
    int ok = UICornerstone_SetBool(inst, btn, "shadow", 1);
    int val = -1;
    int gok = UICornerstone_GetBool(inst, btn, "shadow", &val);
    printf("SetBool=%d GetBool=%d val=%d\n", ok, gok, val);
    if (ok != 1 || gok != 1 || val != 1) { printf("FAIL: shadow 读回（#12 统一键）\n"); pass = 0; }

    /* 3. font-size 对 button 生效（Int 分发，读回标题字号） */
    ok = UICornerstone_SetInt(inst, btn, "font-size", 22);
    int iv = 0;
    gok = UICornerstone_GetInt(inst, btn, "font-size", &iv);
    printf("SetFontSize=%d GetFontSize=%d val=%d\n", ok, gok, iv);
    if (ok != 1 || gok != 1 || iv != 22) { printf("FAIL: font-size 读回\n"); pass = 0; }

    /* 4. caption-label 句柄暴露（P0-5）：GetPtr → 句柄直控 Label 标准属性（SetColor/GetColor 往返） */
    void* cap = NULL;
    int rcCap = UICornerstone_GetPtr(inst, btn, "caption-label", &cap);
    printf("GetPtr(caption-label)=%d cap=%p\n", rcCap, cap);
    int setok = 0, getok = 0;
    UIColor got = {0, 0, 0, 0};
    if (cap) {
        UIColor red = {255, 0, 0, 255};
        getok = UICornerstone_GetColor(inst, cap, "text", &got);
        setok = UICornerstone_SetColor(inst, cap, "text", red);
        getok = UICornerstone_GetColor(inst, cap, "text", &got);
    }
    printf("SetColor(text)=%d GetColor=%d rgba=%d,%d,%d\n", setok, getok, got.r, got.g, got.b);
    if (rcCap != 1 || !cap || setok != 1 || getok != 1 || got.r != 255 || got.g != 0 || got.b != 0) {
        printf("FAIL: caption-label 句柄直控\n"); pass = 0;
    }

    /* 非 Button（Label）无该键：返回 0 不崩 */
    UIControlHandle lbl = UICornerstone_CreateLabel(inst, "L", 12, 0, 60, 80, 20, 1.0f, 1.0f);
    void* cap2 = (void*)0;
    int rcLbl = UICornerstone_GetPtr(inst, lbl, "caption-label", &cap2);
    printf("GetPtr(label caption-label)=%d (expect 0)\n", rcLbl);
    if (rcLbl != 0) { printf("FAIL: 非 Button caption-label 应返回 0\n"); pass = 0; }

    /* 5. P0-8：CheckBox caption-label 句柄（C ABI 端到端）+ WinFrame title-label 既有键回归 */
    UIControlHandle cbx = UICornerstone_CreateCheckBox(inst, "CB", 0, 90, 120, 30, 1.0f, 1.0f);
    void* ccap = NULL;
    int rcCb = cbx ? UICornerstone_GetPtr(inst, cbx, "caption-label", &ccap) : 0;
    int setok2 = 0, getok2 = 0;
    UIColor got2 = {0, 0, 0, 0};
    if (ccap) {
        UIColor blue = {0, 0, 255, 255};
        setok2 = UICornerstone_SetColor(inst, ccap, "text", blue);
        getok2 = UICornerstone_GetColor(inst, ccap, "text", &got2);
    }
    printf("CheckBox GetPtr(caption-label)=%d SetColor=%d GetColor=%d rgba=%d,%d,%d\n",
           rcCb, setok2, getok2, got2.r, got2.g, got2.b);
    if (rcCb != 1 || !ccap || setok2 != 1 || getok2 != 1 || got2.b != 255) {
        printf("FAIL: CheckBox caption-label 句柄直控\n"); pass = 0;
    }

    UIControlHandle wf = UICornerstone_CreateWinFrame(inst, "WF", 200, 0, 240, 160, 1.0f, 1.0f);
    void* wtl = NULL;
    int rcWf = wf ? UICornerstone_GetPtr(inst, wf, "title-label", &wtl) : 0;
    printf("WinFrame GetPtr(title-label)=%d (expect 1, 既有键回归; caption-label 不新增)\n", rcWf);
    if (rcWf != 1 || !wtl) { printf("FAIL: WinFrame title-label 既有键\n"); pass = 0; }
    void* wcl = (void*)0;
    int rcWfAlias = wf ? UICornerstone_GetPtr(inst, wf, "caption-label", &wcl) : 0;
    printf("WinFrame GetPtr(caption-label)=%d (expect 0: 不新增别名)\n", rcWfAlias);
    if (rcWfAlias != 0) { printf("FAIL: WinFrame 不应有 caption-label 别名\n"); pass = 0; }

    /* 6. §3.1/#15：CheckBox 工厂文本与直控色经宿主持久化（recreate 后不丢） */
    UIControlHandle cb2 = UICornerstone_CreateCheckBox(inst, "T1", 0, 130, 120, 30, 1.0f, 1.0f);
    void* capA = NULL;
    if (cb2) UICornerstone_GetPtr(inst, cb2, "caption-label", &capA);
    UIColor grn = {0, 255, 0, 255};
    if (capA) UICornerstone_SetColor(inst, capA, "text.hover", grn);
    if (cb2) UICornerstone_SetRect(inst, cb2, 0, 150, 120, 30);   /* 触发 CheckBox recreate */
    char capbuf[32] = {0};
    int gs = cb2 ? UICornerstone_GetString(inst, cb2, "caption", capbuf, sizeof(capbuf)) : 0;
    printf("CheckBox factory text after recreate: rc=%d text=%s\n", gs, capbuf);
    if (gs != 1 || strcmp(capbuf, "T1") != 0) { printf("FAIL: 工厂文本 recreate 持久\n"); pass = 0; }
    void* capB = NULL;
    if (cb2) UICornerstone_GetPtr(inst, cb2, "caption-label", &capB);
    UIColor got3 = {0, 0, 0, 0};
    int rc3 = capB ? UICornerstone_GetColor(inst, capB, "text.hover", &got3) : 0;
    printf("CheckBox direct color after recreate: rc=%d rgba=%d,%d,%d\n", rc3, got3.r, got3.g, got3.b);
    if (rc3 != 1 || got3.r != 0 || got3.g != 255 || got3.b != 0) { printf("FAIL: 直控色 recreate 持久\n"); pass = 0; }

    UICornerstone_DestroyInstance(inst);
    if (pass) { printf("=== PASS: test_p0_getter ===\n"); return 0; }
    printf("=== FAIL: test_p0_getter ===\n");
    return 1;
}