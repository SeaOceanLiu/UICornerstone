#define NOMINMAX
#include "Dialog.h"
#include "Bench.h"
#include "FocusManager.h"
#include <algorithm>

// ==================== Popup ====================

Popup::Popup(Control* parent, SRect rect,
             float xScale, float yScale)
    : Panel(parent, rect, xScale, yScale)
{
    setFocusable(false);
    setTransparent(false);
    setBorderVisible(true);
    setVisible(false);
    setFocusBoundary(true);
    setShowFocusRing(false);
}

Popup::~Popup() {
}

void Popup::create() {
    Panel::create();
    setTransparent(false);
    setBorderVisible(true);
    setVisible(false);
    setFocusBoundary(true);
    if (m_content) {
        m_content->setRenderDevice(getRenderDevice());
        m_content->setTextRenderer(getTextRenderer());
        m_content->setResourceProvider(getResourceProvider());
        m_content->setInputBackend(getInputBackend());
    }
    layoutContent();
}

void Popup::layoutContent() {
    if (!m_content) return;
    m_content->setRect(SRect(m_padding, m_padding,
                             m_rect.width - m_padding * 2,
                             m_rect.height - m_padding * 2));
}

SRect Popup::computeTargetRect() {
    float sx = getScaleXX();
    float sy = getScaleYY();
    SSize ws = MAINWIN->getWindowSize();
    float screenW = (float)ws.width;
    float screenH = (float)ws.height;

    switch (m_anchorMode) {
    case AnchorMode::Centered: {
        float cx = (screenW - m_rect.width * sx) / 2.0f;
        float cy = (screenH - m_rect.height * sy) / 2.0f;
        return SRect(cx, cy, m_rect.width, m_rect.height);
    }
    case AnchorMode::Anchored: {
        if (!m_anchorControl)
            return SRect(0, 0, m_rect.width, m_rect.height);
        SRect adr = m_anchorControl->getDrawRect();
        float x = adr.left + m_anchorOffset.left * sx;
        float y = adr.bottom() + m_anchorOffset.top * sy;
        // Clamp to screen
        if (x + m_rect.width * sx > screenW) x = screenW - m_rect.width * sx;
        if (x < 0) x = 0;
        if (y + m_rect.height * sy > screenH) y = screenH - m_rect.height * sy;
        if (y < 0) y = 0;
        return SRect(x, y, m_rect.width, m_rect.height);
    }
    case AnchorMode::Absolute:
    default:
        return m_rect;
    }
}

void Popup::registerWatcher() {
    if (m_watcherRegistered) return;
    EventQueue::getInstance()->addBeforeEventHandlingWatcher(
        EventType::KeyDown, getThis());
    EventQueue::getInstance()->addBeforeEventHandlingWatcher(
        EventType::MouseDown, getThis());
    m_watcherRegistered = true;
}

void Popup::focusFirstContent() {
    if (m_content && m_content->isFocusable()) {
        GET_FOCUSMANAGER->focusControl(m_content.get());
        return;
    }
    // Focus first focusable child of content
    if (m_content) {
        GET_FOCUSMANAGER->focusFirstInScope(this);
    }
}

void Popup::open() {
    if (getVisible()) return;
    SRect target = computeTargetRect();
    setRect(target);
    layoutContent();
    setVisible(true);
    BENCH->addControl(static_pointer_cast<Control>(getThis()));
    registerWatcher();
    GET_FOCUSMANAGER->registerBoundary(this);
    m_result = DialogResult::None;
    focusFirstContent();
}

void Popup::close(DialogResult result) {
    if (!getVisible()) return;
    m_result = result;
    setVisible(false);
    BENCH->removeControl(static_pointer_cast<Control>(getThis()));
    GET_FOCUSMANAGER->unregisterBoundary(this);
    // 不在此处 removeBeforeEventHandlingWatcher：
    // close() 可能从 beforeEventHandlingWatcher 内部调用（ESC/outside-click），
    // 此时 EventQueue 已持有 m_mtxForBeforeEventHandlingWatcher，递归 lock → UB。
    // 不可见时 watcher 是安全的（检查 getVisible() 直接返回 false）。
    // EventQueue 静态析构时会自动清理。
    if (m_onClose)
        m_onClose(std::dynamic_pointer_cast<Popup>(getThis()), result);
}

