// CR20261116002：柔光阴影声明式状态 + 主题 :hover 原生支持
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <cstring>
#include <vector>

#include "Bench.h"
#include "ControlBase.h"
#include "Label.h"
#include "Menu.h"
#include "Shape.h"
#include "Panel.h"
#include "PlatformUtils.h"
#include "ShadowRenderer.h"
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

bool nearColor(const UIColor& c, int r, int g, int b, int a, int eps = 2) {
    return std::abs((int)c.r - r) <= eps && std::abs((int)c.g - g) <= eps &&
           std::abs((int)c.b - b) <= eps && std::abs((int)c.a - a) <= eps;
}

float getF(UIControlHandle h, const char* key) {
    float v = -999.0f;
    UICornerstone_GetFloat(g_uiInstance, h, key, &v);
    return v;
}

UIColor getC(UIControlHandle h, const char* key) {
    UIColor c{};
    UICornerstone_GetColor(g_uiInstance, h, key, &c);
    return c;
}

shared_ptr<Panel> addPanel(const char* id, SRect rect) {
    auto p = make_shared<Panel>(nullptr, rect);
    p->setJsonId(id);
    p->setTransparent(false);
    p->setColorProperty(PropertyNames::kBackground, SColor(240, 240, 240, 255));
    BENCH->addControl(p);
    return p;
}

UIControlHandle handleOf(const shared_ptr<Control>& p) {
    return static_cast<UIControlHandle>(p.get());
}

shared_ptr<Panel> g_state, g_selected, g_order, g_spec, g_clean, g_clean2, g_empty, g_clearState, g_cache;
shared_ptr<MenuPanel> g_mp;
shared_ptr<Label> g_lbl;
shared_ptr<Shape> g_shape;
bool g_cacheDone = false;
int g_frame = 0;
std::string g_shotPath;   // shot=<path>：第 8 帧保存整屏 BMP（视觉复核）
bool g_shotMode = false;

const char* kThemeA = R"JSON({
  "styles": [
    { "selector": "#t-state", "declarations": { "box-shadow": {
        "color": "#00000022", "offset-x": 1, "offset-y": 2, "blur": 6,
        "hover": { "color": "#FF0000FF", "blur": 14 },
        "pressed": { "color": "#00FF00FF" },
        "disabled": { "color": "#0000FFFF" } } } },
    { "selector": "#t-selected", "declarations": { "box-shadow": { "color": "#11111122", "blur": 4 } } },
    { "selector": "#t-selected:selected", "declarations": { "box-shadow": { "color": "#FFFF00FF" } } },
    { "selector": "#t-order", "declarations": { "box-shadow": { "color": "#11111122", "blur": 4, "hover": { "color": "#0000FFFF" } } } },
    { "selector": "#t-order:hover", "declarations": { "box-shadow": { "color": "#FF0000FF" }, "box-shadow-color.hover": "#FF00FFFF" } },
    { "selector": "#t-spec:hover", "declarations": { "box-shadow": { "blur": 10 } } },
    { "selector": "panel#t-spec:hover", "declarations": { "box-shadow": { "blur": 22 } } },
    { "selector": "#t-clean", "declarations": { "box-shadow": { "color": "#00000011", "blur": 4 } } },
    { "selector": "#t-clean:hover", "declarations": { "box-shadow": { "color": "#000000AA", "blur": 18 } } },
    { "selector": "#t-clean2", "declarations": { "box-shadow": { "color": "#00000011", "offset-x": 1, "blur": 4 } } },
    { "selector": "#t-clean2:hover", "declarations": { "box-shadow": { "color": "#000000AA" } } },
    { "selector": "#t-empty", "declarations": { "box-shadow": { "color": "#00000022", "blur": 6 } } },
    { "selector": "#t-clear-state", "declarations": { "box-shadow": { "color": "#00000011", "blur": 4 } } },
    { "selector": "#t-clear-state:hover", "declarations": { "box-shadow": { "color": "#000000CC", "blur": 16 } } },
    { "selector": "#t-mp:hover", "declarations": { "box-shadow": { "color": "#FF0000FF", "blur": 30 } } },
    { "selector": "#t-label", "declarations": { "box-shadow": { "color": "#00000022", "blur": 6 } } },
    { "selector": "#t-label:hover", "declarations": { "box-shadow": { "color": "#FF0000FF", "blur": 18 }, "text-shadow": "#FF0000FF", "text-shadow-blur": 30, "shadow-offset-x": 9 } },
    { "selector": "#t-shape", "declarations": { "box-shadow": { "color": "#00000022", "blur": 6 } } },
    { "selector": "#t-shape:hover", "declarations": { "box-shadow": { "color": "#00FF00FF", "blur": 20 } } },
    { "selector": "#t-empty:hover", "declarations": { "box-shadow": { "color": "#000000BB", "blur": 20 } } },
    { "selector": "#shadow-force", "declarations": { "box-shadow": { "color": "#00FF00FF" } } }
  ]
})JSON";

