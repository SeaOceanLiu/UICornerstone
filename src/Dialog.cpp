#define NOMINMAX
#include "Dialog.h"
#include "Bench.h"
#include "FocusManager.h"
#include "PropertyNames.h"
#include <algorithm>

// ==================== Popup ====================

Popup::Popup(Control* parent, SRect rect,
             float xScale, float yScale)
    : Panel(parent, rect, xScale, yScale)
{
    m_ctlType = ControlType::Popup;
    setFocusable(false);
    setTransparent(false);
    setBorderVisible(true);
    setVisible(false);
    setFocusBoundary(true);
    setShowFocusRing(false);
}

Popup::~Popup() {
}

void Popup::setParent(Control* parent) {
    ControlImpl::setParent(parent);
    // 弹层作为画布内容随根变换缩放：复合 = 自身布局缩放 × 父（bench）复合。
    // setParent 仅更新自身快照，挂树后须向子树传播（create() 已就绪的内部
    // 控件如确认按钮/内容面板的复合此时才随根变换生效）。
    for (auto& child : m_children) {
        child->refreshScaleWith(m_xxScale, m_yyScale);
    }
}

void Popup::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建（open() 挂树后重建）

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
    // 父相对坐标语义：m_rect 为相对父（bench）的本地坐标（与普通控件一致，
    // getDrawRect 由通用实现叠加父偏移）。视口/bench 为绝对区域，居中与
    // 锚定计算在本函数内换算为本地坐标。
    // 弹层随根变换缩放：复合 = 自身布局缩放 × 父复合，屏幕宽度 = m_rect.width*sx，
    // 屏幕位置 = m_rect.left*父复合 + 父 DR.left —— 公式同步适配（除以根复合反查）
    SRect vp = GET_CONTEXT ? GET_CONTEXT->viewport : SRect(0, 0, 1024, 768);
    float localW = vp.width;
    float localH = vp.height;
    SRect benchRect = BENCH ? BENCH->getDrawRect() : SRect(0, 0, 0, 0);
    float bx = benchRect.left;
    float by = benchRect.top;

    switch (m_anchorMode) {
    case AnchorMode::Centered: {
        // 弹层随画布缩放（sx = 复合）：居中基准 = 视口中心，反查（- 偏移后 ÷ 父复合）
        // 到父（bench）本地坐标（getDrawRect 位置乘父复合，见 ControlBase.cpp 非根分支）
        float bsx = BENCH ? BENCH->getScaleXX() : 1.0f;
        float bsy = BENCH ? BENCH->getScaleYY() : 1.0f;
        float cx = (vp.left + (localW - m_rect.width * sx) / 2.0f - bx) / bsx;
        float cy = (vp.top + (localH - m_rect.height * sy) / 2.0f - by) / bsy;
        return SRect(cx, cy, m_rect.width, m_rect.height);
    }
    case AnchorMode::Anchored: {
        if (!m_anchorControl)
            return SRect(0, 0, m_rect.width, m_rect.height);
        SRect adr = m_anchorControl->getDrawRect();
        float bsx = BENCH ? BENCH->getScaleXX() : 1.0f;
        float bsy = BENCH ? BENCH->getScaleYY() : 1.0f;
        float x = (adr.left - bx + m_anchorOffset.left * sx) / bsx;
        float y = (adr.bottom() - by + m_anchorOffset.top * sy) / bsy;
        // Clamp to viewport（换算到弹层本地：屏幕 [vp.left, vp.left+W] 反查，
        // 弹层屏幕宽度 = m_rect.width*sx，减项需除以父复合还原到本地单位）
        float loX = (vp.left - bx) / bsx;
        float hiX = (vp.left + localW - bx) / bsx - m_rect.width * sx / bsx;
        float loY = (vp.top - by) / bsy;
        float hiY = (vp.top + localH - by) / bsy - m_rect.height * sy / bsy;
        if (x > hiX) x = hiX;
        if (x < loX) x = loX;
        if (y > hiY) y = hiY;
        if (y < loY) y = loY;
        return SRect(x, y, m_rect.width, m_rect.height);
    }
    case AnchorMode::Absolute:
    default:
        return m_rect;
    }
}

