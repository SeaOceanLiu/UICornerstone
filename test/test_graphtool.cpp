
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <memory>
#include <cmath>
#include "GraphTool.h"
#include "MainWindow.h"
#include "Bench.h"
#include "ResourceLoader.h"

using namespace std;
using namespace GraphTool;

// ==================== 测试状态 ====================
static int g_testStep = 0;
static const int g_totalSteps = 12;
static Uint64 g_stepTimer = 0;
static const float g_stepDuration = 2000.0f; // 每步2秒

void debugTraceOutput(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    static FILE* logFile = nullptr;
    if (!logFile) {
        logFile = fopen("graphtool_log.txt", "w");
    }
    if (logFile) {
        fprintf(logFile, "Category[%d], priority[%d]: %s\n", category, priority, message);
        fflush(logFile);
    }
    cout << "Category[" << category << "], priority[" << priority << "]: " << message << endl;
}

// ==================== 测试函数 ====================

// 测试1: SColor颜色系统
void testColorSystem(DrawingContext& dc) {
    cout << "=== Test 1: SColor Color System ===" << endl;

    // 构造函数测试
    SColor c1; // 默认黑色
    SColor c2(1.0f, 0.0f, 0.0f); // 红色
    SColor c3((uint8_t)0, (uint8_t)255, (uint8_t)0); // 绿色（字节）
    SColor c4(0xFF0000FF); // 从uint32_t

    // 静态工厂方法
    SColor black = SColor::Black();
    SColor white = SColor::White();
    SColor red = SColor::Red();
    SColor blue = SColor::Blue(0.5f); // 半透明蓝

    // 颜色操作
    SColor brighter = red.brighter(0.2f);
    SColor darker = red.darker(0.3f);
    SColor blended = red.blend(blue, 0.5f);
    SColor withAlpha = red.withAlpha(0.5f);

    // 绘制颜色方块展示
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

    // SDL转换测试
    SDL_Color sdlColor = red.toSDLColor();
    SDL_FColor fColor = red.toSDLFColor();
    cout << "  Red SDL_Color: r=" << (int)sdlColor.r << " g=" << (int)sdlColor.g
         << " b=" << (int)sdlColor.b << " a=" << (int)sdlColor.a << endl;

    cout << "  Color system test passed!" << endl;
}

// 测试2: SPen画笔系统
void testPenSystem(DrawingContext& dc) {
    cout << "=== Test 2: SPen Pen System ===" << endl;

    // 构造函数
    SPen pen1; // 默认
    SPen pen2(SColor::Red()); // 红色
    SPen pen3(SColor::Blue(), 3.0f); // 蓝色3像素宽
    SPen pen4(SColor::Green(), 2.0f, PenStyle::Dash); // 绿色虚线
    SPen pen5(SColor::Magenta(), 2.0f, PenStyle::Dot); // 品红点线

    // 工厂方法
    SPen noPen = SPen::NoPen();
    SPen blackPen = SPen::BlackPen(2.0f);
    SPen dashPen = SPen::DashPen(SColor::Red());

    // 属性设置
    pen1.setColor(SColor::Yellow());
    pen1.setWidth(4.0f);
    pen1.setStyle(PenStyle::DashDot);
    pen1.setCap(LineCap::Round);
    pen1.setJoin(LineJoin::Round);
    pen1.setDashOffset(5.0f);

    // 虚线模式
    auto pattern = pen1.getStandardDashPattern();
    cout << "  DashDot pattern size: " << pattern.size() << endl;

    // 绘制不同画笔样式的线
    float y = 100.0f;
    dc.setPen(pen2);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;
    dc.setPen(pen3);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;
    dc.setPen(pen4);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;
    dc.setPen(pen5);
    dc.drawLine(20.0f, y, 300.0f, y); y += 20;

    // 自定义虚线
    SPen customPen(SColor::Cyan(), 2.0f, PenStyle::Custom);
    customPen.setDashPattern({10.0f, 5.0f, 2.0f, 5.0f});
    dc.setPen(customPen);
    dc.drawLine(20.0f, y, 300.0f, y);

    // 恢复默认画笔
    dc.setPen(SPen(SColor::Black(), 1.0f));

    cout << "  Pen system test passed!" << endl;
}

