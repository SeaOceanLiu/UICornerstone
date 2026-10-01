// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "WinFrame.h"
#include "PropertyNames.h"

WinFrame::WinFrame(Control* parent, SRect rect, float xScale, float yScale):
    Panel(parent, rect, xScale, yScale),
    m_title(PropertyNames::kDefaultWinFrameTitle),
    m_titleBar(nullptr),
    m_titleLabel(nullptr),
    m_closeButton(nullptr),
    m_clientPanel(nullptr),
    m_dragging(false),
    m_resizing(false),
    m_resizeFlags(0),
    m_edgeMargin(4.0f),
    m_resizable(true),
    m_lastEdgeFlags(0),
    m_winFrameBg(SColor(0x30, 0x30, 0x30, 0xFF)),
    m_winFrameBorderColor(SColor(0x60, 0x60, 0x60, 0xFF)),
    m_titleBarBg(SColor(173, 216, 230, 255)),
    m_titleTextColor(SColor(255, 255, 255, 255)),
    m_closedTextColor(SColor(219, 219, 219, 255)),
    m_closeOnClickOutside(false),
    m_closeOnEsc(false),
    m_confirmVisible(false),
    m_closedFontSize(12.0f),
    m_cursorDefault(Cursor::getDefault()),
    m_cursorSizeWE(Cursor::createSystem(SystemCursorType::EW_Resize)),
    m_cursorSizeNS(Cursor::createSystem(SystemCursorType::NS_Resize)),
    m_cursorSizeNWSE(Cursor::createSystem(SystemCursorType::NWSE_Resize)),
    m_cursorSizeNESW(Cursor::createSystem(SystemCursorType::NESW_Resize))
{
    m_ctlType = ControlType::WinFrame;
    if (m_rect.width < MIN_WIDTH)  m_rect.width = MIN_WIDTH;
    if (m_rect.height < MIN_HEIGHT) m_rect.height = MIN_HEIGHT;

    setNormalStateBGColor(SColor(0x30, 0x30, 0x30, 0xFF));
    setBorderVisible(true);
    setBorderStateColor(StateColor(
        SColor(0x60, 0x60, 0x60, 0xFF),
        SColor(0x80, 0x80, 0x80, 0xFF),
        SColor(0x60, 0x60, 0x60, 0xFF),
        SColor(0x60, 0x60, 0x60, 0xFF)));

    float titleH = ConstDef::WINDOW_TITLE_HEIGHT;

    // Create close button with original PNG images (proven working in test_winframe)
    addControl(m_closeButton = ButtonBuilder(this,
        SRect(m_rect.width - titleH, 0, titleH, titleH))
        .setNormalStateActor(    make_shared<Actor>(this, fs::path("assets/images/cross_up.png"), true))
        .setHoverStateActor(     make_shared<Actor>(this, fs::path("assets/images/cross_over.png"), true))
        .setPressedStateActor(   make_shared<Actor>(this, fs::path("assets/images/cross_down.png"), true))
        .setBackgroundStateColor(StateColor(
            SColor(0x50,0x50,0x50,0xFF),
            SColor(0x60,0x60,0x60,0xFF),
            SColor(0x40,0x40,0x40,0xFF),
            SColor(0x50,0x50,0x50,0xFF)))
        .setOnClick([this](shared_ptr<Button>) { hide(); })
        .setTransparent(false)
        .build());
    m_closeButton->setBorderVisible(false);   // P0-41：按钮自身无边框线（灰底保留；窗框边框压过其顶/右缘维持现状）

    SRect titleRect = {0, 0, m_rect.width - titleH, titleH};
    addControl(m_titleBar = PanelBuilder(this, titleRect)
        .setBGColor(SColor(173, 216, 230, 255))
        .setBorderVisible(false)
        .build());

    m_titleBar->addControl(m_titleLabel = LabelBuilder(m_titleBar.get(),
        SRect(0, 0, titleRect.width, titleRect.height))
        .setFontSize((int)titleH - 4)
        .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
        .setFont(FontName::HarmonyOS_Sans_SC_Regular)
        .setCaption(m_title)
        .setEnableExpand(false)   // 裁剪长标题，防止titleLabel的hotRect延伸到WinFrame边缘外
        .setClickable(false)      // 标题栏不可点击，保证MouseDown能穿透到WinFrame的拖拽逻辑
        .build());

    SRect clientRect = {0, titleH, m_rect.width, m_rect.height - titleH};
    addControl(m_clientPanel = PanelBuilder(this, clientRect)
        .setBGColor(SColor(48, 48, 48, 255))
        .setTransparent(false)
        .setBorderVisible(false)
        .build());

    m_isFocusBoundary = true;
}

