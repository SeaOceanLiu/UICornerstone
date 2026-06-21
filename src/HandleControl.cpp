#include "HandleControl.h"
#include "Cursor.h"
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

HandleControl::HandleControl()
    : ControlImpl(nullptr)
{
    m_visible = true;
}

HandleControl::~HandleControl()
{
    m_target = nullptr;
    m_targetWeak.reset();
    m_resizing = false;
    m_dragging = false;
    m_activeHandle = HandleType::None;
    setAlwaysOnTop(false);
    // 不调用 detach()：析构时 shared_from_this() 无效，
    // 父容器的 m_children 析构或显式 removeControl 会自动清理 vector
    cleanupCursors();
}

void HandleControl::setTarget(shared_ptr<Control> target)
{
    if (m_target) detach();
    m_target = target.get();
    m_targetWeak = target;
    m_resizing = false;
    m_dragging = false;
    m_activeHandle = HandleType::None;

    Control* parent = target->getParent();
    if (parent) {
        parent->addControl(shared_from_this());
        setAlwaysOnTop(true);
        setVisible(true);
    }
}

void HandleControl::detach()
{
    m_target = nullptr;
    m_targetWeak.reset();
    m_resizing = false;
    m_dragging = false;
    m_activeHandle = HandleType::None;
    setAlwaysOnTop(false);
#ifdef _WIN32
    SetCursor(LoadCursorA(NULL, IDC_ARROW));
#else
    Cursor::setCurrent(m_cursorDefault ? m_cursorDefault : Cursor::getDefault());
#endif

    Control* parent = getParent();
    if (parent) {
        parent->removeControl(shared_from_this());
    }
}

void HandleControl::ensureCursors()
{
    if (!m_cursorDefault) m_cursorDefault = Cursor::createSystem(SystemCursorType::Default);
    if (!m_cursorMove)    m_cursorMove    = Cursor::createSystem(SystemCursorType::Move);
    if (!m_cursorWE)      m_cursorWE      = Cursor::createSystem(SystemCursorType::EW_Resize);
    if (!m_cursorNS)      m_cursorNS      = Cursor::createSystem(SystemCursorType::NS_Resize);
    if (!m_cursorNWSE)    m_cursorNWSE    = Cursor::createSystem(SystemCursorType::NWSE_Resize);
    if (!m_cursorNESW)    m_cursorNESW    = Cursor::createSystem(SystemCursorType::NESW_Resize);
}

void HandleControl::cleanupCursors()
{
    delete m_cursorDefault; m_cursorDefault = nullptr;
    delete m_cursorMove;    m_cursorMove    = nullptr;
    delete m_cursorWE;      m_cursorWE      = nullptr;
    delete m_cursorNS;      m_cursorNS      = nullptr;
    delete m_cursorNWSE;    m_cursorNWSE    = nullptr;
    delete m_cursorNESW;    m_cursorNESW    = nullptr;
}

// ── 坐标转换 ──

SRect HandleControl::targetToScreen()
{
    if (!m_target) return SRect();
    return m_target->getDrawRect();
}

SPoint HandleControl::screenToTargetLocal(float sx, float sy)
{
    if (!m_target || !m_target->getParent())
        return SPoint(sx, sy);

    Control* parent = m_target->getParent();
    SRect parentDraw = parent->getDrawRect();
    float scaleX = parent->getScaleXX();
    float scaleY = parent->getScaleYY();

    return SPoint(
        (sx - parentDraw.left) / scaleX,
        (sy - parentDraw.top)  / scaleY
    );
}

// ── 手柄区域管理 ──

void HandleControl::updateHandleAreas(const SRect& targetScreen)
{
    m_handleAreaCount = 0;
    float h = m_handleSize / 2.0f;
    float l = targetScreen.left, t = targetScreen.top;
    float r = targetScreen.left + targetScreen.width;
    float b = targetScreen.top  + targetScreen.height;
    float cx = l + targetScreen.width  / 2;
    float cy = t + targetScreen.height / 2;

    auto addHandle = [&](HandleType type, float x, float y) {
        m_handleAreas[m_handleAreaCount].rect =
            SRect(x - h, y - h, m_handleSize, m_handleSize);
        m_handleAreas[m_handleAreaCount].type = type;
        m_handleAreaCount++;
    };

    if (m_showCornerHandles) {
        addHandle(HandleType::NW, l,  t);
        addHandle(HandleType::NE, r,  t);
        addHandle(HandleType::SE, r,  b);
        addHandle(HandleType::SW, l,  b);
    }
    if (m_showEdgeHandles) {
        addHandle(HandleType::N,  cx, t);
        addHandle(HandleType::E,  r,  cy);
        addHandle(HandleType::S,  cx, b);
        addHandle(HandleType::W,  l,  cy);
    }
    if (m_showMoveHandle)
        addHandle(HandleType::Move, cx, cy);
}

