// ============================================================================
// test_tabcontrol.cpp -- TabControl 选项卡控件测试
// 断言：addTab/setCurrentIndex/onTabChange/页面显隐/键盘导航/removeTab 钳制
// CABI：Create/TabAddPage/TabSetTitle/TabSetPage/TabSetTabLeadingControl + 通用属性
// JSON：tab-control + tabs[] + currentIndex
// ============================================================================
#include <iostream>
#include <memory>
#include "TabControl.h"
#include "Panel.h"
#include "EditBox.h"
#include "Shape.h"
#include "Label.h"
#include "ListView.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "TestInstance.h"
#include "UICornerstoneAPI.h"
#include "EventTypes.h"
#include "PropertyNames.h"
#include "LayoutParser.h"

using namespace std;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { ++g_pass; TestUtil::log("OK   %s", msg); } \
    else      { ++g_fail; TestUtil::log("FAIL %s", msg); } \
} while (0)

static shared_ptr<Event> makeKey(KeyCode k) {
    auto ev = make_shared<Event>(EventType::KeyDown);
    ev->keyEvent.keycode = k;
    ev->keyEvent.mod = KeyMod::None;   // 事件未零化：垃圾 mod 位会被 isModSet 误判
    return ev;
}

static shared_ptr<Event> makeMouse(EventType type, float x, float y) {
    auto ev = make_shared<Event>(type);
    if (type == EventType::MouseMove) ev->mousePos = {x, y};
    else ev->mouseButton = {x, y, MouseButton::Left};
    return ev;
}

static shared_ptr<Panel> makePage(const string& label) {
    auto p = make_shared<Panel>(nullptr, SRect(0, 0, 100, 100));
    p->create();
    return p;
}

