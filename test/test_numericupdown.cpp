#include <iostream>
#include <memory>
#include "NumericUpDown.h"
#include "Label.h"
#include "Button.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<NumericUpDown> g_nud1;
shared_ptr<NumericUpDown> g_nud2;
shared_ptr<NumericUpDown> g_nud2b;
shared_ptr<NumericUpDown> g_nud2c;
shared_ptr<NumericUpDown> g_nud3;
shared_ptr<NumericUpDown> g_nud4;
shared_ptr<NumericUpDown> g_nud5;
shared_ptr<NumericUpDown> g_nud6;
shared_ptr<NumericUpDown> g_nud7;
shared_ptr<NumericUpDown> g_nud8;
shared_ptr<NumericUpDown> g_nud9;
shared_ptr<NumericUpDown> g_nud10;
shared_ptr<NumericUpDown> g_nud11;
shared_ptr<NumericUpDown> g_nud12;
shared_ptr<Label> g_statusLabel;
int g_valueChangedCount = 0;

void onValueChangedCounter(shared_ptr<NumericUpDown> nud, double val) {
    g_valueChangedCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Value #%d = %.2f", g_valueChangedCount, val);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

static shared_ptr<Label> makeLabel(float x, float y, const string& text) {
    auto lbl = LabelBuilder(nullptr, SRect(x, y, 180, 16))
        .setCaption(text)
        .setFontSize(11)
        .build();
    lbl->setTextNormalStateColor(SColor(180, 180, 180, 255));
    BENCH->addControl(lbl);
    return lbl;
}

void testNumericUpDownInitialize() {
    TestUtil::log("testNumericUpDownInitialize");

    const float X1 = 50;
    const float XL = 250; // label X for right-column descriptions
    float Y = 20;
    const float STEP = 55;

    // ── Test 1 ──
    makeLabel(X1, Y, "step=1, range 0~100");
    Y += 16;
    g_nud1 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(50).setRange(0, 100).setStep(1)
        .setOnValueChanged(onValueChangedCounter)
        .build();
    BENCH->addControl(g_nud1);
    Y += STEP;

    // ── Test 2 ──
    makeLabel(X1, Y, "step=0.1, decimals=2");
    Y += 16;
    g_nud2 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(1.0).setRange(0.1, 10.0).setStep(0.1).setDecimals(2)
        .build();
    BENCH->addControl(g_nud2);
    Y += STEP;

    // ── Test 2b ──
    makeLabel(X1, Y, "step=0.2 (custom step)");
    Y += 16;
    g_nud2b = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(0.6).setRange(0.0, 1.0).setStep(0.2).setDecimals(2)
        .setOnValueChanged([](shared_ptr<NumericUpDown>, double v) {
            printf("[nud2b] step=0.2, value=%.2f\n", v);
        })
        .build();
    BENCH->addControl(g_nud2b);
    Y += STEP;

    // ── Test 2c ──
    makeLabel(X1, Y, "step=50, range 0~10000");
    Y += 16;
    g_nud2c = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(100).setRange(0, 10000).setStep(50)
        .build();
    BENCH->addControl(g_nud2c);
    Y += STEP;

    // ── Test 3 ──
    makeLabel(X1, Y, "step=10");
    Y += 16;
    g_nud3 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(50).setRange(0, 1000).setStep(10)
        .build();
    BENCH->addControl(g_nud3);
    Y += STEP;

    // ── Test 4 ──
    makeLabel(X1, Y, "negative range -100~100, step=5");
    Y += 16;
    g_nud4 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(-50).setRange(-100, 100).setStep(5)
        .build();
    BENCH->addControl(g_nud4);
    Y += STEP;

    // ── Test 5 ──
    makeLabel(X1, Y, "decimals=4, step=0.0001");
    Y += 16;
    g_nud5 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(3.14159).setRange(0.0, 10.0).setStep(0.0001).setDecimals(4)
        .build();
    BENCH->addControl(g_nud5);
    Y += STEP;

    // ── Test 6 ──
    makeLabel(X1, Y, "placeholder test");
    Y += 16;
    g_nud6 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setPlaceholder("Enter value...").setRange(0, 100)
        .build();
    BENCH->addControl(g_nud6);
    Y += STEP;

    // ── Test 7 ──
    makeLabel(X1, Y, "readOnly mode");
    Y += 16;
    g_nud7 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(42).setReadOnly(true)
        .build();
    BENCH->addControl(g_nud7);
    Y += STEP;

    // ── Test 8 ──
    makeLabel(X1, Y, "infinity range, step=0.001");
    Y += 16;
    g_nud8 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(0)
        .setRange(-numeric_limits<double>::infinity(), numeric_limits<double>::infinity())
        .setStep(0.001).setDecimals(3)
        .build();
    BENCH->addControl(g_nud8);
    Y += STEP;

    // ── Test 9 ──
    makeLabel(X1, Y, "wide button (24px), w=200");
    Y += 16;
    g_nud9 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 200, 32))
        .setValue(50).setButtonWidth(24)
        .build();
    BENCH->addControl(g_nud9);
    Y += STEP;

    // ── Test 10 ──
    makeLabel(X1, Y, "2x scaled");
    Y += 16;
    g_nud10 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32), 2.0f, 2.0f)
        .setValue(100).setRange(0, 200).setStep(5)
        .build();
    BENCH->addControl(g_nud10);
    Y += 65; // more space for 2x

    // ── Test 11 ──
    makeLabel(X1, Y, "custom arrow color (yellow)");
    Y += 16;
    g_nud11 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(50)
        .setArrowColor({255, 255, 0, 255}, {255, 255, 100, 255}, {200, 200, 0, 255})
        .build();
    BENCH->addControl(g_nud11);
    Y += STEP;

    // ── Test 12 ──
    makeLabel(X1, Y, "pageStep=25, Home/End test");
    Y += 16;
    g_nud12 = NumericUpDownBuilder(nullptr, SRect(X1, Y, 150, 32))
        .setValue(50).setRange(0, 1000).setStep(1).setPageStep(25)
        .build();
    BENCH->addControl(g_nud12);
    Y += STEP;

    // Status label
    g_statusLabel = LabelBuilder(nullptr, SRect(X1, Y, 500, 20))
        .setCaption("Click arrows or type to test")
        .setFontSize(11)
        .build();
    BENCH->addControl(g_statusLabel);

    TestUtil::log("NumericUpDown test controls created");
}

class NumericUpDownApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_numericupdown");
        BENCH->setOnInitial(testNumericUpDownInitialize);
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
        TestUtil::log("NumericUpDown test quit");
    }
};

int main(int argc, char* argv[]) {
    NumericUpDownApp app;
    return MAINWIN->run(&app);
}

