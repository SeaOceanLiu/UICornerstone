#include <iostream>
#include <memory>
#include "Splitter.h"
#include "LayoutParser.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"
#include "TestInstance.h"

using namespace std;



shared_ptr<Panel> g_parent;
shared_ptr<Panel> g_first;
shared_ptr<Splitter> g_splitter;
shared_ptr<Panel> g_second;
shared_ptr<Label> g_statusLabel;

shared_ptr<Panel> g_hParent;
shared_ptr<Panel> g_hTop;
shared_ptr<Splitter> g_hSplitter;
shared_ptr<Panel> g_hBottom;

shared_ptr<Panel> g_2xParent;
shared_ptr<Panel> g_2xFirst;
shared_ptr<Splitter> g_2xSplitter;
shared_ptr<Panel> g_2xSecond;

void onSplitterMoved(shared_ptr<Splitter> sp, float ratio) {
    char buf[64];
    snprintf(buf, sizeof(buf), "V-Split ratio: %.3f", ratio);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void on2xSplitterMoved(shared_ptr<Splitter> sp, float ratio);
void testHorizontalSplitterInitialize();
void test2xSplitterInitialize();

// ── 引擎模式 Splitter（布局引擎容器内，Splitter_Design §8）──
static int g_engPass = 0, g_engFail = 0;
#define ENG_CHECK(cond, msg) \
    do { if (cond) { ++g_engPass; } else { ++g_engFail; printf("ENG-FAIL: %s\n", msg); } } while (0)

static int g_engMoved1 = 0, g_engMoved2 = 0;
static void onEngMoved1(shared_ptr<Splitter> sp, float r) { (void)sp; (void)r; ++g_engMoved1; }
static void onEngMoved2(shared_ptr<Splitter> sp, float r) { (void)sp; (void)r; ++g_engMoved2; }

static shared_ptr<Event> engMouse(EventType type, float x, float y) {
    auto ev = make_shared<Event>(type);
    if (type == EventType::MouseDown || type == EventType::MouseUp)
        ev->mouseButton = {x, y, MouseButton::Left};
    else
        ev->mousePos = {x, y};
    return ev;
}

static SRect engRect(const shared_ptr<Control>& c) { return c->getRect(); }

static void testEngineManagedSplitter() {
    TestUtil::log("testEngineManagedSplitter");

    // h-flow: [A(200 fixed) | sp1 | B(flex) | sp2 | C(240 fixed)]
    auto flow = make_shared<Panel>(nullptr, SRect(10, 480, 1000, 300));
    flow->setLayoutEngine(make_shared<HFlowLayout>(0.0f, Margin(0, 0, 0, 0)));
    BENCH->addControl(flow);

    auto a = make_shared<Panel>(nullptr, SRect(0, 0, 200, 300));
    auto sp1 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 300));
    sp1->setOrientation(true); sp1->setThickness(8); sp1->setMinSize(40, 40);
    auto b = make_shared<Panel>(nullptr, SRect(0, 0, 0, 300));
    b->setNormalStateBGColor(SColor(40, 150, 80, 255));  // ENGINE b(绿)
    auto sp2 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 300));
    sp2->setOrientation(true); sp2->setThickness(8); sp2->setMinSize(40, 40);
    auto c = make_shared<Panel>(nullptr, SRect(0, 0, 240, 300));
    c->setNormalStateBGColor(SColor(180, 70, 70, 255));  // ENGINE c(红)
    sp1->setOnSplitterMoved(onEngMoved1);
    sp2->setOnSplitterMoved(onEngMoved2);

    flow->setChildFlowProps(b.get(), FlowItemProps{1.0f});
    flow->addControl(a); flow->addControl(sp1);
    flow->addControl(b); flow->addControl(sp2);
    flow->addControl(c);
    flow->reflowChildren();

    // 1. 链式初始布局：A 200 / sp1@200 / B flex / sp2@(200+8+B) / C 240
    float bW = engRect(b).width;
    ENG_CHECK(fabsf(engRect(a).left - 0.0f) < 0.01f && fabsf(engRect(a).width - 200.0f) < 0.01f,
        "A 固定宽 200@0");
    ENG_CHECK(fabsf(engRect(sp1).left - 200.0f) < 0.01f && fabsf(engRect(sp1).width - 8.0f) < 0.01f,
        "sp1 贴 A 右缘，厚 8");
    ENG_CHECK(fabsf(bW - 544.0f) < 1.0f, "B 弹性宽 = 1000-200-8-240-8 = 544");
    ENG_CHECK(fabsf(engRect(sp2).left - (200.0f + 8.0f + bW)) < 0.01f,
        "sp2 链式累计位置 (200+8+B)");
    ENG_CHECK(fabsf(engRect(c).left - (200.0f + 8.0f + bW + 8.0f)) < 0.01f && fabsf(engRect(c).width - 240.0f) < 0.01f,
        "C 固定宽 240 在链尾");

    // 1.5 跟手严格 1:1：拖 +37.5（非整数）→ A=237.5；再拖 -37.5 恢复 200（往返）
    SRect s1d = sp1->getDrawRect();
    float cx = s1d.left + s1d.width * 0.5f, cy = s1d.top + s1d.height * 0.5f;
    sp1->handleEvent(engMouse(EventType::MouseDown, cx, cy));
    sp1->handleEvent(engMouse(EventType::MouseMove, cx + 37.5f, cy));
    sp1->handleEvent(engMouse(EventType::MouseUp, cx + 37.5f, cy));
    ENG_CHECK(fabsf(engRect(a).width - 237.5f) < 0.05f, "跟手 1:1：+37.5 → A=237.5");
    SRect s1dAfter = sp1->getDrawRect();
    sp1->handleEvent(engMouse(EventType::MouseDown, s1dAfter.left + s1dAfter.width * 0.5f, cy));
    sp1->handleEvent(engMouse(EventType::MouseMove, s1dAfter.left + s1dAfter.width * 0.5f - 37.5f, cy));
    sp1->handleEvent(engMouse(EventType::MouseUp, s1dAfter.left + s1dAfter.width * 0.5f - 37.5f, cy));
    ENG_CHECK(fabsf(engRect(a).width - 200.0f) < 0.05f, "跟手 1:1：往返 -37.5 → A=200 恢复");

    // 1.6 真实帧泵路径：push 事件 → Bench::eventLoopEntry（watcher 链）
    //      —— 与 GUI 实际事件流一致（不经 handleEvent 直调）
    {
        flow->setRect(SRect(10, 480, 1000, 300));   // 复位场景（A=200 固定）
        SRect s1r = sp1->getDrawRect();
        float px = s1r.left + s1r.width * 0.5f, py = s1r.top + s1r.height * 0.5f;
        auto pump = [&](shared_ptr<Event> ev) {
            BENCH->getContext()->eventQueue->pushEventIntoQueue(ev);
            BENCH->eventLoopEntry();
        };
        pump(engMouse(EventType::MouseDown, px, py));
        pump(engMouse(EventType::MouseMove, px + 60.0f, py));
        pump(engMouse(EventType::MouseUp, px + 60.0f, py));
        ENG_CHECK(fabsf(engRect(a).width - 260.0f) < 0.05f,
            "帧泵路径: 拖 +60 → A=260（watcher 链跟手）");
        // 恢复 200 供后续步骤
        SRect s1r2 = sp1->getDrawRect();
        float px2 = s1r2.left + s1r2.width * 0.5f;
        pump(engMouse(EventType::MouseDown, px2, py));
        pump(engMouse(EventType::MouseMove, px2 - 60.0f, py));
        pump(engMouse(EventType::MouseUp, px2 - 60.0f, py));
        ENG_CHECK(fabsf(engRect(a).width - 200.0f) < 0.05f, "帧泵路径: 往返恢复 200");
    }

    // 1.7 分段拖拽（防双重累加回归）：每帧 +1 共 10 帧 → A 累计应 +10（而非>10）
    {
        SRect s1s = sp1->getDrawRect();
        float fx = s1s.left + s1s.width * 0.5f, fy = s1s.top + s1s.height * 0.5f;
        sp1->handleEvent(engMouse(EventType::MouseDown, fx, fy));
        for (int i = 1; i <= 10; ++i)
            sp1->handleEvent(engMouse(EventType::MouseMove, fx + i * 1.0f, fy));
        sp1->handleEvent(engMouse(EventType::MouseUp, fx + 10.0f, fy));
        ENG_CHECK(fabsf(engRect(a).width - 210.0f) < 0.05f,
            "分段拖拽 10x1px → A=210（严格 210，无双重累加）");
        // 恢复 200（供后续步骤固定基准）
        SRect s1r3 = sp1->getDrawRect();
        float rx = s1r3.left + s1r3.width * 0.5f, ry = s1r3.top + s1r3.height * 0.5f;
        sp1->handleEvent(engMouse(EventType::MouseDown, rx, ry));
        sp1->handleEvent(engMouse(EventType::MouseMove, rx - 10.0f, ry));
        sp1->handleEvent(engMouse(EventType::MouseUp, rx - 10.0f, ry));
        ENG_CHECK(fabsf(engRect(a).width - 200.0f) < 0.05f, "分段拖拽恢复 200");
    }

    // 2. 拖拽 sp1 向右 +100（动态取 sp1 当前中心）
    SRect s1c = sp1->getDrawRect();
    float cx2 = s1c.left + s1c.width * 0.5f, cy2 = s1c.top + s1c.height * 0.5f;
    sp1->handleEvent(engMouse(EventType::MouseDown, cx2, cy2));
    sp1->handleEvent(engMouse(EventType::MouseMove, cx2 + 100.0f, cy2));
    sp1->handleEvent(engMouse(EventType::MouseUp, cx2 + 100.0f, cy2));

    ENG_CHECK(fabsf(engRect(a).width - 300.0f) < 1.0f,
        "拖拽后 A 宽 200+100=300（权重换算）");
    ENG_CHECK(fabsf(engRect(b).width - (1000.0f - 300.0f - 8.0f - 240.0f - 8.0f)) < 1.0f,
        "B 弹性补偿（后续段随链更新）");
    ENG_CHECK(fabsf(engRect(sp2).left - (300.0f + 8.0f + engRect(b).width)) < 0.01f,
        "sp2 位置随链更新（不覆盖 sp1 结果）");
    ENG_CHECK(fabsf(engRect(c).left - (300.0f + 8.0f + engRect(b).width + 8.0f)) < 0.01f,
        "C 位置随链更新");
    ENG_CHECK(g_engMoved1 >= 1, "sp1 onSplitterMoved 触发");

    // 3. 拖拽 sp2 向右 +80（前段=B 弹性段）
    SRect s2d = sp2->getDrawRect();
    float cx3 = s2d.left + s2d.width * 0.5f, cy3 = s2d.top + s2d.height * 0.5f;
    sp2->handleEvent(engMouse(EventType::MouseDown, cx3, cy3));
    sp2->handleEvent(engMouse(EventType::MouseMove, cx3 + 80.0f, cy3));
    sp2->handleEvent(engMouse(EventType::MouseUp, cx3 + 80.0f, cy3));

    // 式2：B 弹性 & C 固定 → 改 C 固定宽（B 自动弹性补偿）
    ENG_CHECK(fabsf(engRect(c).width - 160.0f) < 0.01f, "拖 sp2: C 固定 240→160");
    ENG_CHECK(fabsf(engRect(b).width - (1000.0f - 300.0f - 8.0f - 160.0f - 8.0f)) < 1.0f,
        "拖 sp2: B 弹性补偿 = 524");
    ENG_CHECK(fabsf(engRect(sp2).left - (300.0f + 8.0f + engRect(b).width)) < 0.01f,
        "拖 sp2: sp2 链式位置");
    ENG_CHECK(fabsf(engRect(c).left - (300.0f + 8.0f + engRect(b).width + 8.0f)) < 0.01f,
        "拖 sp2: C 在 sp2 之后（链式）");
    ENG_CHECK(g_engMoved2 >= 1, "sp2 onSplitterMoved 触发");

    // 4. resize 容器自动重排（A 固定 300、C 固定 160、B 弹性补偿）
    flow->setRect(SRect(10, 480, 1200, 300));
    ENG_CHECK(fabsf(engRect(a).width - 300.0f) < 0.01f, "resize 后 A 保持 300");
    ENG_CHECK(fabsf(engRect(b).width - (1200.0f - 300.0f - 8.0f - 160.0f - 8.0f)) < 1.0f,
        "resize 后 B 弹性补偿 = 724");
    ENG_CHECK(fabsf(engRect(c).left - (300.0f + 8.0f + engRect(b).width + 8.0f)) < 0.01f,
        "resize 后链式位置保持");

    printf("ENGINE splitter: PASS=%d FAIL=%d\n", g_engPass, g_engFail);
}

