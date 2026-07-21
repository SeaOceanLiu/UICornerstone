#define NOMINMAX
#include "Splitter.h"
#include "GraphTool.h"
#include "PlatformUtils.h"
#include "EventQueue.h"
#include <algorithm>

Splitter::Splitter(Control* parent, const SRect& rect,
                   float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_orientation(true)
    , m_first(nullptr), m_second(nullptr)
    , m_thickness(ConstDef::SPLITTER_THICKNESS_H)
    , m_minFirst(ConstDef::SPLITTER_MIN_SIZE_DEFAULT), m_minSecond(ConstDef::SPLITTER_MIN_SIZE_DEFAULT)
    , m_splitRatio(0.5f)
    , m_dragging(false)
    , m_dragWatcherRegistered(false)
    , m_dragStartRatio(0.5f)
    , m_dragStartMousePos{0,0}
    , m_dragStartScreenPos(0.0f)
    , m_dragStartLocalPos(0.0f)
    , m_lastClickTime(0)
    , m_colorNormal(ConstDef::SPLITTER_COLOR_NORMAL)
    , m_colorHover(ConstDef::SPLITTER_COLOR_HOVER)
    , m_colorDrag(ConstDef::SPLITTER_COLOR_DRAG)
    , m_hovered(false)
    , m_cursorResize(nullptr), m_cursorDefault(nullptr)
    , m_lastRect()
    , m_onSplitterMoved(nullptr)
{
    m_rect = rect;
    setFocusable(true);
}

Splitter::~Splitter() {
    cleanupCursors();
}

void Splitter::create() {
    if (m_isCreated) return;
    ControlImpl::create();
    m_focusable = false;  // 强制 FocusManager 重新注册
    setFocusable(true);
    m_isCreated = true;
}

void Splitter::draw() {
    if (!m_visible) return;
    auto* dev = getRenderDevice();
    if (!dev) return;

    beforeDraw();  // sets m_frameDrawRect, m_frameDrawRectValid=true, draws background

    dev->setDrawColor(m_dragging ? m_colorDrag
                      : m_hovered ? m_colorHover : m_colorNormal);
    dev->fillRect(m_frameDrawRect);

    afterDraw();  // draws border, draws focus ring
}

bool Splitter::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;
    if (!m_first && !m_second) return false;
    if (!ensureControls() && !m_dragging) return false;

    if (m_dragging) {
        if (event->m_type == EventType::MouseMove) {
            updateDrag({event->mousePos.x, event->mousePos.y});
            return true;
        }
        if (event->m_type == EventType::MouseUp) {
            endDrag();
            return true;
        }
        return false;
    }

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        if (isContainsPoint(event->mouseButton.x, event->mouseButton.y)) {
            handleDoubleClick();
            startDrag({event->mouseButton.x, event->mouseButton.y});
            return true;
        }
    }

    if (event->m_type == EventType::MouseMove) {
        bool inside = isContainsPoint(event->mousePos.x, event->mousePos.y);
        if (inside != m_hovered) { m_hovered = inside; }
        updateCursor(inside);
    }
    if (event->m_type == EventType::KeyDown && getFocused()) {
        handleKeyEvent(event);
        return true;
    }
    return false;
}

void Splitter::setRect(SRect rect) {
    if (rect == m_lastRect) return;
    m_lastRect = rect;
    ControlImpl::setRect(rect);
    if (m_first && m_second) applySplitRatio(m_splitRatio);
}

void Splitter::setOrientation(bool horizontal) {
    m_orientation = horizontal;
    m_thickness = horizontal ? ConstDef::SPLITTER_THICKNESS_H : ConstDef::SPLITTER_THICKNESS_V;
}

void Splitter::setLinkedControls(shared_ptr<Control> first, shared_ptr<Control> second) {
    m_firstWeak = first; m_secondWeak = second;
    m_first = first.get(); m_second = second.get();
    applySplitRatio(m_splitRatio);
}

void Splitter::clearLinkedControls() {
    m_firstWeak.reset(); m_secondWeak.reset();
    m_first = nullptr; m_second = nullptr;
}

