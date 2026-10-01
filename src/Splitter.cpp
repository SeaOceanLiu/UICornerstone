#define NOMINMAX
#include "Splitter.h"
#include "Window.h"
#include "LayoutEngine.h"
#include "Panel.h"
#include "GraphTool.h"
#include "PropertyNames.h"
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
    , m_colorDisabled(ConstDef::SPLITTER_COLOR_NORMAL)
    , m_lineNormalSet(false)
    , m_lineHoverSet(false)
    , m_hovered(false)
    , m_cursorResize(nullptr), m_cursorDefault(nullptr)
    , m_lastRect()
    , m_onSplitterMoved(nullptr)
{
    m_ctlType = ControlType::Splitter;
    m_rect = rect;
    setFocusable(true);
}

Splitter::~Splitter() {
    cleanupCursors();
}

void Splitter::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建
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

    // P0-42：把手线色 = 通用组（colors.background 三态）驱动，line* 专用键优先；disabled 独立
    SColor lineColor = m_colorNormal;
    if (!m_enable)       lineColor = m_colorDisabled;
    else if (m_dragging) lineColor = m_colorDrag;
    else if (m_hovered)  lineColor = m_colorHover;
    dev->setDrawColor(lineColor);
    dev->fillRect(m_frameDrawRect);

    afterDraw();  // draws border, draws focus ring
}

bool Splitter::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;
    // 三模式均可拖拽：
    // - linked 自动模式（引擎外）：需 linked 面板存活（ensureControls）
    // - 引擎模式（布局引擎容器内）/ 手柄模式（无 linked）：无面板也可拖
    //   （手柄模式：拖动自身 + 上报 ratio，布局由应用侧维护——见 Splitter_Design §8.9）
    bool hasLinked = (m_first || m_second);
    if (!m_dragging && !isEngineManaged() && hasLinked && !ensureControls()) return false;

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
    // 引擎模式：rect 由引擎写入，仅拖拽权重触发重排（不自动联动）；
    // 引擎外：linked 自动布局（first/second 独占父容器）
    if (!isEngineManaged() && m_first && m_second) applySplitRatio(m_splitRatio);
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

void Splitter::setFirstControl(shared_ptr<Control> first) {
    m_firstWeak = first; m_first = first.get();
}

void Splitter::setSecondControl(shared_ptr<Control> second) {
    m_secondWeak = second; m_second = second.get();
    if (m_first && m_second) applySplitRatio(m_splitRatio);
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
    if (m_splitRatio != oldRatio) {
        if (m_onSplitterMoved)
            m_onSplitterMoved(std::static_pointer_cast<Splitter>(shared_from_this()), m_splitRatio);
        fireCCallback(PropertyNames::kEventMoved, CCallbackData::Float, &m_splitRatio);
    }
}

void Splitter::setColor(SColor n, SColor h, SColor d) { m_colorNormal = n; m_colorHover = h; m_colorDrag = d; }
void Splitter::setOnSplitterMoved(OnSplitterMovedHandler h) { m_onSplitterMoved = h; }

// ── Private ──

bool Splitter::isEngineManaged() {
    // 父容器为布局引擎容器（Panel + layoutEngine）→ 引擎分段模式（设计 §8.3）
    auto* pid = dynamic_cast<Panel*>(getParent());
    return pid && pid->getLayoutEngine() != nullptr;
}

