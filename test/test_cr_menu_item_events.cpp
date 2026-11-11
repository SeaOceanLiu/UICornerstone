// CR20261109002/9003：StatusBar item 与独立 ContextMenu 声明式 onClick 解析回归。
#include "LayoutParser.h"
#include "StatusBar.h"
#include "ContextMenu.h"
#include "Menu.h"
#include "EventTypes.h"
#include <cstdio>
#include <stdexcept>

using namespace std;

static int g_status = 0;
static int g_legacy = 0;
static int g_ctx = 0;
static int g_fail = 0;

static shared_ptr<Event> mouseDown(float x, float y) {
    auto ev = make_shared<Event>(EventType::MouseDown);
    ev->mouseButton = {x, y, MouseButton::Left};
    return ev;
}

static void check(bool ok, const char* name) {
    printf("%s: %s\n", name, ok ? "OK" : "FAIL");
    if (!ok) ++g_fail;
}

int main() {
    LayoutParser parser(nullptr);
    parser.registerHandler("onStatus", [](shared_ptr<Control>) { ++g_status; });
    parser.registerHandler("onLegacy", [](shared_ptr<Control>) { ++g_legacy; });
    parser.registerHandler("onCtx", [](shared_ptr<Control>) { ++g_ctx; });
    parser.registerHandler("onThrow", [](shared_ptr<Control>) { throw std::runtime_error("test exception"); });

    const char* statusJson = R"({
      "controls": [{
        "type": "status-bar", "id": "sb",
        "rect": {"x":0,"y":0,"w":400,"h":24},
        "items": [
          {"id":"a","text":"A","events":{"onClick":"onStatus"}},
          {"id":"b","text":"B","onClick":"onLegacy"},
          {"id":"c","text":"C","events":{"onClick":"onStatus"},"onClick":"onLegacy"},
          {"id":"d","text":"D","events":{"onClick":"missing"}},
          {"id":"m","text":"Menu","menu":{"items":[{"caption":"x"}]},"events":{"onClick":"onStatus"}}
        ]
      }]
    })";
    auto root = parser.parseLayout(statusJson);
    check(root != nullptr, "status layout parsed");
    auto bar = dynamic_pointer_cast<StatusBar>(parser.findControlById("sb"));
    check(bar != nullptr, "status bar found");
    if (bar) {
        auto* a = bar->getStatusItem("a");
        auto* b = bar->getStatusItem("b");
        auto* c = bar->getStatusItem("c");
        auto* d = bar->getStatusItem("d");
        auto* m = bar->getStatusItem("m");
        check(a && a->onClick, "events.onClick bound");
        check(b && b->onClick, "legacy onClick bound");
        check(c && c->onClick, "events precedence bound");
        check(d && !d->onClick, "missing handler leaves callback empty");
        check(m && m->onClick, "menu item action parsed (not invoked by StatusBar)");
        g_status = 0; g_legacy = 0;
        auto clickItem = [&](StatusItem* it) {
            if (!it) return;
            const float x = bar->getRect().left + it->hitRect.left + it->hitRect.width / 2.f;
            const float y = bar->getRect().top + it->hitRect.top + it->hitRect.height / 2.f;
            bar->handleEvent(mouseDown(x, y));
        };
        clickItem(a); clickItem(b); clickItem(c); clickItem(d);
        check(g_status == 2, "events handler count (a,c)");
        check(g_legacy == 1, "legacy handler count (b)");
    }

    const char* ctxJson = R"({
      "controls": [{
        "type": "context-menu", "id": "ctx",
        "items": [
          {"caption":"Copy","shortcut":"Ctrl+C","events":{"onClick":"onCtx"}},
          {"type":"separator"},
          {"caption":"Paste","onClick":"onLegacy"},
          {"caption":"Both","events":{"onClick":"onCtx"},"onClick":"onLegacy"},
          {"caption":"Nested","items":[{"caption":"child","events":{"onClick":"onCtx"}}]},
          {"caption":"Missing","events":{"onClick":"missing"}},
          {"caption":"Throw","events":{"onClick":"onThrow"}}
        ]
      }]
    })";
    auto ctxRoot = parser.parseLayout(ctxJson);
    check(ctxRoot != nullptr, "context menu layout parsed");
    auto cm = dynamic_pointer_cast<ContextMenu>(parser.findControlById("ctx"));
    check(cm != nullptr, "context menu found");
    if (cm) {
        auto items = cm->getMenuPanel()->getItems();
        check(items.size() == 7, "context menu item count (6 normal + separator)");
        if (items.size() == 7) {
            check(items[0]->getCaption() == "Copy" && items[0]->getOnClick(), "context first item bound");
            check(items[1]->getType() == MenuItemType::Separator, "context separator");
            check(items[2]->getCaption() == "Paste" && items[2]->getOnClick(), "context legacy onClick");
            check(items[3]->getCaption() == "Both" && items[3]->getOnClick(), "context both keys parsed");
            check(items[4]->getCaption() == "Nested" && items[4]->getOnClick(), "nested parent leaf close-only");
            check(items[5]->getCaption() == "Missing" && items[5]->getOnClick(), "missing handler still close-only");
            check(items[6]->getCaption() == "Throw" && items[6]->getOnClick(), "exception handler still close-only");
        }
        if (items.size() >= 1 && items[0]->getOnClick()) items[0]->getOnClick()(items[0]);
        if (items.size() >= 3 && items[2]->getOnClick()) items[2]->getOnClick()(items[2]);
        if (items.size() >= 4 && items[3]->getOnClick()) items[3]->getOnClick()(items[3]);
        if (items.size() >= 7 && items[6]->getOnClick()) items[6]->getOnClick()(items[6]);   // handler 抛异常：捕获后仍关闭
        check(g_ctx == 2, "context events handler count (Copy/Both)");
        check(g_legacy == 2, "context legacy handler count (Paste)");
    }

    printf("RESULT: %s (%d failures)\n", g_fail ? "FAIL" : "OK", g_fail);
    return g_fail ? 1 : 0;
}