void Splitter::setMinSize(float a, float b) { m_minFirst = a; m_minSecond = b; }
void Splitter::setThickness(float px) { m_thickness = px; }

void Splitter::setSplitRatio(float ratio) {
    float oldRatio = m_splitRatio;
    m_splitRatio = std::clamp(ratio, 0.0f, 1.0f);
    if (m_first && m_second) {
        applySplitRatio(m_splitRatio);
        // 根据实际像素位置重算 ratio（避免 clamp 后仍为 0/1）
        Control* p = getParent();
        if (p) {
            float ps = m_orientation ? p->getScaleXX() : p->getScaleYY();
            float total = (m_orientation
                ? p->getDrawRect().width
                : p->getDrawRect().height) / ps - m_thickness;
            if (total > 0) {
                float actual = m_orientation
                    ? m_first->getRect().width
                    : m_first->getRect().height;
                m_splitRatio = std::clamp(actual / total, 0.0f, 1.0f);
            }
        }
    }
    if (m_onSplitterMoved && m_splitRatio != oldRatio)
        m_onSplitterMoved(std::static_pointer_cast<Splitter>(shared_from_this()), m_splitRatio);
}

void Splitter::setColor(SColor n, SColor h, SColor d) { m_colorNormal = n; m_colorHover = h; m_colorDrag = d; }
void Splitter::setOnSplitterMoved(OnSplitterMovedHandler h) { m_onSplitterMoved = h; }

// ── Private ──

bool Splitter::ensureControls() {
    auto f = m_firstWeak.lock();
    auto s = m_secondWeak.lock();
    if (!f || f.get() != m_first) m_first = nullptr;
    if (!s || s.get() != m_second) m_second = nullptr;
    return m_first && m_second;
}

void Splitter::applySplitRatio(float ratio) {
    if (!m_first || !m_second) return;
    Control* p = getParent();
    if (!p) return;

    float sx = getScaleXX(), sy = getScaleYY();
    float thickPx = m_thickness * (m_orientation ? sx : sy);
    SRect pr = p->getDrawRect();

    float total, minFirstPx, minSecondPx;
    SRect fr, sr;
    if (m_orientation) {
        total = pr.width - thickPx;
        minFirstPx = m_minFirst * sx;
        minSecondPx = m_minSecond * sx;
    } else {
        total = pr.height - thickPx;
        minFirstPx = m_minFirst * sy;
        minSecondPx = m_minSecond * sy;
    }
    float firstPx = std::clamp(total * ratio, minFirstPx, total - minSecondPx);
    float secondPx = total - firstPx;

    if (m_orientation) {
        fr = m_first->getRect(); sr = m_second->getRect();
        m_first->setRect({fr.left, fr.top, firstPx / sx, fr.height});
        m_second->setRect({fr.left + firstPx / sx + m_thickness, sr.top, secondPx / sx, sr.height});
        m_rect.left = fr.left + firstPx / sx;
    } else {
        fr = m_first->getRect(); sr = m_second->getRect();
        m_first->setRect({fr.left, fr.top, fr.width, firstPx / sy});
        m_second->setRect({sr.left, fr.top + firstPx / sy + m_thickness, sr.width, secondPx / sy});
        m_rect.top = fr.top + firstPx / sy;
    }
    m_lastRect = SRect();
}

void Splitter::startDrag(const SPoint& mousePos) {
    m_dragging = true;
    if (!m_dragWatcherRegistered) {
        EventQueue* eq = EventQueue::getInstance();
        eq->addBeforeEventHandlingWatcher(EventType::MouseDown, getThis());
        eq->addBeforeEventHandlingWatcher(EventType::MouseMove, getThis());
        eq->addBeforeEventHandlingWatcher(EventType::MouseUp, getThis());
        m_dragWatcherRegistered = true;
    }
    ensureCursors();
    if (m_cursorResize) Cursor::setCurrent(m_cursorResize);
    m_dragStartMousePos = mousePos;
    m_dragStartRatio = m_splitRatio;
    m_dragStartLocalPos = m_orientation ? m_rect.left : m_rect.top;
}

