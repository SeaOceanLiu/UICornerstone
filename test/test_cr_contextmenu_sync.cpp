// CR20261112002：ContextMenu 外框尺寸随 MenuPanel 自动同步
// 覆盖：状态矩阵 S1~S9、字体/行高/缩放、C ABI、布局 rect 持久化、生命周期。
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

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

bool closeTo(float a, float b) { return a > b - 0.5f && a < b + 0.5f; }

shared_ptr<ContextMenu> makeMenu(float scale = 1.0f) {
    return make_shared<ContextMenu>(BENCH, scale, scale);
}

void addItem(const shared_ptr<ContextMenu>& cm, const char* caption, const char* shortcut) {
    auto item = make_shared<MenuItem>(cm->getMenuPanel().get(), MenuItemType::Normal);
    item->setCaption(caption);
    if (shortcut && shortcut[0]) item->setShortcut(shortcut);
    cm->addItem(item);
}

void checkOuterMatchesPanel(const shared_ptr<ContextMenu>& cm, const char* tag) {
    auto panel = cm ? cm->getMenuPanel() : nullptr;
    CHECK(panel != nullptr, tag);
    if (!panel) return;
    SRect pr = panel->getRect();
    SRect cr = cm->getRect();
    CHECK(closeTo(cr.width, pr.width) && closeTo(cr.height, pr.height), tag);
}

