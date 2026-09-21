// UICornerstone C++ Binding — 事件包装实现
// 许可证 MIT。仅读取 C ABI 的 UIEventData（事件字典见 PropertyNames.h）。
#include "Event.h"
#include "PropertyNames.h"

#include <cstring>

std::string Event::GetName() const {
    return m_raw && m_raw->eventName ? m_raw->eventName : "";
}

// ── 事件名鉴别（字符串比较；值与核心库事件字典一致）──
bool Event::IsClick()          const { return GetName() == PropertyNames::kEventClick; }
bool Event::IsValueChanged()   const { return GetName() == PropertyNames::kEventValueChanged; }
float Event::GetValueChanged() const { return m_raw->data.floatVal; }
double Event::GetValueChangedDouble() const { return m_raw->data.doubleVal; }

bool Event::IsTextChanged()    const { return GetName() == PropertyNames::kEventTextChanged; }
std::string Event::GetTextChanged() const { return m_raw->data.strVal ? m_raw->data.strVal : ""; }

bool Event::IsSelectionChanged() const { return GetName() == PropertyNames::kEventSelectionChanged; }
int Event::GetSelectedIndex() const { return m_raw->data.selection.idx; }
std::string Event::GetSelectedValue() const { return m_raw->data.selection.val ? m_raw->data.selection.val : ""; }

bool Event::IsCheckChanged() const { return GetName() == PropertyNames::kEventCheckChanged; }
int Event::GetCheckState() const { return m_raw->data.intVal; }

// ColorPicker 经 fireCCallback(kEventColorChanged, Color) 触发，Binding 可直接取色
bool Event::IsColorChanged() const { return GetName() == PropertyNames::kEventColorChanged; }
UIColor Event::GetChangedColor() const {
    UIColor c{m_raw->data.color.r, m_raw->data.color.g, m_raw->data.color.b, m_raw->data.color.a};
    return c;
}

bool Event::IsPositionChanged() const { return GetName() == PropertyNames::kEventPositionChanged; }
float Event::GetPositionChanged() const { return m_raw->data.floatVal; }

bool Event::IsMoved() const { return GetName() == PropertyNames::kEventMoved; }
float Event::GetMovedPosition() const { return m_raw->data.floatVal; }

bool Event::IsConfirm() const { return GetName() == PropertyNames::kEventConfirm; }
bool Event::IsCancel()  const { return GetName() == PropertyNames::kEventCancel; }

bool Event::IsClose() const { return GetName() == PropertyNames::kEventClose; }
int Event::GetCloseResult() const { return m_raw->data.intVal; }

bool Event::IsEnter() const { return GetName() == PropertyNames::kEventEnter; }
bool Event::IsSelect()   const { return GetName() == PropertyNames::kEventSelect; }
bool Event::IsExpand()   const { return GetName() == PropertyNames::kEventExpand; }
bool Event::IsCollapse() const { return GetName() == PropertyNames::kEventCollapse; }
std::string Event::GetNodeId() const { return m_raw->data.treeNode.id ? m_raw->data.treeNode.id : ""; }
void* Event::GetNodeUserData() const { return m_raw->data.treeNode.userData; }

bool Event::IsListSelectionChanged() const { return GetName() == PropertyNames::kEventListSelectionChanged; }
bool Event::IsItemClick() const { return GetName() == PropertyNames::kEventItemClick; }
bool Event::IsColumnSort() const { return GetName() == PropertyNames::kEventColumnSort; }
bool Event::IsAnimationEnded() const { return GetName() == PropertyNames::kEventAnimationEnded; }
int Event::GetAnimationEndedId() const { return m_raw->data.intVal; }
bool Event::IsStatusItemClick() const { return GetName() == PropertyNames::kEventStatusItemClick; }
int Event::GetStatusItemIndex() const { return m_raw->data.intVal; }
bool Event::IsTabChanged() const { return GetName() == PropertyNames::kEventTabChanged; }
int Event::GetTabChangedIndex() const { return m_raw->data.intVal; }
int Event::GetGridRow() const { return m_raw->data.grid.row; }
int Event::GetGridCol() const { return m_raw->data.grid.col; }
int Event::GetGridAsc() const { return m_raw->data.grid.asc; }

// ── 通用原始访问 ──
int Event::GetIntVal() const { return m_raw->data.intVal; }
float Event::GetFloatVal() const { return m_raw->data.floatVal; }
double Event::GetDoubleVal() const { return m_raw->data.doubleVal; }
const char* Event::GetStrVal() const { return m_raw->data.strVal; }
void* Event::GetPtrVal() const { return m_raw->data.ptrVal; }