void Splitter::updateEngineDrag(float deltaLogic) {
    auto* pctl = dynamic_cast<Panel*>(getParent());
    if (!pctl || !pctl->getLayoutEngine()) return;

    static bool tempDiag = false;
    (void)tempDiag;

    auto& kids = pctl->getChildren();
    int idx = -1;
    for (size_t i = 0; i < kids.size(); ++i)
        if (kids[i].get() == this) { idx = (int)i; break; }
    if (idx <= 0) return;

    // 前段/后段边界：本 splitter 前后连续非 splitter 元素群（典型 = 各一面板）
    int prevStart = idx - 1;
    while (prevStart >= 0 && kids[prevStart]->getControlType() == ControlType::Splitter) --prevStart;
    if (prevStart < 0) return;
    auto& prevEl = kids[prevStart];
    float prevFw = pctl->getChildFlowWeight(prevEl.get());
    SRect pr = prevEl->getRect();
    float segW = m_orientation ? pr.width : pr.height;

    // 后段总宽（下一 splitter 之前所有非 splitter 元素宽；典型 = 一面板）
    int rearEnd = idx + 1;
    while (rearEnd < (int)kids.size() && kids[rearEnd]->getControlType() != ControlType::Splitter) ++rearEnd;
    if (rearEnd <= idx + 1) return;
    auto& rearEl = kids[idx + 1];
    SRect rr = rearEl->getRect();
    float rearW = m_orientation ? rr.width : rr.height;
    float rearFw = pctl->getChildFlowWeight(rearEl.get());
    float totalSpan = segW + rearW;

    // 首帧：锁定拖拽起始的前段/后段宽（后续帧一律基于它累加）
    if (m_dragStartSegW < 0.0f) {
        m_dragStartSegW = segW;
        m_dragStartRearW = rearW;
    }

    // 其余段固定项（前段/后段之外，fw<=0 且非 splitter 的宽）
    float fixedOthers = 0.0f;
    for (int j = 0; j < (int)kids.size(); ++j) {
        if (j == prevStart || (j > idx && j < rearEnd)) continue;
        auto& k = kids[j];
        if (k->getControlType() == ControlType::Splitter) continue;
        if (pctl->getChildFlowWeight(k.get()) > 0.0f) continue;
        SRect r2 = k->getRect();
        fixedOthers += m_orientation ? r2.width : r2.height;
    }
    float containerInner = m_orientation ? pctl->getRect().width : pctl->getRect().height;
    float splitterThick = getThickness();
    float maxByFixed = containerInner - fixedOthers - splitterThick;

    // 两式拖拽语义（CornerstoneDesigner 布局：左右固定 + 中间弹性）：
    // 1) 前段固定 → 修改前段固定宽（左边界拖动）
    // 2) 前段弹性 & 后段固定 → 修改后段固定宽（右边界拖动），前段弹性自动补偿
    // 3) 双弹性段 → 降级：前段锁定为固定宽（基础保护）
    if (prevFw <= 0.0f) {
        // 固定基准：起点宽 + 累计 delta（而非"当前宽+累计 delta"——避免双重累加）
        float target = std::clamp(m_dragStartSegW + deltaLogic, m_minFirst,
            std::min(totalSpan - m_minSecond, maxByFixed - m_minSecond));
        if (m_orientation)
            prevEl->setRect(SRect(pr.left, pr.top, target, pr.height));
        else
            prevEl->setRect(SRect(pr.left, pr.top, pr.width, target));
    } else if (rearFw <= 0.0f) {
        // 后段固定：拖动边界 → 后段宽 = span - (前段宽+delta)
        float targetRear = std::clamp(m_dragStartRearW - deltaLogic, m_minSecond,
            std::max(m_minSecond, totalSpan - m_minFirst));
        if (m_orientation)
            rearEl->setRect(SRect(rr.left, rr.top, targetRear, rr.height));
        else
            rearEl->setRect(SRect(rr.left, rr.top, rr.width, targetRear));
    } else {
        float target = std::clamp(m_dragStartSegW + deltaLogic, m_minFirst, totalSpan - m_minSecond);
        pctl->setChildFlowProps(prevEl.get(), FlowItemProps{0.0f});
        if (m_orientation)
            prevEl->setRect(SRect(pr.left, pr.top, target, pr.height));
        else
            prevEl->setRect(SRect(pr.left, pr.top, pr.width, target));
    }
    pctl->reflowChildren();

    // ratio 回调：splitter 中线 / 容器内宽（逻辑坐标）
    Control* p = getParent();
    float inner = p ? (m_orientation ? p->getRect().width : p->getRect().height) : 0.0f;
    if (inner > 0.0f) {
        SRect sr = getRect();
        float center = m_orientation ? (sr.left + m_thickness * 0.5f) : (sr.top + m_thickness * 0.5f);
        float r = std::clamp(center / inner, 0.0f, 1.0f);
        if (r != m_splitRatio) {
            m_splitRatio = r;
            if (m_onSplitterMoved)
                m_onSplitterMoved(std::static_pointer_cast<Splitter>(shared_from_this()), r);
            fireCCallback(PropertyNames::kEventMoved, CCallbackData::Float, &m_splitRatio);
        }
    }
}

bool Splitter::ensureControls() {
    auto f = m_firstWeak.lock();
    auto s = m_secondWeak.lock();
    if (!f || f.get() != m_first) m_first = nullptr;
    if (!s || s.get() != m_second) m_second = nullptr;
    return m_first && m_second;
}

