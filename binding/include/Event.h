// UICornerstone C++ Binding — 事件包装（类型安全回调数据访问）
// 许可证 MIT。仅依赖 C ABI 头。
#ifndef UICORNERSTONE_BINDING_EVENT_H
#define UICORNERSTONE_BINDING_EVENT_H

#include <string>
#include "UICornerstoneAPI.h"

class Event {
public:
    explicit Event(const UIEventData* raw) : m_raw(raw) {}

    // 事件名鉴别
    std::string GetName() const;

    // ── 具名访问器（按事件名推导数据类型，避免接触 union）──
    bool IsClick() const;                       // "click"：无负载
    bool IsValueChanged() const;                // "value-changed"：floatVal
    float GetValueChanged() const;
    double GetValueChangedDouble() const;       // NumericUpDown：doubleVal
    bool IsTextChanged() const;                 // "text-changed"：strVal
    std::string GetTextChanged() const;
    bool IsSelectionChanged() const;            // "selection-changed"：{idx,val}
    int GetSelectedIndex() const;
    std::string GetSelectedValue() const;
    bool IsCheckChanged() const;                // "check-changed"：intVal
    int GetCheckState() const;
    bool IsColorChanged() const;                // "color-changed"（ColorPicker 经 C 回调触发）
    UIColor GetChangedColor() const;
    bool IsExpressionChanged() const;           // "expression-changed"（ColorPicker）：strVal
    std::string GetExpression() const;
    bool IsPositionChanged() const;             // "position-changed"：floatVal
    float GetPositionChanged() const;
    bool IsMoved() const;                       // "moved"：floatVal
    float GetMovedPosition() const;
    bool IsConfirm() const;                     // "confirm"：无负载
    bool IsCancel() const;                      // "cancel"：无负载
    bool IsClose() const;                       // "close"：intVal = DialogResult
    int GetCloseResult() const;
    bool IsListSelectionChanged() const;        // "list-selection-changed"：grid{row,col}
    bool IsItemClick() const;                   // "item-click"：grid{row,col}
    bool IsColumnSort() const;                  // "column-sort"：grid{row=col,col=asc}
    bool IsAnimationEnded() const;              // "animation-ended"：intVal=m_id
    int  GetAnimationEndedId() const;
    bool IsStatusItemClick() const;             // "status-item-click"：intVal=段索引
    int  GetStatusItemIndex() const;
    bool IsTabChanged() const;                  // "tab-changed"：intVal=新页索引
    int  GetTabChangedIndex() const;
    int  GetGridRow() const;                    // grid 通用访问
    int  GetGridCol() const;
    int  GetGridAsc() const;
    bool IsEnter() const;                       // "enter"：无负载
    bool IsSelect() const;                      // "select"：treeNode
    bool IsExpand() const;                      // "expand"：treeNode
    bool IsCollapse() const;                    // "collapse"：treeNode
    std::string GetNodeId() const;              // treeNode.id（仅 select/expand/collapse；其他事件返回空串）
    void* GetNodeUserData() const;              // treeNode.userData

    // ── P0-105：拖拽框架（drag.payload / drag.hint / dropped）──
    bool IsDragStart() const;                   // "drag-start"
    bool IsDragOver() const;                    // "drag-over"
    bool IsDrop() const;                        // "drop"
    bool IsDragEnd() const;                     // "drag-end"
    bool IsReordered() const;                   // "reordered"（tree/list 内置换序）
    bool IsTabReordered() const;                // "tab-reordered"（tab 页内置换序）
    int GetDragKind() const;                    // UI_DRAG_KIND_*
    std::string GetDragId() const;              // 源元素 id（drag 事件请用此访问器，勿用 GetNodeId）
    int GetDragIndex() const;                   // 源元素索引（行/页；-1 无）
    std::string GetDragText() const;            // 源元素文本
    void* GetDragUserData() const;              // 源元素 userData（透传）
    UIControlHandle GetDragSource() const;      // 源控件句柄
    int GetHintKind() const;                    // UI_DROP_HINT_*（新父节点推导：BEFORE/AFTER=target 父；INTO=target）
    int GetHintIndex() const;                   // 落点索引
    std::string GetHintTargetId() const;        // 落点目标元素 id
    int GetDropped() const;                     // drag-end：1=已投放；0=中止/被拒

    // ── 通用原始访问 ──
    const char* GetNameRaw() const { return m_raw ? m_raw->eventName : nullptr; }
    int     GetIntVal() const;
    float   GetFloatVal() const;
    double  GetDoubleVal() const;
    const char* GetStrVal() const;
    void*   GetPtrVal() const;
    const UIEventData* Raw() const { return m_raw; }

private:
    const UIEventData* m_raw;
};

#endif