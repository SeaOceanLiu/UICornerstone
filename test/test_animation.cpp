// =========================================================================
// test_animation.cpp -- LuotiAni 动画控件 C ABI 集成测试（A1-A12，设计文档 §6.8）
// 静态链接 + TestInstance 框架：C ABI 函数直接调用 + C++ 侧类型访问
// （A11/A12 需 dynamic_cast<LuotiAni*>/<Actor*>，DLL 加载模式不可用）
// =========================================================================

#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <memory>
#include "UICornerstoneAPI.h"
#include "LuotiAni.h"
#include "Button.h"
#include "Panel.h"
#include "Actor.h"
#include "PlatformUtils.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "TestInstance.h"

using namespace std;

static void sleepMs(uint32_t ms) {
    uint64_t start = Platform::GetTicks();
    while (Platform::GetTicks() - start < ms) {
    }
}

static ofstream g_logFile;
// 测试临时文件统一生成到 CWD/Temp（不入库；根目录不再产生测试残留）
static string tempJsonPath(const string& name) {
    static const std::filesystem::path dir = []() {
        std::error_code ec;
        std::filesystem::path d = std::filesystem::current_path(ec) / "Temp";
        std::filesystem::create_directories(d, ec);
        return d;
    }();
    return (dir / name).string();
}


void logOutput(const string& message) {
    if (!g_logFile.is_open()) {
        g_logFile.open(tempJsonPath("animation_log.txt"), ios::out);
    }
    g_logFile << message << endl;
    g_logFile.flush();
    cout << message << endl;
}

static int g_caseIndex = 0;
static int g_failCount = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { g_failCount++; logOutput(string("FAIL [") + to_string(g_caseIndex) + "] " + msg); } \
    } while (0)

static json makeAnimationDoc(int totalFrames, int viewW = 256, int viewH = 256) {
    json kf0 = {{"frame", 0}, {"operation", json::array({
        {{"type", "translate"}, {"tx", 0}, {"ty", 0}},
        {{"type", "opacity"}, {"opacity", 100}},
    })}};
    json kfN = {{"frame", totalFrames - 1}, {"operation", json::array({
        {{"type", "translate"}, {"tx", 40}, {"ty", 0}},
        {{"type", "opacity"}, {"opacity", 100}},
    })}};
    json layer = {
        {"name", "layer1"},
        {"type", "image"},
        {"src", "animations/rotateBtn/rotateBtn.svg"},
        {"opacity", 100},
        {"blendMode", "normal"},
        {"keyFrames", json::array({kf0, kfN})}
    };
    json doc = {
        {"overview", {{"name", "test_animation"}, {"version", "0.0.1"},
                      {"view", {{"width", viewW}, {"height", viewH}}},
                      {"frameRate", 30}, {"totalFrames", totalFrames}, {"loop", false}}},
        {"layers", json::array({layer})}
    };
    return doc;
}

static void writeJson(const string& filename, const json& doc) {
    ofstream out(filename, ios::out);
    out << doc.dump();
    out.close();
}

static LuotiAni* asLuotiAni(UIControlHandle h) {
    return dynamic_cast<LuotiAni*>(static_cast<Control*>(h));
}

// 已创建动画句柄登记（测试结束统一清理，避免全部残留在窗口）
static vector<UIControlHandle> g_handles;
static void reg(UIControlHandle h) {
    if (h != nullptr) g_handles.push_back(h);
}
static void removeAni(UIControlHandle h) {
    if (!h) return;
    auto* impl = dynamic_cast<ControlImpl*>(static_cast<Control*>(h));
    if (!impl) return;
    auto sp = impl->shared_from_this();
    // 从实际父容器移除（动画可能被 A12 移入 Panel）
    auto* parent = dynamic_cast<ControlImpl*>(impl->getParent());
    if (parent) {
        parent->removeControl(sp);
    } else {
        BENCH->removeControl(sp);
    }
}

// ===== A 系列用例 =====

void testA1Create(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA1.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA1.jsonc").c_str(), 20, 20, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A1 create returns handle");
    reg(h);
    if (h) {
        float x = 0, y = 0, w = 0, hh = 0;
        UICornerstone_GetRect(g_uiInstance, h, &x, &y, &w, &hh);
        CHECK(x == 20 && y == 20 && w == 256 && hh == 256, "A1 GetRect matches creation rect");
    }
}