void Popup::setContent(shared_ptr<ControlImpl> content) {
    if (m_content) {
        removeControl(m_content);
        m_content.reset();
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        m_content->setRenderDevice(getRenderDevice());
        m_content->setTextRenderer(getTextRenderer());
        m_content->setResourceProvider(getResourceProvider());
        m_content->setInputBackend(getInputBackend());
        addControl(m_content);
    }
}

void Popup::setCentered() {
    m_anchorMode = AnchorMode::Centered;
}

void Popup::setAnchored(Control* anchor, const SRect& offset) {
    m_anchorMode = AnchorMode::Anchored;
    m_anchorControl = anchor;
    m_anchorOffset = offset;
}

void Popup::setAbsolute(const SRect& rect) {
    m_anchorMode = AnchorMode::Absolute;
    setRect(rect);
}

// ==================== Popup Event Handlers ====================

bool Popup::handleEvent(shared_ptr<Event> event) {
    if (event->m_type == EventType::MouseWheel) {
        if (isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if ((*it)->getVisible() && (*it)->getEnable() && (*it)->handleEvent(event))
                    return true;
            }
        }
        // Don't fall through to Panel::handleEvent — children already tried above,
        // and without area check they'd incorrectly scroll from outside the popup.
        return false;
    }
    return Panel::handleEvent(event);
}

bool Popup::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    if (!getVisible()) return false;

    if (event->m_type == EventType::KeyDown &&
        event->keyEvent.keycode == KeyCode::Escape && m_closeOnEsc) {
        onEscPressed();
        return true;
    }

    if (event->m_type == EventType::MouseDown && m_closeOnClickOutside) {
        SPoint mp(event->mouseButton.x, event->mouseButton.y);
        // Click inside popup — let it handle normally
        if (isContainsPoint(mp.x, mp.y))
            return false;
        // If anchored, clicks on anchor control also don't close
        if (m_anchorMode == AnchorMode::Anchored && m_anchorControl &&
            m_anchorControl->isContainsPoint(mp.x, mp.y))
            return false;
        onOutsideClicked();
        // Return false so the same click propagates to other controls
        // (e.g., clicking on another ColorPicker should open its popup)
    }
    return false;
}

void Popup::onEscPressed() {
    close(DialogResult::Cancelled);
}

void Popup::onOutsideClicked() {
    close(DialogResult::Cancelled);
}

// ==================== ConfirmPopup ====================

ConfirmPopup::ConfirmPopup(Control* parent, SRect rect,
                           float xScale, float yScale)
    : Popup(parent, rect, xScale, yScale)
{
}

void ConfirmPopup::create() {
    Popup::create();
    if (m_showConfirmButton)
        createConfirmButton();
    layoutContent();
}

void ConfirmPopup::open() {
    layoutContent();
    Popup::open();
}

void ConfirmPopup::createConfirmButton() {
    if (!m_showConfirmButton) return;
    m_btnConfirm = make_shared<Button>(this, SRect(0, 0, 80, m_buttonHeight),
                                       1.0f, 1.0f);
    m_btnConfirm->setCaption(m_btnConfirmText);
    m_btnConfirm->setOnClick([this](shared_ptr<Button>) {
        onConfirmAction();
    });
    m_btnConfirm->create();
    addControl(m_btnConfirm);
}

void ConfirmPopup::layoutContent() {
    float w = m_rect.width;
    float h = m_rect.height;
    float btnH = m_buttonHeight;
    float btnW = 80.0f;

    // Position content to fill most of the dialog, leaving room for buttons
    if (m_content && m_showConfirmButton) {
        SRect contentRect(m_padding, m_padding,
                          w - m_padding * 2,
                          h - m_padding * 2 - btnH - m_padding);
        m_content->setRect(contentRect);
    } else if (m_content) {
        SRect contentRect(m_padding, m_padding,
                          w - m_padding * 2,
                          h - m_padding * 2);
        m_content->setRect(contentRect);
    }

    // Position confirm button at bottom-right (left side of button pair)
    if (m_btnConfirm) {
        if (m_btnConfirmRect.width > 0 && m_btnConfirmRect.height > 0) {
            m_btnConfirm->setRect(m_btnConfirmRect);
        } else {
            float bx = w - m_padding - btnW;
            float by = h - m_padding - btnH;
            m_btnConfirm->setRect(SRect(bx, by, btnW, btnH));
        }
    }
}

