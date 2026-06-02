#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <memory>
#include <fstream>
#include "Button.h"
#include "Actor.h"
#include "MainWindow.h"
#include "Bench.h"
#include "EditBox.h"
#include "TextArea.h"

using namespace std;

static ofstream g_logFile;

void logOutput(const string& message) {
    if (!g_logFile.is_open()) {
        g_logFile.open("button_log.txt", ios::out);
    }
    g_logFile << message << endl;
    g_logFile.flush();
    cout << message << endl;
}

void debugTraceOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    logOutput(string("Category[") + to_string(category) + "], priority[" + to_string(priority) + "]: " + message);
}

shared_ptr<Button> g_button1;
shared_ptr<Button> g_button2;
shared_ptr<Button> g_button3;
shared_ptr<Button> g_button4;
shared_ptr<Button> g_button5;
shared_ptr<Button> g_button6;

void testBenchInitialize(void) {
    SDL_Log(u8"testButtonInitialize");

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    BENCH->addControl(LabelBuilder(nullptr, SRect(50, 50, 120, 50))
        .setCaption(u8"应被遮挡的 Label")
        .build());

    // 1. 普通 button（只有背景，无文字无图片）
    g_button1 = ButtonBuilder(nullptr, SRect(50, 50, 120, 50))
        .setBackgroundStateColor(StateColor::Type::Background)
        .setBorderStateColor(redBorder)
        .setOnClick([](shared_ptr<Button> btn) {
            logOutput(u8"Button1 (普通按钮) clicked!");
        })
        .build();
    g_button1->create();
    BENCH->addControl(g_button1);

    // 2. 带文字的 button
    g_button2 = ButtonBuilder(nullptr, SRect(200, 50, 150, 50))
        .setCaption(u8"带文字的按钮")
        .setTextStateColor(StateColor::Type::Text)
        .setBackgroundStateColor(StateColor::Type::Background)
        .setBorderStateColor(redBorder)
        .setOnClick([](shared_ptr<Button> btn) {
            logOutput(u8"Button2 (带文字) clicked!");
        })
        .build();
    g_button2->create();
    BENCH->addControl(g_button2);

    // 3. 有图片的 button（使用 Actor）
    shared_ptr<Actor> normalActor = ActorBuilder(nullptr)
        .loadFromFile("D:\\GitSpace\\UIControls\\build\\test\\Debug\\assets\\images\\icon.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> hoverActor = ActorBuilder(nullptr)
        .loadFromFile("D:\\GitSpace\\UIControls\\build\\test\\Debug\\assets\\images\\cross_over.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> pressedActor = ActorBuilder(nullptr)
        .loadFromFile("D:\\GitSpace\\UIControls\\build\\test\\Debug\\assets\\images\\cross_down.png")
        .setMatchParentRect(true)
        .build();

    g_button3 = ButtonBuilder(nullptr, SRect(380, 50, 120, 50))
        .setNormalStateActor(normalActor)
        .setHoverStateActor(hoverActor)
        .setPressedStateActor(pressedActor)
        .setBorderStateColor(redBorder)
        .setOnClick([](shared_ptr<Button> btn) {
            logOutput(u8"Button3 (有图片) clicked!");
        })
        .build();
    g_button3->create();
    BENCH->addControl(g_button3);

    // 4. 有动画的 button（LuotiAni）
    {
        shared_ptr<LuotiAni> rotateAni = LuotiAniBuilder(BENCH)
            .loadAniDesc(ResourceLoader::RID_rotateBtn_jsonc)
            .prepare()
            .setAutoStart()
            .build();
        g_button4 = ButtonBuilder(nullptr, SRect(530, 50, 120, 50))
            .setLuotiAni(rotateAni)
            .setTransparent(true)
            .setOnClick([](shared_ptr<Button> btn) {
                logOutput(u8"Button4 (有动画) clicked!");
            })
            .build();
        g_button4->create();
        BENCH->addControl(g_button4);
    }

    // 5. 2x缩放的含有文字、图片的 button
    shared_ptr<Actor> normalActor2x = ActorBuilder(nullptr)
        .loadFromFile("D:\\GitSpace\\UIControls\\build\\test\\Debug\\assets\\images\\icon.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> hoverActor2x = ActorBuilder(nullptr)
        .loadFromFile("D:\\GitSpace\\UIControls\\build\\test\\Debug\\assets\\images\\cross_over.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> pressedActor2x = ActorBuilder(nullptr)
        .loadFromFile("D:\\GitSpace\\UIControls\\build\\test\\Debug\\assets\\images\\cross_down.png")
        .setMatchParentRect(true)
        .build();

    g_button5 = ButtonBuilder(nullptr, SRect(50, 150, 240, 100), 2.0f, 2.0f)
        .setCaption(u8"2x缩放按钮\n含文字图片")
        .setCaptionSize(24)
        .setNormalStateActor(normalActor2x)
        .setHoverStateActor(hoverActor2x)
        .setPressedStateActor(pressedActor2x)
        .setBackgroundStateColor(StateColor::Type::Background)
        .setBorderStateColor(redBorder)
        .setOnClick([](shared_ptr<Button> btn) {
            logOutput(u8"Button5 (2x缩放) clicked!");
        })
        .build();
    g_button5->create();
    BENCH->addControl(g_button5);

    // 6. 2x缩放的动画 button（LuotiAni）
    {
        shared_ptr<LuotiAni> rotateAni2x = LuotiAniBuilder(BENCH)
            .loadAniDesc(ResourceLoader::RID_rotateBtn_jsonc)
            .prepare()
            .setAutoStart()
            .build();
        g_button6 = ButtonBuilder(nullptr, SRect(530, 150, 240, 100), 2.0f, 2.0f)
            .setLuotiAni(rotateAni2x)
            .setTransparent(true)
            .setOnClick([](shared_ptr<Button> btn) {
                logOutput(u8"Button6 (2x缩放动画) clicked!");
            })
            .build();
        g_button6->create();
        BENCH->addControl(g_button6);
    }

    logOutput(u8"Button test controls created");
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void)appstate;
    (void)argc;
    (void)argv;

    BENCH->setOnInitial(testBenchInitialize);

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogOutputFunction(debugTraceOutput, nullptr);

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    logOutput(string("SDL_TOUCH_MOUSE_EVENTS = ") + SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS));

    SDL_SetAppMetadata("ButtonTest", "1.0.0", "com.example.button");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        logOutput(string("Failed to initialize SDL: ") + SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    (void)appstate;

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
                gameEvent = make_shared<Event>(EventName::KEY_DOWN, make_shared<KeyEventData>(keyData));
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_TEXT_INPUT:
            {
                TextInputEventData textData;
                textData.text = event->text.text;
                gameEvent = make_shared<Event>(EventName::TEXT_INPUT, make_shared<TextInputEventData>(textData));
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
                gameEvent = make_shared<Event>(EventName::MOUSE_WHEEL, make_shared<MouseWheelEventData>(wheelData));
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
    (void)appstate;
    (void)result;
    // Clean up resources
    ResourceLoader::getInstance()->detachLoadingThread();
    logOutput(u8"程序结束");
}