// 两阶段：构造时 context 可能未就绪（parent=nullptr），焦点边界注册延迟到
// context 就绪后的 create（registerBoundary 幂等，recreate 多次调用安全）
void WinFrame::create() {
    // 保留初始可见性：ControlImpl::create 会无条件 setVisible(true)，
    // 解析期 hide()（初始隐藏的窗口）不得在挂树 recreate 时被覆盖弹出
    bool vis = m_visible;
    ControlImpl::create();
    m_visible = vis;
    FocusManager* fm = m_context ? m_context->focusManager : nullptr;
    if (fm) fm->registerBoundary(this);
}

WinFrame::~WinFrame() {
    if (UIContext::isActive(m_context)) {
        FocusManager* fm = m_context->focusManager;
        if (fm) fm->unregisterBoundary(this);
    }
    delete m_cursorSizeWE;
    delete m_cursorSizeNS;
    delete m_cursorSizeNWSE;
    delete m_cursorSizeNESW;
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

void WinFrame::onFocusScopeActivated() {
    bringToFront();
}

SPoint WinFrame::screenToLocal(float screenX, float screenY) {
    SRect drawRect = getDrawRect();
    return {
        (screenX - drawRect.left) / getScaleXX(),
        (screenY - drawRect.top) / getScaleYY()
    };
}

void WinFrame::setResizeCursor(uint8_t flags) {
    Cursor* cursor = m_cursorDefault;
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
    // Delegate to backend — consistent with HandleControl approach.
    // Each backend's setCurrent is responsible for both the immediate cursor
    // switch and WM_SETCURSOR persistence.
    if (cursor) Cursor::setCurrent(cursor);
}

bool WinFrame::handleEvent(shared_ptr<Event> event) {
    if (!getVisible() || !getEnable()) return false;

    SPoint mousePos;
    bool hasPos = false;
    if (event->m_type == EventType::MouseMove) {
        mousePos = SPoint(event->mousePos.x, event->mousePos.y);
        hasPos = true;
    } else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mousePos = SPoint(event->mouseButton.x, event->mouseButton.y);
        hasPos = true;
    }

    bool consumedByFocus = false;

    // Step 0: Focus-to-front on any MOUSE_LBUTTON_DOWN within WinFrame
    // (must consume the event so sibling WinFrames don't also bringToFront, causing Z-order toggle)
    if (hasPos && event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        if (getDrawRect().contains(mousePos.x, mousePos.y)) {
            bringToFront();
            GET_FOCUSMANAGER->focusFirstInScope(this);
            applyPressState(true);   // P032：按下切态（title 四态可达）
            consumedByFocus = true;
        }
    }
    if (hasPos && event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        applyPressState(false);      // P032：抬起复位
    }

    // Step 1: During drag, intercept movement
    if (m_dragging && hasPos) {
        if (event->m_type == EventType::MouseMove) {
            setResizeCursor(0);
            Control* parent = getParent();
            SRect parentDraw = parent->getDrawRect();
            float parentX = (mousePos.x - parentDraw.left) / parent->getScaleXX();
            float parentY = (mousePos.y - parentDraw.top) / parent->getScaleYY();
            setRect({parentX - m_dragOffset.x, parentY - m_dragOffset.y,
                     m_rect.width, m_rect.height});
            return true;
        }
        if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
            m_dragging = false;
            return true;
        }
    }

    // During resize, intercept movement
    if (m_resizing && hasPos) {
        if (event->m_type == EventType::MouseMove) {
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
        if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
            m_resizing = false;
            // Mouse may still be at the edge after release — keep edge cursor
            if (hasPos && getDrawRect().contains(mousePos.x, mousePos.y) && m_resizable) {
                SPoint localMouse = screenToLocal(mousePos.x, mousePos.y);
                uint8_t edgeFlags = 0;
                if (localMouse.x - 0 < m_edgeMargin) edgeFlags |= kLeft;
                if (m_rect.width - localMouse.x < m_edgeMargin) edgeFlags |= kRight;
                if (localMouse.y - 0 < m_edgeMargin) edgeFlags |= kTop;
                if (m_rect.height - localMouse.y < m_edgeMargin) edgeFlags |= kBottom;
                setResizeCursor(edgeFlags);
                m_lastEdgeFlags = edgeFlags;
            } else {
                setResizeCursor(0);
                m_lastEdgeFlags = 0;
            }
            return true;
        }
    }

    bool bMouseInsideWinFrame = hasPos && getDrawRect().contains(mousePos.x, mousePos.y);

    // Step 2: Edge detection (before children for cursor/start-resize)
    if (hasPos && bMouseInsideWinFrame && !m_dragging && !m_resizing) {
        SPoint localMouse = screenToLocal(mousePos.x, mousePos.y);

        uint8_t edgeFlags = 0;
        if (localMouse.x >= 0        && localMouse.x - 0             < m_edgeMargin) edgeFlags |= kLeft;
        if (m_rect.width - localMouse.x >= 0 && m_rect.width  - localMouse.x < m_edgeMargin) edgeFlags |= kRight;
        if (localMouse.y >= 0        && localMouse.y - 0             < m_edgeMargin) edgeFlags |= kTop;
        if (m_rect.height - localMouse.y >= 0 && m_rect.height - localMouse.y < m_edgeMargin) edgeFlags |= kBottom;

        if (edgeFlags && m_resizable) {
            if (event->m_type == EventType::MouseMove) {
                setResizeCursor(edgeFlags);
            }
            if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
                m_resizing = true;
                m_resizeFlags = edgeFlags;
                setResizeCursor(edgeFlags);
                m_startScreenRect = getDrawRect();
                m_startLocalRect = m_rect;
                m_resizeStartMouse = mousePos;
                return true;
            }
        } else if (event->m_type == EventType::MouseMove) {
            setResizeCursor(0);
        }
        // Capture edgeFlags for cursor re-application after Step 3
        // (children may overwrite the cursor, e.g. Label sets hand cursor on hover)
        if (event->m_type == EventType::MouseMove) {
            m_lastEdgeFlags = edgeFlags;
        }
    }

    // Step 3: Children
    bool consumed = ControlImpl::handleEvent(event);

    // Step 3b: Re-apply edge cursor (children may have overwritten it)
    if (hasPos && bMouseInsideWinFrame && !m_dragging && !m_resizing && event->m_type == EventType::MouseMove) {
        if (m_lastEdgeFlags && m_resizable)
            setResizeCursor(m_lastEdgeFlags);
        else
            setResizeCursor(0);
    }

    // Step 3c: Mouse left WinFrame — restore default cursor
    if (!bMouseInsideWinFrame && event->m_type == EventType::MouseMove) {
        if (m_lastEdgeFlags != 0) {
            setResizeCursor(0);
            m_lastEdgeFlags = 0;
        }
    }

    // Step 4: Title bar drag (if no child consumed MOUSE_LBUTTON_DOWN)
    // Only the label area is draggable — exclude left/right edges (resize margin)
    // and the close button area.
    if (hasPos && !consumed && !m_dragging && !m_resizing) {
        if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
            SPoint localMouse = screenToLocal(mousePos.x, mousePos.y);
            float titleH = ConstDef::WINDOW_TITLE_HEIGHT;
            bool onTitleBar = localMouse.y >= m_edgeMargin &&
                              localMouse.y < titleH &&
                              localMouse.x >= m_edgeMargin &&
                              localMouse.x < m_rect.width - titleH;

            if (onTitleBar) {
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

    // MouseMove: don't consume unless at a resizable edge, so sibling WinFrames
    // (lower in Z-order) can also process the event and set their resize cursor.
    if (event->m_type == EventType::MouseMove) {
        return m_resizable && m_lastEdgeFlags != 0;
    }
    return consumed || consumedByFocus;
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
    m_isFocusBoundary = true;
    FocusManager* fm = m_context ? m_context->focusManager : nullptr;
    if (fm) fm->registerBoundary(this);
}

void WinFrame::hide(void) {
    FocusManager* fm = m_context ? m_context->focusManager : nullptr;
    if (fm) {
        Control* fc = fm->getCurrentFocused();
        if (fc) {
            Control* p = fc->getParent();
            while (p) {
                if (p == this) {
                    fc->setFocused(false, false);
                    break;
                }
                p = p->getParent();
            }
        }
        fm->unregisterBoundary(this);
    }
    Panel::hide();
}

void WinFrame::setWinFrameBGColor(const SColor& color) {
    m_winFrameBg = color;
    setNormalStateBGColor(color);
}

void WinFrame::setWinFrameBorderColor(const SColor& color) {
    m_winFrameBorderColor = color;
    setBorderStateColor(StateColor(color, color, color, color));
}

void WinFrame::setTitleBarBGColor(const SColor& color) {
    m_titleBarBg = color;
    if (m_titleBar) {
        m_titleBar->setNormalStateBGColor(color);
    }
}

void WinFrame::setTitleTextColor(const SColor& color) {
    m_titleTextColor = color;
    if (m_titleLabel) {
        m_titleLabel->setTextNormalStateColor(color);
    }
}

void WinFrame::setState(ControlState state) {
    Panel::setState(state);
    // 状态联动：标题 Label 的 text.*/text-shadow.* 各态色经此可达（Label::draw 按自身 state 取色）
    if (m_titleLabel) m_titleLabel->setState(state);
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

// ── Property system overrides ──

int WinFrame::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kWinFrameBG) == 0)     { setWinFrameBGColor(color);     return 1; }
    if (strcmp(prop, PropertyNames::kWinFrameBorder) == 0) { setWinFrameBorderColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kTitleBarBG) == 0)     { setTitleBarBGColor(color);     return 1; }
    if (strcmp(prop, PropertyNames::kTitleText) == 0)      { setTitleTextColor(color);      return 1; }
    // P032：background 单态键 → ClientPanel；border 单态键 → 整体窗框
    if (strcmp(prop, PropertyNames::kBackground) == 0 && m_clientPanel) { m_clientPanel->setNormalStateBGColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kStateHover) == 0 && m_clientPanel) { m_clientPanel->setHoverStateBGColor(color);  return 1; }
    // P0-49：per-state 单态键 1:1 转发 ClientPanel（补齐 pressed/disabled，写读一致）
    if (strcmp(prop, PropertyNames::kStatePressed) == 0 && m_clientPanel)  { m_clientPanel->setPressedStateBGColor(color);  return 1; }
    if (strcmp(prop, PropertyNames::kStateDisabled) == 0 && m_clientPanel) { m_clientPanel->setDisabledStateBGColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kBorder) == 0)      { ControlImpl::setNormalStateBDColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kBorderHover) == 0) { ControlImpl::setHoverStateBDColor(color);  return 1; }
    // #13：标题文本/阴影四态单色转发（对照 setTitleTextColor 同步模式；normal 由 kTitleText 承载）
    if (m_titleLabel) {
        if (strcmp(prop, PropertyNames::kTextShadow) == 0)         { m_titleLabel->setTextShadowNormalStateColor(color);   return 1; }
        if (strcmp(prop, PropertyNames::kTextShadowHover) == 0)    { m_titleLabel->setTextShadowHoverStateColor(color);    return 1; }
        if (strcmp(prop, PropertyNames::kTextShadowPressed) == 0)  { m_titleLabel->setTextShadowPressedStateColor(color);  return 1; }
        if (strcmp(prop, PropertyNames::kTextShadowDisabled) == 0) { m_titleLabel->setTextShadowDisabledStateColor(color); return 1; }
        if (strcmp(prop, PropertyNames::kTextHover) == 0)          { m_titleLabel->setTextHoverStateColor(color);          return 1; }
        if (strcmp(prop, PropertyNames::kTextPressed) == 0)        { m_titleLabel->setTextPressedStateColor(color);        return 1; }
        if (strcmp(prop, PropertyNames::kTextDisabled) == 0)       { m_titleLabel->setTextDisabledStateColor(color);       return 1; }
    }
    return Panel::setColorProperty(prop, color);
}

