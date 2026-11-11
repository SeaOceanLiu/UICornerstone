// 用户手册控件截图生成器（开发辅助，临时）
// 用法：manual_shots <layout.json> <out.bmp> <w> <h>
#include "UICornerstoneAPI.h"
#include "nlohmann/json.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

static bool fileGet(const char* path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

int main(int argc, char** argv) {
    if (argc < 5) { printf("usage: %s layout.json out.bmp w h\n", argv[0]); return 2; }
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::string text;
    if (!fileGet(argv[1], text)) { printf("read failed: %s\n", argv[1]); return 1; }
    json doc;
    try { doc = json::parse(text); } catch (const std::exception& e) { printf("json fail: %s\n", e.what()); return 1; }
    json states = doc.value("_states", json::array());
    json visible = doc.value("_visible", json::array());
    json selects = doc.value("_select", json::array());
    json expands = doc.value("_expandAll", json::array());
    json contextMenus = doc.value("_contextMenus", json::array());
    doc.erase("_states"); doc.erase("_visible"); doc.erase("_select"); doc.erase("_expandAll"); doc.erase("_contextMenus");
    int w = atoi(argv[3]), h = atoi(argv[4]);
    UIInstance g = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!g) { printf("instance failed\n"); return 1; }
    UICornerstone_SetViewport(g, 0, 0, (float)w, (float)h);
    if (!UICornerstone_LoadLayout(g, doc.dump().c_str())) { printf("layout failed\n"); return 1; }
    for (const auto& v : visible) {
        if (!v.is_object()) continue;
        UIControlHandle c = UICornerstone_FindControl(g, v.value("id", "").c_str());
        if (c) UICornerstone_SetBool(g, c, "visible", v.value("on", true) ? 1 : 0);
    }
    for (const auto& s : states) {
        if (!s.is_object()) continue;
        UIControlHandle c = UICornerstone_FindControl(g, s.value("id", "").c_str());
        if (c) UICornerstone_SetEnum(g, c, "state", s.value("state", "normal").c_str());
    }
    for (const auto& s : selects) {
        if (!s.is_object()) continue;
        UIControlHandle c = UICornerstone_FindControl(g, s.value("id", "").c_str());
        if (c) UICornerstone_SetInt(g, c, "selected-index", s.value("index", 0));
    }
    for (const auto& s : expands) {
        if (!s.is_string()) continue;
        UIControlHandle c = UICornerstone_FindControl(g, s.get<std::string>().c_str());
        if (c) UICornerstone_TreeViewExpandAll(g, c);
    }
    for (const auto& s : contextMenus) {
        if (!s.is_object()) continue;
        UIControlHandle c = UICornerstone_FindControl(g, s.value("id", "").c_str());
        if (c) UICornerstone_ContextMenuShow(g, c, s.value("x", 100.0f), s.value("y", 100.0f));
    }
    for (int i = 0; i < 6; ++i) {
        UICornerstone_ProcessEvents(g);
        UICornerstone_Update(g, 1.0 / 60.0);
    }
    UICornerstone_Render(g);
    int ow = 0, oh = 0;
    static std::vector<unsigned char> px(1600 * 1000 * 4);
    if (!UICornerstone_CaptureBench(g, px.data(), &ow, &oh) || ow <= 0 || oh <= 0) {
        printf("capture failed\n"); return 1;
    }
    bool ok = UICornerstone_SavePixelsToFile(px.data(), ow, oh, argv[2]) != 0;
    printf("%s %dx%d -> %d\n", argv[2], ow, oh, ok ? 1 : 0);
    UICornerstone_DestroyInstance(g);
    return ok ? 0 : 1;
}