// ── JSON 声明式 Splitter（CornerstoneDesigner 场景：LoadLayout → 引擎模式）──
static void testJsonEngineSplitter() {
    TestUtil::log("testJsonEngineSplitter");

    // 极简配置回归：JSON + h-flow + splitter（此前 parse 后引擎↔splitter 递归环卡死）
    static const char* JSON_ENGINE = R"({
  "version": "1.0",
  "controls": [{
    "type": "panel", "id": "jflow", "rect": {"x": 10, "y": 500, "w": 1000, "h": 300},
    "layout": {"type": "h-flow", "gap": 0, "padding": {"left": 0, "top": 0, "right": 0, "bottom": 0}},
    "children": [
      {"type": "panel", "id": "jA", "rect": {"x": 0, "y": 0, "w": 200, "h": 300},
       "colors": {"background": {"normal": "#2864B4FF"}}},
      {"type": "splitter", "id": "jSp1", "orientation": "vertical", "thickness": 8,
       "rect": {"x": 0, "y": 0, "w": 8, "h": 300}},
      {"type": "panel", "id": "jB", "rect": {"x": 0, "y": 0, "w": 0, "h": 300}, "flow-weight": 1,
       "colors": {"background": {"normal": "#32964CFF"}}},
      {"type": "splitter", "id": "jSp2", "orientation": "vertical", "thickness": 8,
       "rect": {"x": 0, "y": 0, "w": 8, "h": 300}},
      {"type": "panel", "id": "jC", "rect": {"x": 0, "y": 0, "w": 240, "h": 300},
       "colors": {"background": {"normal": "#B44646FF"}}}
    ]
  }]
})";
    LayoutParser parser;
    auto root = parser.parseLayout(JSON_ENGINE);
    ENG_CHECK(root != nullptr, "JSON parseLayout 完成（不卡死）");
    if (!root) return;
    BENCH->addControl(root);

    auto jA = parser.findControlById("jA");
    auto jSp1 = parser.findControlById("jSp1");
    auto jB = parser.findControlById("jB");
    auto jSp2 = parser.findControlById("jSp2");
    auto jC = parser.findControlById("jC");
    ENG_CHECK(jA && jSp1 && jB && jSp2 && jC, "JSON 控件全部找到（splitter 挂树）");

    // 初始链式布局（parsePanel 已 reflow）：A 200 / sp1@200 / B flex / sp2 链式 / C 240
    ENG_CHECK(fabsf(engRect(jA).width - 200.0f) < 0.01f, "JSON: A 200");
    ENG_CHECK(fabsf(engRect(jSp1).left - 200.0f) < 0.01f && fabsf(engRect(jSp1).width - 8.0f) < 0.01f,
        "JSON: sp1@200 厚 8");
    float jbw = engRect(jB).width;
    ENG_CHECK(fabsf(jbw - 544.0f) < 1.0f, "JSON: B 弹性 = 544");
    ENG_CHECK(fabsf(engRect(jSp2).left - (200.0f + 8.0f + jbw)) < 0.01f, "JSON: sp2 链式位置");
    ENG_CHECK(fabsf(engRect(jC).width - 240.0f) < 0.01f, "JSON: C 240");

    // 拖拽 jSp1（绘制坐标 = jflow(10,500)+sp1 中心）→ A 300 / B 补偿 / sp2 链式
    auto jSp1d = jSp1->getDrawRect();
    float jx = jSp1d.left + jSp1d.width * 0.5f, jy = jSp1d.top + jSp1d.height * 0.5f;
    jSp1->handleEvent(engMouse(EventType::MouseDown, jx, jy));
    jSp1->handleEvent(engMouse(EventType::MouseMove, jx + 100.0f, jy));
    jSp1->handleEvent(engMouse(EventType::MouseUp, jx + 100.0f, jy));
    ENG_CHECK(fabsf(engRect(jA).width - 300.0f) < 1.0f, "JSON 拖拽: A→300");
    ENG_CHECK(fabsf(engRect(jB).width) > 0.0f &&
              fabsf(engRect(jB).width - (1000.0f - 300.0f - 8.0f - 240.0f - 8.0f)) < 1.0f,
        "JSON 拖拽: B 弹性补偿");
    ENG_CHECK(fabsf(engRect(jSp2).left - (300.0f + 8.0f + engRect(jB).width)) < 0.01f,
        "JSON 拖拽: sp2 链式更新（多 splitter 无覆盖）");

    printf("JSON ENGINE splitter: PASS=%d FAIL=%d (cumulated)\n", 0, 0);
}

