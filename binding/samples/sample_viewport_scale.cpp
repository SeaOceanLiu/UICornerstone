// UICornerstone C++ Binding — 双视口视图缩放演示样例（ViewportScale_Design 阶段 2）
// 视口 A（固定不可调整，420 宽控制面板）：
//   - "切换缩放模式"按钮：循环设置视口 B 的 mode off→fit→stretch
//   - 4 个滑块：分别控制视口 B 的 left / top / width / height
// 视口 B（位置与大小可调，基准画布 800×600）：
//   - Label1：实时显示当前视口 rect
//   - Label2：实时显示当前 mode 与复合缩放
//   - 图片按钮：点击弹出 Dialog（居中确认弹窗）
//   - TextArea：随意输入文字
//   - 动画按钮：点击弹出 WinFrame，内容显示 TextArea 中的文字
// 说明：双视口模型下 B 内控件布局固定于画布，fit/stretch/off 均由引擎
// 根变换处理，无需应用侧等比重排；拖动滑块即见视口位置/大小变化下
// 三种模式的实时适配（anchor 偏移、等比缩放、拉伸）。
// 运行：sample_viewport_scale [backend=<后端>] [auto=<秒>]
#include "UICornerstone.h"
#include "Control.h"
#include "Event.h"
#include "PropertyNames.h"
#include "auto_args.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>

static int s_pass = 0;
static int s_fail = 0;

static void check(const char* desc, bool cond) {
    if (cond) { s_pass++; std::printf(u8"[sample_vpscale] PASS %s\n", desc); }
    else      { s_fail++; std::printf(u8"[sample_vpscale] FAIL %s\n", desc); }
    std::fflush(stdout);
}

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-3f; }

static const char* kModeNames[3] = {u8"off（画布跟随窗口）", u8"fit（等比居中）", u8"stretch（拉伸铺满）"};

// 视口布局常量
static const float kPanelW = 420.0f;          // 视口 A 宽（固定）
static const UIRect kB0 = {440.0f, 40.0f, 800.0f, 600.0f};   // 视口 B 初始 rect
static const float kCanvasW = 800.0f;         // 视口 B 基准画布
static const float kCanvasH = 600.0f;

// 图片按钮三态图 / 动画资源
static const char* kImgNormal  = "assets/images/down.png";
static const char* kImgHover   = "assets/images/down_hover.png";
static const char* kImgPressed = "assets/images/down_pressed.png";
static const char* kAnimJsonc  = "assets/animations/rotateBtn/rotateBtn.jsonc";

