#include "LayoutParser.h"
#include "Menu.h"
#include "Bench.h"
#include "MainWindow.h"
#include "HotReloader.h"
#include "DataContext.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include "TestUtils.h"

using namespace std;
namespace fs = std::filesystem;

LayoutParser g_parser;
HotReloader g_reloader;
fs::path g_layoutPath;

void reloadLayout() {
    TestUtil::log("[HotReload] Reloading layout...");
    BENCH->removeAllControls();
    g_parser.clear();

    auto root = g_parser.parseLayoutFile(g_layoutPath);
    if (!root) {
        TestUtil::log("[HotReload] Failed to re-parse layout!");
        return;
    }
    BENCH->addControl(root);

    auto menuBars = g_parser.getMenuBars();
    for (auto& menuBar : menuBars) {
        BENCH->addControl(menuBar);
    }

    auto allIds = g_parser.getAllControlIds();
    TestUtil::log("[HotReload] Reloaded: %zu controls with IDs", allIds.size());
}

void onScaleTypeBtnClick(shared_ptr<Control> c) {
    TestUtil::log("ScaleType button clicked! (FIT_CENTER)");
}

void onMenuNew(shared_ptr<Control> c) { TestUtil::log("Menu: New file"); }
void onMenuOpen(shared_ptr<Control> c) { TestUtil::log("Menu: Open file"); }
void onMenuRecent(shared_ptr<Control> c) { TestUtil::log("Menu: Recent file"); }
void onMenuExit(shared_ptr<Control> c) { TestUtil::log("Menu: Exit"); }
void onMenuUndo(shared_ptr<Control> c) { TestUtil::log("Menu: Undo"); }
void onMenuRedo(shared_ptr<Control> c) { TestUtil::log("Menu: Redo"); }
void onMenuAbout(shared_ptr<Control> c) { TestUtil::log("Menu: About"); }

void testBenchInitialize(void) {
    TestUtil::log("testLayoutAdvancedInitialize");

    g_parser.registerHandler("onScaleTypeBtnClick", onScaleTypeBtnClick);
    g_parser.registerHandler("onMenuNew", onMenuNew);
    g_parser.registerHandler("onMenuOpen", onMenuOpen);
    g_parser.registerHandler("onMenuRecent", onMenuRecent);
    g_parser.registerHandler("onMenuExit", onMenuExit);
    g_parser.registerHandler("onMenuUndo", onMenuUndo);
    g_parser.registerHandler("onMenuRedo", onMenuRedo);
    g_parser.registerHandler("onMenuAbout", onMenuAbout);
    g_parser.registerHandler("onGreeterClick", [](shared_ptr<Control>) {
        TestUtil::log("[Component] Greeter button clicked!");
    });
    g_parser.registerHandler("onFramedGreet", [](shared_ptr<Control>) {
        TestUtil::log("[Component] FramedGreeter button clicked! (nested component test)");
    });
    g_parser.registerHandler("onDecProgress", [](shared_ptr<Control>) {
        auto ctx = DataContext::instance();
        double v = ctx->get("progressValue").asDouble() - 10.0;
        if (v < 0) v = 100.0;
        ctx->set("progressValue", v);
    });
    g_parser.registerHandler("onIncProgress", [](shared_ptr<Control>) {
        auto ctx = DataContext::instance();
        double v = ctx->get("progressValue").asDouble() + 10.0;
        if (v > 100) v = 0.0;
        ctx->set("progressValue", v);
    });

    string basePath = Platform::GetBasePath();
    g_layoutPath = fs::path(basePath) / "layouts" / "test_layout_advanced.json";
    TestUtil::log("Loading layout from: %s", g_layoutPath.string().c_str());

    auto root = g_parser.parseLayoutFile(g_layoutPath);
    if (!root) {
        TestUtil::log("Failed to parse layout file!");
        return;
    }

    BENCH->addControl(root);

    auto menuBars = g_parser.getMenuBars();
    for (auto& menuBar : menuBars) {
        BENCH->addControl(menuBar);
    }

    auto allIds = g_parser.getAllControlIds();
    TestUtil::log("Total controls with IDs: %zu", allIds.size());
    for (auto& id : allIds) {
        cout << "  - ID: " << id << endl;
    }

    g_reloader = HotReloader(g_layoutPath, reloadLayout);
    TestUtil::log("[HotReload] Watching: %s", g_layoutPath.filename().string().c_str());

    // Data binding: set initial values
    auto ctx = DataContext::instance();
    ctx->set("sharedText", string("Hello from DataContext!"));
    ctx->set("progressValue", 42.0);
}

class LayoutAdvancedApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->getWindow()->setResizable(true);
        BENCH->setOnInitial(testBenchInitialize);
        return true;
    }

    void onUpdate() override {
        g_reloader.poll();
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
    LayoutAdvancedApp app;
    return MAINWIN->run(&app);
}