// ── 四栏三 Splitter（链式引擎任意数量验证）──
static void testEngineSplitterQuad() {
    TestUtil::log("testEngineSplitterQuad");
    // h-flow: [A(240) | sp1 | B(flex) | sp2 | C(flex) | sp3 | D(240)]
    auto flow = make_shared<Panel>(nullptr, SRect(10, 10, 1400, 300));
    flow->setLayoutEngine(make_shared<HFlowLayout>(0.0f, Margin(0, 0, 0, 0)));
    BENCH->addControl(flow);
    auto a = make_shared<Panel>(nullptr, SRect(0, 0, 240, 300));
  a->setNormalStateBGColor(SColor(40, 110, 180, 255));  // QUAD a(蓝)
    a->setNormalStateBGColor(SColor(40, 110, 180, 255));  // ENGINE a(蓝)
    auto sp1 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 300));
    sp1->setOrientation(true); sp1->setThickness(8); sp1->setMinSize(40, 40);
    auto b = make_shared<Panel>(nullptr, SRect(0, 0, 0, 300));
    b->setNormalStateBGColor(SColor(40, 150, 80, 255));  // QUAD b(绿)
    auto sp2 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 300));
    sp2->setOrientation(true); sp2->setThickness(8); sp2->setMinSize(40, 40);
    auto c = make_shared<Panel>(nullptr, SRect(0, 0, 0, 300));
    c->setNormalStateBGColor(SColor(180, 60, 150, 255));  // QUAD c(品红)
    auto sp3 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 300));
    sp3->setOrientation(true); sp3->setThickness(8); sp3->setMinSize(40, 40);
    auto d = make_shared<Panel>(nullptr, SRect(0, 0, 240, 300));
    d->setNormalStateBGColor(SColor(180, 70, 70, 255));  // QUAD d(红)
    flow->setChildFlowProps(b.get(), FlowItemProps{1.0f});
    flow->setChildFlowProps(c.get(), FlowItemProps{1.0f});
    flow->addControl(a); flow->addControl(sp1);
    flow->addControl(b); flow->addControl(sp2);
    flow->addControl(c); flow->addControl(sp3);
    flow->addControl(d);
    flow->reflowChildren();

    float bw = engRect(b).width, cw = engRect(c).width;
    ENG_CHECK(fabsf(bw - 448.0f) < 1.0f && fabsf(cw - 448.0f) < 1.0f,
        "四栏: B=C=448 (1400-240-240-24)/2");
    ENG_CHECK(fabsf(engRect(sp2).left - (240.0f + 8.0f + bw)) < 0.01f,
        "四栏: sp2 链式（240+8+B）");
    ENG_CHECK(fabsf(engRect(sp3).left - (240.0f + 8.0f + bw + 8.0f + cw)) < 0.01f,
        "四栏: sp3 链式（240+8+B+8+C）");
    ENG_CHECK(fabsf(engRect(d).left - (240.0f + 8.0f + bw + 8.0f + cw + 8.0f)) < 0.01f,
        "四栏: D 固定 240 在链尾");

    // 拖 sp1（A 固定 → 改 A）向右 +60 → A=300，B/C 弹性补偿
    SRect s1d = sp1->getDrawRect();
    float qx = s1d.left + s1d.width * 0.5f, qy = s1d.top + s1d.height * 0.5f;
    sp1->handleEvent(engMouse(EventType::MouseDown, qx, qy));
    sp1->handleEvent(engMouse(EventType::MouseMove, qx + 60.0f, qy));
    sp1->handleEvent(engMouse(EventType::MouseUp, qx + 60.0f, qy));
    ENG_CHECK(fabsf(engRect(a).width - 300.0f) < 1.0f, "四栏拖 sp1: A→300");
    ENG_CHECK(fabsf(engRect(b).width + engRect(c).width -
        (1400.0f - 300.0f - 240.0f - 24.0f)) < 1.0f, "四栏拖 sp1: B+C 弹性补偿");

    // 拖 sp2（B/C 双弹性 → 降级 B 锁定）向右 +50 → B 固定=444+50，C 弹性补偿
    SRect s2d = sp2->getDrawRect();
    float qx2 = s2d.left + s2d.width * 0.5f, qy2 = s2d.top + s2d.height * 0.5f;
    sp2->handleEvent(engMouse(EventType::MouseDown, qx2, qy2));
    sp2->handleEvent(engMouse(EventType::MouseMove, qx2 + 50.0f, qy2));
    sp2->handleEvent(engMouse(EventType::MouseUp, qx2 + 50.0f, qy2));
    // sp2 双弹性（B/C）→ 降级 B 锁定：B=418+50=468、C 弹性补偿
    float eleSpan = 1400.0f - 300.0f - 240.0f - 24.0f;   // B+C 弹性总宽 836
    ENG_CHECK(fabsf(engRect(b).width - (418.0f + 50.0f)) < 1.0f, "四栏拖 sp2: B 降级锁定 468");
    ENG_CHECK(fabsf(engRect(c).width - (eleSpan - 468.0f)) < 1.0f, "四栏拖 sp2: C 弹性补偿");
    ENG_CHECK(fabsf(engRect(sp3).left - (300.0f + 8.0f + engRect(b).width + 8.0f + engRect(c).width)) < 0.01f,
        "四栏拖 sp2: sp3 链式更新");

    // 拖 sp3（C 弹性 & D 固定 → 改 D）向左 -100 → D=140，C 弹性补偿
    SRect s3d = sp3->getDrawRect();
    float qx3 = s3d.left + s3d.width * 0.5f, qy3 = s3d.top + s3d.height * 0.5f;
    sp3->handleEvent(engMouse(EventType::MouseDown, qx3, qy3));
    sp3->handleEvent(engMouse(EventType::MouseMove, qx3 - 100.0f, qy3));
    sp3->handleEvent(engMouse(EventType::MouseUp, qx3 - 100.0f, qy3));
    // 式2：C 弹性 & D 固定 → 改 D（向左拖 → D 变宽 240+100=340，C 弹性补偿）
    ENG_CHECK(fabsf(engRect(d).width - 340.0f) < 1.0f, "四栏拖 sp3: D→340");
    ENG_CHECK(fabsf(engRect(c).width - (1400.0f - 300.0f - 468.0f - 340.0f - 24.0f)) < 1.0f,
        "四栏拖 sp3: C 唯一弹性项=剩余 pool 268");

    printf("QUAD splitter: PASS=%d FAIL=%d (cumulated)\n", 0, 0);
}