void ConfirmPopup::onEscPressed() {
    close(DialogResult::Cancelled);
}

void ConfirmPopup::onConfirmAction() {
    if (m_onConfirm)
        m_onConfirm(std::dynamic_pointer_cast<ConfirmPopup>(getThis()));
    close(DialogResult::Confirmed);
}

bool ConfirmPopup::handleEvent(shared_ptr<Event> event) {
    if (m_ignoreKeyEvent) {
        m_ignoreKeyEvent = false;
        return true;
    }
    if (event->m_type == EventType::KeyDown &&
        event->keyEvent.keycode == KeyCode::Return &&
        m_showConfirmButton) {
        m_ignoreKeyEvent = true;
        onConfirmAction();
        return true;
    }
    return Popup::handleEvent(event);
}

// ==================== Dialog ====================

Dialog::Dialog(Control* parent, SRect rect,
               float xScale, float yScale)
    : ConfirmPopup(parent, rect, xScale, yScale)
{
}

void Dialog::create() {
    ConfirmPopup::create();
    createCancelButton();
    layoutContent();
}

void Dialog::open() {
    layoutContent();
    Popup::open();  // skip ConfirmPopup::open() to avoid double layoutContent
}

void Dialog::createCancelButton() {
    m_btnCancel = make_shared<Button>(this, SRect(0, 0, 80, m_buttonHeight),
                                      1.0f, 1.0f);
    m_btnCancel->setCaption(m_btnCancelText);
    m_btnCancel->setOnClick([this](shared_ptr<Button>) {
        onCancelAction();
    });
    m_btnCancel->create();
    addControl(m_btnCancel);
}

void Dialog::layoutContent() {
    ConfirmPopup::layoutContent();
    float w = m_rect.width;
    float h = m_rect.height;
    float btnH = m_buttonHeight;
    float btnW = 80.0f;

    // Position two buttons side by side at bottom-right
    // Confirm on right, Cancel on left
    if (m_btnCancel) {
        if (m_btnCancelRect.width > 0 && m_btnCancelRect.height > 0) {
            m_btnCancel->setRect(m_btnCancelRect);
            if (m_btnConfirm)
                m_btnConfirm->setRect(m_btnConfirmRect);
        } else {
            float cx = w - m_padding - btnW;
            float cy = h - m_padding - btnH;
            m_btnConfirm->setRect(SRect(cx, cy, btnW, btnH));
            float cax = cx - m_buttonGap - btnW;
            m_btnCancel->setRect(SRect(cax, cy, btnW, btnH));
        }
    }
}

void Dialog::onEscPressed() {
    onCancelAction();
}

void Dialog::onCancelAction() {
    if (m_onCancel)
        m_onCancel(std::dynamic_pointer_cast<Dialog>(getThis()));
    close(DialogResult::Cancelled);
}

bool Dialog::handleEvent(shared_ptr<Event> event) {
    return ConfirmPopup::handleEvent(event);
}

void Dialog::recreateButtons() {
    createConfirmButton();
    createCancelButton();
    layoutContent();
}

// ==================== PopupBuilder ====================

PopupBuilder::PopupBuilder(Control* parent, SRect rect,
                           float xScale, float yScale)
{
    m_popup = make_shared<Popup>(parent, rect, xScale, yScale);
}

PopupBuilder& PopupBuilder::setContent(shared_ptr<ControlImpl> content)
{ m_popup->setContent(content); return *this; }

PopupBuilder& PopupBuilder::setCentered()
{ m_popup->setCentered(); return *this; }

PopupBuilder& PopupBuilder::setOnClose(Popup::OnCloseHandler handler)
{ m_popup->setOnClose(handler); return *this; }

PopupBuilder& PopupBuilder::setCloseOnClickOutside(bool v)
{ m_popup->setCloseOnClickOutside(v); return *this; }

