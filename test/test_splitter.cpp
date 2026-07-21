#include <iostream>
#include <memory>
#include "Splitter.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<Panel> g_parent;
shared_ptr<Panel> g_first;
shared_ptr<Splitter> g_splitter;
shared_ptr<Panel> g_second;
shared_ptr<Label> g_statusLabel;

shared_ptr<Panel> g_hParent;
shared_ptr<Panel> g_hTop;
shared_ptr<Splitter> g_hSplitter;
shared_ptr<Panel> g_hBottom;

shared_ptr<Panel> g_2xParent;
shared_ptr<Panel> g_2xFirst;
shared_ptr<Splitter> g_2xSplitter;
shared_ptr<Panel> g_2xSecond;

void onSplitterMoved(shared_ptr<Splitter> sp, float ratio) {
    char buf[64];
    snprintf(buf, sizeof(buf), "V-Split ratio: %.3f", ratio);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void on2xSplitterMoved(shared_ptr<Splitter> sp, float ratio);
void testHorizontalSplitterInitialize();
void test2xSplitterInitialize();

void testSplitterInitialize() {
    TestUtil::log("testSplitterInitialize");

    // ▸ Vertical Splitter (left/right panels)
    g_parent = make_shared<Panel>(nullptr, SRect(10, 10, 350, 350));
    g_parent->setNormalStateBGColor(SColor(30, 30, 30, 255));
    g_parent->create();
    BENCH->addControl(g_parent);

    // First Panel (140x350)
    g_first = make_shared<Panel>(nullptr, SRect(0, 0, 140, 350));
    g_first->setNormalStateBGColor(SColor(45, 45, 45, 255));
    g_first->create();
    g_parent->addControl(g_first);

    // Splitter (6px wide, 350px tall)
    g_splitter = make_shared<Splitter>(nullptr, SRect(140, 0, 6, 350));
    g_splitter->setOrientation(true);
    g_splitter->setThickness(6);
    g_splitter->setMinSize(50, 50);
    g_splitter->setSplitRatio(0.4f);
    g_splitter->setOnSplitterMoved(onSplitterMoved);
    g_splitter->create();
    g_parent->addControl(g_splitter);

    // Second Panel (204x350)
    g_second = make_shared<Panel>(nullptr, SRect(146, 0, 204, 350));
    g_second->setNormalStateBGColor(SColor(45, 45, 45, 255));
    g_second->create();
    g_parent->addControl(g_second);

    // Link controls
    g_splitter->setLinkedControls(g_first, g_second);
    g_splitter->setSplitRatio(0.4f);

    // Labels for visual
    auto l1 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    l1->setCaption("Left Panel");
    l1->setFontSize(12);
    l1->setTextNormalStateColor(SColor(200, 200, 200, 255));
    l1->create();
    g_first->addControl(l1);

    auto l2 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    l2->setCaption("Right Panel");
    l2->setFontSize(12);
    l2->setTextNormalStateColor(SColor(200, 200, 200, 255));
    l2->create();
    g_second->addControl(l2);

    // Status label
    g_statusLabel = make_shared<Label>(nullptr, SRect(10, 375, 780, 24));
    g_statusLabel->setCaption("Drag splitters or use arrow keys (focused) | V-split (left) | H-split (top-right) | 2x scale (bottom-right)");
    g_statusLabel->setFontSize(11);
    g_statusLabel->setTextNormalStateColor(SColor(180, 180, 180, 255));
    g_statusLabel->create();
    BENCH->addControl(g_statusLabel);

    TestUtil::log("Splitter test controls created");

    testHorizontalSplitterInitialize();
    test2xSplitterInitialize();
}

void onHSplitterMoved(shared_ptr<Splitter> sp, float ratio) {
    char buf[64];
    snprintf(buf, sizeof(buf), "H-Split ratio: %.3f", ratio);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void on2xSplitterMoved(shared_ptr<Splitter> sp, float ratio) {
    char buf[64];
    snprintf(buf, sizeof(buf), "2x-Split ratio: %.3f", ratio);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void testHorizontalSplitterInitialize() {
    // Horizontal Splitter (top/bottom panels)
    g_hParent = make_shared<Panel>(nullptr, SRect(380, 10, 400, 200));
    g_hParent->setNormalStateBGColor(SColor(30, 30, 35, 255));
    g_hParent->create();
    BENCH->addControl(g_hParent);

    g_hTop = make_shared<Panel>(nullptr, SRect(0, 0, 400, 95));
    g_hTop->setNormalStateBGColor(SColor(50, 50, 55, 255));
    g_hTop->create();
    g_hParent->addControl(g_hTop);

    g_hSplitter = make_shared<Splitter>(nullptr, SRect(0, 95, 400, 6));
    g_hSplitter->setOrientation(false);
    g_hSplitter->setThickness(6);
    g_hSplitter->setMinSize(30, 30);
    g_hSplitter->setSplitRatio(0.5f);
    g_hSplitter->setOnSplitterMoved(onHSplitterMoved);
    g_hSplitter->create();
    g_hParent->addControl(g_hSplitter);

    g_hBottom = make_shared<Panel>(nullptr, SRect(0, 101, 400, 99));
    g_hBottom->setNormalStateBGColor(SColor(45, 45, 50, 255));
    g_hBottom->create();
    g_hParent->addControl(g_hBottom);

    g_hSplitter->setLinkedControls(g_hTop, g_hBottom);
    g_hSplitter->setSplitRatio(0.5f);

    auto htLbl = make_shared<Label>(nullptr, SRect(10, 10, 380, 20));
    htLbl->setCaption("Top Panel");
    htLbl->setFontSize(12);
    htLbl->setTextNormalStateColor(SColor(200, 200, 200, 255));
    htLbl->create();
    g_hTop->addControl(htLbl);

    auto hbLbl = make_shared<Label>(nullptr, SRect(10, 10, 380, 20));
    hbLbl->setCaption("Bottom Panel");
    hbLbl->setFontSize(12);
    hbLbl->setTextNormalStateColor(SColor(200, 200, 200, 255));
    hbLbl->create();
    g_hBottom->addControl(hbLbl);

    TestUtil::log("Horizontal splitter test controls created");
}

void test2xSplitterInitialize() {
    // 2X scale Splitter (children inherit parent's scale, use default 1.0)
    float s = 2.0f;
    float avail = 400.0f - 6.0f;
    float firstW = avail * 0.4f;  // 157.6
    g_2xParent = make_shared<Panel>(nullptr, SRect(380, 220, 400, 170), s, s);
    g_2xParent->setNormalStateBGColor(SColor(25, 35, 30, 255));
    g_2xParent->create();
    BENCH->addControl(g_2xParent);

    g_2xFirst = make_shared<Panel>(nullptr, SRect(0, 0, firstW, 170));
    g_2xFirst->setNormalStateBGColor(SColor(45, 55, 50, 255));
    g_2xFirst->create();
    g_2xParent->addControl(g_2xFirst);

    g_2xSplitter = make_shared<Splitter>(nullptr, SRect(firstW, 0, 6, 170));
    g_2xSplitter->setOrientation(true);
    g_2xSplitter->setThickness(6);
    g_2xSplitter->setMinSize(50, 50);
    g_2xSplitter->setOnSplitterMoved(on2xSplitterMoved);
    g_2xSplitter->create();
    g_2xParent->addControl(g_2xSplitter);

    g_2xSecond = make_shared<Panel>(nullptr, SRect(firstW + 6, 0, avail - firstW, 170));
    g_2xSecond->setNormalStateBGColor(SColor(55, 45, 50, 255));
    g_2xSecond->create();
    g_2xParent->addControl(g_2xSecond);

    g_2xSplitter->setLinkedControls(g_2xFirst, g_2xSecond);
    g_2xSplitter->setSplitRatio(0.4f);

    auto xlbl1 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    xlbl1->setCaption("2X Left");
    xlbl1->setFontSize(12);
    xlbl1->setTextNormalStateColor(SColor(200, 200, 200, 255));
    xlbl1->create();
    g_2xFirst->addControl(xlbl1);

    auto xlbl2 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    xlbl2->setCaption("2X Right");
    xlbl2->setFontSize(12);
    xlbl2->setTextNormalStateColor(SColor(200, 200, 200, 255));
    xlbl2->create();
    g_2xSecond->addControl(xlbl2);

    TestUtil::log("2X scale splitter test controls created");
}

class SplitterApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_splitter");
        BENCH->setOnInitial(testSplitterInitialize);
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
        TestUtil::log("Splitter test quit");
    }
};

int main(int argc, char* argv[]) {
    SplitterApp app;
    return MAINWIN->run(&app);
}