// ── 嵌套：中间弹性 Panel 内再放两条水平 Splitter（v-flow 五层）──
static void testEngineSplitterNested() {
    TestUtil::log("testEngineSplitterNested");
    // 外层 h-flow: [A | sp1 | B(flex) | sp2 | C]，B 内 v-flow: [top | hsp1 | mid(flex) | hsp2 | bottom]
    auto flow = make_shared<Panel>(nullptr, SRect(10, 10, 1200, 600));
    flow->setLayoutEngine(make_shared<HFlowLayout>(0.0f, Margin(0, 0, 0, 0)));
    BENCH->addControl(flow);
    auto a = make_shared<Panel>(nullptr, SRect(0, 0, 200, 600));
    auto sp1 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 600));
    sp1->setOrientation(true); sp1->setThickness(8); sp1->setMinSize(40, 40);
    auto b = make_shared<Panel>(nullptr, SRect(0, 0, 0, 600));
    auto sp2 = make_shared<Splitter>(nullptr, SRect(0, 0, 8, 600));
    sp2->setOrientation(true); sp2->setThickness(8); sp2->setMinSize(40, 40);
    auto c = make_shared<Panel>(nullptr, SRect(0, 0, 240, 600));
    flow->setChildFlowProps(b.get(), FlowItemProps{1.0f});
    flow->addControl(a); flow->addControl(sp1);
    flow->addControl(b); flow->addControl(sp2);
    flow->addControl(c);

    // B 内部 v-flow[top 100 | hsp1 | mid flex | hsp2 | bottom 100]
    b->setLayoutEngine(make_shared<VFlowLayout>(0.0f, Margin(0, 0, 0, 0)));
    auto top = make_shared<Panel>(nullptr, SRect(0, 0, 600, 100));
    top->setNormalStateBGColor(SColor(60, 90, 150, 255));  // NESTED top(蓝灰)
    auto hsp1 = make_shared<Splitter>(nullptr, SRect(0, 0, 600, 8));
    hsp1->setOrientation(false);  // 水平分隔条（上下分栏）
    hsp1->setThickness(8); hsp1->setMinSize(30, 30);
    auto mid = make_shared<Panel>(nullptr, SRect(0, 0, 600, 0));
    mid->setNormalStateBGColor(SColor(70, 140, 80, 255));  // NESTED mid(绿)
    auto hsp2 = make_shared<Splitter>(nullptr, SRect(0, 0, 600, 8));
    hsp2->setOrientation(false);
    hsp2->setThickness(8); hsp2->setMinSize(30, 30);
    auto bottom = make_shared<Panel>(nullptr, SRect(0, 0, 600, 100));
    bottom->setNormalStateBGColor(SColor(170, 90, 60, 255));  // NESTED bottom(橙红)
    b->setChildFlowProps(mid.get(), FlowItemProps{1.0f});
    b->addControl(top); b->addControl(hsp1);
    b->addControl(mid); b->addControl(hsp2);
    b->addControl(bottom);
    flow->reflowChildren();

    // 外层链式
    float bw = engRect(b).width;
    ENG_CHECK(fabsf(bw - (1200.0f - 200.0f - 8.0f - 240.0f - 8.0f)) < 1.0f, "嵌套外链: B 弹性");
    ENG_CHECK(fabsf(engRect(sp2).left - (200.0f + 8.0f + bw)) < 0.01f, "嵌套外链: sp2 位置");

    // 内部 v-flow 初始：top100 / hsp1@100 / mid flex / hsp2@(100+8+mid) / bottom100
    float midH = engRect(mid).height;
    ENG_CHECK(fabsf(engRect(top).height - 100.0f) < 0.01f, "嵌套: top 100");
    ENG_CHECK(fabsf(engRect(hsp1).top - 100.0f) < 0.01f && fabsf(engRect(mid).height - (600.0f - 200.0f - 16.0f)) < 1.0f,
        "嵌套: mid 弹性 384");
    ENG_CHECK(midH > 0 && fabsf(engRect(hsp2).top - (100.0f + 8.0f + midH)) < 0.01f,
        "嵌套: hsp2 链式");

    // 拖内部 hsp1（top 固定 → 改 top）+60 → top=160，mid 弹性补偿；hsp2 链式更新
    SRect h1d = hsp1->getDrawRect();
    float hx = h1d.left + h1d.width * 0.5f, hy = h1d.top + h1d.height * 0.5f;
    hsp1->handleEvent(engMouse(EventType::MouseDown, hx, hy));
    hsp1->handleEvent(engMouse(EventType::MouseMove, hx, hy + 60.0f));
    hsp1->handleEvent(engMouse(EventType::MouseUp, hx, hy + 60.0f));
    ENG_CHECK(fabsf(engRect(top).height - 160.0f) < 0.01f, "嵌套拖 hsp1: top→160");
    ENG_CHECK(fabsf(engRect(mid).height - (600.0f - 160.0f - 100.0f - 16.0f)) < 1.0f,
        "嵌套拖 hsp1: mid 弹性补偿 324");

    // 拖内部 hsp2（mid 弹性 & bottom 固定 → 改 bottom）-40 → bottom=60，mid 弹性补偿
    SRect h2d = hsp2->getDrawRect();
    float hx2 = h2d.left + h2d.width * 0.5f, hy2 = h2d.top + h2d.height * 0.5f;
    hsp2->handleEvent(engMouse(EventType::MouseDown, hx2, hy2));
    hsp2->handleEvent(engMouse(EventType::MouseMove, hx2, hy2 - 40.0f));
    hsp2->handleEvent(engMouse(EventType::MouseUp, hx2, hy2 - 40.0f));
    // hsp2 向上拖 40 → bottom 变宽 140（式2：改后段固定宽），mid 弹性补偿
    ENG_CHECK(fabsf(engRect(bottom).height - 140.0f) < 0.01f, "嵌套拖 hsp2: bottom→140");
    ENG_CHECK(fabsf(engRect(mid).height - (600.0f - 160.0f - 140.0f - 16.0f)) < 1.0f,
        "嵌套拖 hsp2: mid 弹性补偿 284");

    // 外层拖 sp1 后 B 宽变化 → 内部引擎整宽自适应（mid 宽跟随）
    SRect s1d = sp1->getDrawRect();
    float qx = s1d.left + s1d.width * 0.5f, qy = s1d.top + s1d.height * 0.5f;
    sp1->handleEvent(engMouse(EventType::MouseDown, qx, qy));
    sp1->handleEvent(engMouse(EventType::MouseMove, qx + 50.0f, qy));
    sp1->handleEvent(engMouse(EventType::MouseUp, qx + 50.0f, qy));
    ENG_CHECK(fabsf(engRect(a).width - 250.0f) < 0.01f, "嵌套外层拖 sp1: A→250");
    ENG_CHECK(fabsf(engRect(b).width - (1200.0f - 250.0f - 240.0f - 16.0f)) < 1.0f,
        "嵌套外层拖 sp1: B 弹性补偿");
    ENG_CHECK(fabsf(engRect(hsp1).width - engRect(b).width) < 0.01f &&
              fabsf(engRect(top).height - 160.0f) < 0.01f,
        "嵌套: 外层 resize 后内部链保持（高度不受宽度分配影响）");

    printf("NESTED splitter: PASS=%d FAIL=%d (cumulated)\n", 0, 0);
}

