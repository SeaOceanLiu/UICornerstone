// CR20261105003：窗口可见性 C ABI 回归（独立窗口 / 子视口 / 幂等 / 空指针）
#include "UICornerstoneAPI.h"
#include <cstdio>
#include <thread>
#include <chrono>
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    int fails = 0;
    auto check = [&](bool ok, const char* name) { printf("%s: %s\n", name, ok ? "OK" : "FAIL"); if (!ok) fails++; };
    UIInstance g = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!g) { printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(g, 0, 0, 640, 480);
    int v = 0;
    check(UICornerstone_GetWindowVisible(g, &v) == 1 && v == 1, "initial visible");
    check(UICornerstone_SetWindowVisible(g, 0) == 1, "hide");
    check(UICornerstone_GetWindowVisible(g, &v) == 1 && v == 0, "hidden");
    check(UICornerstone_SetWindowVisible(g, 0) == 1, "hide idempotent");
    check(UICornerstone_SetWindowVisible(g, 1) == 1, "show");
    check(UICornerstone_GetWindowVisible(g, &v) == 1 && v == 1, "visible again");
    check(UICornerstone_GetWindowVisible(g, nullptr) == 0, "null visible");
    check(UICornerstone_SetWindowVisible(nullptr, 1) == 0, "null instance");
    check(UICornerstone_SetWindowAlwaysOnTop(g, 1) == 1, "always-on-top on");
    int onTop = 0;
    check(UICornerstone_GetWindowAlwaysOnTop(g, &onTop) == 1 && onTop == 1, "always-on-top get");
    check(UICornerstone_SetWindowAlwaysOnTop(g, 1) == 1, "always-on-top idempotent");
    check(UICornerstone_SetWindowAlwaysOnTop(g, 0) == 1, "always-on-top off");
    check(UICornerstone_SetWindowForeground(g) == 1, "foreground");

    UIInstance vp = UICornerstone_CreateViewport(g, UIRect{0, 0, 100, 100});
    check(vp != nullptr, "create viewport");
    if (vp) {
        check(UICornerstone_SetWindowVisible(vp, 0) == 0, "viewport hide rejected");
        check(UICornerstone_GetWindowVisible(vp, &v) == 0, "viewport get rejected");
        check(UICornerstone_GetWindowVisible(g, &v) == 1 && v == 1, "owner remains visible");
        UICornerstone_DestroyInstance(vp);
    }
    UICornerstone_DestroyInstance(g);

    UIInstanceConfig cfg{}; cfg.structSize = sizeof(cfg);
    cfg.windowFlags = UICORN_WINDOW_FLAG_BORDERLESS | UICORN_WINDOW_FLAG_HIDDEN | UICORN_WINDOW_FLAG_NO_SHADOW;
    UIInstance g2 = UICornerstone_CreateInstanceFromPlugin("sdl3", &cfg);
    check(g2 != nullptr, "borderless+hidden+no-shadow create");
    if (g2) {
        int vis = 1;
        check(UICornerstone_GetWindowVisible(g2, &vis) == 1 && vis == 0, "borderless hidden initial");
        check(UICornerstone_SetWindowVisible(g2, 1) == 1, "borderless show");
        UICornerstone_DestroyInstance(g2);
    }
    UIInstanceConfig cfg2{}; cfg2.structSize = sizeof(cfg2);
    cfg2.windowFlags = UICORN_WINDOW_FLAG_BORDERLESS | UICORN_WINDOW_FLAG_NO_SHADOW;
    UIInstance g3 = UICornerstone_CreateInstanceFromPlugin("sdl3", &cfg2);
    check(g3 != nullptr, "borderless+no-shadow create");
    if (g3) {
        int vis = 1;
        check(UICornerstone_GetWindowVisible(g3, &vis) == 1 && vis == 1, "no-shadow visible after create");
        UICornerstone_DestroyInstance(g3);
    }
    UIInstanceConfig cfg3{}; cfg3.structSize = sizeof(cfg3);
    cfg3.windowFlags = UICORN_WINDOW_FLAG_UTILITY | UICORN_WINDOW_FLAG_BORDERLESS;
    UIInstance g4 = UICornerstone_CreateInstanceFromPlugin("sdl3", &cfg3);
    check(g4 != nullptr, "utility+borderless create");
    if (g4) {
        int vis = 0;
        check(UICornerstone_GetWindowVisible(g4, &vis) == 1 && vis == 1, "utility visible after create");
        check(UICornerstone_SetWindowVisible(g4, 0) == 1, "utility hide");
        check(UICornerstone_SetWindowVisible(g4, 1) == 1, "utility show roundtrip");
        check(UICornerstone_GetWindowVisible(g4, &vis) == 1 && vis == 1, "utility visible after roundtrip");
        check(UICornerstone_SetWindowAlwaysOnTop(g4, 1) == 1, "utility always-on-top");
        check(UICornerstone_SetWindowForeground(g4) == 1, "utility foreground");
        int holdSec = 0;
        for (int i = 1; i < argc; ++i) {
            if (sscanf(argv[i], "hold=%d", &holdSec) == 1 && holdSec > 0) break;
        }
        if (holdSec > 0) {
            printf("HOLD utility window %d s\n", holdSec);
            std::this_thread::sleep_for(std::chrono::seconds(holdSec));
        }
        UICornerstone_DestroyInstance(g4);
    }
    UIInstanceConfig cfg4{}; cfg4.structSize = sizeof(cfg4);
    cfg4.windowFlags = UICORN_WINDOW_FLAG_UTILITY | UICORN_WINDOW_FLAG_BORDERLESS | UICORN_WINDOW_FLAG_NO_SHADOW;
    UIInstance g5 = UICornerstone_CreateInstanceFromPlugin("sdl3", &cfg4);
    check(g5 != nullptr, "utility+borderless+no-shadow create");
    if (g5) {
        int vis = 0;
        check(UICornerstone_GetWindowVisible(g5, &vis) == 1 && vis == 1, "utility+no-shadow visible after create");
        UICornerstone_DestroyInstance(g5);
    }
    UIInstanceConfig cfg5{}; cfg5.structSize = sizeof(cfg5);
    cfg5.windowFlags = UICORN_WINDOW_FLAG_UTILITY | UICORN_WINDOW_FLAG_BORDERLESS
                     | UICORN_WINDOW_FLAG_HIDDEN | UICORN_WINDOW_FLAG_NO_SHADOW;
    UIInstance g6 = UICornerstone_CreateInstanceFromPlugin("sdl3", &cfg5);
    check(g6 != nullptr, "utility+borderless+hidden+no-shadow create");
    if (g6) {
        int vis = 1;
        check(UICornerstone_GetWindowVisible(g6, &vis) == 1 && vis == 0, "utility hidden initial");
        check(UICornerstone_SetWindowVisible(g6, 1) == 1, "utility hidden show");
        check(UICornerstone_GetWindowVisible(g6, &vis) == 1 && vis == 1, "utility hidden visible after show");
        UICornerstone_DestroyInstance(g6);
    }
    printf("RESULT: %s (%d failures)\n", fails ? "FAIL" : "OK", fails);
    return fails ? 1 : 0;
}
