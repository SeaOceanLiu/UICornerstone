// =========================================================================
// test_viewport_scale.cpp -- 视口缩放核心链路测试（ViewportScale_Design §6）
//
// 覆盖矩阵（无头断言，auto=3 自动退出）：
//   T0  off 初始兼容：不设任何视图状态 → 根变换退化为现状（DR == rect）
//   T11 深层复合链：A(1)→B(2)→C(3)，bench scale=2 → 末代复合=12；
//        Button 非树成员（状态 Actor / 内嵌 LuotiAni）同步（=4）
//   T1  fit 缩（窗口 1024x768 < 画布 1600x1080）：sx=0.64 居中 anchor=(0,38.4)
//   T2  stretch：sx=0.64, sy=0.7111… 铺满，anchor=(0,0)
//   T3  off 还原 + SetViewportAnchor 增量叠加
//   T4  fit 子视口嵌入（viewport 带偏移 {100,50,800,600}）：anchor 含视口偏移
//   T5  fit + resize 联动：viewport 更新后 recompute 生效
// 既定语义（设计 §3.2）：根变换承载尺寸缩放与基线偏移；子级 drawRect
// 位置保持画布像素不缩放，仅尺寸乘复合（fast-path，改动全链为后续待办）。
// =========================================================================

#include <cstdio>
#include <cmath>
#include <memory>
#include "Button.h"
#include "UICornerstoneAPI.h"
#include "Actor.h"
#include "Panel.h"
#include "Dialog.h"
#include "TextArea.h"
#include "EditBox.h"
#include "Label.h"
#include "TreeView.h"
#include "Slider.h"
#include "Menu.h"
#include "Font.h"
#include "LuotiAni.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestInstance.h"

using namespace std;

static int s_pass = 0;
static int s_fail = 0;

static void check(const char* desc, bool cond) {
    if (cond) { s_pass++; printf(u8"[viewport-scale] PASS %s\n", desc); }
    else      { s_fail++; printf(u8"[viewport-scale] FAIL %s\n", desc); }
    fflush(stdout);
}

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
static bool seq(SRect a, SRect b) {
    return feq(a.left, b.left) && feq(a.top, b.top) &&
           feq(a.width, b.width) && feq(a.height, b.height);
}

static shared_ptr<Panel> g_panelA;
static shared_ptr<Button> g_buttonB;
static shared_ptr<Panel> g_panelC;
static shared_ptr<Actor> g_actor;
static shared_ptr<LuotiAni> g_ani;

