#include <iostream>
#include "Button.h"
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "HandleControl.h"
#include "Cursor.h"

using namespace std;

shared_ptr<Button> g_btnTarget;
shared_ptr<Button> g_btnOcclude;
shared_ptr<Button> g_btnSwitch;
shared_ptr<Label>  g_lblStatus;
shared_ptr<HandleControl> g_handle;

void testBenchInitialize() {
    TestUtil::log("test_handlecontrol: creating controls");

    // 目标按钮 1 —— 手柄附加到此按钮
    g_btnTarget = ButtonBuilder(nullptr, SRect(80, 60, 200, 80))
        .setCaption(u8"目标按钮")
        .setOnClick([](shared_ptr<Button>) {
            TestUtil::log(">>> 目标按钮被点击!");
        })
        .build();
    g_btnTarget->create();
    g_btnTarget->setNormalStateBGColor(SColor(60, 60, 60, 255));
    g_btnTarget->setHoverStateBGColor(SColor(80, 80, 80, 255));
    BENCH->addControl(g_btnTarget);

    // 遮挡按钮 —— 与目标按钮部分重叠（右侧覆盖一部分）
    g_btnOcclude = ButtonBuilder(nullptr, SRect(230, 60, 120, 80))
        .setCaption(u8"遮挡按钮")
        .setOnClick([](shared_ptr<Button>) {
            TestUtil::log(">>> 遮挡按钮被点击!");
        })
        .build();
    g_btnOcclude->create();
    g_btnOcclude->setNormalStateBGColor(SColor(40, 100, 40, 255));
    g_btnOcclude->setHoverStateBGColor(SColor(60, 120, 60, 255));
    BENCH->addControl(g_btnOcclude);

    // 切换目标按钮
    g_btnSwitch = ButtonBuilder(nullptr, SRect(80, 180, 200, 50))
        .setCaption(u8"切换手柄目标")
        .setOnClick([](shared_ptr<Button>) {
            if (g_handle->getTarget() == g_btnTarget.get()) {
                g_handle->setTarget(g_btnOcclude);
                g_lblStatus->setCaption(u8"手柄目标: 遮挡按钮");
                TestUtil::log(">>> 手柄目标已切换到: 遮挡按钮");
            } else {
                g_handle->setTarget(g_btnTarget);
                g_lblStatus->setCaption(u8"手柄目标: 目标按钮");
                TestUtil::log(">>> 手柄目标已切换到: 目标按钮");
            }
        })
        .build();
    g_btnSwitch->create();
    g_btnSwitch->setNormalStateBGColor(SColor(40, 40, 100, 255));
    g_btnSwitch->setHoverStateBGColor(SColor(60, 60, 120, 255));
    BENCH->addControl(g_btnSwitch);

    // 状态标签
    g_lblStatus = LabelBuilder(nullptr, SRect(80, 250, 300, 30))
        .setCaption(u8"手柄目标: 目标按钮")
        .setFontSize(16)
        .build();
    g_lblStatus->create();
    g_lblStatus->setTextNormalStateColor(SColor(200, 200, 200, 255));
    BENCH->addControl(g_lblStatus);

    // 创建并附加手柄控件到目标按钮
    g_handle = HandleControlBuilder()
        .setTarget(g_btnTarget)
        .build();

    TestUtil::log("test_handlecontrol: ready — click buttons to verify event pass-through");
}

class HandleApp : public AppCallbacks {
    uint64_t m_frameCount = 0;
    uint64_t m_lastFpsTime = 0;
public:
    bool onInit() override {
        MAINWIN->setTitle("test_handlecontrol");
        TestUtil::log("HandleApp::onInit");
        BENCH->setOnInitial(testBenchInitialize);
        m_lastFpsTime = TestUtil::getTicks();
        return true;
    }

    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }

    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(0.15f, 0.15f, 0.15f, 1.0f));
        GET_RENDERDEVICE->clear();
        BENCH->draw();

        m_frameCount++;
        uint64_t now = TestUtil::getTicks();
        if (now - m_lastFpsTime >= 1000) {
            printf("FPS: %llu\n", m_frameCount);
            m_frameCount = 0;
            m_lastFpsTime = now;
        }
    }

    void onQuit() override {
        TestUtil::log("test_handlecontrol: quit");
    }
};

int main(int argc, char* argv[]) {
    HandleApp app;
    return MAINWIN->run(&app);
}