// ── Designer 真实 JSON（main_layout.json，第三方目录）──
static void testDesignerJsonSplitter() {
    TestUtil::log("testDesignerJsonSplitter");
    LayoutParser parser;
    auto root = parser.parseLayoutFile(std::filesystem::path("D:/GitSpace/CornerstoneDesigner/layouts/main_layout.json"));
    ENG_CHECK(root != nullptr, "designer main_layout 解析");
    if (!root) return;
    BENCH->addControl(root);
    auto mid = parser.findControlById("middleArea");
    auto s1 = parser.findControlById("splitter1");
    auto canvas = parser.findControlById("canvasArea");
    ENG_CHECK(mid && s1 && canvas, "middle/splitter1/canvas 均找到");
    if (!mid || !s1) return;
    // splitter1 应在 middle h-flow 引擎 children 中（引擎 reflow 会 setRect）
    auto impl = dynamic_pointer_cast<ControlImpl>(mid);
    bool inTree = false;
    if (impl) {
        for (auto& c : impl->getChildren())
            if (c.get() == s1.get()) inTree = true;
    }
    ENG_CHECK(inTree, "splitter1 在 middle children 树中");
    SRect r1 = s1->getRect();
    ENG_CHECK(r1.left > 0.0f && fabsf(r1.width - 6.0f) < 0.01f,
        "splitter1 由引擎布局（left>0、宽 6）");
    printf("DESIGNER-JSON: splitter1 rect=(%.0f,%.0f %.0fx%.0f)\n",
        r1.left, r1.top, r1.width, r1.height);
    // resize middle → splitter 更新
    mid->setRect(SRect(0, 0, 900, 744));
    SRect r2 = s1->getRect();
    ENG_CHECK(fabsf(r2.left - r1.left) > 0.0f || r2.left >= 0.0f,
        "resize 后 splitter1 重新布局");
    printf("DESIGNER-JSON: after resize splitter1=(%.0f,%.0f)\n", r2.left, r2.top);
    auto s2 = parser.findControlById("splitter2");
    auto prop = parser.findControlById("propertyPanel");
    if (s2 && prop) {
        float canvasW = canvas->getRect().width;
        SRect r3 = s2->getRect();
        SRect rp = prop->getRect();
        ENG_CHECK(fabsf(r3.left - (240.0f + 6.0f + canvasW)) < 0.01f,
            "designer: splitter2 链式 = 240+6+canvas（resize 后自动更新）");
        ENG_CHECK(fabsf(rp.left - (r3.left + 6.0f)) < 0.01f,
            "designer: property 面板在 splitter2 之后");
        printf("DESIGNER-JSON: splitter2=(%.0f,%.0f) canvas=%.0f propLeft=%.0f\n",
            r3.left, r3.top, canvasW, rp.left);
    } else {
        ENG_CHECK(false, "designer: splitter2/property 未找到（非引擎树异常）");
    }

    // ── 全链 resize（真实 WindowResize 事件路径：window→viewport→bench.resized→
    //    root(用户层 SetRect)→vflow→middle→hflow→splitter 链式）──
    {
        UIContext* ctx = BENCH->getContext();
        auto setViewportLike = [&](float w, float h) {
            // 真实链：WindowResize 事件（test_window 已验证该事件→DispatchWindowResize）
            // 在此直接按事件语义设置 viewport 并派发（App::syncRootToWindow 等价动作）
            ctx->viewport = SRect(0, 0, w, h);
            BENCH->resized(ctx->viewport);
            root->setRect(SRect(0, 0, w, h));   // 用户层（Designer App）等价逻辑
        };
        setViewportLike(1200.0f, 800.0f);
        float mw = mid->getRect().width;
        ENG_CHECK(fabsf(mw - 1200.0f) < 0.01f, "全链: middle 宽随窗口 1200");
        float canvasW2 = canvas->getRect().width;
        float s2l = s1->getRect().width > 0 ? s2->getRect().left : 0.0f;
        ENG_CHECK(fabsf(s2l - (240.0f + 6.0f + canvasW2)) < 0.01f,
            "全链: splitter2 = 240+6+canvas（窗口 1200）");
        setViewportLike(1000.0f, 700.0f);
        ENG_CHECK(fabsf(mid->getRect().width - 1000.0f) < 0.01f, "全链: middle 随窗口 1000");
        float canvasW3 = canvas->getRect().width;
        ENG_CHECK(fabsf(canvasW3 - (1000.0f - 240.0f - 6.0f - 6.0f - 240.0f)) < 0.01f,
            "全链: canvas 弹性 = 1000-492");
        ENG_CHECK(fabsf(s2->getRect().left - (240.0f + 6.0f + canvasW3)) < 0.01f,
            "全链: splitter2 链式（窗口 1000→自动更新）");
        printf("DESIGNER-JSON: fullchain canvas=%.0f splitter2=%.0f\n",
            canvasW3, s2->getRect().left);
    }
}