void Splitter::applySplitRatio(float ratio) {
    // 重入保护：引擎 reflow 写回 first/second rect 时防止递归死循环
    // （此前 m_lastRect 被清空导致引擎↔splitter 互相触发直至栈溢出，见 Splitter_Design §8.1）
    if (m_applyingRatio) return;
    m_applyingRatio = true;

    if (!m_first || !m_second) { m_applyingRatio = false; return; }
    Control* p = getParent();
    if (!p) { m_applyingRatio = false; return; }

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
    m_applyingRatio = false;
}

void Splitter::startDrag(const SPoint& mousePos) {
    m_dragging = true;
    if (!m_dragWatcherRegistered) {
        EventQueue* eq = m_context->eventQueue;
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
    // 引擎模式：记录"前段/后段"拖拽起始宽——目标 = 起点宽 + 累计 delta（固定基准，
    // 防止每帧以"已含 delta 的当前宽"再叠加累计 delta 造成双重累加（超动 bug）
    m_dragStartSegW = -1.0f;   // -1 = 未初始化（updateEngineDrag 首帧填充）
}

void Splitter::updateDrag(const SPoint& mousePos) {
    if (!m_dragging) return;

    // 坐标换算：视口（绘制）坐标 → 画布/逻辑坐标（经 mapViewportToCanvas，
    // 含控件/视口缩放全链）——保证任意缩放(zoom/DPI/fit)下 1:1 跟手。
    // 两次 map 的基准偏移相同，差值即逻辑位移。
    SPoint curLocal = mapViewportToCanvas(mousePos);
    SPoint startLocal = mapViewportToCanvas(m_dragStartMousePos);
    float delta = m_orientation ? (curLocal.x - startLocal.x)
                                : (curLocal.y - startLocal.y);

    // 引擎模式：拖拽只调整"前段弹性权重"并请引擎重排（引擎统一计算整链位置）
    if (isEngineManaged()) {
        updateEngineDrag(delta);
        return;
    }

    // 手柄模式（无 linked）：仅移动自身并上报 ratio，布局由应用侧（onSplitterMoved）维护
    if (!m_first || !m_second) {
        Control* p = getParent();
        if (!p) return;
        SRect pr = p->getRect();
        if (m_orientation) {
            float inner = pr.width - m_thickness;
            m_rect.left = std::clamp(m_rect.left + delta, 0.0f, std::max(0.0f, inner));
        } else {
            float inner = pr.height - m_thickness;
            m_rect.top = std::clamp(m_rect.top + delta, 0.0f, std::max(0.0f, inner));
        }
        m_lastRect = SRect();
        float inner = m_orientation ? pr.width : pr.height;
        if (inner > 0.0f) {
            float center = m_orientation ? (m_rect.left + m_thickness * 0.5f)
                                         : (m_rect.top + m_thickness * 0.5f);
            float r = std::clamp(center / inner, 0.0f, 1.0f);
            if (r != m_splitRatio) {
                m_splitRatio = r;
                if (m_onSplitterMoved)
                    m_onSplitterMoved(std::static_pointer_cast<Splitter>(shared_from_this()), r);
                fireCCallback(PropertyNames::kEventMoved, CCallbackData::Float, &m_splitRatio);
            }
        }
        return;
    }

    Control* p = getParent();
    if (!p) return;
    float ps = m_orientation ? p->getScaleXX() : p->getScaleYY();

    // delta 已为画布/逻辑位移（updateDrag 顶部 mapViewportToCanvas 换算），不再 /ps
    float rawNewPos = m_dragStartLocalPos + delta;

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
    // 引擎模式：拖拽期间权重已实时生效（每帧 reflow），无收尾联动
    if (isEngineManaged()) return;
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

    float oldRatio = m_splitRatio;
    if (p && parentTotal > 0)
        m_splitRatio = std::clamp(firstSize / parentTotal, 0.0f, 1.0f);

    if (m_splitRatio != oldRatio) {
        if (m_onSplitterMoved)
            m_onSplitterMoved(std::static_pointer_cast<Splitter>(shared_from_this()), m_splitRatio);
        fireCCallback(PropertyNames::kEventMoved, CCallbackData::Float, &m_splitRatio);
    }

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

// ── Property system overrides ──

int Splitter::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kLine) == 0)      { m_colorNormal = color; m_lineNormalSet = true; return 1; }
    if (strcmp(prop, PropertyNames::kLineHover) == 0)  { m_colorHover  = color; m_lineHoverSet  = true; return 1; }
    if (strcmp(prop, PropertyNames::kLineDrag) == 0)   { m_colorDrag   = color; return 1; }
    // P0-42：通用组（colors.background 三态）驱动把手线色；Set 顺序无关（line* 专用键优先）
    if (strcmp(prop, PropertyNames::kBackground) == 0)    { if (!m_lineNormalSet) m_colorNormal = color; return 1; }
    if (strcmp(prop, PropertyNames::kStateHover) == 0)    { if (!m_lineHoverSet)  m_colorHover  = color; return 1; }
    if (strcmp(prop, PropertyNames::kStateDisabled) == 0) { m_colorDisabled = color; return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

// P0-42：colors.background 对象路径（normal/hover/disabled 三态）→ 把手线色
void Splitter::setBackgroundStateColor(StateColor stateColor) {
    if (!m_lineNormalSet) m_colorNormal = stateColor.getNormal();
    if (!m_lineHoverSet)  m_colorHover  = stateColor.getHover();
    m_colorDisabled = stateColor.getDisabled();
}
StateColor Splitter::getBackgroundStateColor(void) {
    // 读回 = 有效线色（pressed 取 hover 语义）
    return StateColor(m_colorNormal, m_colorHover, m_colorHover, m_colorDisabled);
}

int Splitter::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kRatio) == 0)     { setSplitRatio(value); return 1; }
    if (strcmp(prop, PropertyNames::kThickness) == 0) { setThickness(value);  return 1; }
    if (strcmp(prop, PropertyNames::kEdgeMargin) == 0){ m_minFirst = value; m_minSecond = value; return 1; }
    if (strcmp(prop, PropertyNames::kFirstMin) == 0)  { m_minFirst = value; return 1; }
    if (strcmp(prop, PropertyNames::kSecondMin) == 0) { m_minSecond = value; return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}

