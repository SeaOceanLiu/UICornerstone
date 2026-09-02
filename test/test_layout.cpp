#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include "LayoutParser.h"
#include "Label.h"
#include "CheckBox.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TextArea.h"
#include "Button.h"
#include "EditBox.h"
#include "WinFrame.h"
#include "TabControl.h"
#include "ListView.h"
#include "Slider.h"
#include "PlatformUtils.h"
#include "TestUtils.h"
#include "TestInstance.h"

using namespace std;

static LayoutParser g_parser;
static shared_ptr<WinFrame> g_resultWinFrame;

static void testFontInheritance();

void onSubmitClicked(shared_ptr<Control> c) {
    TestUtil::log("Button clicked via auto-binding!");
    if (g_resultWinFrame) {
        g_resultWinFrame->show();
    }
}

void onNotesChanged(shared_ptr<Control> c) {
    TestUtil::log("TextArea content changed via auto-binding!");
}

void onAgreeChanged(shared_ptr<Control> c) {
    TestUtil::log("CheckBox state changed via auto-binding!");
}

void onColorChanged(shared_ptr<Control> c) {
    TestUtil::log("ColorPicker color changed!");
}

void onImageBtnClick(shared_ptr<Control> c) {
    TestUtil::log("Image button clicked!");
}

void onAniBtnClick(shared_ptr<Control> c) {
    TestUtil::log("Animated button clicked!");
}

void onCaptionLabelBtnClick(shared_ptr<Control> c) {
    TestUtil::log("CaptionLabel button clicked!");
}

void onMenuNew(shared_ptr<Control> c) {
    TestUtil::log("Menu: New file");
}

void onMenuOpen(shared_ptr<Control> c) {
    TestUtil::log("Menu: Open file");
}

void onMenuRecent(shared_ptr<Control> c) {
    TestUtil::log("Menu: Recent file");
}

void onMenuExit(shared_ptr<Control> c) {
    TestUtil::log("Menu: Exit");
    MAINWIN->quit();
}

void onMenuUndo(shared_ptr<Control> c) {
    TestUtil::log("Menu: Undo");
}

void onMenuRedo(shared_ptr<Control> c) {
    TestUtil::log("Menu: Redo");
}

void onMenuAbout(shared_ptr<Control> c) {
    TestUtil::log("Menu: About");
}

void testBenchInitialize(shared_ptr<Bench>) {
    TestUtil::log("testLayoutInitialize");

    g_parser.registerHandler("onSubmitClick", onSubmitClicked);
    g_parser.registerHandler("onNotesChanged", onNotesChanged);
    g_parser.registerHandler("onAgreeChanged", onAgreeChanged);
    g_parser.registerHandler("onImageBtnClick", onImageBtnClick);
    g_parser.registerHandler("onAniBtnClick", onAniBtnClick);
    g_parser.registerHandler("onCaptionLabelBtnClick", onCaptionLabelBtnClick);
    g_parser.registerHandler("onMenuNew", onMenuNew);
    g_parser.registerHandler("onMenuOpen", onMenuOpen);
    g_parser.registerHandler("onMenuRecent", onMenuRecent);
    g_parser.registerHandler("onMenuExit", onMenuExit);
    g_parser.registerHandler("onMenuUndo", onMenuUndo);
    g_parser.registerHandler("onMenuRedo", onMenuRedo);
    g_parser.registerHandler("onMenuAbout", onMenuAbout);
    string layoutPath = Platform::GetBasePath() + "layouts/test_layout.json";
    auto root = g_parser.parseLayoutFile(layoutPath);
    if (!root) {
        TestUtil::log("Failed to parse layout file!");
        return;
    }

    BENCH->addControl(root);

    auto resultCtrl = g_parser.findControlById("resultWinFrame");
    if (resultCtrl) {
        g_resultWinFrame = dynamic_pointer_cast<WinFrame>(resultCtrl);
        if (g_resultWinFrame) {
            TestUtil::log("WinFrame parsed from JSON: id=resultWinFrame");
        }
    }

    auto menuBars = g_parser.getMenuBars();
    for (auto& menuBar : menuBars) {
        BENCH->addControl(menuBar);
    }

    auto allIds = g_parser.getAllControlIds();
    TestUtil::log("Total controls with IDs: %zu", allIds.size());
    for (auto& id : allIds) {
        cout << "  - ID: " << id << endl;
    }

    TestUtil::log("Layout initialization complete");

    testFontInheritance();
}

