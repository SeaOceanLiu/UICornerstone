// ============================================================================
// test_statusbar.cpp -- StatusBar 状态栏控件测试
// 断言：数据模型（增删改/右对齐）/ 属性回环 / 弹窗绑定
// 可视化：底部状态栏（左：分支+问题数；右：编码/行尾/缩进）+ 点击分支弹出菜单
// ============================================================================
#include <iostream>
#include <memory>
#include <cmath>
#include "StatusBar.h"
#include "Menu.h"
#include "Shape.h"
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "TestInstance.h"
#include "UICornerstoneAPI.h"
#include "EventTypes.h"
#include "PropertyNames.h"

using namespace std;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { ++g_pass; TestUtil::log("OK   %s", msg); } \
    else      { ++g_fail; TestUtil::log("FAIL %s", msg); } \
} while (0)

static shared_ptr<StatusBar> g_probe;   // 数据模型断言探针（不挂树）
static shared_ptr<StatusBar> g_bar;     // 可视化控件（供 App 阶段点击弹出）
static bool g_itemClicked = false;      // status item onClick 回调标记

static shared_ptr<Event> makeMouse(EventType type, float x, float y) {
    auto ev = make_shared<Event>(type);
    if (type == EventType::MouseMove) ev->mousePos = {x, y};
    else ev->mouseButton = {x, y, MouseButton::Left};
    return ev;
}