const char* kThemeB = R"JSON({
  "styles": [
    { "selector": "#t-clean", "declarations": { "box-shadow": { "color": "#00000044", "blur": 8 } } },
    { "selector": "#t-clean2", "declarations": { "box-shadow": { "offset-x": 5 } } },
    { "selector": "#t-b5", "declarations": { "box-shadow": { "color": "#FF0000FF", "blur": 6 } } }
  ]
})JSON";

void runChecks() {
    // S1/S2/S3/S4：JSON 解析、部分继承、alpha=0、四态
    int ok = UICornerstone_LoadLayoutFromFile(g_uiInstance,
        (Platform::GetBasePath() + "layouts/cr_shadow_state.json").c_str());
    CHECK(ok == 1, "load cr_shadow_state layout");
    UIControlHandle h = UICornerstone_FindControl(g_uiInstance, "shadow-json");
    UIControlHandle hp = UICornerstone_FindControl(g_uiInstance, "shadow-partial");
    CHECK(h && hp, "find shadow panels");
    if (h && hp) {
        { UIColor dbg = getC(h, "box-shadow-color"); printf("DBG normal color=%d,%d,%d,%d\n", dbg.r, dbg.g, dbg.b, dbg.a); }
        CHECK(nearColor(getC(h, "box-shadow-color"), 0, 0, 0, 0x33), "S1 normal color");
        CHECK(std::abs(getF(h, "box-shadow-offset-x") - 2.0f) < 0.01f, "S1 normal offset-x");
        CHECK(std::abs(getF(h, "box-shadow-radius") - 12.0f) < 0.01f, "S1 normal blur");
        CHECK(std::abs(getF(h, "box-shadow-spread") - 1.0f) < 0.01f, "S1 normal spread");

        CHECK(nearColor(getC(h, "box-shadow-color.hover"), 0, 0, 0, 0x55), "S2 hover color override");
        CHECK(std::abs(getF(h, "box-shadow-offset-y.hover") - 8.0f) < 0.01f, "S2 hover offset-y override");
        CHECK(std::abs(getF(h, "box-shadow-offset-x.hover") - 2.0f) < 0.01f, "S2 hover offset-x inherits normal");
        CHECK(std::abs(getF(h, "box-shadow-radius.hover") - 12.0f) < 0.01f, "S2 hover blur inherits normal");

        CHECK(std::abs(getF(h, "box-shadow-radius.pressed") - 20.0f) < 0.01f, "S4 pressed blur");
        CHECK(nearColor(getC(h, "box-shadow-color.pressed"), 0, 0, 0, 0x33), "S4 pressed color inherits normal");
        CHECK(nearColor(getC(h, "box-shadow-color.disabled"), 0, 0, 0, 0x00), "S3 disabled alpha=0 closes shadow");

        CHECK(std::abs(getF(hp, "box-shadow-radius.hover") - 16.0f) < 0.01f, "S2 partial hover blur");
        CHECK(nearColor(getC(hp, "box-shadow-color.hover"), 0x11, 0x22, 0x33, 0x44), "S2 partial hover color inherits");
    }

    // S8：C ABI 写状态键 + Get 生效值
    if (h) {
        CHECK(UICornerstone_SetFloat(g_uiInstance, h, "box-shadow-radius.pressed", 30.0f) == 1, "S8 CABI set state key");
        CHECK(std::abs(getF(h, "box-shadow-radius.pressed") - 30.0f) < 0.01f, "S8 CABI get state key");
        UICornerstone_SetColor(g_uiInstance, h, "box-shadow-color.hover", UIColor{ 255, 0, 0, 128 });
        CHECK(nearColor(getC(h, "box-shadow-color.hover"), 255, 0, 0, 128), "S8 CABI set state color");
    }

    // S4/S9：状态优先级与 resolve（禁用 > pressed > hover > normal）
    if (h) {
        auto* impl = dynamic_cast<ControlImpl*>(static_cast<Control*>(h));
        CHECK(impl != nullptr, "S4 control impl");
        if (impl) {
            ShadowRenderer::ShadowSpec spec;
            impl->setState(ControlState::Normal);  impl->resolveBoxShadowSpec(spec);
            CHECK(nearColor(UIColor{ (unsigned char)(spec.color.red()*255), (unsigned char)(spec.color.green()*255),
                                     (unsigned char)(spec.color.blue()*255), (unsigned char)(spec.color.alpha()*255) },
                            0, 0, 0, 0x33), "S4 normal spec");
            impl->setState(ControlState::Hover);   impl->resolveBoxShadowSpec(spec);
            CHECK(spec.color.alpha() > 0.49f, "S4 hover spec selected");
            impl->setState(ControlState::Pressed); impl->resolveBoxShadowSpec(spec);
            CHECK(std::abs(spec.blur - 30.0f) < 0.01f, "S4 pressed spec blur");
            impl->setState(ControlState::Disabled); impl->resolveBoxShadowSpec(spec);
            CHECK(spec.color.alpha() == 0.0f, "S4 disabled spec alpha=0");
            impl->setState(ControlState::Normal);

            SRect r = impl->getRect();
            bool hit0 = impl->isContainsPoint(r.left + 1, r.top + 1);
            impl->setState(ControlState::Pressed);
            bool hit1 = impl->isContainsPoint(r.left + 1, r.top + 1);
            SRect r1 = impl->getRect();
            CHECK(hit0 && hit1 && std::abs(r1.width - r.width) < 0.01f && std::abs(r1.height - r.height) < 0.01f,
                  "S9 state switch keeps rect/hit");
            impl->setState(ControlState::Normal);
        }
    }

    // 主题测试控件
    g_state    = addPanel("t-state", SRect(20, 140, 120, 60));
    g_selected = addPanel("t-selected", SRect(160, 140, 120, 60));
    g_order    = addPanel("t-order", SRect(300, 140, 120, 60));
    g_spec     = addPanel("t-spec", SRect(440, 140, 120, 60));
    g_clean    = addPanel("t-clean", SRect(20, 220, 120, 60));
    g_clean2   = addPanel("t-clean2", SRect(160, 220, 120, 60));
    g_empty    = addPanel("t-empty", SRect(300, 220, 120, 60));
    g_clearState = addPanel("t-clear-state", SRect(440, 220, 120, 60));
    g_mp = make_shared<MenuPanel>(nullptr, 1.0f, 1.0f);
    g_mp->setJsonId("t-mp");
    g_mp->setRect(SRect(700, 140, 160, 100));
    g_mp->show();
    BENCH->addControl(g_mp);

    g_lbl = make_shared<Label>(nullptr, SRect(700, 250, 160, 40));
    g_lbl->setJsonId("t-label");
    g_lbl->setCaption("Label");
    BENCH->addControl(g_lbl);

    g_shape = make_shared<Shape>(nullptr, SRect(880, 250, 120, 60));
    g_shape->setJsonId("t-shape");
    g_shape->setShape(ShapeType::FilledRect);
    g_shape->setFillColor(SColor(0.8f, 0.8f, 0.85f, 1.0f));
    BENCH->addControl(g_shape);

    CHECK(UICornerstone_ApplyTheme(g_uiInstance, kThemeA) == 1, "S5 apply theme A");

    // S5：主题状态选择器
    CHECK(nearColor(getC(handleOf(g_state), "box-shadow-color"), 0, 0, 0, 0x22), "S5 theme normal");
    CHECK(nearColor(getC(handleOf(g_state), "box-shadow-color.hover"), 255, 0, 0, 255), "S5 theme :hover");
    CHECK(nearColor(getC(handleOf(g_state), "box-shadow-color.pressed"), 0, 255, 0, 255), "S5 theme :pressed");
    CHECK(nearColor(getC(handleOf(g_state), "box-shadow-color.disabled"), 0, 0, 255, 255), "S5 theme :disabled");
    CHECK(std::abs(getF(handleOf(g_state), "box-shadow-radius.hover") - 14.0f) < 0.01f, "S5 theme hover blur");

    // S18：:selected → pressed
    CHECK(nearColor(getC(handleOf(g_selected), "box-shadow-color.pressed"), 255, 255, 0, 255), "S18 :selected -> pressed");

    // S6/S14：普通规则内嵌状态 vs :hover 状态；拆散键优先对象
    CHECK(nearColor(getC(handleOf(g_order), "box-shadow-color.hover"), 255, 0, 255, 255),
          "S14/S15 state selector + scattered key win");

    // S20：同类别 specificity
    CHECK(std::abs(getF(handleOf(g_spec), "box-shadow-radius.hover") - 22.0f) < 0.01f, "S20 specificity in same category");

    // S7：显式 JSON normal 优先于主题 normal；显式状态保持
    if (UIControlHandle hf = UICornerstone_FindControl(g_uiInstance, "shadow-force")) {
        CHECK(nearColor(getC(hf, "box-shadow-color"), 255, 0, 0, 255), "S7 explicit JSON normal wins over theme normal");
        CHECK(nearColor(getC(hf, "box-shadow-color.hover"), 255, 0, 0, 255), "S7 explicit hover stays");
    }

    // CR2/S13c：主题状态 + 显式状态 → ClearThemeOverrides 后显式消失、主题状态恢复
    CHECK(nearColor(getC(handleOf(g_clearState), "box-shadow-color.hover"), 0, 0, 0, 0xCC),
          "S13c theme hover before clear");
    UICornerstone_SetColor(g_uiInstance, handleOf(g_clearState), "box-shadow-color.hover", UIColor{ 0, 0, 255, 255 });
    CHECK(nearColor(getC(handleOf(g_clearState), "box-shadow-color.hover"), 0, 0, 255, 255),
          "S13c explicit hover applied");
    CHECK(UICornerstone_ClearThemeOverrides(g_uiInstance, handleOf(g_clearState)) == 1, "S13c clear theme overrides");
    CHECK(nearColor(getC(handleOf(g_clearState), "box-shadow-color.hover"), 0, 0, 0, 0xCC),
          "S13c after clear, theme hover restored");
    CHECK(UICornerstone_ClearThemeOverrides(g_uiInstance, handleOf(g_clearState)) == 1, "S13c clear again (no explicit)");
    CHECK(nearColor(getC(handleOf(g_clearState), "box-shadow-color.hover"), 0, 0, 0, 0xCC),
          "S13c theme hover kept without explicit");

    // Label / Shape：走通用 beforeDraw → 状态阴影应生效
    {
        // Q7：状态规则里的 text-shadow blur/offset 必须忽略，不能改 normal
        g_lbl->setFloatProperty(PropertyNames::kTextShadowBlur, 4.0f);
        g_lbl->setFloatProperty(PropertyNames::kShadowOffsetX, 3.0f);
        CHECK(nearColor(getC(handleOf(g_lbl), "box-shadow-color.hover"), 255, 0, 0, 255), "L1 Label theme hover color");
        CHECK(std::abs(getF(handleOf(g_lbl), "box-shadow-radius.hover") - 18.0f) < 0.01f, "L1 Label theme hover blur");
        auto* li = dynamic_cast<ControlImpl*>(static_cast<Control*>(handleOf(g_lbl)));
        if (li) {
            ShadowRenderer::ShadowSpec s;
            li->setState(ControlState::Hover); li->resolveBoxShadowSpec(s);
            CHECK(nearColor(UIColor{ (unsigned char)(s.color.red()*255), (unsigned char)(s.color.green()*255),
                                     (unsigned char)(s.color.blue()*255), (unsigned char)(s.color.alpha()*255) },
                            255, 0, 0, 255) && std::abs(s.blur - 18.0f) < 0.01f, "L1 Label hover spec");
            li->setState(ControlState::Normal);
        }
        CHECK(nearColor(getC(handleOf(g_lbl), "text-shadow.hover"), 255, 0, 0, 255), "L2 text-shadow color state still works");
        CHECK(std::abs(getF(handleOf(g_lbl), "text-shadow-blur") - 4.0f) < 0.01f, "L2 state text-shadow-blur ignored");
        CHECK(std::abs(getF(handleOf(g_lbl), "shadow-offset-x") - 3.0f) < 0.01f, "L2 state shadow-offset ignored");
        CHECK(nearColor(getC(handleOf(g_shape), "box-shadow-color.hover"), 0, 255, 0, 255), "SH1 Shape theme hover color");
        CHECK(std::abs(getF(handleOf(g_shape), "box-shadow-radius.hover") - 20.0f) < 0.01f, "SH1 Shape theme hover blur");
        auto* si = dynamic_cast<ControlImpl*>(static_cast<Control*>(handleOf(g_shape)));
        if (si) {
            ShadowRenderer::ShadowSpec s;
            si->setState(ControlState::Hover); si->resolveBoxShadowSpec(s);
            CHECK(std::abs(s.blur - 20.0f) < 0.01f && s.color.green() > 0.9f, "SH1 Shape hover spec");
            si->setState(ControlState::Normal);
        }
    }

    // CR1：MenuPanel 的状态阴影不改变 normal 外观（legacy/generic 只看 normal 字段）
    if (g_mp) {
        CHECK(!g_mp->hasExplicitNormalBoxShadowField(), "CR1 menu-panel state-only shadow does not mark normal");
        CHECK(nearColor(getC(handleOf(g_mp), "box-shadow-color"), 0, 0, 0, 0), "CR1 menu-panel normal color unchanged");
        UICornerstone_SetColor(g_uiInstance, handleOf(g_mp), "box-shadow-color", UIColor{ 0, 0, 0, 64 });
        CHECK(g_mp->hasExplicitNormalBoxShadowField(), "CR1 menu-panel explicit normal recognized");
    }

    // S13a：空主题 no-op 不清状态槽
    float emptyHover = getF(handleOf(g_empty), "box-shadow-radius.hover");
    CHECK(UICornerstone_ApplyTheme(g_uiInstance, "{}") == 1, "S13a empty theme accepted");
    CHECK(std::abs(getF(handleOf(g_empty), "box-shadow-radius.hover") - emptyHover) < 0.01f,
          "S13a empty theme keeps state shadow");
    CHECK(UICornerstone_ApplyTheme(g_uiInstance, "{\"variables\":{\"--x\":\"#FFFFFF\"}}") == 1,
          "S13a variables-only theme accepted");
    CHECK(std::abs(getF(handleOf(g_empty), "box-shadow-radius.hover") - emptyHover) < 0.01f,
          "S13a variables-only keeps state shadow");

    // S13/B1：主题 B（有数据、无状态）清理旧状态槽
    CHECK(UICornerstone_ApplyTheme(g_uiInstance, kThemeB) == 1, "S13 apply theme B");
    CHECK(std::abs(getF(handleOf(g_clean), "box-shadow-radius.hover") - 8.0f) < 0.01f,
          "S13 hover falls back to B normal blur");
    CHECK(nearColor(getC(handleOf(g_clean), "box-shadow-color.hover"), 0, 0, 0, 0x44),
          "S13 hover falls back to B normal color");
    // Label/Shape 的主题状态也随主题切换清理，回退各自 normal
    CHECK(nearColor(getC(handleOf(g_lbl), "box-shadow-color.hover"), 0, 0, 0, 0x22), "L1 Label hover falls back after theme switch");
    CHECK(std::abs(getF(handleOf(g_lbl), "box-shadow-radius.hover") - 6.0f) < 0.01f, "L1 Label hover blur fallback");
    CHECK(nearColor(getC(handleOf(g_shape), "box-shadow-color.hover"), 0, 0, 0, 0x22), "SH1 Shape hover falls back after theme switch");

    // B11：B 只有部分 normal 字段
    CHECK(std::abs(getF(handleOf(g_clean2), "box-shadow-offset-x") - 5.0f) < 0.01f, "B11 partial normal offset-x");
    CHECK(nearColor(getC(handleOf(g_clean2), "box-shadow-color"), 0, 0, 0, 0x11), "B11 partial normal keeps A color");
    CHECK(std::abs(getF(handleOf(g_clean2), "box-shadow-radius.hover") - 4.0f) < 0.01f,
          "B11 hover falls back to A normal blur");

    // S13c：显式状态经过主题清理后 replay；ClearThemeOverrides 后回到主题 B
    UICornerstone_SetColor(g_uiInstance, handleOf(g_clean), "box-shadow-color.hover", UIColor{ 0, 0, 0, 0x88 });
    CHECK(UICornerstone_ApplyTheme(g_uiInstance, kThemeB) == 1, "S13c reapply theme B");
    CHECK(nearColor(getC(handleOf(g_clean), "box-shadow-color.hover"), 0, 0, 0, 0x88),
          "S13c explicit hover survives theme reapply");
    CHECK(UICornerstone_ClearThemeOverrides(g_uiInstance, handleOf(g_clean)) == 1, "S13c clear overrides");
    CHECK(nearColor(getC(handleOf(g_clean), "box-shadow-color.hover"), 0, 0, 0, 0x44),
          "S13c after clear, hover returns theme B normal");

    // B5：显式 hover 等于 normal 且槽被清空 → replay 仍强制写入
    {
        auto pB5 = addPanel("t-b5", SRect(440, 300, 120, 60));
        UICornerstone_SetColor(g_uiInstance, handleOf(pB5), "box-shadow-color", UIColor{ 255, 0, 0, 255 });
        UICornerstone_SetColor(g_uiInstance, handleOf(pB5), "box-shadow-color.hover", UIColor{ 255, 0, 0, 255 });
        auto* impl = dynamic_cast<ControlImpl*>(static_cast<Control*>(handleOf(pB5)));
        CHECK(impl != nullptr, "B5 impl");
        if (impl) {
            impl->clearThemeBoxShadowStates(1u);   // 清 hover color 槽，模拟主题清理
            CHECK(!impl->isBoxShadowStateSet(0, 0), "B5 hover slot cleared");
            CHECK(UICornerstone_ApplyTheme(g_uiInstance, kThemeB) == 1, "B5 reapply theme B");
            CHECK(impl->isBoxShadowStateSet(0, 0), "B5 explicit hover forced even when equal to normal");
        }
    }

    // S13b：显式 alpha=0 关闭（normal / 状态）
    UICornerstone_SetFloat(g_uiInstance, handleOf(g_clean), "box-shadow-radius.hover", 0.0f);
    CHECK(std::abs(getF(handleOf(g_clean), "box-shadow-radius.hover")) < 0.01f, "S13b explicit state value");

    // B4a/S19：ContextMenu 不支持 box-shadow 绘制，状态键只存储 + 一次性诊断（不崩溃）
    {
        UIControlHandle cm = UICornerstone_CreateContextMenu(g_uiInstance, 600, 320, 120, 60, 1, 1);
        CHECK(cm != nullptr, "S19 ContextMenu created");
        if (cm) {
            CHECK(UICornerstone_SetColor(g_uiInstance, cm, "box-shadow-color.hover", UIColor{ 0, 0, 0, 128 }) == 1,
                  "S19 ContextMenu accepts state key (stored, diagnosed)");
            UICornerstone_ContextMenuClose(g_uiInstance, cm);
        }
    }

    // S12：strict/schema 由外部 validate_layout 覆盖；此处验证带状态布局已加载
    printf("---- CR20261116002 shadow state: pass=%d fail=%d ----\n", g_pass, g_fail);
}

