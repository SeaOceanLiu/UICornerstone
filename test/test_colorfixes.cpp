// test_colorfixes.cpp — 第三批 P0-5/P0-6 修复验证（内部测试：LayoutParser + 控件内部断言）
// P0-6：LoadLayout 创建的 ColorPicker 关闭态恰好一对（swatch + hex Label），无重复构建
//   - 用例 1（设计器同构）：{color, font{name,size}}——无 closed-* 键（复核补充的触发器场景）
//   - 用例 2（closed-* 属性路径）：{swatch-size, closed-font-size, closed-text-color}
//   复现路径：parseLayout（未挂树，create 延迟）→ BENCH->addControl（setContext 级联 recreate）
// P0-5：Button getPtrProperty("caption-label") 返回内部 caption Label 句柄（== getCaptionLabel()）
#include <iostream>
#include <string>
#include <memory>
#include "LayoutParser.h"
#include "ColorPicker.h"
#include "Button.h"
#include "CheckBox.h"
#include "WinFrame.h"
#include "Label.h"
#include "PropertyNames.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "TestInstance.h"

using namespace std;

static LayoutParser g_parser;
static int g_fail = 0;

#define CHECK(cond, name) do { \
    if (cond) TestUtil::log("PASS: %s", name); \
    else { TestUtil::log("FAIL: %s", name); ++g_fail; } } while (0)

static const char* kLayoutJson = R"JSON({
  "version": "1.1",
  "controls": [
    {
      "type": "panel", "id": "root",
      "rect": { "x": 0, "y": 0, "w": 900, "h": 600 },
      "children": [
        { "type": "color-picker", "id": "cp_iso",
          "color": "#4A90D9", "rect": { "x": 20, "y": 20, "w": 120, "h": 24 },
          "font": { "name": "MapleMono_NF_CN_Regular", "size": 16 } },
        { "type": "color-picker", "id": "cp_closed",
          "color": "#228833", "rect": { "x": 20, "y": 60, "w": 120, "h": 24 },
          "swatch-size": 16, "closed-font-size": 14, "closed-text-color": "#DCDCDCFF" },
        { "type": "button", "id": "btn_cap",
          "caption": "Cap", "rect": { "x": 20, "y": 100, "w": 120, "h": 30 },
          "caption-label": { "caption": "CustomCap", "font": { "size": 20 } },
          "shadow": { "enabled": true, "offset": { "x": 2, "y": 3 } } },
        { "type": "check-box", "id": "cb_json",
          "caption": "CBJ", "rect": { "x": 20, "y": 140, "w": 120, "h": 30 },
          "shadow": { "enabled": true, "offset": { "x": 4, "y": 5 } } },
        { "type": "win-frame", "id": "wf_json", "title": "WFJ",
          "rect": { "x": 20, "y": 180, "w": 200, "h": 120 },
          "shadow": { "enabled": true, "offset": { "x": 6, "y": 7 } } }
      ]
    }
  ]
})JSON";