static void runAssertions() {
    TestUtil::log("---- StatusBar assertions ----");

    g_probe->addStatusItem("branch", u8"main", false);
    g_probe->addStatusItem("problems", u8"0 问题", false);
    g_probe->addStatusItem("encoding", u8"UTF-8", true);
    g_probe->addStatusItem("eol", u8"LF", true);
    CHECK(g_probe->getStatusItem("branch") != nullptr, "addStatusItem x4");
    CHECK(g_probe->getStatusItem("branch")->rightAlign == false, "left item rightAlign=false");
    CHECK(g_probe->getStatusItem("encoding")->rightAlign == true, "right item rightAlign=true");

    // 改文本
    g_probe->updateStatusItemText("encoding", u8"GBK");
    CHECK(g_probe->getStatusItem("encoding")->text == u8"GBK", "updateStatusItemText");

    // 移除
    g_probe->removeStatusItem("eol");
    CHECK(g_probe->getStatusItem("eol") == nullptr, "removeStatusItem");

    // 弹窗面板初始为空
    CHECK(g_probe->getPopupPanel() == nullptr, "popup panel null initially");

    // 属性回环
    g_probe->setIntProperty("font-size", 14);
    int fsz = 0;
    CHECK(g_probe->getIntProperty("font-size", fsz) == 1 && fsz == 14, "font-size roundtrip");
    g_probe->setFloatProperty("item-height", 28.f);
    float f = 0.f;
    CHECK(g_probe->getFloatProperty("item-height", f) == 1 && f == 28.f, "item-height roundtrip");

    // P0-53：文本更新触发重排（段宽随文本变化）
    g_probe->addStatusItem("grow", "x", false);
    const float wShort = g_probe->getStatusItem("grow")->hitRect.width;
    g_probe->updateStatusItemText("grow", "1234567890123456789012345678901234567890");
    const float wLong = g_probe->getStatusItem("grow")->hitRect.width;
    CHECK(wLong > wShort, "P0-53 updateStatusItemText triggers relayout (hitRect grows)");

    // P0-55：段着色（四态 mask 稀疏 + 背景）
    g_probe->setStatusItemTextColor("grow", SColor(255, 0, 0, 255), ControlState::Normal);
    g_probe->setStatusItemTextColor("grow", SColor(0, 255, 0, 255), ControlState::Hover);
    g_probe->setStatusItemBackgroundColor("grow", SColor(0, 0, 255, 255));
    {
        StatusItem* it = g_probe->getStatusItem("grow");
        CHECK(it && it->textColorMask == 3 && it->hasBackground, "P0-55 item color set (mask/bg)");
        CHECK(it && it->textColor.getNormal().redByte() == 255 && it->textColor.getHover().greenByte() == 255,
              "P0-55 item text color normal/hover stored");
        CHECK(it && it->background.blueByte() == 255, "P0-55 item background stored");
    }

    // P0-64②④：控件级 hover 键 + 段显式 hover 背景
    {
        SColor hc;
        CHECK(g_probe->setColorProperty(PropertyNames::kTreeHover, SColor(9, 8, 7, 255)) == 1, "P0-64b set hover key");
        CHECK(g_probe->getColorProperty(PropertyNames::kTreeHover, hc) == 1 && hc.blueByte() == 7, "P0-64b get hover key");
        g_probe->setStatusItemHoverBackgroundColor("grow", SColor(111, 222, 3, 255));
        StatusItem* ih = g_probe->getStatusItem("grow");
        CHECK(ih && ih->hasHoverBackground && ih->hoverBackground.redByte() == 111, "P0-64④ segment hover bg stored");
    }

    // 图标控件绑定（API 接受）
    auto icon = make_shared<Label>(nullptr, SRect(0, 0, 16, 16));
    g_probe->setStatusItemLeadingControl("branch", icon);
    CHECK(g_probe->getStatusItem("branch")->leadingControl == icon, "setStatusItemLeadingControl");

    TestUtil::log("---- assertions done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── C ABI 抽查 ──
static void runCabiChecks() {
    TestUtil::log("---- StatusBar CABI checks ----");
    UIControlHandle h = UICornerstone_CreateStatusBar(
        g_uiInstance, 700.f, 820.f, 360.f, 24.f, 1.f, 1.f);
    CHECK(h != nullptr, "CreateStatusBar");

    CHECK(UICornerstone_StatusBarAddItem(g_uiInstance, h, "a", u8"左段", 0) == 1 &&
          UICornerstone_StatusBarAddItem(g_uiInstance, h, "b", u8"右段", 1) == 1,
          "StatusBarAddItem x2");

    CHECK(UICornerstone_StatusBarSetItemText(g_uiInstance, h, "a", u8"已改") == 1,
          "StatusBarSetItemText");

    // 图标（Label 句柄）
    UIControlHandle lbl = UICornerstone_CreateLabel(g_uiInstance, u8"@", 14.f, 0.f, 0.f, 16.f, 16.f, 1.f, 1.f);
    CHECK(UICornerstone_StatusBarSetItemIcon(g_uiInstance, h, "a", lbl) == 1,
          "StatusBarSetItemIcon");

    // 弹窗（MenuPanel 句柄）
    UIControlHandle mp = UICornerstone_CreateMenuPanel(g_uiInstance, 1.f, 1.f);
    CHECK(UICornerstone_StatusBarSetItemMenu(g_uiInstance, h, "a", mp) == 1,
          "StatusBarSetItemMenu");

    CHECK(UICornerstone_StatusBarRemoveItem(g_uiInstance, h, "b") == 1,
          "StatusBarRemoveItem");

    // P0-57：运行期 font-size 失效缓存字体（段宽随新字号变化）
    {
        auto* bar = dynamic_cast<StatusBar*>(static_cast<Control*>(h));
        UICornerstone_SetInt(g_uiInstance, h, "font-size", 10);
        const float w10 = bar->getStatusItem("a")->hitRect.width;
        UICornerstone_SetInt(g_uiInstance, h, "font-size", 20);
        const float w20 = bar->getStatusItem("a")->hitRect.width;
        CHECK(w20 > w10, "P0-57 setFontSize invalidates cached font (hitRect grows)");

        // P0-58：段级字号 / 文字阴影（ABI + 字段）
        CHECK(UICornerstone_StatusBarSetItemFontSize(g_uiInstance, h, "a", 26.f) == 1,
              "P0-58 SetItemFontSize");
        const float wItem = bar->getStatusItem("a")->hitRect.width;
        CHECK(wItem > w20, "P0-58 item font size affects layout");
        CHECK(UICornerstone_StatusBarSetItemTextShadow(g_uiInstance, h, "a",
                  UIColor{10, 20, 30, 255}, 2.f, 3.f) == 1, "P0-58 SetItemTextShadow");
        StatusItem* it = bar->getStatusItem("a");
        CHECK(it && it->hasShadow && it->shadowOffset.x == 2.f && it->shadowColor.blueByte() == 30,
              "P0-58 item shadow stored");
    }

    TestUtil::log("---- CABI checks done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── 可视化矩阵 ──
static void testStatusBarVisualize(Bench* bench) {
    g_probe = make_shared<StatusBar>(nullptr, SRect(0, 0, 100, 24));   // 断言探针不挂树
    runAssertions();

    // ── status-item-click C ABI 事件：点击无菜单段 → intVal=段索引 ──
    g_probe->setVisible(true);
    static int gEvIdx = -9, gEvCnt = 0;
    gEvIdx = -9; gEvCnt = 0;
    using CbFn = void(*)(void*, const void*, void*);
    g_probe->setCallbackProperty(PropertyNames::kEventStatusItemClick, CbFn(
        [](void*, const void* raw, void*) {
            const UIEventData* ev = static_cast<const UIEventData*>(raw);
            if (ev->eventName && strcmp(ev->eventName, PropertyNames::kEventStatusItemClick) == 0) {
                gEvIdx = ev->data.intVal; ++gEvCnt;
            }
        }), nullptr);
    auto* it0 = g_probe->getStatusItem("branch");
    SRect r0 = it0 ? it0->hitRect : SRect(0, 0, 0, 0);
    g_probe->handleEvent(makeMouse(EventType::MouseDown, r0.left + 2, r0.top + 2));   // 命中首个左段
    CHECK(gEvCnt == 1 && gEvIdx == 0, "status-item-click fired idx=0");
    auto* it1 = g_probe->getStatusItem("problems");
    SRect r1 = it1 ? it1->hitRect : SRect(0, 0, 0, 0);
    g_probe->handleEvent(makeMouse(EventType::MouseDown, r1.left + 2, r1.top + 2));   // 命中第 2 左段
    CHECK(gEvCnt == 2 && gEvIdx == 1, "status-item-click fired idx=1");
    TestUtil::log("---- status-item-click events: pass=%d fail=%d ----", g_pass, g_fail);

    auto bar = make_shared<StatusBar>(nullptr, SRect(0, 876, 1400, 24));
    bar->addStatusItem("branch", u8"main", false);
    bar->addStatusItem("problems", u8"0 问题 0 警告", false);
    bar->addStatusItem("encoding", u8"UTF-8", true);
    bar->addStatusItem("eol", u8"LF", true);
    bar->addStatusItem("indent", u8"空格: 4", true);

    // 分支段绑定弹窗菜单
    auto branchMenu = make_shared<MenuPanel>(nullptr, 1.0f, 1.0f);
    branchMenu->addItem(MenuItemBuilder(u8"master").build());
    branchMenu->addItem(MenuItemBuilder(u8"develop").build());
    branchMenu->addItem(MenuItemBuilder(u8"feature/login").build());
    bar->setStatusItemMenu("branch", branchMenu);

    // 分支段绑定图标（带文字的小 Label，可视化可见）
    // 注意：leadingControl 不在子控件链上，须显式补 context 再 create
    // 几何图标（Shape 圆点）：验证槽内几何居中；文字型图标受 Label 基线常量偏移影响
    auto icon = make_shared<Shape>(nullptr, SRect(0, 0, 16, 16));
    icon->setShape(ShapeType::Circle);
    icon->setFillColor(SColor(120, 200, 255));
    bar->setStatusItemLeadingControl("branch", icon);

    // 点击回调（encoding 段）
    g_itemClicked = false;
    bar->setStatusItemOnClick("encoding", [](shared_ptr<StatusItem>) { g_itemClicked = true; });

    bar->create();
    bench->addControl(bar);
    icon->setContext(UIContext::getLastInstance());
    icon->create();

    // 缩放可视化：1.5x 状态栏（StatusBarBuilder 路径；getDrawRect = rect × scale）
    auto sbScaled = StatusBarBuilder(nullptr, SRect(120, 560, 600, 24), 2.0f, 2.0f)
                        .addStatusItem("s1", u8"缩放 2.0x", false)
                        .addStatusItem("s2", u8"右段", true)
                        .build();
    bench->addControl(sbScaled);
    CHECK(fabs(sbScaled->getDrawRect().width - 1200.f) < 0.01f, "scaled statusbar drawRect = rect*2.0");
    CHECK(fabs(sbScaled->getDrawRect().height - 48.f) < 0.01f, "scaled statusbar drawRect.height = h*2.0");

    runCabiChecks();

    // 记录控件供 App 阶段模拟点击弹出
    g_bar = bar;
}

class StatusBarApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_statusbar");
        BENCH->setOnInitial([](shared_ptr<Bench> b) { testStatusBarVisualize(b.get()); });
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(40, 40, 44));
        GET_RENDERDEVICE->clear();
        BENCH->draw();

        static uint8_t pixels[1400 * 900 * 4];
        int w = 0, h = 0;
        const int cap = UICornerstone_CaptureViewport(g_uiInstance, pixels, &w, &h);

        if (m_frames == 20 && !m_savedNormal) {
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/statusbar_normal.bmp") : 0;
            TestUtil::log("capture normal: cap=%d saved=%d -> %s", cap, saved, saved ? "Temp/statusbar_normal.bmp" : "FAILED");
            m_savedNormal = true;
        }

        // 第 30 帧：模拟点击分支段，弹出菜单
        if (m_frames == 30 && g_bar) {
            auto* it = g_bar->getStatusItem("branch");
            if (it) {
                const float cx = g_bar->getRect().left + it->hitRect.left + it->hitRect.width / 2.f;
                const float cy = g_bar->getRect().top + it->hitRect.top + it->hitRect.height / 2.f;
                g_bar->handleEvent(makeMouse(EventType::MouseDown, cx, cy));
                g_bar->handleEvent(makeMouse(EventType::MouseUp, cx, cy));
                TestUtil::log("clicked branch at (%.0f,%.0f) popupOpen=%d", cx, cy,
                              g_bar->isPopupOpen() ? 1 : 0);
            }
        }

        // 第 30 帧：模拟点击分支段，弹出菜单
        if (m_frames == 30 && g_bar) {
            auto* it = g_bar->getStatusItem("branch");
            if (it) {
                const float cx = g_bar->getRect().left + it->hitRect.left + it->hitRect.width / 2.f;
                const float cy = g_bar->getRect().top + it->hitRect.top + it->hitRect.height / 2.f;
                g_bar->handleEvent(makeMouse(EventType::MouseDown, cx, cy));
                g_bar->handleEvent(makeMouse(EventType::MouseUp, cx, cy));
                TestUtil::log("clicked branch at (%.0f,%.0f) popupOpen=%d", cx, cy,
                              g_bar->isPopupOpen() ? 1 : 0);
                CHECK(g_bar->isPopupOpen(), "click branch opens popup");
            }
        }

        // 第 38 帧：点击弹窗菜单第一项（master）→ 回调 + 关闭
        if (m_frames == 38 && g_bar && g_bar->isPopupOpen()) {
            auto* panel = g_bar->getPopupPanel().get();
            if (panel) {
                SRect pr = panel->getDrawRect();
                const float ix = pr.left + 20.f, iy = pr.top + 12.f;
                panel->handleEvent(makeMouse(EventType::MouseDown, ix, iy));
                panel->handleEvent(makeMouse(EventType::MouseUp, ix, iy));
                CHECK(!g_bar->isPopupOpen(), "popup item click closes popup");
            }
        }

        // 第 46 帧：点击 encoding 段 → onClick 回调
        if (m_frames == 46 && g_bar) {
            auto* it = g_bar->getStatusItem("encoding");
            if (it) {
                const float cx = g_bar->getRect().left + it->hitRect.left + it->hitRect.width / 2.f;
                const float cy = g_bar->getRect().top + it->hitRect.top + it->hitRect.height / 2.f;
                g_bar->handleEvent(makeMouse(EventType::MouseDown, cx, cy));
                g_bar->handleEvent(makeMouse(EventType::MouseUp, cx, cy));
                CHECK(g_itemClicked, "status item onClick fires");
            }
        }

        if (m_frames == 55 && !m_savedPopup) {
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/statusbar_popup.bmp") : 0;
            TestUtil::log("capture popup: cap=%d saved=%d -> %s", cap, saved, saved ? "Temp/statusbar_popup.bmp" : "FAILED");
            m_savedPopup = true;
            TestUtil::log("---- StatusBar test result: pass=%d fail=%d ----", g_pass, g_fail);
        }
        ++m_frames;
    }
    void onQuit() override { TestUtil::log("StatusBar test quit"); }

private:
    int m_frames = 0;
    bool m_savedNormal = false;
    bool m_savedPopup = false;
};

int main(int argc, char* argv[]) {
    return TestRunMain<StatusBarApp>(argc, argv);
}
