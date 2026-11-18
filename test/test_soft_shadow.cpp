#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "Bench.h"
#include "Label.h"
#include "Menu.h"
#include "ContextMenu.h"
#include "Panel.h"
#include "ShadowRenderer.h"
#include "TestUtils.h"
#include "TestInstance.h"

using namespace std;

namespace {

// 应用层 hover 联动示例：阴影是静态属性，业务可在 onMouseEnter/Leave 中调整。
class HoverShadowPanel : public Panel {
public:
    HoverShadowPanel(Control* parent, SRect rect) : Panel(parent, rect) {
        setTransparent(false);
        setBorderVisible(false);
        setCornerRadius(12.0f);
        setColorProperty(PropertyNames::kBackground, SColor(235, 235, 235, 255));
        setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 51));
        setFloatProperty(PropertyNames::kBoxShadowRadius, 10.0f);
        setFloatProperty(PropertyNames::kBoxShadowSpread, 0.0f);
    }
    void onMouseEnter(float x, float y) override {
        Panel::onMouseEnter(x, y);
        setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 102));
        setFloatProperty(PropertyNames::kBoxShadowRadius, 18.0f);
    }
    void onMouseLeave(float x, float y) override {
        Panel::onMouseLeave(x, y);
        setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 51));
        setFloatProperty(PropertyNames::kBoxShadowRadius, 10.0f);
    }
};

int g_pass = 0;
int g_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { ++g_pass; printf("OK   %s\n", msg); } \
         else { ++g_fail; printf("FAIL %s\n", msg); } } while (0)

shared_ptr<Panel> g_panelA;
shared_ptr<Panel> g_panelB;
shared_ptr<Panel> g_panelC;
shared_ptr<HoverShadowPanel> g_hoverPanel;
shared_ptr<Label> g_label;
shared_ptr<MenuPanel> g_menuPanel;
shared_ptr<ContextMenu> g_contextMenu;

int g_frame = 0;
bool g_checkedCache = false;
bool g_checkedPixels = false;
bool g_checkedSoft = false;
bool g_summaryPrinted = false;
std::string g_shotPath;   // shot=<path>：第 7 帧保存 BMP，便于视觉复核

constexpr uint32_t kBG = 0xFFFFFFFFu;   // 白底，便于观察柔光阴影渐变

uint32_t pixelAt(int x, int y) {
    BENCH->getRenderDevice()->flush();
    SRect r(static_cast<float>(x), static_cast<float>(y), 1.0f, 1.0f);
    uint32_t p = 0;
    BENCH->getRenderDevice()->readPixels(&p, r);
    return p;
}

int redOf(uint32_t p) { return static_cast<int>(p & 0xFF); }

