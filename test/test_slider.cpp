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

    // ===== Layout Overview =====
    // Window: 1920x1080
    // Left column (HX=50, width=250): 11 horizontal sliders + 1 status label
    //   Y ranges from 40 to 800 — comfortably within 1080
    // Right column (4 vertical sliders, height=250, Y=10):
    //   X=580~910, no Y overlap with left column (left col is X=50~300, hTick2x at X=50~550)
    //
    // Spacing formula (value label extends 22px above track, ticks extend 22px/44px below):
    //   prev_track_bottom + [prev_tick_labels] + 8px gap + 22px val_label + 4px labelGap = next_Y
    //   No-value-label rows: simpler 20px gap minimum
    //
    // Calculated positions:
    //   Slider1:  Y=40,  h=20  → bottom=60
    //   Slider2:  Y=90,  h=20  → bottom=110   (+50 from prev)
    //   Slider4:  Y=140, h=20  → bottom=160   (+50)
    //   Slider5:  Y=180, h=20  → bottom=200   (+40, no val label)
    //   Slider6:  Y=230, h=20  → bottom=250   (+50, prev no val label)
    //   Slider7:  Y=285, h=30  → bottom=315   (+55, thick track)
    //   Slider8:  Y=345, h=20  → bottom=365   (+60)
    //   Slider9:  Y=415, h=20  → bottom=435, ticks to ~457   (+70, ticks)
    //   hTick1x:  Y=490, h=20  → bottom=510, ticks to ~532   (+75)
    //   hTick2x:  Y=580, h=20  → visual h=40, visual bottom=620, 2x ticks to ~664   (+90, 2x ticks)
    //   hReverse: Y=695, h=20  → bottom=715, ticks to ~737   (+115, gap from 2x ticks)
    //   Status:   Y=770, h=30  → bottom=800   (+75)

    const float HX = 50;

    // Slider 1: Default range 0-100, value label with callback
    g_slider1 = SliderBuilder(nullptr, SRect(HX, 40, 250, 20))
        .setRange(0, 100)
        .setValue(50)
        .setShowValueLabel(true)
        .setOnValueChanged([](shared_ptr<Slider> s, float v) {
            cout << "Slider1 value changed to: " << v << endl;
        })
        .build();
    BENCH->addControl(g_slider1);

    // Slider 2: Step=10, format with step info
    g_slider2 = SliderBuilder(nullptr, SRect(HX, 90, 250, 20))
        .setRange(0, 50)
        .setStep(10)
        .setValue(20)
        .setShowValueLabel(true)
        .setLabelFormat("%.0f (step=10)")
        .build();
    BENCH->addControl(g_slider2);

    // Slider 4: Custom green fill/border colors
    g_slider4 = SliderBuilder(nullptr, SRect(HX, 140, 250, 20))
        .setRange(0, 100)
        .setValue(30)
        .setTrackFillColor(SColor(76, 175, 80, 255))
        .setThumbBorderColor(SColor(76, 175, 80, 255))
        .setShowValueLabel(true)
        .build();
    BENCH->addControl(g_slider4);

    // Slider 5: Negative range, no value label
    g_slider5 = SliderBuilder(nullptr, SRect(HX, 180, 250, 20))
        .setRange(-50, 50)
        .setValue(0)
        .setStep(5)
        .build();
    BENCH->addControl(g_slider5);

    // Slider 6: Narrow range step=1, snap-on-release test
    g_slider6 = SliderBuilder(nullptr, SRect(HX, 230, 250, 20))
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
    g_slider7 = SliderBuilder(nullptr, SRect(HX, 285, 250, 30))
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
    g_slider8 = SliderBuilder(nullptr, SRect(HX, 345, 250, 20))
        .setRange(0.0f, 1.0f)
        .setStep(0.1f)
        .setValue(0.5f)
        .setShowValueLabel(true)
        .setLabelFormat("%.1f")
        .build();
    BENCH->addControl(g_slider8);

    // Slider 9: Ticks + value label (tick labels below track)
    g_slider9 = SliderBuilder(nullptr, SRect(HX, 415, 250, 20))
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

    // H_Tick1x: 1x ticks + labels
    g_hTick1x = SliderBuilder(nullptr, SRect(HX, 490, 250, 20))
        .setRange(0, 100)
        .setStep(1)
        .setValue(50)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(10)
        .setTickColor(SColor(180, 180, 180, 255))
        .build();
    BENCH->addControl(g_hTick1x);

    // H_Tick2x: 2x ticks + labels (visual width=500, visual height=40)
    g_hTick2x = SliderBuilder(nullptr, SRect(HX, 580, 250, 20), 2.0f, 2.0f)
        .setRange(0, 100)
        .setStep(1)
        .setValue(50)
        .setShowValueLabel(true)
        .setTickInterval(10)
        .setTickLength(10)
        .setTickColor(SColor(180, 180, 180, 255))
        .build();
    BENCH->addControl(g_hTick2x);

    // ===== Right: Vertical sliders (X=580~910, Y=10, h=250) =====
    // vTick2x at 2x scale: visual h=500, bottom=510
    //   left column at this Y is hTick2x area (X=50~300) — no horizontal overlap

    // Slider 3: Vertical with value label only (no ticks)
    g_slider3 = SliderBuilder(nullptr, SRect(580, 10, 20, 250))
        .setRange(0, 100)
        .setValue(75)
        .setStyle(SliderStyle::Vertical)
        .setShowValueLabel(true)
        .build();
    BENCH->addControl(g_slider3);

    // V_Tick1x: 1x vertical ticks + labels
    g_vTick1x = SliderBuilder(nullptr, SRect(690, 10, 20, 250))
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

    // V_Tick2x: 2x vertical ticks + labels, visual height=500
    g_vTick2x = SliderBuilder(nullptr, SRect(800, 10, 20, 250), 2.0f, 2.0f)
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

    // V_Reverse: vertical reversed (min=100 on top, max=0 on bottom)
    g_vReverse = SliderBuilder(nullptr, SRect(910, 10, 20, 250))
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

    // ===== Reverse horizontal =====
    // H_Reverse: horizontal reversed (min=100 on left, max=0 on right)
    g_hReverse = SliderBuilder(nullptr, SRect(HX, 695, 250, 20))
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

    // Status label
    g_statusLabel = LabelBuilder(nullptr, SRect(50, 770, 700, 30))
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