// P032：background 两态 → ClientPanel（内容区底色；标题栏装饰保持专用键）
void WinFrame::setBackgroundStateColor(StateColor stateColor) {
    if (!m_clientPanel) return;
    StateColor cp;
    cp.setNormal(stateColor.getNormal());
    cp.setHover(stateColor.getHover());
    cp.setPressed(stateColor.getNormal());     // 仅两态声明
    cp.setDisabled(stateColor.getNormal());
    m_clientPanel->setBackgroundStateColor(cp);
}
StateColor WinFrame::getBackgroundStateColor(void) {
    if (!m_clientPanel) return ControlImpl::getBackgroundStateColor();
    return m_clientPanel->getBackgroundStateColor();   // P0-49：返回实际四态（取消 pressed/disabled→hover 折叠，写读一致）
}

// P032：border 两态 → 整体窗框（基类存储 + border-visible；hover 由 WinFrame 状态驱动）
void WinFrame::setBorderStateColor(StateColor stateColor) {
    ControlImpl::setBorderStateColor(stateColor);
}

int WinFrame::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {   // P032：标题字体名
        m_fontName = FontNameFromString(value);
        if (m_titleLabel) m_titleLabel->setFont(m_fontName);
        return 1;
    }
    return Panel::setEnumProperty(prop, value);
}
int WinFrame::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_fontName); return 1; }
    return Panel::getEnumProperty(prop, out);
}
int WinFrame::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) {   // P032：标题字号
        m_titleFontSize = value;
        if (m_titleLabel) m_titleLabel->setFontSize(value);
        return 1;
    }
    return Panel::setIntProperty(prop, value);
}
int WinFrame::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { out = m_titleFontSize; return 1; }
    return Panel::getIntProperty(prop, out);
}

