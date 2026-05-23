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
shared_ptr<MainMenu> g_fileMenu;
shared_ptr<MenuItem> newFileItem;
shared_ptr<MenuItem> openFileItem;
shared_ptr<MenuItem> recentFilesItem;
shared_ptr<MenuItem> saveFileItem;
shared_ptr<MenuSeparator> separatorItem;
shared_ptr<MenuItem> exitItem;

shared_ptr<MainMenu> g_editMenu;
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
    SDL_Log("testBenchInitialize");
    // 使用链形式调用创建菜单栏
    g_menuBar = dynamic_pointer_cast<MenuBar>(MenuBarBuilder()
                // // .setBGColor(SDL_Color{23, 23, 24, 255})  // 灰色背景
                // // .setTextColor(SDL_Color{255, 255, 255, 255})  // 白色文字
                // .addBeforeEventHandlingWatcher(EventName::MOUSE_LBUTTON_DOWN)
                // .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)

                // .addMainMenu(MainMenuBuilder(u8"测试(T)")
                //                 .addMenuItem(MenuItemBuilder(u8"开始测试")
                //                             .setOnClick([](shared_ptr<MenuBase> item) {
                //                                 cout << "Start test clicked" << endl;
                //                             }).build()
                //                 )
                //                 .build()
                // )
                .addMainMenu(g_fileMenu = dynamic_pointer_cast<MainMenu>(MainMenuBuilder(u8"文件(F)")
                                .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                // .addMenuItem(newFileItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder(u8"新建(N)")
                                //             .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                //             .setOnClick([](shared_ptr<MenuBase> item) {
                                //                 cout << "New file clicked" << endl;
                                //             })
                                //             .build())
                                // )
                                // .addMenuItem(openFileItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder(u8"打开(O)")
                                //             .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                //             .setOnClick([](shared_ptr<MenuBase> item) {
                                //                 cout << "Open file clicked" << endl;
                                //             })
                                //             .build())
                                // )
                                .addMenuItem(recentFilesItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder(u8"最近打开的文件")
                                            .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                            .addSubMenuItem(MenuItemBuilder(u8"file1.txt")
                                                            .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                                            .setOnClick([](shared_ptr<MenuBase> item) {
                                                                cout << "Recent file1.txt clicked" << endl;
                                                            })
                                                            .build()
                                            )
                                            .addSubMenuItem(MenuItemBuilder(u8"file2.txt")
                                                            .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                                            .setOnClick([](shared_ptr<MenuBase> item) {
                                                                cout << "Recent file2.txt clicked" << endl;
                                                            })
                                                            .build()
                                            )
                                            .build())
                                )
                                // .addMenuItem(saveFileItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder(u8"保存(S)")
                                //             .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                //             .setOnClick([](shared_ptr<MenuBase> item) {
                                //                 cout << "Save file clicked" << endl;
                                //             })
                                //             .build())
                                // )
                                // .addMenuItem(separatorItem = dynamic_pointer_cast<MenuSeparator>(MenuSeparatorBuilder()
                                //             .build())
                                // )
                                // .addMenuItem(exitItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder(u8"退出")
                                //             .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
                                //             .setOnClick([](shared_ptr<MenuBase> item) {
                                //                 cout << "Exit program" << endl;
                                //                 SDL_Event quitEvent;
                                //                 quitEvent.type = SDL_EVENT_QUIT;
                                //                 SDL_PushEvent(&quitEvent);
                                //             })
                                //             .build())
                                // )
                                .build()
                ))
                .build());

    // SDL_Log("to create edit menu");
    // // 创建编辑菜单
    // g_editMenu = dynamic_pointer_cast<MainMenu>(MainMenuBuilder(u8"编辑(E)")
    //             .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
    //             // .setNormalStateBGColor(SDL_Color{128, 128, 128, 255})  // 灰色背景
    //             // .setTextNormalStateColor(SDL_Color{255, 255, 255, 255})  // 白色文字
    //             // .setHoverStateBGColor(SDL_Color{200, 200, 200, 255})  // 浅灰色悬停
    //             .build());

    // SDL_Log("to create undo menu items");
    // // 创建编辑菜单项
    // auto undoItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder("UndoUndoUndo")
    //                 .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
    //                 .setOnClick([](shared_ptr<MenuBase> item) {
    //                     cout << "Undo operation" << endl;
    //                 })
    //                 .build());

    // SDL_Log("to create redo menu items");
    // auto redoItem = dynamic_pointer_cast<MenuItem>(MenuItemBuilder("RedoRedoRedo")
    //                 .addBeforeEventHandlingWatcher(EventName::MOUSE_MOVING)
    //                 .setOnClick([](shared_ptr<MenuBase> item) {
    //                     cout << "Redo operation" << endl;
    //                 })
    //                 .build());

    // SDL_Log("to add menu items to edit menu");
    // // 将菜单项添加到编辑菜单
    // g_editMenu->addMenuItem(undoItem);
    // g_editMenu->addMenuItem(redoItem);

    // SDL_Log("to add main menu(Edit) to menu bar");
    // // 将菜单添加到菜单栏
    // // g_menuBar->addMainMenu(g_fileMenu);
    // g_menuBar->addMainMenu(g_editMenu);

    SDL_Log("Add menu bar to bench");
    // 将菜单栏添加到Bench控件管理器
    BENCH->addControl(g_menuBar);

    // SDL_Log("Menubar rect={%f, %f, %f, %f}", g_menuBar->getRect().left, g_menuBar->getRect().top, g_menuBar->getRect().width, g_menuBar->getRect().height);

    // // 调试信息
    // cout << "MenuBar created: " << (g_menuBar ? "Yes" : "No") << endl;
    // cout << "File menu created: " << (g_fileMenu ? "Yes" : "No") << endl;
    // cout << "Edit menu created: " << (g_editMenu ? "Yes" : "No") << endl;
    // cout << "MenuBar enabled: " << (g_menuBar->getEnable() ? "Yes" : "No") << endl;
    // cout << "File menu enabled: " << (g_fileMenu->getEnable() ? "Yes" : "No") << endl;
    // cout << "Edit menu enabled: " << (g_editMenu->getEnable() ? "Yes" : "No") << endl;

    // cout << "Menu controls test program started successfully" << endl;
    // cout << "Click File menu to see dropdown menu" << endl;
    // cout << "Click Exit menu item to exit program" << endl;
    // cout << "Press ESC to exit program" << endl;

    BENCH->addControl(g_Button = ButtonBuilder(BENCH, {400, 100, 100, 50})
                    .setCaption("Button")
                    .setBackgroundStateColor(StateColor(StateColor::Type::Background).setNormal({128,128,128,255}))
                    .setBorderStateColor(StateColor(StateColor::Type::Border).setNormal({0,0,255,255}))
                    .setOnClick([](shared_ptr<Button> btn) {
                        SDL_Log("g_menuBar->getRect()={%f, %f, %f, %f}", g_menuBar->getRect().left, g_menuBar->getRect().top, g_menuBar->getRect().width, g_menuBar->getRect().height);
                        // SDL_Log("Button clicked, draw polygon shapes using GraphTool");
                        // vector<SPoint> polygonPoints = {
                        //     SPoint(100, 300),
                        //     SPoint(400, 300),
                        //     // SPoint(400, 600),
                        //     SPoint(100, 600)
                        // };
                        // g_dc->drawPolygon(polygonPoints, false, true);

                    })
                    .build()
                );

}
void testGraphToolInitialize(void){
    SDL_Log("testGraphToolInitialize");
    g_dc = make_shared<GraphTool::DrawingContext>(BENCH->getRenderer());
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
    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
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

    /* clear the window to the draw color. */
    SDL_SetRenderDrawColor(MainWindow::getInstance()->getRenderer(), 40, 40, 40, 255);  // 深灰色背景
    SDL_RenderClear(MainWindow::getInstance()->getRenderer());

    BENCH->draw();
    // testGraphTool();

    // SDL_FRect rect = { 102.000000, 157.000000, 182.000000, 44.000000};
    // SDL_RenderRect(MainWindow::getInstance()->getRenderer(), &rect);

    // SDL_SetRenderDrawColor(MainWindow::getInstance()->getRenderer(), 255, 0, 0, 0); // White
    // SDL_FRect rect2 = { 257.000000, 157.000000, 22.000000, 44.000000};
    // SDL_RenderRect(MainWindow::getInstance()->getRenderer(), &rect2);

    // SDL_SetRenderDrawColor(MainWindow::getInstance()->getRenderer(), 0, 0, 255, 0); // White
    // SDL_FRect rect3 = { 417.000000, 167.000000, 12.000000, 24.000000};
    // SDL_RenderRect(MainWindow::getInstance()->getRenderer(), &rect3);

    // Present rendering
    SDL_RenderPresent(GET_RENDERER);
    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_Log("SDL_AppQuit");
    // Clean up resources
    // 这里要强制释放资源，因为要确保在后面调用TTF_Quit()之前，要把FontSuite打开的字体都关闭掉
    // BENCH.reset();
    // 线程需要显式detach，否则Android下会报泄漏
    ResourceLoader::getInstance()->detachLoadingThread();
    TTF_Quit();
    cout << "Menu test program exiting" << endl;
}
