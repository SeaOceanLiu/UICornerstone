// CR20261116001：Button / CheckBox caption-label 内嵌 Label 契约
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

#include "Button.h"
#include "CheckBox.h"
#include "Label.h"
#include "PlatformUtils.h"
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

bool nearColor(const SColor& c, int r, int g, int b, int a, int eps = 2) {
    return std::abs((int)(c.red() * 255) - r) <= eps &&
           std::abs((int)(c.green() * 255) - g) <= eps &&
           std::abs((int)(c.blue() * 255) - b) <= eps &&
           std::abs((int)(c.alpha() * 255) - a) <= eps;
}

void runChecks() {
    // E1：Button 全字段
    auto* btn = dynamic_cast<Button*>(static_cast<Control*>(UICornerstone_FindControl(g_uiInstance, "btnEmb")));
    CHECK(btn != nullptr, "E1 Button found");
    if (btn) {
        auto cap = btn->getCaptionLabel();
        CHECK(cap != nullptr, "E1 Button caption label");
        if (cap) {
            CHECK(cap->getCaption() == "InnerBtn", "E1 caption inner wins");
            CHECK(cap->getFontName() == FontName::MapleMono_NF_CN_Regular, "E1 font.name");
            CHECK(cap->getFontSize() == 20, "E1 font.size");
            CHECK(cap->GetFontStyle() != 0, "E1 font.style");
            CHECK(cap->getAlignmentMode() == AlignmentMode::AM_CENTER, "E1 alignment");
            CHECK(cap->isShadowEnabled(), "E1 shadow.enabled");
            CHECK(std::abs(cap->getShadowOffset().x - 3.0f) < 0.01f, "E1 shadow.offset.x");
            CHECK(std::abs(cap->getShadowOffset().y - 4.0f) < 0.01f, "E1 shadow.offset.y");
            float blur = 0; cap->getFloatProperty(PropertyNames::kTextShadowBlur, blur);
            CHECK(std::abs(blur - 9.0f) < 0.01f, "E1 shadow.blur -> text-shadow-blur");
            CHECK(cap->getLineHeight() == 24, "E1 line-height");
            CHECK(std::abs(cap->getLineSpacingRatio() - 1.4f) < 0.01f, "E1 line-spacing-ratio");
            SColor c;
            CHECK(cap->getColorProperty(PropertyNames::kText, c) && nearColor(c, 255, 0, 0, 255), "E1 colors.text");
            CHECK(cap->getColorProperty(PropertyNames::kTextShadow, c) && nearColor(c, 0, 255, 0, 255), "E1 colors.text-shadow");
        }
    }

    // E2：CheckBox 全字段
    auto* cb = dynamic_cast<CheckBox*>(static_cast<Control*>(UICornerstone_FindControl(g_uiInstance, "cbEmb")));
    CHECK(cb != nullptr, "E2 CheckBox found");
    if (cb) {
        auto cap = cb->getCaption();
        CHECK(cap != nullptr, "E2 CheckBox caption label");
        if (cap) {
            CHECK(cap->getCaption() == "InnerCb", "E2 caption inner wins");
            CHECK(cap->getFontName() == FontName::MapleMono_NF_CN_Regular, "E2 font.name");
            CHECK(cap->getFontSize() == 18, "E2 font.size");
            CHECK(cap->GetFontStyle() != 0, "E2 font.style");
            CHECK(cap->getAlignmentMode() == AlignmentMode::AM_CENTER, "E2 alignment (not overridden by layout)");
            CHECK(cap->isShadowEnabled(), "E2 shadow.enabled");
            CHECK(std::abs(cap->getShadowOffset().x - 2.0f) < 0.01f, "E2 shadow.offset.x");
            float blur = 0; cap->getFloatProperty(PropertyNames::kTextShadowBlur, blur);
            CHECK(std::abs(blur - 8.0f) < 0.01f, "E2 shadow.blur");
            CHECK(cap->getLineHeight() == 22, "E2 line-height");
            CHECK(std::abs(cap->getLineSpacingRatio() - 1.2f) < 0.01f, "E2 line-spacing-ratio");
            SColor c;
            CHECK(cap->getColorProperty(PropertyNames::kText, c) && nearColor(c, 0, 0, 255, 255), "E2 colors.text");
            CHECK(cap->getColorProperty(PropertyNames::kTextShadow, c) && nearColor(c, 255, 0, 255, 255), "E2 colors.text-shadow");
        }
        // E3/E12：recreate（setRect）后字段保持
        cb->setRect(SRect(220, 20, 240, 44));
        cap = cb->getCaption();
        CHECK(cap != nullptr, "E3 caption after recreate");
        if (cap) {
            CHECK(cap->getFontName() == FontName::MapleMono_NF_CN_Regular, "E12 font.name survives recreate");
            CHECK(cap->GetFontStyle() != 0, "E12 font.style survives recreate");
            CHECK(cap->getAlignmentMode() == AlignmentMode::AM_CENTER, "E12 alignment survives recreate");
            CHECK(cap->getLineHeight() == 22, "E3 line-height survives recreate");
            float blur = 0; cap->getFloatProperty(PropertyNames::kTextShadowBlur, blur);
            CHECK(std::abs(blur - 8.0f) < 0.01f, "E3 blur survives recreate");
        }
        // E4：GetPtr 直写 + recreate
        if (cap) {
            cap->setAlignmentMode(AlignmentMode::AM_MID_RIGHT);
            cap->setLineHeight(30);
            cap->setFloatProperty(PropertyNames::kTextShadowBlur, 12.0f);
            cb->setRect(SRect(220, 20, 250, 46));
            cap = cb->getCaption();
            CHECK(cap != nullptr, "E14 caption after direct write recreate");
            if (cap) {
                CHECK(cap->getAlignmentMode() == AlignmentMode::AM_MID_RIGHT, "E14 direct align survives recreate");
                CHECK(cap->getLineHeight() == 30, "E14 direct line-height survives recreate");
                float blur = 0; cap->getFloatProperty(PropertyNames::kTextShadowBlur, blur);
                CHECK(std::abs(blur - 12.0f) < 0.01f, "E14 direct blur survives recreate");
            }
            // E15：GetPtr 直写 align（属性通道）立即读回 + recreate 保持
            cap = cb->getCaption();
            if (cap) {
                CHECK(cap->setEnumProperty(PropertyNames::kAlign, "bottom-right") == 1, "E15 SetEnum align accepted");
                CHECK(cap->getAlignmentMode() == AlignmentMode::AM_BOTTOM_RIGHT, "E15 align immediate readback");
                cb->setRect(SRect(220, 20, 255, 48));
                cap = cb->getCaption();
                CHECK(cap != nullptr && cap->getAlignmentMode() == AlignmentMode::AM_BOTTOM_RIGHT,
                      "E15 direct align survives recreate / adjustSpaceAssignment");
            }
        }
    }

    // E10：顶层 vs 内嵌（inner wins）
    auto* bp = dynamic_cast<Button*>(static_cast<Control*>(UICornerstone_FindControl(g_uiInstance, "btnPriority")));
    CHECK(bp != nullptr, "E10 priority button");
    if (bp) {
        auto cap = bp->getCaptionLabel();
        CHECK(cap != nullptr, "E10 priority caption");
        if (cap) {
            CHECK(cap->getCaption() == "InnerP", "E10 caption inner wins");
            CHECK(nearColor(cap->getTextStateColor().getNormal(), 0xAA, 0xBB, 0xCC, 0xFF), "E10 colors inner wins");
            CHECK(cap->isShadowEnabled(), "E10 shadow.enabled inner wins");
            CHECK(std::abs(cap->getShadowOffset().x - 5.0f) < 0.01f, "E10 shadow.offset inner wins");
            float blur = 0; cap->getFloatProperty(PropertyNames::kTextShadowBlur, blur);
            CHECK(std::abs(blur - 7.0f) < 0.01f, "E10 shadow.blur");
        }
    }

    // E11：shadow.blur 不自动启用（字面语义）
    auto cb2 = make_shared<CheckBox>(nullptr, SRect(500, 20, 160, 36));
    cb2->setCaptionTextShadowBlur(8.0f);
    BENCH->addControl(cb2);
    auto cap2 = cb2->getCaption();
    CHECK(cap2 == nullptr || !cap2->isShadowEnabled(), "E11 blur alone does not enable shadow");
    cb2->setBoolProperty(PropertyNames::kShadow, 1);
    cap2 = cb2->getCaption();
    CHECK(cap2 != nullptr && cap2->isShadowEnabled(), "E11 enabled=true makes shadow visible");

    // E13：空 caption-label / 非对象宽松接受（不崩溃）
    const char* emptyLayout = R"({"controls":[{"type":"check-box","id":"cbEmpty","rect":{"x":600,"y":20,"w":160,"h":36},"caption":"K","caption-label":{}}]})";
    CHECK(UICornerstone_LoadLayout(g_uiInstance, emptyLayout) == 1, "E13 empty caption-label accepted");
    UIControlHandle he = UICornerstone_FindControl(g_uiInstance, "cbEmpty");
    CHECK(he != nullptr, "E13 empty caption-label control created");

    printf("---- CR20261116001 embedded label: pass=%d fail=%d ----\n", g_pass, g_fail);
}

} // namespace

class EmbeddedLabelApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_cr_embedded_label");
        BENCH->setOnInitial([](shared_ptr<Bench>) {
            UICornerstone_LoadLayoutFromFile(g_uiInstance,
                (Platform::GetBasePath() + "layouts/cr_embedded_label.json").c_str());
            runChecks();
        });
        return true;
    }
    void onUpdate() override { BENCH->eventLoopEntry(); BENCH->update(); }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(40, 40, 44));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
    }
    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    return TestRunMain<EmbeddedLabelApp>(argc, argv);
}