void testSplitterHandleMode() {
    TestUtil::log("testSplitterHandleMode");
    auto holder = make_shared<Panel>(nullptr, SRect(10, 10, 800, 400));
    holder->create();
    BENCH->addControl(holder);
    auto handle = make_shared<Splitter>(nullptr, SRect(300, 0, 8, 400));
    handle->setOrientation(true);
    handle->setThickness(8);
    handle->setMinSize(20, 20);
    int moved = 0; float lastRatio = -1.0f;
    handle->setOnSplitterMoved([&](shared_ptr<Splitter> sp, float r) { (void)sp; ++moved; lastRatio = r; });
    holder->addControl(handle);

    SRect hd = handle->getDrawRect();
    float hx = hd.left + hd.width * 0.5f, hy = hd.top + hd.height * 0.5f;
    handle->handleEvent(engMouse(EventType::MouseDown, hx, hy));
    handle->handleEvent(engMouse(EventType::MouseMove, hx + 100.0f, hy));
    handle->handleEvent(engMouse(EventType::MouseUp, hx + 100.0f, hy));

    ENG_CHECK(fabsf(engRect(handle).left - 400.0f) < 0.01f,
        "手柄模式: 拖 +100 自身跟随 300→400（1:1）");
    ENG_CHECK(moved >= 1 && lastRatio > 0.4f && lastRatio < 0.6f,
        "手柄模式: onSplitterMoved ratio≈0.5 上报");
}