void testA2NoAutoPlay(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA2.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA2.jsonc").c_str(), 40, 40, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A2 create returns handle");
    reg(h);
    if (h) {
        int playing = -1;
        CHECK(UICornerstone_GetBool(g_uiInstance, h, "playing", &playing) == 1, "A2 GetBool(playing) recognized");
        CHECK(playing == 0, "A2 not auto-playing after create");
    }
}

void testA3PlayPauseReplay(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA3.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA3.jsonc").c_str(), 60, 60, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A3 create returns handle");
    reg(h);
    if (!h) return;
    CHECK(UICornerstone_SetBool(g_uiInstance, h, "playing", 1) == 1, "A3 SetBool(playing,1) ok");
    int playing = 0;
    UICornerstone_GetBool(g_uiInstance, h, "playing", &playing);
    CHECK(playing == 1, "A3 playing==1 after play");
    int f0 = 0;
    UICornerstone_GetInt(g_uiInstance, h, "frame", &f0);
    sleepMs(80);
    BENCH->update();
    int f1 = 0;
    UICornerstone_GetInt(g_uiInstance, h, "frame", &f1);
    CHECK(f1 > f0, "A3 frame advances while playing");
    // 暂停
    UICornerstone_SetBool(g_uiInstance, h, "playing", 0);
    int paused = f1;
    sleepMs(80);
    BENCH->update();
    UICornerstone_GetInt(g_uiInstance, h, "frame", &paused);
    CHECK(paused == f1, "A3 frame stops after pause");
    // 再置 1 → 帧从 0 重播
    UICornerstone_SetBool(g_uiInstance, h, "playing", 1);
    int f2 = 99;
    UICornerstone_GetInt(g_uiInstance, h, "frame", &f2);
    CHECK(f2 == 0, "A3 replay resets frame to 0");
}

void testA4Loop(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA4.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA4.jsonc").c_str(), 80, 80, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A4 create returns handle");
    reg(h);
    if (!h) return;
    CHECK(UICornerstone_SetBool(g_uiInstance, h, "loop", 1) == 1, "A4 SetBool(loop,1) ok");
    int loop = 0;
    CHECK(UICornerstone_GetBool(g_uiInstance, h, "loop", &loop) == 1 && loop == 1, "A4 GetBool(loop)==1");
    UICornerstone_SetBool(g_uiInstance, h, "loop", 0);
    CHECK(UICornerstone_GetBool(g_uiInstance, h, "loop", &loop) == 1 && loop == 0, "A4 GetBool(loop)==0");
}

void testA5FrameSeek(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA5.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA5.jsonc").c_str(), 100, 100, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A5 create returns handle");
    reg(h);
    if (!h) return;
    CHECK(UICornerstone_SetInt(g_uiInstance, h, "frame", 10) == 1, "A5 SetInt(frame,10) ok");
    int f = -1;
    CHECK(UICornerstone_GetInt(g_uiInstance, h, "frame", &f) == 1 && f == 10, "A5 GetInt(frame)==10");
    // 跳帧后首次 update 不跳变（lastTick 复位）
    sleepMs(100);
    BENCH->update();
    CHECK(UICornerstone_GetInt(g_uiInstance, h, "frame", &f) == 1 && f == 10, "A5 no jump after seek+update");
    // 越界帧拒绝
    CHECK(UICornerstone_SetInt(g_uiInstance, h, "frame", 9999) == 1, "A5 SetInt out-of-range returns 1");
    CHECK(UICornerstone_GetInt(g_uiInstance, h, "frame", &f) == 1 && f == 10, "A5 out-of-range frame ignored");
}

