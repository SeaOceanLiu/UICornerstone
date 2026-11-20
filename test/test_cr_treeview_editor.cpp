// CR20261117002：TreeView 行内编辑器 / 逐节点行高 / 标签文本列宽
#include <cmath>
#include <cstdio>
#include <memory>
#include <cstring>
#include <string>
#include <vector>

#include "Bench.h"
#include "CheckBox.h"
#include "ScrollBar.h"
#include "EditBox.h"
#include "TestInstance.h"
#include "TestUtils.h"
#include "TextArea.h"
#include "TreeView.h"
#include "UICornerstoneAPI.h"

using namespace std;

namespace {
int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) \
    do { if (cond) { ++g_pass; printf("OK   %s\n", msg); } \
         else { ++g_fail; printf("FAIL %s\n", msg); } } while (0)

shared_ptr<TreeView> g_tv;
shared_ptr<EditBox> g_edit;
shared_ptr<TextArea> g_area;
shared_ptr<CheckBox> g_check;
UIControlHandle g_handle = nullptr;
bool g_done = false;
int g_frame = 0;
std::string g_shotPath;

bool near(float a, float b, float eps = 0.6f) { return std::fabs(a - b) <= eps; }
uint32_t pixelAt(int x, int y) {
    BENCH->getRenderDevice()->flush();
    SRect r((float)x, (float)y, 1, 1);
    uint32_t p = 0;
    BENCH->getRenderDevice()->readPixels(&p, r);
    return p;
}

