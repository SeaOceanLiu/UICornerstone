// CR20261113001：ContextMenu 设计态坐标不被 Popup 默认 Centered 重置
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "Bench.h"
#include "ContextMenu.h"
#include "Menu.h"
#include "PlatformUtils.h"
#include "TestUtils.h"
#include "TestInstance.h"
#include "UICornerstoneAPI.h"

using namespace std;

namespace {

int g_pass = 0;
int g_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { ++g_pass; printf("OK   %s\n", msg); } \
         else { ++g_fail; printf("FAIL %s\n", msg); } } while (0)

bool closeTo(float a, float b, float eps = 1.0f) { return std::fabs(a - b) <= eps; }

void setVisible(const shared_ptr<ContextMenu>& cm, bool on) {
    if (cm) cm->setBoolProperty(PropertyNames::kVisible, on ? 1 : 0);
}

void addItems(const shared_ptr<ContextMenu>& cm, int count = 2) {
    for (int i = 0; i < count; ++i) {
        string caption = (i == 0) ? "Open" : "Item";
        cm->addItem(caption, nullptr);
    }
}

void assertPanelSize(const shared_ptr<ContextMenu>& cm, const char* tag) {
    auto panel = cm ? cm->getMenuPanel() : nullptr;
    CHECK(panel != nullptr, tag);
    if (!panel) return;
    SRect pr = panel->getRect();
    SRect cr = cm->getRect();
    CHECK(closeTo(cr.width, pr.width) && closeTo(cr.height, pr.height), tag);
}

float centeredX(ContextMenu& cm) {
    SRect vp = g_uiInstance ? g_uiInstance->viewport : SRect(0, 0, 1400, 900);
    float scale = cm.getScaleXX();
    return (vp.left + (vp.width - cm.getRect().width * scale) * 0.5f) / 1.0f;
}

float centeredY(ContextMenu& cm) {
    SRect vp = g_uiInstance ? g_uiInstance->viewport : SRect(0, 0, 1400, 900);
    float scale = cm.getScaleYY();
    return (vp.top + (vp.height - cm.getRect().height * scale) * 0.5f) / 1.0f;
}

