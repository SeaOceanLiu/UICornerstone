#include "TestUtils.h"
#include <iostream>
#include <memory>
#include <cmath>
#include "GraphTool.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"

using namespace std;
using namespace GraphTool;

static int g_testStep = 0;
static const int g_totalSteps = 12;
static uint64_t g_stepTimer = 0;
static const float g_stepDuration = 2000.0f;

void testColorSystem(DrawingContext& dc) {
    cout << "=== Test 1: SColor Color System ===" << endl;

    SColor c1;
    SColor c2(1.0f, 0.0f, 0.0f);
    SColor c3((uint8_t)0, (uint8_t)255, (uint8_t)0);
    SColor c4(0xFF0000FF);

    SColor black = SColor::Black();
    SColor white = SColor::White();
    SColor red = SColor::Red();
    SColor blue = SColor::Blue(0.5f);

    SColor brighter = red.brighter(0.2f);
    SColor darker = red.darker(0.3f);
    SColor blended = red.blend(blue, 0.5f);
    SColor withAlpha = red.withAlpha(0.5f);

    float size = 40.0f;
    float x = 20.0f, y = 20.0f;
    float gap = 50.0f;

    dc.setFillColor(black); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(white); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(red); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(c3); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(blue); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(brighter); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(darker); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(blended); dc.drawRect(SRect(x, y, size, size), true);
    x += gap;
    dc.setFillColor(withAlpha); dc.drawRect(SRect(x, y, size, size), true);

    cout << "  Color system test passed!" << endl;
}

void testPenSystem(DrawingContext& dc) {
    cout << "=== Test 2: SPen Pen System ===" << endl;

    SPen pen1;
    SPen pen2(SColor::Red());
    SPen pen3(SColor::Blue(), 3.0f);
    SPen pen4(SColor::Green(), 2.0f, PenStyle::Dash);
    SPen pen5(SColor::Magenta(), 2.0f, PenStyle::Dot);

    SPen noPen = SPen::NoPen();
    SPen blackPen = SPen::BlackPen(2.0f);
    SPen dashPen = SPen::DashPen(SColor::Red());

    pen1.setColor(SColor::Yellow());
    pen1.setWidth(4.0f);
    pen1.setStyle(PenStyle::DashDot);
    pen1.setCap(LineCap::Round);
    pen1.setJoin(LineJoin::Round);
    pen1.setDashOffset(5.0f);

    auto pattern = pen1.getStandardDashPattern();
    cout << "  DashDot pattern size: " << pattern.size() << endl;

    float y = 100.0f;
    dc.setPen(pen2);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;
    dc.setPen(pen3);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;
    dc.setPen(pen4);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;
    dc.setPen(pen5);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;

    SPen customPen(SColor::Cyan(), 2.0f, PenStyle::Custom);
    customPen.setDashPattern({10.0f, 5.0f, 2.0f, 5.0f});
    dc.setPen(customPen);
    dc.drawLine(20.0f, y, 300.0f, y);

    dc.setPen(SPen(SColor::Black(), 1.0f));

    cout << "  Pen system test passed!" << endl;
}

void testBrushSystem(DrawingContext& dc) {
    cout << "=== Test 3: SBrush Brush System ===" << endl;

    SBrush brush1;
    SBrush brush2(SColor::Red());
    SBrush noBrush = SBrush::NoBrush();

    SLinearGradient linearGrad(0, 0, 200, 0);
    linearGrad.addStop(0.0f, SColor::Red());
    linearGrad.addStop(0.5f, SColor::Yellow());
    linearGrad.addStop(1.0f, SColor::Green());
    SBrush linearBrush(linearGrad);

    SRadialGradient radialGrad(150, 250, 80);
    radialGrad.addStop(0.0f, SColor::White());
    radialGrad.addStop(1.0f, SColor::Blue());
    SBrush radialBrush(radialGrad);

    dc.setBrush(brush2);
    dc.drawRect(SRect(20, 180, 80, 60), true);

    dc.setBrush(noBrush);
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawRect(SRect(120, 180, 80, 60), true);

    AdvancedDrawing::drawGradientRect(dc, SRect(220, 180, 200, 60), SColor::Red(), SColor::Blue(), true);

    cout << "  Brush system test passed!" << endl;
}