void runCacheChecks() {
    g_cache = addPanel("t-cache", SRect(20, 320, 140, 70));
    g_cache->setColorProperty(PropertyNames::kBoxShadowColor, SColor(0.0f, 0.0f, 0.0f, 0.2f));
    g_cache->setFloatProperty(PropertyNames::kBoxShadowRadius, 10.0f);
    BENCH->draw();

    using Cache = ShadowRenderer::ShadowCache;
    Cache* cache = ShadowRenderer::getCache(g_uiInstance);
    CHECK(cache != nullptr, "S17 shadow cache available");
    if (!cache) return;

    g_cache->setState(ControlState::Normal);
    BENCH->draw();
    size_t h0 = cache->hitCount(), m0 = cache->missCount();
    BENCH->draw();
    CHECK(cache->hitCount() > h0, "S17 same spec reuses texture (hit)");

    // hover 无覆盖 → 与 normal 同规格 → 命中
    h0 = cache->hitCount();
    g_cache->setState(ControlState::Hover);
    BENCH->draw();
    CHECK(cache->hitCount() > h0, "S17 same effective spec across states reuses");

    // hover 不同规格 → miss，再画命中
    g_cache->setFloatProperty(PropertyNames::kBoxShadowRadiusHover, 24.0f);
    size_t m1 = cache->missCount();
    BENCH->draw();
    CHECK(cache->missCount() > m1, "S17 different state spec misses");
    h0 = cache->hitCount();
    BENCH->draw();
    CHECK(cache->hitCount() > h0, "S17 new state spec reuses after bake");

    // 回到 normal → 原纹理仍命中
    h0 = cache->hitCount();
    g_cache->setState(ControlState::Normal);
    BENCH->draw();
    CHECK(cache->hitCount() > h0, "S17 switching back reuses normal texture");
    g_cacheDone = true;
}

} // namespace

class ShadowStateApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_cr_shadow_state");
        if (!g_shotPath.empty()) {
            g_shotMode = true;
            // 截图场景：白底 normal/hover/pressed/disabled 四联对比（便于观察柔光）
            UICornerstone_SetViewport(g_uiInstance, 0, 0, 1200, 420);
            const char* names[4] = { "normal", "hover", "pressed", "disabled" };
            for (int i = 0; i < 4; ++i) {
                auto p = make_shared<Panel>(nullptr, SRect(60.0f + i * 280.0f, 150.0f, 220.0f, 120.0f));
                p->setTransparent(false);
                p->setColorProperty(PropertyNames::kBackground, SColor(255, 255, 255, 255));
                p->setCornerRadius(12.0f);
                p->setColorProperty(PropertyNames::kBoxShadowColor, SColor(0.0f, 0.0f, 0.0f, 0.22f));
                p->setFloatProperty(PropertyNames::kBoxShadowOffsetY, 6.0f);
                p->setFloatProperty(PropertyNames::kBoxShadowRadius, 14.0f);
                p->setColorProperty(PropertyNames::kBoxShadowColorHover, SColor(0.0f, 0.0f, 0.0f, 0.38f));
                p->setFloatProperty(PropertyNames::kBoxShadowRadiusHover, 22.0f);
                p->setColorProperty(PropertyNames::kBoxShadowColorPressed, SColor(0.0f, 0.0f, 0.0f, 0.50f));
                p->setFloatProperty(PropertyNames::kBoxShadowRadiusPressed, 26.0f);
                p->setColorProperty(PropertyNames::kBoxShadowColorDisabled, SColor(0.0f, 0.0f, 0.0f, 0.10f));
                p->setFloatProperty(PropertyNames::kBoxShadowRadiusDisabled, 6.0f);
                BENCH->addControl(p);
                p->setState(i == 0 ? ControlState::Normal : i == 1 ? ControlState::Hover :
                             i == 2 ? ControlState::Pressed : ControlState::Disabled);
                auto lbl = make_shared<Label>(nullptr, SRect(60.0f + i * 280.0f, 100.0f, 220.0f, 30.0f));
                lbl->setCaption(names[i]);
                lbl->setAlignmentMode(AlignmentMode::AM_CENTER);
                lbl->setFontSize(18);
                lbl->setColorProperty(PropertyNames::kText, SColor(45.0f / 255.0f, 45.0f / 255.0f, 50.0f / 255.0f, 1.0f));
                BENCH->addControl(lbl);
            }
            return true;
        }
        BENCH->setOnInitial([](shared_ptr<Bench>) { runChecks(); });
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
        ++g_frame;
        if (g_frame == 4 && !g_cacheDone) runCacheChecks();
    }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(g_shotMode ? SColor(242, 243, 246, 255) : SColor(40, 40, 44));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
        if (!g_shotPath.empty() && g_frame >= 3) {
            int cw = 0, ch = 0;
            std::vector<uint8_t> cap(1400u * 900u * 4u);
            if (UICornerstone_CaptureViewport(g_uiInstance, cap.data(), &cw, &ch) && cw > 0 && ch > 0) {
                UICornerstone_SavePixelsToFile(cap.data(), cw, ch, g_shotPath.c_str());
            }
            g_shotPath.clear();
        }
    }
    void onQuit() override {}
};

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "shot=", 5) == 0) g_shotPath = argv[i] + 5;
    }
    return TestRunMain<ShadowStateApp>(argc, argv);
}