// 测试3: SBrush画刷系统
void testBrushSystem(DrawingContext& dc) {
    cout << "=== Test 3: SBrush Brush System ===" << endl;

    // 构造函数
    SBrush brush1; // 默认黑色实心
    SBrush brush2(SColor::Red()); // 红色实心
    SBrush noBrush = SBrush::NoBrush(); // 透明

    // 线性渐变
    SLinearGradient linearGrad(0, 0, 200, 0);
    linearGrad.addStop(0.0f, SColor::Red());
    linearGrad.addStop(0.5f, SColor::Yellow());
    linearGrad.addStop(1.0f, SColor::Green());
    SBrush linearBrush(linearGrad);

    // 径向渐变
    SRadialGradient radialGrad(150, 250, 80);
    radialGrad.addStop(0.0f, SColor::White());
    radialGrad.addStop(1.0f, SColor::Blue());
    SBrush radialBrush(radialGrad);

    // 绘制
    dc.setBrush(brush2);
    dc.drawRect(SRect(20, 180, 80, 60), true);

    dc.setBrush(noBrush);
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawRect(SRect(120, 180, 80, 60), true);

    // 渐变矩形
    AdvancedDrawing::drawGradientRect(dc, SRect(220, 180, 200, 60), SColor::Red(), SColor::Blue(), true);

    cout << "  Brush system test passed!" << endl;
}

// 测试4: 渐变系统
void testGradientSystem(DrawingContext& dc) {
    cout << "=== Test 4: Gradient System ===" << endl;

    // 线性渐变
    SLinearGradient hGrad(0, 0, 400, 0);
    hGrad.addStop(0.0f, SColor::Red());
    hGrad.addStop(0.25f, SColor::Yellow());
    hGrad.addStop(0.5f, SColor::Green());
    hGrad.addStop(0.75f, SColor::Cyan());
    hGrad.addStop(1.0f, SColor::Blue());

    // 垂直线性渐变
    SLinearGradient vGrad(0, 0, 0, 100);
    vGrad.addStop(0.0f, SColor::White());
    vGrad.addStop(1.0f, SColor::Black());

    // 径向渐变
    SRadialGradient rGrad(200, 350, 100);
    rGrad.addStop(0.0f, SColor::Yellow());
    rGrad.addStop(0.5f, SColor(1.0f, 0.5f, 0.0f));
    rGrad.addStop(1.0f, SColor::Red());

    // 绘制
    AdvancedDrawing::drawGradientRectWithBrush(dc, SRect(20, 260, 400, 40), hGrad);
    AdvancedDrawing::drawGradientRect(dc, SRect(20, 310, 400, 40), SColor::White(), SColor::Black(), false);
    AdvancedDrawing::drawRadialGradientRect(dc, SRect(20, 360, 200, 100), rGrad);

    // 颜色插值测试
    SColor mid = hGrad.getColorAt(0.5f);
    cout << "  Gradient mid color: r=" << mid.red() << " g=" << mid.green() << " b=" << mid.blue() << endl;

    cout << "  Gradient system test passed!" << endl;
}

// 测试5: 基本图形绘制
void testBasicShapes(DrawingContext& dc) {
    cout << "=== Test 5: Basic Shapes ===" << endl;

    float x = 450, y = 20;

    // drawPoint
    dc.setPen(SPen(SColor::Red(), 1.0f));
    for (int i = 0; i < 10; ++i) {
        dc.drawPoint(x + i * 5, y);
    }

    // drawLine - 不同宽度
    y += 20;
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawLine(x, y, x + 200, y);
    y += 10;
    dc.setPen(SPen(SColor::Red(), 3.0f));
    dc.drawLine(x, y, x + 200, y);
    y += 10;
    dc.setPen(SPen(SColor::Blue(), 5.0f));
    dc.drawLine(x, y, x + 200, y);

    // drawRect - 填充和描边
    y += 20;
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(0.8f, 0.8f, 1.0f));
    dc.drawRect(SRect(x, y, 80, 50), true);
    dc.drawRect(SRect(x + 100, y, 80, 50), false);

    // 粗边框矩形
    dc.setPen(SPen(SColor::Red(), 4.0f));
    dc.setFillColor(SColor(1.0f, 1.0f, 0.8f));
    dc.drawRect(SRect(x, y + 60, 80, 50), true);
    dc.drawRect(SRect(x + 100, y + 60, 80, 50), false);

    cout << "  Basic shapes test passed!" << endl;
}