void Splitter::updateDrag(const SPoint& mousePos) {
    if (!m_dragging || !m_first || !m_second) return;

    float cur = m_orientation ? mousePos.x : mousePos.y;
    float start = m_orientation ? m_dragStartMousePos.x : m_dragStartMousePos.y;
    float delta = cur - start;

    Control* p = getParent();
    if (!p) return;
    float ps = m_orientation ? p->getScaleXX() : p->getScaleYY();

    float rawNewPos = m_dragStartLocalPos + delta / ps;

    if (m_orientation) {
        float firstLeft = m_first->getRect().left;
        float minL = m_minFirst + firstLeft;
        float maxL = (p->getDrawRect().width / ps) - m_minSecond - m_thickness + firstLeft;
        m_rect.left = std::clamp(rawNewPos, minL, maxL);
        m_first->setRect({firstLeft, m_first->getRect().top,
            m_rect.left - firstLeft, m_first->getRect().height});
        m_second->setRect({m_rect.left + m_thickness, m_second->getRect().top,
            (p->getDrawRect().width / ps) - m_thickness - (m_rect.left - firstLeft), m_second->getRect().height});
    } else {
        float firstTop = m_first->getRect().top;
        float minL = m_minFirst + firstTop;
        float maxL = (p->getDrawRect().height / ps) - m_minSecond - m_thickness + firstTop;
        m_rect.top = std::clamp(rawNewPos, minL, maxL);
        m_first->setRect({m_first->getRect().left, firstTop,
            m_first->getRect().width, m_rect.top - firstTop});
        m_second->setRect({m_second->getRect().left, m_rect.top + m_thickness,
            m_second->getRect().width, (p->getDrawRect().height / ps) - m_thickness - (m_rect.top - firstTop)});
    }
    m_lastRect = SRect();
}

void Splitter::endDrag() {
    if (!m_dragging) return;
    m_dragging = false;
    if (!m_first || !m_second) return;

    ::SRect fRect = m_first->getRect();
    float firstSize, secondSize;
    Control* p = getParent();
    float parentTotal = 0;

    if (m_orientation) {
        firstSize = m_rect.left - fRect.left;
        if (p) parentTotal = (p->getDrawRect().width / p->getScaleXX()) - m_thickness;
        secondSize = parentTotal - firstSize;
        m_first->setRect({fRect.left, fRect.top, firstSize, fRect.height});
        m_second->setRect({m_rect.left + m_thickness, m_second->getRect().top, secondSize, m_second->getRect().height});
    } else {
        firstSize = m_rect.top - fRect.top;
        if (p) parentTotal = (p->getDrawRect().height / p->getScaleYY()) - m_thickness;
        secondSize = parentTotal - firstSize;
        m_first->setRect({fRect.left, fRect.top, fRect.width, firstSize});
        m_second->setRect({m_second->getRect().left, m_rect.top + m_thickness, m_second->getRect().width, secondSize});
    }

    if (p && parentTotal > 0)
        m_splitRatio = std::clamp(firstSize / parentTotal, 0.0f, 1.0f);

    if (m_onSplitterMoved)
        m_onSplitterMoved(std::static_pointer_cast<Splitter>(shared_from_this()), m_splitRatio);

    // 不在此处 removeBeforeEventHandlingWatcher：
    // endDrag() 可能从 beforeEventHandlingWatcher 内部调用，
    // 此时 EventQueue 已持有 m_mtxForBeforeEventHandlingWatcher，递归 lock → UB。
    // 不拖拽时 watcher 检查 m_dragging 直接返回 false，是安全的。
    // std::weak_ptr 在 EventQueue 内部确保 watcher 不延长控件生命周期，
    // 避免静态析构顺序问题。
}

bool Splitter::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    if (!m_dragging) return false;
    if (event->m_type == EventType::MouseMove) {
        ensureCursors();
        if (m_cursorResize) Cursor::setCurrent(m_cursorResize);
        updateDrag({event->mousePos.x, event->mousePos.y});
        return true;
    }
    if (event->m_type == EventType::MouseUp) {
        endDrag();
        return true;
    }
    if (event->m_type == EventType::MouseDown) {
        endDrag();
        return true;
    }
    return false;
}