// 字体继承链：TabControl 声明 fontSize，未声明的子控件（ListView）应沿父链继承
static void testFontInheritance() {
    TestUtil::log("---- Font inheritance assertions ----");
    const string jsonc = R"({
      "controls": [
        { "type": "tab-control", "id": "tcFont", "rect": { "x": 10, "y": 10, "w": 300, "h": 200 },
          "fontSize": 18, "currentIndex": 0,
          "tabs": [
            { "title": "A", "page": {
                "type": "list-view", "id": "lvFont",
                "columns": [ { "title": "Name", "width": 100 } ],
                "rows": [ { "id": "r1", "cells": ["x"] } ]
            } }
          ] }
      ]
    })";
    LayoutParser parser;
    auto root = parser.parseLayout(jsonc);
    if (!root) {
        TestUtil::log("FAIL Font inherit: parse layout");
        return;
    }
    auto lv = parser.findControlById("lvFont");
    if (!lv) {
        TestUtil::log("FAIL Font inherit: list-view not found");
        return;
    }
    auto list = dynamic_pointer_cast<ListView>(lv);
    if (!list) {
        TestUtil::log("FAIL Font inherit: lvFont not ListView");
        return;
    }
    if (list->getFontSize() == 18) {
        TestUtil::log("OK   Font inherit: ListView inherits TabControl fontSize=18");
    } else {
        TestUtil::log("FAIL Font inherit: ListView fontSize=%d (expect 18)", list->getFontSize());
    }
    if (list->getFontName() == FontName::HarmonyOS_Sans_SC_Regular) {
        TestUtil::log("OK   Font inherit: ListView fontName default (no explicit font declared)");
    } else {
        TestUtil::log("FAIL Font inherit: ListView fontName=%d", (int)list->getFontName());
    }
    // 显式声明应覆盖继承
    const string jsonc2 = R"({
      "controls": [
        { "type": "tab-control", "id": "tc2", "rect": { "x": 10, "y": 10, "w": 300, "h": 200 },
          "fontSize": 18, "currentIndex": 0,
          "tabs": [
            { "title": "A", "page": {
                "type": "list-view", "id": "lv2", "fontSize": 12,
                "columns": [ { "title": "Name", "width": 100 } ],
                "rows": [ { "id": "r1", "cells": ["x"] } ]
            } }
          ] }
      ]
    })";
    LayoutParser parser2;
    auto root2 = parser2.parseLayout(jsonc2);
    auto lv2 = root2 ? parser2.findControlById("lv2") : nullptr;
    auto list2 = dynamic_pointer_cast<ListView>(lv2);
    if (list2 && list2->getFontSize() == 12) {
        TestUtil::log("OK   Font inherit: explicit fontSize=12 overrides inheritance");
    } else {
        TestUtil::log("FAIL Font inherit: explicit fontSize override (got %d)", list2 ? list2->getFontSize() : -1);
    }
    // 父显式声明 font{name}：子应继承字体名
    const string jsonc3 = R"({
      "controls": [
        { "type": "tab-control", "id": "tc3", "rect": { "x": 10, "y": 10, "w": 300, "h": 200 },
          "font": { "name": "HarmonyOS_Sans_SC_Regular", "size": 20 }, "currentIndex": 0,
          "tabs": [
            { "title": "A", "page": {
                "type": "list-view", "id": "lv3",
                "columns": [ { "title": "Name", "width": 100 } ],
                "rows": [ { "id": "r1", "cells": ["x"] } ]
            } }
          ] }
      ]
    })";
    LayoutParser parser3;
    auto root3 = parser3.parseLayout(jsonc3);
    auto lv3 = root3 ? parser3.findControlById("lv3") : nullptr;
    auto list3 = dynamic_pointer_cast<ListView>(lv3);
    if (list3 && list3->getFontSize() == 20 && list3->getFontName() == FontName::HarmonyOS_Sans_SC_Regular) {
        TestUtil::log("OK   Font inherit: explicit font{name,size} propagated to ListView");
    } else {
        TestUtil::log("FAIL Font inherit: font{name,size} propagation (size=%d)", list3 ? list3->getFontSize() : -1);
    }
    // 带文字的内部子控件（Button/CheckBox/WinFrame 的文字载体为内部 Label）应沿父链继承
    const string jsonc4 = R"({
      "controls": [
        { "type": "panel", "id": "pFont", "rect": { "x": 10, "y": 10, "w": 400, "h": 300 },
          "fontSize": 16,
          "children": [
            { "type": "button", "id": "btnFont", "rect": { "x": 10, "y": 10, "w": 100, "h": 30 }, "caption": "OK" },
            { "type": "check-box", "id": "cbFont", "rect": { "x": 10, "y": 50, "w": 120, "h": 24 }, "caption": "Check" },
            { "type": "win-frame", "id": "wfFont", "rect": { "x": 10, "y": 140, "w": 200, "h": 120 },
              "title": "Frame" }
          ] }
      ]
    })";
    LayoutParser parser4;
    auto root4 = parser4.parseLayout(jsonc4);
    auto btn = root4 ? dynamic_pointer_cast<Button>(parser4.findControlById("btnFont")) : nullptr;
    auto cb  = root4 ? dynamic_pointer_cast<CheckBox>(parser4.findControlById("cbFont")) : nullptr;
    auto wf  = root4 ? dynamic_pointer_cast<WinFrame>(parser4.findControlById("wfFont")) : nullptr;
    int btnSize = -1, cbSize = -1, wfSize = -1;
    if (btn && btn->getCaptionLabel()) btnSize = btn->getCaptionLabel()->getFontSize();
    if (cb && cb->getCaption()) cbSize = cb->getCaption()->getFontSize();
    if (wf && wf->getTitleLabel()) wfSize = wf->getTitleLabel()->getFontSize();
    if (btnSize == 16 && cbSize == 16 && wfSize == 16) {
        TestUtil::log("OK   Font inherit: Button/CheckBox/WinFrame inner label inherit fontSize=16");
    } else {
        TestUtil::log("FAIL Font inherit: inner labels btn=%d cb=%d wf=%d (expect 16)",
                      btnSize, cbSize, wfSize);
    }
    // Slider 数值标签走专用 label-font 键（kLabelFont，独立于通用继承，见 4.20.2），不参与通用继承
    TestUtil::log("INFO Font inherit: Slider value label uses dedicated label-font key (not generic inherit)");
    TestUtil::log("---- Font inheritance done ----");
}

class LayoutApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_layout");
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
    return TestRunMain<LayoutApp>(argc, argv);
}
