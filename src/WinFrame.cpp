#include "WinFrame.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

WinFrame::WinFrame(Control* parent, SRect rect, float xScale, float yScale):
    Panel(parent, rect, xScale, yScale),
    m_title("WinFrame"),
    m_titleBar(nullptr),
    m_titleLabel(nullptr),
    m_closeButton(nullptr),
    m_clientPanel(nullptr),
    m_dragging(false),
    m_resizing(false),
    m_resizeFlags(0),
    m_edgeMargin(4.0f),
    m_resizable(true),
    m_cursorDefault(SDL_GetCursor()),
    m_cursorSizeWE(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE)),
    m_cursorSizeNS(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE)),
    m_cursorSizeNWSE(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE)),
    m_cursorSizeNESW(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE))
{
    if (m_rect.width < MIN_WIDTH)  m_rect.width = MIN_WIDTH;
    if (m_rect.height < MIN_HEIGHT) m_rect.height = MIN_HEIGHT;

    setNormalStateBGColor(SDL_Color{0x30, 0x30, 0x30, 0xFF});
    setBorderVisible(true);
    setBorderStateColor(StateColor(
        SDL_Color{0x60, 0x60, 0x60, 0xFF},
        SDL_Color{0x80, 0x80, 0x80, 0xFF},
        SDL_Color{0x60, 0x60, 0x60, 0xFF},
        SDL_Color{0x60, 0x60, 0x60, 0xFF}));

    float titleH = ConstDef::WINDOW_TITLE_HEIGHT;

    addControl(m_closeButton = ButtonBuilder(this,
        SRect(m_rect.width - titleH, 0, titleH, titleH))
        .setNormalStateActor(    make_shared<Actor>(this, ResourceLoader::RID_cross_up_png, true))
        .setHoverStateActor(     make_shared<Actor>(this, ResourceLoader::RID_cross_over_png, true))
        .setPressedStateActor(   make_shared<Actor>(this, ResourceLoader::RID_cross_down_png, true))
        .setBackgroundStateColor(StateColor(
            SDL_Color{0x50,0x50,0x50,0xFF},
            SDL_Color{0x60,0x60,0x60,0xFF},
            SDL_Color{0x40,0x40,0x40,0xFF},
            SDL_Color{0x50,0x50,0x50,0xFF}))
        .setOnClick([this](shared_ptr<Button>) { hide(); })
        .setTransparent(false)
        .build());

    SRect titleRect = {0, 0, m_rect.width - titleH, titleH};
    addControl(m_titleBar = PanelBuilder(this, titleRect)
        .setBGColor(SDL_Color{173, 216, 230, 255})
        .setBorderVisible(false)
        .build());

    m_titleBar->addControl(m_titleLabel = LabelBuilder(m_titleBar.get(),
        SRect(0, 0, titleRect.width, titleRect.height))
        .setFontSize((int)titleH - 4)
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setFont(FontName::HarmonyOS_Sans_SC_Regular)
        .setCaption(m_title)
        .build());

    SRect clientRect = {0, titleH, m_rect.width, m_rect.height - titleH};
    addControl(m_clientPanel = PanelBuilder(this, clientRect)
        .setBGColor(SDL_Color{48, 48, 48, 255})
        .setTransparent(false)
        .setBorderVisible(false)
        .build());
}

WinFrame::~WinFrame() {
    if (m_cursorSizeWE)   { SDL_DestroyCursor(m_cursorSizeWE);   m_cursorSizeWE = nullptr; }
    if (m_cursorSizeNS)   { SDL_DestroyCursor(m_cursorSizeNS);   m_cursorSizeNS = nullptr; }
    if (m_cursorSizeNWSE) { SDL_DestroyCursor(m_cursorSizeNWSE); m_cursorSizeNWSE = nullptr; }
    if (m_cursorSizeNESW) { SDL_DestroyCursor(m_cursorSizeNESW); m_cursorSizeNESW = nullptr; }
}

void WinFrame::bringToFront() {
    Control* p = getParent();
    if (!p) return;
    // Find self in parent's children by raw pointer, reorder to end (topmost)
    // (Avoids shared_from_this which fails with MSVC's multiple enable_shared_from_this bases)
    auto* parentImpl = dynamic_cast<ControlImpl*>(p);
    if (!parentImpl) return;
    auto& children = parentImpl->getChildren();
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == this) {
            auto self = *it;
            children.erase(it);
            children.push_back(self);
            break;
        }
    }
}