int Splitter::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kLine) == 0)      { out = m_colorNormal; return 1; }
    if (strcmp(prop, PropertyNames::kLineHover) == 0)  { out = m_colorHover;  return 1; }
    if (strcmp(prop, PropertyNames::kLineDrag) == 0)   { out = m_colorDrag;   return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int Splitter::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kValue) == 0)    { out = m_splitRatio; return 1; }
    if (strcmp(prop, PropertyNames::kRatio) == 0)    { out = m_splitRatio; return 1; }
    if (strcmp(prop, PropertyNames::kRangeMin) == 0) { out = m_minFirst;   return 1; }
    if (strcmp(prop, PropertyNames::kRangeMax) == 0) { out = m_minSecond;  return 1; }
    if (strcmp(prop, PropertyNames::kFirstMin) == 0) { out = m_minFirst;   return 1; }
    if (strcmp(prop, PropertyNames::kSecondMin) == 0){ out = m_minSecond;  return 1; }
    if (strcmp(prop, PropertyNames::kThickness) == 0){ out = m_thickness;  return 1; }
    if (strcmp(prop, PropertyNames::kEdgeMargin) == 0){ out = m_minFirst;  return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

int Splitter::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventMoved) == 0 ||
        strcmp(event, PropertyNames::kEventPositionChanged) == 0) {
        return ControlImpl::setCallbackProperty(PropertyNames::kEventMoved, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

int Splitter::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kHorizontal) == 0) { setOrientation(value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int Splitter::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kHorizontal) == 0) { out = m_orientation ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int Splitter::setPtrProperty(const char* prop, void* value) {
    if (strcmp(prop, PropertyNames::kFirstLinked) == 0) {
        auto* impl = dynamic_cast<ControlImpl*>(static_cast<Control*>(value));
        if (impl) setFirstControl(impl->shared_from_this());
        return 1;
    }
    if (strcmp(prop, PropertyNames::kSecondLinked) == 0) {
        auto* impl = dynamic_cast<ControlImpl*>(static_cast<Control*>(value));
        if (impl) setSecondControl(impl->shared_from_this());
        return 1;
    }
    return ControlImpl::setPtrProperty(prop, value);
}

int Splitter::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kFirstLinked) == 0) {
        out = static_cast<void*>(m_first);
        return m_first ? 1 : 0;
    }
    if (strcmp(prop, PropertyNames::kSecondLinked) == 0) {
        out = static_cast<void*>(m_second);
        return m_second ? 1 : 0;
    }
    return ControlImpl::getPtrProperty(prop, out);
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
