#include <iostream>
#include <memory>
#include <cstdio>
#include "CheckBox.h"
#include "Label.h"
#include "PlatformUtils.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "EditBox.h"
#include "TextArea.h"
#include "GraphTool.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<CheckBox> g_checkbox1;
shared_ptr<CheckBox> g_checkbox2;
shared_ptr<CheckBox> g_checkbox3;
shared_ptr<CheckBox> g_checkbox4;
shared_ptr<CheckBox> g_checkbox5;
shared_ptr<CheckBox> g_checkbox6;
shared_ptr<CheckBox> g_checkbox7;
shared_ptr<CheckBox> g_checkbox8;
shared_ptr<CheckBox> g_checkbox9;
shared_ptr<CheckBox> g_checkbox10;
shared_ptr<CheckBox> g_checkbox11;
shared_ptr<CheckBox> g_checkbox11x;
shared_ptr<CheckBox> g_checkbox12;
shared_ptr<CheckBox> g_checkbox13;
shared_ptr<CheckBox> g_checkbox14;
shared_ptr<CheckBox> g_checkbox15;
shared_ptr<CheckBox> g_checkbox16;

void testBenchInitialize(void) {
    TestUtil::log("testCheckBoxInitialize");

    uint64_t t0 = Platform::GetTicks();

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    g_checkbox1 = make_shared<CheckBox>(nullptr, SRect(50, 50, 100, 30));
    g_checkbox1->getCaption()->setCaption("1. Accept Terms");
    g_checkbox1->setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
        cout << "Checkbox1 state changed to: " << (int)state << endl;
    });
    g_checkbox1->create();
    BENCH->addControl(g_checkbox1);
    printf("[TIME] cb1: %llu ms\n", (Platform::GetTicks() - t0));

    g_checkbox2 = make_shared<CheckBox>(nullptr, SRect(50, 100, 200, 30));
    g_checkbox2->getCaption()->setCaption("2. Enable Feature");
    g_checkbox2->setCheckState(CheckState::Checked);
    g_checkbox2->setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
        cout << "Checkbox2 state changed to: " << (int)state << endl;
    });
    g_checkbox2->create();
    BENCH->addControl(g_checkbox2);
    printf("[TIME] cb2: %llu ms\n", (Platform::GetTicks() - t0));

    g_checkbox3 = CheckBoxBuilder(nullptr, SRect(50, 150, 200, 30))
        .setStyle(CheckBoxStyle::Cross)
        .setCaptionText("3. Cross Style")
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox3 (Cross) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox3);
    printf("[TIME] cb3: %llu ms\n", (Platform::GetTicks() - t0));

    g_checkbox4 = CheckBoxBuilder(nullptr, SRect(50, 200, 200, 30))
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionText("4. Circle Style")
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox4 (Circle) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox4);

    g_checkbox5 = CheckBoxBuilder(nullptr, SRect(50, 250, 200, 30))
        .setLayout(CheckBoxLayout::TextLeft)
        .setCaptionText("5. Text on Left")
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox5 (TextLeft) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox5);

    g_checkbox6 = CheckBoxBuilder(nullptr, SRect(50, 300, 200, 30))
        .setCaptionText("6. Tri-state Checkbox")
        .setCheckState(CheckState::Indeterminate)
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox6 (Tri-state) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox6);

    g_checkbox7 = CheckBoxBuilder(nullptr, SRect(50, 350, 200, 30))
        .setCaptionText("7. Disabled Checkbox")
        .setEnable(false)
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox7);

    g_checkbox8 = CheckBoxBuilder(nullptr, SRect(300, 50, 250, 30))
        .setCaptionText("8. Custom Check Color")
        .setCheckState(CheckState::Checked)
        .setCheckColor({255, 0, 255, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox8);

    g_checkbox9 = CheckBoxBuilder(nullptr, SRect(300, 100, 250, 30))
        .setStyle(CheckBoxStyle::Cross)
        .setCaptionText("9. Custom Cross Color")
        .setCheckState(CheckState::Checked)
        .setCrossColor({0, 255, 255, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox9);

    g_checkbox10 = CheckBoxBuilder(nullptr, SRect(300, 150, 250, 30))
        .setCaptionText("10. Custom Indeterminate Color")
        .setCheckState(CheckState::Indeterminate)
        .setIndeterminateColor({255, 165, 0, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox10);
    printf("[TIME] cb10: %llu ms\n", (Platform::GetTicks() - t0));

    g_checkbox11 = CheckBoxBuilder(nullptr, SRect(300, 200, 300, 60), 2.0f, 2.0f)
        .setCaptionText(u8"11. 2x缩放复选框")
        .setCheckState(CheckState::Checked)
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionSize(24)
        .setBorderStateColor(redBorder)
        .build();
    StateColor sc = g_checkbox11->getCaption()->getTextStateColor();
    sc.setNormal({0, 0, 255, 255});
    g_checkbox11->getCaption()->setBorderStateColor(sc);

    BENCH->addControl(g_checkbox11);

    g_checkbox11x = CheckBoxBuilder(nullptr, SRect(300, 320, 300, 60), 1.0f, 1.0f)
        .setCaptionText(u8"11x. 1x缩放复选框")
        .setCheckState(CheckState::Checked)
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionSize(24)
        .setBorderStateColor(redBorder)
        .build();
    StateColor scx = g_checkbox11x->getCaption()->getTextStateColor();
    scx.setNormal({0, 0, 255, 255});
    g_checkbox11x->getCaption()->setBorderStateColor(scx);

    BENCH->addControl(g_checkbox11x);

    g_checkbox12 = CheckBoxBuilder(nullptr, SRect(1000, 50, 250, 30))
        .setCaptionText("12. Custom Box Border Color")
        .setCheckState(CheckState::Checked)
        .setBoxBorderColor({0, 128, 255, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox12);

    g_checkbox13 = CheckBoxBuilder(nullptr, SRect(1000, 100, 200, 30))
        .setCaptionText("13. FontSize 12")
        .setCaptionSize(12)
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox13);

    g_checkbox14 = CheckBoxBuilder(nullptr, SRect(1000, 140, 200, 30))
        .setCaptionText("14. FontSize 20")
        .setCaptionSize(20)
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox14);

    g_checkbox15 = CheckBoxBuilder(nullptr, SRect(1000, 190, 250, 80))
        .setCaptionText("15. Line 1\nLine 2\nLine 3")
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .setVerticalAlign(CheckBoxVerticalAlign::Center)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox15);

    g_checkbox16 = CheckBoxBuilder(nullptr, SRect(1000, 280, 250, 30))
        .setCaptionText("16. Two-State Only (No Tri-state)")
        .setCheckState(CheckState::Checked)
        .setTriStateEnabled(false)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox16);
    printf("[TIME] cb16: %llu ms\n", (Platform::GetTicks() - t0));
    StateColor stateColor;
    stateColor.setNormal({0, 0, 255, 255});
    g_checkbox16->getCaption()->setTextStateColor(stateColor);

    TestUtil::log("CheckBox test controls created");
    printf("[TIME] done: %llu ms\n", (Platform::GetTicks() - t0));
}

class CheckBoxApp : public AppCallbacks {
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
    CheckBoxApp app;
    return MAINWIN->run(&app);
}