static void testInit(shared_ptr<Bench>) {
    TestUtil::log("test_colorfixes: begin");

    auto root = g_parser.parseLayout(kLayoutJson);
    CHECK(root != nullptr, "layout parsed");
    if (!root) return;
    BENCH->addControl(root);   // 挂树 → setContext 级联（P0-6 复现路径）

    // ── P0-6 用例 1：设计器同构（font，无 closed-*） ──
    auto cpIso = dynamic_pointer_cast<ColorPicker>(g_parser.findControlById("cp_iso"));
    CHECK(cpIso != nullptr, "P0-6 iso color-picker found");
    if (cpIso) {
        size_t n = cpIso->getChildren().size();
        TestUtil::log("cp_iso children=%zu", n);
        CHECK(n == 2, "P0-6 iso closed-state children == 2 (swatch+label)");
    }

    // ── P0-6 用例 2：closed-* 属性路径 ──
    auto cpClosed = dynamic_pointer_cast<ColorPicker>(g_parser.findControlById("cp_closed"));
    CHECK(cpClosed != nullptr, "P0-6 closed-variant color-picker found");
    if (cpClosed) {
        size_t n = cpClosed->getChildren().size();
        TestUtil::log("cp_closed children=%zu", n);
        CHECK(n == 2, "P0-6 closed-variant closed-state children == 2");
    }

    // ── P0-5：Button caption Label 句柄暴露 ──
    auto btn = dynamic_pointer_cast<Button>(g_parser.findControlById("btn_cap"));
    CHECK(btn != nullptr, "P0-5 button found");
    if (btn) {
        void* out = nullptr;
        int rc = btn->getPtrProperty(PropertyNames::kCaptionLabel, out);
        // 句柄约定：out 为 Control*（虚继承偏移已修正）——按约定还原比较/下转
        Control* capCtl = static_cast<Control*>(out);
        CHECK(rc == 1 && capCtl != nullptr &&
              capCtl == static_cast<Control*>(btn->getCaptionLabel().get()),
              "P0-5 getPtrProperty(caption-label) == caption Label handle");
        CHECK(btn->getCaptionLabel() && btn->getCaptionLabel()->getCaption() == "CustomCap",
              "P0-5-6 LayoutParser caption-label JSON applied (kebab key live)");
        // 句柄直控：Label 标准属性生效（状态色读写回）
        auto cap = dynamic_cast<Label*>(capCtl);
        SColor red(255, 0, 0, 255);
        cap->setTextNormalStateColor(red);
        CHECK(cap->getTextStateColor().getNormal() == red,
              "P0-5 caption handle direct SetColor(text) roundtrip");
    }

    // ── P0-7：状态联动（Button/CheckBox/WinFrame → 内部 caption/title Label） ──
    if (btn) {
        auto label = btn->getCaptionLabel();
        btn->setState(ControlState::Hover);
        CHECK(label->getState() == ControlState::Hover, "P0-7-1 Button Hover -> caption state");
        btn->setState(ControlState::Pressed);
        CHECK(label->getState() == ControlState::Pressed, "P0-7-1 Button Pressed -> caption state");
        btn->setEnable(false);
        CHECK(label->getState() == ControlState::Disabled, "P0-7-2 setEnable(false) -> caption Disabled");
        btn->setEnable(true);
        CHECK(label->getState() == ControlState::Normal, "P0-7-2 setEnable(true) -> caption Normal");
        // caption 替换按当前态同步
        btn->setState(ControlState::Hover);
        auto newCap = LabelBuilder(btn.get(), SRect(0, 0, 120, 30)).setCaption("R").build();
        btn->setCaptionLabel(newCap);
        CHECK(newCap->getState() == ControlState::Hover, "P0-7-3 replace caption syncs current state");
        btn->setState(ControlState::Normal);
    }

    // CheckBox 同型
    auto cb = CheckBoxBuilder(nullptr, SRect(300, 100, 160, 30)).setCaptionText("CB").build();
    BENCH->addControl(cb);
    auto cbCap = cb->getCaption();
    CHECK(cbCap != nullptr, "P0-7-4 CheckBox caption exists");
    if (cbCap) {
        cb->setState(ControlState::Hover);
        CHECK(cbCap->getState() == ControlState::Hover, "P0-7-4 CheckBox Hover -> caption state");
        cb->setEnable(false);
        CHECK(cbCap->getState() == ControlState::Disabled, "P0-7-4 CheckBox Disabled -> caption state");
        cb->setEnable(true);
    }

    // WinFrame 标题联动（P0-7-7）
    auto wf = WinFrameBuilder(nullptr, SRect(300, 150, 220, 160)).setTitle("WF").build();
    BENCH->addControl(wf);
    auto wfTitle = wf->getTitleLabel();
    CHECK(wfTitle != nullptr, "P0-7-7 WinFrame title label exists");
    if (wfTitle) {
        wf->setState(ControlState::Normal);
        CHECK(wfTitle->getState() == ControlState::Normal, "P0-7-7 WinFrame Normal -> title state");
        wf->setEnable(false);
        CHECK(wfTitle->getState() == ControlState::Disabled, "P0-7-7 WinFrame Disabled -> title state");
        wf->setEnable(true);
    }

    // ── P0-8：CheckBox caption-label 句柄 + WinFrame title-label 既有键回归 ──
    if (cb) {
        void* out = nullptr;
        int rc = cb->getPtrProperty(PropertyNames::kCaptionLabel, out);
        Control* capCtl = static_cast<Control*>(out);
        CHECK(rc == 1 && capCtl != nullptr &&
              capCtl == static_cast<Control*>(cb->getCaption().get()),
              "P0-8-1 CheckBox getPtrProperty(caption-label) == caption handle");
        if (capCtl) {
            auto cap = dynamic_cast<Label*>(capCtl);
            SColor blue(0, 0, 255, 255);
            cap->setTextHoverStateColor(blue);
            cb->setState(ControlState::Hover);
            CHECK(cap->getTextStateColor().getHover() == blue &&
                  cap->getState() == ControlState::Hover,
                  "P0-8-4 CheckBox full chain (handle set hover color + state)");
            cb->setState(ControlState::Normal);
        }
    }
    if (wf) {
        void* out = nullptr;
        int rc = wf->getPtrProperty(PropertyNames::kTitleLabel, out);
        Control* tCtl = static_cast<Control*>(out);
        CHECK(rc == 1 && tCtl != nullptr &&
              tCtl == static_cast<Control*>(wf->getTitleLabel().get()),
              "P0-8-2 WinFrame title-label existing key regression (no alias)");
        if (tCtl) {
            auto title = dynamic_cast<Label*>(tCtl);
            SColor green(0, 200, 0, 255);
            title->setTextHoverStateColor(green);
            wf->setState(ControlState::Hover);
            CHECK(title->getTextStateColor().getHover() == green &&
                  title->getState() == ControlState::Hover,
                  "P0-8-4 WinFrame full chain via title-label (handle + state)");
            wf->setState(ControlState::Normal);
        }
    }

    // ── #9/#10/#12：CheckBox 文本/shadow + 三控件 shadow 统一 + JSON shadow 解析 ──
    // JSON shadow 解析（button / check-box / win-frame）
    {
        auto btnJ = dynamic_pointer_cast<Button>(g_parser.findControlById("btn_cap"));
        auto cbJ  = dynamic_pointer_cast<CheckBox>(g_parser.findControlById("cb_json"));
        auto wfJ  = dynamic_pointer_cast<WinFrame>(g_parser.findControlById("wf_json"));
        CHECK(btnJ && btnJ->getCaptionLabel() && btnJ->getCaptionLabel()->isShadowEnabled() &&
              btnJ->getCaptionLabel()->getShadowOffset().x == 2.0f &&
              btnJ->getCaptionLabel()->getShadowOffset().y == 3.0f,
              "#12 JSON button shadow parsed (enabled + offset 2,3)");
        CHECK(cbJ && cbJ->getCaption() && cbJ->getCaption()->isShadowEnabled() &&
              cbJ->getCaption()->getShadowOffset().x == 4.0f &&
              cbJ->getCaption()->getShadowOffset().y == 5.0f,
              "#12 JSON check-box shadow parsed (enabled + offset 4,5)");
        CHECK(wfJ && wfJ->getTitleLabel() && wfJ->getTitleLabel()->isShadowEnabled() &&
              wfJ->getTitleLabel()->getShadowOffset().x == 6.0f &&
              wfJ->getTitleLabel()->getShadowOffset().y == 7.0f,
              "#12 JSON win-frame shadow parsed (enabled + offset 6,7)");
    }
    if (cb) {
        // #9 caption 字符串分发
        int src = cb->setStringProperty(PropertyNames::kCaption, "NewCB");
        const char* sout = nullptr;
        int grc = cb->getStringProperty(PropertyNames::kCaption, sout);
        CHECK(src == 1 && grc == 1 && sout && std::string(sout) == "NewCB",
              "#9 CheckBox caption string roundtrip");
        CHECK(cb->getCaption() && cb->getCaption()->getCaption() == "NewCB",
              "#9 CheckBox caption applied to label");
        // #10/#12 shadow 开关与偏移
        CHECK(cb->setBoolProperty(PropertyNames::kShadow, 1) == 1, "#10 CheckBox set shadow=1");
        int sb = 0;
        CHECK(cb->getBoolProperty(PropertyNames::kShadow, sb) == 1 && sb == 1, "#10 CheckBox shadow readback=1");
        cb->setFloatProperty(PropertyNames::kShadowOffsetX, 9.0f);
        cb->setFloatProperty(PropertyNames::kShadowOffsetY, 11.0f);
        float ox = 0, oy = 0;
        cb->getFloatProperty(PropertyNames::kShadowOffsetX, ox);
        cb->getFloatProperty(PropertyNames::kShadowOffsetY, oy);
        CHECK(ox == 9.0f && oy == 11.0f, "#12 CheckBox shadow offset roundtrip");
        CHECK(cb->getCaption() && cb->getCaption()->isShadowEnabled(),
              "#12 CheckBox caption label shadow enabled");
    }
    if (btn) {
        btn->setBoolProperty(PropertyNames::kShadow, 1);
        int sb = 0;
        CHECK(btn->getBoolProperty(PropertyNames::kShadow, sb) == 1 && sb == 1,
              "#12 Button kShadow (统一键) set/get");
        btn->setFloatProperty(PropertyNames::kShadowOffsetX, 3.0f);
        btn->setFloatProperty(PropertyNames::kShadowOffsetY, 4.0f);
        float ox = 0, oy = 0;
        btn->getFloatProperty(PropertyNames::kShadowOffsetX, ox);
        btn->getFloatProperty(PropertyNames::kShadowOffsetY, oy);
        CHECK(ox == 3.0f && oy == 4.0f, "#12 Button shadow offset roundtrip");
    }
    if (wf) {
        wf->setBoolProperty(PropertyNames::kShadow, 1);
        int sb = 0;
        CHECK(wf->getBoolProperty(PropertyNames::kShadow, sb) == 1 && sb == 1,
              "#12 WinFrame kShadow set/get");
        wf->setFloatProperty(PropertyNames::kShadowOffsetX, 7.0f);
        wf->setFloatProperty(PropertyNames::kShadowOffsetY, 8.0f);
        float ox = 0, oy = 0;
        wf->getFloatProperty(PropertyNames::kShadowOffsetX, ox);
        wf->getFloatProperty(PropertyNames::kShadowOffsetY, oy);
        CHECK(ox == 7.0f && oy == 8.0f, "#12 WinFrame title shadow offset roundtrip");
    }

    // ── #15：CheckBox caption 残留修复 + 文本/字号/色持久化（recreate 场景） ──
    {
        auto cbT = CheckBoxBuilder(nullptr, SRect(500, 100, 160, 30)).setCaptionText("T0").build();
        BENCH->addControl(cbT);
        cbT->setStringProperty(PropertyNames::kCaption, "T1");
        cbT->setCaptionSize(18.0f);
        // 直控路径设各态色/shadow（§3.2：经 caption Label 自身字段）
        if (cbT->getCaption()) {
            cbT->getCaption()->setTextHoverStateColor(SColor(0, 255, 0, 255));
            cbT->getCaption()->setShadow(true);
            cbT->getCaption()->setShadowOffset(SPoint(6, 6));
        }
        cbT->setRect(SRect(500, 130, 160, 30));   // 触发 recreate（CheckBox::setRect → recreate）
        int labelCount = 0;
        for (auto& ch : cbT->getChildren())
            if (dynamic_cast<Label*>(ch.get())) ++labelCount;
        CHECK(labelCount == 1, "#15-1 recreate 后 caption Label 唯一（无残留渲染树）");
        CHECK(cbT->getCaption() && cbT->getCaption()->getCaption() == "T1",
              "#15-2 文本持久（recreate 后）");
        CHECK(cbT->getCaption() && cbT->getCaption()->getFontSize() == 18,
              "#15-2 字号持久（recreate 后）");
        CHECK(cbT->getCaption() && cbT->getCaption()->getTextStateColor().getHover() == SColor(0, 255, 0, 255),
              "#3.2 直控 hover 色持久（快照重建）");
        CHECK(cbT->getCaption() && cbT->getCaption()->isShadowEnabled() &&
              cbT->getCaption()->getShadowOffset().x == 6.0f,
              "#3.2 直控 shadow/offset 持久（快照重建）");
    }

    // ── #13：WinFrame 文本/阴影四态单色转发 + 读回 ──
    if (wf) {
        SColor red(255, 0, 0, 255);
        SColor back(0, 0, 0, 0);
        CHECK(wf->setColorProperty(PropertyNames::kTextShadow, red) == 1 &&
              wf->getColorProperty(PropertyNames::kTextShadow, back) == 1 && back == red,
              "#13 WinFrame text-shadow roundtrip");
        SColor g(0, 255, 0, 255);
        bool ok = true;
        wf->setColorProperty(PropertyNames::kTextShadowHover, g);
        wf->getColorProperty(PropertyNames::kTextShadowHover, back); ok = ok && (back == g);
        wf->setColorProperty(PropertyNames::kTextShadowPressed, g);
        wf->getColorProperty(PropertyNames::kTextShadowPressed, back); ok = ok && (back == g);
        wf->setColorProperty(PropertyNames::kTextShadowDisabled, g);
        wf->getColorProperty(PropertyNames::kTextShadowDisabled, back); ok = ok && (back == g);
        CHECK(ok, "#13 WinFrame text-shadow 四态（hover/pressed/disabled）roundtrip");
        ok = true;
        wf->setColorProperty(PropertyNames::kTextHover, g);
        wf->getColorProperty(PropertyNames::kTextHover, back); ok = ok && (back == g);
        wf->setColorProperty(PropertyNames::kTextPressed, g);
        wf->getColorProperty(PropertyNames::kTextPressed, back); ok = ok && (back == g);
        wf->setColorProperty(PropertyNames::kTextDisabled, g);
        wf->getColorProperty(PropertyNames::kTextDisabled, back); ok = ok && (back == g);
        CHECK(ok, "#13 WinFrame text 三态（hover/pressed/disabled）roundtrip");
        // offset 回归（上批实施）
        wf->setFloatProperty(PropertyNames::kShadowOffsetX, 5.0f);
        float ox = 0;
        wf->getFloatProperty(PropertyNames::kShadowOffsetX, ox);
        CHECK(ox == 5.0f, "#13 WinFrame shadow-offset-x 回归");
    }

    TestUtil::log("test_colorfixes: done (fail=%d)", g_fail);
}

class ColorFixesApp : public AppCallbacks {
public:
    bool onInit() override {
        BENCH->setOnInitial(testInit);
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }
    void onRender() override {
        GET_RENDERDEVICE->clear();
        BENCH->draw();
    }
    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    return TestRunMain<ColorFixesApp>(argc, argv);
}