SPoint WinFrame::screenToLocal(float screenX, float screenY) {
    SRect drawRect = getDrawRect();
    return {
        (screenX - drawRect.left) / getScaleXX(),
        (screenY - drawRect.top) / getScaleYY()
    };
}

void WinFrame::setResizeCursor(uint8_t flags) {
    SDL_Cursor* cursor = m_cursorDefault;
    switch (flags & 0x0F) {
        case 0:                                     cursor = m_cursorDefault; break;
        case kLeft|kRight:                          cursor = m_cursorSizeWE;  break;
        case kTop|kBottom:                          cursor = m_cursorSizeNS;  break;
        case kLeft:  case kRight:                   cursor = m_cursorSizeWE;  break;
        case kTop:   case kBottom:                  cursor = m_cursorSizeNS;  break;
        case kLeft|kTop:      case kRight|kBottom:  cursor = m_cursorSizeNWSE; break;
        case kRight|kTop:     case kLeft|kBottom:   cursor = m_cursorSizeNESW; break;
        case kLeft|kRight|kTop|kBottom:             cursor = m_cursorSizeWE;  break;
    }
    if (cursor) {
        SDL_SetCursor(cursor);
    }
#ifdef _WIN32
    static HCURSOR hcursors[16] = {NULL};
    int idx = flags & 0x0F;
    if (!hcursors[idx]) {
        switch (idx) {
            case 0:                                     hcursors[idx] = LoadCursorA(NULL, IDC_ARROW); break;
            case kLeft|kRight:                          hcursors[idx] = LoadCursorA(NULL, IDC_SIZEWE); break;
            case kTop|kBottom:                          hcursors[idx] = LoadCursorA(NULL, IDC_SIZENS); break;
            case kLeft:  case kRight:                   hcursors[idx] = LoadCursorA(NULL, IDC_SIZEWE); break;
            case kTop:   case kBottom:                  hcursors[idx] = LoadCursorA(NULL, IDC_SIZENS); break;
            case kLeft|kTop:      case kRight|kBottom:  hcursors[idx] = LoadCursorA(NULL, IDC_SIZENWSE); break;
            case kRight|kTop:     case kLeft|kBottom:   hcursors[idx] = LoadCursorA(NULL, IDC_SIZENESW); break;
            default:                                    hcursors[idx] = LoadCursorA(NULL, IDC_ARROW); break;
        }
    }
    if (hcursors[idx]) {
        SetCursor(hcursors[idx]);
    }
#endif
}

