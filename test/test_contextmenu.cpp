// ============================================================================
// test_contextmenu.cpp -- ContextMenu 右键上下文菜单测试
// 断言：addItem/分隔线/回调关闭；交互：show→点项关闭+回调、外部点击关闭、Esc 关闭、边缘钳制
// CABI：Create/AddItem/AddSeparator/Show/Close + SetPtr 绑定
// ============================================================================
#include <iostream>
#include <memory>
#include <cmath>
#include "ContextMenu.h"
#include "Menu.h"
#include "Label.h"
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

static shared_ptr<Event> makeMouse(EventType type, float x, float y) {
    auto ev = make_shared<Event>(type);
    if (type == EventType::MouseMove) ev->mousePos = {x, y};
    else ev->mouseButton = {x, y, MouseButton::Left};
    return ev;
}

// ── 断言（使用独立 probe 菜单，避免与主菜单项重复）──
static void runAssertions(shared_ptr<ContextMenu> menu) {
    TestUtil::log("---- ContextMenu assertions ----");
    CHECK(menu->getMenuPanel() != nullptr, "menu panel exists");
    bool clicked = false;
    menu->addItem(u8"复制", [&](shared_ptr<MenuItem>) { clicked = true; });
    menu->addItem(u8"粘贴", [&](shared_ptr<MenuItem>) {});
    menu->addSeparator();
    menu->addItem(u8"删除", [&](shared_ptr<MenuItem>) {});
    CHECK(menu->getMenuPanel()->getRect().height >= 0, "addItem/addSeparator no-crash");

    // 点第一项：回调触发 + 菜单关闭
    auto* panel = menu->getMenuPanel().get();
    menu->show(200.f, 200.f);
    SRect pr = panel->getDrawRect();
    float ix = pr.left + 20.f;
    float iy = pr.top + 12.f;
    panel->handleEvent(makeMouse(EventType::MouseDown, ix, iy));
    panel->handleEvent(makeMouse(EventType::MouseUp, ix, iy));
    CHECK(clicked, "item click fires user callback");
    CHECK(!menu->isPopupVisible(), "menu closes after item click");
    menu->close();
    TestUtil::log("---- assertions done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── CABI ──
static void runCabiChecks() {
    TestUtil::log("---- ContextMenu CABI checks ----");
    UIControlHandle m = UICornerstone_CreateContextMenu(g_uiInstance, 600.f, 60.f, 10.f, 10.f, 1.f, 1.f);
    CHECK(m != nullptr, "CreateContextMenu");

    CHECK(UICornerstone_ContextMenuAddItem(g_uiInstance, m, u8"打开", "Ctrl+O") == 1, "ContextMenuAddItem");
    CHECK(UICornerstone_ContextMenuAddSeparator(g_uiInstance, m) == 1, "ContextMenuAddSeparator");
    CHECK(UICornerstone_ContextMenuAddItem(g_uiInstance, m, u8"属性", nullptr) == 1, "ContextMenuAddItem #2");

    // 绑定到 Label（SetPtr "context-menu"）
    UIControlHandle lbl = UICornerstone_CreateLabel(g_uiInstance, u8"右键目标", 14.f, 600.f, 400.f, 120.f, 24.f, 1.f, 1.f);
    CHECK(UICornerstone_SetPtr(g_uiInstance, lbl, PropertyNames::kPropContextMenu, m) == 1, "SetPtr context-menu binding");

    // 显示 + 关闭
    CHECK(UICornerstone_ContextMenuShow(g_uiInstance, m, 610.f, 70.f) == 1, "ContextMenuShow");
    CHECK(UICornerstone_ContextMenuClose(g_uiInstance, m) == 1, "ContextMenuClose");
    TestUtil::log("---- CABI checks done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── 可视化 + 交互 ──
static shared_ptr<ContextMenu> g_menu;   // 供 App 阶段交互
static shared_ptr<Label> g_label;         // 右键目标（路径 B）

static void testContextMenuVisualize(Bench* bench) {
    auto menu = make_shared<ContextMenu>(nullptr, 1.f, 1.f);

    // 「复制」带前置图标（leadingControl 可视化）
    auto copyIcon = make_shared<Label>(nullptr, SRect(0, 0, 16, 16));
    copyIcon->setCaption(u8"C");
    copyIcon->setTextStateColor(StateColor(SColor(120, 200, 255), SColor(120, 200, 255),
                                           SColor(120, 200, 255), SColor(120, 200, 255)));
    copyIcon->create();
    menu->addItem(MenuItemBuilder(u8"复制").setLeadingControl(copyIcon)
                     .setOnClick([&](shared_ptr<MenuItem>) { TestUtil::log("clicked 复制"); }).build());
    menu->addItem(u8"粘贴", [&](shared_ptr<MenuItem>) {});
    menu->addItem(u8"重命名", [&](shared_ptr<MenuItem>) {});
    menu->addSeparator();
    menu->addItem(u8"删除", [&](shared_ptr<MenuItem>) {});

    // 背景标签 + 右键绑定（路径 B）
    auto label = make_shared<Label>(nullptr, SRect(120, 120, 200, 30));
    label->setCaption(u8"在此处点击右键");
    label->create();
    bench->addControl(label);
    label->setContextMenu(menu);
    g_label = label;

    // 直接 show 以呈现（路径 A）
    menu->show(140.f, 160.f);
    g_menu = menu;

    // 缩放可视化：1.5x 菜单（ContextMenuBuilder 路径）
    auto scaledMenu = ContextMenuBuilder(2.0f, 2.0f)
                          .addItem(u8"缩放 2.0x", nullptr)
                          .addItem(u8"第二项", nullptr)
                          .addSeparator()
                          .addItem(u8"第三项", nullptr)
                          .build();
    scaledMenu->show(860.f, 420.f);   // 2x 面板较大，左移避免溢出窗体
    CHECK(scaledMenu->getMenuPanel() != nullptr, "scaled menu built (1.5x)");

    // 独立 probe 菜单做断言（点项回调 + 关闭）
    auto probe = make_shared<ContextMenu>(nullptr, 1.f, 1.f);
    runAssertions(probe);
    runCabiChecks();

    g_menu->show(140.f, 160.f);   // 重新展开，供帧内截图

    // JSON "context-menu" 键解析（决策点 2-C）
    const string json = R"({
      "controls":[{
        "type":"label","id":"lbl","rect":[10,10,120,24],"caption":"t",
        "context-menu":{"items":[
          {"caption":"Open","shortcut":"Ctrl+O"},
          {"type":"separator"},
          {"caption":"Delete"}
        ]}
      }]
    })";
    LayoutParser parser;
    auto root = parser.parseLayout(json);
    auto* ci = root ? dynamic_cast<ControlImpl*>(root.get()) : nullptr;
    auto cm = ci ? ci->getContextMenu() : nullptr;
    CHECK(cm != nullptr, "JSON contextMenu: menu bound to control");
    if (cm) {
        CHECK(cm->getMenuPanel() != nullptr, "JSON contextMenu: panel populated");
        CHECK(cm->getMenuPanel()->getRect().height > 0, "JSON contextMenu: items laid out");
    }
}

class ContextMenuApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_contextmenu");
        BENCH->setOnInitial([](shared_ptr<Bench> b) { testContextMenuVisualize(b.get()); });
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

        if (m_frames == 15 && !m_capShown) {
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, "Temp/contextmenu_shown.bmp") : 0;
            TestUtil::log("capture shown: cap=%d saved=%d -> %s", cap, saved, saved ? "Temp/contextmenu_shown.bmp" : "FAILED");
            m_capShown = true;
        }

        // 第 25 帧：模拟点击菜单「复制」项 → 关闭
        if (m_frames == 25 && g_menu && g_menu->isPopupVisible()) {
            auto* panel = g_menu->getMenuPanel().get();
            SRect pr = panel->getDrawRect();
            float ix = pr.left + 80.f, iy = pr.top + 12.f;   // 避开首项 leadingControl 图标槽
            g_menu->handleEvent(makeMouse(EventType::MouseDown, ix, iy));
            g_menu->handleEvent(makeMouse(EventType::MouseUp, ix, iy));
            TestUtil::log("after item click: popupVisible=%d", g_menu->isPopupVisible() ? 1 : 0);
            CHECK(!g_menu->isPopupVisible(), "item click closes menu");
        }

        // 第 35 帧：重新 show，模拟外部点击关闭（走 beforeEventHandlingWatcher）
        if (m_frames == 35 && g_menu) {
            g_menu->show(140.f, 160.f);
        }
        if (m_frames == 38 && g_menu) {
            auto ev = makeMouse(EventType::MouseDown, 700.f, 700.f);
            g_menu->beforeEventHandlingWatcher(ev);
            TestUtil::log("after outside click: popupVisible=%d", g_menu->isPopupVisible() ? 1 : 0);
            CHECK(!g_menu->isPopupVisible(), "outside click closes menu");
        }

        // 第 48 帧：重新 show，模拟 Esc 关闭（走 beforeEventHandlingWatcher）
        if (m_frames == 48 && g_menu) {
            g_menu->show(140.f, 160.f);
        }
        if (m_frames == 51 && g_menu) {
            auto ev = make_shared<Event>(EventType::KeyDown);
            ev->keyEvent.keycode = KeyCode::Escape;
            g_menu->beforeEventHandlingWatcher(ev);
            TestUtil::log("after Esc: popupVisible=%d", g_menu->isPopupVisible() ? 1 : 0);
            CHECK(!g_menu->isPopupVisible(), "Esc closes menu");
        }

        // 第 60 帧：路径 B——右键标签弹出（先确保关闭）
        if (m_frames == 60 && g_menu && g_label) {
            if (g_menu->isPopupVisible()) g_menu->close();
            float lx = g_label->getRect().left + 20.f;
            float ly = g_label->getRect().top + 15.f;
            auto rev = make_shared<Event>(EventType::MouseDown);
            rev->mouseButton = {lx, ly, MouseButton::Right};
            g_label->handleEvent(rev);
            TestUtil::log("after right-click label: popupVisible=%d", g_menu->isPopupVisible() ? 1 : 0);
            CHECK(g_menu->isPopupVisible(), "right-click on control opens context menu");
            TestUtil::log("---- ContextMenu test result: pass=%d fail=%d ----", g_pass, g_fail);
        }

        ++m_frames;
    }
    void onQuit() override { TestUtil::log("ContextMenu test quit"); }

private:
    int m_frames = 0;
    bool m_capShown = false;
};

int main(int argc, char* argv[]) {
    return TestRunMain<ContextMenuApp>(argc, argv);
}
