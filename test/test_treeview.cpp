#include <iostream>
#include <filesystem>
#include <memory>
#include "TreeView.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "CheckBox.h"
#include "Actor.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "LayoutParser.h"
#include "PlatformUtils.h"
#include "TestUtils.h"
#include "TestInstance.h"
#include "StateMachine.h"

using namespace std;

shared_ptr<Panel> g_rootPanel;
shared_ptr<TreeView> g_treeView;
shared_ptr<Label> g_statusLabel;
shared_ptr<TreeView> g_treeView2x;
shared_ptr<TreeView> g_treeViewCollapsed;
shared_ptr<TreeView> g_treeViewStyled;
shared_ptr<TreeView> g_treeViewEnhanced;
shared_ptr<Label> g_dataLabel;

int g_selectCount = 0;
int g_expandCount = 0;
int g_collapseCount = 0;
string g_lastSelectedId;
int g_clearNodeCount = 0;
int g_enhancedCheckCount = 0;

void onEnhancedCheckChanged(shared_ptr<CheckBox>, CheckState, CheckState) {
    g_enhancedCheckCount++;
}

void onTreeSelect(shared_ptr<TreeView>, const string& nodeId) {
    g_selectCount++;
    g_lastSelectedId = nodeId;
    char buf[128];
    snprintf(buf, sizeof(buf), "Selected: %s (count: %d)", nodeId.c_str(), g_selectCount);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void onTreeExpand(shared_ptr<TreeView>, const string& nodeId) {
    g_expandCount++;
    cout << "Expanded: " << nodeId << endl;
}

void onTreeCollapse(shared_ptr<TreeView>, const string& nodeId) {
    g_collapseCount++;
    cout << "Collapsed: " << nodeId << endl;
}

void onTreeSelectData(shared_ptr<TreeView>, const string& nodeId, void* userData) {
    char buf[256];
    auto* s = static_cast<const string*>(userData);
    const char* data = s ? s->c_str() : "(null)";
    snprintf(buf, sizeof(buf), "Styled select: %s, data: %s", nodeId.c_str(), data);
    if (g_dataLabel) g_dataLabel->setCaption(buf);
    cout << buf << endl;
}

void onClearNode(shared_ptr<TreeView>, void* userData) {
    g_clearNodeCount++;
    const auto* s = static_cast<const string*>(userData);
    const char* data = s ? s->c_str() : "(null)";
    cout << "ClearNode called, userData: " << data << endl;
    delete s;
}

void initTestTree() {
    auto subChild2_1 = makeNode("btn1", "Button (btn1)");
    auto subChild2_2 = makeNode("subPnl2", "Panel (subPanel2)", false,
        { makeNode("inner1", "Inner 1") });
    auto subPanel = makeNode("subPnl", "Sub Panel", true,
        { subChild2_1, subChild2_2 });
    auto lbl1 = makeNode("lbl1", "Label (lbl1)");
    auto rootPanel = makeNode("rootPnl", "Root Panel", false,
        { subPanel, lbl1 });

    function<shared_ptr<TreeNode>(int)> deepChain;
    deepChain = [&](int level) -> shared_ptr<TreeNode> {
        if (level >= 20) return nullptr;
        auto child = deepChain(level + 1);
        vector<shared_ptr<TreeNode>> children;
        if (child) children.push_back(child);
        string label = "Level " + to_string(level);
        if (level < 3) {
            children.push_back(makeNode("sib_" + to_string(level), "Sibling " + to_string(level)));
        }
        return makeNode("deep_" + to_string(level), label, false, children);
    };
    auto deepRoot = deepChain(0);

    auto items = vector{ rootPanel, deepRoot };
    g_treeView->setItems(items);
}

void initTest2xTree() {
    auto items = g_treeView->getItems();
    vector<shared_ptr<TreeNode>> clonedItems;
    int tailIndex = 0;
    for (const auto& item : items) {
        auto cloned = cloneNode(item, "2x_", "2x ");
        int rootIdx = tailIndex;

        function<shared_ptr<TreeNode>(int)> deepTail;
        deepTail = [&](int level) -> shared_ptr<TreeNode> {
            if (level >= 8) return nullptr;
            auto child = deepTail(level + 1);
            vector<shared_ptr<TreeNode>> children;
            if (child) children.push_back(child);
            string label = "Lv" + to_string(level) + " long label for h-scroll test";
            return makeNode("2x_tail_" + to_string(rootIdx) + "_" + to_string(level), label, true, children);
        };
        cloned->children.push_back(deepTail(0));
        clonedItems.push_back(cloned);
        tailIndex++;
    }

    g_treeView2x->setItems(clonedItems);
}

void initTestCollapsed() {
    auto node = makeNode("coll_root", "Collapsed Root", false,
        { makeNode("coll_child", "Collapsed Child", false,
            { makeNode("coll_grand", "Collapsed Grandchild") }) });
    g_treeViewCollapsed->setDefaultExpand(false);
    g_treeViewCollapsed->setItems({ node });
}

void initTestStyled() {
    function<shared_ptr<TreeNode>(int)> deep;
    deep = [&](int level) -> shared_ptr<TreeNode> {
        if (level >= 5) return nullptr;
        auto child = deep(level + 1);
        vector<shared_ptr<TreeNode>> children;
        if (child) children.push_back(child);
        // Attach custom data (heap string) to each node
        auto node = makeNode("sty_" + to_string(level),
            "Sty " + to_string(level) + " (custom data)", true, children);
        string* data = new string("CustomData_Lv" + to_string(level));
        node->userData = data;
        return node;
    };
    vector<shared_ptr<TreeNode>> items;
    items.push_back(deep(0));

    // Additional root nodes with custom data
    for (int i = 0; i < 3; i++) {
        auto node = makeNode("sty_extra_" + to_string(i),
            "Styled Item " + to_string(i) + " with long label for horizontal scroll", false);
        string* data = new string("ExtraData_" + to_string(i));
        node->userData = data;
        items.push_back(node);
    }

    g_treeViewStyled->setItems(items);
}

// TreeView 增强（leadingControl + 逐节点字体）：演示 + 自动断言
void initTestEnhanced() {
    g_treeViewEnhanced = TreeViewBuilder(nullptr, SRect(800, 320, 300, 200))
        .setFontSize(12)
        .setOnSelect(onTreeSelect)
        .setId(104)
        .build();
    g_rootPanel->addControl(g_treeViewEnhanced);

    auto node1 = makeNode("enh_cb", "Checkbox Row");
    auto cb = CheckBoxBuilder(nullptr, SRect(0, 0, 16, 16))
        .setOnCheckChanged(onEnhancedCheckChanged)
        .build();
    node1->leadingControl = cb;

    auto node2 = makeNode("enh_big", "Big Font Row");
    node2->fontSize = 18;

    auto node3 = makeNode("enh_img", "Image Row");
    auto img = make_shared<Actor>(nullptr, fs::path("assets/images/cross_down.png"), false);
    node3->leadingControl = img;

    g_treeViewEnhanced->setItems({ node1, node2, node3 });

    int pass = 0, fail = 0;
    auto check = [&](bool ok, const char* name) {
        if (ok) { pass++; cout << "PASS: " << name << endl; }
        else { fail++; cout << "FAIL: " << name << endl; }
    };

    // 1. 行控件挂树（addControl 进 TreeView）
    check(cb->getParent() == g_treeViewEnhanced.get(), "enh row checkbox attached");
    check(img->getParent() == g_treeViewEnhanced.get(), "enh row image attached");

    // 先跑一帧 draw：刷新 m_frameDrawRect + 行控件 setRect（行高自适应）
    g_treeViewEnhanced->draw();

    // 5. 槽高 = 文字高度：图片与 CheckBox 等高、小于行高 24，行间留空隙；宽按 1:1 等比
    check(img->getTexture() != nullptr, "enh image texture loaded");
    check(img->getRect().height > 0 && img->getRect().height < 24.0f, "enh image height follows text height");
    check(img->getRect().width == img->getRect().height, "enh image scaled to text height");
    check(cb->getRect().height == img->getRect().height && cb->getRect().width == cb->getRect().height,
          "enh checkbox scaled to text height");
    // 6. 槽起点 = 原文本起点（LEFT_PADDING 4 + arrowGap 16 = 20），局部坐标非负（缩进不越界）
    check(img->getRect().left == 20.0f && cb->getRect().left == 20.0f,
          "enh slot aligned with original text indent");

    // 2. 点击 CheckBox 中心 → 选中该行 + 勾选翻转（MouseDown 不消费落子控件分发，MouseUp 触发翻转）
    SRect cbRect = cb->getDrawRect();
    float cx = cbRect.left + cbRect.width / 2;
    float cy = cbRect.top + cbRect.height / 2;
    auto down = make_shared<Event>(EventType::MouseDown);
    down->mouseButton = { cx, cy, MouseButton::Left };
    g_treeViewEnhanced->handleEvent(down);
    auto up = make_shared<Event>(EventType::MouseUp);
    up->mouseButton = { cx, cy, MouseButton::Left };
    g_treeViewEnhanced->handleEvent(up);
    check(g_lastSelectedId == "enh_cb", "enh click selects row");
    check(cb->getCheckState() == CheckState::Checked, "enh click toggles checkbox");
    check(g_enhancedCheckCount == 1, "enh check callback fired once");

    // 3. 移除节点 → 行控件摘除（事件不再响应：点击树内空白区不选中、不勾选）
    int selBefore = g_selectCount;
    int cbBefore = g_enhancedCheckCount;
    g_treeViewEnhanced->removeNode("enh_cb");
    auto down2 = make_shared<Event>(EventType::MouseDown);
    down2->mouseButton = { cx, cbRect.top + 150, MouseButton::Left };  // 树下方空白（第 3 行之后）
    auto up2 = make_shared<Event>(EventType::MouseUp);
    up2->mouseButton = { cx, cbRect.top + 150, MouseButton::Left };
    g_treeViewEnhanced->handleEvent(down2);
    g_treeViewEnhanced->handleEvent(up2);
    check(g_selectCount == selBefore, "enh detached row no longer selects");
    check(g_enhancedCheckCount == cbBefore, "enh detached row no longer toggles");

    // 4. cloneNode：leadingControl 置空 + 字体字段照常复制
    auto cloned = cloneNode(node1);
    check(cloned->leadingControl == nullptr, "enh cloneNode nulls leadingControl");
    check(cloned->fontSize == node1->fontSize && cloned->fontName == node1->fontName,
          "enh cloneNode copies font fields");

    cout << (fail ? "Enhanced assertions: FAILURES = " + to_string(fail)
                  : "Enhanced assertions: ALL PASS") << " (" << pass << " passed)" << endl;
}

// 2x 缩放断言（验收清单第 3 项）：布局矩形不变、绘制矩形翻倍、事件命中按逻辑坐标
static void initTestScale2x() {
    int pass = 0, fail = 0;
    auto check = [&](bool ok, const char* name) {
        if (ok) { pass++; cout << "PASS: " << name << endl; }
        else { fail++; cout << "FAIL: " << name << endl; }
    };
    // 1. 逻辑矩形不受缩放影响
    SRect lr = g_treeView2x->getRect();
    check(lr.left == 280.0f && lr.top == 10.0f && lr.width == 250.0f && lr.height == 300.0f,
          "2x treeview logic rect unchanged (280,10,250,300)");
    // 2. 绘制矩形 = 逻辑 × 2（父 rootPanel 位于 (10,10)）
    SRect dr = g_treeView2x->getDrawRect();
    check(dr.left == 290.0f && dr.top == 20.0f && dr.width == 500.0f && dr.height == 600.0f,
          "2x treeview drawRect doubled (290,20,500,600)");
    // 3. 事件命中按绘制坐标：点击第一行（2x 树 draw (310, 20+12×2=44)）→ 选中
    g_treeView2x->draw();
    float rowHalf = (g_treeView2x->getRowHeight()) / 2.0f;
    auto down = make_shared<Event>(EventType::MouseDown);
    down->mouseButton = { 290.0f + 60.0f, 20.0f + rowHalf * 2.0f, MouseButton::Left };
    g_treeView2x->handleEvent(down);
    auto up = make_shared<Event>(EventType::MouseUp);
    up->mouseButton = { 290.0f + 60.0f, 20.0f + rowHalf * 2.0f, MouseButton::Left };
    g_treeView2x->handleEvent(up);
    check(!g_treeView2x->getSelectedId().empty(), "2x treeview row hit by draw coords");
    // 清选：避免 2x 树首行残留选中高亮（用户目测对比时无蓝色底色）
    g_treeView2x->clearSelection();

    // 4. 增强对比：1x 与 2x 树各追加一行 leadingControl + 一行大字号（18），
    //    断言两者逻辑布局一致、2x 绘制尺寸翻倍
    auto lc1 = CheckBoxBuilder(nullptr, SRect(0, 0, 16, 16)).build();
    auto n1lc = makeNode("s_lc", "LC Row");
    n1lc->leadingControl = lc1;
    auto n1big = makeNode("s_big", "Big Font Row");
    n1big->fontSize = 18;
    auto items1 = g_treeView->getItems();
    items1.push_back(n1lc);
    items1.push_back(n1big);
    g_treeView->setItems(items1);

    auto lc2 = CheckBoxBuilder(nullptr, SRect(0, 0, 16, 16)).build();
    auto n2lc = makeNode("s2_lc", "s2 LC Row");
    n2lc->leadingControl = lc2;
    auto n2big = makeNode("s2_big", "s2 Big Font Row");
    n2big->fontSize = 18;
    auto items2 = g_treeView2x->getItems();
    items2.push_back(n2lc);
    items2.push_back(n2big);
    g_treeView2x->setItems(items2);

    g_treeView->draw();
    g_treeView2x->draw();

    check(lc1 && lc2 && lc1->getParent() == g_treeView.get() && lc2->getParent() == g_treeView2x.get(),
          "scale lc attached to both trees");
    check(lc1->getRect().left == 20.0f && lc2->getRect().left == 20.0f,
          "scale lc slot left == 20 (1x == 2x logic)");
    // 字体像素度量有取整噪声（如 15 vs 14.5），用 ≤1px 容差
    check(fabsf(lc1->getRect().width - lc2->getRect().width) <= 1.0f && lc1->getRect().width > 0,
          "scale lc size same logic (1x == 2x)");
    check(fabsf(lc2->getDrawRect().width - lc1->getDrawRect().width * 2.0f) <= 2.0f &&
          fabsf(lc2->getDrawRect().height - lc1->getDrawRect().height * 2.0f) <= 2.0f,
          "scale 2x lc draw size doubled vs 1x");
    // 垂直居中：槽位中心 = 所在行中心（1x/2x 均成立，2x 槽位不再垂直错位）。
    // s_lc 为顶层第 3 行（index=2）：rootPnl(折叠)1 行 + deep_0(折叠)1 行 + s_lc
    float lc1c = lc1->getDrawRect().top + lc1->getDrawRect().height / 2;
    float lc2c = lc2->getDrawRect().top + lc2->getDrawRect().height / 2;
    float row1c = g_treeView->getDrawRect().top + 2 * g_treeView->getRowHeight()
                  + g_treeView->getRowHeight() / 2;
    float row2c = g_treeView2x->getDrawRect().top + 2 * g_treeView2x->getRowHeight() * 2
                  + g_treeView2x->getRowHeight();
    check(fabsf(lc1c - row1c) <= 1.0f, "scale 1x lc vertically centered");
    check(fabsf(lc2c - row2c) <= 2.0f, "scale 2x lc vertically centered");

    // 逐项字号经 item-id 定位读取，1x/2x 一致
    g_treeView->setStringProperty("item-id", "s_big");
    int fs1 = 0;
    g_treeView->getIntProperty("item-font-size", fs1);
    g_treeView2x->setStringProperty("item-id", "s2_big");
    int fs2 = 0;
    g_treeView2x->getIntProperty("item-font-size", fs2);
    check(fs1 == 18 && fs2 == 18, "scale big row font size via item-id (1x == 2x)");

    // 5. 槽位对齐：item-leading-align = bottom-left → 2x 槽位贴行底（复用 Label 9 宫格语义）
    g_treeView2x->setStringProperty("item-id", "s2_lc");
    check(g_treeView2x->setEnumProperty("item-leading-align", "bottom-left"),
          "scale item-leading-align set");
    g_treeView2x->draw();
    const char* alignOut = "";
    check(g_treeView2x->getEnumProperty("item-leading-align", alignOut) &&
          alignOut && strcmp(alignOut, "bottom-left") == 0,
          "scale item-leading-align get (bottom-left)");
    float slotBottom = lc2->getDrawRect().top + lc2->getDrawRect().height;
    float rowBottom = row2c + g_treeView2x->getRowHeight();
    check(fabsf(slotBottom - rowBottom) <= 2.0f, "scale bottom-left slot flush to row bottom");
    // 还原居中，供目视对比（1x/2x 均居中）
    g_treeView2x->setEnumProperty("item-leading-align", "mid-left");
    g_treeView2x->draw();

    cout << (fail ? "Scale2x assertions: FAILURES = " + to_string(fail)
                  : "Scale2x assertions: ALL PASS") << " (" << pass << " passed)" << endl;
}

// JSON 用例：TreeView item 增加前置控件容器（CheckBox/Image）+ 逐 Item 字体（粗体）+ leadingGap
static const char* ENH_JSON = R"({
  "controls": [{
    "type": "panel",
    "id": "rootJsonEnh",
    "rect": { "x": 800, "y": 10, "w": 170, "h": 180 },
    "children": [{
      "type": "tree-view",
      "id": "jsonTreeEnh",
      "rect": { "x": 10, "y": 10, "w": 150, "h": 160 },
      "items": [
        { "id": "j1", "label": "JSON CB row",
          "leadingControl": { "type": "check-box", "checkState": "checked" },
          "leadingGap": 10 },
        { "id": "j2", "label": "JSON img row",
          "leadingControl": { "type": "image", "image": "assets/images/cross_down.png" },
          "font": "harmonyos-sans-sc-regular", "size": 16 },
        { "id": "j3", "label": "JSON align row",
          "leadingControl": { "type": "check-box" },
          "alignment": "bottom-left" }
      ]
    }]
  }]
})";

