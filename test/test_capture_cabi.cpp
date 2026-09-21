// test_capture_cabi.cpp — 截图 API（Capture_*）测试：像素级断言 + 人工落盘 BMP
// 遵循测试用例规范：纯 DLL 动态加载（LoadLibrary + GetProcAddress）。
// 布局：视口背景深红（200,30,30）+ 蓝色 Panel（60,60,120,80,背景 30,30,200）。
// auto 模式断言（每帧 Render 后、Present 前）：
//   1. CaptureViewport 尺寸 1024x768，中心/角落 == 背景红
//   2. CaptureControl(panel) 尺寸 120x80，中心/角落 == 蓝
//   3. CaptureRect 部分越界裁剪（(-20,-20,100,100) → 80x80，首像素红）
//   4. CaptureRect 完全出视口 → 0；outPixels 空 → 0
//   5. CaptureBench 尺寸 1024x768，中心 == 背景红
//   6. 落盘：CaptureControl → SavePixelsToFile("capture_ctl.bmp") →
//      读回校验 BMP 头（120x80, 32 位）与全像素蓝（B=200,G=30,R=30）
// 人工模式（auto=0）：窗口驻留，首次渲染后保存 capture_manual.bmp 供与窗口对照。
#include "UICornerstoneAPI.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
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
typedef int        (*UISetViewportBgFn)(UIInstance, uint8_t, uint8_t, uint8_t, uint8_t);
typedef void*      (*UICreatePanelFn)(UIInstance, float, float, float, float, float, float);
typedef void*      (*UICreateImageFn)(UIInstance, const char*, float, float, float, float, float, float);
typedef int        (*UISetEnumFn)(UIInstance, void*, const char*, const char*);
typedef int        (*UIActorSetSourceRectFn)(UIInstance, void*, float, float, float, float);
typedef int        (*UISetColorFn)(UIInstance, void*, const char*, UIColor);
typedef void*      (*UICreateEditBoxFn)(UIInstance, float, float, float, float, float, float);
typedef void       (*UIAddChildFn)(UIInstance, void*, void*);
typedef int        (*UISetBoolFn)(UIInstance, void*, const char*, int);
typedef int        (*UISetStringFn)(UIInstance, void*, const char*, const char*);
typedef uint32_t   (*UIGetBackendCapsFn)(UIInstance);
typedef int        (*UICaptureRectFn)(UIInstance, float, float, float, float, uint8_t*, int*, int*);
typedef int        (*UICaptureViewportFn)(UIInstance, uint8_t*, int*, int*);
typedef int        (*UICaptureBenchFn)(UIInstance, uint8_t*, int*, int*);
typedef int        (*UICaptureControlFn)(UIInstance, void*, uint8_t*, int*, int*);
typedef int        (*UISavePixelsFn)(const uint8_t*, int, int, const char*);

static UIPluginCreateInstanceFn uiCreateInstanceFromPlugin  = nullptr;
static UIDestroyInstanceFn      uiDestroyInstance           = nullptr;
static UIProcessEventsFn        uiProcessEvents             = nullptr;
static UIUpdateFn               uiUpdate                    = nullptr;
static UIClearFn                uiClear                     = nullptr;
static UIRenderFn               uiRender                    = nullptr;
static UIPresentFn              uiPresent                   = nullptr;
static UIIsQuitRequestedFn      uiIsQuitRequested           = nullptr;
static UISetViewportBgFn        uiSetViewportBackgroundColor= nullptr;
static UICreatePanelFn          uiCreatePanel               = nullptr;
static UICreateImageFn          uiCreateImage               = nullptr;
static UISetEnumFn              uiSetEnum                   = nullptr;
static UIActorSetSourceRectFn   uiActorSetSourceRect        = nullptr;
static UISetColorFn             uiSetColor                  = nullptr;
static UICreateEditBoxFn        uiCreateEditBox             = nullptr;
static UIAddChildFn             uiAddChildControl           = nullptr;
static UISetBoolFn              uiSetBool                   = nullptr;
static UISetStringFn            uiSetString                 = nullptr;
static UIGetBackendCapsFn       uiGetBackendCapabilities    = nullptr;
static UICaptureRectFn          uiCaptureRect               = nullptr;
static UICaptureViewportFn      uiCaptureViewport           = nullptr;
static UICaptureBenchFn         uiCaptureBench              = nullptr;
static UICaptureControlFn       uiCaptureControl            = nullptr;
static UISavePixelsFn           uiSavePixelsToFile          = nullptr;

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
    RESOLVE(SetViewportBackgroundColor)
    RESOLVE(CreatePanel)
    RESOLVE(CreateImage)
    RESOLVE(SetEnum)
    RESOLVE(ActorSetSourceRect)
    RESOLVE(SetColor)
    RESOLVE(CreateEditBox)
    RESOLVE(AddChildControl)
    RESOLVE(SetBool)
    RESOLVE(SetString)
    RESOLVE(GetBackendCapabilities)
    RESOLVE(CaptureRect)
    RESOLVE(CaptureViewport)
    RESOLVE(CaptureBench)
    RESOLVE(CaptureControl)
    RESOLVE(SavePixelsToFile)
