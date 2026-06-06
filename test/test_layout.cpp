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
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

static LayoutParser g_parser;
static shared_ptr<WinFrame> g_resultWinFrame;

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

void testBenchInitialize(void) {
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
}

class LayoutApp : public AppCallbacks {
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
    LayoutApp app;
    return MAINWIN->run(&app);
}
