// InlineEditBox — 列表/树就地编辑覆盖层（P0-111/P0-112）
// 仅引擎内部使用：Enter/失焦提交；Esc 取消；生命周期由宿主控件管理。
#ifndef UICORNERSTONE_INLINE_EDIT_BOX_H
#define UICORNERSTONE_INLINE_EDIT_BOX_H

#include "EditBox.h"
#include <functional>

class InlineEditBox : public EditBox {
public:
    using CommitHandler = std::function<void(const std::string& text)>;
    using CancelHandler = std::function<void()>;

    InlineEditBox(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    void setCommitHandler(CommitHandler h) { m_commit = std::move(h); }
    void setCancelHandler(CancelHandler h) { m_cancel = std::move(h); }
    void setEditActive(bool active) { m_editActive = active; }

    bool handleEvent(std::shared_ptr<Event> event) override;
    void onFocusLost() override;
    bool commitEditOnTab() override { if (!m_editActive) return false; finish(true); return true; }

private:
    void finish(bool commit);

    CommitHandler m_commit;
    CancelHandler m_cancel;
    bool m_editActive = false;
    bool m_finishing = false;
};

#endif