#undef RESOLVE
    return true;
}

// ===== 像素断言辅助 =====
static bool pxEq(const uint8_t* p, uint8_t r, uint8_t g, uint8_t b) {
    return p[0] == r && p[1] == g && p[2] == b;
}

static bool readBmpPixelsAllEq(const char* path, int expectW, int expectH,
                               uint8_t b, uint8_t g, uint8_t r) {
    FILE* fp = fopen(path, "rb");
    if (!fp) { printf("  FAIL: cannot open %s\n", path); return false; }
    uint8_t hdr[54];
    if (fread(hdr, 1, 54, fp) != 54) { fclose(fp); printf("  FAIL: %s 头不足 54 字节\n", path); return false; }
    int w = hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24);
    int h = hdr[22] | (hdr[23] << 8) | (hdr[24] << 16) | (hdr[25] << 24);
    int bits = hdr[28] | (hdr[29] << 8);
    if (hdr[0] != 'B' || hdr[1] != 'M' || w != expectW || h != expectH || bits != 32) {
        printf("  FAIL: %s 头不符 (w=%d h=%d bits=%d, 期望 %dx%d@32)\n", path, w, h, bits, expectW, expectH);
        fclose(fp); return false;
    }
    int rowSize = w * 4;
    std::vector<uint8_t> row(static_cast<size_t>(rowSize));
    bool ok = true;
    for (int y = 0; y < h && ok; ++y) {           // bottom-up：文件行 0 = 图像最后一行
        if (fread(row.data(), 1, static_cast<size_t>(rowSize), fp) != static_cast<size_t>(rowSize)) {
            printf("  FAIL: %s 像素区截断\n", path); ok = false; break;
        }
        for (int x = 0; x < w; ++x) {
            if (row[x * 4 + 0] != b || row[x * 4 + 1] != g || row[x * 4 + 2] != r) {
                printf("  FAIL: %s 像素(%d,%d)=%02X%02X%02X 期望 BGR %02X%02X%02X\n",
                       path, x, y, row[x * 4 + 2], row[x * 4 + 1], row[x * 4 + 0], r, g, b);
                ok = false; break;
            }
        }
    }
    fclose(fp);
    return ok;
}

// ===== 布局与背景色 =====
// 三层模型：视口背景红（200,30,30）先填充，随后 bench 根容器背景（ConstDef::DEFAULT_NORMAL_COLOR
// = 23,23,24，默认铺满视口）覆盖，故整窗可见色为根容器背景；Panel 蓝色（30,30,200）在控件层。
static const uint8_t kBgR = 23, kBgG = 23, kBgB = 24;        // bench 根容器背景（覆盖视口红）
static const uint8_t kPnlR = 30, kPnlG = 30, kPnlB = 200;    // Panel 蓝