void initTestJsonEnh(Bench* bench) {
    LayoutParser parser;
    auto root = parser.parseLayout(ENH_JSON);
    if (!root) {
        TestUtil::log("json enh: FAILED to parse ENH_JSON");
        return;
    }
    bench->addControl(root);

    auto tv = dynamic_pointer_cast<TreeView>(parser.findControlById("jsonTreeEnh"));
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char* name) {
        if (cond) { pass++; cout << "PASS: " << name << endl; }
        else      { fail++; cout << "FAIL: " << name << endl; }
    };
    check(tv != nullptr, "json enh treeview parsed");
    if (!tv) {
        cout << (fail ? "json enh: FAILURES = " + to_string(fail) : "json enh: ALL PASS")
             << " (" << pass << " passed)" << endl;
        return;
    }

    // item 级字段
    auto j1 = tv->findNodeById("j1");
    auto j2 = tv->findNodeById("j2");
    auto j3 = tv->findNodeById("j3");
    check(j1 != nullptr && j2 != nullptr, "json enh items found");
    check(j1 && j1->leadingGap == 10.0f, "json enh item leadingGap");
    check(j2 && j2->fontName == FontName::HarmonyOS_Sans_SC_Regular, "json enh item regular font");
    check(j2 && j2->fontSize == 16, "json enh item font size");

    // 前置控件容器：挂树 + 类型 + 勾选态
    check(j1 && j1->leadingControl != nullptr, "json enh j1 leadingControl attached");
    if (j1 && j1->leadingControl) {
        check(j1->leadingControl->getParent() == tv.get(), "json enh j1 leadingControl attached to tree");
        auto cb = dynamic_pointer_cast<CheckBox>(j1->leadingControl);
        check(cb != nullptr && cb->getCheckState() == CheckState::Checked, "json enh j1 checkbox checked");
    }
    check(j2 && j2->leadingControl != nullptr, "json enh j2 leadingControl attached");
    if (j2 && j2->leadingControl) {
        auto img = dynamic_pointer_cast<Actor>(j2->leadingControl);
        tv->draw();
        check(img != nullptr && img->getTexture() != nullptr, "json enh j2 image texture loaded");
    }
    check(j3 && j3->leadingControl != nullptr && j3->leadingAlign == AlignmentMode::AM_BOTTOM_LEFT,
          "json enh j3 alignment parsed (bottom-left)");

    // CABI item 级属性（item-id 定位 → item-leading-gap / item-font-size / item-font）
    check(tv->setStringProperty(PropertyNames::kTreeItemId, "j2"), "json enh item-id set");
    float gap = 0; check(tv->getFloatProperty(PropertyNames::kTreeItemLeadingGap, gap) && gap == 6.0f,
                         "json enh item-leading-gap default 6");
    check(tv->setFloatProperty(PropertyNames::kTreeItemLeadingGap, 14.0f) &&
          tv->getFloatProperty(PropertyNames::kTreeItemLeadingGap, gap) && gap == 14.0f,
          "json enh item-leading-gap set/get");
    check(j2 && j2->leadingGap == 14.0f, "json enh item-leading-gap applied to node");
    check(tv->setIntProperty(PropertyNames::kTreeItemFontSize, 18), "json enh item-font-size set");
    int fs = 0;
    check(tv->getIntProperty(PropertyNames::kTreeItemFontSize, fs) && fs == 18 && j2->fontSize == 18,
          "json enh item-font-size applied");

    cout << (fail ? "json enh: FAILURES = " + to_string(fail) : "json enh: ALL PASS")
         << " (" << pass << " passed)" << endl;
}

