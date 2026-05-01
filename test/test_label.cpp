#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "EventQueue.h"
#include "ResourceLoader.h"
#include "EditBox.h"
#include "TextArea.h"

using namespace std;

shared_ptr<Label> g_label1;
shared_ptr<Label> g_label2;
shared_ptr<Label> g_label3;
shared_ptr<Label> g_label4;
shared_ptr<Label> g_label5;
shared_ptr<Label> g_label6;
shared_ptr<Label> g_label7;
shared_ptr<Label> g_label8;
shared_ptr<Label> g_label9;
shared_ptr<Label> g_label10;
shared_ptr<Label> g_label11;
shared_ptr<Label> g_label12;
shared_ptr<Label> g_label13;
shared_ptr<Label> g_label14;
shared_ptr<Label> g_label15;
shared_ptr<Label> g_label16;
shared_ptr<Label> g_label17;
shared_ptr<Label> g_label18;
shared_ptr<Label> g_label19;

void testBenchInitialize(void) {
    SDL_Log("testLabelInitialize");

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    // Row 1: Single line alignment (y=30, height=30)
    g_label1 = LabelBuilder(nullptr, SRect(20, 30, 130, 30))
        .setCaption("L1: Left")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(1)
        .build();
    BENCH->addControl(g_label1);

    g_label2 = LabelBuilder(nullptr, SRect(160, 30, 130, 30))
        .setCaption("L2: Right")
        .setAlignmentMode(AlignmentMode::AM_TOP_RIGHT)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(2)
        .build();
    BENCH->addControl(g_label2);

    g_label3 = LabelBuilder(nullptr, SRect(300, 30, 130, 30))
        .setCaption("L3: Center")
        .setAlignmentMode(AlignmentMode::AM_TOP_CENTER)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(3)
        .build();
    BENCH->addControl(g_label3);

    // Row 2: Single line truncate (y=80, height=25)
    g_label4 = LabelBuilder(nullptr, SRect(20, 80, 70, 25))
        .setCaption("L4: long text 70px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(4)
        .build();
    BENCH->addControl(g_label4);

    g_label5 = LabelBuilder(nullptr, SRect(100, 80, 40, 25))
        .setCaption("L5: long t 40px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(5)
        .build();
    BENCH->addControl(g_label5);

    g_label6 = LabelBuilder(nullptr, SRect(150, 80, 35, 25))
        .setCaption("L6: long 35px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(6)
        .build();
    BENCH->addControl(g_label6);

    g_label7 = LabelBuilder(nullptr, SRect(195, 80, 30, 25))
        .setCaption("L7: long 30px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(7)
        .build();
    BENCH->addControl(g_label7);

    g_label8 = LabelBuilder(nullptr, SRect(235, 80, 20, 25))
        .setCaption("L8: lo 20px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(8)
        .build();
    BENCH->addControl(g_label8);

    // Row 3: Multi-line (y=125, height=50)
    g_label9 = LabelBuilder(nullptr, SRect(20, 125, 110, 60))
        .setCaption(u8"L9: Line1\nLine2\nLine3")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(9)
        .build();
    BENCH->addControl(g_label9);

    g_label10 = LabelBuilder(nullptr, SRect(140, 125, 90, 95))
        .setEnableExpand(false)
        .setCaption(u8"L10: L1\nL2\nL3\nL4")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(10)
        .build();
    BENCH->addControl(g_label10);

    // Row 4: 2x scaled Chinese top alignment (y=200, height=70 -> 140 with scale)
    g_label11 = LabelBuilder(nullptr, SRect(20, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L11: Top左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(11)
        .build();
    BENCH->addControl(g_label11);

    g_label12 = LabelBuilder(nullptr, SRect(450, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L12: Top中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(12)
        .build();
    BENCH->addControl(g_label12);

    g_label13 = LabelBuilder(nullptr, SRect(880, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L13: Top右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(13)
        .build();
    BENCH->addControl(g_label13);

    // Row 5: 2x scaled Chinese middle alignment (y=400, after 140+60 gap)
    g_label14 = LabelBuilder(nullptr, SRect(20, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L14: Mid左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(14)
        .build();
    BENCH->addControl(g_label14);

    g_label15 = LabelBuilder(nullptr, SRect(450, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L15: Mid中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(15)
        .build();
    BENCH->addControl(g_label15);

    g_label16 = LabelBuilder(nullptr, SRect(880, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L16: Mid右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_MID_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(16)
        .build();
    BENCH->addControl(g_label16);

    // Row 6: 2x scaled Chinese bottom alignment (y=600, after 140+60 gap)
    g_label17 = LabelBuilder(nullptr, SRect(20, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L17: Bot左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(17)
        .build();
    BENCH->addControl(g_label17);

    g_label18 = LabelBuilder(nullptr, SRect(450, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L18: Bot中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(18)
        .build();
    BENCH->addControl(g_label18);

    g_label19 = LabelBuilder(nullptr, SRect(880, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L19: Bot右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        // .setDebugDraw(true)
        .setId(19)
        .build();
    BENCH->addControl(g_label19);

    SDL_Log("Label test controls created");
}

void debugTraceOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    cout << "Category[" << category << "], priority[" << priority << "]: " << message << endl;
    static FILE* logFile = nullptr;
    if (!logFile) {
        logFile = fopen("label_log.txt", "w");
    }
    if (logFile) {
        fprintf(logFile, "Category[%d], priority[%d]: %s\n", category, priority, message);
        fflush(logFile);
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    cout << "Using SDL3 library for Label test" << endl;

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogOutputFunction(debugTraceOutput, nullptr);

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    cout << "SDL_TOUCH_MOUSE_EVENTS = " << SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS) << endl;

    SDL_SetAppMetadata("LabelTest", "1.0.0", "com.example.labeltest");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cout << "Failed to initialize SDL: " << SDL_GetError() << endl;
        return SDL_APP_FAILURE;
    }
    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
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
            BENCH->resized({0, 0, (float)event->window.data1, (float)event->window.data2});
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
            gameEvent = make_shared<Event>(EventName::MOUSE_MOVING, make_shared<SPoint>((float)event->motion.x, (float)event->motion.y));
            BENCH->inputControl(gameEvent);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_DOWN, make_shared<SPoint>((float)event->button.x, (float)event->button.y));
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_UP, make_shared<SPoint>((float)event->button.x, (float)event->button.y));
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