static void runChecks() {
    Bench* bench = BENCH;
    int mode = -1;
    if (bench == nullptr) { printf("[viewport-scale] BENCH null\n"); return; }

    // ── T0：off 初始逐像素兼容 ──
    check("T0 bench 复合=1", feq(bench->getScaleXX(), 1.0f) && feq(bench->getScaleYY(), 1.0f));
    SRect dr0 = bench->getDrawRect();
    check("T0 根变换退化 == rect",
          seq(dr0, SRect{0, 0, 1024.0f, 768.0f}));
    // 创建时 bench->resized(viewport) → rect=1024x768
    check("T0 bench rect 初始化", seq(bench->getRect(), SRect{0, 0, (float)1024, (float)768}));
    check("T0 A 复合=1, B=2, C=6",
          feq(g_panelA->getScaleXX(), 1.0f) &&
          feq(g_buttonB->getScaleXX(), 2.0f) &&
          feq(g_panelC->getScaleXX(), 6.0f));

    // ── T11：深层链 + 非树成员收口（off 下手动 setScaleX）──
    bench->setScaleX(2.0f);
    check("T11 bench.xx=2", feq(bench->getScaleXX(), 2.0f));
    check("T11 A=2, B=4, C=12", feq(g_panelA->getScaleXX(), 2.0f) &&
          feq(g_buttonB->getScaleXX(), 4.0f) && feq(g_panelC->getScaleXX(), 12.0f));
    check("T11 Actor=4", feq(g_actor->getScaleXX(), 4.0f));
    check("T11 LuotiAni=4", feq(g_ani->getScaleXX(), 4.0f));
    check("T11 根 DR 放大",
          seq(bench->getDrawRect(), SRect{0, 0, 2048.0f, 768.0f}));
    if (!seq(bench->getDrawRect(), SRect{0, 0, 2048.0f, 768.0f})) {
        SRect d = bench->getDrawRect();
        printf(u8"[viewport-scale]   T11 debug DR=(%.3f,%.3f,%.3f,%.3f) xx=%.3f yy=%.3f rect=(%.0f,%.0f,%.0f,%.0f)\n",
               d.left, d.top, d.width, d.height,
               bench->getScaleXX(), bench->getScaleYY(),
               bench->getRect().left, bench->getRect().top,
               bench->getRect().width, bench->getRect().height);
        fflush(stdout);
    }
    check("T11 Y 向复合=2", feq(bench->getScaleYY(), 1.0f));
    bench->setScaleX(1.0f);
    check("T11 还原后 B=2", feq(g_buttonB->getScaleXX(), 2.0f));

    // 画布 1600x1080（fit/stretch 阶段，rect 视为基准画布）
    bench->setRect(SRect{0, 0, 1600.0f, 1080.0f});

    // ── T1：fit（1024x768 视口 vs 1600x1080 画布）──
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Fit);
    check("T1 fit sx=0.64", feq(bench->getScaleXX(), 0.64f));
    SRect drFit = bench->getDrawRect();
    check("T1 fit 根 DR(0,38.4,1024,691.2)", seq(drFit, SRect{0, 38.4f, 1024.0f, 691.2f}));
    SRect da = g_panelA->getDrawRect();
    check("T1 A DR=(0,38.4,128,96)", seq(da, SRect{0, 38.4f, 128.0f, 96.0f}));
    SRect db = g_buttonB->getDrawRect();
    // 子级相对位置乘父复合（getDrawRect 全链等比：10*0.64=6.4, 10*0.64+38.4=44.8）
    check("T1 B DR=(6.4,44.8,128,64)", seq(db, SRect{6.4f, 44.8f, 128.0f, 64.0f}));
    if (!seq(db, SRect{6.4f, 44.8f, 128.0f, 64.0f})) {
        printf(u8"[viewport-scale]   T1 B debug DR=(%.3f,%.3f,%.3f,%.3f) Bxx=%.3f Byy=%.3f Axx=%.3f\n",
               db.left, db.top, db.width, db.height,
               g_buttonB->getScaleXX(), g_buttonB->getScaleYY(),
               g_panelA->getScaleXX());
        fflush(stdout);
    }
    check("T1 C=3.84", feq(g_panelC->getScaleXX(), 3.84f));

    // ── T2：stretch ──
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Stretch);
    check("T2 stretch sx=0.64, sy=0.7111", feq(bench->getScaleXX(), 0.64f) &&
          feq(bench->getScaleYY(), 768.0f / 1080.0f));
    check("T2 stretch 根 DR 铺满(0,0,1024,768)",
          seq(bench->getDrawRect(), SRect{0, 0, 1024.0f, 768.0f}));

    // ── T3：off 还原 + 锚点增量 ──
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Off);
    check("T3 off 还原 scale=1", feq(bench->getScaleXX(), 1.0f) && feq(bench->getScaleYY(), 1.0f));
    check("T3 off 根 DR==画布", seq(bench->getDrawRect(), SRect{0, 0, 1600.0f, 1080.0f}));
    bench->setViewportAnchor(10.0f, 20.0f);
    check("T3 anchor 增量叠加", seq(bench->getDrawRect(), SRect{10, 20, 1600.0f, 1080.0f}));
    bench->setViewportAnchor(-10.0f, -20.0f);

    // ── T4：fit 子视口嵌入（viewport 偏移）──
    g_uiInstance->viewport = SRect{100, 50, 800, 600};
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Fit);
    check("T4 fit sx=0.5", feq(bench->getScaleXX(), 0.5f));
    check("T4 fit 子视口 DR=(100,80,800,540)",
          seq(bench->getDrawRect(), SRect{100, 80, 800.0f, 540.0f}));

    // ── T5：fit + resize 联动（视口先更新再 resized）──
    g_uiInstance->viewport = SRect{0, 0, 800, 600};
    bench->resized(SRect{0, 0, 800.0f, 600.0f});
    check("T5 resize 后 recompute DR=(0,30,800,540)",
          seq(bench->getDrawRect(), SRect{0, 30, 800.0f, 540.0f}));

    // ── 还原环境：视图状态 off、视口复原、scale=1 ──
    g_uiInstance->viewport = SRect{0, 0, 1024, 768};
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Off);
    bench->setScaleX(1.0f);
    bench->setScaleY(1.0f);

    // ── T6 弹层契约（fit 下随根变换缩放 + 视口居中反查，§ViewportScale_Design C1）──
    bench->setRect(SRect{0, 0, 1600.0f, 1080.0f});
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Fit);
    auto popup = make_shared<Popup>(nullptr, SRect(0, 0, 300, 200));
    popup->setCentered();
    popup->open();
    // fit(1024x768 vs 1600x1080): sx=0.64, anchor=(0,38.4)
    // 弹层复合 = 布局(1) × 根变换(0.64) = 0.64，屏幕宽 = 300*0.64 = 192
    // 反查本地坐标: left=(0+(1024-192)/2-0)/0.64=650, top=(0+(768-128)/2-38.4)/0.64=440
    // 屏幕 DR = 本地*0.64 + anchor = (416, 320, 192, 128) = 视口正中心（弹层随画布缩放）
    SRect pd = popup->getDrawRect();
    check("T6 弹层复合随根变换（布局×根复合=0.64）",
          feq(popup->getScaleXX(), 0.64f) && feq(popup->getScaleYY(), 0.64f));
    check("T6 弹层视口居中 + 尺寸随画布缩放",
          seq(pd, SRect{416.0f, 320.0f, 192.0f, 128.0f}));
    if (!seq(pd, SRect{416.0f, 320.0f, 192.0f, 128.0f})) {
        printf(u8"[viewport-scale]   T6 debug DR=(%.3f,%.3f,%.3f,%.3f) xx=%.3f benchDR=(%.3f,%.3f)\n",
               pd.left, pd.top, pd.width, pd.height,
               popup->getScaleXX(),
               bench->getDrawRect().left, bench->getDrawRect().top);
        fflush(stdout);
    }
    popup->close();
    bench->setViewportScaleMode(Bench::ViewportScaleMode::Off);
    bench->setScaleX(1.0f);
    bench->setScaleY(1.0f);

    // ── T8：C ABI 接口（SetViewportScaleMode/GetViewportScaleMode/SetCanvasSize/
    //        GetViewportScale/SetViewportAnchor + 参数校验）──
    // 当前状态：off、scale=1、bench rect=1600x1080（T6 设置）、canvas=0
    check("T8 初始 off", UICornerstone_GetViewportScaleMode(g_uiInstance, &mode) == 1 && mode == 0);
    check("T8 设 fit 成功", UICornerstone_SetViewportScaleMode(g_uiInstance, 1) == 1);
    UICornerstone_GetViewportScaleMode(g_uiInstance, &mode);
    check("T8 fit 查询一致", mode == 1);
    {
        float sx = 0, sy = 0;
        UICornerstone_GetViewportScale(g_uiInstance, &sx, &sy);
        check("T8 fit 复合=0.64（画布=bench rect）", feq(sx, 0.64f) && feq(sy, 0.64f));
    }
    check("T8 SetCanvasSize(1280,720)", UICornerstone_SetCanvasSize(g_uiInstance, 1280, 720) == 1);
    {
        float sx = 0, sy = 0;
        UICornerstone_GetViewportScale(g_uiInstance, &sx, &sy);
        check("T8 fit 新画布 0.8", feq(sx, 0.8f) && feq(sy, 0.8f));
    }
    check("T8 stretch 独立轴", UICornerstone_SetViewportScaleMode(g_uiInstance, 2) == 1);
    {
        float sx = 0, sy = 0;
        UICornerstone_GetViewportScale(g_uiInstance, &sx, &sy);
        check("T8 stretch sx=0.8 sy=1.0667", feq(sx, 0.8f) && feq(sy, 768.0f / 720.0f));
    }
    check("T8 回 off", UICornerstone_SetViewportScaleMode(g_uiInstance, 0) == 1);
    {
        float sx = 0, sy = 0;
        UICornerstone_GetViewportScale(g_uiInstance, &sx, &sy);
        check("T8 off 复合=1", feq(sx, 1.0f) && feq(sy, 1.0f));
    }
    check("T8 anchor 增量（off）", UICornerstone_SetViewportAnchor(g_uiInstance, 5, 10) == 1);
    {
        SRect d = bench->getDrawRect();
        check("T8 anchor DR=(5,10,1280,720)", seq(d, SRect{5, 10, 1280.0f, 720.0f}));
    }
    check("T8 anchor 还原", UICornerstone_SetViewportAnchor(g_uiInstance, -5, -10) == 1);
    check("T8 非法 mode 拒绝", UICornerstone_SetViewportScaleMode(g_uiInstance, 5) == 0);
    check("T8 非法 canvas 拒绝", UICornerstone_SetCanvasSize(g_uiInstance, 0, 0) == 0);
    // ── T8x：视口背景色（RGBA8888 + 状态存储）──
    check("T8x 默认透明（a=0）", feq(g_uiInstance->viewportBackground.alpha(), 0.0f));
    check("T8x set 成功", UICornerstone_SetViewportBackgroundColor(g_uiInstance, 30, 34, 42, 255) == 1);
    check("T8x 存储 RGBA8888",
          g_uiInstance->viewportBackground.redByte() == 30 &&
          g_uiInstance->viewportBackground.greenByte() == 34 &&
          g_uiInstance->viewportBackground.blueByte() == 42 &&
          g_uiInstance->viewportBackground.alphaByte() == 255);
    check("T8x 重设为透明", UICornerstone_SetViewportBackgroundColor(g_uiInstance, 0, 0, 0, 0) == 1 &&
          feq(g_uiInstance->viewportBackground.alpha(), 0.0f));

    // ── T9x：TextArea/EditBox 字号随复合重建 + 滚动范围（stretch 独立轴）──
    // 状态：off、canvas=1280x720、viewport=1024x768 → stretch sx=0.8、sy=1.0667
    {
        auto ta = make_shared<TextArea>(nullptr, SRect(0, 0, 300, 200));
        BENCH->addControl(ta);
        ta->setLineHeight(20);
        ta->setWordWrap(false);
        std::string manyLines;
        for (int i = 0; i < 200; ++i) manyLines += "line" + std::to_string(i) + "\n";
        manyLines += std::string(500, 'x');   // 超长行 → 水平滚动条出现（占 16 高）
        ta->setText(manyLines);
        // 滚动范围（本地语义）：201 行×20 = 4020 总高 - (200-8 边距 -16 横滚) = 3844
        ta->setScrollY(99999);
        check("T9x off 滚动范围 clamp=3844", ta->getScrollY() == 3844);
        Font* fOff = ta->getFont();
        check("T9x 字号已加载", fOff != nullptr);
        check("T9x stretch 生效", UICornerstone_SetViewportScaleMode(g_uiInstance, 2) == 1);
        {
            float sx = 0, sy = 0;
            UICornerstone_GetViewportScale(g_uiInstance, &sx, &sy);
            check("T9x stretch sx=0.8 sy=1.0667", feq(sx, 0.8f) && feq(sy, 768.0f / 720.0f));
        }
        check("T9x 字号随复合及时重建", ta->getFont() != nullptr && ta->getFont() != fOff);
        ta->setScrollY(99999);
        check("T9x stretch 滚动范围不变=3844", ta->getScrollY() == 3844);
        UICornerstone_SetViewportScaleMode(g_uiInstance, 0);
    }

    // ── T9y：默认行高自适应（未 setLineHeight 定制时 = 实际字体高度/垂直复合）──
    {
        auto t2 = make_shared<TextArea>(nullptr, SRect(0, 0, 300, 200));
        BENCH->addControl(t2);
        t2->setText("aaa\nbbb\nccc");
        check("T9y 初始默认行高=20", t2->getLineHeight() == 20);
        t2->update();   // 懒检测：字体就绪后重算行高
        int fhOff = (t2->getTextRenderer() && t2->getFont())
            ? t2->getTextRenderer()->getFontHeight(t2->getFont()) : 0;
        check("T9y 行高自适应=实际字高", fhOff > 0 && t2->getLineHeight() == fhOff);
        check("T9y stretch 生效", UICornerstone_SetViewportScaleMode(g_uiInstance, 2) == 1);
        t2->update();
        int fhStretch = (t2->getTextRenderer() && t2->getFont())
            ? t2->getTextRenderer()->getFontHeight(t2->getFont()) : 0;
        check("T9y stretch 行高随复合重算",
              fhStretch > 0 &&
              t2->getLineHeight() == std::max(1, (int)(fhStretch / (768.0f / 720.0f))));
        UICornerstone_SetViewportScaleMode(g_uiInstance, 0);
    }

    // ── Ta：属性系统运行期切换（§4.7）：SetEnum("viewport-scale-mode", …) ──
    {
        int r1 = bench->setEnumProperty("viewport-scale-mode", "stretch");
        check("Ta 属性 stretch 生效", r1 == 1 && bench->getViewportScaleMode() == Bench::ViewportScaleMode::Stretch);
        int r2 = bench->setEnumProperty("viewport-scale-mode", "fit");
        check("Ta 属性 fit 生效", r2 == 1 && bench->getViewportScaleMode() == Bench::ViewportScaleMode::Fit);
        int r3 = bench->setEnumProperty("viewport-scale-mode", "off");
        check("Ta 属性 off 还原", r3 == 1 && bench->getViewportScaleMode() == Bench::ViewportScaleMode::Off);
        int r4 = bench->setEnumProperty("viewport-scale-mode", "bogus");
        check("Ta 非法枚举拒绝", r4 == 0);
        int r5 = bench->setEnumProperty("other-prop", "x");
        check("Ta 未知属性透传基类", r5 == 0);
        bench->setScaleX(1.0f);
        bench->setScaleY(1.0f);
    }

    // ── Tb：TreeView 字号随复合重建（§4.5 补全）──
    {
        auto tv = make_shared<TreeView>(bench, SRect(0, 0, 300, 200), 1.0f, 1.0f);
        bench->addControl(tv);
        tv->create();
        tv->setFontSize(14);
        check("Tb 基准字号14", tv->getFont() && tv->getFont()->getSize() == 14);
        tv->refreshScaleWith(2.0f, 1.0f);
        check("Tb 缩放2重建=28", tv->getFont() && tv->getFont()->getSize() == 28);
        tv->refreshScaleWith(1.0f, 1.0f);
        check("Tb 还原=14", tv->getFont() && tv->getFont()->getSize() == 14);
    }

    // ── Tc：Slider 刻度字号随复合重建（懒加载字体，refreshScaleWith 兜底创建）──
    {
        auto sl = make_shared<Slider>(bench, SRect(0, 0, 200, 30), 1.0f, 1.0f);
        bench->addControl(sl);
        sl->create();
        sl->refreshScaleWith(2.0f, 1.0f);
        check("Tc 刻度字号随复合=20", sl->getTickFont() && sl->getTickFont()->getSize() == 20);
        sl->refreshScaleWith(1.0f, 1.0f);
        check("Tc 刻度字号还原=10", sl->getTickFont() && sl->getTickFont()->getSize() == 10);
    }

    // ── Td：MenuBar/MenuPanel 字号随复合重建 + 面板传播 ──
    {
        auto bar = make_shared<MenuBar>(bench, 1.0f, 1.0f);
        bench->addControl(bar);
        bar->setContext(g_uiInstance);
        bar->create();
        auto mp = make_shared<MenuPanel>(bar.get(), 1.0f, 1.0f);
        mp->setContext(g_uiInstance);
        mp->create();
        bar->addMenu(u8"文件", mp);   // 挂入 MenuBar 条目（面板经此随 Bar 传播缩放）
        bar->setFontSize(12);
        mp->setFontSize(14);
        check("Td Bar 基准=12", bar->getFont() && bar->getFont()->getSize() == 12);
        check("Td Panel 基准=14", mp->getFont() && mp->getFont()->getSize() == 14);
        bar->refreshScaleWith(2.0f, 1.0f);
        check("Td Bar 缩放2=24", bar->getFont() && bar->getFont()->getSize() == 24);
        check("Td Panel 随 Bar 传播=28", mp->getFont() && mp->getFont()->getSize() == 28);
    }

    // ── Te：字号重建可见性过滤（§8）：不可见延后，可见帧补重建 ──
    {
        auto lb = make_shared<Label>(bench, SRect(0, 0, 100, 30), 1.0f, 1.0f);
        bench->addControl(lb);
        lb->setCaption(u8"可见性过滤");
        SharedFont f0 = lb->getFont();
        check("Te 基准字号16", f0 && f0->getSize() == 16);
        lb->setVisible(false);
        lb->refreshScaleWith(2.0f, 1.0f);
        check("Te 不可见不重建", lb->getFont() == f0);
        lb->setVisible(true);
        lb->update();
        check("Te 可见帧重建=32", lb->getFont() && lb->getFont()->getSize() == 32 && lb->getFont() != f0);
    }

    // ── Tf：EditBox 可见性过滤 + mapViewportToCanvas 逆变换（§4.8）──
    {
        auto eb = make_shared<EditBox>(bench, SRect(0, 0, 200, 40), 1.0f, 1.0f);
        bench->addControl(eb);
        Font* fe0 = eb->getFont();
        check("Tf 基准字号16", fe0 && fe0->getSize() == 16);
        SRect d1 = eb->getDrawRect();
        SPoint ca = eb->mapViewportToCanvas(SPoint{d1.left + 7, d1.top + 9});
        check("Tf 逆变换 scale1", feq(ca.x, 7) && feq(ca.y, 9));
        eb->setVisible(false);
        eb->refreshScaleWith(2.0f, 1.0f);
        check("Tf 不可见不重建", eb->getFont() == fe0);
        eb->setVisible(true);
        eb->update();
        check("Tf 可见帧重建=32", eb->getFont() && eb->getFont()->getSize() == 32 && eb->getFont() != fe0);
    }

    // ── Tg：字号缓存统计 + LuotiAni 帧过滤开关 ──
    {
        auto lb = make_shared<Label>(bench, SRect(0, 0, 100, 30), 1.0f, 1.0f);
        bench->addControl(lb);
        lb->setCaption(u8"缓存统计");
        auto* tr = lb->getTextRenderer();
        check("Tg 缓存计数非零", tr != nullptr && tr->getFontCacheEntryCount() > 0);
        g_ani->setFrameFilter(true);
        check("Tg 双线性开", g_ani->getFrameFilter());
        g_ani->setFrameFilter(false);
        check("Tg 双线性关", !g_ani->getFrameFilter());
    }

    // ── T9：JSON 顶层 viewport 键（canvas + scale-mode）──
    const char* kViewportJson = u8R"({
  "viewport": { "width": 1600, "height": 1080, "scale-mode": "fit" },
  "controls": [
    { "type": "panel", "id": "vpRoot", "rect": { "x": 0, "y": 0, "w": 1600, "h": 1080 } }
  ]
})";
    g_uiInstance->canvasWidth = 0;
    g_uiInstance->canvasHeight = 0;
    bench->setRect(SRect{0, 0, 1600.0f, 1080.0f});
    int rc = UICornerstone_LoadLayout(g_uiInstance, kViewportJson);
    check("T9 LoadLayout 带 viewport 键成功", rc != 0);
    {
        float sx = 0, sy = 0;
        UICornerstone_GetViewportScale(g_uiInstance, &sx, &sy);
        check("T9 JSON fit 生效 sx=0.64",
              bench->getViewportScaleMode() == Bench::ViewportScaleMode::Fit &&
              feq(sx, 0.64f) && feq(sy, 0.64f) &&
              feq(g_uiInstance->canvasWidth, 1600.0f) && feq(g_uiInstance->canvasHeight, 1080.0f));
    }

    // ── T12：实例缩放（画布语义，C ABI SetInstanceScale）──
    {
        // 环境：off、rect 1024x768、scale=1
        UICornerstone_SetViewportScaleMode(g_uiInstance, 0);
        bench->setRect(SRect{0, 0, 1024.0f, 768.0f});
        float ix = 0, iy = 0;
        check("T12 初始 GetInstanceScale=1,1",
              UICornerstone_GetInstanceScale(g_uiInstance, &ix, &iy) == 1 &&
              feq(ix, 1.0f) && feq(iy, 1.0f));
        // 参数拒绝：x≤0
        check("T12 非法 scale 拒绝", UICornerstone_SetInstanceScale(g_uiInstance, 0.0f, 1.0f) == 0 &&
              UICornerstone_SetInstanceScale(g_uiInstance, 1.0f, -1.0f) == 0);
        // 设置 1.5：bench 复合=1.5；子控件（g_panelA 复合 1 → 1.5）drawRect×1.5
        SRect ra0 = g_panelA->getRect();
        check("T12 SetInstanceScale(1.5,1.5) 成功", UICornerstone_SetInstanceScale(g_uiInstance, 1.5f, 1.5f) == 1);
        check("T12 GetInstanceScale=1.5,1.5", UICornerstone_GetInstanceScale(g_uiInstance, &ix, &iy) == 1 &&
              feq(ix, 1.5f) && feq(iy, 1.5f));
        check("T12 A 复合=1.5", feq(g_panelA->getScaleXX(), 1.5f) && feq(g_panelA->getScaleYY(), 1.5f));
        SRect ra1 = g_panelA->getDrawRect();
        check("T12 A drawRect×1.5", feq(ra1.width, ra0.width * 1.5f) && feq(ra1.height, ra0.height * 1.5f));
        // 逻辑 rect 不变（drawRect 变、rect 不变）
        SRect rlog = g_panelA->getRect();
        check("T12 逻辑 rect 不变", seq(rlog, ra0));
        // 幂等：重复调用相同值 → 快速返回、scale 不变
        check("T12 幂等重复调用成功且不变", UICornerstone_SetInstanceScale(g_uiInstance, 1.5f, 1.5f) == 1 &&
              feq(g_panelA->getScaleXX(), 1.5f));
        // resize 保持 override（off 分支尊重手动缩放，不重置 1）
        g_uiInstance->viewport = SRect{0, 0, 800, 600};
        bench->resized(SRect{0, 0, 800.0f, 600.0f});
        check("T12 resize 后手动缩放保持 1.5", feq(bench->getScaleXX(), 1.5f) && feq(bench->getScaleYY(), 1.5f));
        g_uiInstance->viewport = SRect{0, 0, 1024, 768};
        bench->resized(SRect{0, 0, 1024.0f, 768.0f});
        // 切 fit 引擎接管（清除 override），scale=视口/画布比（清显式画布 → 用 rect）
        g_uiInstance->canvasWidth = 0; g_uiInstance->canvasHeight = 0;
        bench->setRect(SRect{0, 0, 1024.0f, 768.0f});
        bench->setViewportScaleMode(Bench::ViewportScaleMode::Fit);
        check("T12 fit 接管 override 清除", bench->getViewportScaleMode() == Bench::ViewportScaleMode::Fit &&
              !bench->getManualScaleOverride() && feq(bench->getScaleXX(), 1.0f) && feq(bench->getScaleYY(), 1.0f));
        // 还原
        UICornerstone_SetViewportScaleMode(g_uiInstance, 0);
        bench->setScaleX(1.0f);
        bench->setScaleY(1.0f);
    }

    // ── 最终还原：off、scale=1、anchor 清、canvas 归零 ──
    UICornerstone_SetViewportScaleMode(g_uiInstance, 0);
    bench->setScaleX(1.0f);
    bench->setScaleY(1.0f);
    g_uiInstance->canvasWidth = 0;
    g_uiInstance->canvasHeight = 0;
    bench->setRect(SRect{0, 0, 1024.0f, 768.0f});

    printf(u8"[viewport-scale] check count: pass=%d fail=%d\n", s_pass, s_fail);
    fflush(stdout);
}

class ViewportScaleApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle(u8"test_viewport_scale: 视口缩放核心链路断言（无头，auto=3）");
        BENCH->setOnInitial([this](shared_ptr<Bench>) {
            g_panelA = make_shared<Panel>(BENCH, SRect(0, 0, 200, 150), 1.0f, 1.0f);
            g_buttonB = make_shared<Button>(g_panelA.get(), SRect(10, 10, 100, 50), 2.0f, 2.0f);
            g_panelC = make_shared<Panel>(g_buttonB.get(), SRect(5, 5, 50, 30), 3.0f, 3.0f);
            g_actor = make_shared<Actor>(nullptr, false, 1.0f, 1.0f);
            g_buttonB->setNormalStateActor(g_actor);
            g_ani = make_shared<LuotiAni>(nullptr, 1.0f, 1.0f);
            g_buttonB->setLuotiAni(g_ani);
            BENCH->addControl(g_panelA);
            g_panelA->addControl(g_buttonB);
            g_buttonB->addControl(g_panelC);
        });
        return true;
    }

    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }

    void onRender() override {
        static bool s_run = false;
        GET_RENDERDEVICE->setDrawColor(SColor(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
        if (!s_run) {
            s_run = true;
            runChecks();
        }
    }

    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    return TestRunMain<ViewportScaleApp, 1024, 768>(argc, argv);
}