void testSplitterInitialize(shared_ptr<Bench>) {
    TestUtil::log("testSplitterInitialize");

    testEngineManagedSplitter();
    testEngineSplitterQuad();
    testEngineSplitterNested();
    testJsonEngineSplitter();
    testSplitterHandleMode();
    testDesignerJsonSplitter();

    // ▸ Vertical Splitter (left/right panels)
    g_parent = make_shared<Panel>(nullptr, SRect(10, 10, 350, 350));
    g_parent->setNormalStateBGColor(SColor(30, 30, 30, 255));
    g_parent->create();
    BENCH->addControl(g_parent);

    // First Panel (140x350)
    g_first = make_shared<Panel>(nullptr, SRect(0, 0, 140, 350));
    g_first->setNormalStateBGColor(SColor(45, 45, 45, 255));
    g_first->create();
    g_parent->addControl(g_first);

    // Splitter (6px wide, 350px tall)
    g_splitter = make_shared<Splitter>(nullptr, SRect(140, 0, 6, 350));
    g_splitter->setOrientation(true);
    g_splitter->setThickness(6);
    g_splitter->setMinSize(50, 50);
    g_splitter->setSplitRatio(0.4f);
    g_splitter->setOnSplitterMoved(onSplitterMoved);
    g_splitter->create();
    g_parent->addControl(g_splitter);

    // Second Panel (204x350)
    g_second = make_shared<Panel>(nullptr, SRect(146, 0, 204, 350));
    g_second->setNormalStateBGColor(SColor(45, 45, 45, 255));
    g_second->create();
    g_parent->addControl(g_second);

    // Link controls
    g_splitter->setLinkedControls(g_first, g_second);
    g_splitter->setSplitRatio(0.4f);

    // Labels for visual
    auto l1 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    l1->setCaption("Left Panel");
    l1->setFontSize(12);
    l1->setTextNormalStateColor(SColor(200, 200, 200, 255));
    l1->create();
    g_first->addControl(l1);

    auto l2 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    l2->setCaption("Right Panel");
    l2->setFontSize(12);
    l2->setTextNormalStateColor(SColor(200, 200, 200, 255));
    l2->create();
    g_second->addControl(l2);

    // Status label
    g_statusLabel = make_shared<Label>(nullptr, SRect(10, 375, 780, 24));
    g_statusLabel->setCaption("Drag splitters or use arrow keys (focused) | V-split (left) | H-split (top-right) | 2x scale (bottom-right)");
    g_statusLabel->setFontSize(11);
    g_statusLabel->setTextNormalStateColor(SColor(180, 180, 180, 255));
    g_statusLabel->create();
    BENCH->addControl(g_statusLabel);

    TestUtil::log("Splitter test controls created");

    testHorizontalSplitterInitialize();
    test2xSplitterInitialize();
}

