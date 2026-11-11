// CR20261110001：EditBox 后置 content-padding 的 offset 刷新回归。
#include "UICornerstoneAPI.h"
#include <cstdio>
#include <cmath>
#define protected public
#include "EditBox.h"
#undef protected

int main() {
    int fails = 0;
    auto check = [&](bool ok, const char* name) { std::printf("%s: %s\n", name, ok ? "OK" : "FAIL"); if (!ok) ++fails; };
    UIInstance inst = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!inst) { std::printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(inst, 0, 0, 800, 600);
    UIControlHandle h = UICornerstone_CreateEditBox(inst, 100, 100, 200, 32, 1, 1);
    check(h != nullptr, "create editbox");
    if (h) {
        UICornerstone_SetString(inst, h, "placeholder", "Search");
        UICornerstone_SetContentPadding(inst, h, 4, 6, 4, 6);
        auto* eb = dynamic_cast<EditBox*>(static_cast<Control*>(h));
        check(eb != nullptr, "cast editbox");
        if (eb) {
            float fh = eb->getTextRenderer() && eb->getFont()
                ? (float)eb->getTextRenderer()->getFontHeight(eb->getFont())
                : eb->m_fontSize * eb->getScaleYY();
            SRect cr = eb->getContentDrawRect();
            float expected = (cr.height - fh) / 2.0f;
            check(std::fabs(eb->m_textOffsetY - expected) < 0.1f,
                  "content-padding refreshes text offset without focus");
            // 后置 placeholder 不应破坏 offset
            UICornerstone_SetString(inst, h, "placeholder", "Search updated");
            check(std::fabs(eb->m_textOffsetY - expected) < 0.1f, "placeholder refresh keeps offset");
        }
    }
    UICornerstone_DestroyInstance(inst);
    std::printf("RESULT: %s (%d failures)\n", fails ? "FAIL" : "OK", fails);
    return fails ? 1 : 0;
}
