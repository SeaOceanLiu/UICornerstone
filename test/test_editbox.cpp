#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>
#include "EditBox.h"
#include "TextArea.h"
#include "ScrollBar.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "Button.h"
#include "GraphTool.h"

using namespace std;

shared_ptr<EditBox> g_editBox1;
shared_ptr<EditBox> g_editBox2;
shared_ptr<EditBox> g_editBox3;
shared_ptr<EditBox> g_editBox4;
shared_ptr<TextArea> g_textArea;
shared_ptr<TextArea> g_textAreaWrap;
shared_ptr<TextArea> g_textAreaBuilder2x;
shared_ptr<Button> g_clearButton;
shared_ptr<Button> g_addTextButton;

void testBenchInitialize(void) {
    SDL_Log("testEditBoxInitialize");

    g_editBox1 = make_shared<EditBox>(nullptr, SRect(50, 50, 300, 40));
    g_editBox1->setPlaceholder("Single line input...");
    g_editBox1->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "EditBox1 text changed: " << text << endl;
    });
    g_editBox1->setOnEnter([](shared_ptr<Control>) {
        cout << "EditBox1 Enter pressed" << endl;
    });
    BENCH->addControl(g_editBox1);
    SDL_Log("EditBox1 created with placeholder and event handlers");

    g_editBox2 = make_shared<EditBox>(nullptr, SRect(50, 100, 300, 40));
    g_editBox2->setPlaceholder("Password input...");
    g_editBox2->setPasswordMode(true);
    g_editBox2->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "EditBox2 (password) text changed, length: " << text.length() << endl;
    });
    BENCH->addControl(g_editBox2);
    SDL_Log("EditBox2 created in password mode with placeholder and event handler");

    g_editBox3 = make_shared<EditBox>(nullptr, SRect(50, 150, 300, 40));
    g_editBox3->setText("Disabled editbox");
    g_editBox3->setEnable(false);
    BENCH->addControl(g_editBox3);
    SDL_Log("EditBox3 created as disabled editbox");


    g_editBox4 = EditBoxBuilder(nullptr, SRect(50, 430, 300, 80), 2.0f, 2.0f)
        .setPlaceholder("2x scale editbox (builder)...")
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setOnTextChanged([](shared_ptr<Control>, string text) {
            cout << "EditBox4 (2x scale) text changed: " << text << endl;
        })
        .build();
    BENCH->addControl(g_editBox4);
    SDL_Log("EditBox4 created with builder pattern and 2x scale");

    g_textArea = make_shared<TextArea>(nullptr, SRect(50, 210, 400, 200));
    g_textArea->setPlaceholder("Multi-line text area...\nTry typing here!");
    g_textArea->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "TextArea text changed, length: " << text.length() << endl;
    });
    BENCH->addControl(g_textArea);
    SDL_Log("TextArea created with placeholder and event handler");

    g_textAreaWrap = make_shared<TextArea>(nullptr, SRect(500, 50, 250, 300));
    g_textAreaWrap->setWordWrap(true);
    g_textAreaWrap->setLineHeight(22);
    g_textAreaWrap->setText(
        "This is a long text that should wrap to multiple lines automatically. "
        "Line wrapping allows text to flow within the boundaries of the text area.");
    g_textAreaWrap->setPlaceholder("Line wrapping TextArea. Type long text here!");
    g_textAreaWrap->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "TextAreaWrap text changed, length: " << text.length() << endl;
    });
    BENCH->addControl(g_textAreaWrap);
    SDL_Log("TextAreaWrap created with word wrap enabled");

    g_textAreaBuilder2x = TextAreaBuilder(nullptr, SRect(750, 360, 200, 100), 2.0f, 2.0f)
        .setText("This is a 2x scaled TextArea created with Builder pattern. Try typing long text here to test horizontal scrolling!")
        .setPlaceholder("2x scaled TextArea (Builder)...")
        .setOnTextChanged([](shared_ptr<Control>, string text) {
            cout << "TextAreaBuilder2x text changed, length: " << text.length() << endl;
        })
        .build();
    BENCH->addControl(g_textAreaBuilder2x);
    SDL_Log("TextAreaBuilder2x created with builder pattern and 2x scale");

    g_clearButton = make_shared<Button>(nullptr, SRect(50, 420, 100, 40));
    SDL_Log("Creating clear button...") ;
    g_clearButton->setCaption("Clear All");
    SDL_Log("Setting clear button caption...");
    g_clearButton->setOnClick([](shared_ptr<Button> btn) {
        if (g_textArea) {
            g_textArea->setText("");
        }
        cout << "Clear button clicked" << endl;
    });
    SDL_Log("Setting clear button click handler...");
    BENCH->addControl(g_clearButton);
    SDL_Log("Clear button created");

    g_addTextButton = make_shared<Button>(nullptr, SRect(160, 420, 100, 40));
    g_addTextButton->setCaption("Add Text");
    g_addTextButton->setOnClick([](shared_ptr<Button> btn) {
        if (g_textArea) {
            string current = g_textArea->getText();
            g_textArea->setText(current + "\nNew line added at " + to_string(SDL_GetTicks()));
        }
        cout << "Add text button clicked" << endl;
    });
    BENCH->addControl(g_addTextButton);
    SDL_Log("Add text button created");

    SDL_Log("EditBox test controls created");
}

void debugTraceOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    cout << "Category[" << category << "], priority[" << priority << "]: " << message << endl;
    static FILE* logFile = nullptr;
    if (!logFile) {
        logFile = fopen("editbox_log.txt", "w");
    }
    if (logFile) {
        fprintf(logFile, "Category[%d], priority[%d]: %s\n", category, priority, message);
        fflush(logFile);
    }
}

// ==================== AppCallbacks ====================

class EditBoxApp : public AppCallbacks {
public:
    bool onInit() override {
        cout << "Using SDL3 library for EditBox test" << endl;

        SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
        SDL_SetLogOutputFunction(debugTraceOutput, nullptr);

        SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
        cout << "SDL_TOUCH_MOUSE_EVENTS = " << SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS) << endl;

        SSize displaySize = MAINWIN->getDisplaySize();
        GET_INPUTBACKEND->startTextInput();
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
        SDL_Log("Application quit");
    }
};

static EditBoxApp g_app;

// ==================== SDL回调函数 ====================

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!g_app.onInit()) return SDL_APP_FAILURE;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    shared_ptr<Event> gameEvent = nullptr;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_WINDOW_RESIZED:
            MAINWIN->onWindowResized(event->window.data1, event->window.data2);
            BENCH->resized({0, 0, (float)event->window.data1, (float)event->window.data2});
            break;

        case SDL_EVENT_WINDOW_MOVED:
            MAINWIN->onWindowMoved(event->window.data1, event->window.data2);
            break;

        case SDL_EVENT_KEY_DOWN:
            {
                KeyEventData keyData;
                keyData.keycode = SDLKeycodeToKeyCode(event->key.key);
                keyData.scancode = event->key.scancode;
                keyData.mod = SDLKeymodToKeyMod(event->key.mod);
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
    MAINWIN->update(&g_app);
    MAINWIN->render(&g_app);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    MAINWIN->shutdown(&g_app);
}
