#include <iostream>
#include <memory>
#include <fstream>
#include "Button.h"
#include "Actor.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "LuotiAni.h"
#include "TestUtils.h"
#include "PlatformUtils.h"
#include "TestInstance.h"

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

shared_ptr<Button> g_button1;
shared_ptr<Button> g_button2;
shared_ptr<Button> g_button3;
shared_ptr<Button> g_button4;
shared_ptr<Button> g_button5;
shared_ptr<Button> g_button6;

void testBenchInitialize(shared_ptr<Bench>) {
    TestUtil::log("testButtonInitialize");

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    // luotiAni 属性系统 SetPtr（kJsonLuotiAni → setLuotiAni）
    {
        auto btn = ButtonBuilder(nullptr, SRect(10, 300, 160, 40))
            .setCaption("LuotiAni Src")
            .build();
        BENCH->addControl(btn);
        auto la = std::make_shared<LuotiAni>(nullptr);
        int rc = btn->setPtrProperty(PropertyNames::kLuotiAni, la.get());
        printf("[TEST] setPtrProperty(luotiAni)=%d\n", rc);
        if (rc != 1) printf("[TEST-FAIL] luotiAni setPtr\n");
        int rc2 = btn->setPtrProperty(PropertyNames::kLuotiAni, nullptr);
        if (rc2 != 1) printf("[TEST-FAIL] luotiAni setPtr(null)\n");
        else printf("[TEST] setPtrProperty(luotiAni,null)=1\n");
        BENCH->removeControl(btn);
    }

    BENCH->addControl(LabelBuilder(nullptr, SRect(50, 50, 120, 50))
        .setCaption(u8"应被遮挡的 Label")
        .build());

    g_button1 = ButtonBuilder(nullptr, SRect(50, 50, 120, 50))
        .setBackgroundStateColor(StateColor::Type::Background)
        .setBorderStateColor(redBorder)
        .setOnClick([](shared_ptr<Button> btn) {
            logOutput(u8"Button1 (普通按钮) clicked!");
        })
        .build();
    g_button1->create();
    BENCH->addControl(g_button1);

    // 属性读回：font-size 写 24 → 读 24（Button 标题字号统一 font-size）
    g_button1->setIntProperty("font-size", 24);
    int cbFs = 0;
    if (g_button1->getIntProperty("font-size", cbFs) == 1 && cbFs == 24) {
        printf("PASS: Button font-size roundtrip = 24\n");
    } else {
        printf("FAIL: Button font-size readback = %.1f\n", cbFs);
    }
    g_button1->setStringProperty("caption", "readback");
    const char* cbCap = nullptr;
    if (g_button1->getStringProperty("caption", cbCap) == 1 && cbCap && strcmp(cbCap, "readback") == 0) {
        printf("PASS: Button caption readback\n");
    } else {
        printf("FAIL: Button caption readback (%s)\n", cbCap ? cbCap : "null");
    }

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

    shared_ptr<Actor> normalActor = ActorBuilder(nullptr)
        .loadFromFile("assets/images/icon.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> hoverActor = ActorBuilder(nullptr)
        .loadFromFile("assets/images/cross_over.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> pressedActor = ActorBuilder(nullptr)
        .loadFromFile("assets/images/cross_down.png")
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

    {
        shared_ptr<LuotiAni> rotateAni = LuotiAniBuilder(BENCH)
            .loadAniDesc(string("animations/rotateBtn/rotateBtn.jsonc"))
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

    shared_ptr<Actor> normalActor2x = ActorBuilder(nullptr)
        .loadFromFile("assets/images/icon.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> hoverActor2x = ActorBuilder(nullptr)
        .loadFromFile("assets/images/cross_over.png")
        .setMatchParentRect(true)
        .build();

    shared_ptr<Actor> pressedActor2x = ActorBuilder(nullptr)
        .loadFromFile("assets/images/cross_down.png")
        .setMatchParentRect(true)
        .build();

    // 与 g_button3 (1x, 380,50,120,50, icon.png) 对比：同素材同 rect 同样式（无 caption），仅差 2x 缩放
    g_button5 = ButtonBuilder(nullptr, SRect(50, 150, 120, 50), 2.0f, 2.0f)
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

    {
        shared_ptr<LuotiAni> rotateAni2x = LuotiAniBuilder(BENCH)
            .loadAniDesc(string("animations/rotateBtn/rotateBtn.jsonc"))
            .prepare()
            .setAutoStart()
            .build();
        // 与 g_button4 (1x, 530,50,120,50) 对比：同 rect 同动画，仅差 2x 缩放
        g_button6 = ButtonBuilder(nullptr, SRect(530, 150, 120, 50), 2.0f, 2.0f)
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

class ButtonApp : public AppCallbacks {
    uint64_t m_frameCount = 0;
    uint64_t m_lastFpsTime = 0;
public:
    bool onInit() override {
        MAINWIN->setTitle("test_button");
        logOutput(u8"ButtonApp::onInit");
        BENCH->setOnInitial(testBenchInitialize);
        m_lastFpsTime = Platform::GetTicks();
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

        m_frameCount++;
        uint64_t now = Platform::GetTicks();
        if (now - m_lastFpsTime >= 1000) {
            printf("FPS: %llu\n", m_frameCount);
            m_frameCount = 0;
            m_lastFpsTime = now;
        }
    }

    void onQuit() override {
        logOutput(u8"程序结束");
    }
};

int main(int argc, char* argv[]) {
    return TestRunMain<ButtonApp>(argc, argv);
}
