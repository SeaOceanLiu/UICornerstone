// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <memory>
#include "WinFrame.h"
#include "MainWindow.h"
#include "Bench.h"
#include "Label.h"
#include "Button.h"

using namespace std;

shared_ptr<WinFrame> g_winFrame1;
shared_ptr<WinFrame> g_winFrame2;
shared_ptr<Button> g_btn1;
shared_ptr<Button> g_btn2;

void testBenchInitialize(void) {
    SDL_Log("testWinFrameInitialize");

    // WinFrame 1: resizable (default)
    g_winFrame1 = WinFrameBuilder(nullptr, SRect(100, 100, 400, 300))
        .setTitle("Resizable WinFrame")
        .setTitleFontSize(24)
        .setTitleTextColor(SColor((uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)255))
        .setTitleBarBGColor(SColor((uint8_t)70, (uint8_t)130, (uint8_t)180, (uint8_t)255))
        .setWinFrameBGColor(SColor((uint8_t)50, (uint8_t)50, (uint8_t)50, (uint8_t)255))
        .setWinFrameBorderColor(SColor((uint8_t)100, (uint8_t)100, (uint8_t)100, (uint8_t)255))
        .setClientBGColor(SColor((uint8_t)60, (uint8_t)60, (uint8_t)60, (uint8_t)255))
        .setEdgeMargin(6.0f)
        .setOnClose([](shared_ptr<Control>) {
            g_btn1->setCaption("Show WinFrame 1");
            SDL_Log("WinFrame 1 closed");
        })
        .build();

    // WinFrame 2: NOT resizable
    g_winFrame2 = WinFrameBuilder(nullptr, SRect(350, 250, 400, 300))
        .setTitle("Unresizable WinFrame")
        .setTitleFontSize(24)
        .setTitleTextColor(SColor((uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)255))
        .setTitleBarBGColor(SColor((uint8_t)180, (uint8_t)70, (uint8_t)70, (uint8_t)255))
        .setWinFrameBGColor(SColor((uint8_t)50, (uint8_t)50, (uint8_t)50, (uint8_t)255))
        .setWinFrameBorderColor(SColor((uint8_t)100, (uint8_t)100, (uint8_t)100, (uint8_t)255))
        .setClientBGColor(SColor((uint8_t)60, (uint8_t)60, (uint8_t)60, (uint8_t)255))
        .setEdgeMargin(6.0f)
        .setResizable(false)
        .setOnClose([](shared_ptr<Control>) {
            g_btn2->setCaption("Show WinFrame 2");
            SDL_Log("WinFrame 2 closed");
        })
        .build();

    SDL_Log("Adding WinFrames to BENCH...");
    BENCH->addControl(g_winFrame1);
    BENCH->addControl(g_winFrame2);
    // Both start hidden — user must click buttons to show them

    // Toggle button for WinFrame 1
    BENCH->addControl(g_btn1 = ButtonBuilder(BENCH, SRect(20, 20, 180, 40))
        .setCaption("Show WinFrame 1")
        .setBackgroundStateColor(
            StateColor(StateColor::Type::Background)
                .setNormal({100,100,100,255})
                .setHover({130,130,130,255})
                .setPressed({80,80,80,255}))
        .setOnClick([](shared_ptr<Button>) {
            if (g_winFrame1->getVisible()) {
                g_winFrame1->hide();
                g_btn1->setCaption("Show WinFrame 1");
            } else {
                g_winFrame1->show();
                g_btn1->setCaption("Hide WinFrame 1");
            }
        })
        .build());

    // Toggle button for WinFrame 2
    BENCH->addControl(g_btn2 = ButtonBuilder(BENCH, SRect(220, 20, 190, 40))
        .setCaption("Show WinFrame 2")
        .setBackgroundStateColor(
            StateColor(StateColor::Type::Background)
                .setNormal({100,100,100,255})
                .setHover({130,130,130,255})
                .setPressed({80,80,80,255}))
        .setOnClick([](shared_ptr<Button>) {
            if (g_winFrame2->getVisible()) {
                g_winFrame2->hide();
                g_btn2->setCaption("Show WinFrame 2");
            } else {
                g_winFrame2->show();
                g_btn2->setCaption("Hide WinFrame 2");
            }
        })
        .build());

    SDL_Log("Initialization complete");
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void)appstate; (void)argc; (void)argv;
    BENCH->setOnInitial(testBenchInitialize);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetAppMetadata("WinFrameTest", "1.0.0", "com.example.winframetest");
    if (!SDL_Init(SDL_INIT_VIDEO)) return SDL_APP_FAILURE;
    if (!TTF_Init()) return SDL_APP_FAILURE;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    (void)appstate;
    shared_ptr<Event> gameEvent = nullptr;
    switch (event->type) {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        case SDL_EVENT_WINDOW_RESIZED:
            MAINWIN->handleWindowEvent(event->window);
            BENCH->resized({0, 0, (float)event->window.data1, (float)event->window.data2});
            break;
        case SDL_EVENT_WINDOW_MOVED: MAINWIN->handleWindowEvent(event->window); break;
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
    (void)appstate;
    BENCH->eventLoopEntry();
    BENCH->update();
    GET_RENDERDEVICE->setDrawColor(SColor(40.0f/255.0f, 40.0f/255.0f, 40.0f/255.0f, 1.0f));
    GET_RENDERDEVICE->clear();
    BENCH->draw();
    GET_RENDERDEVICE->present();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)appstate; (void)result;
    ResourceLoader::getInstance()->detachLoadingThread();
    TTF_Quit();
}
