#include <iostream>
#include <memory>
#include "ComboBox.h"
#include "Label.h"
#include "Button.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<ComboBox> g_combo1;
shared_ptr<ComboBox> g_combo2;
shared_ptr<ComboBox> g_combo3;
shared_ptr<ComboBox> g_combo4;
shared_ptr<ComboBox> g_combo5;
shared_ptr<ComboBox> g_combo6_2x;
shared_ptr<Label> g_statusLabel;

void testComboBoxInitialize(void) {
    TestUtil::log("testComboBoxInitialize");

    const float CX = 50;
    const float CW = 240;
    const float HH = 32;
    const float GAP = 16;

    // Combo 1: Basic read-only combobox with items
    vector<ComboBoxItem> items1 = {
        {"Option A",   "val_a"},
        {"Option B",   "val_b"},
        {"Option C",   "val_c"},
        {"Option D",   "val_d"},
        {"Disabled E", "val_e", true},
        {"Option F",   "val_f"},
    };
    g_combo1 = ComboBoxBuilder(nullptr, SRect(CX, 40, CW, HH))
        .setItems(items1)
        .setSelectedIndex(0)
        .setPlaceholder("Select an option...")
        .setOnSelectionChanged([](shared_ptr<ComboBox>, int idx, const string& value) {
            cout << "Combo1 selected: index=" << idx << " value=" << value << endl;
        })
        .build();
    BENCH->addControl(g_combo1);

    // Combo 2: Larger font, custom colors, taller
    vector<ComboBoxItem> items2 = {
        {"Red",    "red"},
        {"Green",  "green"},
        {"Blue",   "blue"},
        {"Yellow", "yellow", true},
        {"Purple", "purple"},
    };
    g_combo2 = ComboBoxBuilder(nullptr, SRect(CX, 40 + (HH+GAP)*1, CW, 36))
        .setItems(items2)
        .setSelectedIndex(0)
        .setFontSize(18)
        .setItemHeight(28)
        .setArrowColor(SColor(60, 60, 60, 255))
        .setArrowHoverColor(SColor(200, 60, 60, 255))
        .setItemSelectedColor(SColor(180, 220, 255, 255))
        .setItemHoverColor(SColor(220, 240, 255, 255))
        .setListBgColor(SColor(245, 248, 250, 255))
        .setListBorderColor(SColor(150, 180, 200, 255))
        .setOnSelectionChanged([](shared_ptr<ComboBox>, int idx, const string& value) {
            cout << "Combo2: color=" << value << endl;
        })
        .build();
    BENCH->addControl(g_combo2);

    // Combo 3: Many items, with scrollbar
    vector<ComboBoxItem> items3;
    for (int i = 1; i <= 30; ++i) {
        bool disabled = (i % 10 == 0);
        items3.push_back({"Item " + to_string(i), to_string(i), disabled});
    }
    g_combo3 = ComboBoxBuilder(nullptr, SRect(CX, 40 + (HH+GAP)*2, CW, HH))
        .setItems(items3)
        .setSelectedIndex(4)
        .setMaxVisibleItems(8)
        .setOnSelectionChanged([](shared_ptr<ComboBox>, int idx, const string& value) {
            cout << "Combo3: item=" << idx << endl;
        })
        .build();
    BENCH->addControl(g_combo3);

    // Combo 4: No initial selection, placeholder text
    vector<ComboBoxItem> items4 = {
        {"Small",  "s"},
        {"Medium", "m", true},
        {"Large",  "l"},
        {"XL",     "xl"},
    };
    g_combo4 = ComboBoxBuilder(nullptr, SRect(CX, 40 + (HH+GAP)*3, CW, HH))
        .setItems(items4)
        .setPlaceholder("Choose size...")
        .build();
    BENCH->addControl(g_combo4);

    // Combo 5: Wide with arrow color, tests truncation
    vector<ComboBoxItem> items5 = {
        {"Very long option text that should be truncated in the combobox dropdown"},
        {"Another extremely long label for testing text truncation feature"},
        {"Short label"},
    };
    g_combo5 = ComboBoxBuilder(nullptr, SRect(CX, 40 + (HH+GAP)*4, CW, HH))
        .setItems(items5)
        .setSelectedIndex(0)
        .build();
    BENCH->addControl(g_combo5);

    // Status label
    g_statusLabel = LabelBuilder(nullptr, SRect(CX, 40 + (HH+GAP)*6, 500, 24))
        .setCaption("ComboBox: click arrow to open, Enter/Esc to confirm/cancel, scroll wheel cycles")
        .setFontSize(13)
        .build();
    BENCH->addControl(g_statusLabel);

    // Combo 6: 2x scaled, no cycle wrapping
    vector<ComboBoxItem> items6 = {
        {"Alpha",   "a"},
        {"Beta",    "b"},
        {"Gamma",   "g"},
        {"Delta",   "d"},
        {"Epsilon", "e", true},
        {"Zeta",    "z"},
    };
    g_combo6_2x = ComboBoxBuilder(nullptr, SRect(320, 40, CW, HH), 2.0f, 2.0f)
        .setItems(items6)
        .setSelectedIndex(0)
        .setMaxVisibleItems(5)
        .setCycleEnabled(false)
        .setOnSelectionChanged([](shared_ptr<ComboBox>, int idx, const string& value) {
            cout << "Combo6(2x) selected: index=" << idx << " value=" << value << endl;
        })
        .build();
    BENCH->addControl(g_combo6_2x);

    TestUtil::log("ComboBox test controls created");
}

class ComboBoxApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_combobox");
        BENCH->setOnInitial(testComboBoxInitialize);
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
        TestUtil::log("ComboBox test quit");
    }
};

int main(int argc, char* argv[]) {
    ComboBoxApp app;
    return MAINWIN->run(&app);
}
