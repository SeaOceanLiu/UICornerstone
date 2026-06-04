#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>
#include "EditBox.h"
#include "TextArea.h"
#include "ScrollBar.h"
#include "Menu.h"
#include "MainWindow.h"
#include "Bench.h"
#include "Button.h"
#include "GraphTool.h"

using namespace std;

// 全局变量存储菜单控件
shared_ptr<MenuBar> g_menuBar;
shared_ptr<MenuPanel> g_filePanel;
shared_ptr<MenuPanel> g_editPanel;
shared_ptr<Button> g_Button;
shared_ptr<GraphTool::DrawingContext> g_dc;

void debugTraceOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    cout << "Category[" << category << "], priority[" << priority << "]: " << message << endl;
    static FILE* logFile = nullptr;
    if (!logFile) {
        logFile = fopen("menu_log.txt", "w");
    }
    if (logFile) {
        fprintf(logFile, "Category[%d], priority[%d]: %s\n", category, priority, message);
        fflush(logFile);
    }
}
void testBenchInitialize(void) {
    SDL_Log("testBenchInitialize - START");

    SDL_Log("testBenchInitialize - creating item1");
    auto item1 = MenuItemBuilder(u8"新建(N)")
        .setShortcut("Ctrl+N")
        .setOnClick([](shared_ptr<MenuItem> item) {
            cout << "New file clicked" << endl;
        })
        .build();
    SDL_Log("testBenchInitialize - item1 created OK");

    // 设置菜单字体大小（必须在创建菜单面板之前调用）
    MenuBar::setFontSize(16);

    // 创建文件菜单面板
    SDL_Log("testBenchInitialize - creating MenuPanelBuilder");
    g_filePanel = MenuPanelBuilder()
        .addItem(MenuItemBuilder(u8"新建(N)")
            .setShortcut("Ctrl+N")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "New file clicked" << endl;
            })
            .build())
        .addItem(MenuItemBuilder(u8"打开(O)")
            .setShortcut("Ctrl+O")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Open file clicked" << endl;
            })
            .build())
        .addItem(MenuItemBuilder(u8"最近打开的文件")
            .setSubMenu(MenuPanelBuilder()
                .addItem(MenuItemBuilder(u8"file1.txt")
                    .setOnClick([](shared_ptr<MenuItem> item) {
                        cout << "Recent file1.txt clicked" << endl;
                    })
                    .build())
                .addItem(MenuItemBuilder(u8"file2.txt")
                    .setOnClick([](shared_ptr<MenuItem> item) {
                        cout << "Recent file2.txt clicked" << endl;
                    })
                    .build())
                .build())
            .build())
        .addSeparator()
        .addItem(MenuItemBuilder(u8"保存(S)")
            .setShortcut("Ctrl+S")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Save file clicked" << endl;
            })
            .build())
        .addSeparator()
        .addItem(MenuItemBuilder(u8"退出")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Exit program" << endl;
                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            })
            .build())
        .build();

    // 创建编辑菜单面板
    g_editPanel = MenuPanelBuilder()
        .addItem(MenuItemBuilder(u8"撤销")
            .setShortcut("Ctrl+Z")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Undo operation" << endl;
            })
            .build())
        .addItem(MenuItemBuilder(u8"重做")
            .setShortcut("Ctrl+Y")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Redo operation" << endl;
            })
            .build())
        .addSeparator()
        .addItem(MenuItemBuilder(u8"剪切")
            .setShortcut("Ctrl+X")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Cut operation" << endl;
            })
            .build())
        .addItem(MenuItemBuilder(u8"复制")
            .setShortcut("Ctrl+C")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Copy operation" << endl;
            })
            .build())
        .addItem(MenuItemBuilder(u8"粘贴")
            .setShortcut("Ctrl+V")
            .setOnClick([](shared_ptr<MenuItem> item) {
                cout << "Paste operation" << endl;
            })
            .build())
        .build();

    // 使用Builder链式调用创建菜单栏
    g_menuBar = MenuBarBuilder(BENCH)
        .addMenu(u8"文件(F)", g_filePanel)
        .addMenu(u8"编辑(E)", g_editPanel)
        .build();

    SDL_Log("Add menu bar to bench");
    BENCH->addControl(g_menuBar);

    BENCH->addControl(g_Button = ButtonBuilder(BENCH, {400, 100, 100, 50})
                    .setCaption("Button")
                    .setBackgroundStateColor(StateColor(StateColor::Type::Background).setNormal({128,128,128,255}))
                    .setBorderStateColor(StateColor(StateColor::Type::Border).setNormal({0,0,255,255}))
                    .setOnClick([](shared_ptr<Button> btn) {
                        SDL_Log("g_menuBar->getRect()={%f, %f, %f, %f}",
                            g_menuBar->getRect().left, g_menuBar->getRect().top,
                            g_menuBar->getRect().width, g_menuBar->getRect().height);
                    })
                    .build()
                );
}
void testGraphToolInitialize(void){
    SDL_Log("testGraphToolInitialize");
    g_dc = make_shared<GraphTool::DrawingContext>(BENCH->getRenderDevice());
    g_dc->setPenColor(GraphTool::SColor((uint8_t)255, 0, 0));
    g_dc->setCornerStyle(GraphTool::CornerStyle::Hard);
    g_dc->setPenWidth(20);
}
void testGraphTool(void){
    g_dc->drawLine(0, 0, 100, 100);
    g_dc->drawRect({150, 50, 100, 80}, false);
    g_dc->drawRoundedRect({300, 50, 100, 80}, 15, false);
    g_dc->drawEllipse(SPoint(550, 90), 50, 30, false);
    g_dc->drawArc(SPoint(300, 200), 200, 0, M_PI/2, false);
    g_dc->drawCircle(SPoint(700, 200), 40, false);
    // g_dc->drawPolygon(vector<SPoint>({SPoint(500, 250), SPoint(600, 250), SPoint(650, 300), SPoint(550, 350), SPoint(450, 300)}), false, true);
    g_dc->drawPolyline(vector<SPoint>({SPoint(100, 200), SPoint(10, 100), SPoint(300, 200), SPoint(200, 300)}), true);
    // vector<SPoint> polygonPoints = {
    //     SPoint(100, 300),
    //     SPoint(400, 300),
    //     // SPoint(400, 600),
    //     SPoint(100, 600)
    // };
    // g_dc->drawPolygon(polygonPoints, false, true);
}
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    cout << "Using SDL3 library for menu test" << endl;

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogOutputFunction(debugTraceOutput, nullptr);

    // 禁止触摸事件转换为鼠标事件
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    cout << "SDL_TOUCH_MOUSE_EVENTS = " << SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS) << endl;

    SDL_SetAppMetadata("MenuTest", "1.0.0", "com.example.menutest");

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cout << "Failed to initialize SDL: " << SDL_GetError() << endl;
        return SDL_APP_FAILURE;
    }
    // 初始化主窗口和Bench
    SSize displaySize = MAINWIN->getDisplaySize();
    BENCH->setOnInitial(testBenchInitialize);
    testGraphToolInitialize();
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    shared_ptr<Event> gameEvent = nullptr;
    shared_ptr<SPoint> pos;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_MOUSE_MOTION:
            // SDL_Log("Pushing mouse button moving event to Bench");
            pos = make_shared<SPoint>(event->motion.x, event->motion.y);
            gameEvent = make_shared<Event>(EventName::MOUSE_MOVING, pos);
            BENCH->inputControl(gameEvent);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            pos = make_shared<SPoint>(event->button.x, event->button.y);
            cout << "Mouse button down @ {" << event->button.x << ", " << event->button.y << "}" << endl;
            switch(event->button.button){
                case SDL_BUTTON_LEFT:
                    gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_DOWN, pos);
                    break;
                case SDL_BUTTON_RIGHT:
                    gameEvent = make_shared<Event>(EventName::MOUSE_RBUTTON_DOWN, pos);
                    break;
                case SDL_BUTTON_MIDDLE:
                    gameEvent = make_shared<Event>(EventName::MOUSE_MBUTTON_DOWN, pos);
                    break;
                default:
                    break;
            }
            if (gameEvent != nullptr) {
                SDL_Log("Pushing mouse button down event to Bench");
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            pos = make_shared<SPoint>(event->button.x, event->button.y);
            cout << "Mouse button up @ {" << event->button.x << ", " << event->button.y << "}" << endl;
            switch(event->button.button){
                case SDL_BUTTON_LEFT:
                    gameEvent = make_shared<Event>(EventName::MOUSE_LBUTTON_UP, pos);
                    break;
                case SDL_BUTTON_RIGHT:
                    gameEvent = make_shared<Event>(EventName::MOUSE_RBUTTON_UP, pos);
                    break;
                case SDL_BUTTON_MIDDLE:
                    gameEvent = make_shared<Event>(EventName::MOUSE_MBUTTON_UP, pos);
                    break;
                default:
                    break;
            }
            if (gameEvent != nullptr) {
                SDL_Log("Pushing mouse button up event to Bench");
                BENCH->inputControl(gameEvent);
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_ESCAPE) {
                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            MainWindow::handleWindowEvent(event->window);
            BENCH->resized({0, 0, (float)event->window.data1, (float)event->window.data2});
            break;

        case SDL_EVENT_WINDOW_MOVED:
            MainWindow::handleWindowEvent(event->window);
            break;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    BENCH->eventLoopEntry();
    BENCH->update();

    // 清屏
    GET_RENDERDEVICE->setDrawColor(SColor(40.0f/255.0f, 40.0f/255.0f, 40.0f/255.0f, 1.0f));
    GET_RENDERDEVICE->clear();

    BENCH->draw();

    // Present rendering
    GET_RENDERDEVICE->present();
    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_Log("SDL_AppQuit");
    // Clean up resources

    cout << "Menu test program exiting" << endl;
}