void Popup::registerWatcher() {
    if (m_watcherRegistered) return;
    m_context->eventQueue->addBeforeEventHandlingWatcher(
        EventType::KeyDown, getThis());
    m_context->eventQueue->addBeforeEventHandlingWatcher(
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
    // 无 parent 归属的浮层（测试直接 make_shared<Popup>(nullptr,...)）：
    // 挂树前先归属最近实例，保证 BENCH/GET_CONTEXT/事件队列有效
    if (GET_CONTEXT == nullptr) {
        UIContext* ctx = UIContext::getLastInstance();
        if (ctx) setContext(ctx);
    }
    auto self = static_pointer_cast<Control>(getThis());
    // 先挂树再算位置：弹层作为画布内容，复合 = 布局缩放 × 根变换，须在
    // setParent 继承复合后 computeTargetRect 才能得到正确的复合与反查基准
    BENCH->addControl(self);
    SRect target = computeTargetRect();
    setRect(target);
    layoutContent();
    setVisible(true);
    registerWatcher();
    GET_FOCUSMANAGER->registerBoundary(this);
    m_result = DialogResult::None;
    focusFirstContent();
}

void Popup::close(DialogResult result) {
    if (!getVisible()) return;
    m_result = result;
    setVisible(false);
    // 防御性保活：浮层可能以 bench 为唯一持有者（C ABI 裸句柄场景），摘树后须存活至
    // close 尾部（m_onClose/回调）结束，避免 use-after-free
    auto selfKeepAlive = static_pointer_cast<Control>(getThis());
    BENCH->removeControl(selfKeepAlive);
    GET_FOCUSMANAGER->unregisterBoundary(this);
    // 不在此处 removeBeforeEventHandlingWatcher：
    // close() 可能从 beforeEventHandlingWatcher 内部调用（ESC/outside-click），
    // 此时 EventQueue 已持有 m_mtxForBeforeEventHandlingWatcher，递归 lock → UB。
    // 不可见时 watcher 是安全的（检查 getVisible() 直接返回 false）。
    // EventQueue 静态析构时会自动清理。
    if (m_onClose)
        m_onClose(std::dynamic_pointer_cast<Popup>(getThis()), result);
    int r = static_cast<int>(result);
    fireCCallback(PropertyNames::kEventClose, CCallbackData::Int, &r);
}

void Popup::setContent(shared_ptr<ControlImpl> content) {
    if (m_content) {
        removeControl(m_content);
        m_content.reset();
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        // 两阶段：context 未就绪（未挂树）时不传播 renderDevice 等（避免空值缓存
        // 与错误日志），挂树后由 Popup::create / inheritRenderer 补齐
        if (GET_CONTEXT) {
            m_content->setRenderDevice(getRenderDevice());
            m_content->setTextRenderer(getTextRenderer());
            m_content->setResourceProvider(getResourceProvider());
            m_content->setInputBackend(getInputBackend());
        }
        addControl(m_content);
    }
}

void Popup::setRect(SRect rect) {
    m_anchorMode = AnchorMode::Absolute;
    Panel::setRect(rect);
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
    if (!m_enable || !m_visible) return false;   // enable/visible 守卫（隐藏弹窗不响应）
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
    m_ctlType = ControlType::ConfirmPopup;
}

void ConfirmPopup::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建

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
    fireCCallback(PropertyNames::kEventConfirm, CCallbackData::None, nullptr);
    close(DialogResult::Confirmed);
}

bool ConfirmPopup::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;   // enable/visible 守卫
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
    m_ctlType = ControlType::Dialog;
}

void Dialog::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建

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
    fireCCallback(PropertyNames::kEventCancel, CCallbackData::None, nullptr);
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

