#include <iostream>
#include <memory>
#include "Label.h"
#include "LayoutParser.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "EditBox.h"
#include "TextArea.h"
#include "TestUtils.h"
#include "TestInstance.h"

using namespace std;

shared_ptr<Label> g_label1;
shared_ptr<Label> g_label2;
shared_ptr<Label> g_label3;
shared_ptr<Label> g_label4;
shared_ptr<Label> g_label5;
shared_ptr<Label> g_label6;
shared_ptr<Label> g_label7;
shared_ptr<Label> g_label8;
shared_ptr<Label> g_label9;
shared_ptr<Label> g_label10;
shared_ptr<Label> g_label11;
shared_ptr<Label> g_label12;
shared_ptr<Label> g_label13;
shared_ptr<Label> g_label14;
shared_ptr<Label> g_label15;
shared_ptr<Label> g_label16;
shared_ptr<Label> g_label17;
shared_ptr<Label> g_label18;
shared_ptr<Label> g_label19;

void testBenchInitialize(shared_ptr<Bench>) {
    TestUtil::log("testLabelInitialize");

    StateColor redBorder(StateColor::Type::Border);
    redBorder.setNormal({255, 0, 0, 255});

    // Row 1: Single line alignment (y=30, height=30)
    g_label1 = LabelBuilder(nullptr, SRect(20, 30, 130, 30))
        .setCaption("L1: Left")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        .setId(1)
        .build();
    BENCH->addControl(g_label1);

    g_label2 = LabelBuilder(nullptr, SRect(160, 30, 130, 30))
        .setCaption("L2: Right")
        .setAlignmentMode(AlignmentMode::AM_TOP_RIGHT)
        .setBorderStateColor(redBorder)
        .setId(2)
        .build();
    BENCH->addControl(g_label2);

    g_label3 = LabelBuilder(nullptr, SRect(300, 30, 130, 30))
        .setCaption("L3: Center")
        .setAlignmentMode(AlignmentMode::AM_TOP_CENTER)
        .setBorderStateColor(redBorder)
        .setId(3)
        .build();
    BENCH->addControl(g_label3);

    // Row 2: Single line truncate (y=80, height=25)
    g_label4 = LabelBuilder(nullptr, SRect(20, 80, 70, 25))
        .setCaption("L4: long text 70px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(4)
        .build();
    BENCH->addControl(g_label4);

    g_label5 = LabelBuilder(nullptr, SRect(100, 80, 40, 25))
        .setCaption("L5: long t 40px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(5)
        .build();
    BENCH->addControl(g_label5);

    g_label6 = LabelBuilder(nullptr, SRect(150, 80, 35, 25))
        .setCaption("L6: long 35px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(6)
        .build();
    BENCH->addControl(g_label6);

    g_label7 = LabelBuilder(nullptr, SRect(195, 80, 30, 25))
        .setCaption("L7: long 30px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(7)
        .build();
    BENCH->addControl(g_label7);

    g_label8 = LabelBuilder(nullptr, SRect(235, 80, 20, 25))
        .setCaption("L8: lo 20px")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(8)
        .build();
    BENCH->addControl(g_label8);

    // Row 3: Multi-line (y=125, height=50)
    g_label9 = LabelBuilder(nullptr, SRect(20, 125, 110, 60))
        .setCaption(u8"L9: Line1\nLine2\nLine3")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(9)
        .build();
    BENCH->addControl(g_label9);

    g_label10 = LabelBuilder(nullptr, SRect(140, 125, 90, 95))
        .setEnableExpand(false)
        .setCaption(u8"L10: L1\nL2\nL3\nL4")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        .setId(10)
        .build();
    BENCH->addControl(g_label10);

    // Row 4: 2x scaled Chinese top alignment (y=200, height=70 -> 140 with scale)
    g_label11 = LabelBuilder(nullptr, SRect(20, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L11: Top左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(11)
        .build();
    BENCH->addControl(g_label11);

    g_label12 = LabelBuilder(nullptr, SRect(450, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L12: Top中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(12)
        .build();
    BENCH->addControl(g_label12);

    g_label13 = LabelBuilder(nullptr, SRect(880, 250, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L13: Top右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_TOP_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(13)
        .build();
    BENCH->addControl(g_label13);

    // Row 5: 2x scaled Chinese middle alignment (y=400, after 140+60 gap)
    g_label14 = LabelBuilder(nullptr, SRect(20, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L14: Mid左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(14)
        .build();
    BENCH->addControl(g_label14);

    g_label15 = LabelBuilder(nullptr, SRect(450, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L15: Mid中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(15)
        .build();
    BENCH->addControl(g_label15);

    g_label16 = LabelBuilder(nullptr, SRect(880, 450, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L16: Mid右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_MID_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(16)
        .build();
    BENCH->addControl(g_label16);

    // Row 6: 2x scaled Chinese bottom alignment (y=600, after 140+60 gap)
    g_label17 = LabelBuilder(nullptr, SRect(20, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L17: Bot左\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_LEFT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(17)
        .build();
    BENCH->addControl(g_label17);

    g_label18 = LabelBuilder(nullptr, SRect(450, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L18: Bot中\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_CENTER)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(18)
        .build();
    BENCH->addControl(g_label18);

    g_label19 = LabelBuilder(nullptr, SRect(880, 650, 200, 90), 2.0f, 2.0f)
        .setCaption(u8"L19: Bot右\n第一行\n第二行\n第三行")
        .setAlignmentMode(AlignmentMode::AM_BOTTOM_RIGHT)
        .setEnableExpand(false)
        .setBorderStateColor(redBorder)
        .setId(19)
        .build();
    BENCH->addControl(g_label19);

    // ── JSON 字体声明 + 父链继承（v1.1.1）──
    {
        static const char* FONT_JSON = R"({
  "version": "1.0",
  "controls": [{
    "type": "panel", "id": "fontParent", "rect": {"x": 0, "y": 700, "w": 600, "h": 120},
    "font": {"size": 20, "name": "harmonyos-sans-sc-regular"},
    "children": [
      { "type": "label", "id": "fontInherit", "caption": "inherit",
        "rect": {"x": 0, "y": 0, "w": 200, "h": 30} },
      { "type": "label", "id": "fontOverride", "caption": "override",
        "rect": {"x": 200, "y": 0, "w": 200, "h": 30},
        "font": {"size": 24} },
      { "type": "label", "id": "fontDirect", "caption": "direct",
        "rect": {"x": 0, "y": 40, "w": 200, "h": 30}, "font-size": 16 }
    ]
  }]
})";
#define FCHECK(cond, msg) do { if (cond) {} else { printf("FONT-CHK-FAIL: %s\n", msg); } } while (0)
        LayoutParser parser;
        auto froot = parser.parseLayout(FONT_JSON);
        FCHECK(froot != nullptr, "字体 JSON 解析");
        if (!froot) { printf("FONT-JSON FAILED\n"); }
        else {
            BENCH->addControl(froot);
            auto inherit = parser.findControlById("fontInherit");
            auto override_ = parser.findControlById("fontOverride");
            auto direct = parser.findControlById("fontDirect");
            if (inherit && override_ && direct) {
                auto li = dynamic_pointer_cast<Label>(inherit);
                auto lo = dynamic_pointer_cast<Label>(override_);
                auto ld = dynamic_pointer_cast<Label>(direct);
                if (li && lo && ld) {
                    FCHECK(li->getFontSize() == 20, "JSON 继承: 子无 font → 父 20");
                    FCHECK(lo->getFontSize() == 24, "JSON 覆盖: font.size=24 覆盖父");
                    FCHECK(ld->getFontSize() == 16, "JSON 便捷键: fontSize=16");
                    // getFontSize 语义已断言，字体名不做公开 getter 断言（Label 无 getFontName）
                     printf("FONT-JSON OK\n");
                } else {
                    printf("FONT-JSON FAILED: cast\n");
                }
            } else {
                printf("FONT-JSON FAILED: find\n");
            }
        }
    }

    // ── 字体样式断言（锁住 SetFontStyle 修复：属性回环 + 渲染无崩溃）──
    {
        static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { if (cond) { ++g_pass; TestUtil::log("OK   %s", msg); } \
                                  else { ++g_fail; TestUtil::log("FAIL %s", msg); } } while (0)

        // 属性系统回环：setEnumProperty("font-style","bold") 返回 1 且 get 读回 "bold"
        auto probe = LabelBuilder(nullptr, SRect(120, 780, 200, 40)).setCaption(u8"font-style").build();
        probe->create();
        BENCH->addControl(probe);
        CHECK(probe->setEnumProperty(PropertyNames::kFontStyle, PropertyNames::kFontStyleBold) == 1,
              "setEnumProperty(font-style, bold) accepted");
        const char* got = nullptr;
        CHECK(probe->getEnumProperty(PropertyNames::kFontStyle, got) == 1 && got && strcmp(got, PropertyNames::kFontStyleBold) == 0,
              "getEnumProperty(font-style) round-trip = bold");

        // 非法样式值拒绝
        CHECK(probe->setEnumProperty(PropertyNames::kFontStyle, "superbold") == 0,
              "setEnumProperty(font-style, superbold) rejected");

        // SetFontStyle 组合样式（3 = 粗体|斜体）加载后渲染不崩溃（粗体 Label 已在矩阵）
        probe->SetFontStyle(3);
        CHECK(probe->GetFontStyle() == 3, "SetFontStyle(3) stored");

        // 4 个 Bool 属性组合：font-bold + font-italic → 位 3（属性系统组合通道）
        probe->setBoolProperty(PropertyNames::kFontBold, 1);
        probe->setBoolProperty(PropertyNames::kFontItalic, 1);
        CHECK(probe->GetFontStyle() == 3, "font-bold + font-italic (Bool) -> style 3");
        int bitOut = 0;
        CHECK(probe->getBoolProperty(PropertyNames::kFontBold, bitOut) == 1 && bitOut == 1, "getBool(font-bold) readback");
        CHECK(probe->getBoolProperty(PropertyNames::kFontItalic, bitOut) == 1 && bitOut == 1, "getBool(font-italic) readback");
        // 关闭斜体 → 仅粗体（位 1）
        probe->setBoolProperty(PropertyNames::kFontItalic, 0);
        CHECK(probe->GetFontStyle() == 1, "clear font-italic -> style 1");

        // 可视化：粗体 Label（矩阵中区分于普通文本）
        auto bold = LabelBuilder(nullptr, SRect(560, 780, 240, 40))
            .setCaption(u8"L20: 粗体 Bold")
            .setId(20)
            .build();
        bold->setEnumProperty(PropertyNames::kFontStyle, PropertyNames::kFontStyleBold);
        BENCH->addControl(bold);

        // JSON font.style 数组组合（["bold","italic"] → 位 3 = 粗斜体）
        {
            const string styleJson = R"({
              "controls":[{
                "type":"label","id":"boldItalic","rect":[30,780,200,30],
                "font":{"name":"HarmonyOS_Sans_SC_Regular","size":14,"style":["bold","italic"]}
              }]
            })";
#define FCHECK(cond, msg) do { if (cond) {} else { printf("FONT-CHK-FAIL: %s\n", msg); } } while (0)
        LayoutParser parser;
            auto root = parser.parseLayout(styleJson);
            auto* ci = root ? dynamic_cast<ControlImpl*>(root.get()) : nullptr;
            auto clabel = ci ? std::dynamic_pointer_cast<Label>(ci->getThis()) : nullptr;
            CHECK(clabel != nullptr, "JSON font.style array label parsed");
            if (clabel) CHECK(clabel->GetFontStyle() == 3, "font.style [bold,italic] -> style 3 (bold|italic)");

            const string singleJson = R"({
              "controls":[{
                "type":"label","id":"lblItalic","rect":[60,780,200,30],
                "font":{"name":"HarmonyOS_Sans_SC_Regular","size":14,"style":"italic"}
              }]
            })";
            auto root2 = parser.parseLayout(singleJson);
            auto* ci2 = root2 ? dynamic_cast<ControlImpl*>(root2.get()) : nullptr;
            auto clabel2 = ci2 ? std::dynamic_pointer_cast<Label>(ci2->getThis()) : nullptr;
            if (clabel2) CHECK(clabel2->GetFontStyle() == 2, "font.style \"italic\" (string) -> style 2");
        }

        // 默认对齐：文档/schema 声明 Label 默认 mid-left（垂直中线+水平左），
        // 与右侧 EditBox/NUD 文字中线对齐（v1.1.1 修复构造默认 AM_TOP_LEFT → AM_MID_LEFT）
        auto defLabel = LabelBuilder(nullptr, SRect(820, 780, 120, 30)).setCaption(u8"默认对齐").build();
        CHECK(defLabel->getAlignmentMode() == AlignmentMode::AM_MID_LEFT,
              "default alignment = mid-left (schema/document agreement)");

        TestUtil::log("---- font-style assertions: pass=%d fail=%d ----", g_pass, g_fail);
#undef CHECK
    }

    TestUtil::log("Label test controls created");
}

class LabelApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_label");
        BENCH->setOnInitial(testBenchInitialize);
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
        TestUtil::log("Application quit");
    }
};

int main(int argc, char* argv[]) {
    return TestRunMain<LabelApp>(argc, argv);
}