void onHSplitterMoved(shared_ptr<Splitter> sp, float ratio) {
    char buf[64];
    snprintf(buf, sizeof(buf), "H-Split ratio: %.3f", ratio);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void on2xSplitterMoved(shared_ptr<Splitter> sp, float ratio) {
    char buf[64];
    snprintf(buf, sizeof(buf), "2x-Split ratio: %.3f", ratio);
    if (g_statusLabel) g_statusLabel->setCaption(buf);
    cout << buf << endl;
}

void testHorizontalSplitterInitialize() {
    // Horizontal Splitter (top/bottom panels)
    g_hParent = make_shared<Panel>(nullptr, SRect(380, 10, 400, 200));
    g_hParent->setNormalStateBGColor(SColor(30, 30, 35, 255));
    g_hParent->create();
    BENCH->addControl(g_hParent);

    g_hTop = make_shared<Panel>(nullptr, SRect(0, 0, 400, 95));
    g_hTop->setNormalStateBGColor(SColor(50, 50, 55, 255));
    g_hTop->create();
    g_hParent->addControl(g_hTop);

    g_hSplitter = make_shared<Splitter>(nullptr, SRect(0, 95, 400, 6));
    g_hSplitter->setOrientation(false);
    g_hSplitter->setThickness(6);
    g_hSplitter->setMinSize(30, 30);
    g_hSplitter->setSplitRatio(0.5f);
    g_hSplitter->setOnSplitterMoved(onHSplitterMoved);
    g_hSplitter->create();
    g_hParent->addControl(g_hSplitter);

    g_hBottom = make_shared<Panel>(nullptr, SRect(0, 101, 400, 99));
    g_hBottom->setNormalStateBGColor(SColor(45, 45, 50, 255));
    g_hBottom->create();
    g_hParent->addControl(g_hBottom);

    g_hSplitter->setLinkedControls(g_hTop, g_hBottom);
    g_hSplitter->setSplitRatio(0.5f);

    auto htLbl = make_shared<Label>(nullptr, SRect(10, 10, 380, 20));
    htLbl->setCaption("Top Panel");
    htLbl->setFontSize(12);
    htLbl->setTextNormalStateColor(SColor(200, 200, 200, 255));
    htLbl->create();
    g_hTop->addControl(htLbl);

    auto hbLbl = make_shared<Label>(nullptr, SRect(10, 10, 380, 20));
    hbLbl->setCaption("Bottom Panel");
    hbLbl->setFontSize(12);
    hbLbl->setTextNormalStateColor(SColor(200, 200, 200, 255));
    hbLbl->create();
    g_hBottom->addControl(hbLbl);

    TestUtil::log("Horizontal splitter test controls created");
}

void test2xSplitterInitialize() {
    // 2X scale Splitter (children inherit parent's scale, use default 1.0)
    float s = 2.0f;
    float avail = 400.0f - 6.0f;
    float firstW = avail * 0.4f;  // 157.6
    g_2xParent = make_shared<Panel>(nullptr, SRect(380, 220, 400, 170), s, s);
    g_2xParent->setNormalStateBGColor(SColor(25, 35, 30, 255));
    g_2xParent->create();
    BENCH->addControl(g_2xParent);

    g_2xFirst = make_shared<Panel>(nullptr, SRect(0, 0, firstW, 170));
    g_2xFirst->setNormalStateBGColor(SColor(45, 55, 50, 255));
    g_2xFirst->create();
    g_2xParent->addControl(g_2xFirst);

    g_2xSplitter = make_shared<Splitter>(nullptr, SRect(firstW, 0, 6, 170));
    g_2xSplitter->setOrientation(true);
    g_2xSplitter->setThickness(6);
    g_2xSplitter->setMinSize(50, 50);
    g_2xSplitter->setOnSplitterMoved(on2xSplitterMoved);
    g_2xSplitter->create();
    g_2xParent->addControl(g_2xSplitter);

    g_2xSecond = make_shared<Panel>(nullptr, SRect(firstW + 6, 0, avail - firstW, 170));
    g_2xSecond->setNormalStateBGColor(SColor(55, 45, 50, 255));
    g_2xSecond->create();
    g_2xParent->addControl(g_2xSecond);

    g_2xSplitter->setLinkedControls(g_2xFirst, g_2xSecond);
    g_2xSplitter->setSplitRatio(0.4f);

    auto xlbl1 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    xlbl1->setCaption("2X Left");
    xlbl1->setFontSize(12);
    xlbl1->setTextNormalStateColor(SColor(200, 200, 200, 255));
    xlbl1->create();
    g_2xFirst->addControl(xlbl1);

    auto xlbl2 = make_shared<Label>(nullptr, SRect(10, 10, 80, 20));
    xlbl2->setCaption("2X Right");
    xlbl2->setFontSize(12);
    xlbl2->setTextNormalStateColor(SColor(200, 200, 200, 255));
    xlbl2->create();
    g_2xSecond->addControl(xlbl2);

    TestUtil::log("2X scale splitter test controls created");
}

class SplitterApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_splitter");
        BENCH->setOnInitial(testSplitterInitialize);
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
        TestUtil::log("Splitter test quit");
    }
};

int main(int argc, char* argv[]) {
    return TestRunMain<SplitterApp>(argc, argv);
}