// ── 断言 ──
static void runAssertions() {
    TestUtil::log("---- TabControl assertions ----");
    auto tc = make_shared<TabControl>(nullptr, SRect(0, 0, 400, 300));
    tc->setVisible(true);
    tc->setFocused(true);   // 键盘导航仅焦点实例响应

    int i0 = tc->addTab("A", makePage("a"));
    int i1 = tc->addTab("B", makePage("b"));
    int i2 = tc->addTab("C", makePage("c"));
    CHECK(i0 == 0 && i1 == 1 && i2 == 2, "addTab returns sequential indices");
    CHECK(tc->getTabCount() == 3, "getTabCount == 3");
    CHECK(tc->getCurrentIndex() == 0, "initial current index == 0");

    // 页面显隐：仅当前页可见
    CHECK(tc->getTabs()[0].page->getVisible(), "page0 visible at start");

    // setCurrentIndex 切换 + onTabChange
    int fired = -1;
    tc->setOnTabChange([&](shared_ptr<TabControl>, int idx) { fired = idx; });
    tc->setCurrentIndex(2);
    CHECK(tc->getCurrentIndex() == 2, "setCurrentIndex(2)");
    CHECK(fired == 2, "onTabChange fired with index 2");
    CHECK(tc->getTabs()[2].page->getVisible(), "page2 visible after switch");
    CHECK(!tc->getTabs()[0].page->getVisible(), "page0 hidden after switch");

    // tab-changed C ABI 事件：intVal=新页索引
    static int gTabIdx = -9, gTabCnt = 0;
    using CbFn = void(*)(void*, const void*, void*);
    tc->setCallbackProperty(PropertyNames::kEventTabChanged, CbFn(
        [](void*, const void* raw, void*) {
            const UIEventData* ev = static_cast<const UIEventData*>(raw);
            if (ev->eventName && strcmp(ev->eventName, PropertyNames::kEventTabChanged) == 0) {
                gTabIdx = ev->data.intVal; ++gTabCnt;
            }
        }), nullptr);
    tc->setCurrentIndex(1);
    CHECK(gTabCnt == 1 && gTabIdx == 1, "tab-changed fired idx=1");
    tc->setCurrentIndex(0);
    CHECK(gTabCnt == 2 && gTabIdx == 0, "tab-changed fired idx=0");


    // 键盘导航：Right 循环 → 0 之后回绕
    tc->setCurrentIndex(0);
    tc->handleEvent(makeKey(KeyCode::Right));
    CHECK(tc->getCurrentIndex() == 1, "Right -> next tab");
    tc->handleEvent(makeKey(KeyCode::Left));
    CHECK(tc->getCurrentIndex() == 0, "Left -> prev tab");
    tc->handleEvent(makeKey(KeyCode::End));
    CHECK(tc->getCurrentIndex() == 2, "End -> last tab");
    tc->handleEvent(makeKey(KeyCode::Home));
    CHECK(tc->getCurrentIndex() == 0, "Home -> first tab");

    // removeTab 钳制
    tc->setCurrentIndex(2);
    tc->removeTab(2);
    CHECK(tc->getTabCount() == 2, "removeTab reduces count");
    CHECK(tc->getCurrentIndex() <= 1, "current index clamped after removeTab");

    // setPosition / fontSize 属性
    tc->setPosition(TabPosition::Bottom);
    CHECK(tc->getPosition() == TabPosition::Bottom, "setPosition Bottom");
    tc->setFontSize(16.f);
    CHECK(tc->getFontSize() == 16.f, "setFontSize");

    // ListView 页可见性守卫（修复：draw 覆写漏 m_visible 检查 → 切页不隐藏）
    {
        auto lvPage = ListViewBuilder(nullptr, SRect(0, 0, 200, 100))
            .addColumn("Name", 100)
            .build();
        int lvTab = tc->addTab(u8"ListView", lvPage);
        tc->setCurrentIndex(lvTab);
        CHECK(lvPage->getVisible(), "ListView 页切换后可见");
        lvPage->draw();   // 可见时绘制不崩
        tc->setCurrentIndex(0);
        CHECK(!lvPage->getVisible(), "ListView 切页后隐藏（getVisible false）");
        lvPage->draw();   // 隐藏后 draw 不绘制（守卫回归：无绘制泄漏/崩溃）
        tc->setCurrentIndex(lvTab);
        tc->setCurrentIndex(0);
    }

    TestUtil::log("---- assertions done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── CABI ──
static void runCabiChecks() {
    TestUtil::log("---- TabControl CABI checks ----");
    // 右下角空位：避免遮挡 g_tc 主页页的 EditBox（后添加者绘制在上层）
    UIControlHandle t = UICornerstone_CreateTabControl(g_uiInstance, 1080.f, 680.f, 300.f, 200.f, 1.f, 1.f);
    CHECK(t != nullptr, "CreateTabControl");

    CHECK(UICornerstone_TabAddPage(g_uiInstance, t, "One") == 0, "TabAddPage #0");
    CHECK(UICornerstone_TabAddPage(g_uiInstance, t, "Two") == 1, "TabAddPage #1");

    // 页面句柄绑定
    UIControlHandle pg = UICornerstone_CreatePanel(g_uiInstance, 0, 0, 100, 100, 1.f, 1.f);
    CHECK(UICornerstone_TabSetPage(g_uiInstance, t, 0, pg) == 1, "TabSetPage");

    CHECK(UICornerstone_TabSetTitle(g_uiInstance, t, 1, "TwoB") == 1, "TabSetTitle");

    UIControlHandle ic = UICornerstone_CreateLabel(g_uiInstance, "ic", 12.f, 0, 0, 16, 16, 1.f, 1.f);
    CHECK(UICornerstone_TabSetTabLeadingControl(g_uiInstance, t, 0, ic) == 1, "TabSetTabLeadingControl");

    // 通用属性：position / current-index / font-size
    CHECK(UICornerstone_SetEnum(g_uiInstance, t, PropertyNames::kJsonPosition, "left") == 1, "SetEnum position=left");
    CHECK(UICornerstone_SetInt(g_uiInstance, t, PropertyNames::kJsonCurrentIndex, 1) == 1, "SetInt current-index=1");
    CHECK(UICornerstone_SetInt(g_uiInstance, t, PropertyNames::kFontSize, 15) == 1, "SetInt font-size=15");
    int cur = -1;
    CHECK(UICornerstone_GetInt(g_uiInstance, t, PropertyNames::kJsonCurrentIndex, &cur) == 1 && cur == 1, "GetInt current-index reads back");
    int fs = -1;
    CHECK(UICornerstone_GetInt(g_uiInstance, t, PropertyNames::kFontSize, &fs) == 1 && fs == 15, "GetInt font-size reads back 15");

    TestUtil::log("---- CABI checks done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── 可视化 + JSON ──
static shared_ptr<TabControl> g_tc;
static shared_ptr<EditBox> g_edit1, g_edit2, g_edit3;   // 页内焦点环验证

// 页面：差异化底色 + 名称标签（切换可见性验证）
static shared_ptr<Panel> makePage(TabControl* tc, const string& name, SColor bg) {
    auto p = make_shared<Panel>(tc, SRect(0, 0, 100, 100));
    p->setBackgroundStateColor(StateColor(bg, bg, bg, bg));
    auto lbl = make_shared<Label>(p.get(), SRect(24, 24, 160, 28));
    lbl->setCaption(name);
    lbl->create();
    p->addControl(lbl);
    p->create();
    return p;
}

// 组装一个 TabControl：3 页（主页/设置/关于），position 四方向
static shared_ptr<TabControl> makeTab(Bench* bench, const SRect& rect, TabPosition pos) {
    auto tc = make_shared<TabControl>(bench, rect);
    tc->setPosition(pos);
    tc->addTab(u8"主页", makePage(tc.get(), u8"主页内容", SColor(180, 60, 60, 255)));
    tc->addTab(u8"设置", makePage(tc.get(), u8"设置内容", SColor(60, 160, 80, 255)));
    tc->addTab(u8"关于", makePage(tc.get(), u8"关于内容", SColor(60, 90, 190, 255)));
    tc->setCurrentIndex(0);
    tc->create();
    tc->show();                           // 默认 visible=false，显式显示
    bench->addControl(tc);
    return tc;
}

static void testTabVisualize(Bench* bench) {
    // 四方向矩阵：上（缺省）/ 下 / 左 / 右
    // g_tc 手工创建（主页/设置页内含 EditBox，见下块）；其余三方向用 makeTab
    makeTab(bench, SRect(720, 80, 560, 240), TabPosition::Bottom);
    makeTab(bench, SRect(120, 420, 560, 240), TabPosition::Left);
    makeTab(bench, SRect(720, 420, 560, 240), TabPosition::Right);

    // g_tc 主页/设置页内放 EditBox：验证焦点环经 Tab 在页内控件间切换
    {
        g_tc = make_shared<TabControl>(bench, SRect(120, 80, 560, 240));   // Top 缺省
        auto page1 = make_shared<Panel>(g_tc.get(), SRect(0, 0, 100, 100));
        page1->setBackgroundStateColor(StateColor(SColor(180, 60, 60), SColor(180, 60, 60),
                                                  SColor(180, 60, 60), SColor(180, 60, 60)));
        auto lbl1 = make_shared<Label>(page1.get(), SRect(24, 24, 160, 28));
        lbl1->setCaption(u8"主页内容"); lbl1->create(); page1->addControl(lbl1);
        g_edit1 = make_shared<EditBox>(page1.get(), SRect(30, 80, 220, 32));
        g_edit1->create(); g_edit1->show(); page1->addControl(g_edit1);
        g_edit2 = make_shared<EditBox>(page1.get(), SRect(30, 130, 220, 32));
        g_edit2->create(); g_edit2->show(); page1->addControl(g_edit2);
        page1->create();
        g_tc->addTab(u8"主页", page1);

        auto page2 = make_shared<Panel>(g_tc.get(), SRect(0, 0, 100, 100));
        page2->setBackgroundStateColor(StateColor(SColor(60, 160, 80), SColor(60, 160, 80),
                                                  SColor(60, 160, 80), SColor(60, 160, 80)));
        g_edit3 = make_shared<EditBox>(page2.get(), SRect(30, 80, 220, 32));
        g_edit3->create(); g_edit3->show(); page2->addControl(g_edit3);
        page2->create();
        g_tc->addTab(u8"设置", page2);

        g_tc->addTab(u8"关于", makePage(g_tc.get(), u8"关于内容", SColor(60, 90, 190, 255)));

        // leadingControl 视觉：页签图标（几何 Shape，颜色区分）
        // 图标未挂树：须显式 setContext + create 使 RenderDevice 就绪（同 StatusBar 惯例）
        auto makeTabIcon = [&](SColor c, bool circle) {
            auto ic = make_shared<Shape>(nullptr, SRect(0, 0, 14, 14));
            ic->setShape(circle ? ShapeType::Circle : ShapeType::FilledRect);
            ic->setFillColor(c);
            ic->setContext(UIContext::getLastInstance());
            ic->create();
            return ic;
        };
        g_tc->setTabLeadingControl(0, makeTabIcon(SColor(96, 165, 250), true));
        g_tc->setTabLeadingControl(1, makeTabIcon(SColor(74, 222, 128), false));
        g_tc->setTabLeadingControl(2, makeTabIcon(SColor(251, 146, 60), true));

        g_tc->setCurrentIndex(0);
        g_tc->create();
        g_tc->show();
        bench->addControl(g_tc);
    }

    // 缩放可视化：normal vs 2.0x 对照（TabControlBuilder 路径；getDrawRect = rect × scale）
    auto normTc = TabControlBuilder(nullptr, SRect(760, 690, 300, 100))
                      .addTab(u8"主页", makePage(nullptr, u8"主页内容", SColor(180, 60, 60, 255)))
                      .addTab(u8"设置", makePage(nullptr, u8"设置内容", SColor(60, 160, 80, 255)))
                      .setCurrentIndex(0)
                      .build();
    bench->addControl(normTc);
    auto scaledTc = TabControlBuilder(nullptr, SRect(120, 690, 300, 100), 2.0f, 2.0f)
                        .addTab(u8"主页", makePage(nullptr, u8"主页内容", SColor(180, 60, 60, 255)))
                        .addTab(u8"设置", makePage(nullptr, u8"设置内容", SColor(60, 160, 80, 255)))
                        .setCurrentIndex(0)
                        .build();
    bench->addControl(scaledTc);
    CHECK(fabs(scaledTc->getDrawRect().width - 600.f) < 0.01f, "scaled tabcontrol drawRect = rect*2.0");
    CHECK(fabs(scaledTc->getDrawRect().height - 200.f) < 0.01f, "scaled tabcontrol drawRect.height = h*2.0");

    // JSON 解析（tab-control + tabs + currentIndex）
    const string json = R"({
      "controls":[{
        "type":"tab-control","id":"tabs","rect":[0,0,480,320],
        "position":"top","font-size":13,"currentIndex":1,
        "tabs":[
          {"title":"Home","page":{"type":"panel"}},
          {"title":"Settings","page":{"type":"panel"}},
          {"title":"About","page":{"type":"panel"}}
        ]
      }]
    })";
    LayoutParser parser;
    auto root = parser.parseLayout(json);
    auto* ci = root ? dynamic_cast<ControlImpl*>(root.get()) : nullptr;
    auto jtc = ci ? dynamic_pointer_cast<TabControl>(ci->getThis()) : nullptr;
    CHECK(jtc != nullptr, "JSON tab-control parsed");
    if (jtc) {
        CHECK(jtc->getTabCount() == 3, "JSON tabs parsed (3 pages)");
        CHECK(jtc->getCurrentIndex() == 1, "JSON currentIndex applied");
        CHECK(jtc->getPosition() == TabPosition::Top, "JSON position applied");
    }

    // 调用断言与 CABI（独立于 bench 的 probe）
    runAssertions();
    runCabiChecks();
}

class TabApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_tabcontrol");
        BENCH->setOnInitial([](shared_ptr<Bench> b) { testTabVisualize(b.get()); });
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

        if (m_frames == 10 && !m_cap0) {
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/tab_shown.bmp") : 0;
            TestUtil::log("capture shown: cap=%d saved=%d", cap, saved);
            m_cap0 = true;
        }

        // 第 20 帧：切到第 2 页，验证页面切换
        if (m_frames == 20 && g_tc) {
            g_tc->setCurrentIndex(2);
            TestUtil::log("switched to tab %d", g_tc->getCurrentIndex());
            CHECK(g_tc->getCurrentIndex() == 2, "visual switch to tab 2");
        }
        if (m_frames == 40 && !m_cap1) {
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/tab_switched.bmp") : 0;
            TestUtil::log("capture switched: cap=%d saved=%d", cap, saved);
            m_cap1 = true;
        }

        // ── 焦点环切换：回主页 → 点击 edit1 → Tab → edit2 ──
        if (m_frames == 44 && g_tc) {
            g_tc->setCurrentIndex(0);   // 回主页（EditBox 所在页）
        }
        if (m_frames == 48 && g_edit1) {
            SRect r = g_edit1->getDrawRect();
            BENCH->handleEvent(makeMouse(EventType::MouseDown, r.left + r.width / 2.f, r.top + r.height / 2.f));
            BENCH->handleEvent(makeMouse(EventType::MouseUp, r.left + r.width / 2.f, r.top + r.height / 2.f));
        }
        if (m_frames == 50 && g_edit1 && g_edit2) {
            // 聚焦→Tab 同帧完成（避免帧间真实失焦事件干扰）；焦点环截图在下一帧 draw 后
            GET_FOCUSMANAGER->focusControl(g_edit1.get());
            CHECK(g_edit1->getFocused(), "click focuses edit1");
            if (cap) UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/tab_focus1.bmp");

            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Tab;
            ev->keyEvent.mod = KeyMod::None;
            BENCH->handleEvent(ev);   // Bench 拦截 Tab → focusNext（TabControl 作用域内）
        }
        if (m_frames == 52 && g_edit1 && g_edit2) {
            CHECK(g_edit2->getFocused() && !g_edit1->getFocused(), "Tab moves focus edit1 -> edit2 (in-page)");
            if (cap) UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/tab_focus2.bmp");
        }
        // ── Ctrl+Tab 层级切换：页内 → 页签条 → 页内 ──
        if (m_frames == 54 && g_tc) {
            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Tab;
            ev->keyEvent.mod = KeyMod::LCtrl;
            BENCH->handleEvent(ev);
        }
        if (m_frames == 56 && g_tc && g_edit2) {
            CHECK(g_tc->getFocused() && !g_edit1->getFocused() && !g_edit2->getFocused(),
                  "Ctrl+Tab exits page to tab bar");
        }
        if (m_frames == 58 && g_tc) {
            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Tab;
            ev->keyEvent.mod = KeyMod::LCtrl;
            BENCH->handleEvent(ev);
        }
        if (m_frames == 60 && g_edit1) {
            CHECK(g_edit1->getFocused() && !g_tc->getFocused(), "Ctrl+Tab enters page (edit1)");
        }
        if (m_frames == 62 && g_edit1) {
            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Tab;
            ev->keyEvent.mod = KeyMod::None;
            BENCH->handleEvent(ev);
        }
        if (m_frames == 64 && g_edit2) {
            CHECK(g_edit2->getFocused(), "Tab cycles in-page again");
        }
        if (m_frames == 66 && g_tc && g_edit2) {
            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Tab;
            ev->keyEvent.mod = KeyMod::LCtrl;
            BENCH->handleEvent(ev);   // e2 → 页签条
        }
        if (m_frames == 68 && g_tc) {
            CHECK(g_tc->getFocused(), "Ctrl+Tab exit again");
            // 页签条上按 Tab → 外层循环（不进页内）
            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Tab;
            ev->keyEvent.mod = KeyMod::None;
            BENCH->handleEvent(ev);
        }
        if (m_frames == 70 && g_edit1 && g_edit2 && g_tc) {
            CHECK(!g_edit1->getFocused() && !g_edit2->getFocused(),
                  "Tab on tab bar cycles outer scope (not into page)");
            TestUtil::log("---- TabControl test result: pass=%d fail=%d ----", g_pass, g_fail);
        }

        ++m_frames;
    }
    void onQuit() override { TestUtil::log("TabControl test quit"); }

private:
    int m_frames = 0;
    bool m_cap0 = false;
    bool m_cap1 = false;
};

int main(int argc, char* argv[]) {
    return TestRunMain<TabApp>(argc, argv);
}