void runChecks() {
    // V2-1/V2-3：直接构造 + addItem + visible=1（不 create/show）
    auto direct = make_shared<ContextMenu>(nullptr);
    addItems(direct, 2);
    setVisible(direct, true);
    CHECK(direct->isPopupVisible(), "direct no-rect visible=1 opens");
    CHECK(direct->getMenuPanel() && direct->getMenuPanel()->isCreated(),
          "direct no-rect open creates MenuPanel");
    assertPanelSize(direct, "direct no-rect outer matches panel");
    CHECK(closeTo(direct->getRect().left, centeredX(*direct)) &&
          closeTo(direct->getRect().top, centeredY(*direct)),
          "direct no-rect opens centered with final size");
    float dx = direct->getRect().left, dy = direct->getRect().top;
    float dw = direct->getRect().width, dh = direct->getRect().height;
    setVisible(direct, false);
    setVisible(direct, true);
    CHECK(closeTo(direct->getRect().left, dx) && closeTo(direct->getRect().top, dy) &&
          closeTo(direct->getRect().width, dw) && closeTo(direct->getRect().height, dh),
          "direct no-rect repeated visible keeps centered size");
    setVisible(direct, false);
    // 无 rect + 内容尺寸变化后再 open：仍按新尺寸重新居中
    addItems(direct, 1);
    setVisible(direct, true);
    assertPanelSize(direct, "direct no-rect content change outer matches panel");
    CHECK(closeTo(direct->getRect().left, centeredX(*direct)) &&
          closeTo(direct->getRect().top, centeredY(*direct)),
          "direct no-rect content change re-centers");
    setVisible(direct, false);

    // P2/C1：显式 setRect 后 Absolute；setCentered/setAnchored no-op
    auto explicitPos = make_shared<ContextMenu>(BENCH);
    explicitPos->setRect(SRect(509, 407.5f, 140, 80));
    addItems(explicitPos, 2);
    explicitPos->setCentered();
    explicitPos->setAnchored(BENCH, SRect(1, 2, 3, 4));
    setVisible(explicitPos, true);
    CHECK(closeTo(explicitPos->getRect().left, 509) && closeTo(explicitPos->getRect().top, 407.5f),
          "explicit setRect keeps x/y after setCentered/setAnchored");
    assertPanelSize(explicitPos, "explicit setRect outer matches panel");
    explicitPos->setRect(SRect(620, 430, explicitPos->getRect().width, explicitPos->getRect().height));
    addItems(explicitPos, 1);
    CHECK(closeTo(explicitPos->getRect().left, 620) && closeTo(explicitPos->getRect().top, 430),
          "explicit move keeps x/y after addItem");
    assertPanelSize(explicitPos, "explicit move outer matches panel");
    setVisible(explicitPos, false);

    // C3：缩放/非空 parent
    auto scaled = make_shared<ContextMenu>(BENCH, 1.5f, 1.5f);
    scaled->setRect(SRect(100, 120, 60, 40));
    addItems(scaled, 2);
    setVisible(scaled, true);
    CHECK(closeTo(scaled->getRect().left, 100) && closeTo(scaled->getRect().top, 120),
          "1.5x scaled ContextMenu keeps explicit x/y");
    assertPanelSize(scaled, "1.5x outer matches panel");
    setVisible(scaled, false);

    // C4：设计态 visible 后再 show
    auto shown = make_shared<ContextMenu>(BENCH);
    shown->setRect(SRect(30, 40, 120, 60));
    addItems(shown, 2);
    setVisible(shown, true);
    shown->show(10, 10);
    CHECK(shown->isPopupVisible(), "visible then show stays visible");
    CHECK(shown->getRect().left >= 0 && shown->getRect().top >= 0,
          "visible then show clamps to viewport");
    shown->close();
    shown->show(100, 120);
    CHECK(shown->isPopupVisible(), "close then show reopens");
    shown->close();

    // C6：重复 open/close 不递归/不漂移
    auto repeat = make_shared<ContextMenu>(BENCH);
    repeat->setRect(SRect(300, 200, 140, 80));
    addItems(repeat, 2);
    for (int i = 0; i < 3; ++i) {
        setVisible(repeat, true);
        float x = repeat->getRect().left, y = repeat->getRect().top;
        float w = repeat->getRect().width, h = repeat->getRect().height;
        setVisible(repeat, false);
        setVisible(repeat, true);
        CHECK(closeTo(repeat->getRect().left, x) && closeTo(repeat->getRect().top, y) &&
              closeTo(repeat->getRect().width, w) && closeTo(repeat->getRect().height, h),
              "repeated visible open/close is stable");
        setVisible(repeat, false);
    }
    repeat->show(50, 60);
    repeat->close();
    repeat->show(70, 80);
    repeat->close();

    // C3：C ABI SetRect/SetBool/GetRect
    UIControlHandle h = UICornerstone_CreateContextMenu(g_uiInstance, 10, 20, 140, 80, 1, 1);
    CHECK(h != nullptr, "CABI CreateContextMenu");
    CHECK(UICornerstone_ContextMenuAddItem(g_uiInstance, h, "Open", "Ctrl+O") == 1,
          "CABI AddItem");
    UICornerstone_SetRect(g_uiInstance, h, 509, 407.5f, 140, 80);
    CHECK(UICornerstone_SetBool(g_uiInstance, h, "visible", 1) == 1, "CABI SetBool visible");
    float x = 0, y = 0, w = 0, hh = 0;
    UICornerstone_GetRect(g_uiInstance, h, &x, &y, &w, &hh);
    CHECK(closeTo(x, 509) && closeTo(y, 407.5f), "CABI SetRect x/y preserved");
    float px = -1, py = -1, pw = -1, ph = -1;
    CHECK(UICornerstone_ContextMenuGetMenuPanelRect(g_uiInstance, h, &px, &py, &pw, &ph) == 1,
          "CABI panel rect readable");
    CHECK(closeTo(w, pw) && closeTo(hh, ph), "CABI outer matches panel");
    UICornerstone_SetBool(g_uiInstance, h, "visible", 0);
    UICornerstone_SetBool(g_uiInstance, h, "visible", 1);
    UICornerstone_GetRect(g_uiInstance, h, &x, &y, &w, &hh);
    CHECK(closeTo(x, 509) && closeTo(y, 407.5f), "CABI repeated visible keeps x/y");
    UICornerstone_ContextMenuClose(g_uiInstance, h);

    // C3：C ABI 空菜单/无效句柄
    UIControlHandle empty = UICornerstone_CreateContextMenu(g_uiInstance, 30, 40, 120, 60, 1, 1);
    CHECK(empty != nullptr, "CABI CreateContextMenu empty");
    UICornerstone_SetBool(g_uiInstance, empty, "visible", 1);
    CHECK(UICornerstone_ContextMenuSyncSize(g_uiInstance, empty) == 0,
          "CABI empty SyncSize returns 0");
    CHECK(UICornerstone_ContextMenuSyncSize(g_uiInstance, nullptr) == 0,
          "CABI invalid SyncSize returns 0");
    UICornerstone_ContextMenuClose(g_uiInstance, empty);

    // P9：Popup 默认 Centered 行为不变
    auto popup = make_shared<Popup>(BENCH, SRect(0, 0, 100, 50));
    popup->setBoolProperty(PropertyNames::kVisible, 1);
    CHECK(popup->isPopupVisible(), "base Popup opens");
    {
        SRect vp = g_uiInstance ? g_uiInstance->viewport : SRect(0, 0, 1400, 900);
        float ex = vp.left + (vp.width - popup->getRect().width * popup->getScaleXX()) * 0.5f;
        CHECK(closeTo(popup->getRect().left, ex), "base Popup remains centered");
    }
    popup->close();

    // B1：JSON rect 持久化 + no-rect JSON 兼容
    CHECK(UICornerstone_LoadLayoutFromFile(g_uiInstance,
              (Platform::GetBasePath() + "layouts/cr_contextmenu_sync.json").c_str()) == 1,
          "load ContextMenu rect layout");
    UIControlHandle lh = UICornerstone_FindControl(g_uiInstance, "cmSync");
    CHECK(lh != nullptr, "find ContextMenu rect layout");
    if (lh) {
        UICornerstone_SetBool(g_uiInstance, lh, "visible", 1);
        UICornerstone_GetRect(g_uiInstance, lh, &x, &y, &w, &hh);
        CHECK(closeTo(x, 640) && closeTo(y, 260), "JSON rect x/y preserved");
        CHECK(UICornerstone_ContextMenuGetMenuPanelRect(g_uiInstance, lh, &px, &py, &pw, &ph) == 1,
              "JSON panel rect readable");
        CHECK(closeTo(w, pw) && closeTo(hh, ph), "JSON outer matches panel");
        UICornerstone_ContextMenuClose(g_uiInstance, lh);
    }

    const char* noRectJson = R"({"controls":[{"type":"context-menu","id":"cmNoRect","items":[{"caption":"A"},{"caption":"B"}]}]})";
    CHECK(UICornerstone_LoadLayout(g_uiInstance, noRectJson) == 1, "load no-rect JSON");
    UIControlHandle nh = UICornerstone_FindControl(g_uiInstance, "cmNoRect");
    CHECK(nh != nullptr, "find no-rect JSON ContextMenu");
    if (nh) {
        UICornerstone_SetBool(g_uiInstance, nh, "visible", 1);
        UICornerstone_GetRect(g_uiInstance, nh, &x, &y, &w, &hh);
        CHECK(x >= 0 && y >= 0, "no-rect JSON opens with valid position");
        UICornerstone_ContextMenuClose(g_uiInstance, nh);
    }

    printf("---- CR20261113001 contextmenu position: pass=%d fail=%d ----\n", g_pass, g_fail);
}

} // namespace

class ContextMenuPositionApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_cr_contextmenu_position");
        BENCH->setOnInitial([](shared_ptr<Bench>) { runChecks(); });
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
    }
    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    return TestRunMain<ContextMenuPositionApp>(argc, argv);
}
