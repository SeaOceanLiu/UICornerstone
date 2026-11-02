#include "InlineEditBox.h"

InlineEditBox::InlineEditBox(Control* parent, SRect rect, float xScale, float yScale)
    : EditBox(parent, rect, xScale, yScale) {
}

void InlineEditBox::finish(bool commit) {
    if (!m_editActive || m_finishing) return;
    m_finishing = true;
    if (commit) {
        std::string text = getText();
        if (m_commit) m_commit(text);
    } else {
        if (m_cancel) m_cancel();
    }
    m_editActive = false;
}

bool InlineEditBox::handleEvent(std::shared_ptr<Event> event) {
    if (m_editActive && event->m_type == EventType::KeyDown &&
        event->keyEvent.keycode == KeyCode::Escape) {
        finish(false);
        return true;
    }
    return EditBox::handleEvent(event);
}

void InlineEditBox::onFocusLost() {
    EditBox::onFocusLost();
    if (m_editActive) finish(true);
}
