// ============================================================================
// test_shape.cpp -- Shape 形状控件测试
// 断言：属性四层回环 / points 缩放 / mapToDrawPoint / ringWidth·lineWidth 负值钳制 / 未知形状拒绝
// 可视化：7 形状矩阵（空心/实心/环/凹多边形耳切）挂树，供视觉验收
// ============================================================================
#include <iostream>
#include <memory>
#include <cmath>
#include "Shape.h"
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "TestInstance.h"
#include "UICornerstoneAPI.h"
#include "LayoutParser.h"

using namespace std;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { ++g_pass; TestUtil::log("OK   %s", msg); } \
    else      { ++g_fail; TestUtil::log("FAIL %s", msg); } \
} while (0)

static shared_ptr<Shape> g_probe;    // 属性断言探针

static void runAssertions() {
    TestUtil::log("---- Shape assertions ----");

    // 1) C++ setter 回环
    g_probe->setShape(ShapeType::Circle);
    CHECK(g_probe->getShape() == ShapeType::Circle, "setShape/getShape Circle");

    // 2) CABI 同键：SetEnum("shape","ellipse")
    CHECK(g_probe->setEnumProperty("shape", "ellipse") == 1 &&
          g_probe->getShape() == ShapeType::Ellipse, "SetEnum(shape)=ellipse");

    // 3) 负例：未知形状拒绝且保持原值
    CHECK(g_probe->setEnumProperty("shape", "hexagon") == 0 &&
          g_probe->getShape() == ShapeType::Ellipse, "unknown shape rejected, keep ellipse");

    // 4) 颜色属性回环
    g_probe->setFillColor(SColor(59, 130, 246));
    SColor c;
    CHECK(g_probe->getColorProperty("fill", c) == 1 && c.alphaByte() == 255,
          "fill color roundtrip (alphaByte=255)");

    // 5) 数值属性回环 + 负值钳制
    CHECK(g_probe->setFloatProperty("line-width", 3.5f) == 1, "SetFloat(line-width)");
    float f = 0.f;
    CHECK(g_probe->getFloatProperty("line-width", f) == 1 && f == 3.5f,
          "line-width roundtrip 3.5");
    g_probe->setRingWidth(-5.f);
    CHECK(g_probe->getRingWidth() == 0.f, "negative ringWidth clamped to 0");
    g_probe->setLineWidth(-1.f);
    CHECK(g_probe->getLineWidth() == 0.f, "negative lineWidth clamped to 0");

    // 6) points 缩放（决策 3.3：以写入时 rect 为基准等比缩放）
    auto probe2 = make_shared<Shape>(nullptr, SRect(100, 100, 200, 100));
    probe2->setPoints({{10.f, 10.f}, {190.f, 90.f}});
    probe2->setRect(SRect(100, 100, 400, 200));            // x/y 各 2 倍缩放
    auto scaled = probe2->getPoints();
    CHECK(scaled.size() == 2 && fabs(scaled[1].x - 380.f) < 0.01f &&
          fabs(scaled[1].y - 180.f) < 0.01f, "points scale x2 after setRect");

    // 7) mapToDrawPoint / getDrawPoint（本地→全局 = rect.left/top + 缩放后本地）
    const SPoint g = probe2->mapToDrawPoint(10.f, 10.f);
    CHECK(fabs(g.x - 110.f) < 0.01f && fabs(g.y - 110.f) < 0.01f,
          "mapToDrawPoint local->global");
    const SPoint gp = probe2->getDrawPoint(0);
    CHECK(fabs(gp.x - 120.f) < 0.01f && fabs(gp.y - 120.f) < 0.01f,
          "getDrawPoint(0)=mapToDrawPoint(scaled[0])");

    // 8) 多图元（组合图形）
    CHECK(g_probe->addPrimitive(ShapeType::Circle, SRect(0, 0, 20, 20)) == 0, "addPrimitive #0");
    CHECK(g_probe->addPrimitive(ShapeType::Rect, SRect(10, 10, 20, 20)) == 1, "addPrimitive #1");
    CHECK(g_probe->getPrimitiveCount() == 2, "getPrimitiveCount == 2");
    g_probe->setPrimitiveFill(0, SColor(255, 0, 0, 255));
    g_probe->setPrimitiveStroke(1, SColor(0, 255, 0, 255));
    g_probe->setPrimitiveLineWidth(1, 2.f);
    g_probe->setPrimitiveRadius(1, 4.f);
    g_probe->setPrimitiveRingWidth(0, 3.f);
    g_probe->setPrimitivePoints(1, {{0.f, 0.f}, {10.f, 10.f}});
    g_probe->clearPrimitives();
    CHECK(g_probe->getPrimitiveCount() == 0, "clearPrimitives");

    // 9) 背景色（setBackgroundStateColor 自动取消透明）
    CHECK(g_probe->getTransparent(), "shape default transparent");
    g_probe->setBackgroundStateColor(StateColor(SColor(32, 40, 60, 255), SColor(32, 40, 60, 255),
                                                SColor(32, 40, 60, 255), SColor(32, 40, 60, 255)));
    CHECK(!g_probe->getTransparent(), "setBackgroundStateColor -> not transparent");

    TestUtil::log("---- assertions done: pass=%d fail=%d ----", g_pass, g_fail);
}