// ── Popup property system ──
int Popup::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kCloseOnClickOutside) == 0) { setCloseOnClickOutside(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kCloseOnEsc) == 0) { setCloseOnEsc(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kVisible) == 0) {
        if (value) open(); else close();
        return 1;
    }
    return Panel::setBoolProperty(prop, value);
}
int Popup::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kCloseOnClickOutside) == 0) { out = m_closeOnClickOutside ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kCloseOnEsc) == 0) { out = m_closeOnEsc ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kPopupVisible) == 0) { out = isPopupVisible() ? 1 : 0; return 1; }
    return Panel::getBoolProperty(prop, out);
}
int Popup::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kResult) == 0) {
        if (value >= 0 && value <= 2) {  // DialogResult: None=0 Confirmed=1 Cancelled=2
            m_result = static_cast<DialogResult>(value);
            return 1;
        }
        return 0;
    }
    return Panel::setIntProperty(prop, value);
}
int Popup::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kResult) == 0) { out = static_cast<int>(m_result); return 1; }
    return Panel::getIntProperty(prop, out);
}
int Popup::setPtrProperty(const char* prop, void* value) {
    if (strcmp(prop, PropertyNames::kContent) == 0) {
        auto* impl = dynamic_cast<ControlImpl*>(static_cast<Control*>(value));
        if (impl) setContent(impl->shared_from_this());
        return 1;
    }
    return Panel::setPtrProperty(prop, value);
}
int Popup::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kCenteredMode) == 0) {
        if (strcmp(value, PropertyNames::kCentered) == 0) { setCentered(); return 1; }
        return 0;
    }
    return Panel::setEnumProperty(prop, value);
}
int Popup::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventClose) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return Panel::setCallbackProperty(event, cb, userData);
}

// ── Dialog property system ──
int Dialog::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kButtonHeight) == 0) { setButtonHeight(value); return 1; }
    if (strcmp(prop, PropertyNames::kButtonGap) == 0)    { setButtonGap(value);    return 1; }
    if (strcmp(prop, PropertyNames::kPadding) == 0)      { setPadding(value);      return 1; }
    return Popup::setFloatProperty(prop, value);
}
int Dialog::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kButtonHeight) == 0) { out = m_buttonHeight; return 1; }
    if (strcmp(prop, PropertyNames::kButtonGap) == 0)    { out = m_buttonGap;    return 1; }
    if (strcmp(prop, PropertyNames::kPadding) == 0)      { out = m_padding;      return 1; }
    return Popup::getFloatProperty(prop, out);
}
int Dialog::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kConfirmVisible) == 0) { setConfirmButtonVisible(value != 0); return 1; }
    return Popup::setBoolProperty(prop, value);
}
int Dialog::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kConfirmText) == 0) { setConfirmButtonText(value); return 1; }
    if (strcmp(prop, PropertyNames::kCancelText) == 0)  { setCancelButtonText(value);  return 1; }
    return Popup::setStringProperty(prop, value);
}
int Dialog::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kConfirmVisible) == 0) { out = m_showConfirmButton ? 1 : 0; return 1; }
    return Popup::getBoolProperty(prop, out);
}
int Dialog::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kConfirmText) == 0) { out = m_btnConfirmText.c_str(); return 1; }
    if (strcmp(prop, PropertyNames::kCancelText) == 0)  { out = m_btnCancelText.c_str();  return 1; }
    return Popup::getStringProperty(prop, out);
}
int ConfirmPopup::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kConfirmButton) == 0) { out = m_btnConfirm ? static_cast<Control*>(m_btnConfirm.get()) : nullptr; return 1; }
    return Popup::getPtrProperty(prop, out);
}
int Dialog::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kCancelButton) == 0) { out = m_btnCancel ? static_cast<Control*>(m_btnCancel.get()) : nullptr; return 1; }
    return ConfirmPopup::getPtrProperty(prop, out);
}
int Dialog::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventConfirm) == 0 ||
        strcmp(event, PropertyNames::kEventCancel) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return Popup::setCallbackProperty(event, cb, userData);
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