int WinFrame::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kResizable) == 0) { setResizable(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // #12：标题阴影开关转发内部 title Label
        if (m_titleLabel) { m_titleLabel->setShadow(value != 0); return 1; }
        return 0;
    }
    return Panel::setBoolProperty(prop, value);
}

int WinFrame::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kEdgeMargin) == 0) { setEdgeMargin(value); return 1; }
    // #12：标题阴影偏移转发内部 title Label
    if (m_titleLabel != nullptr) {
        if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) {
            SPoint o = m_titleLabel->getShadowOffset(); o.x = value; m_titleLabel->setShadowOffset(o); return 1;
        }
        if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
            SPoint o = m_titleLabel->getShadowOffset(); o.y = value; m_titleLabel->setShadowOffset(o); return 1;
        }
    }
    return Panel::setFloatProperty(prop, value);
}

int WinFrame::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kTitle) == 0) { setTitle(value); return 1; }
    return Panel::setStringProperty(prop, value);
}

int WinFrame::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kWinFrameBG) == 0)     { out = m_winFrameBg;         return 1; }
    if (strcmp(prop, PropertyNames::kWinFrameBorder) == 0) { out = m_winFrameBorderColor; return 1; }
    if (strcmp(prop, PropertyNames::kTitleBarBG) == 0)     { out = m_titleBarBg;          return 1; }
    if (strcmp(prop, PropertyNames::kTitleText) == 0)      { out = m_titleTextColor;      return 1; }
    if (strcmp(prop, PropertyNames::kBackground) == 0 && m_clientPanel) { out = m_clientPanel->getBackgroundStateColor().getNormal(); return 1; }
    if (strcmp(prop, PropertyNames::kStateHover) == 0 && m_clientPanel) { out = m_clientPanel->getBackgroundStateColor().getHover();  return 1; }
    if (strcmp(prop, PropertyNames::kBorder) == 0)      { out = m_borderColor.getNormal(); return 1; }
    if (strcmp(prop, PropertyNames::kClosedText) == 0)     { out = m_closedTextColor;     return 1; }
    // #13：标题文本/阴影四态单色读回（内部 title Label）
    if (m_titleLabel) {
        if (strcmp(prop, PropertyNames::kTextShadow) == 0)         { out = m_titleLabel->getTextShadowStateColor().getNormal();   return 1; }
        if (strcmp(prop, PropertyNames::kTextShadowHover) == 0)    { out = m_titleLabel->getTextShadowStateColor().getHover();    return 1; }
        if (strcmp(prop, PropertyNames::kTextShadowPressed) == 0)  { out = m_titleLabel->getTextShadowStateColor().getPressed();  return 1; }
        if (strcmp(prop, PropertyNames::kTextShadowDisabled) == 0) { out = m_titleLabel->getTextShadowStateColor().getDisabled(); return 1; }
        if (strcmp(prop, PropertyNames::kTextHover) == 0)          { out = m_titleLabel->getTextStateColor().getHover();          return 1; }
        if (strcmp(prop, PropertyNames::kTextPressed) == 0)        { out = m_titleLabel->getTextStateColor().getPressed();        return 1; }
        if (strcmp(prop, PropertyNames::kTextDisabled) == 0)       { out = m_titleLabel->getTextStateColor().getDisabled();       return 1; }
    }
    return Panel::getColorProperty(prop, out);
}