bool WinFrame::handleEvent(shared_ptr<Event> event) {
    if (!getVisible() || !getEnable()) return false;

    SPoint mousePos;
    bool hasPos = false;
    if (EventQueue::isPositionEvent(event->m_eventName)) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            mousePos = *pos;
            hasPos = true;
        } catch (const std::bad_any_cast&) {
            // Not an SPoint event (e.g. MOUSE_WHEEL uses MouseWheelEventData).
            // Leave hasPos=false so position checks are skipped,
            // but the event still propagates to children below.
        }
    }

    // Step 0: Focus-to-front on any MOUSE_LBUTTON_DOWN within WinFrame
    if (hasPos && event->m_eventName == EventName::MOUSE_LBUTTON_DOWN) {
        if (getDrawRect().contains(mousePos.x, mousePos.y)) {
            bringToFront();
        }
    }

    // Step 1: During drag, intercept movement
    if (m_dragging && hasPos) {
        if (event->m_eventName == EventName::MOUSE_MOVING) {
            setResizeCursor(0);
            Control* parent = getParent();
            SRect parentDraw = parent->getDrawRect();
            float parentX = (mousePos.x - parentDraw.left) / parent->getScaleXX();
            float parentY = (mousePos.y - parentDraw.top) / parent->getScaleYY();
            setRect({parentX - m_dragOffset.x, parentY - m_dragOffset.y,
                     m_rect.width, m_rect.height});
            return true;
        }
        if (event->m_eventName == EventName::MOUSE_LBUTTON_UP) {
            m_dragging = false;
            return true;
        }
    }

    // During resize, intercept movement
    if (m_resizing && hasPos) {
        if (event->m_eventName == EventName::MOUSE_MOVING) {
            setResizeCursor(m_resizeFlags);
            SRect newScreenRect = m_startScreenRect;
            float dx = mousePos.x - m_resizeStartMouse.x;
            float dy = mousePos.y - m_resizeStartMouse.y;

            if (m_resizeFlags & kLeft)   { newScreenRect.left += dx; newScreenRect.width -= dx; }
            if (m_resizeFlags & kRight)  { newScreenRect.width += dx; }
            if (m_resizeFlags & kTop)    { newScreenRect.top += dy; newScreenRect.height -= dy; }
            if (m_resizeFlags & kBottom) { newScreenRect.height += dy; }

            Control* parent = getParent();
            SRect parentDraw = parent->getDrawRect();
            float newLocalLeft   = (newScreenRect.left   - parentDraw.left) / parent->getScaleXX();
            float newLocalTop    = (newScreenRect.top    - parentDraw.top)  / parent->getScaleYY();
            float newLocalWidth  = newScreenRect.width  / getScaleXX();
            float newLocalHeight = newScreenRect.height / getScaleYY();

            if (newLocalWidth < MIN_WIDTH) {
                if (m_resizeFlags & kLeft) newLocalLeft = m_startLocalRect.right() - MIN_WIDTH;
                newLocalWidth = MIN_WIDTH;
            }
            if (newLocalHeight < MIN_HEIGHT) {
                if (m_resizeFlags & kTop) newLocalTop = m_startLocalRect.bottom() - MIN_HEIGHT;
                newLocalHeight = MIN_HEIGHT;
            }

            setRect({newLocalLeft, newLocalTop, newLocalWidth, newLocalHeight});
            return true;
        }
        if (event->m_eventName == EventName::MOUSE_LBUTTON_UP) {
            m_resizing = false;
            setResizeCursor(0);
            return true;
        }
    }

    // Step 2: Edge detection (before children for cursor/start-resize)
    if (hasPos && !m_dragging && !m_resizing) {
        SPoint localMouse = screenToLocal(mousePos.x, mousePos.y);

        uint8_t edgeFlags = 0;
        if (localMouse.x >= 0        && localMouse.x - 0             < m_edgeMargin) edgeFlags |= kLeft;
        if (m_rect.width - localMouse.x >= 0 && m_rect.width  - localMouse.x < m_edgeMargin) edgeFlags |= kRight;
        if (localMouse.y >= 0        && localMouse.y - 0             < m_edgeMargin) edgeFlags |= kTop;
        if (m_rect.height - localMouse.y >= 0 && m_rect.height - localMouse.y < m_edgeMargin) edgeFlags |= kBottom;

        if (edgeFlags && m_resizable) {
            if (event->m_eventName == EventName::MOUSE_MOVING) {
                setResizeCursor(edgeFlags);
            }
            if (event->m_eventName == EventName::MOUSE_LBUTTON_DOWN) {
                m_resizing = true;
                m_resizeFlags = edgeFlags;
                m_startScreenRect = getDrawRect();
                m_startLocalRect = m_rect;
                m_resizeStartMouse = mousePos;
                return true;
            }
        } else if (event->m_eventName == EventName::MOUSE_MOVING) {
            setResizeCursor(0);
        }
    }

    // Step 3: Children
    bool consumed = ControlImpl::handleEvent(event);

    // Step 4: Title bar drag (if no child consumed MOUSE_LBUTTON_DOWN)
    if (hasPos && !consumed && !m_dragging && !m_resizing) {
        if (event->m_eventName == EventName::MOUSE_LBUTTON_DOWN) {
            SPoint localMouse = screenToLocal(mousePos.x, mousePos.y);
            float titleH = ConstDef::WINDOW_TITLE_HEIGHT;
            bool onTitleBar = localMouse.y >= 0 && localMouse.y < titleH;
            bool onCloseBtn = localMouse.x >= m_rect.width - titleH && localMouse.x < m_rect.width;

            if (onTitleBar && !onCloseBtn) {
                Control* parent = getParent();
                SRect parentDraw = parent->getDrawRect();
                float parentX = (mousePos.x - parentDraw.left) / parent->getScaleXX();
                float parentY = (mousePos.y - parentDraw.top) / parent->getScaleYY();

                m_dragging = true;
                m_dragOffset = {parentX - m_rect.left, parentY - m_rect.top};
                return true;
            }
        }
    }

    return consumed;
}