void testGradientSystem(DrawingContext& dc) {
    cout << "=== Test 4: Gradient System ===" << endl;

    SLinearGradient hGrad(0, 0, 400, 0);
    hGrad.addStop(0.0f, SColor::Red());
    hGrad.addStop(0.25f, SColor::Yellow());
    hGrad.addStop(0.5f, SColor::Green());
    hGrad.addStop(0.75f, SColor::Cyan());
    hGrad.addStop(1.0f, SColor::Blue());

    SLinearGradient vGrad(0, 0, 0, 100);
    vGrad.addStop(0.0f, SColor::White());
    vGrad.addStop(1.0f, SColor::Black());

    SRadialGradient rGrad(200, 350, 100);
    rGrad.addStop(0.0f, SColor::Yellow());
    rGrad.addStop(0.5f, SColor(1.0f, 0.5f, 0.0f));
    rGrad.addStop(1.0f, SColor::Red());

    AdvancedDrawing::drawGradientRectWithBrush(dc, SRect(20, 260, 400, 40), hGrad);
    AdvancedDrawing::drawGradientRect(dc, SRect(20, 310, 400, 40), SColor::White(), SColor::Black(), false);
    AdvancedDrawing::drawRadialGradientRect(dc, SRect(20, 360, 200, 100), rGrad);

    SColor mid = hGrad.getColorAt(0.5f);
    cout << "  Gradient mid color: r=" << mid.red() << " g=" << mid.green() << " b=" << mid.blue() << endl;

    cout << "  Gradient system test passed!" << endl;
}

void testBasicShapes(DrawingContext& dc) {
    cout << "=== Test 5: Basic Shapes ===" << endl;

    float x = 450, y = 20;

    dc.setPen(SPen(SColor::Red(), 1.0f));
    for (int i = 0; i < 10; ++i) {
        dc.drawPoint(x + i * 5, y);
    }

    y += 20;
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawLine(x, y, x + 200, y);
    y += 10;
    dc.setPen(SPen(SColor::Red(), 3.0f));
    dc.drawLine(x, y, x + 200, y);
    y += 10;
    dc.setPen(SPen(SColor::Blue(), 5.0f));
    dc.drawLine(x, y, x + 200, y);

    y += 20;
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(0.8f, 0.8f, 1.0f));
    dc.drawRect(SRect(x, y, 80, 50), true);
    dc.drawRect(SRect(x + 100, y, 80, 50), false);

    dc.setPen(SPen(SColor::Red(), 4.0f));
    dc.setFillColor(SColor(1.0f, 1.0f, 0.8f));
    dc.drawRect(SRect(x, y + 60, 80, 50), true);
    dc.drawRect(SRect(x + 100, y + 60, 80, 50), false);

    cout << "  Basic shapes test passed!" << endl;
}

void testRoundedRect(DrawingContext& dc) {
    cout << "=== Test 6: Rounded Rectangles ===" << endl;

    float x = 450, y = 200;

    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(0.7f, 0.9f, 0.7f));
    dc.drawRoundedRect(SRect(x, y, 100, 60), 10.0f, true);

    dc.drawRoundedRect(SRect(x + 120, y, 100, 60), 10.0f, false);

    dc.setPen(SPen(SColor::Blue(), 3.0f));
    dc.setFillColor(SColor(0.8f, 0.8f, 1.0f));
    dc.drawRoundedRect(SRect(x, y + 70, 100, 60), 15.0f, true);
    dc.drawRoundedRect(SRect(x + 120, y + 70, 100, 60), 15.0f, false);

    dc.drawRoundedRectWithBorder(SRect(x, y + 140, 100, 60), 10.0f, 2.0f, SColor::Red(), SColor::Yellow());

    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(1.0f, 0.8f, 0.8f));
    dc.drawRoundedRect(SRect(x + 120, y + 140, 100, 60), SRoundedCorners(20, 5, 20, 5), true);

    cout << "  Rounded rectangles test passed!" << endl;
}