void runChecks() {
    CHECK(g_tv && g_edit && g_area, "T1 controls created");
    if (!g_tv) return;

    SRect er = g_edit->getRect();
    SRect ar = g_area->getRect();
    CHECK(er.width > 10 && er.height > 5, "T1 editor has non-zero rect");
    CHECK(er.left > 100, "T1 editor starts after label column");
    CHECK(near(er.left, ar.left, 1.0f), "T18 cross-level editor column aligned");

    SRect rRoot, rChild, rArea;
    CHECK(g_tv->getNodeRect("root", rRoot), "T9 GetNodeRect root");
    CHECK(g_tv->getNodeRect("child1", rChild), "T9 GetNodeRect child1");
    CHECK(g_tv->getNodeRect("area", rArea), "T9 GetNodeRect textarea row");
    CHECK(near(rRoot.height, 26.0f) && near(rChild.height, 26.0f), "T4 inherited row height");
    CHECK(near(rArea.height, 70.0f), "T4 per-node row height");
    CHECK(g_tv->hitTestRow(40, 30) == 0, "T1 hit root row");
    CHECK(g_tv->hitTestRow(40, 110) >= 0, "T7 hit lower row");

    // T21：最深行标签列仍有文本空间；editor 列不压标签
    CHECK(near(er.left, ar.left, 1.0f), "T21 deep row editor same column");

    // C ABI：layout/label width/row height/GetNodeRect
    UIControlHandle h = static_cast<UIControlHandle>(static_cast<Control*>(g_tv.get()));
    CHECK(UICornerstone_TreeViewSetLabelWidth(g_uiInstance, h, 140.0f) == 1, "T10 SetLabelWidth");
    float x = -1, y = -1, w = -1, hh = -1;
    CHECK(UICornerstone_TreeViewGetNodeRect(g_uiInstance, h, "range", &x, &y, &w, &hh) == 1,
          "T10 GetNodeRect via C ABI");
    CHECK(w > 0 && hh > 0, "T10 GetNodeRect dimensions");
    CHECK(UICornerstone_TreeViewSetNodeEditorLayout(g_uiInstance, h, "child1", 90.0f, 8.0f) == 1,
          "T10 SetNodeEditorLayout");
    CHECK(UICornerstone_TreeViewSetNodeEditorAlign(g_uiInstance, h, "child1", "right") == 1,
          "T10 SetNodeEditorAlign");
    CHECK(UICornerstone_TreeViewSetNodeRowHeight(g_uiInstance, h, "range", 80.0f) == 1,
          "T10 SetNodeRowHeight");
    CHECK(UICornerstone_TreeViewSetNodeEditorControl(g_uiInstance, h, "child1", nullptr) == 1,
          "T10 editor NULL unbind");
    CHECK(UICornerstone_TreeViewSetNodeEditorControl(g_uiInstance, h, "nope", nullptr) == 0,
          "T10 invalid id returns 0");

    // T24：横向滚动偏移进入 GetNodeRect 逻辑坐标
    if (auto hs = g_tv->getHScrollBar(); hs && hs->getVisible()) {
        hs->setValue(20.0f);
        SRect hr;
        CHECK(g_tv->getNodeRect("range", hr) && near(hr.left, -20.0f, 1.0f),
              "T24 GetNodeRect includes horizontal scroll");
        hs->setValue(0.0f);
    }

    // T26：逐节点行高下的 PageUp/PageDown
    {
        g_tv->setFocused(true);
        g_tv->selectNode("range");
        int before = g_tv->getSelectedRow();
        auto pgup = make_shared<Event>(EventType::KeyDown);
        pgup->keyEvent.keycode = KeyCode::PageUp;
        g_tv->handleEvent(pgup);
        int afterUp = g_tv->getSelectedRow();
        auto pgdn = make_shared<Event>(EventType::KeyDown);
        pgdn->keyEvent.keycode = KeyCode::PageDown;
        g_tv->handleEvent(pgdn);
        int afterDown = g_tv->getSelectedRow();
        CHECK(afterUp < before && afterDown > afterUp, "T26 PageUp/PageDown use variable row heights");
    }

    // T27：行尾 spacing 选中高亮（行槽背景）
    {
        SRect rr;
        if (g_tv->getNodeRect("range", rr)) {
            float sx = g_tv->getScaleXX(), sy = g_tv->getScaleYY();
            SRect dr = g_tv->getDrawRect();
            int px = (int)(dr.left + 6 * sx);
            int py = (int)(dr.top + (rr.top + rr.height + 2.0f) * sy);
            uint32_t gapPx = pixelAt(px, py);
            uint32_t bgPx = pixelAt(5, 5);
            CHECK(gapPx != bgPx, "T27 selected row background covers lineSpacing slot");
        }
    }

    // T28/CR20261119001：点击编辑器派发（CheckBox 切换、EditBox 聚焦、树行不选中）
    {
        g_tv->setNodeEditorControl("child1", g_edit.get());   // T10 曾解绑
        g_tv->selectNode("root");
        const std::string selBefore = g_tv->getSelectedId();
        auto clickAt = [&](Control* c) {
            SRect dr = g_tv->getDrawRect();
            SRect er = c->getRect();
            float x = dr.left + er.left * g_tv->getScaleXX() + er.width * g_tv->getScaleXX() * 0.5f;
            float y = dr.top + er.top * g_tv->getScaleYY() + er.height * g_tv->getScaleYY() * 0.5f;
            auto down = make_shared<Event>(EventType::MouseDown);
            down->mouseButton.button = MouseButton::Left;
            down->mouseButton.x = x; down->mouseButton.y = y;
            g_tv->handleEvent(down);
            auto up = make_shared<Event>(EventType::MouseUp);
            up->mouseButton.button = MouseButton::Left;
            up->mouseButton.x = x; up->mouseButton.y = y;
            g_tv->handleEvent(up);
        };
        if (g_check) {
            CheckState before = g_check->getCheckState();
            clickAt(g_check.get());
            CHECK(g_check->getCheckState() != before, "T28 CheckBox editor toggles on click");
            CHECK(g_tv->getSelectedId() == selBefore, "T28 editor click does not change tree selection");
        }
        if (g_edit) {
            clickAt(g_edit.get());
            CHECK(g_edit->getFocused(), "T28 EditBox editor receives focus on click");
        }
        // 编辑器外行点击：仍正常选中行
        SRect rr;
        if (g_tv->getNodeRect("range", rr)) {
            SRect dr = g_tv->getDrawRect();
            float x = dr.left + 8.0f * g_tv->getScaleXX();
            float y = dr.top + (rr.top + rr.height * 0.5f) * g_tv->getScaleYY();
            auto down = make_shared<Event>(EventType::MouseDown);
            down->mouseButton.button = MouseButton::Left;
            down->mouseButton.x = x; down->mouseButton.y = y;
            g_tv->handleEvent(down);
            auto up = make_shared<Event>(EventType::MouseUp);
            up->mouseButton.button = MouseButton::Left;
            up->mouseButton.x = x; up->mouseButton.y = y;
            g_tv->handleEvent(up);
            CHECK(g_tv->getSelectedId() == "range", "T28 non-editor row click still selects");
        }
    }

    // T17：非法/折叠节点
    CHECK(!g_tv->getNodeRect("nope", rRoot), "T17 invalid id");
    CHECK(!g_tv->getNodeRect("deepHidden", rRoot), "T17 collapsed child invalid");

    printf("---- CR20261117002 treeview editor: pass=%d fail=%d ----\n", g_pass, g_fail);
}
} // namespace

class TreeEditorApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_cr_treeview_editor");
        BENCH->setOnInitial([](shared_ptr<Bench>) {
            g_tv = make_shared<TreeView>(nullptr, SRect(20, 20, 320, 300));
            g_tv->setRowHeight(26.0f);
            g_tv->setIndentWidth(16.0f);
            g_tv->setLabelWidth(120.0f);
            g_tv->setLineSpacing(6.0f);
            g_handle = static_cast<UIControlHandle>(static_cast<Control*>(g_tv.get()));
            BENCH->addControl(g_tv);

            auto child1 = makeNode("child1", "Field");
            auto hidden = makeNode("hiddenChild", "Hidden");
            auto root = makeNode("root", "Group", true, { hidden, child1 });
            auto area = makeNode("area", "TextAreaRow");
            area->rowHeight = 70.0f;
            auto range = makeNode("range", std::string("Range ") + std::string(180, 'x'));
            auto collapsed = makeNode("collapsed", "Collapsed", false, { makeNode("deepHidden", "Deep") });
            g_tv->addRootItem(root);
            g_tv->addRootItem(area);
            g_tv->addRootItem(range);
            g_tv->addRootItem(collapsed);

            g_edit = make_shared<EditBox>(nullptr, SRect(0, 0, 120, 22));
            g_tv->setNodeEditorControl("child1", g_edit.get());

            g_area = make_shared<TextArea>(nullptr, SRect(0, 0, 160, 50));
            g_tv->setNodeEditorControl("area", g_area.get());
            g_check = make_shared<CheckBox>(nullptr, SRect(0, 0, 18, 18));
            g_tv->setNodeEditorControl("range", g_check.get());
        });
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
        ++g_frame;
        if (g_frame == 3 && g_handle) {
            g_tv->selectNode("range");
        }
    }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(30, 30, 34));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
        if (g_frame == 4 && !g_done) { g_done = true; runChecks(); }
        if (!g_shotPath.empty() && g_frame >= 6) {
            int cw = 0, ch = 0;
            std::vector<uint8_t> cap(1400u * 900u * 4u);
            if (UICornerstone_CaptureViewport(g_uiInstance, cap.data(), &cw, &ch) && cw > 0 && ch > 0)
                UICornerstone_SavePixelsToFile(cap.data(), cw, ch, g_shotPath.c_str());
            g_shotPath.clear();
        }
    }
    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i)
        if (std::strncmp(argv[i], "shot=", 5) == 0) g_shotPath = argv[i] + 5;
    return TestRunMain<TreeEditorApp>(argc, argv);
}