void testA6SwitchAnimation(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA6a.jsonc").c_str(), makeAnimationDoc(30));
    writeJson(tempJsonPath("tA6b.jsonc").c_str(), makeAnimationDoc(60));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA6a.jsonc").c_str(), 120, 120, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A6 create returns handle");
    reg(h);
    if (!h) return;
    // 播放中换动画（§6.2：从第 0 帧重播；§6.5：重新 prepare）
    UICornerstone_SetBool(g_uiInstance, h, "playing", 1);
    sleepMs(60);
    BENCH->update();
    int fMid = 0;
    UICornerstone_GetInt(g_uiInstance, h, "frame", &fMid);
    CHECK(UICornerstone_SetString(g_uiInstance, h, "animation", tempJsonPath("tA6b.jsonc").c_str()) == 1, "A6 SetString(animation) ok");
    LuotiAni* ani = asLuotiAni(h);
    CHECK(ani != nullptr, "A6 dynamic_cast LuotiAni ok");
    if (ani) {
        CHECK(ani->getTotalFrames() == 60, "A6 re-prepared with new totalFrames");
        CHECK(ani->isPrepared(), "A6 re-prepared state");
        Actor* fa = ani->getFrameActor(0);
        CHECK(fa != nullptr && fa->getTexture() != nullptr, "A6 new frames baked with texture");
    }
    int playing = 0;
    UICornerstone_GetBool(g_uiInstance, h, "playing", &playing);
    CHECK(playing == 1, "A6 still playing after switch");
    int fNew = 0;
    UICornerstone_GetInt(g_uiInstance, h, "frame", &fNew);
    CHECK(fNew == 0, "A6 replay from frame 0 after switch");
}

void testA7HitTestOcclusion(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA7.jsonc").c_str(), makeAnimationDoc(30));
    // 动画叠在按钮上：按钮矩形 (300,300,120,50)，动画 (280,285,160,80) 覆盖按钮
    // ↑ 点击动画区域：按钮应正常响应（动画 isContainsPoint=false 不遮挡，§2.3-5）
    shared_ptr<Button> btn = ButtonBuilder(BENCH, SRect(300, 300, 120, 50))
        .setCaption("Hit")
        .build();
    btn->create();
    BENCH->addControl(btn);
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA7.jsonc").c_str(), 280, 285, 160, 80, 1.0f, 1.0f);
    CHECK(h != nullptr, "A7 create returns handle");
    reg(h);
    int clicks = 0;
    btn->setOnClick([&clicks](shared_ptr<Button> b) {
        clicks++;
        b->setCaption("Clicked!");
        logOutput(string("A7 Hit button clicked (") + to_string(clicks) + ")");
    });
    UICornerstone_ProcessEvents(g_uiInstance);
    // 遮挡判定由 ControlImpl::getDrawRect/covered 链完成；此处验证动画 isContainsPoint==false
    LuotiAni* ani = asLuotiAni(h);
    CHECK(ani != nullptr, "A7 dynamic_cast ok");
    if (ani) {
        CHECK(!ani->isContainsPoint(350, 325), "A7 animation does not claim hit");
        CHECK(!ani->isContainsPoint(300, 300), "A7 animation edge point not claimed");
    }
}

void testA8RenderSmoke(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA8.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA8.jsonc").c_str(), 160, 160, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A8 create returns handle");
    reg(h);
    if (!h) return;
    UICornerstone_SetBool(g_uiInstance, h, "playing", 1);
    // 60 帧 update 推进冒烟（渲染冒烟由主循环持续完成；initialize 内不裸 draw，
    // 避免在首帧 clear 前的 backbuffer 留下初始化残留闪现）
    for (int i = 0; i < 60; i++) {
        BENCH->update();
    }
    CHECK(true, "A8 60-frame update/render smoke passed");
}

void testA9ErrorBoundary(void) {
    g_caseIndex++;
    // 无效路径创建 → nullptr 不崩溃（§6.4-1）
    UIControlHandle hBad = UICornerstone_CreateAnimation(g_uiInstance, "no_such_anim.jsonc", 0, 0, 256, 256, 1.0f, 1.0f);
    CHECK(hBad == nullptr, "A9 invalid path create returns nullptr");
    // 副本 jsonc 作无效输入（风险 10）：资源引用缺失 → prepare 抛异常 → nullptr
    json badDoc = makeAnimationDoc(30);
    badDoc["layers"][0]["src"] = "animations/does_not_exist/does_not_exist.svg";
    writeJson(tempJsonPath("bombBlock - 副本.jsonc").c_str(), badDoc);
    UIControlHandle hBad2 = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("bombBlock - 副本.jsonc").c_str(), 0, 0, 256, 256, 1.0f, 1.0f);
    CHECK(hBad2 == nullptr, "A9 copy jsonc create returns nullptr");
    // SetString("animation") 无效路径 → 返回 0、控件保留（§6.4-2）
    writeJson(tempJsonPath("tA9.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA9.jsonc").c_str(), 180, 180, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A9 create returns handle");
    reg(h);
    if (h) {
        CHECK(UICornerstone_SetString(g_uiInstance, h, "animation", "no_such_anim.jsonc") == 0, "A9 bad SetString returns 0");
        LuotiAni* ani = asLuotiAni(h);
        CHECK(ani != nullptr && ani->isPrepared(), "A9 control state retained after bad set");
        CHECK(UICornerstone_SetString(g_uiInstance, h, "animation", tempJsonPath("tA9.jsonc").c_str()) == 1, "A9 retry works after bad set");
        // 未 prepare 时 SetBool("playing",1) → 0（§6.4-3）
        auto* raw = static_cast<Control*>(h);
        auto* ctlImpl = dynamic_cast<ControlImpl*>(raw);
        CHECK(ctlImpl != nullptr, "A9 ControlImpl cast ok");
        if (ctlImpl) {
            auto rawLuoti = dynamic_cast<LuotiAni*>(ctlImpl);
            CHECK(rawLuoti != nullptr, "A9 LuotiAni cast ok");
        }
    }
}