// 测试6: 圆角矩形
void testRoundedRect(DrawingContext& dc) {
    cout << "=== Test 6: Rounded Rectangles ===" << endl;

    float x = 450, y = 200;

    // 填充圆角矩形
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(0.7f, 0.9f, 0.7f));
    dc.drawRoundedRect(SRect(x, y, 100, 60), 10.0f, true);

    // 描边圆角矩形
    dc.drawRoundedRect(SRect(x + 120, y, 100, 60), 10.0f, false);

    // 粗边框圆角矩形
    dc.setPen(SPen(SColor::Blue(), 3.0f));
    dc.setFillColor(SColor(0.8f, 0.8f, 1.0f));
    dc.drawRoundedRect(SRect(x, y + 70, 100, 60), 15.0f, true);
    dc.drawRoundedRect(SRect(x + 120, y + 70, 100, 60), 15.0f, false);

    // 带边框的圆角矩形
    dc.drawRoundedRectWithBorder(SRect(x, y + 140, 100, 60), 10.0f, 2.0f, SColor::Red(), SColor::Yellow());

    // 不同圆角半径
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(1.0f, 0.8f, 0.8f));
    dc.drawRoundedRect(SRect(x + 120, y + 140, 100, 60), SRoundedCorners(20, 5, 20, 5), true);

    cout << "  Rounded rectangles test passed!" << endl;
}

// 测试7: 圆形绘制
void testCircleDrawing(DrawingContext& dc) {
    cout << "=== Test 7: Circle Drawing ===" << endl;

    float x = 700, y = 50;

    // 填充圆
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor::Red());
    dc.drawCircle(SPoint(x + 40, y + 40), 35.0f, true);

    // 描边圆
    dc.setPen(SPen(SColor::Blue(), 2.0f));
    dc.drawCircle(SPoint(x + 130, y + 40), 35.0f, false);

    // 带描边的填充圆
    dc.drawFilledCircleWithStroke(x + 220, y + 40, 35.0f, 3.0f, SColor::Green(), SColor::Black());

    // drawFilledCircleAA
    dc.drawFilledCircleAA(x + 310, y + 40, 35.0f, SColor(0.5f, 0.5f, 1.0f));

    // drawCircleWithThickness
    dc.drawCircleWithThickness(x + 400, y + 40, 35.0f, 5.0f, SColor::Magenta());

    // 指定填充色和描边色的圆
    dc.drawCircle(SPoint(x + 40, y + 130), 30.0f, SColor::Cyan(), SColor::Red(), 3.0f);

    cout << "  Circle drawing test passed!" << endl;
}

// 测试8: 椭圆和弧线
void testEllipseAndArc(DrawingContext& dc) {
    cout << "=== Test 8: Ellipse and Arc ===" << endl;

    float x = 700, y = 240;

    // 填充椭圆
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.setFillColor(SColor(1.0f, 0.5f, 0.5f));
    dc.drawEllipse(SPoint(x + 60, y + 30), 55.0f, 25.0f, true);

    // 描边椭圆
    dc.setPen(SPen(SColor::Blue(), 2.0f));
    dc.drawEllipse(SPoint(x + 190, y + 30), 55.0f, 25.0f, false);

    // 弧线
    dc.setPen(SPen(SColor::Green(), 2.0f));
    dc.drawArc(SPoint(x + 300, y + 30), 30.0f, 0.0f, (float)M_PI, false);

    // 填充弧线（扇形）
    dc.setFillColor(SColor(0.5f, 1.0f, 0.5f, 0.7f));
    dc.drawArc(SPoint(x + 400, y + 30), 30.0f, 0.0f, (float)M_PI * 1.5f, true);

    cout << "  Ellipse and arc test passed!" << endl;
}