void testAppInitialize(shared_ptr<Bench>) {
    TestUtil::log("testTreeViewInitialize");

    g_rootPanel = make_shared<Panel>(nullptr, SRect(10, 10, 780, 560));
    g_rootPanel->setNormalStateBGColor(SColor(30, 30, 30, 255));
    g_rootPanel->create();
    BENCH->addControl(g_rootPanel);

    // Normal TreeView
    g_treeView = TreeViewBuilder(nullptr, SRect(10, 10, 250, 300))
        .setOnSelect(onTreeSelect)
        .setOnExpand(onTreeExpand)
        .setOnCollapse(onTreeCollapse)
        .setFontSize(12)
        .setId(100)
        .build();
    g_rootPanel->addControl(g_treeView);

    // 2x scale TreeView — same rect as 1x for visual comparison
    g_treeView2x = TreeViewBuilder(nullptr,
        SRect(280, 10, 250, 300), 2.0f, 2.0f)
        .setFontSize(12)
        .setId(101)
        .build();
    g_rootPanel->addControl(g_treeView2x);

    // Collapsed TreeView (defaultExpand=false)
    g_treeViewCollapsed = TreeViewBuilder(nullptr, SRect(10, 320, 250, 100))
        .setDefaultExpand(false)
        .setId(102)
        .build();
    g_rootPanel->addControl(g_treeViewCollapsed);

    // Styled TreeView — custom colors, spacing, arrow gap, custom data
    g_treeViewStyled = TreeViewBuilder(nullptr, SRect(10, 430, 250, 110))
        .setBgColor(SColor(20, 30, 40, 255))
        .setBorderColor(SColor(0, 120, 215, 255))
        .setHoverColor(SColor(40, 60, 80, 255))
        .setSelectedColor(SColor(0, 100, 180, 255))
        .setTextColor(SColor(200, 230, 255, 255))
        .setRowHeight(28)
        .setLineSpacing(4)
        .setArrowGap(24)
        .setIndentWidth(20)
        .setFontSize(12)
        .setOnSelectData(onTreeSelectData)
        .setOnClearNode(onClearNode)
        .setId(103)
        .build();
    g_rootPanel->addControl(g_treeViewStyled);

    // Status labels
    g_statusLabel = make_shared<Label>(nullptr, SRect(10, 550, 270, 20));
    g_statusLabel->setCaption("TreeView test — click nodes to select, arrows to expand/collapse");
    g_statusLabel->setFontSize(11);
    g_statusLabel->setTextNormalStateColor(SColor(180, 180, 180, 255));
    g_statusLabel->create();
    BENCH->addControl(g_statusLabel);

    g_dataLabel = make_shared<Label>(nullptr, SRect(10, 575, 270, 20));
    g_dataLabel->setCaption("Custom data display");
    g_dataLabel->setFontSize(11);
    g_dataLabel->setTextNormalStateColor(SColor(180, 220, 180, 255));
    g_dataLabel->create();
    BENCH->addControl(g_dataLabel);

    TestUtil::log("TreeView test controls created");

    // Initialize tree data
    initTestTree();
    initTest2xTree();
    initTestCollapsed();
    initTestStyled();
    initTestEnhanced();
    initTestJsonEnh(BENCH);
    initTestScale2x();

    TestUtil::log("TreeView test data initialized");
}

class TreeViewApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_treeview");
        BENCH->setOnInitial(testAppInitialize);
        return true;
    }

    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }

    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(40.0f/255.0f, 40.0f/255.0f, 40.0f/255.0f, 1.0f));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
    }

    void onQuit() override {
        TestUtil::log("TreeView test quit");
        cout << "Select count: " << g_selectCount << endl;
        cout << "Expand count: " << g_expandCount << endl;
        cout << "Collapse count: " << g_collapseCount << endl;
        cout << "ClearNode count: " << g_clearNodeCount << endl;
    }
};

int main(int argc, char* argv[]) {
    return TestRunMain<TreeViewApp>(argc, argv);
}