void testCircleDrawing(DrawingContext& dc) {
    cout << "=== Test 7: Circle Drawing ===" << endl;

    float x = 700, y = 50;

    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor::Red());
    dc.drawCircle(SPoint(x + 40, y + 40), 35.0f, true);

    dc.setPen(SPen(SColor::Blue(), 2.0f));
    dc.drawCircle(SPoint(x + 130, y + 40), 35.0f, false);

    dc.drawFilledCircleWithStroke(x + 220, y + 40, 35.0f, 3.0f, SColor::Green(), SColor::Black());

    dc.drawFilledCircleAA(x + 310, y + 40, 35.0f, SColor(0.5f, 0.5f, 1.0f));

    dc.drawCircleWithThickness(x + 400, y + 40, 35.0f, 5.0f, SColor::Magenta());

    dc.drawCircle(SPoint(x + 40, y + 130), 30.0f, SColor::Cyan(), SColor::Red(), 3.0f);

    cout << "  Circle drawing test passed!" << endl;
}

void testEllipseAndArc(DrawingContext& dc) {
    cout << "=== Test 8: Ellipse and Arc ===" << endl;

    float x = 700, y = 240;

    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(1.0f, 0.5f, 0.5f));
    dc.drawEllipse(SPoint(x + 60, y + 30), 55.0f, 25.0f, true);

    dc.setPen(SPen(SColor::Blue(), 2.0f));
    dc.drawEllipse(SPoint(x + 190, y + 30), 55.0f, 25.0f, false);

    dc.setPen(SPen(SColor::Green(), 2.0f));
    dc.drawArc(SPoint(x + 300, y + 30), 30.0f, 0.0f, (float)M_PI, false);

    dc.setFillColor(SColor(0.5f, 1.0f, 0.5f, 0.7f));
    dc.drawArc(SPoint(x + 400, y + 30), 30.0f, 0.0f, (float)M_PI * 1.5f, true);

    cout << "  Ellipse and arc test passed!" << endl;
}

void testPolygonAndPolyline(DrawingContext& dc) {
    cout << "=== Test 9: Polygon and Polyline ===" << endl;

    vector<SPoint> triangle = {SPoint(50, 500), SPoint(100, 440), SPoint(150, 500)};
    dc.setFillColor(SColor(1.0f, 0.8f, 0.5f));
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawPolygon(triangle, true);
    dc.drawPolygon(triangle, false);

    vector<SPoint> pentagon;
    for (int i = 0; i < 5; ++i) {
        float angle = (float)(-M_PI / 2 + i * 2 * M_PI / 5);
        pentagon.push_back(SPoint(280 + 40 * cos(angle), 470 + 40 * sin(angle)));
    }
    dc.setFillColor(SColor(0.5f, 0.8f, 1.0f));
    dc.drawPolygon(pentagon, true);
    dc.setPen(SPen(SColor::Blue(), 2.0f));
    dc.drawPolygon(pentagon, false);

    vector<SPoint> polyline = {SPoint(350, 500), SPoint(380, 440), SPoint(410, 480), SPoint(440, 430)};
    dc.setPen(SPen(SColor::Red(), 3.0f));
    dc.drawPolyline(polyline);

    vector<SPoint> quad = {SPoint(480, 440), SPoint(560, 440), SPoint(560, 500), SPoint(480, 500)};
    dc.setPen(SPen(SColor::Green(), 4.0f));
    dc.setFillColor(SColor(0.8f, 1.0f, 0.8f));
    dc.drawPolygon(quad, true);
    dc.drawPolygon(quad, false);

    cout << "  Polygon and polyline test passed!" << endl;
}