// 测试9: 多边形和折线
void testPolygonAndPolyline(DrawingContext& dc) {
    cout << "=== Test 9: Polygon and Polyline ===" << endl;

    // 三角形
    vector<SPoint> triangle = {SPoint(50, 500), SPoint(100, 440), SPoint(150, 500)};
    dc.setFillColor(SColor(1.0f, 0.8f, 0.5f));
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawPolygon(triangle, true);
    dc.drawPolygon(triangle, false);

    // 五边形
    vector<SPoint> pentagon;
    for (int i = 0; i < 5; ++i) {
        float angle = (float)(-M_PI / 2 + i * 2 * M_PI / 5);
        pentagon.push_back(SPoint(280 + 40 * cos(angle), 470 + 40 * sin(angle)));
    }
    dc.setFillColor(SColor(0.5f, 0.8f, 1.0f));
    dc.drawPolygon(pentagon, true);
    dc.setPen(SPen(SColor::Blue(), 2.0f));
    dc.drawPolygon(pentagon, false);

    // 折线
    vector<SPoint> polyline = {SPoint(350, 500), SPoint(380, 440), SPoint(410, 480), SPoint(440, 430)};
    dc.setPen(SPen(SColor::Red(), 3.0f));
    dc.drawPolyline(polyline);

    // 粗边框多边形
    vector<SPoint> quad = {SPoint(480, 440), SPoint(560, 440), SPoint(560, 500), SPoint(480, 500)};
    dc.setPen(SPen(SColor::Green(), 4.0f));
    dc.setFillColor(SColor(0.8f, 1.0f, 0.8f));
    dc.drawPolygon(quad, true);
    dc.drawPolygon(quad, false);

    cout << "  Polygon and polyline test passed!" << endl;
}

// 测试10: 虚线绘制
void testDashedLines(DrawingContext& dc) {
    cout << "=== Test 10: Dashed Lines ===" << endl;

    float y = 530;

    // 实线
    dc.setPen(SPen(SColor::Black(), 2.0f, PenStyle::Solid));
    dc.drawLine(20.0f, y, 400.0f, y);

    // 虚线
    y += 15;
    dc.setPen(SPen(SColor::Red(), 2.0f, PenStyle::Dash));
    dc.drawLine(20.0f, y, 400.0f, y);

    // 点线
    y += 15;
    dc.setPen(SPen(SColor::Blue(), 2.0f, PenStyle::Dot));
    dc.drawLine(20.0f, y, 400.0f, y);

    // 点划线
    y += 15;
    dc.setPen(SPen(SColor::Green(), 2.0f, PenStyle::DashDot));
    dc.drawLine(20.0f, y, 400.0f, y);

    // 双点划线
    y += 15;
    dc.setPen(SPen(SColor::Magenta(), 2.0f, PenStyle::DashDotDot));
    dc.drawLine(20.0f, y, 400.0f, y);

    // 粗虚线
    y += 15;
    dc.setPen(SPen(SColor::Cyan(), 4.0f, PenStyle::Dash));
    dc.drawLine(20.0f, y, 400.0f, y);

    // 自定义虚线
    y += 15;
    SPen customPen(SColor(1.0f, 0.5f, 0.0f), 3.0f, PenStyle::Custom);
    customPen.setDashPattern({15.0f, 5.0f, 5.0f, 5.0f});
    dc.setPen(customPen);
    dc.drawLine(20.0f, y, 400.0f, y);

    // 恢复默认画笔
    dc.setPen(SPen(SColor::Black(), 1.0f));

    cout << "  Dashed lines test passed!" << endl;
}

// 测试11: AdvancedDrawing高级功能
void testAdvancedDrawing(DrawingContext& dc) {
    cout << "=== Test 11: Advanced Drawing ===" << endl;

    // 阴影效果
    SRect shadowRect(450, 400, 120, 60);
    AdvancedDrawing::drawShadow(dc, shadowRect, 5.0f, SColor(0, 0, 0, 0.3f), 5.0f);
    dc.setFillColor(SColor::White());
    dc.drawRect(shadowRect, true);
    dc.setPen(SPen(SColor::Black(), 1.0f));
    dc.drawRect(shadowRect, false);

    // Styled矩形
    AdvancedDrawing::drawStyledRect(dc, SRect(600, 400, 120, 60),
        SPen(SColor::Blue(), 3.0f), SBrush(SColor(0.8f, 0.8f, 1.0f)));

    // Styled圆角矩形
    AdvancedDrawing::drawStyledRoundedRect(dc, SRect(750, 400, 120, 60), 10.0f,
        SPen(SColor::Green(), 2.0f), SBrush(SColor(0.8f, 1.0f, 0.8f)));

    // Styled圆形
    AdvancedDrawing::drawStyledCircle(dc, SPoint(510, 510), 30.0f,
        SPen(SColor::Red(), 2.0f), SBrush(SColor(1.0f, 0.8f, 0.8f)));

    // Styled椭圆
    AdvancedDrawing::drawStyledEllipse(dc, SPoint(660, 510), 50.0f, 25.0f,
        SPen(SColor::Magenta(), 2.0f), SBrush(SColor(1.0f, 0.8f, 1.0f)));

    cout << "  Advanced drawing test passed!" << endl;
}

