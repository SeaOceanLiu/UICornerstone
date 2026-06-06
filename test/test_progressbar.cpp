#include <iostream>
#include <memory>
#include "ProgressBar.h"
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "EditBox.h"
#include "TextArea.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<ProgressBar> g_progressbar1;
shared_ptr<ProgressBar> g_progressbar2;
shared_ptr<ProgressBar> g_progressbar3;
shared_ptr<ProgressBar> g_progressbar4;
shared_ptr<ProgressBar> g_progressbar5;
shared_ptr<ProgressBar> g_progressbar6;
shared_ptr<ProgressBar> g_progressbar7;
shared_ptr<ProgressBar> g_progressbar8;
shared_ptr<ProgressBar> g_progressbar9;

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
shared_ptr<ProgressBar> g_progressbar10;
shared_ptr<Label> g_label11;
shared_ptr<ProgressBar> g_progressbar11;
shared_ptr<Label> g_label12;
shared_ptr<ProgressBar> g_progressbar12;

uint64_t g_startTime = 0;
bool g_animationStarted = false;

void testBenchInitialize(void) {
    TestUtil::log("testProgressBarInitialize");

    g_label1 = make_shared<Label>(nullptr, SRect(50, 20, 300, 25));
    g_label1->setCaption("Test 1: Basic 50% progress with percent display");
    g_label1->setFontSize(14);
    g_label1->create();
    BENCH->addControl(g_label1);

    g_progressbar1 = make_shared<ProgressBar>(nullptr, SRect(50, 50, 300, 30));
    g_progressbar1->setValue(50.0f);
    g_progressbar1->setOnValueChanged([](shared_ptr<ProgressBar>, float, float value) {
        cout << "ProgressBar1 value changed to: " << value << endl;
    });
    g_progressbar1->create();
    BENCH->addControl(g_progressbar1);

    g_label2 = make_shared<Label>(nullptr, SRect(50, 90, 300, 25));
    g_label2->setCaption("Test 2: No text mode (hidden text)");
    g_label2->setFontSize(14);
    g_label2->create();
    BENCH->addControl(g_label2);

    g_progressbar2 = make_shared<ProgressBar>(nullptr, SRect(50, 120, 300, 30));
    g_progressbar2->setValue(25.0f);
    g_progressbar2->setTextMode(ProgressBarTextMode::None);
    g_progressbar2->create();
    BENCH->addControl(g_progressbar2);

    g_label3 = make_shared<Label>(nullptr, SRect(50, 160, 300, 25));
    g_label3->setCaption("Test 3: Custom colors (green progress, dark background)");
    g_label3->setFontSize(14);
    g_label3->create();
    BENCH->addControl(g_label3);

    g_progressbar3 = ProgressBarBuilder(nullptr, SRect(50, 190, 300, 30))
        .setValue(75.0f)
        .setTextMode(ProgressBarTextMode::Percent)
        .setProgressColor({0, 128, 0, 255})
        .setBackgroundColor({50, 50, 50, 255})
        .setOnValueChanged([](shared_ptr<ProgressBar>, float, float value) {
            cout << "ProgressBar3 value changed to: " << value << endl;
        })
        .build();
    BENCH->addControl(g_progressbar3);

    g_label4 = make_shared<Label>(nullptr, SRect(50, 230, 300, 25));
    g_label4->setCaption("Test 4: Custom text 'Loading...'");
    g_label4->setFontSize(14);
    g_label4->create();
    BENCH->addControl(g_label4);

    g_progressbar4 = ProgressBarBuilder(nullptr, SRect(50, 260, 300, 30))
        .setValue(50.0f)
        .setTextMode(ProgressBarTextMode::Custom)
        .setCustomText("Loading...")
        .setProgressColor({0, 100, 200, 255})
        .setOnValueChanged([](shared_ptr<ProgressBar>, float, float value) {
            cout << "ProgressBar4 value changed to: " << value << endl;
        })
        .build();
    BENCH->addControl(g_progressbar4);

    g_label5 = make_shared<Label>(nullptr, SRect(50, 300, 300, 25));
    g_label5->setCaption("Test 5: Custom text 'Complete!' with green");
    g_label5->setFontSize(14);
    g_label5->create();
    BENCH->addControl(g_label5);

    g_progressbar5 = ProgressBarBuilder(nullptr, SRect(50, 330, 300, 30))
        .setValue(100.0f)
        .setTextMode(ProgressBarTextMode::Custom)
        .setCustomText("Complete!")
        .setProgressColor({0, 200, 100, 255})
        .build();
    BENCH->addControl(g_progressbar5);

    g_label6 = make_shared<Label>(nullptr, SRect(50, 370, 350, 25));
    g_label6->setCaption("Test 6: 2x scale (300x30 -> 600x60) with orange color");
    g_label6->setFontSize(14);
    g_label6->create();
    BENCH->addControl(g_label6);

    g_progressbar6 = ProgressBarBuilder(nullptr, SRect(50, 400, 300, 30), 2.0f, 2.0f)
        .setValue(60.0f)
        .setTextMode(ProgressBarTextMode::Percent)
        .setProgressColor({200, 100, 0, 255})
        .build();
    BENCH->addControl(g_progressbar6);

    g_label7 = make_shared<Label>(nullptr, SRect(50, 470, 350, 25));
    g_label7->setCaption("Test 7: 2x scale with Chinese custom text");
    g_label7->setFontSize(14);
    g_label7->create();
    BENCH->addControl(g_label7);

    g_progressbar7 = ProgressBarBuilder(nullptr, SRect(50, 500, 300, 30), 2.0f, 2.0f)
        .setValue(80.0f)
        .setTextMode(ProgressBarTextMode::Custom)
        .setCustomText(u8"加载中...")
        .setFontSize(24)
        .build();
    BENCH->addControl(g_progressbar7);

    g_label8 = make_shared<Label>(nullptr, SRect(400, 20, 300, 25));
    g_label8->setCaption("Test 8: Custom range 0-1000 with purple");
    g_label8->setFontSize(14);
    g_label8->create();
    BENCH->addControl(g_label8);

    g_progressbar8 = ProgressBarBuilder(nullptr, SRect(400, 50, 300, 30))
        .setValue(0.0f)
        .setRange(0.0f, 1000.0f)
        .setTextMode(ProgressBarTextMode::Percent)
        .setProgressColor({128, 0, 128, 255})
        .build();
    BENCH->addControl(g_progressbar8);

    g_label9 = make_shared<Label>(nullptr, SRect(400, 90, 300, 25));
    g_label9->setCaption("Test 9: Animation 0-100% in 10 seconds");
    g_label9->setFontSize(14);
    g_label9->create();
    BENCH->addControl(g_label9);

    g_progressbar9 = ProgressBarBuilder(nullptr, SRect(400, 120, 300, 30))
        .setValue(0.0f)
        .setTextMode(ProgressBarTextMode::Percent)
        .setProgressColor({255, 165, 0, 255})
        .setAnimationSpeed(0.05f)
        .build();
    BENCH->addControl(g_progressbar9);

    g_label10 = make_shared<Label>(nullptr, SRect(800, 20, 300, 25));
    g_label10->setCaption("Test 10: Left alignment text");
    g_label10->setFontSize(14);
    g_label10->create();
    BENCH->addControl(g_label10);

    g_progressbar10 = ProgressBarBuilder(nullptr, SRect(800, 50, 300, 30))
        .setValue(50.0f)
        .setTextMode(ProgressBarTextMode::Percent)
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setProgressColor({0, 150, 255, 255})
        .build();
    BENCH->addControl(g_progressbar10);

    g_label11 = make_shared<Label>(nullptr, SRect(800, 90, 300, 25));
    g_label11->setCaption("Test 11: Vertical progress bar");
    g_label11->setFontSize(14);
    g_label11->create();
    BENCH->addControl(g_label11);

    g_progressbar11 = ProgressBarBuilder(nullptr, SRect(800, 120, 40, 150))
        .setValue(70.0f)
        .setStyle(ProgressBarStyle::Vertical)
        .setTextMode(ProgressBarTextMode::Percent)
        .setProgressColor({255, 100, 100, 255})
        .build();
    BENCH->addControl(g_progressbar11);

    g_label12 = make_shared<Label>(nullptr, SRect(800, 280, 300, 25));
    g_label12->setCaption("Test 12: Vertical with custom text");
    g_label12->setFontSize(14);
    g_label12->create();
    BENCH->addControl(g_label12);

    g_progressbar12 = ProgressBarBuilder(nullptr, SRect(800, 310, 40, 150))
        .setValue(50.0f)
        .setStyle(ProgressBarStyle::Vertical)
        .setTextMode(ProgressBarTextMode::Custom)
        .setCustomText(u8"加载中")
        .setProgressColor({100, 200, 100, 255})
        .setFontSize(16)
        .build();
    BENCH->addControl(g_progressbar12);

    g_startTime = Platform::GetTicks();
    g_animationStarted = true;

    TestUtil::log("ProgressBar test controls created");
}

class ProgressBarApp : public AppCallbacks {
public:
    bool onInit() override {
        BENCH->setOnInitial(testBenchInitialize);
        return true;
    }

    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();

        if (g_animationStarted && g_progressbar9 != nullptr) {
            uint64_t currentTime = Platform::GetTicks();
            uint64_t elapsed = currentTime - g_startTime;

            if (elapsed <= 10000) {
                float progress = (elapsed / 10000.0f) * 100.0f;
                g_progressbar9->setValue(progress);

                if (elapsed % 1000 == 0) {
                    cout << "Animation: " << (int)progress << "% (elapsed: " << (elapsed/1000) << "s)" << endl;
                }
            } else {
                g_progressbar9->setValue(100.0f);
                cout << "Animation complete: 100%" << endl;
            }
        }
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
    ProgressBarApp app;
    return MAINWIN->run(&app);
}
