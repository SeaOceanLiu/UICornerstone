#include <iostream>
#include <memory>
#include <string>
#include "ColorPicker.h"
#include "Label.h"
#include "Button.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<ColorPicker> g_cp1;
shared_ptr<ColorPicker> g_cp2;
shared_ptr<ColorPicker> g_cp3;
shared_ptr<ColorPicker> g_cp4;
shared_ptr<ColorPicker> g_cp5;
shared_ptr<Label> g_statusLabel;
shared_ptr<Label> g_lastColorLabel;
SColor g_lastColor = SColor::White();

void onColorPicked(shared_ptr<ColorPicker>, const SColor& color) {
    g_lastColor = color;
    char buf[64];
    snprintf(buf, sizeof(buf), "上次选中: %s",
        color.toHex(false).c_str());
    if (g_lastColorLabel)
        g_lastColorLabel->setCaption(buf);
    cout << "Color changed to: " << color.toHex(false) << endl;
}

void testColorPickerInitialize() {
    TestUtil::log("testColorPickerInitialize");

    const float X1 = 50;

    // ColorPicker 1: Default red
    g_cp1 = ColorPickerBuilder(nullptr, SRect(X1, 30, 96, 24))
        .setColor("#FF0000")
        .setOnColorChanged(onColorPicked)
        .build();
    BENCH->addControl(g_cp1);

    // ColorPicker 2: Green with custom presets
    vector<string> smallPresets = {
        "#00FF00", "#00CC00", "#009900", "#006600", "#003300"
    };
    g_cp2 = ColorPickerBuilder(nullptr, SRect(X1, 70, 96, 24))
        .setColor("#00AA00")
        .setPresetColors(smallPresets)
        .setOnColorChanged(onColorPicked)
        .build();
    BENCH->addControl(g_cp2);

    // ColorPicker 3: Blue
    g_cp3 = ColorPickerBuilder(nullptr, SRect(X1, 110, 96, 24))
        .setColor("#3366CC")
        .setOnColorChanged(onColorPicked)
        .build();
    BENCH->addControl(g_cp3);

    // ColorPicker 4: Custom #ABABAB
    g_cp4 = ColorPickerBuilder(nullptr, SRect(X1, 150, 96, 24))
        .setColor("#000000")
        .setOnColorChanged(onColorPicked)
        .build();
    BENCH->addControl(g_cp4);

    // ColorPicker 5: 2x scale (same unscaled size as CP1-4, rendered 2x)
    g_cp5 = ColorPickerBuilder(nullptr, SRect(X1, 200, 96, 24),
        2.0f, 2.0f)
        .setColor("#FF8800")
        .setOnColorChanged(onColorPicked)
        .build();
    BENCH->addControl(g_cp5);

    // Status label
    g_statusLabel = LabelBuilder(nullptr, SRect(50, 300, 700, 20))
        .setCaption(u8"点击色块打开调色板，支持 Hex 输入 / RGB 滑块 / 预设色 / Alpha")
        .setFontSize(12)
        .build();
    BENCH->addControl(g_statusLabel);

    // Last picked color label
    g_lastColorLabel = LabelBuilder(nullptr, SRect(50, 330, 300, 20))
        .setCaption(u8"上次选中: #FFFFFF")
        .setFontSize(12)
        .build();
    BENCH->addControl(g_lastColorLabel);

    TestUtil::log("ColorPicker test controls created");
}

class ColorPickerApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_colorpicker");
        BENCH->setOnInitial(testColorPickerInitialize);
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
        TestUtil::log("ColorPicker test quit");
    }
};

int main(int argc, char* argv[]) {
    ColorPickerApp app;
    return MAINWIN->run(&app);
}

