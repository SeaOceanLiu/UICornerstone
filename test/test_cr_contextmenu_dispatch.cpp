// CR20261110003：右键 context-menu 统一派发（Button/EditBox/父子优先级）。
#include "UICornerstoneAPI.h"
#include "ContextMenu.h"
#include "ControlBase.h"
#include "UIContext.h"
#include "Bench.h"
#include "EventTypes.h"
#include <cstdio>
#include <cstring>
#include <memory>

static int g_fail = 0;
static ContextMenu* cmOf(UIControlHandle h) {
    return dynamic_cast<ContextMenu*>(static_cast<Control*>(h));
}
static void check(bool ok, const char* name) {
    printf("%s: %s\n", name, ok ? "OK" : "FAIL");
    if (!ok) ++g_fail;
}
static void pushRight(UIInstance inst, float x, float y) {
    UIInstance ctx = inst;
    if (!ctx || !ctx->bench) return;
    auto ev = std::make_shared<Event>(EventType::MouseDown);
    ev->mouseButton = {x, y, MouseButton::Right};
    ctx->bench->handleEvent(ev);
}
static UIControlHandle makeMenu(UIInstance inst, const char* caption) {
    UIControlHandle h = UICornerstone_CreateContextMenu(inst, 0, 0, 10, 10, 1, 1);
    UICornerstone_ContextMenuAddItem(inst, h, caption, nullptr);
    return h;
}

int main() {
    UIInstance inst = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!inst) { printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(inst, 0, 0, 800, 600);

    // Button 根控件
    UIControlHandle btn = UICornerstone_CreateButton(inst, "btn", 100, 100, 120, 40, 1, 1);
    UIControlHandle m1 = makeMenu(inst, "btn-menu");
    check(UICornerstone_SetPtr(inst, btn, "context-menu", m1) == 1, "bind button menu");
    pushRight(inst, 120, 120);
    check(cmOf(m1) && cmOf(m1)->isPopupVisible(), "button right opens menu");
    UICornerstone_ContextMenuClose(inst, m1);

    // EditBox 根控件（原先 handleEvent 不进入基类）
    UIControlHandle eb = UICornerstone_CreateEditBox(inst, 100, 200, 160, 32, 1, 1);
    UIControlHandle m2 = makeMenu(inst, "edit-menu");
    check(UICornerstone_SetPtr(inst, eb, "context-menu", m2) == 1, "bind edit menu");
    pushRight(inst, 120, 216);
    check(cmOf(m2) && cmOf(m2)->isPopupVisible(), "editbox right opens menu");
    UICornerstone_ContextMenuClose(inst, m2);

    // 父 Panel + 子 Button：子菜单优先，父菜单不打开
    UIControlHandle panel = UICornerstone_CreatePanel(inst, 300, 100, 240, 120, 1, 1);
    UIControlHandle child = UICornerstone_CreateButton(inst, "child", 20, 20, 120, 40, 1, 1);
    UICornerstone_AddChildControl(inst, panel, child);
    UIControlHandle mp = makeMenu(inst, "parent-menu");
    UIControlHandle mc = makeMenu(inst, "child-menu");
    UICornerstone_SetPtr(inst, panel, "context-menu", mp);
    UICornerstone_SetPtr(inst, child, "context-menu", mc);
    pushRight(inst, 340, 140);
    check(cmOf(mc) && cmOf(mc)->isPopupVisible(), "child menu opens");
    check(cmOf(mp) && !cmOf(mp)->isPopupVisible(), "parent menu stays closed");
    UICornerstone_ContextMenuClose(inst, mc);

    // 子无菜单时右键父菜单兜底（子 Button 会吞右键，但不能阻止父菜单）
    UICornerstone_ContextMenuClose(inst, mc);
    UICornerstone_SetPtr(inst, child, "context-menu", nullptr);
    pushRight(inst, 340, 140);
    check(cmOf(mp) && cmOf(mp)->isPopupVisible(), "parent menu fallback when child has none");
    UICornerstone_ContextMenuClose(inst, mp);

    // 覆盖其它会自行消费右键/不进入基类的控件
    UIControlHandle genericMenu = makeMenu(inst, "generic-menu");
    auto testRight = [&](UIControlHandle h, float x, float y, const char* label) {
        UICornerstone_SetPtr(inst, h, "context-menu", genericMenu);
        pushRight(inst, x, y);
        check(cmOf(genericMenu) && cmOf(genericMenu)->isPopupVisible(), label);
        UICornerstone_ContextMenuClose(inst, genericMenu);
    };
    UIControlHandle lv = UICornerstone_CreateListView(inst, 480, 80, 200, 100, 1, 1);
    testRight(lv, 500, 100, "listview right opens menu");
    UIControlHandle sl = UICornerstone_CreateSlider(inst, 480, 220, 200, 30, 0, 100, 50, 1, 1);
    testRight(sl, 500, 235, "slider right opens menu");
    UIControlHandle sb = UICornerstone_CreateScrollBar(inst, 480, 280, 30, 100, 1, 1, 1);
    testRight(sb, 495, 300, "scrollbar right opens menu");
    UIControlHandle sp = UICornerstone_CreateSplitter(inst, 480, 400, 120, 12, 0, 1, 1);
    testRight(sp, 500, 405, "splitter right opens menu");
    UIControlHandle cbx = UICornerstone_CreateComboBox(inst, 100, 300, 180, 32, 1, 1);
    testRight(cbx, 120, 316, "combobox right opens menu");
    UIControlHandle nud = UICornerstone_CreateNumericUpDown(inst, 300, 300, 120, 32, 1, 1);
    testRight(nud, 320, 316, "numericupdown right opens menu");
    UIControlHandle tv = UICornerstone_CreateTreeView(inst, 100, 350, 160, 120, 1, 1);
    testRight(tv, 120, 370, "treeview right opens menu");
    UIControlHandle tab = UICornerstone_CreateTabControl(inst, 300, 350, 200, 120, 1, 1);
    testRight(tab, 320, 370, "tabcontrol right opens menu");
    UIControlHandle status = UICornerstone_CreateStatusBar(inst, 100, 500, 200, 28, 1, 1);
    testRight(status, 120, 514, "statusbar right opens menu");

    // CR20261110004：纯显示控件使用 context-menu 几何命中
    UIControlHandle ani = UICornerstone_CreateAnimation(inst, nullptr, 480, 450, 120, 80, 1, 1);
    testRight(ani, 500, 470, "animation right opens menu");
    UIControlHandle actor = UICornerstone_CreateActor(inst, nullptr, 480, 540, 120, 40, 1, 1);
    testRight(actor, 500, 560, "actor right opens menu");
    // HandleControl V1 明确不纳入：即使绑定也不弹出（动画本身解绑，避免误测下层动画菜单）
    UICornerstone_SetPtr(inst, ani, "context-menu", nullptr);
    UIControlHandle handleMenu = makeMenu(inst, "handle-menu");
    UIControlHandle handle = UICornerstone_CreateHandleControl(inst, ani, 480, 450, 20, 20, 1, 1);
    UICornerstone_SetPtr(inst, handle, "context-menu", handleMenu);
    pushRight(inst, 490, 460);
    check(cmOf(handleMenu) && !cmOf(handleMenu)->isPopupVisible(), "handlecontrol menu stays closed");

    UICornerstone_DestroyInstance(inst);
    printf("RESULT: %s (%d failures)\n", g_fail ? "FAIL" : "OK", g_fail);
    return g_fail ? 1 : 0;
}
