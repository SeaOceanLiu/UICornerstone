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
    std::string GetNodeId() const;              // treeNode.id
    void* GetNodeUserData() const;              // treeNode.userData

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