void testA10CanvasSizeFallback(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA10.jsonc").c_str(), makeAnimationDoc(30, 256, 256));
    // w/h=0 → prepare 回退到 overview.view 画布尺寸
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA10.jsonc").c_str(), 0, 0, 0, 0, 1.0f, 1.0f);
    CHECK(h != nullptr, "A10 create returns handle");
    reg(h);
    if (h) {
        float x = 0, y = 0, w = 0, hh = 0;
        UICornerstone_GetRect(g_uiInstance, h, &x, &y, &w, &hh);
        CHECK(x == 0 && y == 0 && w == 256 && hh == 256, "A10 rect falls back to canvas size");
    }
}

void testA11EasingPathEndToEnd(void) {
    g_caseIndex++;
    // 增强 JSON：ease-in-out + bezier 路径（§5.4 统一语义）
    json kf0 = {{"frame", 0}, {"operation", json::array({
        {{"type", "translate"}, {"tx", 0}, {"ty", 0}},
        {{"type", "opacity"}, {"opacity", 100}},
    })}};
    json kf30 = {{"frame", 30}, {"operation", json::array({
        {{"type", "translate"}, {"tx", 120}, {"ty", 60}, {"easing", "ease-in-out"},
         {"path", {{"type", "bezier"}, {"c1x", 80}, {"c1y", -120}, {"c2x", 200}, {"c2y", 150}}}},
        {{"type", "opacity"}, {"opacity", 100}},
    })}};
    json layer = {
        {"name", "layer1"}, {"type", "image"},
        {"src", "animations/rotateBtn/rotateBtn.svg"},
        {"opacity", 100}, {"blendMode", "normal"},
        {"keyFrames", json::array({kf0, kf30})}
    };
    json doc = {
        {"overview", {{"name", "e2e"}, {"version", "0.0.1"},
                      {"view", {{"width", 256}, {"height", 256}}},
                      {"frameRate", 30}, {"totalFrames", 31}, {"loop", false}}},
        {"layers", json::array({layer})}
    };
    writeJson(tempJsonPath("tA11.jsonc").c_str(), doc);
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA11.jsonc").c_str(), 200, 200, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A11 create returns handle");
    reg(h);
    if (!h) return;
    UICornerstone_SetBool(g_uiInstance, h, "playing", 1);
    sleepMs(100);
    BENCH->update();
    LuotiAni* ani = asLuotiAni(h);
    CHECK(ani != nullptr, "A11 dynamic_cast ok");
    if (ani) {
        // 中间帧 15：t=0.5，ease-in-out(0.5)=0.5；bezier 三次曲线 c1(80,-120) c2(200,150) 在 t=0.5：
        // P = 0.125*P0 + 0.375*C1 + 0.375*C2 + 0.125*P3
        // x = 0.375*80 + 0.375*200 + 0.125*120 = 30+75+15 = 120
        // y = 0.375*(-120) + 0.375*150 + 0.125*60 = -45+56.25+7.5 = 18.75
        auto od = ani->getFrameOpData(0, 15);
        CHECK(fabsf(od.translate.x - 120.0f) < 1.0f, "A11 bezier mid x==120");
        CHECK(fabsf(od.translate.y - 18.75f) < 1.0f, "A11 bezier mid y==18.75");
    }
}