HandleControl::HandleType HandleControl::hitTestHandle(float mx, float my)
{
    for (int i = 0; i < m_handleAreaCount; i++) {
        if (m_handleAreas[i].rect.contains(mx, my))
            return m_handleAreas[i].type;
    }
    return HandleType::None;
}

// ── 缩放操作 ──

void HandleControl::startResize(HandleType type, const SPoint& mousePos)
{
    m_resizing = true;
    m_activeHandle = type;
    m_startTargetRect = m_target->getRect();
    m_startScreenRect = m_target->getDrawRect();
    m_startMousePos = mousePos;
}

void HandleControl::updateResize(const SPoint& mousePos)
{
    float dx = mousePos.x - m_startMousePos.x;
    float dy = mousePos.y - m_startMousePos.y;

    SRect newScreen = m_startScreenRect;

    switch (m_activeHandle) {
    case HandleType::SE:
        newScreen.width  += dx;
        newScreen.height += dy;
        break;
    case HandleType::NW:
        newScreen.left   += dx;
        newScreen.top    += dy;
        newScreen.width  -= dx;
        newScreen.height -= dy;
        break;
    case HandleType::NE:
        newScreen.top    += dy;
        newScreen.width  += dx;
        newScreen.height -= dy;
        break;
    case HandleType::SW:
        newScreen.left   += dx;
        newScreen.width  -= dx;
        newScreen.height += dy;
        break;
    case HandleType::N:
        newScreen.top    += dy;
        newScreen.height -= dy;
        break;
    case HandleType::S:
        newScreen.height += dy;
        break;
    case HandleType::E:
        newScreen.width  += dx;
        break;
    case HandleType::W:
        newScreen.left   += dx;
        newScreen.width  -= dx;
        break;
    default:
        return;
    }

    // 最小尺寸约束
    if (newScreen.width < m_minWidth) {
        if (m_activeHandle == HandleType::NW || m_activeHandle == HandleType::W || m_activeHandle == HandleType::SW)
            newScreen.left = m_startScreenRect.right() - m_minWidth;
        newScreen.width = m_minWidth;
    }
    if (newScreen.height < m_minHeight) {
        if (m_activeHandle == HandleType::NW || m_activeHandle == HandleType::N || m_activeHandle == HandleType::NE)
            newScreen.top = m_startScreenRect.bottom() - m_minHeight;
        newScreen.height = m_minHeight;
    }

    // 屏幕坐标 → 局部坐标
    Control* parent = m_target->getParent();
    SRect parentDraw = parent->getDrawRect();
    float scaleX = parent->getScaleXX();
    float scaleY = parent->getScaleYY();
    float targetScaleX = m_target->getScaleXX();
    float targetScaleY = m_target->getScaleYY();

    float localLeft   = (newScreen.left - parentDraw.left) / scaleX;
    float localTop    = (newScreen.top  - parentDraw.top)  / scaleY;
    float localWidth  = newScreen.width  / targetScaleX;
    float localHeight = newScreen.height / targetScaleY;

    m_target->setRect(SRect(localLeft, localTop, localWidth, localHeight));
}

void HandleControl::endResize()
{
    m_resizing = false;
    m_activeHandle = HandleType::None;
    setResizeCursor(HandleType::None);
}

// ── 移动操作 ──

void HandleControl::startDrag(const SPoint& mousePos)
{
    m_dragging = true;
    m_activeHandle = HandleType::Move;
    m_startTargetRect = m_target->getRect();
    m_startScreenRect = m_target->getDrawRect();
    m_startMousePos = mousePos;
}

void HandleControl::updateDrag(const SPoint& mousePos)
{
    float dx = mousePos.x - m_startMousePos.x;
    float dy = mousePos.y - m_startMousePos.y;

    Control* parent = m_target->getParent();
    SRect parentDraw = parent->getDrawRect();
    float scaleX = parent->getScaleXX();
    float scaleY = parent->getScaleYY();

    float newLeft = (m_startScreenRect.left + dx - parentDraw.left) / scaleX;
    float newTop  = (m_startScreenRect.top  + dy - parentDraw.top)  / scaleY;

    m_target->setRect(SRect(
        newLeft, newTop,
        m_startTargetRect.width,
        m_startTargetRect.height
    ));
}

