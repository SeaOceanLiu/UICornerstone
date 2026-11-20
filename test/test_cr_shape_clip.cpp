// CR20261117001：Shape 图元绘制裁剪到控件 rect
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

#include "Bench.h"
#include "Panel.h"
#include "PlatformUtils.h"
#include "Shape.h"
#include "TestInstance.h"
#include "TestUtils.h"
#include "UICornerstoneAPI.h"

using namespace std;

namespace {

int g_pass = 0;
int g_fail = 0;
#define CHECK(cond, msg) \
    do { if (cond) { ++g_pass; printf("OK   %s\n", msg); } \
         else { ++g_fail; printf("FAIL %s\n", msg); } } while (0)

shared_ptr<Shape> g_clip, g_noclip, g_shadow, g_poly, g_scaled;
shared_ptr<Panel> g_parent;
shared_ptr<Shape> g_child;
bool g_done = false;
int g_frame = 0;

uint32_t pixelAt(int x, int y) {
    BENCH->getRenderDevice()->flush();
    SRect r(static_cast<float>(x), static_cast<float>(y), 1.0f, 1.0f);
    uint32_t p = 0;
    BENCH->getRenderDevice()->readPixels(&p, r);
    return p;   // RGBA8888：R<<24|G<<16|B<<8|A
}
// 读回值按 RGBA8888 打包；直接比较整像素，避免后端字节序差异。
bool isWhite(uint32_t p) { return p == 0xFFFFFFFFu; }
bool isRed(uint32_t p)   { return p == 0xFF0000FFu; }
bool isShadowPixel(uint32_t p) { return p != 0xFFFFFFFFu && p != 0xFF0000FFu; }

shared_ptr<Shape> makeOverflowShape(float x, float y, bool clip, float scale = 1.0f) {
    auto s = make_shared<Shape>(nullptr, SRect(x, y, 100, 100), scale, scale);
    s->addPrimitive(ShapeType::FilledRect, SRect(-40, -40, 180, 180));
    s->setPrimitiveFill(0, SColor(255, 0, 0, 255));
    s->setBoolProperty(PropertyNames::kClipShape, clip ? 1 : 0);
    BENCH->addControl(s);
    return s;
}

void runChecks() {
    // C1：单/多图元越界默认裁剪
    CHECK(isWhite(pixelAt(70, 150)), "C1 clip=true: left overflow hidden");
    CHECK(isRed(pixelAt(150, 150)), "C1 clip=true: inside primitive drawn");
    CHECK(isWhite(pixelAt(230, 150)), "C1 clip=true: right overflow hidden");

    // C5：clip=false 保留溢出
    CHECK(isRed(pixelAt(270, 150)), "C5 clip=false: left overflow drawn");
    CHECK(isRed(pixelAt(430, 150)), "C5 clip=false: right overflow drawn");

    // C6：box-shadow 在裁剪外可见，图元不溢出
{ uint32_t pp = pixelAt(495, 150); printf("DBG shadow %08X%c", pp, 10); }
    CHECK(isShadowPixel(pixelAt(495, 150)), "C6 box-shadow visible outside rect");
    CHECK(!isRed(pixelAt(495, 150)), "C6 primitive clipped under shadow");

    // C3：polygon points 越界裁剪
    CHECK(isRed(pixelAt(330, 280)), "C3 polygon inside drawn");
    CHECK(isWhite(pixelAt(290, 340)), "C3 polygon overflow clipped");

    // C7：父 Panel clip-children 交集
    CHECK(isRed(pixelAt(760, 160)), "C7 child shape inside parent clip");
    CHECK(isWhite(pixelAt(790, 160)), "C7 parent clip wins over shape overflow");

    // C8：缩放后裁剪边界随 getDrawRect
    CHECK(isRed(pixelAt(150, 280)), "C8 scaled shape inside");
    CHECK(isWhite(pixelAt(90, 300)), "C8 scaled clip left boundary");
    CHECK(isWhite(pixelAt(260, 300)), "C8 scaled clip right boundary");

    // C11/C9：C ABI 读写 clip + JSON 默认/显式 false
    UIControlHandle hd = UICornerstone_FindControl(g_uiInstance, "clipDefault");
    UIControlHandle ho = UICornerstone_FindControl(g_uiInstance, "clipOff");
    CHECK(hd && ho, "C9 JSON shapes found");
    if (hd && ho) {
        int v = -1;
        CHECK(UICornerstone_GetBool(g_uiInstance, hd, "clip", &v) == 1 && v == 1, "C9 JSON default clip=true");
        v = -1;
        CHECK(UICornerstone_GetBool(g_uiInstance, ho, "clip", &v) == 1 && v == 0, "C9 JSON clip=false");
        CHECK(UICornerstone_SetBool(g_uiInstance, hd, "clip", 0) == 1, "C11 C ABI SetBool clip");
        v = -1; UICornerstone_GetBool(g_uiInstance, hd, "clip", &v);
        CHECK(v == 0, "C11 C ABI GetBool clip");
        UICornerstone_SetBool(g_uiInstance, hd, "clip", 1);
    }

    // C12：clip + box-shadow + primitives 顺序已在 C6 覆盖

    printf("---- CR20261117001 shape clip: pass=%d fail=%d ----\n", g_pass, g_fail);
}

} // namespace

class ShapeClipApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_cr_shape_clip");
        BENCH->setOnInitial([](shared_ptr<Bench>) {
            // 先加载 JSON 样本（默认/显式 false）
            UICornerstone_LoadLayoutFromFile(g_uiInstance,
                (Platform::GetBasePath() + "layouts/cr_shape_clip.json").c_str());

            g_clip   = makeOverflowShape(100, 100, true);
            g_noclip = makeOverflowShape(300, 100, false);
            g_shadow = makeOverflowShape(500, 100, true);
            g_shadow->setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 128));
            g_shadow->setFloatProperty(PropertyNames::kBoxShadowRadius, 10.0f);

            g_poly = make_shared<Shape>(nullptr, SRect(300, 250, 80, 80));
            g_poly->setShape(ShapeType::Polygon);
            g_poly->setFillColor(SColor(255, 0, 0, 255));
            g_poly->setPoints({ {-20, -20}, {120, -20}, {50, 120} });
            BENCH->addControl(g_poly);

            g_scaled = makeOverflowShape(100, 250, true, 1.5f);

            g_parent = make_shared<Panel>(nullptr, SRect(700, 100, 80, 80));
            g_parent->setTransparent(true);
            g_parent->setClipChildren(true);
            BENCH->addControl(g_parent);
            g_child = make_shared<Shape>(nullptr, SRect(20, 20, 100, 100));
            g_child->addPrimitive(ShapeType::FilledRect, SRect(0, 0, 100, 100));
            g_child->setPrimitiveFill(0, SColor(255, 0, 0, 255));
            g_parent->addControl(g_child);
        });
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
        ++g_frame;
    }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(255, 255, 255, 255));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
        if (g_frame == 4 && !g_done) { g_done = true; runChecks(); }
    }
    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    return TestRunMain<ShapeClipApp>(argc, argv);
}