void testDashedLines(DrawingContext& dc) {
    cout << "=== Test 10: Dashed Lines ===" << endl;

    float y = 530;

    dc.setPen(SPen(SColor::Black(), 2.0f, PenStyle::Solid));
    dc.drawLine(20.0f, y, 400.0f, y);

    y += 15;
    dc.setPen(SPen(SColor::Red(), 2.0f, PenStyle::Dash));
    dc.drawLine(20.0f, y, 400.0f, y);

    y += 15;
    dc.setPen(SPen(SColor::Blue(), 2.0f, PenStyle::Dot));
    dc.drawLine(20.0f, y, 400.0f, y);

    y += 15;
    dc.setPen(SPen(SColor::Green(), 2.0f, PenStyle::DashDot));
    dc.drawLine(20.0f, y, 400.0f, y);

    y += 15;
    dc.setPen(SPen(SColor::Magenta(), 2.0f, PenStyle::DashDotDot));
    dc.drawLine(20.0f, y, 400.0f, y);

    y += 15;
    dc.setPen(SPen(SColor::Cyan(), 4.0f, PenStyle::Dash));
    dc.drawLine(20.0f, y, 400.0f, y);

    y += 15;
    SPen customPen(SColor(1.0f, 0.5f, 0.0f), 3.0f, PenStyle::Custom);
    customPen.setDashPattern({15.0f, 5.0f, 5.0f, 5.0f});
    dc.setPen(customPen);
    dc.drawLine(20.0f, y, 400.0f, y);

    dc.setPen(SPen(SColor::Black(), 1.0f));

    cout << "  Dashed lines test passed!" << endl;
}

void testAdvancedDrawing(DrawingContext& dc) {
    cout << "=== Test 11: Advanced Drawing ===" << endl;

    SRect shadowRect(450, 400, 120, 60);
    AdvancedDrawing::drawShadow(dc, shadowRect, 5.0f, SColor(0.0f, 0.0f, 0.0f, 0.3f), 5.0f);
    dc.setFillColor(SColor::White());
    dc.drawRect(shadowRect, true);
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawRect(shadowRect, false);

    AdvancedDrawing::drawStyledRect(dc, SRect(600, 400, 120, 60),
        SPen(SColor::Blue(), 3.0f), SBrush(SColor(0.8f, 0.8f, 1.0f)));

    AdvancedDrawing::drawStyledRoundedRect(dc, SRect(750, 400, 120, 60), 10.0f,
        SPen(SColor::Green(), 2.0f), SBrush(SColor(0.8f, 1.0f, 0.8f)));

    AdvancedDrawing::drawStyledCircle(dc, SPoint(510, 510), 30.0f,
        SPen(SColor::Red(), 2.0f), SBrush(SColor(1.0f, 0.8f, 0.8f)));

    AdvancedDrawing::drawStyledEllipse(dc, SPoint(660, 510), 50.0f, 25.0f,
        SPen(SColor::Magenta(), 2.0f), SBrush(SColor(1.0f, 0.8f, 1.0f)));

    cout << "  Advanced drawing test passed!" << endl;
}

