#include <iostream>
#include <memory>
#include "WinFrame.h"
#include "MainWindow.h"
#include "Bench.h"
#include "Label.h"
#include "Button.h"
#include "EditBox.h"
#include "AppCallbacks.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<WinFrame> g_winFrame1;
shared_ptr<WinFrame> g_winFrame2;
shared_ptr<Button> g_btn1;
shared_ptr<Button> g_btn2;

void testBenchInitialize(void) {
    TestUtil::log("testWinFrameInitialize");

    StateColor defaultBtnBG(StateColor::Type::Background);
    defaultBtnBG.setNormal({80, 80, 80, 255})
                 .setHover({110, 110, 110, 255})
                 .setPressed({60, 60, 60, 255});

    g_winFrame1 = WinFrameBuilder(nullptr, SRect(100, 100, 400, 300))
        .setTitle("Resizable WinFrame")
        .setTitleFontSize(24)
        .setTitleTextColor(SColor((uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)255))
        .setTitleBarBGColor(SColor((uint8_t)70, (uint8_t)130, (uint8_t)180, (uint8_t)255))
        .setWinFrameBGColor(SColor((uint8_t)50, (uint8_t)50, (uint8_t)50, (uint8_t)255))
        .setWinFrameBorderColor(SColor((uint8_t)100, (uint8_t)100, (uint8_t)100, (uint8_t)255))
        .setClientBGColor(SColor((uint8_t)60, (uint8_t)60, (uint8_t)60, (uint8_t)255))
        .setEdgeMargin(6.0f)
        .setOnClose([](shared_ptr<Control>) {
            g_btn1->setCaption("Show WinFrame 1");
            TestUtil::log("WinFrame 1 closed");
        })
        .addToClient(ButtonBuilder(nullptr, SRect(20, 20, 160, 36))
            .setCaption("Button A")
            .setBackgroundStateColor(defaultBtnBG)
            .setOnClick([](shared_ptr<Button>) { TestUtil::log("WinFrame1: Button A clicked"); })
            .build())
        .addToClient(ButtonBuilder(nullptr, SRect(20, 70, 160, 36))
            .setCaption("Button B")
            .setBackgroundStateColor(defaultBtnBG)
            .setOnClick([](shared_ptr<Button>) { TestUtil::log("WinFrame1: Button B clicked"); })
            .build())
        .addToClient(EditBoxBuilder(nullptr, SRect(20, 120, 200, 36))
            .build())
        .build();

    g_winFrame2 = WinFrameBuilder(nullptr, SRect(350, 250, 400, 300))
        .setTitle("Unresizable WinFrame")
        .setTitleFontSize(24)
        .setTitleTextColor(SColor((uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)255))
        .setTitleBarBGColor(SColor((uint8_t)180, (uint8_t)70, (uint8_t)70, (uint8_t)255))
        .setWinFrameBGColor(SColor((uint8_t)50, (uint8_t)50, (uint8_t)50, (uint8_t)255))
        .setWinFrameBorderColor(SColor((uint8_t)100, (uint8_t)100, (uint8_t)100, (uint8_t)255))
        .setClientBGColor(SColor((uint8_t)60, (uint8_t)60, (uint8_t)60, (uint8_t)255))
        .setEdgeMargin(6.0f)
        .setResizable(false)
        .setOnClose([](shared_ptr<Control>) {
            g_btn2->setCaption("Show WinFrame 2");
            TestUtil::log("WinFrame 2 closed");
        })
        .addToClient(ButtonBuilder(nullptr, SRect(20, 20, 160, 36))
            .setCaption("Button C")
            .setBackgroundStateColor(defaultBtnBG)
            .setOnClick([](shared_ptr<Button>) { TestUtil::log("WinFrame2: Button C clicked"); })
            .build())
        .addToClient(EditBoxBuilder(nullptr, SRect(20, 70, 200, 36))
            .build())
        .build();

    TestUtil::log("Adding WinFrames to BENCH...");
    BENCH->addControl(g_winFrame1);
    BENCH->addControl(g_winFrame2);

    BENCH->addControl(g_btn1 = ButtonBuilder(BENCH, SRect(20, 20, 180, 40))
        .setCaption("Show WinFrame 1")
        .setBackgroundStateColor(defaultBtnBG)
        .setOnClick([](shared_ptr<Button>) {
            if (g_winFrame1->getVisible()) {
                g_winFrame1->hide();
                g_btn1->setCaption("Show WinFrame 1");
            } else {
                g_winFrame1->show();
                g_btn1->setCaption("Hide WinFrame 1");
            }
        })
        .build());

    BENCH->addControl(g_btn2 = ButtonBuilder(BENCH, SRect(220, 20, 190, 40))
        .setCaption("Show WinFrame 2")
        .setBackgroundStateColor(defaultBtnBG)
        .setOnClick([](shared_ptr<Button>) {
            if (g_winFrame2->getVisible()) {
                g_winFrame2->hide();
                g_btn2->setCaption("Show WinFrame 2");
            } else {
                g_winFrame2->show();
                g_btn2->setCaption("Hide WinFrame 2");
            }
        })
        .build());

    TestUtil::log("Initialization complete");
}

class WinFrameApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_winframe");
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
    }
};

int main(int argc, char* argv[]) {
    WinFrameApp app;
    return MAINWIN->run(&app);
}