void HandleControl::endDrag()
{
    m_dragging = false;
    m_activeHandle = HandleType::None;
    setResizeCursor(HandleType::None);
}

// ── 光标反馈 ──

void HandleControl::setResizeCursor(HandleType type)
{
    ensureCursors();
    Cursor* c = nullptr;
    switch (type) {
    case HandleType::Move: c = m_cursorMove; break;
    case HandleType::NW:
    case HandleType::SE:   c = m_cursorNWSE; break;
    case HandleType::NE:
    case HandleType::SW:   c = m_cursorNESW; break;
    case HandleType::N:
    case HandleType::S:    c = m_cursorNS; break;
    case HandleType::E:
    case HandleType::W:    c = m_cursorWE; break;
    default:               c = m_cursorDefault; break;
    }
#ifdef _WIN32
    // Update backend internal state (SFML/GLFW need this for WM_SETCURSOR)
    if (c) Cursor::setCurrent(c);
    // Direct Win32 SetCursor — SDL3's WM_SETCURSOR uses DefWindowProc which
    // relies on the last SetCursor call, so SDL_SetCursor alone is insufficient.
    static HCURSOR hcursors[10] = {NULL};
    static bool hcInit = false;
    if (!hcInit) {
        hcInit = true;
        hcursors[(int)HandleType::None] = LoadCursorA(NULL, IDC_ARROW);
        hcursors[(int)HandleType::Move] = LoadCursorA(NULL, IDC_SIZEALL);
        hcursors[(int)HandleType::NW]   = LoadCursorA(NULL, IDC_SIZENWSE);
        hcursors[(int)HandleType::N]    = LoadCursorA(NULL, IDC_SIZENS);
        hcursors[(int)HandleType::NE]   = LoadCursorA(NULL, IDC_SIZENESW);
        hcursors[(int)HandleType::E]    = LoadCursorA(NULL, IDC_SIZEWE);
        hcursors[(int)HandleType::SE]   = LoadCursorA(NULL, IDC_SIZENWSE);
        hcursors[(int)HandleType::S]    = LoadCursorA(NULL, IDC_SIZENS);
        hcursors[(int)HandleType::SW]   = LoadCursorA(NULL, IDC_SIZENESW);
        hcursors[(int)HandleType::W]    = LoadCursorA(NULL, IDC_SIZEWE);
    }
    HCURSOR hc = hcursors[(int)type];
    if (hc) SetCursor(hc);
#else
    if (c) Cursor::setCurrent(c);
#endif
}

// ── 事件处理 ──

bool HandleControl::handleEvent(shared_ptr<Event> event)
{
    // 目标已过期
    if (m_targetWeak.expired()) {
        m_target = nullptr;
        m_targetWeak.reset();
        setVisible(false);
        return false;
    }
    if (!m_target) return false;

    // 处理拖拽中的鼠标事件
    if (m_resizing) {
        if (event->m_type == EventType::MouseMove) {
            setResizeCursor(m_activeHandle);
            updateResize(SPoint(event->mousePos.x, event->mousePos.y));
            return true;
        }
        if (event->m_type == EventType::MouseUp) {
            endResize();
            return true;
        }
        return false;
    }

    if (m_dragging) {
        if (event->m_type == EventType::MouseMove) {
            setResizeCursor(HandleType::Move);
            updateDrag(SPoint(event->mousePos.x, event->mousePos.y));
            return true;
        }
        if (event->m_type == EventType::MouseUp) {
            endDrag();
            return true;
        }
        return false;
    }

    // 非拖拽状态：仅处理命中手柄的事件
    if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        SPoint mp(event->mouseButton.x, event->mouseButton.y);
        HandleType ht = hitTestHandle(mp.x, mp.y);
        if (ht != HandleType::None) {
            if (ht == HandleType::Move)
                startDrag(mp);
            else
                startResize(ht, mp);
            setResizeCursor(ht);
            return true;
        }
        return false;
    }

    // 悬停时光标反馈（在 handleEvent 中实时更新手柄区域，不依赖 draw 顺序）
    if (event->m_type == EventType::MouseMove) {
        SRect targetScreen = m_target->getDrawRect();
        updateHandleAreas(targetScreen);
        HandleType ht = hitTestHandle(event->mousePos.x, event->mousePos.y);
        setResizeCursor(ht);
    }

    return false;
}

// ── 绘制 ──

