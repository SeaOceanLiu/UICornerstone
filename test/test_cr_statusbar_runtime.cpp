// CR20261110002：通过 C ABI LoadLayout + RegisterAction 验证 StatusBar item action。
#include "UICornerstoneAPI.h"
#include "StatusBar.h"
#include "EventTypes.h"
#include <cstdio>
#include <memory>

using namespace std;
static int g_hits = 0;
static void onStatus(UIControlHandle, void*) { ++g_hits; printf("STATUS ACTION HIT\n"); }

int main() {
    UIInstance inst = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!inst) { printf("FAIL create instance\n"); return 1; }
    UICornerstone_SetViewport(inst, 0, 0, 640, 480);
    UICornerstone_RegisterAction(inst, "onStatus1_1_Click", onStatus, nullptr);
    UICornerstone_RegisterAction(inst, "onStatus1_2_Click", onStatus, nullptr);
    const char* json = R"({
      "controls": [{
        "type":"status-bar", "id":"status_1", "anchor":"bottom-stretch",
        "font":{"size":13}, "font-size":13,
        "rect":{"x":0,"y":300,"w":640,"h":28},
        "items": [
          {"id":"stb_0","text":"Sector1","background-color":"#00A0DFFF",
           "hover-background-color":"##00CADCFF","font-name":"muyao-softbrush","font-size":20,
           "text-color":"#FFFFFFFF","text-shadow":"#FF0000FF",
           "text-shadow-offset-x":4.0,"text-shadow-offset-y":4.0,
           "events":{"onClick":"onStatus1_1_Click"}},
          {"id":"stb_1","text":"Sector2","background-color":"#00A0DFFF",
           "hover-background-color":"##00CADCFF","font-name":"muyao-softbrush","font-size":20,
           "text-color":"#FFFFFFFF","text-shadow":"#FF0000FF",
           "text-shadow-offset-x":4.0,"text-shadow-offset-y":4.0,
           "events":{"onClick":"onStatus1_2_Click"}}
        ]
      }]
    })";
    int ok = UICornerstone_LoadLayout(inst, json);
    printf("load=%d\n", ok);
    auto* bar = dynamic_cast<StatusBar*>(static_cast<Control*>(UICornerstone_FindControl(inst, "status_1")));
    printf("bar=%p\n", (void*)bar);
    if (bar && bar->getStatusItem("stb_0")) {
        auto* it = bar->getStatusItem("stb_0");
        printf("action-bound=%d\n", it->onClick ? 1 : 0);
        auto ev = make_shared<Event>(EventType::MouseDown);
        ev->mouseButton = {bar->getRect().left + it->hitRect.left + 4.f,
                           bar->getRect().top + it->hitRect.top + 4.f, MouseButton::Left};
        bar->handleEvent(ev);
    }
    printf("hits=%d\n", g_hits);
    UICornerstone_DestroyInstance(inst);
    return g_hits == 1 ? 0 : 1;
}
