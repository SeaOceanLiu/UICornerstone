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

    /* 7. P0-21：四态图/animation 运行时读回（设置原值保持）+ Bonus A/B */
    char sbuf[256];
    int s1 = UICornerstone_SetString(inst, btn, "normal-image", "assets/images/bitmap1.bmp");
    memset(sbuf, 0, sizeof(sbuf));
    int g1 = UICornerstone_GetString(inst, btn, "normal-image", sbuf, sizeof(sbuf));
    printf("Set(normal-image)=%d Get=%d val=%s\n", s1, g1, sbuf);
    if (s1 != 1 || g1 != 1 || strcmp(sbuf, "assets/images/bitmap1.bmp") != 0) {
        printf("FAIL: normal-image 原值读回\n"); pass = 0;
    }

    /* 未设置态：读回 0（不崩） */
    memset(sbuf, 0, sizeof(sbuf));
    int g2 = UICornerstone_GetString(inst, btn, "hover-image", sbuf, sizeof(sbuf));
    printf("Get(hover-image unset)=%d (expect 0)\n", g2);
    if (g2 != 0) { printf("FAIL: 未设置态应返回 0\n"); pass = 0; }

    /* animation 往返（有效资源；basePath 由引擎解析，原值读回） */
    int s3 = UICornerstone_SetString(inst, btn, "animation", "assets/animations/rotateBtn/rotateBtn.jsonc");
    memset(sbuf, 0, sizeof(sbuf));
    int g3 = UICornerstone_GetString(inst, btn, "animation", sbuf, sizeof(sbuf));
    printf("Set(animation)=%d Get=%d val=%s\n", s3, g3, sbuf);
    if (s3 != 1 || g3 != 1 || strcmp(sbuf, "assets/animations/rotateBtn/rotateBtn.jsonc") != 0) {
        printf("FAIL: animation 原值读回\n"); pass = 0;
    }

    /* Bonus B：内嵌动画 frames 读回转发 */
    int tf = 0, cf = 0;
    int b1 = UICornerstone_GetInt(inst, btn, "total-frames", &tf);
    int b2 = UICornerstone_GetInt(inst, btn, "current-frame", &cf);
    printf("Get(total-frames)=%d val=%d Get(current-frame)=%d val=%d\n", b1, tf, b2, cf);
    if (b1 != 1 || tf <= 0 || b2 != 1) { printf("FAIL: frames 读回转发\n"); pass = 0; }

    /* Bonus A：无效 animation 路径 → 返回 0、进程存活、旧动画保留 */
    int s4 = UICornerstone_SetString(inst, btn, "animation", "assets/animations/__missing__.jsonc");
    memset(sbuf, 0, sizeof(sbuf));
    int g4 = UICornerstone_GetString(inst, btn, "animation", sbuf, sizeof(sbuf));
    printf("Set(invalid animation)=%d (expect 0) Get after=%d val=%s\n", s4, g4, sbuf);
    if (s4 != 0 || g4 != 1 || strcmp(sbuf, "assets/animations/rotateBtn/rotateBtn.jsonc") != 0) {
        printf("FAIL: 无效 animation 应返回 0 且旧动画保留\n"); pass = 0;
    }

    /* 8. LuotiAni 独立控件 animation 原值读回（P0-21 同型延伸） */
    UIControlHandle ani = UICornerstone_CreateAnimation(inst, "assets/animations/rotateBtn/rotateBtn.jsonc",
                                                        300, 300, 120, 120, 1.0f, 1.0f);
    memset(sbuf, 0, sizeof(sbuf));
    int g5 = ani ? UICornerstone_GetString(inst, ani, "animation", sbuf, sizeof(sbuf)) : 0;
    printf("LuotiAni GetString(animation)=%d val=%s\n", g5, sbuf);
    if (g5 != 1 || strcmp(sbuf, "assets/animations/rotateBtn/rotateBtn.jsonc") != 0) {
        printf("FAIL: LuotiAni animation 原值读回\n"); pass = 0;
    }

    /* 9. P0-22：image 空串 = 卸载（读回 0）；再设有效路径恢复；失败路径读回=尝试值 */
    int s5 = UICornerstone_SetString(inst, btn, "normal-image", "");
    memset(sbuf, 0, sizeof(sbuf));
    int g6 = UICornerstone_GetString(inst, btn, "normal-image", sbuf, sizeof(sbuf));
    printf("Set(normal-image empty)=%d Get=%d (expect 1/0)\n", s5, g6);
    if (s5 != 1 || g6 != 0) { printf("FAIL: 空串卸载\n"); pass = 0; }

    int s6 = UICornerstone_SetString(inst, btn, "normal-image", "assets/images/bitmap2.bmp");
    memset(sbuf, 0, sizeof(sbuf));
    int g7 = UICornerstone_GetString(inst, btn, "normal-image", sbuf, sizeof(sbuf));
    printf("Set(restore)=%d Get=%d val=%s\n", s6, g7, sbuf);
    if (s6 != 1 || g7 != 1 || strcmp(sbuf, "assets/images/bitmap2.bmp") != 0) {
        printf("FAIL: 卸载后恢复\n"); pass = 0;
    }

    int s7 = UICornerstone_SetString(inst, btn, "normal-image", "assets/images/__missing__.bmp");
    memset(sbuf, 0, sizeof(sbuf));
    int g8 = UICornerstone_GetString(inst, btn, "normal-image", sbuf, sizeof(sbuf));
    printf("Set(bad image)=%d Get=%d val=%s (readback=attempted)\n", s7, g8, sbuf);
    if (s7 != 1 || g8 != 1 || strcmp(sbuf, "assets/images/__missing__.bmp") != 0) {
        printf("FAIL: 失败路径保留尝试值\n"); pass = 0;
    }

    /* 10. P0-24/P0-25：path 别名 + CreateAnimation 空路径占位 */
    memset(sbuf, 0, sizeof(sbuf));
    int a1 = UICornerstone_GetString(inst, ani, "path", sbuf, sizeof(sbuf));
    printf("GetString(path alias)=%d val=%s\n", a1, sbuf);
    if (a1 != 1 || strcmp(sbuf, "assets/animations/rotateBtn/rotateBtn.jsonc") != 0) {
        printf("FAIL: path 别名读回\n"); pass = 0;
    }
    int a2 = UICornerstone_SetString(inst, ani, "path", "assets/animations/rotateBtn/rotateBtn.jsonc");
    memset(sbuf, 0, sizeof(sbuf));
    int a3 = UICornerstone_GetString(inst, ani, "animation", sbuf, sizeof(sbuf));
    printf("Set(path)=%d Get(animation)=%d val=%s\n", a2, a3, sbuf);
    if (a2 != 1 || a3 != 1 || strcmp(sbuf, "assets/animations/rotateBtn/rotateBtn.jsonc") != 0) {
        printf("FAIL: path 别名写入\n"); pass = 0;
    }

    UIControlHandle ani2 = UICornerstone_CreateAnimation(inst, "", 450, 300, 120, 120, 1.0f, 1.0f);
    memset(sbuf, 0, sizeof(sbuf));
    int b0 = ani2 ? UICornerstone_GetString(inst, ani2, "animation", sbuf, sizeof(sbuf)) : -1;
    printf("CreateAnimation(\"\") handle=%s GetString=%d (expect OK/0)\n", ani2 ? "OK" : "NULL", b0);
    if (!ani2 || b0 != 0) { printf("FAIL: 空路径占位创建\n"); pass = 0; }
    int c1 = ani2 ? UICornerstone_SetString(inst, ani2, "path", "assets/animations/rotateBtn/rotateBtn.jsonc") : 0;
    memset(sbuf, 0, sizeof(sbuf));
    int c2 = ani2 ? UICornerstone_GetString(inst, ani2, "animation", sbuf, sizeof(sbuf)) : 0;
    int tot = 0;
    int c3 = ani2 ? UICornerstone_GetInt(inst, ani2, "total-frames", &tot) : 0;
    printf("placeholder Set(path)=%d Get=%d val=%s total=%d\n", c1, c2, sbuf, tot);
    if (c1 != 1 || c2 != 1 || c3 != 1 || tot <= 0) { printf("FAIL: 占位后加载\n"); pass = 0; }

    /* 11. P0-26：视觉能力补齐读回（各类型） */
    {
        UIColor c = {10, 20, 30, 255};
        UIColor got = {0, 0, 0, 0};

        /* image：背景/边框色（补齐绘制） */
        UIControlHandle img2 = UICornerstone_CreateImage(inst, NULL, 0, 0, 60, 30, 1.0f, 1.0f);
        int r1 = img2 ? UICornerstone_SetColor(inst, img2, "background", c) : 0;
        int r2 = img2 ? UICornerstone_GetColor(inst, img2, "background", &got) : 0;
        printf("P0-26 image background set=%d get=%d rgba=%d,%d,%d\n", r1, r2, got.r, got.g, got.b);
        if (r1 != 1 || r2 != 1 || got.r != 10 || got.g != 20 || got.b != 30) { printf("FAIL: image 背景\n"); pass = 0; }

        /* color-picker：关闭态文字色（四态）/阴影/字号/字体名 */
        UIControlHandle cp2 = UICornerstone_CreateColorPicker(inst, 0, 60, 80, 30, "#11223344", 1.0f, 1.0f);
        got = (UIColor){0, 0, 0, 0};
        int p1 = cp2 ? UICornerstone_SetColor(inst, cp2, "text.hover", c) : 0;
        int p2 = cp2 ? UICornerstone_GetColor(inst, cp2, "text.hover", &got) : 0;
        int p3 = cp2 ? UICornerstone_SetBool(inst, cp2, "shadow", 1) : 0;
        int p4 = 0;
        if (cp2) UICornerstone_GetBool(inst, cp2, "shadow", &p4);
        int p5 = cp2 ? UICornerstone_SetInt(inst, cp2, "font-size", 17) : 0;
        int p6 = 0;
        if (cp2) UICornerstone_GetInt(inst, cp2, "font-size", &p6);
        printf("P0-26 colorpicker text.hover=%d/%d rgba=%d,%d,%d shadow=%d/%d font-size=%d/%d\n",
               p1, p2, got.r, got.g, got.b, p3, p4, p5, p6);
        if (p1 != 1 || p2 != 1 || got.r != 10 || p3 != 1 || p4 != 1 || p5 != 1 || p6 != 17) {
            printf("FAIL: colorpicker 关闭态补齐\n"); pass = 0;
        }
        char fbuf2[64] = {0};
        int p7 = cp2 ? UICornerstone_SetEnum(inst, cp2, "font", "maplemono-nf-cn-regular") : 0;
        int p8 = cp2 ? UICornerstone_GetEnum(inst, cp2, "font", fbuf2, sizeof(fbuf2)) : 0;
        if (p7 != 1 || p8 != 1 || strcmp(fbuf2, "maplemono-nf-cn-regular") != 0) {
            printf("FAIL: colorpicker font（%d/%d %s）\n", p7, p8, fbuf2); pass = 0;
        }

        /* slider：label 四态/阴影/偏移 */
        UIControlHandle sl2 = UICornerstone_CreateSlider(inst, 0, 100, 120, 30, 0, 100, 50, 1.0f, 1.0f);
        got = (UIColor){0, 0, 0, 0};
        int s1 = sl2 ? UICornerstone_SetColor(inst, sl2, "text.hover", c) : 0;
        int s2 = sl2 ? UICornerstone_GetColor(inst, sl2, "text.hover", &got) : 0;
        if (sl2) UICornerstone_SetBool(inst, sl2, "show-value-label", 1);   /* P0-26：valueLabel 需先启用 */
        int s3 = sl2 ? UICornerstone_SetBool(inst, sl2, "shadow", 1) : 0;
        int s4 = sl2 ? UICornerstone_SetFloat(inst, sl2, "shadow-offset-x", 3.0f) : 0;
        float sf = 0;
        int s5 = sl2 ? UICornerstone_GetFloat(inst, sl2, "shadow-offset-x", &sf) : 0;
        printf("P0-26 slider text.hover=%d/%d shadow=%d offx=%d/%d %.1f\n", s1, s2, s3, s4, s5, sf);
        if (s1 != 1 || s2 != 1 || got.r != 10 || s3 != 1 || s4 != 1 || s5 != 1 || sf != 3.0f) {
            printf("FAIL: slider label 补齐\n"); pass = 0;
        }

        /* menu-bar：背景/文本/阴影（统一覆盖 bar/panel/item） */
        UIControlHandle mb2 = UICornerstone_CreateMenuBar(inst, 0, 140, 300, 24, 1.0f, 1.0f);
        got = (UIColor){0, 0, 0, 0};
        int m1 = mb2 ? UICornerstone_SetColor(inst, mb2, "background", c) : 0;
        int m2 = mb2 ? UICornerstone_GetColor(inst, mb2, "background", &got) : 0;
        int m3 = mb2 ? UICornerstone_SetBool(inst, mb2, "shadow", 1) : 0;
        int m4 = 0;
        if (mb2) UICornerstone_GetBool(inst, mb2, "shadow", &m4);
        printf("P0-26 menu background=%d/%d rgba=%d,%d,%d shadow=%d/%d\n", m1, m2, got.r, got.g, got.b, m3, m4);
        if (m1 != 1 || m2 != 1 || got.r != 10 || m3 != 1 || m4 != 1) { printf("FAIL: menu 补齐\n"); pass = 0; }

        /* status-bar：文本色/字体名/阴影 */
        UIControlHandle sb2 = UICornerstone_CreateStatusBar(inst, 0, 170, 300, 24, 1.0f, 1.0f);
        got = (UIColor){0, 0, 0, 0};
        int b1b = sb2 ? UICornerstone_SetColor(inst, sb2, "text", c) : 0;
        int b2b = sb2 ? UICornerstone_GetColor(inst, sb2, "text", &got) : 0;
        int b3b = sb2 ? UICornerstone_SetBool(inst, sb2, "shadow", 1) : 0;
        char fbuf3[64] = {0};
        int b4b = sb2 ? UICornerstone_SetEnum(inst, sb2, "font", "maplemono-nf-cn-regular") : 0;
        int b5b = sb2 ? UICornerstone_GetEnum(inst, sb2, "font", fbuf3, sizeof(fbuf3)) : 0;
        printf("P0-26 statusbar text=%d/%d rgba=%d,%d,%d shadow=%d font=%d/%d %s\n",
               b1b, b2b, got.r, got.g, got.b, b3b, b4b, b5b, fbuf3);
        if (b1b != 1 || b2b != 1 || got.r != 10 || b3b != 1 || b4b != 1 || b5b != 1) {
            printf("FAIL: statusbar 补齐\n"); pass = 0;
        }

        /* tab-control：常态文本四态 + selected-text */
        UIControlHandle tc2 = UICornerstone_CreateTabControl(inst, 0, 200, 200, 100, 1.0f, 1.0f);
        got = (UIColor){0, 0, 0, 0};
        int t1b = tc2 ? UICornerstone_SetColor(inst, tc2, "text.hover", c) : 0;
        int t2b = tc2 ? UICornerstone_GetColor(inst, tc2, "text.hover", &got) : 0;
        UIColor sel = {200, 100, 50, 255};
        int t3b = tc2 ? UICornerstone_SetColor(inst, tc2, "selected-text", sel) : 0;
        int t4b = tc2 ? UICornerstone_GetColor(inst, tc2, "selected-text", &got) : 0;
        printf("P0-26 tabcontrol text.hover=%d/%d selected-text=%d/%d rgba=%d,%d,%d\n",
               t1b, t2b, t3b, t4b, got.r, got.g, got.b);
        if (t1b != 1 || t2b != 1 || t3b != 1 || t4b != 1 || got.r != 200) { printf("FAIL: tabcontrol 补齐\n"); pass = 0; }

        /* progress-bar：阴影转发（内嵌 Label） */
        UIControlHandle pb2 = UICornerstone_CreateProgressBar(inst, 0, 310, 150, 20, 1.0f, 1.0f);
        int g1b = pb2 ? UICornerstone_SetBool(inst, pb2, "shadow", 1) : 0;
        int g2b = 0;
        if (pb2) UICornerstone_GetBool(inst, pb2, "shadow", &g2b);
        float pf = 0;
        int g3b = pb2 ? UICornerstone_SetFloat(inst, pb2, "shadow-offset-y", 2.0f) : 0;
        int g4b = pb2 ? UICornerstone_GetFloat(inst, pb2, "shadow-offset-y", &pf) : 0;
        printf("P0-26 progressbar shadow=%d/%d offy=%d/%d %.1f\n", g1b, g2b, g3b, g4b, pf);
        if (g1b != 1 || g2b != 1 || g3b != 1 || g4b != 1 || pf != 2.0f) { printf("FAIL: progressbar 补齐\n"); pass = 0; }

        /* tree-view：item 阴影（item-id 定位） */
        UIControlHandle tv2 = UICornerstone_CreateTreeView(inst, 0, 340, 150, 80, 1.0f, 1.0f);
        if (tv2) UICornerstone_TreeViewAddNode(inst, tv2, "", "n1", "Node1", 1);
        int v1 = tv2 ? UICornerstone_SetString(inst, tv2, "item-id", "n1") : 0;
        got = (UIColor){0, 0, 0, 0};
        int v2 = tv2 ? UICornerstone_SetColor(inst, tv2, "item-text-shadow", c) : 0;
        int v3 = tv2 ? UICornerstone_GetColor(inst, tv2, "item-text-shadow", &got) : 0;
        int v4 = tv2 ? UICornerstone_SetFloat(inst, tv2, "item-shadow-offset-x", 4.0f) : 0;
        float vf = 0;
        int v5 = tv2 ? UICornerstone_GetFloat(inst, tv2, "item-shadow-offset-x", &vf) : 0;
        printf("P0-26 treeview item shadow set=%d/%d rgba=%d,%d,%d offx=%d/%d %.1f\n",
               v1, v2, got.r, got.g, got.b, v4, v5, vf);
        if (v1 != 1 || v2 != 1 || v3 != 1 || got.r != 10 || v4 != 1 || v5 != 1 || vf != 4.0f) {
            printf("FAIL: treeview item 阴影\n"); pass = 0;
        }

        /* P0-26 复核修正：tree 文本四态（原遮蔽基类成员 → 已解除） */
        got = (UIColor){0, 0, 0, 0};
        int z1 = tv2 ? UICornerstone_SetColor(inst, tv2, "text.hover", c) : 0;
        int z2 = tv2 ? UICornerstone_GetColor(inst, tv2, "text.hover", &got) : 0;
        printf("P0-26 treeview text.hover=%d/%d rgba=%d,%d,%d\n", z1, z2, got.r, got.g, got.b);
        if (z1 != 1 || z2 != 1 || got.r != 10 || got.g != 20 || got.b != 30) { printf("FAIL: treeview 文本四态\n"); pass = 0; }

        /* list-view：cell 阴影（新 C ABI） */
        UIControlHandle lv2 = UICornerstone_CreateListView(inst, 0, 430, 200, 80, 1.0f, 1.0f);
        if (lv2) {
            const char* cells[2] = {"a", "b"};
            UICornerstone_ListViewAddRow(inst, lv2, "r1", 2, cells);
        }
        int w1 = lv2 ? UICornerstone_ListViewSetCellShadow(inst, lv2, 0, 0, 10, 20, 30, 255, 2.0f, 2.0f) : 0;
        printf("P0-26 listview cell shadow set=%d\n", w1);
        if (w1 != 1) { printf("FAIL: listview cell 阴影\n"); pass = 0; }
        got = (UIColor){0, 0, 0, 0};
        int z3 = lv2 ? UICornerstone_SetColor(inst, lv2, "text.hover", c) : 0;
        int z4 = lv2 ? UICornerstone_GetColor(inst, lv2, "text.hover", &got) : 0;
        printf("P0-26 listview text.hover=%d/%d rgba=%d,%d,%d\n", z3, z4, got.r, got.g, got.b);
        if (z3 != 1 || z4 != 1 || got.r != 10 || got.g != 20 || got.b != 30) { printf("FAIL: listview 文本四态\n"); pass = 0; }
    }

    /* 12. P0-27d/P0-28：Popup/ConfirmPopup 工厂 + 编辑态常显键 */
    UIControlHandle pop = UICornerstone_CreatePopup(inst, 0, 500, 200, 100, 1.0f, 1.0f);
    int q1 = pop ? UICornerstone_SetBool(inst, pop, "close-on-click-outside", 0) : 0;
    int q2 = -1; if (pop) UICornerstone_GetBool(inst, pop, "close-on-click-outside", &q2);
    int q3 = pop ? UICornerstone_SetBool(inst, pop, "close-on-esc", 0) : 0;
    int q4 = pop ? UICornerstone_SetBool(inst, pop, "visible", 1) : 0;
    int q5 = 0; if (pop) UICornerstone_GetBool(inst, pop, "popup-visible", &q5);
    int q6 = pop ? UICornerstone_SetBool(inst, pop, "visible", 0) : 0;
    printf("P0-27d popup=%s coc=%d/%d esc=%d visible=%d/%d/%d\n",
           pop ? "OK" : "NULL", q1, q2, q3, q4, q5, q6);
    if (!pop || q1 != 1 || q2 != 0 || q3 != 1 || q4 != 1 || q5 != 1 || q6 != 1) {
        printf("FAIL: Popup 工厂/常显键\n"); pass = 0;
    }
    UIControlHandle cpop = UICornerstone_CreateConfirmPopup(inst, "OK", 0, 0, 240, 120, 1.0f, 1.0f);
    printf("P0-27d confirm-popup=%s\n", cpop ? "OK" : "NULL");
    if (!cpop) { printf("FAIL: ConfirmPopup 工厂\n"); pass = 0; }

    UICornerstone_DestroyInstance(inst);
    if (pass) { printf("=== PASS: test_p0_getter ===\n"); return 0; }
    printf("=== FAIL: test_p0_getter ===\n");
    return 1;
}