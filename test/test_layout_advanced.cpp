#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "LayoutParser.h"
#include "Menu.h"
#include "ResourceLoader.h"
#include "Bench.h"
#include "MainWindow.h"
#include "HotReloader.h"
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include <SDL3_ttf/SDL_ttf.h>

using namespace std;
namespace fs = std::filesystem;

LayoutParser g_parser;
HotReloader g_reloader;
fs::path g_layoutPath;

void reloadLayout() {
    SDL_Log("[HotReload] Reloading layout...");
    BENCH->removeAllControls();
    g_parser.clear();

    auto root = g_parser.parseLayoutFile(g_layoutPath);
    if (!root) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HotReload] Failed to re-parse layout!");
        return;
    }
    BENCH->addControl(root);

    auto menuBars = g_parser.getMenuBars();
    for (auto& menuBar : menuBars) {
        BENCH->addControl(menuBar);
    }

    auto allIds = g_parser.getAllControlIds();
    SDL_Log("[HotReload] Reloaded: %zu controls with IDs", allIds.size());
}

void onScaleTypeBtnClick(shared_ptr<Control> c) {
    SDL_Log("ScaleType button clicked! (FIT_CENTER)");
}

void onMenuNew(shared_ptr<Control> c) { SDL_Log("Menu: New file"); }
void onMenuOpen(shared_ptr<Control> c) { SDL_Log("Menu: Open file"); }
void onMenuRecent(shared_ptr<Control> c) { SDL_Log("Menu: Recent file"); }
void onMenuExit(shared_ptr<Control> c) { SDL_Log("Menu: Exit"); }
void onMenuUndo(shared_ptr<Control> c) { SDL_Log("Menu: Undo"); }
void onMenuRedo(shared_ptr<Control> c) { SDL_Log("Menu: Redo"); }
void onMenuAbout(shared_ptr<Control> c) { SDL_Log("Menu: About"); }

void testBenchInitialize(void) {
    SDL_Log("testLayoutAdvancedInitialize");

    g_parser.registerHandler("onScaleTypeBtnClick", onScaleTypeBtnClick);
    g_parser.registerHandler("onMenuNew", onMenuNew);
    g_parser.registerHandler("onMenuOpen", onMenuOpen);
    g_parser.registerHandler("onMenuRecent", onMenuRecent);
    g_parser.registerHandler("onMenuExit", onMenuExit);
    g_parser.registerHandler("onMenuUndo", onMenuUndo);
    g_parser.registerHandler("onMenuRedo", onMenuRedo);
    g_parser.registerHandler("onMenuAbout", onMenuAbout);

    string basePath = string(SDL_GetBasePath());
    g_layoutPath = fs::path(basePath) / "layouts" / "test_layout_advanced.json";
    SDL_Log("Loading layout from: %s", g_layoutPath.string().c_str());

    auto root = g_parser.parseLayoutFile(g_layoutPath);
    if (!root) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to parse layout file!");
        return;
    }

    BENCH->addControl(root);

    auto menuBars = g_parser.getMenuBars();
    for (auto& menuBar : menuBars) {
        BENCH->addControl(menuBar);
    }

    auto allIds = g_parser.getAllControlIds();
    SDL_Log("Total controls with IDs: %zu", allIds.size());
    for (auto& id : allIds) {
        SDL_Log("  - ID: %s", id.c_str());
    }

    g_reloader = HotReloader(g_layoutPath, reloadLayout);
    SDL_Log("[HotReload] Watching: %s", g_layoutPath.filename().string().c_str());
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("LayoutParserAdvancedTest", "1.0.0", "com.example.layoutparseradvancedtest");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cout << "Failed to initialize SDL: " << SDL_GetError() << endl;
        return SDL_APP_FAILURE;
    }
    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    BENCH->setOnInitial(testBenchInitialize);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    shared_ptr<Event> gameEvent = nullptr;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_WINDOW_RESIZED:
            MAINWIN->handleWindowEvent(event->window);
            BENCH->resized(SRect(0, 0, (float)event->window.data1, (float)event->window.data2));
            break;

        case SDL_EVENT_WINDOW_MOVED:
            MAINWIN->handleWindowEvent(event->window);
            break;

        case SDL_EVENT_KEY_DOWN:
            {
                KeyEventData keyData;
                keyData.keycode = event->key.key;
                keyData.scancode = event->key.scancode;
                keyData.mod = event->key.mod;
                keyData.repeat = event->key.repeat != 0;
                gameEvent = make_shared<Event>(EventName::KEY_DOWN, keyData);
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_TEXT_INPUT:
            {
                TextInputEventData textData;
                textData.text = event->text.text;
                textData.start = -1;
                textData.length = -1;
                gameEvent = make_shared<Event>(EventName::TEXT_INPUT, textData);
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_TEXT_EDITING:
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            {
                MouseWheelEventData wheelData;
                wheelData.x = event->wheel.x;
                wheelData.y = event->wheel.y;
                wheelData.mouseX = event->wheel.mouse_x;
                wheelData.mouseY = event->wheel.mouse_y;
                gameEvent = make_shared<Event>(EventName::MOUSE_WHEEL, wheelData);
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            gameEvent = make_shared<Event>(EventName::MOUSE_MOVING,
                make_shared<SPoint>((float)event->motion.x, (float)event->motion.y));
            BENCH->inputControl(gameEvent);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_DOWN,
                    make_shared<SPoint>((float)event->button.x, (float)event->button.y));
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_UP,
                    make_shared<SPoint>((float)event->button.x, (float)event->button.y));
                BENCH->inputControl(gameEvent);
            }
            break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    g_reloader.poll();
    BENCH->eventLoopEntry();
    BENCH->update();

    SDL_SetRenderDrawColor(MainWindow::getInstance()->getRenderer(), 40, 40, 40, 255);
    SDL_RenderClear(MainWindow::getInstance()->getRenderer());

    BENCH->draw();

    SDL_RenderPresent(GET_RENDERER);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    SDL_Log("Application quit");
}