static void addShape(Bench* bench, const shared_ptr<Shape>& s) {
    s->create();
    bench->addControl(s);
}

static void addCaption(Bench* bench, int x, int y, const char* txt) {
    auto lbl = LabelBuilder(nullptr, SRect(x, y, 240, 22))
                   .setCaption(txt).build();
    lbl->create();
    bench->addControl(lbl);
}

// ── 可视化矩阵 ──
static void testShapeVisualize(Bench* bench) {
    g_probe = make_shared<Shape>(nullptr, SRect(0, 0, 40, 40));   // 探针不挂树
    runAssertions();

    int row = 40, col = 30;

    // 行1：rect 描边 / filled-rect 实心 / round-rect 圆角
    addCaption(bench, col, row, "rect(stroke) / filled-rect / round-rect");
    row += 28;
    {
        auto s = make_shared<Shape>(nullptr, SRect(col, row, 90, 90));
        s->setStrokeColor(SColor(30, 41, 59)); s->setLineWidth(2);
        addShape(bench, s);
    }
    {
        auto s = make_shared<Shape>(nullptr, SRect(col + 120, row, 90, 90));
        s->setShape(ShapeType::FilledRect); s->setFillColor(SColor(59, 130, 246));
        addShape(bench, s);
    }
    {
        auto s = make_shared<Shape>(nullptr, SRect(col + 240, row, 90, 90));
        s->setShape(ShapeType::RoundRect); s->setRadius(18);
        s->setFillColor(SColor(16, 185, 129)); s->setStrokeColor(SColor(6, 78, 59)); s->setLineWidth(3);
        addShape(bench, s);
    }

    // 行2：circle 实心 / circle 空心 / circle 环(ringWidth)
    addCaption(bench, col, row + 110, "circle solid / hollow / ring");
    row += 138;
    {
        auto s = make_shared<Shape>(nullptr, SRect(col, row, 90, 90));
        s->setShape(ShapeType::Circle); s->setFillColor(SColor(239, 68, 68));
        addShape(bench, s);
    }
    {
        auto s = make_shared<Shape>(nullptr, SRect(col + 120, row, 90, 90));
        s->setShape(ShapeType::Circle); s->setStrokeColor(SColor(30, 41, 59)); s->setLineWidth(2);
        addShape(bench, s);
    }
    {
        auto s = make_shared<Shape>(nullptr, SRect(col + 240, row, 90, 90));
        s->setShape(ShapeType::Circle); s->setFillColor(SColor(59, 130, 246)); s->setRingWidth(14);
        addShape(bench, s);
    }

    // 行3：ellipse 拉伸 / polyline 粗折线 / polygon 凹多边形(箭头,验证耳切)
    addCaption(bench, col, row + 110, "ellipse stretch / polyline w4 / polygon concave(earclip)");
    row += 138;
    {
        auto s = make_shared<Shape>(nullptr, SRect(col, row, 140, 70));
        s->setShape(ShapeType::Ellipse); s->setFillColor(SColor(168, 85, 247));
        s->setStrokeColor(SColor(59, 7, 100)); s->setLineWidth(2);
        addShape(bench, s);
    }
    {
        auto s = make_shared<Shape>(nullptr, SRect(col + 160, row, 150, 80));
        s->setShape(ShapeType::Polyline);
        s->setPoints({{5.f, 75.f}, {45.f, 15.f}, {85.f, 55.f}, {125.f, 5.f}});
        s->setStrokeColor(SColor(234, 88, 12)); s->setLineWidth(4);
        addShape(bench, s);
    }
    {
        auto s = make_shared<Shape>(nullptr, SRect(col + 330, row, 150, 90));
        s->setShape(ShapeType::Polygon);                       // 凹多边形（箭头）
        s->setPoints({{0.f, 0.f}, {70.f, 0.f}, {70.f, 20.f}, {145.f, 35.f},
                      {70.f, 50.f}, {70.f, 70.f}, {0.f, 70.f}});
        s->setFillColor(SColor(245, 158, 11)); s->setStrokeColor(SColor(124, 45, 18)); s->setLineWidth(2);
        addShape(bench, s);
    }

    // 行4：JSON 声明式路径抽查（经 LayoutParser 的控件由 test_layout 覆盖；此处补 CABI 属性路径）
    addCaption(bench, col, row + 110, "CABI: SetEnum(shape)+SetColor(fill)+SetFloat(ring-width)");
    row += 138;
    {
        auto s = make_shared<Shape>(nullptr, SRect(col, row, 90, 90));
        s->create(); bench->addControl(s);
        s->setEnumProperty("shape", "ellipse");
        s->setColorProperty("fill", SColor(34, 197, 94));
        s->setFloatProperty("ring-width", 10.f);
    }

    // 行5：背景色 / 多图元组合（房子：round-rect 墙 + polygon 屋顶 + circle 窗）/ CABI 图元
    addCaption(bench, col, row + 110, "bg-color / primitives(house) / CABI primitives");
    row += 138;
    {
        // 背景色
        auto s = make_shared<Shape>(nullptr, SRect(col, row, 90, 90));
        s->setShape(ShapeType::Circle);
        s->setStrokeColor(SColor(30, 41, 59)); s->setLineWidth(2);
        s->setBackgroundStateColor(StateColor(SColor(32, 40, 60), SColor(32, 40, 60),
                                              SColor(32, 40, 60), SColor(32, 40, 60)));
        addShape(bench, s);
    }
    {
        // 组合图形（房子）：墙 round-rect + 屋顶 polygon + 窗 circle + 门 rect
        auto s = make_shared<Shape>(nullptr, SRect(col + 120, row, 150, 130));
        const SColor wall(52, 211, 153), roof(239, 68, 68), win(59, 130, 246), door(245, 158, 11);
        const int w = s->addPrimitive(ShapeType::RoundRect, SRect(10, 50, 130, 75));
        s->setPrimitiveFill(w, wall); s->setPrimitiveRadius(w, 6.f);
        const int r = s->addPrimitive(ShapeType::Polygon, SRect(0, 0, 0, 0));
        s->setPrimitivePoints(r, {{0.f, 52.f}, {75.f, 0.f}, {150.f, 52.f}});
        s->setPrimitiveFill(r, roof);
        const int win1 = s->addPrimitive(ShapeType::Circle, SRect(28, 68, 30, 30));
        s->setPrimitiveFill(win1, win);
        const int d = s->addPrimitive(ShapeType::FilledRect, SRect(92, 85, 30, 40));
        s->setPrimitiveFill(d, door);
        addShape(bench, s);
    }
    {
        // CABI 图元路径：CreateShape + ShapeAddPrimitive + SetPrimitiveColor/Float
        UIControlHandle h = UICornerstone_CreateShape(g_uiInstance,
            float(col + 300), float(row), 120.f, 90.f, 1.f, 1.f);
        CHECK(h != nullptr, "CABI CreateShape");
        const int p0 = UICornerstone_ShapeAddPrimitive(g_uiInstance, h, "round-rect", 0.f, 0.f, 120.f, 90.f);
        CHECK(p0 == 0, "CABI ShapeAddPrimitive #0");
        const int p1 = UICornerstone_ShapeAddPrimitive(g_uiInstance, h, "circle", 15.f, 20.f, 50.f, 50.f);
        CHECK(p1 == 1, "CABI ShapeAddPrimitive #1");
        const int p2 = UICornerstone_ShapeAddPrimitive(g_uiInstance, h, "bogus", 0.f, 0.f, 10.f, 10.f);
        CHECK(p2 == -1, "CABI ShapeAddPrimitive unknown type rejected");
        UIColor blue{59, 130, 246, 255};
        CHECK(UICornerstone_ShapeSetPrimitiveColor(g_uiInstance, h, p0, "fill", blue) == 1,
              "CABI SetPrimitiveColor(fill)");
        CHECK(UICornerstone_ShapeSetPrimitiveFloat(g_uiInstance, h, p0, "radius", 10.f) == 1,
              "CABI SetPrimitiveFloat(radius)");
        UIColor yellow{250, 204, 21, 255};
        CHECK(UICornerstone_ShapeSetPrimitiveColor(g_uiInstance, h, p1, "fill", yellow) == 1,
              "CABI SetPrimitiveColor(fill) #1");
    }

    // JSON：primitives + colors.background 解析
    {
        const string json = R"({
          "controls":[{
            "type":"shape","id":"comp","rect":[600,40,120,120],
            "colors":{"background":"#324057FF"},
            "primitives":[
              {"shape":"round-rect","rect":[0,0,120,120],"radius":12,"fill":"#1E293B","stroke":"#0F172A","line-width":2},
              {"shape":"circle","rect":[10,10,40,40],"fill":"#EF4444"},
              {"shape":"polyline","points":[{"x":0,"y":100},{"x":60,"y":60},{"x":120,"y":100}],"stroke":"#F59E0B","line-width":3}
            ]
          }]
        })";
        LayoutParser parser;
        auto root = parser.parseLayout(json);
        auto* ci = root ? dynamic_cast<ControlImpl*>(root.get()) : nullptr;
        auto js = ci ? dynamic_pointer_cast<Shape>(ci->getThis()) : nullptr;
        CHECK(js != nullptr, "JSON shape+primitives parsed");
        if (js) {
            CHECK(js->getPrimitiveCount() == 3, "JSON primitives count == 3");
            CHECK(!js->getTransparent(), "JSON colors.background -> not transparent");
        }
    }

    // 缩放可视化：normal vs 1.5x（ShapeBuilder 路径；getDrawRect = rect × scale）
    addCaption(bench, 480, 570, "scale: normal(80) vs 2.0x(160) (builder)");
    {
        auto norm = ShapeBuilder(nullptr, SRect(480, 620, 80, 80))
                        .setShape(ShapeType::Circle)
                        .setFillColor(SColor(59, 130, 246))
                        .build();
        bench->addControl(norm);
        auto scaled = ShapeBuilder(nullptr, SRect(620, 600, 80, 80), 2.0f, 2.0f)
                          .setShape(ShapeType::Circle)
                          .setFillColor(SColor(239, 68, 68))
                          .build();
        bench->addControl(scaled);
        CHECK(fabs(scaled->getDrawRect().width - 160.f) < 0.01f, "scaled shape drawRect = rect*2.0");
        CHECK(fabs(scaled->getDrawRect().left - 620.f) < 0.01f, "scaled shape drawRect.left unscaled");
    }
}

class ShapeApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_shape");
        BENCH->setOnInitial([](shared_ptr<Bench> b) { testShapeVisualize(b.get()); });
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(40, 40, 44));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
        // 视觉验证：帧内读回（Render 后、Present 前，design/Capture_API_Design.md §3.6）
        if (++m_frames == 30 && !m_saved) {
            static uint8_t pixels[1400 * 900 * 4];
            int w = 0, h = 0;
            const int cap = UICornerstone_CaptureViewport(g_uiInstance, pixels, &w, &h);
            const std::string out = "Temp/shape_capture.bmp";
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, out.c_str()) : 0;
            TestUtil::log("capture: cap=%d saved=%d (%dx%d) -> %s", cap, saved, w, h,
                          saved ? out.c_str() : "FAILED");
            m_saved = true;
        }
    }
    void onQuit() override { TestUtil::log("Shape test quit"); }

private:
    int m_frames = 0;
    bool m_saved = false;
};

int main(int argc, char* argv[]) {
    return TestRunMain<ShapeApp>(argc, argv);
}
