// CR20261111001：Splitter hover 离开/禁用后的 owner 清理。
#include "UICornerstoneAPI.h"
#include "Splitter.h"
#include "Cursor.h"
#include "EventTypes.h"
#include <cstdio>

static int g_fail = 0;
static void check(bool ok, const char* name) {
    std::printf("%s: %s\n", name, ok ? "OK" : "FAIL");
    if (!ok) ++g_fail;
}

int main() {
    UIInstance inst = UICornerstone_CreateInstanceFromPlugin("sdl3", nullptr);
    if (!inst) { std::printf("FAIL instance\n"); return 1; }
    UICornerstone_SetViewport(inst, 0, 0, 800, 600);
    UIControlHandle h = UICornerstone_CreateSplitter(inst, 100, 100, 12, 200, 1, 1, 1);
    auto* sp = dynamic_cast<Splitter*>(static_cast<Control*>(h));
    check(sp != nullptr, "create splitter");
    if (!sp) { UICornerstone_DestroyInstance(inst); return 1; }

    // ControlImpl::update hover 路径：进入 → owner = splitter
    UICornerstone_Debug_SetMousePosition(inst, 106, 150);
    sp->update();
    check(Cursor::getCurrentOwner() == static_cast<Control*>(sp), "hover enter owns resize cursor");

    // 离开 → owner 清理、默认光标
    UICornerstone_Debug_SetMousePosition(inst, 300, 150);
    sp->update();
    check(Cursor::getCurrentOwner() == nullptr, "hover leave restores owner");

    // 再次进入
    UICornerstone_Debug_SetMousePosition(inst, 106, 150);
    sp->update();
    check(Cursor::getCurrentOwner() == static_cast<Control*>(sp), "hover re-enter owns cursor");

    // 拖出 Splitter 后释放：endDrag 按最终位置恢复默认
    auto down = std::make_shared<Event>(EventType::MouseDown);
    down->mouseButton = {106, 150, MouseButton::Left};
    auto move = std::make_shared<Event>(EventType::MouseMove);
    move->mousePos = {180, 150};
    auto upOut = std::make_shared<Event>(EventType::MouseUp);
    upOut->mouseButton = {300, 150, MouseButton::Left};
    sp->handleEvent(down);
    sp->handleEvent(move);
    sp->handleEvent(upOut);
    check(Cursor::getCurrentOwner() == nullptr, "external drag release restores owner");

    // disabled/hidden 清理 owner
    sp->setEnable(false);
    check(Cursor::getCurrentOwner() == nullptr, "disable restores owner");
    sp->setEnable(true);
    SRect r = sp->getDrawRect();
    UICornerstone_Debug_SetMousePosition(inst, r.left + 2.f, r.top + r.height / 2.f);
    sp->update();
    check(Cursor::getCurrentOwner() == static_cast<Control*>(sp), "enable + hover owns cursor");
    sp->setVisible(false);
    check(Cursor::getCurrentOwner() == nullptr, "hide restores owner");

    UICornerstone_Debug_ClearMousePosition(inst);
    UICornerstone_DestroyInstance(inst);
    std::printf("RESULT: %s (%d failures)\n", g_fail ? "FAIL" : "OK", g_fail);
    return g_fail ? 1 : 0;
}