void HandleControl::draw()
{
    if (m_targetWeak.expired()) {
        m_target = nullptr;
        m_targetWeak.reset();
        setVisible(false);
        return;
    }
    if (!m_target || !m_target->getVisible()) {
        return;
    }

    // 校正自身位置（跟随目标）
    SRect tr = m_target->getRect();
    float halfHandle = m_handleSize / 2.0f;
    setRect(SRect(
        tr.left - halfHandle,
        tr.top - halfHandle,
        tr.width  + m_handleSize,
        tr.height + m_handleSize
    ));

    // 获取目标控件屏幕坐标
    SRect targetScreen = m_target->getDrawRect();
    updateHandleAreas(targetScreen);

    GraphTool::DrawingContext ctx(getRenderDevice());

    // 虚线选择框
    if (m_showSelectionBox)
        drawSelectionBox(targetScreen);

    // 手柄方块（8 个边角/边中点 — 排除 Move，由 drawMoveHandle 单独绘制）
    for (int i = 0; i < m_handleAreaCount; i++) {
        if (m_handleAreas[i].type == HandleType::Move) continue;
        bool active = (m_resizing && m_handleAreas[i].type == m_activeHandle);
        drawHandle(m_handleAreas[i].rect, m_handleAreas[i].type, active);
    }

    // 中心十字移动指示器（绘制在移动手柄方块之上）
    if (m_showMoveHandle)
        drawMoveHandle(targetScreen);
}

// ── 绘制辅助方法 ──

void HandleControl::drawSelectionBox(const SRect& targetScreen)
{
    GraphTool::DrawingContext ctx(getRenderDevice());
    ctx.setPen(GraphTool::SPen(m_selectionColor, 1.5f, GraphTool::PenStyle::Dash));

    float l = targetScreen.left, t = targetScreen.top;
    float r = targetScreen.left + targetScreen.width;
    float b = targetScreen.top  + targetScreen.height;

    ctx.drawLine(l, t, r, t);
    ctx.drawLine(r, t, r, b);
    ctx.drawLine(r, b, l, b);
    ctx.drawLine(l, b, l, t);
}

void HandleControl::drawHandle(const SRect& rect, HandleType type, bool active)
{
    (void)type;
    GraphTool::DrawingContext ctx(getRenderDevice());

    SColor fill = active ? m_activeFill : m_handleFill;

    ctx.setPen(GraphTool::SPen(fill, 0));
    ctx.setBrush(GraphTool::SBrush(fill));
    ctx.drawRect(rect, true);

    ctx.setPen(GraphTool::SPen(m_handleBorder, 1.0f));
    ctx.setBrush(GraphTool::SBrush::NoBrush());
    ctx.drawRect(rect, false);
}

void HandleControl::drawMoveHandle(const SRect& targetScreen)
{
    float cx = targetScreen.left + targetScreen.width  / 2;
    float cy = targetScreen.top  + targetScreen.height / 2;
    float s = m_handleSize;

    GraphTool::DrawingContext ctx(getRenderDevice());

    // 白底蓝边十字：蓝线比白线两端各多 1px，完全包裹白色
    ctx.setPen(GraphTool::SPen(m_handleBorder, 5.0f));
    ctx.drawLine(cx,     cy - s - 1, cx,     cy + s + 1);
    ctx.drawLine(cx - s - 1, cy,     cx + s + 1, cy);

    ctx.setPen(GraphTool::SPen(m_handleFill, 3.0f));
    ctx.drawLine(cx,     cy - s, cx,     cy + s);
    ctx.drawLine(cx - s, cy,     cx + s, cy);
}

// ── HandleControlBuilder ──

HandleControlBuilder::HandleControlBuilder()
    : m_handle(make_shared<HandleControl>())
{
    m_handle->create();
}

HandleControlBuilder& HandleControlBuilder::setTarget(shared_ptr<Control> target) {
    m_handle->setTarget(target);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setHandleSize(float size) {
    m_handle->setHandleSize(size);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setMinSize(float w, float h) {
    m_handle->setMinSize(w, h);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setHandleColor(SColor fill, SColor border) {
    m_handle->setHandleColor(fill, border);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setActiveColor(SColor color) {
    m_handle->setActiveColor(color);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setSelectionColor(SColor color) {
    m_handle->setSelectionColor(color);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setCornerHandlesVisible(bool show) {
    m_handle->setCornerHandlesVisible(show);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setEdgeHandlesVisible(bool show) {
    m_handle->setEdgeHandlesVisible(show);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setMoveHandleVisible(bool show) {
    m_handle->setMoveHandleVisible(show);
    return *this;
}

HandleControlBuilder& HandleControlBuilder::setSelectionBoxVisible(bool show) {
    m_handle->setSelectionBoxVisible(show);
    return *this;
}

shared_ptr<HandleControl> HandleControlBuilder::build() {
    return m_handle;
}
