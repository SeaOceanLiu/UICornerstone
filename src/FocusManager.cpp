#include "FocusManager.h"
#include "ControlBase.h"
#include <algorithm>

FocusManager::FocusManager()
{
}

FocusManager::~FocusManager()
{
}

void FocusManager::registerControl(Control* ctl)
{
    if (!ctl) return;
    if (std::find(m_controls.begin(), m_controls.end(), ctl) != m_controls.end())
        return;
    m_controls.push_back(ctl);
}

void FocusManager::unregisterControl(Control* ctl)
{
    if (!ctl) return;
    auto it = std::remove(m_controls.begin(), m_controls.end(), ctl);
    if (it != m_controls.end())
        m_controls.erase(it, m_controls.end());

    if (m_currentFocused == ctl)
        m_currentFocused = nullptr;

    // Also remove from boundaries if present
    auto bit = std::remove(m_boundaries.begin(), m_boundaries.end(), ctl);
    if (bit != m_boundaries.end())
        m_boundaries.erase(bit, m_boundaries.end());
}

void FocusManager::registerBoundary(Control* boundary)
{
    if (!boundary) return;
    if (std::find(m_boundaries.begin(), m_boundaries.end(), boundary) != m_boundaries.end())
        return;
    m_boundaries.push_back(boundary);
}

void FocusManager::unregisterBoundary(Control* boundary)
{
    if (!boundary) return;
    auto it = std::remove(m_boundaries.begin(), m_boundaries.end(), boundary);
    if (it != m_boundaries.end())
        m_boundaries.erase(it, m_boundaries.end());
}

void FocusManager::notifyControlFocused(Control* ctl, bool byKeyboard) {
    if (!ctl) return;
    if (ctl->getFocused()) {
        if (m_currentFocused == ctl) return;
        if (m_currentFocused)
            m_currentFocused->setFocused(false, false);
        m_currentFocused = ctl;
    } else {
        if (m_currentFocused == ctl)
            m_currentFocused = nullptr;
    }
}

void FocusManager::clearFocus() {
    if (m_currentFocused) {
        m_currentFocused->setFocused(false, false);
        m_currentFocused = nullptr;
    }
}

bool FocusManager::focusControl(Control* ctl) {
    if (!ctl || !ctl->isFocusable() || !ctl->getVisible() || !ctl->getEnable())
        return false;
    if (m_currentFocused && m_currentFocused != ctl)
        m_currentFocused->setFocused(false, false);
    ctl->setFocused(true, true);
    m_currentFocused = ctl;
    return true;
}

Control* FocusManager::findFocusScope(Control* ctl)
{
    while (ctl) {
        if (ctl->isFocusBoundary()) return ctl;
        ctl = ctl->getParent();
    }
    return nullptr;
}

bool FocusManager::isInScope(Control* candidate, Control* scope)
{
    if (!scope) return true;
    Control* cs = findFocusScope(candidate);
    return cs == scope;
}

bool FocusManager::isDescendantOf(Control* ancestor, Control* candidate)
{
    if (!ancestor || !candidate) return false;
    Control* p = candidate->getParent();
    while (p) {
        if (p == ancestor) return true;
        p = p->getParent();
    }
    return false;
}

bool FocusManager::focusNext(Control* current)
{
    Control* scope = current ? findFocusScope(current) : nullptr;

    auto it = current ? std::find(m_controls.begin(), m_controls.end(), current)
                      : m_controls.end();
    if (it != m_controls.end()) ++it;

    for (size_t i = 0; i < m_controls.size(); i++) {
        if (it == m_controls.end()) it = m_controls.begin();

        Control* c = *it;
        if (isInScope(c, scope) && c->getVisible() && c->getEnable()) {
            if (c != current) {
                if (current) current->setFocused(false, false);
                c->setFocused(true, true);
                m_currentFocused = c;
                return true;
            }
            break;
        }
        ++it;
    }
    return false;
}

bool FocusManager::focusPrev(Control* current)
{
    Control* scope = current ? findFocusScope(current) : nullptr;

    auto it = current ? std::find(m_controls.begin(), m_controls.end(), current)
                      : m_controls.begin();
    if (it == m_controls.begin()) it = m_controls.end();
    if (it != m_controls.begin()) --it;

    for (size_t i = 0; i < m_controls.size(); i++) {
        Control* c = *it;
        if (isInScope(c, scope) && c->getVisible() && c->getEnable()) {
            if (c != current) {
                if (current) current->setFocused(false, false);
                c->setFocused(true, true);
                m_currentFocused = c;
                return true;
            }
            break;
        }
        if (it == m_controls.begin()) it = m_controls.end();
        if (it != m_controls.begin()) --it;
    }
    return false;
}

bool FocusManager::focusNextScope()
{
    Control* current = m_currentFocused;
    Control* currentScope = current ? findFocusScope(current) : nullptr;

    // Find current scope's index in boundaries
    int startIdx = -1;
    for (size_t i = 0; i < m_boundaries.size(); i++) {
        if (m_boundaries[i] == currentScope) {
            startIdx = (int)i;
            break;
        }
    }

    // Try each boundary from next to end, then wrap around
    for (size_t offset = 1; offset <= m_boundaries.size(); offset++) {
        int idx = (startIdx + (int)offset) % (int)m_boundaries.size();
        Control* boundary = m_boundaries[idx];
        if (!boundary->getVisible()) continue;

        if (focusFirstInScope(boundary))
            return true;
    }

    // Fallback: find first visible enabled control in the flat list
    for (Control* c : m_controls) {
        if (c->getVisible() && c->getEnable()) {
            if (current && current != c) current->setFocused(false, false);
            c->setFocused(true, true);
            m_currentFocused = c;
            return true;
        }
    }
    return false;
}

bool FocusManager::focusPrevScope()
{
    Control* current = m_currentFocused;
    Control* currentScope = current ? findFocusScope(current) : nullptr;

    int startIdx = -1;
    for (size_t i = 0; i < m_boundaries.size(); i++) {
        if (m_boundaries[i] == currentScope) {
            startIdx = (int)i;
            break;
        }
    }

    for (size_t offset = 1; offset <= m_boundaries.size(); offset++) {
        int idx = (startIdx - (int)offset);
        while (idx < 0) idx += (int)m_boundaries.size();
        idx = idx % (int)m_boundaries.size();
        Control* boundary = m_boundaries[idx];
        if (!boundary->getVisible()) continue;

        if (focusFirstInScope(boundary))
            return true;
    }

    for (Control* c : m_controls) {
        if (c->getVisible() && c->getEnable()) {
            if (current && current != c) current->setFocused(false, false);
            c->setFocused(true, true);
            m_currentFocused = c;
            return true;
        }
    }
    return false;
}

bool FocusManager::focusFirstInScope(Control* scope)
{
    if (!scope) return false;
    for (Control* c : m_controls) {
        if (isDescendantOf(scope, c) && c->getVisible() && c->getEnable()) {
            if (m_currentFocused && m_currentFocused != c)
                m_currentFocused->setFocused(false, false);
            c->setFocused(true, true);
            m_currentFocused = c;
            return true;
        }
    }
    // Fallback: focus the scope itself if it's focusable and visible
    if (scope->isFocusable() && scope->getVisible()) {
        if (m_currentFocused && m_currentFocused != scope)
            m_currentFocused->setFocused(false, false);
        scope->setFocused(true, true);
        m_currentFocused = scope;
        return true;
    }
    return false;
}