void WinFrame::setRect(SRect rect) {
    Panel::setRect(rect);

    float titleH = ConstDef::WINDOW_TITLE_HEIGHT;
    float newTitleWidth = (m_rect.width > titleH) ? (m_rect.width - titleH) : 0.0f;
    float newClientHeight = (m_rect.height > titleH) ? (m_rect.height - titleH) : 0.0f;

    if (m_titleBar) {
        m_titleBar->setRect({0, 0, newTitleWidth, titleH});
    }
    if (m_titleLabel) {
        m_titleLabel->setRect({0, 0, newTitleWidth, titleH});
    }
    if (m_closeButton) {
        m_closeButton->setRect({newTitleWidth, 0, titleH, titleH});
    }
    if (m_clientPanel) {
        m_clientPanel->setRect({0, titleH, m_rect.width, newClientHeight});
    }
}

void WinFrame::show(void) {
    Panel::show();
    bringToFront();
}

void WinFrame::setWinFrameBGColor(const SColor& color) {
    setNormalStateBGColor(color.toSDLColor());
}

void WinFrame::setWinFrameBorderColor(const SColor& color) {
    SDL_Color c = color.toSDLColor();
    setBorderStateColor(StateColor(c, c, c, c));
}

void WinFrame::setTitleBarBGColor(const SColor& color) {
    if (m_titleBar) {
        m_titleBar->setNormalStateBGColor(color.toSDLColor());
    }
}

void WinFrame::setTitleTextColor(const SColor& color) {
    if (m_titleLabel) {
        m_titleLabel->setTextNormalStateColor(color.toSDLColor());
    }
}

void WinFrame::setTitle(const string& title) {
    m_title = title;
    if (m_titleLabel) {
        m_titleLabel->setCaption(title);
    }
}

void WinFrame::addToClient(shared_ptr<Control> control) {
    if (m_clientPanel) {
        m_clientPanel->addControl(control);
    }
}

// ==================== WinFrameBuilder ====================

WinFrameBuilder::WinFrameBuilder(Control* parent, SRect rect, float xScale, float yScale):
    m_winFrame(nullptr)
{
    m_winFrame = make_shared<WinFrame>(parent, rect, xScale, yScale);
}

WinFrameBuilder& WinFrameBuilder::setWinFrameBGColor(const SColor& color) {
    m_winFrame->setWinFrameBGColor(color);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setWinFrameBorderColor(const SColor& color) {
    m_winFrame->setWinFrameBorderColor(color);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setTitleBarBGColor(const SColor& color) {
    m_winFrame->setTitleBarBGColor(color);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setClientBGColor(const SColor& color) {
    if (m_winFrame->m_clientPanel) {
        m_winFrame->m_clientPanel->setNormalStateBGColor(color.toSDLColor());
    }
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setTitle(const string& title) {
    m_winFrame->setTitle(title);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setTitleFont(FontName font) {
    if (m_winFrame->m_titleLabel) {
        m_winFrame->m_titleLabel->setFont(font);
    }
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setTitleFontSize(int size) {
    if (m_winFrame->m_titleLabel) {
        m_winFrame->m_titleLabel->setFontSize(size);
    }
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setTitleTextColor(const SColor& color) {
    m_winFrame->setTitleTextColor(color);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setTitleAlignment(AlignmentMode align) {
    if (m_winFrame->m_titleLabel) {
        m_winFrame->m_titleLabel->setAlignmentMode(align);
    }
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setEdgeMargin(float margin) {
    m_winFrame->setEdgeMargin(margin);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setResizable(bool resizable) {
    m_winFrame->setResizable(resizable);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::addToClient(shared_ptr<Control> control) {
    m_winFrame->addToClient(control);
    return *this;
}

WinFrameBuilder& WinFrameBuilder::setOnClose(WinFrameBuilder::OnClickHandler handler) {
    weak_ptr<WinFrame> wf = m_winFrame;
    m_winFrame->m_closeButton->setOnClick(
        [wf, handler](shared_ptr<Button>) {
            auto sp = wf.lock();
            if (sp) {
                if (handler) handler(sp);
                sp->hide();
            }
        });
    return *this;
}

shared_ptr<WinFrame> WinFrameBuilder::build(void) {
    m_winFrame->create();
    m_winFrame->hide();
    return m_winFrame;
}
