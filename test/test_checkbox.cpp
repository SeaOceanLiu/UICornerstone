#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>
#include "CheckBox.h"
#include "MainWindow.h"
#include "Bench.h"

#include "EditBox.h"
#include "TextArea.h"
#include "GraphTool.h"

using namespace std;

shared_ptr<CheckBox> g_checkbox1;
shared_ptr<CheckBox> g_checkbox2;
shared_ptr<CheckBox> g_checkbox3;
shared_ptr<CheckBox> g_checkbox4;
shared_ptr<CheckBox> g_checkbox5;
shared_ptr<CheckBox> g_checkbox6;
shared_ptr<CheckBox> g_checkbox7;
shared_ptr<CheckBox> g_checkbox8;
shared_ptr<CheckBox> g_checkbox9;
shared_ptr<CheckBox> g_checkbox10;
shared_ptr<CheckBox> g_checkbox11;
shared_ptr<CheckBox> g_checkbox11x;
shared_ptr<CheckBox> g_checkbox12;
shared_ptr<CheckBox> g_checkbox13;
shared_ptr<CheckBox> g_checkbox14;
shared_ptr<CheckBox> g_checkbox15;
shared_ptr<CheckBox> g_checkbox16;

void debugTraceOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    static FILE* logFile = nullptr;
    if (!logFile) {
        logFile = fopen("checkbox_log.txt", "w");
    }
    if (logFile) {
        fprintf(logFile, "Category[%d], priority[%d]: %s\n", category, priority, message);
        fflush(logFile);
    }
    cout << "Category[" << category << "], priority[" << priority << "]: " << message << endl;
}

