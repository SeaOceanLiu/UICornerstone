#include <iostream>
#include <memory>
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "EditBox.h"
#include "TextArea.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<Label> g_label1;
shared_ptr<Label> g_label2;
shared_ptr<Label> g_label3;
shared_ptr<Label> g_label4;
shared_ptr<Label> g_label5;
shared_ptr<Label> g_label6;
shared_ptr<Label> g_label7;
shared_ptr<Label> g_label8;
shared_ptr<Label> g_label9;
shared_ptr<Label> g_label10;
shared_ptr<Label> g_label11;
shared_ptr<Label> g_label12;
shared_ptr<Label> g_label13;
shared_ptr<Label> g_label14;
shared_ptr<Label> g_label15;
shared_ptr<Label> g_label16;
shared_ptr<Label> g_label17;
shared_ptr<Label> g_label18;
shared_ptr<Label> g_label19;

void testBenchInitialize(void) {
    TestUtil::log("testLabelInitialize");

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    // Row 1: Single line alignment (y=30, height=30)
    g_label1 = LabelBuilder(nullptr, SRect(20, 30, 130, 30))
        .setCaption("L1: Left")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        .setId(1)
        .build();
    BENCH->addControl(g_label1);

    g_label2 = LabelBuilder(nullptr, SRect(160, 30, 130, 30))
        .setCaption("L2: Right")
        .setAlignmentMode(AlignmentMode::AM_TOP_RIGHT)
        .setBorderStateColor(redBorder)
        .setId(2)
        .build();
    BENCH->addControl(g_label2);

    g_label3 = LabelBuilder(nullptr, SRect(300, 30, 130, 30))
        .setCaption("L3: Center")
        .setAlignmentMode(AlignmentMode::AM_TOP_CENTER)
        .setBorderStateColor(redBorder)
        .setId(3)
        .build();
    BENCH->addControl(g_label3);

    // Row 2: Single line truncate (y=80, height=25)
    g_label4 = LabelBuilder(nullptr, SRect(20, 80, 70, 25))
        .setCaption("L4: long text 70px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(4)
        .build();
    BENCH->addControl(g_label4);

    g_label5 = LabelBuilder(nullptr, SRect(100, 80, 40, 25))
        .setCaption("L5: long t 40px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(5)
        .build();
    BENCH->addControl(g_label5);

    g_label6 = LabelBuilder(nullptr, SRect(150, 80, 35, 25))
        .setCaption("L6: long 35px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(6)
        .build();
    BENCH->addControl(g_label6);

    g_label7 = LabelBuilder(nullptr, SRect(195, 80, 30, 25))
        .setCaption("L7: long 30px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(7)
        .build();
    BENCH->addControl(g_label7);

    g_label8 = LabelBuilder(nullptr, SRect(235, 80, 20, 25))
        .setCaption("L8: lo 20px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(8)
        .build();
    BENCH->addControl(g_label8);

    // Row 3: Multi-line (y=125, height=50)
    g_label9 = LabelBuilder(nullptr, SRect(20, 125, 110, 60))
        .setCaption(u8"L9: Line1\nLine2\nLine3")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(9)
        .build();
    BENCH->addControl(g_label9);

    g_label10 = LabelBuilder(nullptr, SRect(140, 125, 90, 95))
        .setEnableExpand(false)
        .setCaption(u8"L10: L1\nL2\nL3\nL4")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        .setId(10)
        .build();
    BENCH->addControl(g_label10);

    // Row 4: 2x scaled Chinese top alignment (y=200, height=70 -> 140 with scale)
    g_label11 = LabelBuilder(nullptr, SRect(20, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L11: Top左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(11)
        .build();
    BENCH->addControl(g_label11);

    g_label12 = LabelBuilder(nullptr, SRect(450, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L12: Top中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(12)
        .build();
    BENCH->addControl(g_label12);

    g_label13 = LabelBuilder(nullptr, SRect(880, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L13: Top右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(13)
        .build();
    BENCH->addControl(g_label13);

    // Row 5: 2x scaled Chinese middle alignment (y=400, after 140+60 gap)
    g_label14 = LabelBuilder(nullptr, SRect(20, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L14: Mid左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(14)
        .build();
    BENCH->addControl(g_label14);

    g_label15 = LabelBuilder(nullptr, SRect(450, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L15: Mid中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(15)
        .build();
    BENCH->addControl(g_label15);

    g_label16 = LabelBuilder(nullptr, SRect(880, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L16: Mid右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_MID_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(16)
        .build();
    BENCH->addControl(g_label16);

    // Row 6: 2x scaled Chinese bottom alignment (y=600, after 140+60 gap)
    g_label17 = LabelBuilder(nullptr, SRect(20, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L17: Bot左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(17)
        .build();
    BENCH->addControl(g_label17);

    g_label18 = LabelBuilder(nullptr, SRect(450, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L18: Bot中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(18)
        .build();
    BENCH->addControl(g_label18);

    g_label19 = LabelBuilder(nullptr, SRect(880, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L19: Bot右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(19)
        .build();
    BENCH->addControl(g_label19);

    TestUtil::log("Label test controls created");
}

class LabelApp : public AppCallbacks {
public:
    bool onInit() override {
        BENCH->setOnInitial(testBenchInitialize);
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
        TestUtil::log("Application quit");
    }
};

int main(int argc, char* argv[]) {
    LabelApp app;
    return MAINWIN->run(&app);
}