PopupBuilder& PopupBuilder::setCloseOnEsc(bool v)
{ m_popup->setCloseOnEsc(v); return *this; }

PopupBuilder& PopupBuilder::setBackgroundStateColor(StateColor stateColor)
{ m_popup->setBackgroundStateColor(stateColor); return *this; }

PopupBuilder& PopupBuilder::setBorderStateColor(StateColor stateColor)
{ m_popup->setBorderStateColor(stateColor); return *this; }

shared_ptr<Popup> PopupBuilder::build() {
    m_popup->create();
    if (!m_popup->getVisible())
        m_popup->setVisible(false);
    return m_popup;
}

// ==================== ConfirmPopupBuilder ====================

ConfirmPopupBuilder::ConfirmPopupBuilder(Control* parent, SRect rect,
    float xScale, float yScale)
    : PopupBuilder(parent, rect, xScale, yScale)
{
    m_popup = make_shared<ConfirmPopup>(parent, rect, xScale, yScale);
}

ConfirmPopupBuilder& ConfirmPopupBuilder::setConfirmButtonText(const string& text)
{ static_pointer_cast<ConfirmPopup>(m_popup)->setConfirmButtonText(text); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setConfirmButtonRect(SRect rect)
{ static_pointer_cast<ConfirmPopup>(m_popup)->setConfirmButtonRect(rect); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setButtonHeight(float h)
{ static_pointer_cast<ConfirmPopup>(m_popup)->setButtonHeight(h); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setButtonGap(float gap)
{ static_pointer_cast<ConfirmPopup>(m_popup)->setButtonGap(gap); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setPadding(float pad)
{ static_pointer_cast<ConfirmPopup>(m_popup)->setPadding(pad); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setOnConfirm(ConfirmPopup::OnConfirmHandler handler)
{ static_pointer_cast<ConfirmPopup>(m_popup)->setOnConfirm(handler); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setBackgroundStateColor(StateColor stateColor)
{ m_popup->setBackgroundStateColor(stateColor); return *this; }

ConfirmPopupBuilder& ConfirmPopupBuilder::setBorderStateColor(StateColor stateColor)
{ m_popup->setBorderStateColor(stateColor); return *this; }

shared_ptr<ConfirmPopup> ConfirmPopupBuilder::build() {
    m_popup->create();
    if (!m_popup->getVisible())
        m_popup->setVisible(false);
    return static_pointer_cast<ConfirmPopup>(m_popup);
}

// ==================== DialogBuilder ====================

DialogBuilder::DialogBuilder(Control* parent, SRect rect,
    float xScale, float yScale)
    : ConfirmPopupBuilder(parent, rect, xScale, yScale)
{
    m_popup = make_shared<Dialog>(parent, rect, xScale, yScale);
}

DialogBuilder& DialogBuilder::setCancelButtonText(const string& text)
{ static_pointer_cast<Dialog>(m_popup)->setCancelButtonText(text); return *this; }

DialogBuilder& DialogBuilder::setCancelButtonRect(SRect rect)
{ static_pointer_cast<Dialog>(m_popup)->setCancelButtonRect(rect); return *this; }

DialogBuilder& DialogBuilder::setOnCancel(Dialog::OnCancelHandler handler)
{ static_pointer_cast<Dialog>(m_popup)->setOnCancel(handler); return *this; }

DialogBuilder& DialogBuilder::setOnConfirm(ConfirmPopup::OnConfirmHandler handler)
{ static_pointer_cast<Dialog>(m_popup)->setOnConfirm(handler); return *this; }

DialogBuilder& DialogBuilder::setBackgroundStateColor(StateColor stateColor)
{ m_popup->setBackgroundStateColor(stateColor); return *this; }

DialogBuilder& DialogBuilder::setBorderStateColor(StateColor stateColor)
{ m_popup->setBorderStateColor(stateColor); return *this; }

shared_ptr<Dialog> DialogBuilder::build() {
    m_popup->create();
    if (!m_popup->getVisible())
        m_popup->setVisible(false);
    return static_pointer_cast<Dialog>(m_popup);
}
