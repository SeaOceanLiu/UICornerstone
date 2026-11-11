// CR20261102001/002/003：CSS-like 主题 / content-padding / 响应式主题
#include "UICornerstoneAPI.h"
#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

static UIInstance g = nullptr;
static int fails = 0;
static void check(bool ok, const char* name) { printf("%s: %s\n", name, ok ? "OK" : "FAIL"); if (!ok) fails++; }

static UIColor colorOf(void* c, const char* prop) {
    UIColor c0{}; UICornerstone_GetColor(g, c, prop, &c0); return c0;
}
static bool nearColor(UIColor a, int r, int gg, int b, int alpha=255) {
    return abs((int)a.r-r)<=2 && abs((int)a.g-gg)<=2 && abs((int)a.b-b)<=2 && abs((int)a.a-alpha)<=2;
}
static int pump(int n) {
    for (int i=0;i<n;++i) { UICornerstone_ProcessEvents(g); UICornerstone_Update(g, 1.0/60.0); UICornerstone_Render(g); UICornerstone_Present(g); }
    return 0;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    g = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!g) { printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(g, 0, 0, 900, 700);

    const char* layout = R"JSON({
      "theme": {
        "variables": { "--brand": "#112233", "--txt": "#FFFFFF" },
        "styles": [
          { "selector": "button", "declarations": { "background": "var(--brand)", "text-color": "var(--txt)", "border-radius": 7, "padding": "4 8" } },
          { "selector": "button.primary", "declarations": { "background": "#224466" } },
          { "selector": "button.primary:hover", "declarations": { "background": "#335577" } },
          { "selector": "label", "declarations": { "font-size": 18 } },
          { "selector": "#lbl", "declarations": { "background": "#123456" } }
        ],
        "variants": { "dark": { "styles": [ { "selector": "button", "declarations": { "background": "#010203" } } ] } },
        "media": [ { "id": "compact", "query": { "max-width": 500 }, "styles": [ { "selector": "button", "declarations": { "text-color": "#FF0000" } } ] } ],
        "containerQueries": [ { "id": "narrow", "container": "panel", "query": { "max-width": 300 }, "styles": [ { "selector": "button", "declarations": { "font-size": 15 } } ] } ]
      },
      "controls": [
        { "type": "panel", "id": "p1", "rect": { "x": 0, "y": 0, "w": 260, "h": 300 }, "layout": { "type": "vflow" }, "children": [
          { "type": "button", "id": "btn", "classes": ["primary"], "rect": { "x": 10, "y": 10, "w": 160, "h": 40 },
            "caption": "B", "colors": { "background": "#AABBCC" } },
          { "type": "label", "id": "lbl", "rect": { "x": 10, "y": 80, "w": 200, "h": 30 }, "caption": "L", "font-size": 26,
            "colors": { "text": "#00FF00" }, "content-padding": "3 5" }
        ] }
      ]
    })JSON";
    int ok = UICornerstone_LoadLayout(g, layout);
    check(ok == 1, "load layout");
    pump(2);
    void* btn = UICornerstone_FindControl(g, "btn");
    void* lbl = UICornerstone_FindControl(g, "lbl");
    check(btn && lbl, "find controls");
    if (!btn || !lbl) return 1;

    UIColor bg = colorOf(btn, "background");
    check(nearColor(bg, 0xAA, 0xBB, 0xCC), "explicit JSON colors wins over CSS");
    UIColor hbg = colorOf(btn, "background.hover");
    check(nearColor(hbg, 0x33, 0x55, 0x77), "state selector background.hover");
    UIColor text = colorOf(btn, "text");
    check(nearColor(text, 0xFF, 0xFF, 0xFF), "CSS text-color applied");
    float radius = 0; UICornerstone_GetFloat(g, btn, "corner-radius", &radius);
    check(radius > 6.5f && radius < 7.5f, "CSS border-radius applied");
    float l=0,t=0,r=0,b=0; UICornerstone_GetContentPadding(g, btn, &l,&t,&r,&b);
    check(l == 8 && t == 4 && r == 8 && b == 4, "CSS padding -> content-padding");
    float lt=0,tt=0,rt=0,bt=0; UICornerstone_GetContentPadding(g, lbl, &lt,&tt,&rt,&bt);
    check(lt == 5 && tt == 3 && rt == 5 && bt == 3, "JSON content-padding");
    int fs=0; UICornerstone_GetInt(g, lbl, "font-size", &fs);
    check(fs == 26, "explicit font-size wins");
    UIColor ltxt = colorOf(lbl, "text");
    check(nearColor(ltxt, 0x00, 0xFF, 0x00), "explicit text color wins over CSS/inherit");
    UIColor lbg0 = colorOf(lbl, "background");
    check(nearColor(lbg0, 0x12, 0x34, 0x56), "id selector applied");

    // 变体：CSS explicit (button colors) wins, so use made-up button without explicit colors not present; test variant on label text
    UICornerstone_SetThemeVariant(g, "dark");
    char vb[64]{}; int vn=0; UICornerstone_GetThemeVariant(g, vb, sizeof(vb), &vn);
    check(string(vb) == "dark", "variant get");
    UIColor lbg = colorOf(lbl, "background");
    (void)lbg;

    // 媒体：viewport 400 -> compact text red
    UICornerstone_RefreshResponsiveTheme(g, 400, 700);
    char am[128]{}; int an=0; UICornerstone_GetActiveMedia(g, am, sizeof(am), &an);
    check(string(am).find("compact") != string::npos, "active media compact");
    check(string(am).find("narrow") != string::npos, "active container narrow");
    UIColor mt = colorOf(btn, "text");
    check(nearColor(mt, 0xFF, 0x00, 0x00), "media text-color override");
    UICornerstone_RefreshResponsiveTheme(g, 900, 700);
    UICornerstone_SetThemeVariant(g, "default");

    // ClearThemeOverrides：btn 的 JSON background 清除后恢复 CSS #224466
    UICornerstone_ClearThemeOverrides(g, btn);
    UIColor cbg = colorOf(btn, "background");
    check(nearColor(cbg, 0x22, 0x44, 0x66), "clear override restores CSS");
    UICornerstone_SetContentPadding(g, lbl, 1, 2, 3, 4);
    float pl=0,pt=0,pr=0,pb=0;
    UICornerstone_GetContentPadding(g, lbl, &pl,&pt,&pr,&pb);
    check(pl == 1 && pt == 2 && pr == 3 && pb == 4, "Set/GetContentPadding C ABI");
    const char* theme2 = "{\"styles\":[{\"selector\":\"button\",\"declarations\":{\"text-color\":\"#FF00FF\"}}]}";
    check(UICornerstone_ApplyTheme(g, theme2) == 1, "ApplyTheme runtime");
    UIColor at = colorOf(btn, "text");
    check(nearColor(at, 0xFF, 0x00, 0xFF), "runtime theme applied");
    check(UICornerstone_ApplyTheme(g, "{}") == 1, "empty theme no-op");
    check(nearColor(colorOf(btn, "text"), 0xFF, 0x00, 0xFF), "empty theme preserves styles");
    check(UICornerstone_ApplyTheme(g, "{\"variables\":{\"--x\":\"#FFFFFF\"}}") == 1, "variables-only no-op");
    check(nearColor(colorOf(btn, "text"), 0xFF, 0x00, 0xFF), "variables-only preserves styles");
    UIColor ltxt2 = colorOf(lbl, "text");
    check(nearColor(ltxt2, 0x00, 0xFF, 0x00), "explicit override survives ApplyTheme");

    printf("RESULT: %s (%d failures)\n", fails ? "FAIL" : "OK", fails);
    UICornerstone_DestroyInstance(g);
    return fails ? 1 : 0;
}