void testBenchInitialize(void) {
    SDL_Log("testCheckBoxInitialize");

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    g_checkbox1 = make_shared<CheckBox>(nullptr, SRect(50, 50, 100, 30));
    g_checkbox1->getCaption()->setCaption("1. Accept Terms");
    g_checkbox1->setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
        cout << "Checkbox1 state changed to: " << (int)state << endl;
    });
    g_checkbox1->create();
    BENCH->addControl(g_checkbox1);

    g_checkbox2 = make_shared<CheckBox>(nullptr, SRect(50, 100, 200, 30));
    g_checkbox2->getCaption()->setCaption("2. Enable Feature");
    g_checkbox2->setCheckState(CheckState::Checked);
    g_checkbox2->setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
        cout << "Checkbox2 state changed to: " << (int)state << endl;
    });
    g_checkbox2->create();
    BENCH->addControl(g_checkbox2);

    g_checkbox3 = CheckBoxBuilder(nullptr, SRect(50, 150, 200, 30))
        .setStyle(CheckBoxStyle::Cross)
        .setCaptionText("3. Cross Style")
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox3 (Cross) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox3);

    g_checkbox4 = CheckBoxBuilder(nullptr, SRect(50, 200, 200, 30))
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionText("4. Circle Style")
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox4 (Circle) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox4);

    g_checkbox5 = CheckBoxBuilder(nullptr, SRect(50, 250, 200, 30))
        .setLayout(CheckBoxLayout::TextLeft)
        .setCaptionText("5. Text on Left")
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox5 (TextLeft) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox5);

    g_checkbox6 = CheckBoxBuilder(nullptr, SRect(50, 300, 200, 30))
        .setCaptionText("6. Tri-state Checkbox")
        .setCheckState(CheckState::Indeterminate)
        .setBorderStateColor(redBorder)
        .setOnCheckChanged([](shared_ptr<CheckBox> cb, CheckState, CheckState state) {
            cout << "Checkbox6 (Tri-state) state: " << (int)state << endl;
        })
        .build();
    BENCH->addControl(g_checkbox6);

    g_checkbox7 = CheckBoxBuilder(nullptr, SRect(50, 350, 200, 30))
        .setCaptionText("7. Disabled Checkbox")
        .setEnable(false)
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox7);

    g_checkbox8 = CheckBoxBuilder(nullptr, SRect(300, 50, 250, 30))
        .setCaptionText("8. Custom Check Color")
        .setCheckState(CheckState::Checked)
        .setCheckColor({255, 0, 255, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox8);

    g_checkbox9 = CheckBoxBuilder(nullptr, SRect(300, 100, 250, 30))
        .setStyle(CheckBoxStyle::Cross)
        .setCaptionText("9. Custom Cross Color")
        .setCheckState(CheckState::Checked)
        .setCrossColor({0, 255, 255, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox9);

    g_checkbox10 = CheckBoxBuilder(nullptr, SRect(300, 150, 250, 30))
        .setCaptionText("10. Custom Indeterminate Color")
        .setCheckState(CheckState::Indeterminate)
        .setIndeterminateColor({255, 165, 0, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox10);

    g_checkbox11 = CheckBoxBuilder(nullptr, SRect(300, 200, 300, 60), 2.0f, 2.0f)
        .setCaptionText(u8"11. 2x缩放复选框")
        .setCheckState(CheckState::Checked)
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionSize(24)
        .setBorderStateColor(redBorder)
        .build();
    StateColor sc = g_checkbox11->getCaption()->getTextStateColor();
    sc.setNormal({0, 0, 255, 255});
    g_checkbox11->getCaption()->setBorderStateColor(sc);

    BENCH->addControl(g_checkbox11);

    g_checkbox11x = CheckBoxBuilder(nullptr, SRect(300, 320, 300, 60), 1.0f, 1.0f)
        .setCaptionText(u8"11x. 1x缩放复选框")
        .setCheckState(CheckState::Checked)
        .setStyle(CheckBoxStyle::Circle)
        .setCaptionSize(24)
        .setBorderStateColor(redBorder)
        .build();
    StateColor scx = g_checkbox11x->getCaption()->getTextStateColor();
    scx.setNormal({0, 0, 255, 255});
    g_checkbox11x->getCaption()->setBorderStateColor(scx);

    BENCH->addControl(g_checkbox11x);

    g_checkbox12 = CheckBoxBuilder(nullptr, SRect(1000, 50, 250, 30))
        .setCaptionText("12. Custom Box Border Color")
        .setCheckState(CheckState::Checked)
        .setBoxBorderColor({0, 128, 255, 255})
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox12);

    g_checkbox13 = CheckBoxBuilder(nullptr, SRect(1000, 100, 200, 30))
        .setCaptionText("13. FontSize 12")
        .setCaptionSize(12)
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox13);

    g_checkbox14 = CheckBoxBuilder(nullptr, SRect(1000, 140, 200, 30))
        .setCaptionText("14. FontSize 20")
        .setCaptionSize(20)
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox14);

    g_checkbox15 = CheckBoxBuilder(nullptr, SRect(1000, 190, 250, 80))
        .setCaptionText("15. Line 1\nLine 2\nLine 3")
        .setCheckState(CheckState::Checked)
        .setBorderStateColor(redBorder)
        .setVerticalAlign(CheckBoxVerticalAlign::Center)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox15);

    g_checkbox16 = CheckBoxBuilder(nullptr, SRect(1000, 280, 250, 30))
        .setCaptionText("16. Two-State Only (No Tri-state)")
        .setCheckState(CheckState::Checked)
        .setTriStateEnabled(false)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_checkbox16);
    StateColor stateColor;
    stateColor.setNormal({0, 0, 255, 255});
    g_checkbox16->getCaption()->setTextStateColor(stateColor);

    SDL_Log("CheckBox test controls created");
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    BENCH->setOnInitial(testBenchInitialize);

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogOutputFunction(debugTraceOutput, nullptr);

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    cout << "SDL_TOUCH_MOUSE_EVENTS = " << SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS) << endl;

    SDL_SetAppMetadata("CheckBoxTest", "1.0.0", "com.example.checkbox");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cout << "Failed to initialize SDL: " << SDL_GetError() << endl;
        return SDL_APP_FAILURE;
    }
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

    GET_RENDERDEVICE->setDrawColor(SColor(40.0f/255.0f, 40.0f/255.0f, 40.0f/255.0f, 1.0f));
    GET_RENDERDEVICE->clear();

    BENCH->draw();

    // GraphTool::DrawingContext dc(MainWindow::getInstance()->getRenderer());
    // dc.setPenColor(GraphTool::SColor(1.0f, 1.0f, 1.0f,1.0f));
    // float penWidth = 10.0f;  // 根据X轴缩放比例调整线宽
    // dc.setPenWidth(penWidth);
    // dc.drawCircleWithThickness(500, 500, 50, penWidth, GraphTool::SColor(1.0f, 0.0f, 0.0f, 1.0f));


    GET_RENDERDEVICE->present();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Clean up resources

    SDL_Log("Application quit");
}