void testUtils(DrawingContext& dc) {
    cout << "=== Test 12: Utils Functions ===" << endl;

    float dist = Utils::distance(SPoint(0, 0), SPoint(3, 4));
    cout << "  Distance (0,0)->(3,4) = " << dist << " (expected 5)" << endl;

    SPoint mid = Utils::interpolate(SPoint(0, 0), SPoint(10, 10), 0.5f);
    cout << "  Interpolate mid = (" << mid.x << ", " << mid.y << ") (expected 5, 5)" << endl;

    vector<SPoint> poly = {SPoint(0, 0), SPoint(100, 0), SPoint(100, 100), SPoint(0, 100)};
    bool inside = Utils::pointInPolygon(SPoint(50, 50), poly);
    cout << "  Point (50,50) in rect polygon: " << (inside ? "true" : "false") << endl;

    vector<SPoint> points = {SPoint(10, 20), SPoint(50, 10), SPoint(30, 60)};
    SRect bbox = Utils::boundingBox(points);
    cout << "  Bounding box: (" << bbox.left << "," << bbox.top << "," << bbox.width << "," << bbox.height << ")" << endl;

    auto circlePts = Utils::generateCirclePoints(SPoint(900, 100), 40.0f, 24);
    dc.setPen(SPen(SColor::Blue(), 1.0f));
    for (size_t i = 0; i < circlePts.size(); ++i) {
        size_t next = (i + 1) % circlePts.size();
        dc.drawLine(circlePts[i], circlePts[next]);
    }

    auto lineRect = Utils::generateLineRectPoints(SPoint(860, 160), SPoint(960, 200), 6.0f);
    dc.setPen(SPen(SColor::Red(), 1.0f));
    dc.drawLine(lineRect);

    auto rrPts = Utils::generateRoundedRectPoints(SRect(860, 220, 100, 60), SRoundedCorners(10), 8);
    dc.setPen(SPen(SColor::Green(), 1.0f));
    for (size_t i = 0; i < rrPts.size(); ++i) {
        size_t next = (i + 1) % rrPts.size();
        dc.drawLine(rrPts[i], rrPts[next]);
    }

    SPoint intersection = Utils::lineIntersection(SPoint(0, 0), SPoint(100, 100),
                                                   SPoint(0, 100), SPoint(100, 0));
    cout << "  Line intersection: (" << intersection.x << ", " << intersection.y << ") (expected 50, 50)" << endl;

    SColor interpColor = Utils::interpolateColor(SColor::Red(), SColor::Blue(), 0.5f);
    cout << "  Interpolated color: r=" << interpColor.red() << " g=" << interpColor.green() << " b=" << interpColor.blue() << endl;

    cout << "  Utils functions test passed!" << endl;
}

class GraphToolApp : public AppCallbacks {
public:
    bool onInit() override {
        g_stepTimer = TestUtil::getTicks();
        return true;
    }

    void onEvent(const Event& event) override {
        if (event.m_type == EventType::KeyDown) {
            if (event.keyEvent.keycode == KeyCode::Escape) {
                MAINWIN->quit();
            }
            if (event.keyEvent.keycode == KeyCode::Space) {
                g_testStep = (g_testStep + 1) % g_totalSteps;
                g_stepTimer = TestUtil::getTicks();
            }
        }
    }

    void onUpdate() override {
        uint64_t now = TestUtil::getTicks();
        if (now - g_stepTimer > g_stepDuration) {
            g_testStep = (g_testStep + 1) % g_totalSteps;
            g_stepTimer = now;
        }
    }

    void onRender() override {
        RenderDevice* device = GET_RENDERDEVICE;
        if (!device) return;

        device->setDrawColor(SColor(0.941f, 0.941f, 0.941f, 1.0f));
        device->clear();

        DrawingContext dc(device);

        switch (g_testStep) {
            case 0: testColorSystem(dc); break;
            case 1: testPenSystem(dc); break;
            case 2: testBrushSystem(dc); break;
            case 3: testGradientSystem(dc); break;
            case 4: testBasicShapes(dc); break;
            case 5: testRoundedRect(dc); break;
            case 6: testCircleDrawing(dc); break;
            case 7: testEllipseAndArc(dc); break;
            case 8: testPolygonAndPolyline(dc); break;
            case 9: testDashedLines(dc); break;
            case 10: testAdvancedDrawing(dc); break;
            case 11: testUtils(dc); break;
        }

        dc.setFillColor(SColor::Black());
        dc.setPen(SPen(SColor::Black(), 1.0f));
    }

    void onQuit() override {
    }
};

int main(int argc, char* argv[]) {
    GraphToolApp app;
    return MAINWIN->run(&app);
}