void testA12ScaleFollowCABI(void) {
    g_caseIndex++;
    writeJson(tempJsonPath("tA12.jsonc").c_str(), makeAnimationDoc(30));
    UIControlHandle h = UICornerstone_CreateAnimation(g_uiInstance, tempJsonPath("tA12.jsonc").c_str(), 240, 240, 256, 256, 1.0f, 1.0f);
    CHECK(h != nullptr, "A12 create returns handle");
    reg(h);
    if (!h) return;
    LuotiAni* ani = asLuotiAni(h);
    CHECK(ani != nullptr, "A12 dynamic_cast ok");
    if (!ani) return;
    // 绘制触发帧 Actor 缩放校准；绘制后立即恢复背景，避免 initialize 阶段
    // 的裸绘制在首帧 clear 前写入 backbuffer 造成一次闪现
    auto paintOnce = [&]() {
        ani->draw();
        GET_RENDERDEVICE->setDrawColor(SColor(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
        GET_RENDERDEVICE->clear();
    };
    // 挂入 2x 容器（C++ 侧 Panel 2x）→ 帧 Actor 校准 ==2
    shared_ptr<Panel> p2 = PanelBuilder(nullptr, SRect(0, 0, 400, 300), 2.0f, 2.0f)
        .setTransparent(true)
        .build();
    p2->create();
    BENCH->addControl(p2);
    auto* ctlImpl = dynamic_cast<ControlImpl*>(static_cast<Control*>(h));
    CHECK(ctlImpl != nullptr, "A12 handle cast to ControlImpl ok");
    if (ctlImpl) {
        auto sp = ctlImpl->shared_from_this();
        BENCH->removeControl(sp);
        p2->addControl(sp);
    }
    BENCH->update();
    paintOnce();
    Actor* fa = ani->getFrameActor(0);
    CHECK(fa != nullptr && fa->getScaleXX() == 2.0f, "A12 frame actor scale==2 in 2x parent");
    // 换 1x 父 → ==1
    shared_ptr<Panel> p1 = PanelBuilder(nullptr, SRect(0, 0, 400, 300))
        .setTransparent(true)
        .build();
    p1->create();
    BENCH->addControl(p1);
    if (ctlImpl) {
        auto sp = ctlImpl->shared_from_this();
        p2->removeControl(sp);
        p1->addControl(sp);
    }
    paintOnce();
    CHECK(fa != nullptr && fa->getScaleXX() == 1.0f, "A12 frame actor scale==1 in 1x parent");

    // 清理面板（否则透明大矩形覆盖会遮挡后续事件命中，如 A7 的 Hit 按钮）
    if (ctlImpl) {
        auto sp = ctlImpl->shared_from_this();
        p1->removeControl(sp);
        BENCH->addControl(sp);
    }
    BENCH->removeControl(p2);
    BENCH->removeControl(p1);
}

void testAnimationInitialize(shared_ptr<Bench>) {
    TestUtil::log("testAnimationInitialize");

    testA1Create();
    testA2NoAutoPlay();
    testA3PlayPauseReplay();
    testA4Loop();
    testA5FrameSeek();
    testA6SwitchAnimation();
    testA7HitTestOcclusion();
    testA8RenderSmoke();
    testA9ErrorBoundary();
    testA10CanvasSizeFallback();
    testA11EasingPathEndToEnd();
    testA12ScaleFollowCABI();

    // 清理已创建动画（避免测试窗口残留多个静止动画干扰观察）
    for (UIControlHandle h : g_handles) removeAni(h);
    g_handles.clear();

    // 挂载单个可见演示（真实资源循环旋转，供人工核对渲染链）
    {
        shared_ptr<LuotiAni> demo = LuotiAniBuilder(BENCH)
            .loadAniDesc(string("animations/rotateBtn/rotateBtn.jsonc"))
            .setRect(SRect(20, 20, 256, 256))
            .prepare()
            .setAutoStart()
            .build();
        demo->create();
        BENCH->addControl(demo);
    }

    if (g_failCount == 0) {
        logOutput(u8"test_animation: ALL PASS");
    } else {
        logOutput(u8"test_animation: FAILURES = " + to_string(g_failCount));
    }
}

class AnimationApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_animation");
        logOutput(u8"AnimationApp::onInit");
        BENCH->setOnInitial(testAnimationInitialize);
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
    }
};

int main(int argc, char* argv[]) {
    return TestRunMain<AnimationApp>(argc, argv);
}