void Splitter::handleKeyEvent(shared_ptr<Event> event) {
    if (!m_first || !m_second) return;
    float step = ConstDef::SPLITTER_KEY_STEP;
    if (isModSet(event->keyEvent.mod, KeyMod::Shift)) step = ConstDef::SPLITTER_KEY_FINE_STEP;

    Control* p = getParent();
    if (!p) return;
    float total = m_orientation
        ? ((p->getDrawRect().width / p->getScaleXX()) - m_thickness)
        : ((p->getDrawRect().height / p->getScaleYY()) - m_thickness);
    float ratioStep = (total > 0) ? step / total : 0;

    switch (event->keyEvent.keycode) {
        case KeyCode::Left:  if (m_orientation) setSplitRatio(m_splitRatio - ratioStep); break;
        case KeyCode::Right: if (m_orientation) setSplitRatio(m_splitRatio + ratioStep); break;
        case KeyCode::Up:    if (!m_orientation) setSplitRatio(m_splitRatio - ratioStep); break;
        case KeyCode::Down:  if (!m_orientation) setSplitRatio(m_splitRatio + ratioStep); break;
        case KeyCode::Home:  setSplitRatio(0.5f); break;
        default: break;
    }
}

void Splitter::handleDoubleClick() {
    uint64_t now = Platform::GetTicks();
    if (m_lastClickTime > 0 && now - m_lastClickTime < ConstDef::SPLITTER_DOUBLE_CLICK_MS) {
        setSplitRatio(0.5f);
        m_lastClickTime = 0;
    } else {
        m_lastClickTime = now;
    }
}

void Splitter::ensureCursors() {
    if (!m_cursorResize)
        m_cursorResize = Cursor::createSystem(
            m_orientation ? SystemCursorType::EW_Resize : SystemCursorType::NS_Resize);
}

void Splitter::cleanupCursors() {
    delete m_cursorResize; m_cursorResize = nullptr;
    m_cursorDefault = nullptr;  // Cursor::getDefault() returns a static/backend-owned cursor, do not delete
}

void Splitter::updateCursor(bool inside) {
    if (inside) { ensureCursors(); if (m_cursorResize) Cursor::setCurrent(m_cursorResize); }
    else { if (!m_cursorDefault) m_cursorDefault = Cursor::getDefault(); Cursor::setCurrent(m_cursorDefault); }
}

// ── Builder ──

SplitterBuilder::SplitterBuilder(Control* parent, const SRect& rect, float xScale, float yScale)
    : m_splitter(make_shared<Splitter>(parent, rect, xScale, yScale)) {}

SplitterBuilder& SplitterBuilder::setOrientation(bool h) { m_splitter->m_orientation = h; return *this; }
SplitterBuilder& SplitterBuilder::setLinkedControls(shared_ptr<Control> f, shared_ptr<Control> s) { m_splitter->setLinkedControls(f, s); return *this; }
SplitterBuilder& SplitterBuilder::setMinSize(float a, float b) { m_splitter->m_minFirst = a; m_splitter->m_minSecond = b; return *this; }
SplitterBuilder& SplitterBuilder::setThickness(float px) { m_splitter->m_thickness = px; return *this; }
SplitterBuilder& SplitterBuilder::setSplitRatio(float r) { m_splitter->m_splitRatio = r; return *this; }
SplitterBuilder& SplitterBuilder::setColor(SColor n, SColor h, SColor d) { m_splitter->m_colorNormal = n; m_splitter->m_colorHover = h; m_splitter->m_colorDrag = d; return *this; }
SplitterBuilder& SplitterBuilder::setOnSplitterMoved(Splitter::OnSplitterMovedHandler cb) { m_splitter->m_onSplitterMoved = cb; return *this; }
SplitterBuilder& SplitterBuilder::setBackgroundStateColor(StateColor sc) { m_splitter->setBackgroundStateColor(sc); return *this; }
SplitterBuilder& SplitterBuilder::setBorderStateColor(StateColor sc) { m_splitter->setBorderStateColor(sc); return *this; }
SplitterBuilder& SplitterBuilder::setId(int id) { m_splitter->setId(id); return *this; }

shared_ptr<Splitter> SplitterBuilder::build() { m_splitter->create(); return m_splitter; }