void setupControls() {
    g_panelA = make_shared<Panel>(nullptr, SRect(100, 100, 200, 100));
    g_panelA->setTransparent(false);
    g_panelA->setBorderVisible(false);
    g_panelA->setCornerRadius(0.0f);
    g_panelA->setColorProperty(PropertyNames::kBackground, SColor(235, 235, 235, 255));
    g_panelA->setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 153)); // 0.6
    g_panelA->setFloatProperty(PropertyNames::kBoxShadowRadius, 0.0f);
    g_panelA->setFloatProperty(PropertyNames::kBoxShadowSpread, 6.0f);
    BENCH->addControl(g_panelA);

    g_panelB = make_shared<Panel>(nullptr, SRect(400, 100, 200, 100));
    g_panelB->setTransparent(false);
    g_panelB->setBorderVisible(false);
    g_panelB->setCornerRadius(0.0f);
    g_panelB->setColorProperty(PropertyNames::kBackground, SColor(235, 235, 235, 255));
    g_panelB->setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 153));
    g_panelB->setFloatProperty(PropertyNames::kBoxShadowRadius, 0.0f);
    g_panelB->setFloatProperty(PropertyNames::kBoxShadowSpread, 6.0f);
    BENCH->addControl(g_panelB);

    g_label = make_shared<Label>(nullptr, SRect(100, 300, 360, 60));
    g_label->setCaption("Shadow Blur");
    g_label->setFontSize(32);
    g_label->setShadow(true);
    g_label->setShadowOffset(SPoint(1, 2));
    {
        StateColor shadowColor;
        shadowColor.setNormal(SColor(0, 0, 0, 204));
        g_label->setTextShadowStateColor(shadowColor);
        StateColor textColor;
        textColor.setNormal(SColor(20, 20, 20, 255));
        g_label->setTextStateColor(textColor);
    }
    g_label->setFloatProperty(PropertyNames::kTextShadowBlur, 8.0f);
    BENCH->addControl(g_label);

    // B2：MenuPanel 默认 8；显式 0 关闭投影，随后恢复 8 验证柔光。
    g_menuPanel = make_shared<MenuPanel>(nullptr);
    g_menuPanel->addItem(MenuItemBuilder("New").setShortcut("Ctrl+N").build());
    g_menuPanel->addItem(MenuItemBuilder("Open...").setShortcut("Ctrl+O").build());
    g_menuPanel->addSeparator();
    g_menuPanel->addItem(MenuItemBuilder("Exit").setShortcut("Alt+F4").build());
    g_menuPanel->recalculateSize();
    g_menuPanel->setPosition(600, 500);
    // 白底视觉验证：菜单改用浅色主题，避免深色面板在白色背景上过于突兀。
    g_menuPanel->setColorProperty(PropertyNames::kBackground, SColor(255, 255, 255, 255));
    g_menuPanel->setColorProperty(PropertyNames::kBorder, SColor(208, 215, 226, 255));
    g_menuPanel->setColorProperty(PropertyNames::kBorderHover, SColor(208, 215, 226, 255));
    g_menuPanel->setColorProperty(PropertyNames::kBorderDisabled, SColor(226, 232, 240, 255));
    g_menuPanel->setColorProperty(PropertyNames::kStateHover, SColor(232, 240, 254, 255));
    g_menuPanel->setColorProperty(PropertyNames::kStateDisabled, SColor(248, 250, 252, 255));
    g_menuPanel->setColorProperty(PropertyNames::kText, SColor(31, 41, 55, 255));
    g_menuPanel->setColorProperty(PropertyNames::kTextHover, SColor(31, 41, 55, 255));
    g_menuPanel->setColorProperty(PropertyNames::kTextDisabled, SColor(148, 163, 184, 255));
    g_menuPanel->setColorProperty(PropertyNames::kShadowColor, SColor(0, 0, 0, 51));
    g_menuPanel->setFloatProperty(PropertyNames::kShadowRadius, 0.0f);
    BENCH->addControl(g_menuPanel);
    g_menuPanel->show();

    // 真实 ContextMenu 视觉验证：Popup 外层不得再画矩形阴影。
    g_contextMenu = make_shared<ContextMenu>(nullptr);
    g_contextMenu->addItem("Copy", [](shared_ptr<MenuItem>) {});
    g_contextMenu->addItem("Paste", [](shared_ptr<MenuItem>) {});
    g_contextMenu->addSeparator();
    g_contextMenu->addItem("Delete", [](shared_ptr<MenuItem>) {});
    if (auto panel = g_contextMenu->getMenuPanel()) {
        panel->setColorProperty(PropertyNames::kBackground, SColor(255, 255, 255, 255));
        panel->setColorProperty(PropertyNames::kBorder, SColor(208, 215, 226, 255));
        panel->setColorProperty(PropertyNames::kStateHover, SColor(232, 240, 254, 255));
        panel->setColorProperty(PropertyNames::kText, SColor(31, 41, 55, 255));
        panel->setColorProperty(PropertyNames::kTextHover, SColor(31, 41, 55, 255));
        panel->setColorProperty(PropertyNames::kShadowColor, SColor(0, 0, 0, 51));
    }
    g_contextMenu->show(900, 500);

    // 预先创建、隐藏；后续帧显示以验证缓存 miss 与柔光梯度（避免 addControl 延迟帧）。
    g_panelC = make_shared<Panel>(nullptr, SRect(800, 300, 150, 80));
    g_panelC->setTransparent(false);
    g_panelC->setBorderVisible(false);
    g_panelC->setCornerRadius(9.0f);
    g_panelC->setColorProperty(PropertyNames::kBackground, SColor(235, 235, 235, 255));
    g_panelC->setColorProperty(PropertyNames::kBoxShadowColor, SColor(0, 0, 0, 64));
    g_panelC->setFloatProperty(PropertyNames::kBoxShadowRadius, 12.0f);
    g_panelC->setFloatProperty(PropertyNames::kBoxShadowSpread, 0.0f);
    BENCH->addControl(g_panelC);
    g_panelC->hide();

    // hover 联动演示：鼠标移入后阴影加深/变大，移出恢复。
    g_hoverPanel = make_shared<HoverShadowPanel>(nullptr, SRect(800, 120, 150, 80));
    BENCH->addControl(g_hoverPanel);
}

} // namespace

class SoftShadowApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_soft_shadow");
        BENCH->setOnInitial([](shared_ptr<Bench>) { setupControls(); });
        return true;
    }

    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }

    void onRender() override {
        if (g_frame == 4) {
            // 恢复默认半径，验证柔光阴影可见
            if (g_menuPanel) g_menuPanel->setFloatProperty(PropertyNames::kShadowRadius, 8.0f);
        }
        if (g_frame == 5 && g_panelC && !g_panelC->getVisible()) {
            g_panelC->show();
        }
        if (g_frame == 5 && g_menuPanel) {
            // 视觉验证：制造一次 item hover，确认 hover 背景不会覆盖菜单左边框。
            g_menuPanel->setHoveredIndex(0);
        }

        GET_RENDERDEVICE->setDrawColor(SColor(1.0f, 1.0f, 1.0f, 1.0f));
        GET_RENDERDEVICE->clear();
        BENCH->draw();

        // 绘制后读取：raylib 仅帧内可读；SDL/SFML 读取上一帧，测试场景静态，两者均稳定。
        if (g_frame == 3 && !g_checkedPixels) {
            g_checkedPixels = true;

            // 硬边 spread=6：控件外侧 3px 应有阴影，外侧 9px 应回到背景。
            uint32_t outside3 = pixelAt(94, 150);
            uint32_t outside9 = pixelAt(88, 150);
            uint32_t inside = pixelAt(150, 150);
            CHECK(redOf(outside3) < redOf(kBG) - 10, "blur=0 hard shadow darkens 3px outside");
            CHECK(redOf(outside9) >= redOf(kBG) - 2, "blur=0 hard shadow ends at spread");
            // 阴影不得覆盖控件内容
            CHECK(redOf(inside) == 235, "shadow does not cover control background");
            // B2：shadow-radius=0 关闭菜单投影
            CHECK(redOf(pixelAt(596, 520)) >= redOf(kBG) - 2, "menu shadow-radius=0 disables shadow");
        }

        if (g_frame == 2 && !g_checkedCache) {
            g_checkedCache = true;
            auto* cache = ShadowRenderer::getCache(g_uiInstance);
            CHECK(cache != nullptr, "shadow cache created");
            if (cache) {
                // 两个同规格面板共享同一 9-slice 纹理；另有 Label 字形柔光与 ContextMenu 阴影。
                CHECK(cache->textureCount() >= 2, "equal rect shadows share one texture");
                CHECK(cache->usedBytes() > 0, "cache accounts texture bytes");
            }
        }

        if (g_frame == 7 && !g_checkedSoft) {
            g_checkedSoft = true;
            if (!g_shotPath.empty()) {
                int cw = 0, ch = 0;
                std::vector<uint8_t> cap(1400u * 900u * 4u);
                if (UICornerstone_CaptureViewport(g_uiInstance, cap.data(), &cw, &ch) &&
                    cw > 0 && ch > 0) {
                    UICornerstone_SavePixelsToFile(cap.data(), cw, ch, g_shotPath.c_str());
                }
                g_shotPath.clear();
            }
            int softEdge = redOf(pixelAt(796, 340));
            int softFar = redOf(pixelAt(770, 340));
            int menuEdge = redOf(pixelAt(596, 520));
            CHECK(softEdge < redOf(kBG), "blur>0 soft shadow has visible tail");
            CHECK(softFar >= redOf(kBG) - 2, "blur>0 shadow ends at support");
            CHECK(menuEdge < redOf(kBG) - 1, "menu shadow-radius=8 draws soft shadow");
            auto* cache = ShadowRenderer::getCache(g_uiInstance);
            if (cache) {
                CHECK(cache->textureCount() >= 3, "distinct shadow specs create distinct textures");
                CHECK(cache->hitCount() > 10, "cached shadows are reused across frames");
            }
        }

        if (g_frame == 8 && !g_summaryPrinted) {
            g_summaryPrinted = true;
            auto* cache = ShadowRenderer::getCache(g_uiInstance);
            // LRU/预算：设置极小预算后新增规格，必须收敛到预算内，超限走程序化回退。
            if (cache) {
                cache->setBudgetBytes(1024);
                ShadowRenderer::ShadowSpec tiny;
                tiny.enabled = true;
                tiny.color = SColor(0, 0, 0, 128);
                tiny.blur = 3.0f;
                tiny.cornerRadius = 0.0f;
                cache->drawRectShadow(GET_RENDERDEVICE, tiny, SRect(0, 0, 10, 10), 1.0f, 1.0f);
                CHECK(cache->usedBytes() <= 1024, "cache budget evicts oversized entries");
                cache->setBudgetBytes(32u * 1024u * 1024u);
            }
            printf("---- soft shadow: pass=%d fail=%d textures=%zu hits=%zu misses=%zu ----\n",
                   g_pass, g_fail,
                   cache ? cache->textureCount() : 0,
                   cache ? cache->hitCount() : 0,
                   cache ? cache->missCount() : 0);
        }

        ++g_frame;
    }

    void onQuit() override {
        TestUtil::log("soft shadow test quit");
    }
};

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "shot=", 5) == 0) g_shotPath = argv[i] + 5;
    }
    return TestRunMain<SoftShadowApp>(argc, argv);
}
