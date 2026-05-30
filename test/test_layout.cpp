#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <string>
#include <memory>
#include "LayoutParser.h"
#include "MainWindow.h"
#include "Bench.h"
#include "ResourceLoader.h"
#include "TextArea.h"
#include "Button.h"
#include "EditBox.h"
#include "Dialog.h"
#include <SDL3_ttf/SDL_ttf.h>

using namespace std;

static LayoutParser g_parser;
static shared_ptr<Dialog> g_resultDialog;

void onSubmitClicked(shared_ptr<Control> c) {
    SDL_Log("Button clicked via auto-binding!");
    if (g_resultDialog) {
        g_resultDialog->show();
    }
}

void onTextChanged(string text) {
    SDL_Log("Text changed via manual binding: %s", text.c_str());
}

void onNotesChanged(shared_ptr<Control> c) {
    SDL_Log("TextArea content changed via auto-binding!");
}

void onAgreeChanged(shared_ptr<Control> c) {
    SDL_Log("CheckBox state changed via auto-binding!");
}

void onImageBtnClick(shared_ptr<Control> c) {
    SDL_Log("Image button clicked!");
}

void onAniBtnClick(shared_ptr<Control> c) {
    SDL_Log("Animated button clicked!");
}

void onCaptionLabelBtnClick(shared_ptr<Control> c) {
    SDL_Log("CaptionLabel button clicked!");
}

// ===== Menu event handlers =====
void onMenuNew(shared_ptr<Control> c) {
    SDL_Log("Menu: New file");
}

void onMenuOpen(shared_ptr<Control> c) {
    SDL_Log("Menu: Open file");
}

void onMenuRecent(shared_ptr<Control> c) {
    SDL_Log("Menu: Recent file");
}

void onMenuExit(shared_ptr<Control> c) {
    SDL_Log("Menu: Exit");
    SDL_Event quitEvent;
    quitEvent.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quitEvent);
}

void onMenuUndo(shared_ptr<Control> c) {
    SDL_Log("Menu: Undo");
}

void onMenuRedo(shared_ptr<Control> c) {
    SDL_Log("Menu: Redo");
}

void onMenuAbout(shared_ptr<Control> c) {
    SDL_Log("Menu: About");
}

void testBenchInitialize(void) {
    SDL_Log("testLayoutInitialize");

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

    string basePath = string(SDL_GetBasePath());
    fs::path layoutPath = fs::path(basePath) / "layouts" / "test_layout.json";
    SDL_Log("Loading layout from: %s", layoutPath.string().c_str());

    shared_ptr<Control> root = g_parser.parseLayoutFile(layoutPath);
    if (root != nullptr) {
        BENCH->addControl(root);

        shared_ptr<Control> found = g_parser.findControlById("nameEdit");
        if (found != nullptr) {
            shared_ptr<EditBox> editBox = dynamic_pointer_cast<EditBox>(found);
            if (editBox) {
                editBox->setOnTextChanged(onTextChanged);
            }
        }

        shared_ptr<Control> submitBtn = g_parser.findControlById("submitBtn");
        if (submitBtn != nullptr) {
            SDL_Log("findControlById: submitBtn found");
        }

        vector<string> ids = g_parser.getAllControlIds();
        SDL_Log("Total controls with IDs: %zu", ids.size());
        for (size_t i = 0; i < ids.size(); i++) {
            SDL_Log("  - ID: %s", ids[i].c_str());
        }

        auto resultCtrl = g_parser.findControlById("resultDialog");
        if (resultCtrl) {
            g_resultDialog = dynamic_pointer_cast<Dialog>(resultCtrl);
            if (g_resultDialog) {
                SDL_Log("Dialog parsed from JSON: id=resultDialog");
            }
        }

        auto captionLabelCtrl = g_parser.findControlById("captionLabelBtn");
        if (captionLabelCtrl) {
            shared_ptr<Button> captionLabelBtn = dynamic_pointer_cast<Button>(captionLabelCtrl);
            if (captionLabelBtn) {
                shared_ptr<Label> label = captionLabelBtn->getCaptionLabel();
                if (label) {
                    SDL_Log("captionLabelBtn::getCaptionLabel() = \"%s\", fontSize=%d, alignment=%d",
                        label->getCaption().c_str(),
                        label->getFontSize(),
                        (int)label->getAlignmentMode());
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "captionLabelBtn: getCaptionLabel() returned null!");
                }
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "captionLabelBtn: dynamic_pointer_cast<Button> failed!");
            }
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "findControlById: captionLabelBtn not found!");
        }

        auto menuBarCtrl = g_parser.findControlById("mainMenuBar");
        if (menuBarCtrl) {
            shared_ptr<MenuBar> menuBar = dynamic_pointer_cast<MenuBar>(menuBarCtrl);
            if (menuBar) {
                SDL_Log("MenuBar parsed from JSON: id=mainMenuBar, barHeight=%f, fontSize=%f",
                    menuBar->getBarHeight(), MenuBar::getFontSize());
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "mainMenuBar: dynamic_pointer_cast<MenuBar> failed!");
            }
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "findControlById: mainMenuBar not found!");
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to parse layout file!");
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    cout << "Using SDL3 library for Layout Parser test" << endl;
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetAppMetadata("LayoutParserTest", "1.0.0", "com.example.layoutparsertest");

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
    ResourceLoader::getInstance()->detachLoadingThread();
    TTF_Quit();
}