int main(int argc, char* argv[]) {
    const char* backend = "sdl3";
    const char* extra[] = {"backend="};
    uicorn_sample::AutoTimer autoTimer;
    int autoSeconds = autoTimer.parse(argc, argv, extra, 1);
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "backend=", 8) == 0) backend = argv[i] + 8;
    }
    std::printf(u8"[sample_vpscale] autoSeconds=%d backend=%s\n", autoSeconds, backend);

    auto ui = UICornerstone::UICornerstone::Create(
        UICornerstone::UICornerstone::Config{}
            .WithBackend(backend)
            .WithWindowFlags(0x20)   // UIWindowFlags::Resizable —— 拖拽窗口实测视口变化
            .WithWindow(u8"双视口缩放演示：A 控制面板（固定）/ B 演示区（可调位置大小）", 1280, 800));
    if (!ui) {
        std::printf(u8"Create failed\n");
        return 1;
    }

    // ── 创建两个子视口：A 固定控制面板，B 可调演示区 ──
    auto vpA = ui->CreateViewport(0, 0, kPanelW, 800);
    auto vpB = ui->CreateViewport(kB0.x, kB0.y, kB0.w, kB0.h);
    if (!vpA || !vpB) {
        std::printf(u8"CreateViewport failed\n");
        return 1;
    }
    vpB->SetCanvasSize(kCanvasW, kCanvasH);   // B 基准画布（fit/stretch 适配基准）

    // ── 引擎断言（针对视口 B，与测试矩阵一致，不依赖视觉）──
    float sx = 0.0f, sy = 0.0f;
    check(u8"B 初始 off", vpB->GetViewportScaleMode() == 0);
    check(u8"B off 复合=1", vpB->GetViewportScale(sx, sy) && feq(sx, 1.0f) && feq(sy, 1.0f));
    check(u8"B SetCanvasSize(800,600)", vpB->GetViewportScale(sx, sy) && feq(sx, 1.0f));
    check(u8"B fit 设置成功", vpB->SetViewportScaleMode(1));
    check(u8"B fit 查询一致", vpB->GetViewportScaleMode() == 1);
    check(u8"B fit 复合=1(初始窗口=画布)", vpB->GetViewportScale(sx, sy) && feq(sx, 1.0f) && feq(sy, 1.0f));
    check(u8"B 改宽后 fit 缩放到 0.75", [&]() {
        vpB->SetViewport(kB0.x, kB0.y, 600, 600);
        return vpB->GetViewportScale(sx, sy) && feq(sx, 0.75f) && feq(sy, 0.75f);
    }());
    check(u8"B stretch 独立轴", vpB->SetViewportScaleMode(2) &&
          vpB->GetViewportScale(sx, sy) && feq(sx, 0.75f) && feq(sy, 1.0f));
    check(u8"B 回 off", vpB->SetViewportScaleMode(0));
    check(u8"B 非法 mode 拒绝", !vpB->SetViewportScaleMode(5));
    check(u8"B 非法画布拒绝", !vpB->SetCanvasSize(0, 0));
    check(u8"B 背景色设置成功", vpB->SetViewportBackgroundColor(30, 34, 42, 255));
    // 断言结束必须恢复运行时状态：viewport 视口 = 屏幕空间，不受模式/断言影响
    vpB->SetViewport(kB0.x, kB0.y, kB0.w, kB0.h);
    vpB->SetViewportScaleMode(0);
    std::printf(u8"[sample_vpscale] 引擎断言 check count: pass=%d fail=%d\n", s_pass, s_fail);
    bool ok = (s_fail == 0);

    // ── 视口 B 内容（布局固定于 800×600 画布，适配由引擎完成）──
    // 视口背景色：fit 模式下画布四周留白以此色填充（默认透明 = 仅透出外层 Clear 色）
    vpB->SetViewportBackgroundColor(30, 34, 42, 255);
    auto canvasB = vpB->CreatePanel(0, 0, kCanvasW, kCanvasH);
    canvasB.SetColor("background", UIColor{32, 48, 64, 255});
    canvasB.SetColor("border", UIColor{100, 220, 255, 255});
    canvasB.SetBool("border-visible", true);

    auto labelRect = vpB->CreateLabel(u8"视口 B rect: 440,40 800×600", 24.0f, 20, 18, 560, 36);
    labelRect.SetColor("text", UIColor{255, 255, 255, 255});
    auto labelMode = vpB->CreateLabel(u8"mode: off（画布跟随窗口）  scale: 1.000×1.000", 22.0f, 20, 62, 760, 34);
    labelMode.SetColor("text", UIColor{255, 220, 100, 255});

    // 图片按钮 → 弹出 Dialog
    auto imgBtn = vpB->CreateButton(u8"", 20, 120, 200, 90);   // P0-31：ImageButton 已移除 → 四态图 Button
    imgBtn.SetString(PropertyNames::kNormalImage, kImgNormal);
    imgBtn.SetString(PropertyNames::kHoverImage, kImgHover);
    imgBtn.SetString(PropertyNames::kPressedImage, kImgPressed);
    imgBtn.SetCallback(PropertyNames::kEventClick, [&](const Event&) {
        std::printf(u8"[sample_vpscale] 图片按钮点击 -> 弹出 Dialog\n");
        auto dialog = vpB->CreateDialog(u8"确定", "", 0, 0, 320, 140);   // 居中弹窗
        auto label = vpB->CreateLabel(u8"这是图片按钮触发的 Dialog\n基准画布上的控件布局不受视口缩放影响", 18.0f, 24, 26, 272, 70);
        dialog.AddChild(label);
    });

    // TextArea：随意输入文字
    auto textArea = vpB->CreateTextArea(240, 120, 540, 420);
    textArea.SetString(PropertyNames::kTextContent, u8"在这里随意输入文字……\n点击右侧动画按钮，将本内容显示在 WinFrame 中。");
    vpB->CreateLabel(u8"TextArea（可输入）", 18.0f, 240, 84, 200, 28);

    // 动画按钮 → 弹出 WinFrame 显示 TextArea 文字
    auto aniBtn = vpB->CreateAnimatedButton(kAnimJsonc, 20, 300, 200, 150);
    aniBtn.SetBool(PropertyNames::kPlaying, true);
    aniBtn.SetCallback(PropertyNames::kEventClick, [&](const Event&) {
        std::string content = textArea.GetString(PropertyNames::kTextContent);
        std::printf(u8"[sample_vpscale] 动画按钮点击 -> 弹出 WinFrame（%d 字节）\n", (int)content.size());
        auto wf = vpB->CreateWinFrame(u8"TextArea 内容", 80, 90, 620, 360);
        auto label = vpB->CreateLabel(content, 18.0f, 24, 44, 560, 280);
        wf.AddChild(label);
    });
    vpB->CreateLabel(u8"图片按钮→Dialog / 动画按钮→WinFrame", 16.0f, 20, 474, 300, 26);

    // ── 视口 A 内容（固定控制面板）──
    auto panelA = vpA->CreatePanel(0, 0, kPanelW, 800);
    panelA.SetColor("background", UIColor{24, 30, 40, 255});
    panelA.SetColor("border", UIColor{120, 140, 160, 255});
    panelA.SetBool("border-visible", true);

    vpA->CreateLabel(u8"视口 A（固定控制面板）", 24.0f, 24, 20, 380, 36);
    auto btnMode = vpA->CreateButton(u8"切换视口 B 模式（off→fit→stretch）", 24, 80, 372, 64);
    btnMode.SetColor("border", UIColor{100, 255, 180, 255});
    btnMode.SetBool("border-visible", true);
    auto aModeLabel = vpA->CreateLabel(u8"B mode: off", 20.0f, 24, 158, 380, 30);
    aModeLabel.SetColor("text", UIColor{255, 220, 100, 255});

    // 滑块控制 B 的 left/top/width/height
    struct SliderCfg {
        const char* name;
        float y;                       // 组首行 y（名称 label）
        float min, max, init;          // 滑块范围与初始值
    };
    static const SliderCfg kSliders[4] = {
        {u8"B.left",  220.0f, 420.0f, 1000.0f, 440.0f},
        {u8"B.top",   340.0f,   0.0f,  720.0f,  40.0f},
        {u8"B.width", 460.0f, 300.0f, 1000.0f, 800.0f},
        {u8"B.height",580.0f, 300.0f,  800.0f, 600.0f},
    };
    float bLeft = kB0.x, bTop = kB0.y, bW = kB0.w, bH = kB0.h;

    // B 状态回显（rect + mode/scale）：一律读真实视口，避免与滑块目标值脱节
    auto refreshB = [&]() {
        UIRect r = vpB->GetViewport();
        char buf[160];
        snprintf(buf, sizeof(buf), u8"视口 B rect: %d,%d %d×%d",
                 (int)r.x, (int)r.y, (int)r.w, (int)r.h);
        labelRect.SetString(PropertyNames::kCaption, buf);
        float csx = 0.0f, csy = 0.0f;
        vpB->GetViewportScale(csx, csy);
        int mode = vpB->GetViewportScaleMode();
        snprintf(buf, sizeof(buf), u8"mode: %s  scale: %.3f×%.3f", kModeNames[mode], csx, csy);
        labelMode.SetString(PropertyNames::kCaption, buf);
    };
    auto applyB = [&]() {
        vpB->SetViewport(bLeft, bTop, bW, bH);
        refreshB();
    };

    // 模式按钮：off(0) → fit(1) → stretch(2) → off(0)
    btnMode.SetCallback(PropertyNames::kEventClick, [&](const Event&) {
        int mode = (vpB->GetViewportScaleMode() + 1) % 3;
        vpB->SetViewportScaleMode(mode);
        char buf[64];
        snprintf(buf, sizeof(buf), u8"B mode: %s", kModeNames[mode]);
        aModeLabel.SetString(PropertyNames::kCaption, buf);
        refreshB();
        std::printf(u8"[sample_vpscale] B mode -> %s\n", kModeNames[mode]);
    });

    // 4 个滑块
    Control valueLabels[4];
    for (int i = 0; i < 4; i++) {
        const SliderCfg& sc = kSliders[i];
        vpA->CreateLabel(sc.name, 18.0f, 24, sc.y, 200, 26);
        valueLabels[i] = vpA->CreateLabel("", 16.0f, 250, sc.y, 140, 26);
        auto sl = vpA->CreateSlider(24, sc.y + 30, 372, 40, sc.min, sc.max, sc.init);
        auto* pv = &valueLabels[i];
        sl.SetCallback(PropertyNames::kEventValueChanged, [&, i, pv](const Event& e) {
            float v = e.GetValueChanged();
            char buf[32];
            snprintf(buf, sizeof(buf), u8"%d", (int)v);
            pv->SetString(PropertyNames::kCaption, buf);
            switch (i) {
            case 0: bLeft = v; break;
            case 1: bTop  = v; break;
            case 2: bW    = v; break;
            default: bH   = v; break;
            }
            applyB();
        });
        // 初始值回显
        char buf[32];
        snprintf(buf, sizeof(buf), u8"%d", (int)sc.init);
        valueLabels[i].SetString(PropertyNames::kCaption, buf);
    }

    std::printf(u8"[sample_vpscale] 双视口创建完毕：A=控制面板(420px) B=可调演示区(画布800×600)\n");
    std::fflush(stdout);

    // ── 主循环：多视口必须显式驱动（ProcessEvents 按坐标路由到各视口）──
    using Clock = std::chrono::steady_clock;
    auto last = Clock::now();
    int frame = 0;
    while (!ui->IsQuitRequested()) {
        ui->ProcessEvents();        // 窗口输入轮询（按坐标路由到子视口）+ owner 注入队列
        vpA->ProcessEvents();
        vpB->ProcessEvents();

        if (autoTimer.expired()) {
            std::printf(u8"[sample_vpscale] %s auto=%d 秒渲染完成后自动退出\n",
                        ok ? u8"PASS" : u8"FAIL", autoSeconds);
            break;
        }

        // auto 模式：每 2.5 秒自动切换一次 B 的 mode（人工模式用按钮切换）
        if (autoSeconds > 0 && frame % 150 == 0 && frame > 0) {
            int mode = (vpB->GetViewportScaleMode() + 1) % 3;
            vpB->SetViewportScaleMode(mode);
            char buf[64];
            snprintf(buf, sizeof(buf), u8"B mode: %s", kModeNames[mode]);
            aModeLabel.SetString(PropertyNames::kCaption, buf);
            refreshB();
            std::printf(u8"[sample_vpscale] auto -> B mode %s\n", kModeNames[mode]);
        }

        auto now = Clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;
        if (dt > 0.1) dt = 0.1;

        vpA->Update(dt);
        vpB->Update(dt);
        ui->Clear();
        vpA->Render();              // 各自 clip 到自身视口区域
        vpB->Render();
        ui->Present();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ++frame;
    }
    ui->Shutdown();
    return ok ? 0 : 2;
}
