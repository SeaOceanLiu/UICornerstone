#include <iostream>
#include <memory>
#include "Slider.h"
#include "Label.h"
#include "Button.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<Slider> g_slider1;
shared_ptr<Slider> g_slider2;
shared_ptr<Slider> g_slider3;
shared_ptr<Slider> g_slider4;
shared_ptr<Slider> g_slider5;
shared_ptr<Slider> g_slider6;
shared_ptr<Slider> g_slider7;
shared_ptr<Slider> g_slider8;
shared_ptr<Slider> g_slider9;
shared_ptr<Slider> g_hTick1x;
shared_ptr<Slider> g_hTick2x;
shared_ptr<Slider> g_vTick1x;
shared_ptr<Slider> g_vTick2x;
shared_ptr<Slider> g_hReverse;
shared_ptr<Slider> g_vReverse;
shared_ptr<Label> g_statusLabel;

void testSliderInitialize(void) {
    TestUtil::log("testSliderInitialize");

    // ===== Left: Horizontal sliders (X=50) =====
    const float HX = 50;

    // Slider 1: Default range 0-100, value label with callback
    g_slider1 = SliderBuilder(nullptr, SRect(HX, 20, 250, 20))
        .setRange(0, 100)
        .setValue(50)
        .setShowValueLabel(true)
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "Slider1 value changed to: " << v << endl;
        })
        .build();
    BENCH->addControl(g_slider1);

    // Slider 2: Step=10, format with step info
    g_slider2 = SliderBuilder(nullptr, SRect(HX, 65, 250, 20))
        .setRange(0, 50)
        .setStep(10)
        .setValue(20)
        .setShowValueLabel(true)
        .setLabelFormat("%.0f (step=10)")
        .build();
    BENCH->addControl(g_slider2);

    // Slider 4: Custom green fill/border colors
    g_slider4 = SliderBuilder(nullptr, SRect(HX, 110, 250, 20))
        .setRange(0, 100)
        .setValue(30)
        .setTrackFillColor(SColor(76, 175, 80, 255))
        .setThumbBorderColor(SColor(76, 175, 80, 255))
        .setShowValueLabel(true)
        .build();
    BENCH->addControl(g_slider4);

    // Slider 5: Negative range, no value label
    g_slider5 = SliderBuilder(nullptr, SRect(HX, 155, 250, 20))
        .setRange(-50, 50)
        .setValue(0)
        .setStep(5)
        .build();
    BENCH->addControl(g_slider5);

    // Slider 6: Narrow range step=1, snap-on-release test
    g_slider6 = SliderBuilder(nullptr, SRect(HX, 200, 250, 20))
        .setRange(0, 3)
        .setStep(1)
        .setValue(0)
        .setShowValueLabel(true)
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "Slider6 snap value: " << v << endl;
        })
        .build();
    BENCH->addControl(g_slider6);

    // Slider 7: Thick track + large thumb
    g_slider7 = SliderBuilder(nullptr, SRect(HX, 250, 250, 30))
        .setRange(0, 100)
        .setValue(50)
        .setTrackThickness(12)
        .setThumbSize(28)
        .setShowValueLabel(true)
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "Slider7 (thick) value: " << v << endl;
        })
        .build();
    BENCH->addControl(g_slider7);

    // Slider 8: Small range 0-1, step=0.1
    g_slider8 = SliderBuilder(nullptr, SRect(HX, 310, 250, 20))
        .setRange(0.0f, 1.0f)
        .setStep(0.1f)
        .setValue(0.5f)
        .setShowValueLabel(true)
        .setLabelFormat("%.1f")
        .build();
    BENCH->addControl(g_slider8);

    // ===== Tick scale comparison (same rect size, different scales) =====
    // Slider 9: 1x ticks (tick labels also drawn), rect=250x20
    g_slider9 = SliderBuilder(nullptr, SRect(HX, 370, 250, 20))
        .setRange(0, 100)
        .setStep(1)
        .setValue(30)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(8)
        .setTickColor(SColor(180, 180, 180, 255))
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "Slider9 (ticks) value: " << v << endl;
        })
        .build();
    BENCH->addControl(g_slider9);

    // H_Tick1x: 1x ticks + tick labels, rect=250x20
    // label bottom ≈ 430+10+3+10+2+14 = 469
    g_hTick1x = SliderBuilder(nullptr, SRect(HX, 430, 250, 20))
        .setRange(0, 100)
        .setStep(1)
        .setValue(50)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(10)
        .setTickColor(SColor(180, 180, 180, 255))
        .build();
    BENCH->addControl(g_hTick1x);

    // H_Tick2x: 2x ticks + labels, rect SAME as 1x (250x20), visual=(50,500,500,40)
    // label bottom ≈ 500+20+6+20+2+28 = 576
    g_hTick2x = SliderBuilder(nullptr, SRect(HX, 500, 250, 20), 2.0f, 2.0f)
        .setRange(0, 100)
        .setStep(1)
        .setValue(50)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(10)
        .setTickColor(SColor(180, 180, 180, 255))
        .build();
    BENCH->addControl(g_hTick2x);

    // ===== Right: Vertical sliders =====
    // Slider 3: Vertical with value label only (no ticks)
    g_slider3 = SliderBuilder(nullptr, SRect(620, 20, 20, 300))
        .setRange(0, 100)
        .setValue(75)
        .setStyle(SliderStyle::Vertical)
        .setShowValueLabel(true)
        .build();
    BENCH->addControl(g_slider3);

    // V_Tick1x: 1x vertical ticks + labels, visual=(710,20,20,300)
    g_vTick1x = SliderBuilder(nullptr, SRect(710, 20, 20, 300))
        .setRange(0, 100)
        .setStep(1)
        .setValue(30)
        .setStyle(SliderStyle::Vertical)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(10)
        .setTickColor(SColor(180, 180, 180, 255))
        .build();
    BENCH->addControl(g_vTick1x);

    // V_Tick2x: 2x vertical ticks + labels, rect SAME as 1x (20x300)
    // visual=(820,20,40,600) → content=600-40=560 = 2x ✓
    g_vTick2x = SliderBuilder(nullptr, SRect(820, 20, 20, 300), 2.0f, 2.0f)
        .setRange(0, 100)
        .setStep(1)
        .setValue(60)
        .setStyle(SliderStyle::Vertical)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(10)
        .setTickColor(SColor(180, 180, 180, 255))
        .build();
    BENCH->addControl(g_vTick2x);

    // ===== Reverse sliders =====
    // H_Reverse: horizontal reversed (min=100 on left, max=0 on right)
    g_hReverse = SliderBuilder(nullptr, SRect(HX, 610, 250, 20))
        .setRange(0, 100)
        .setStep(1)
        .setValue(50)
        .setReverse(true)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(8)
        .setTickColor(SColor(180, 180, 180, 255))
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "hReverse value: " << v << endl;
        })
        .build();
    BENCH->addControl(g_hReverse);

    // V_Reverse: vertical reversed (min=100 on top, max=0 on bottom)
    g_vReverse = SliderBuilder(nullptr, SRect(940, 20, 20, 260))
        .setRange(0, 100)
        .setStep(1)
        .setValue(50)
        .setStyle(SliderStyle::Vertical)
        .setReverse(true)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(8)
        .setTickColor(SColor(180, 180, 180, 255))
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "vReverse value: " << v << endl;
        })
        .build();
    BENCH->addControl(g_vReverse);

    // Status label
    g_statusLabel = LabelBuilder(nullptr, SRect(50, 650, 700, 30))
        .setCaption("1x/2x horiz ticks | Vert: noTicks / 1x / 2x ticks | hReverse/vReverse")
        .setFontSize(14)
        .build();
    BENCH->addControl(g_statusLabel);

    TestUtil::log("Slider test controls created");
}

class SliderApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_slider");
        BENCH->setOnInitial(testSliderInitialize);
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
        TestUtil::log("Slider test quit");
    }
};

int main(int argc, char* argv[]) {
    SliderApp app;
    return MAINWIN->run(&app);
}