void runChecks() {
    // S1：空 items 保持手动 rect；syncSize 返回 false；面板局部矩形 0×0
    auto empty = makeMenu();
    empty->setRect(SRect(300, 300, 120, 60));
    CHECK(closeTo(empty->getRect().width, 120) && closeTo(empty->getRect().height, 60),
          "S1 empty menu keeps manual rect");
    CHECK(empty->syncSize() == false, "S7 empty syncSize returns false");
    SRect er = empty->getMenuPanelRect();
    CHECK(er.width == 0.0f && er.height == 0.0f, "N1 empty panel rect is 0,0,0,0");
    CHECK(closeTo(empty->getRect().width, 120), "S7 empty syncSize keeps outer rect");

    // S2/S3/S8/S9：首次添加、separator、非空 setRect、显式 syncSize
    auto cm = makeMenu();
    cm->setRect(SRect(500, 500, 140, 80));
    addItem(cm, "Open", "Ctrl+O");
    addItem(cm, "Save As...", "Ctrl+Shift+S");
    CHECK(closeTo(cm->getRect().left, 500) && closeTo(cm->getRect().top, 500),
          "S2 addItem keeps x/y");
    checkOuterMatchesPanel(cm, "S2 first items sync outer size");
    float firstW = cm->getRect().width;
    cm->addSeparator();
    checkOuterMatchesPanel(cm, "S3 separator syncs outer size");
    CHECK(cm->getRect().width >= firstW, "S3 separator keeps/increases width");

    cm->setRect(SRect(620, 430, 999, 999));
    CHECK(closeTo(cm->getRect().left, 620) && closeTo(cm->getRect().top, 430),
          "S8 non-empty setRect keeps x/y");
    checkOuterMatchesPanel(cm, "S8 non-empty setRect w/h from content");
    CHECK(cm->syncSize() == true, "S9 non-empty syncSize returns true");
    checkOuterMatchesPanel(cm, "S9 syncSize result matches panel");

    // 字体/行高/字体名：全部经 MenuPanel 重算回调同步
    auto panel = cm->getMenuPanel();
    cm->create();
    panel->setFontSize(20.0f);
    checkOuterMatchesPanel(cm, "S3 fontSize syncs outer size");
    panel->setItemHeightRatio(1.8f);
    checkOuterMatchesPanel(cm, "S3 itemHeightRatio syncs outer size");
    panel->setFontName(FontName::HarmonyOS_Sans_SC_Regular);
    checkOuterMatchesPanel(cm, "S3 fontName syncs outer size");

    // S4/S5：删除全部保持“最后一次非空尺寸”；再次添加使用新内容尺寸
    float lastW = cm->getRect().width;
    float lastH = cm->getRect().height;
    while (!panel->getItems().empty()) {
        if (panel->getItems().size() == 1) {
            lastW = cm->getRect().width;
            lastH = cm->getRect().height;
        }
        panel->removeItem(panel->getItems().front());
    }
    CHECK(closeTo(cm->getRect().width, lastW) && closeTo(cm->getRect().height, lastH),
          "S4 remove all keeps last non-empty outer size");
    CHECK(cm->syncSize() == false, "S4 empty syncSize returns false");
    addItem(cm, "New", "Ctrl+N");
    CHECK(cm->getRect().width > 0.0f && cm->getRect().height > 0.0f,
          "S5 re-add syncs non-zero size");
    checkOuterMatchesPanel(cm, "S5 re-add matches content");

    // S6：空菜单 setRect 接受 w/h
    auto empty2 = makeMenu();
    empty2->setRect(SRect(10, 20, 222, 77));
    CHECK(closeTo(empty2->getRect().width, 222) && closeTo(empty2->getRect().height, 77),
          "S6 empty setRect accepts w/h");

    // 缩放：1.5x 实例外框与面板一致
    auto scaled = makeMenu(1.5f);
    scaled->setRect(SRect(100, 100, 50, 50));
    addItem(scaled, "Scale", "Ctrl+1");
    checkOuterMatchesPanel(scaled, "S3 scale instance matches panel");

    // 生命周期：外部持有 MenuPanel，销毁 ContextMenu 后重算不崩溃
    auto owner = makeMenu();
    addItem(owner, "Temp", "");
    auto heldPanel = owner->getMenuPanel();
    owner.reset();
    heldPanel->recalculateSize();
    CHECK(true, "B2 held panel after ContextMenu destruction is safe");

    // C ABI：syncSize/getMenuPanelRect + 无效/空菜单返回值
    UIControlHandle h = UICornerstone_CreateContextMenu(g_uiInstance, 500, 500, 140, 80, 1, 1);
    CHECK(h != nullptr, "CABI CreateContextMenu");
    CHECK(UICornerstone_ContextMenuAddItem(g_uiInstance, h, "Open", "Ctrl+O") == 1,
          "CABI AddItem");
    CHECK(UICornerstone_ContextMenuSyncSize(g_uiInstance, h) == 1, "CABI SyncSize non-empty");
    float px = -1, py = -1, pw = -1, ph = -1;
    CHECK(UICornerstone_ContextMenuGetMenuPanelRect(g_uiInstance, h, &px, &py, &pw, &ph) == 1,
          "CABI GetMenuPanelRect");
    CHECK(closeTo(px, 0) && closeTo(py, 0) && pw > 0 && ph > 0, "CABI panel local rect");
    float ox = -1, oy = -1, ow = -1, oh = -1;
    UICornerstone_GetRect(g_uiInstance, h, &ox, &oy, &ow, &oh);
    CHECK(closeTo(ox, 500) && closeTo(oy, 500), "CABI outer x/y preserved");
    CHECK(closeTo(ow, pw) && closeTo(oh, ph), "CABI outer size equals panel size");

    UIControlHandle emptyH = UICornerstone_CreateContextMenu(g_uiInstance, 20, 20, 120, 60, 1, 1);
    CHECK(emptyH != nullptr, "CABI CreateContextMenu empty");
    CHECK(UICornerstone_ContextMenuSyncSize(g_uiInstance, emptyH) == 0,
          "CABI SyncSize empty returns 0");
    CHECK(UICornerstone_ContextMenuGetMenuPanelRect(g_uiInstance, emptyH, &px, &py, &pw, &ph) == 1,
          "CABI GetMenuPanelRect empty still succeeds");
    CHECK(pw == 0 && ph == 0, "CABI empty panel rect 0,0,0,0");
    CHECK(UICornerstone_ContextMenuSyncSize(g_uiInstance, nullptr) == 0,
          "CABI SyncSize invalid handle returns 0");

    // B1：含 rect + items 的布局加载后 x/y 保留、外框=内容
    CHECK(UICornerstone_LoadLayoutFromFile(g_uiInstance,
              (Platform::GetBasePath() + "layouts/cr_contextmenu_sync.json").c_str()) == 1,
          "B1 load context-menu layout");
    UIControlHandle lh = UICornerstone_FindControl(g_uiInstance, "cmSync");
    CHECK(lh != nullptr, "B1 find context-menu");
    if (lh) {
        UICornerstone_GetRect(g_uiInstance, lh, &ox, &oy, &ow, &oh);
        CHECK(closeTo(ox, 640) && closeTo(oy, 260), "B1 rect x/y persisted");
        CHECK(UICornerstone_ContextMenuGetMenuPanelRect(g_uiInstance, lh, &px, &py, &pw, &ph) == 1,
              "B1 layout panel rect readable");
        CHECK(closeTo(ow, pw) && closeTo(oh, ph), "B1 layout outer matches panel");
    }

    printf("---- CR20261112002 contextmenu sync: pass=%d fail=%d ----\n", g_pass, g_fail);
}

} // namespace

class ContextMenuSyncApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_cr_contextmenu_sync");
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
    return TestRunMain<ContextMenuSyncApp>(argc, argv);
}