int main(int argc, char** argv) {
    DISABLE_ASSERT_DIALOG();
    int autoSec = 0;
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "auto=", 5) == 0) autoSec = atoi(argv[i] + 5);
    }
    printf("模式: %s\n", autoSec ? "auto（自动断言）" : "人工（窗口驻留，对照 capture_manual.bmp）");

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary(UICornerstone.dll)\n"); return 1; }
    if (!loadAllProcs()) { FreeLibrary(g_uiDll); return 1; }

    UIInstance inst = uiCreateInstanceFromPlugin(UICORNERSTONE_BACKEND_NAME, NULL);
    assert(inst);
    assert(uiSetViewportBackgroundColor(inst, kBgR, kBgG, kBgB, 255) == 1);

    void* panel = uiCreatePanel(inst, 60.0f, 60.0f, 120.0f, 80.0f, 1.0f, 1.0f);
    assert(panel);
    assert(uiSetColor(inst, panel, "background", UIColor{kPnlR, kPnlG, kPnlB, 255}) == 1);

    // I0 用例对象（渲染循环前创建，首帧即可被绘制/捕获）
    // 纹理 srcrect_split.bmp（24x24：顶部 2 行绿、其余蓝），目标 240x24（10:1）。
    // center-crop：scale=max(10,1)=10 → srcRect=(0,10.8,24,2.4) 取垂直居中蓝带。
    // 顶缘应蓝（srcRect 生效）；旧 SDL3 忽略 srcRect=整图拉伸 → 顶缘露纹理绿行。
    static void* i0Img = uiCreateImage(inst, "assets/images/srcrect_split.bmp",
                                       60.0f, 200.0f, 240.0f, 24.0f, 1.0f, 1.0f);
    assert(i0Img);
    assert(uiSetEnum(inst, i0Img, "scale-type", "center-crop") == 1);
    static void* i0Disp = uiCreateImage(inst, "assets/images/srcrect_split.bmp",
                                        320.0f, 200.0f, 240.0f, 24.0f, 1.0f, 1.0f);
    assert(i0Disp);
    assert(uiSetEnum(inst, i0Disp, "scale-type", "center-crop") == 1);
    // I0b：tile 平铺（纹理整图含顶部绿行 → 目标顶缘=绿）
    static void* i0bTile = uiCreateImage(inst, "assets/images/srcrect_split.bmp",
                                         60.0f, 240.0f, 240.0f, 24.0f, 1.0f, 1.0f);
    assert(i0bTile);
    assert(uiSetEnum(inst, i0bTile, "scale-type", "tile") == 1);
    // I0c：tile + source-rect（瓦片=垂直居中蓝带 y10..12 → 目标顶缘=蓝）
    static void* i0cTileSrc = uiCreateImage(inst, "assets/images/srcrect_split.bmp",
                                            320.0f, 240.0f, 240.0f, 24.0f, 1.0f, 1.0f);
    assert(i0cTileSrc);
    assert(uiSetEnum(inst, i0cTileSrc, "scale-type", "tile") == 1);
    // 瓦片 = source-rect 子区域（y 10..12 蓝带）→ 目标顶缘应为蓝
    assert(uiActorSetSourceRect(inst, i0cTileSrc, 0.f, 10.f, 24.f, 2.f) == 1);

    // ── CLIP 用例（P0-11）：clip-children 容器 + 越界子控件（自内容裁剪路径） ──
    // 容器 (60,520,200,100)；子 EditBox 局部 y=-30（上缘越出）与 y=-70（完全越出）
    static void* clipPanel = uiCreatePanel(inst, 60.0f, 520.0f, 200.0f, 100.0f, 1.0f, 1.0f);
    assert(clipPanel);
    assert(uiSetBool(inst, clipPanel, "clip-children", 1) == 1);
    static void* clipEbPart = uiCreateEditBox(inst, 5.0f, -15.0f, 180.0f, 30.0f, 1.0f, 1.0f);
    assert(clipEbPart);
    uiAddChildControl(inst, clipPanel, clipEbPart);          // 局部 (5,-15) → 屏幕 (65,505,180,30)：上缘越出 15px（下半可见）
    uiSetString(inst, clipEbPart, "text", "OVERFLOW");
    static void* clipEbFull = uiCreateEditBox(inst, 5.0f, -70.0f, 180.0f, 30.0f, 1.0f, 1.0f);
    assert(clipEbFull);
    uiAddChildControl(inst, clipPanel, clipEbFull);          // 局部 (5,-70) → 屏幕 (65,450,180,30)：完全越出
    uiSetString(inst, clipEbFull, "text", "FULLOUT");
    // 嵌套容器：outer(300,520,200,100) clip；inner 局部 (50,80,100,60) → 屏幕 (350,600,100,60) 下缘越出 outer
    static void* clipOuter = uiCreatePanel(inst, 300.0f, 520.0f, 200.0f, 100.0f, 1.0f, 1.0f);
    assert(clipOuter);
    assert(uiSetBool(inst, clipOuter, "clip-children", 1) == 1);
    static void* clipInner = uiCreatePanel(inst, 50.0f, 80.0f, 100.0f, 60.0f, 1.0f, 1.0f);
    assert(clipInner);
    assert(uiSetBool(inst, clipInner, "clip-children", 1) == 1);
    static void* clipEbNested = uiCreateEditBox(inst, 5.0f, 5.0f, 90.0f, 50.0f, 1.0f, 1.0f);
    assert(clipEbNested);
    uiAddChildControl(inst, clipInner, clipEbNested);
    uiSetString(inst, clipEbNested, "text", "NESTED");
    uiAddChildControl(inst, clipOuter, clipInner);           // inner 下缘越出 outer（600+60=660 > 620）

    uint32_t caps = uiGetBackendCapabilities(inst);
    printf("backend capabilities: 0x%08X (READBACK=%s)\n", caps,
           (caps & UICORN_BACKEND_CAP_READBACK) ? "yes" : "no");

    static uint8_t vpPixels[1024 * 768 * 4];
    static uint8_t ctlPixels[120 * 80 * 4];
    static uint8_t rectPixels[1024 * 768 * 4];

    bool savedManual = false;
    ULONGLONG t0 = GetTickCount64();
    bool allPass = true;
    while (!uiIsQuitRequested(inst)) {
        uiProcessEvents(inst);
        if (uiIsQuitRequested(inst)) break;
        uiClear(inst);
        uiRender(inst);

        // ── Render 后、Present 前：帧内读回 ──
        if (autoSec > 0) {
            int w = 0, h = 0;
            // 1. CaptureViewport
            assert(uiCaptureViewport(inst, vpPixels, &w, &h) == 1);
            assert(w == 1024 && h == 768);
            assert(pxEq(vpPixels + (0 * 1024 + 0) * 4, kBgR, kBgG, kBgB));            // 左上角
            assert(pxEq(vpPixels + (384 * 1024 + 512) * 4, kBgR, kBgG, kBgB));        // 中心
            // 2. CaptureControl(panel)
            assert(uiCaptureControl(inst, panel, ctlPixels, &w, &h) == 1);
            assert(w == 120 && h == 80);
            assert(pxEq(ctlPixels + (0 * 120 + 0) * 4, kPnlR, kPnlG, kPnlB));         // 左上角
            assert(pxEq(ctlPixels + (40 * 120 + 60) * 4, kPnlR, kPnlG, kPnlB));       // 中心
            // 3. CaptureRect 部分越界 → 裁剪
            assert(uiCaptureRect(inst, -20, -20, 100, 100, rectPixels, &w, &h) == 1);
            assert(w == 80 && h == 80);
            assert(pxEq(rectPixels + 0, kBgR, kBgG, kBgB));                           // 裁剪后原点=视口(0,0)
            // 4. CaptureRect 完全出视口 → 0
            assert(uiCaptureRect(inst, 5000, 5000, 10, 10, rectPixels, &w, &h) == 0);
            // 4b. outPixels 空 → 0
            assert(uiCaptureRect(inst, 0, 0, 10, 10, NULL, &w, &h) == 0);
            // 5. CaptureBench
            assert(uiCaptureBench(inst, vpPixels, &w, &h) == 1);
            assert(w == 1024 && h == 768);
            assert(pxEq(vpPixels + (384 * 1024 + 512) * 4, kBgR, kBgG, kBgB));        // 中心（无控件）
            // 6. 落盘：CaptureControl → BMP → 读回全像素蓝
            assert(uiCaptureControl(inst, panel, ctlPixels, &w, &h) == 1);
            assert(uiSavePixelsToFile(ctlPixels, w, h, "capture_ctl.bmp") == 1);
            allPass = readBmpPixelsAllEq("capture_ctl.bmp", w, h, kPnlB, kPnlG, kPnlR) && allPass;

            // 7. I0：SDL3 drawTexture srcRect 恢复——center-crop 像素回归
            static bool i0Checked = false;
            if (!i0Checked) {
                i0Checked = true;
                int iw = 0, ih = 0;
                assert(uiCaptureControl(inst, i0Img, ctlPixels, &iw, &ih) == 1);
                assert(iw == 240 && ih == 24);
                // 顶缘中心像素：crop=蓝(0,0,255)；拉伸=绿(0,255,0)/背景灰(23,23,24)
                if (pxEq(ctlPixels + (0 * 240 + 120) * 4, 0, 0, 255)) {
                    printf("PASS: I0 center-crop top edge is blue (srcRect honored)\n");
                } else {
                    printf("FAIL: I0 center-crop top edge not blue (srcRect ignored?)\n");
                    allPass = false;
                }

                // I0b：tile 平铺——每瓦片含纹理顶部绿行 → 目标顶缘=绿(0,255,0)
                int bw = 0, bh = 0;
                assert(uiCaptureControl(inst, i0bTile, ctlPixels, &bw, &bh) == 1);
                assert(bw == 240 && bh == 24);
                if (pxEq(ctlPixels + (0 * 240 + 120) * 4, 0, 255, 0)) {
                    printf("PASS: I0b tile top edge is green (tiled full texture)\n");
                } else {
                    printf("FAIL: I0b tile top edge not green\n");
                    allPass = false;
                }

                // I0c：tile + source-rect——瓦片=纯蓝带 → 目标顶缘=蓝(0,0,255)
                assert(uiCaptureControl(inst, i0cTileSrc, ctlPixels, &bw, &bh) == 1);
                if (pxEq(ctlPixels + (0 * 240 + 120) * 4, 0, 0, 255)) {
                    printf("PASS: I0c tile+source-rect top edge is blue (sub-region tiled)\n");
                } else {
                    printf("FAIL: I0c tile+source-rect top edge not blue\n");
                    allPass = false;
                }
            }

            // ── CLIP 断言（一次性，P0-11）：容器外区域无泄漏内容 ──
            static bool clipChecked = false;
            if (!clipChecked) {
                clipChecked = true;
                int cw = 0, ch = 0;
                bool clipOk = true;
                // 1) 部分越出行：容器上方 45px 带（60,470,200,45）应全为背景（修复前 EditBox 自裁剪替换容器裁剪 → 文本/背景泄漏）
                assert(uiCaptureRect(inst, 60, 470, 200, 45, rectPixels, &cw, &ch) == 1);
                for (int yy = 0; yy < ch && clipOk; ++yy)
                    for (int xx = 0; xx < cw; ++xx)
                        if (!pxEq(rectPixels + (yy * cw + xx) * 4, kBgR, kBgG, kBgB)) { clipOk = false; break; }
                if (clipOk) printf("PASS: clip part-overflow outside clean\n");
                else { printf("FAIL: clip part-overflow leaked above container\n"); allPass = false; }
                // 2) 完全越出行：更上方 40px 带（60,450,200,40）同样干净
                bool clipOk2 = true;
                assert(uiCaptureRect(inst, 60, 450, 200, 40, rectPixels, &cw, &ch) == 1);
                for (int yy = 0; yy < ch && clipOk2; ++yy)
                    for (int xx = 0; xx < cw; ++xx)
                        if (!pxEq(rectPixels + (yy * cw + xx) * 4, kBgR, kBgG, kBgB)) { clipOk2 = false; break; }
                if (clipOk2) printf("PASS: clip full-overflow outside clean\n");
                else { printf("FAIL: clip full-overflow leaked\n"); allPass = false; }
                // 3) 嵌套容器：outer 下缘外 40px 带（350,620,100,40）干净（inner 内容不得越 outer）
                bool clipOk3 = true;
                assert(uiCaptureRect(inst, 350, 620, 100, 40, rectPixels, &cw, &ch) == 1);
                for (int yy = 0; yy < ch && clipOk3; ++yy)
                    for (int xx = 0; xx < cw; ++xx)
                        if (!pxEq(rectPixels + (yy * cw + xx) * 4, kBgR, kBgG, kBgB)) { clipOk3 = false; break; }
                if (clipOk3) printf("PASS: clip nested inner not escaping outer\n");
                else { printf("FAIL: clip nested inner escaped outer\n"); allPass = false; }
                // 4) 容器内可见部分应存在内容（EditBox 文本/背景）——诊断/回归：避免"全裁掉"或"空场景"误判
                {
                    bool insideInk = false;
                    assert(uiCaptureRect(inst, 65, 522, 170, 16, rectPixels, &cw, &ch) == 1);
                    for (int yy = 0; yy < ch && !insideInk; ++yy)
                        for (int xx = 0; xx < cw; ++xx)
                            if (!pxEq(rectPixels + (yy * cw + xx) * 4, kBgR, kBgG, kBgB)) { insideInk = true; break; }
                    if (insideInk) printf("PASS: clip container interior has content\n");
                    else { printf("FAIL: clip container interior empty (test scene draws nothing)\n"); allPass = false; }
                }
            }
        } else {
            // 人工模式：首次渲染后落盘整窗截图供对照
            if (!savedManual) {
                int w = 0, h = 0;
                if (uiCaptureViewport(inst, vpPixels, &w, &h) == 1 &&
                    uiSavePixelsToFile(vpPixels, w, h, "capture_manual.bmp") == 1) {
                    printf("已保存 capture_manual.bmp（%dx%d，与窗口内容对照；关闭窗口退出）\n", w, h);
                } else {
                    printf("FAIL: 人工模式截图失败\n");
                    allPass = false;
                }
                savedManual = true;
            }
        }

        uiPresent(inst);
        uiUpdate(inst, 0.016);
        if (autoSec && GetTickCount64() - t0 >= (ULONGLONG)autoSec * 1000) break;
    }

    printf("窗口已关闭（%s）\n", autoSec ? "自动超时" : "人工");
    if (autoSec > 0 && allPass) printf("PASS: 全部截图断言通过\n");
    if (autoSec > 0 && !allPass) printf("FAIL: 存在失败断言\n");
    uiDestroyInstance(inst);
    FreeLibrary(g_uiDll);
    return (autoSec > 0 && !allPass) ? 1 : 0;
}