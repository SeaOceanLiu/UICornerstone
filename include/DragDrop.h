#ifndef DRAGDROP_H
#define DRAGDROP_H
// P0-94：通用拖拽框架（Phase 1 核心 + Phase 2 内置行为）——引擎内部钩子
#include "EventTypes.h"

class UIContext;
class Control;
class Event;   // 定义见 StateMachine.h

// 事件循环前置（owner/vp 队列弹出后、watcher 之前调用；返回 true=已消费）
bool DragPreHandleEvent(UIContext* instance, const Event& evt);
// 浮层渲染（RenderOverlays 内调用：ghost + 落点指示）
void DragRenderOverlay(UIContext* instance);
// 实例销毁清理
void DragRelease(UIContext* instance);
// 只读查询（属性系统）
bool DragQueryActive(const Control* ctl);
// drag-over 回调内设置接受/拒绝
void DragSetAccept(const Control* ctl, bool accept);

#endif // DRAGDROP_H
