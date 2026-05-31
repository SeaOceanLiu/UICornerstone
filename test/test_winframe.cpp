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

shared_ptr<WinFrame> g_winFrame;

void testBenchInitialize(void) {
    SDL_Log("testWinFrameInitialize");

    g_winFrame = WinFrameBuilder(nullptr, SRect(200, 150, 500, 350))
        .setTitle("Minimal Test")
        .setTitleFontSize(24)
        .setTitleTextColor(SColor((uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)255))
        .setTitleBarBGColor(SColor((uint8_t)70, (uint8_t)130, (uint8_t)180, (uint8_t)255))
        .setWinFrameBGColor(SColor((uint8_t)50, (uint8_t)50, (uint8_t)50, (uint8_t)255))
        .setWinFrameBorderColor(SColor((uint8_t)100, (uint8_t)100, (uint8_t)100, (uint8_t)255))
        .setClientBGColor(SColor((uint8_t)60, (uint8_t)60, (uint8_t)60, (uint8_t)255))
        .setEdgeMargin(6.0f)
        .setOnClose([](shared_ptr<Control>) {
            SDL_Log("Close button clicked");
        })
        .build();

    SDL_Log("Adding WinFrame to BENCH...");
    BENCH->addControl(g_winFrame);
    SDL_Log("Showing WinFrame...");
    g_winFrame->show();
    SDL_Log("Initialization complete - after show()");
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
                SDL_Log("MOUSE_DOWN: %f,%f", event->button.x, event->button.y);
                gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_DOWN,
                    make_shared<SPoint>((float)event->button.x, (float)event->button.y));
                BENCH->inputControl(gameEvent);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                SDL_Log("MOUSE_UP: %f,%f", event->button.x, event->button.y);
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
    SDL_SetRenderDrawColor(MainWindow::getInstance()->getRenderer(), 40, 40, 40, 255);
    SDL_RenderClear(MainWindow::getInstance()->getRenderer());
    BENCH->draw();
    SDL_RenderPresent(GET_RENDERER);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)appstate; (void)result;
    ResourceLoader::getInstance()->detachLoadingThread();
    TTF_Quit();
}
