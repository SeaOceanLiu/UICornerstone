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
typedef int        (*UISetColorFn)(UIInstance, void*, const char*, UIColor);
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
static UISetColorFn             uiSetColor                  = nullptr;
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
    RESOLVE(SetColor)
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