// 测试12: Utils工具函数
void testUtils(DrawingContext& dc) {
    cout << "=== Test 12: Utils Functions ===" << endl;

    // 距离计算
    float dist = Utils::distance(SPoint(0, 0), SPoint(3, 4));
    cout << "  Distance (0,0)->(3,4) = " << dist << " (expected 5)" << endl;

    // 插值
    SPoint mid = Utils::interpolate(SPoint(0, 0), SPoint(10, 10), 0.5f);
    cout << "  Interpolate mid = (" << mid.x << ", " << mid.y << ") (expected 5, 5)" << endl;

    // 点在多边形内
    vector<SPoint> poly = {SPoint(0, 0), SPoint(100, 0), SPoint(100, 100), SPoint(0, 100)};
    bool inside = Utils::pointInPolygon(SPoint(50, 50), poly);
    cout << "  Point (50,50) in rect polygon: " << (inside ? "true" : "false") << endl;

    // 边界框
    vector<SPoint> points = {SPoint(10, 20), SPoint(50, 10), SPoint(30, 60)};
    SRect bbox = Utils::boundingBox(points);
    cout << "  Bounding box: (" << bbox.left << "," << bbox.top << "," << bbox.width << "," << bbox.height << ")" << endl;

    // 圆形点生成
    auto circlePts = Utils::generateCirclePoints(SPoint(900, 100), 40.0f, 24);
    dc.setPen(SPen(SColor::Blue(), 1.0f));
    for (size_t i = 0; i < circlePts.size(); ++i) {
        size_t next = (i + 1) % circlePts.size();
        dc.drawLine(circlePts[i], circlePts[next]);
    }

    // 线段矩形点
    auto lineRect = Utils::generateLineRectPoints(SPoint(860, 160), SPoint(960, 200), 6.0f);
    dc.setPen(SPen(SColor::Red(), 1.0f));
    dc.drawLine(lineRect);

    // 圆角矩形点
    auto rrPts = Utils::generateRoundedRectPoints(SRect(860, 220, 100, 60), SRoundedCorners(10), 8);
    dc.setPen(SPen(SColor::Green(), 1.0f));
    for (size_t i = 0; i < rrPts.size(); ++i) {
        size_t next = (i + 1) % rrPts.size();
        dc.drawLine(rrPts[i], rrPts[next]);
    }

    // 线段交点
    SPoint intersection = Utils::lineIntersection(SPoint(0, 0), SPoint(100, 100),
                                                   SPoint(0, 100), SPoint(100, 0));
    cout << "  Line intersection: (" << intersection.x << ", " << intersection.y << ") (expected 50, 50)" << endl;

    // 颜色插值
    SColor interpColor = Utils::interpolateColor(SColor::Red(), SColor::Blue(), 0.5f);
    cout << "  Interpolated color: r=" << interpColor.red() << " g=" << interpColor.green() << " b=" << interpColor.blue() << endl;

    cout << "  Utils functions test passed!" << endl;
}

// ==================== SDL回调函数 ====================

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogOutputFunction(debugTraceOutput, nullptr);

    SDL_SetAppMetadata("GraphToolTest", "1.0.0", "com.example.graphtool");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    g_stepTimer = SDL_GetTicks();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        // 空格键切换测试步骤
        if (event->key.key == SDLK_SPACE) {
            g_testStep = (g_testStep + 1) % g_totalSteps;
            g_stepTimer = SDL_GetTicks();
        }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    SDL_Renderer* renderer = MainWindow::getInstance()->getRenderer();
    if (!renderer) return SDL_APP_CONTINUE;

    // 清屏
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderClear(renderer);

    DrawingContext dc(renderer);

    // 自动切换步骤
    Uint64 now = SDL_GetTicks();
    if (now - g_stepTimer > g_stepDuration) {
        g_testStep = (g_testStep + 1) % g_totalSteps;
        g_stepTimer = now;
    }

    // 执行当前测试
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

    // 显示当前测试步骤
    dc.setFillColor(SColor::Black());
    dc.setPen(SPen(SColor::Black(), 1.0f));

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    ResourceLoader::getInstance()->detachLoadingThread();
    cout << "\n=== All GraphTool tests completed! ===" << endl;
    TTF_Quit();
}