int WinFrame::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kCloseOnClickOutside) == 0) { out = m_closeOnClickOutside ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kCloseOnEsc) == 0)          { out = m_closeOnEsc          ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kResizable) == 0)           { out = m_resizable           ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kConfirmVisible) == 0)      { out = m_confirmVisible      ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // #12：读回内部 title Label
        if (m_titleLabel) { out = m_titleLabel->isShadowEnabled() ? 1 : 0; return 1; }
        return 0;
    }
    return Panel::getBoolProperty(prop, out);
}

int WinFrame::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kClosedFontSize) == 0) { out = m_closedFontSize; return 1; }
    if (m_titleLabel != nullptr) {
        if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_titleLabel->getShadowOffset().x; return 1; }
        if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_titleLabel->getShadowOffset().y; return 1; }
    }
    return Panel::getFloatProperty(prop, out);
}

int WinFrame::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kTitle) == 0) { out = m_title.c_str(); return 1; }
    return Panel::getStringProperty(prop, out);
}

int WinFrame::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kTitleBar) == 0)    { out = m_titleBar ? static_cast<Control*>(m_titleBar.get()) : nullptr;    return 1; }
    if (strcmp(prop, PropertyNames::kTitleLabel) == 0)  { out = m_titleLabel ? static_cast<Control*>(m_titleLabel.get()) : nullptr;  return 1; }
    if (strcmp(prop, PropertyNames::kCloseButton) == 0) { out = m_closeButton ? static_cast<Control*>(m_closeButton.get()) : nullptr; return 1; }
    if (strcmp(prop, PropertyNames::kClientPanel) == 0) { out = m_clientPanel ? static_cast<Control*>(m_clientPanel.get()) : nullptr; return 1; }
    return Panel::getPtrProperty(prop, out);
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
        m_winFrame->m_clientPanel->setNormalStateBGColor(color);
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
