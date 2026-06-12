#include <iostream>
#include <memory>
#include "EditBox.h"
#include "TextArea.h"
#include "ScrollBar.h"
#include "Menu.h"
#include "MainWindow.h"
#include "AppCallbacks.h"
#include "Bench.h"
#include "Button.h"
#include "GraphTool.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<MenuBar> g_menuBar;
shared_ptr<MenuPanel> g_filePanel;
shared_ptr<MenuPanel> g_editPanel;
shared_ptr<Button> g_Button;
shared_ptr<GraphTool::DrawingContext> g_dc;

void testBenchInitialize(void) {
    TestUtil::log("testBenchInitialize - START");

    TestUtil::log("testBenchInitialize - creating item1");
    auto item1 = MenuItemBuilder(u8"新建(N)")
        .setShortcut("Ctrl+N")
        .setOnClick([](shared_ptr<MenuItem> item) {
            cout << "New file clicked" << endl;
        })
        .build();
    TestUtil::log("testBenchInitialize - item1 created OK");

    MenuBar::setFontSize(16);

    TestUtil::log("testBenchInitialize - creating MenuPanelBuilder");
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
                MAINWIN->quit();
            })
            .build())
        .build();

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

    g_menuBar = MenuBarBuilder(BENCH)
        .addMenu(u8"文件(F)", g_filePanel)
        .addMenu(u8"编辑(E)", g_editPanel)
        .build();

    TestUtil::log("Add menu bar to bench");
    BENCH->addControl(g_menuBar);

    BENCH->addControl(g_Button = ButtonBuilder(BENCH, {400, 100, 100, 50})
                    .setCaption("Button")
                    .setBackgroundStateColor(StateColor(StateColor::Type::Background).setNormal({128,128,128,255}))
                    .setBorderStateColor(StateColor(StateColor::Type::Border).setNormal({0,0,255,255}))
                    .setOnClick([](shared_ptr<Button> btn) {
                        TestUtil::log("g_menuBar->getRect()={%f, %f, %f, %f}",
                            g_menuBar->getRect().left, g_menuBar->getRect().top,
                            g_menuBar->getRect().width, g_menuBar->getRect().height);
                    })
                    .build()
                );
}

void testGraphToolInitialize(void){
    TestUtil::log("testGraphToolInitialize");
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
    g_dc->drawPolyline(vector<SPoint>({SPoint(100, 200), SPoint(10, 100), SPoint(300, 200), SPoint(200, 300)}), true);
}

class MenuApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_menu");
        cout << "test_menu test" << endl;

        SSize displaySize = MAINWIN->getDisplaySize();
        BENCH->setOnInitial(testBenchInitialize);
        testGraphToolInitialize();
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
        TestUtil::log("SDL_AppQuit");
        cout << "Menu test program exiting" << endl;
    }
};

int main(int argc, char* argv[]) {
    MenuApp app;
    return MAINWIN->run(&app);
}
