#ifndef HandleControlH
#define HandleControlH

#include "ControlBase.h"
#include "GraphTool.h"
#include "Cursor.h"

class HandleControl : public ControlImpl {
    friend class HandleControlBuilder;
public:
    HandleControl();
    ~HandleControl() override;

    // ── 附加/分离 ──
    void setTarget(shared_ptr<Control> target);
    Control* getTarget() const { return m_target; }
    void detach();

    // ── 配置 ──
    void setHandleSize(float size) { m_handleSize = size; }
    float handleSize() const { return m_handleSize; }

    void setMinSize(float w, float h) { m_minWidth = w; m_minHeight = h; }
    SSize minSize() const { return SSize(m_minWidth, m_minHeight); }

    void setHandleColor(SColor fill, SColor border) { m_handleFill = fill; m_handleBorder = border; }
    SColor handleFillColor() const { return m_handleFill; }
    SColor handleBorderColor() const { return m_handleBorder; }

    void setActiveColor(SColor color) { m_activeFill = color; }
    SColor activeColor() const { return m_activeFill; }

    void setSelectionColor(SColor color) { m_selectionColor = color; }
    SColor selectionColor() const { return m_selectionColor; }

    // ── 手柄可见性 ──
    void setCornerHandlesVisible(bool show) { m_showCornerHandles = show; }
    void setEdgeHandlesVisible(bool show) { m_showEdgeHandles = show; }
    void setMoveHandleVisible(bool show) { m_showMoveHandle = show; }
    void setSelectionBoxVisible(bool show) { m_showSelectionBox = show; }

    // ── Control 接口重写 ──
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override { return false; }

private:
    enum class HandleType : uint8_t {
        None,
        Move,
        NW, N, NE,
        E, SE, S, SW, W
    };

    struct HandleArea {
        SRect rect;
        HandleType type;
    };

    // ── 拖拽状态 ──
    bool m_resizing = false;
    bool m_dragging = false;
    HandleType m_activeHandle = HandleType::None;

    SRect  m_startTargetRect;
    SRect  m_startScreenRect;
    SPoint m_startMousePos;

    // ── 目标引用 ──
    Control*         m_target = nullptr;
    weak_ptr<Control> m_targetWeak;

    // ── 配置 ──
    float  m_handleSize       = 8.0f;
    float  m_minWidth         = 20.0f;
    float  m_minHeight        = 20.0f;
    SColor m_handleFill       = SColor(255, 255, 255, 255);
    SColor m_handleBorder     = SColor(0, 120, 255, 255);
    SColor m_activeFill       = SColor(0, 120, 255, 255);
    SColor m_selectionColor   = SColor(0, 120, 255, 255);

    bool m_showCornerHandles  = true;
    bool m_showEdgeHandles    = true;
    bool m_showMoveHandle     = true;
    bool m_showSelectionBox   = true;

    // ── 光标缓存（持久化，避免频繁创建/销毁）──
    Cursor* m_cursorDefault = nullptr;
    Cursor* m_cursorMove    = nullptr;
    Cursor* m_cursorWE      = nullptr;
    Cursor* m_cursorNS      = nullptr;
    Cursor* m_cursorNWSE    = nullptr;
    Cursor* m_cursorNESW    = nullptr;

    void ensureCursors();
    void cleanupCursors();

    // ── 手柄区域缓存 ──
    HandleArea m_handleAreas[9];
    int m_handleAreaCount = 0;

    // ── 内部方法 ──
    SRect targetToScreen();
    SPoint screenToTargetLocal(float sx, float sy);

    void updateHandleAreas(const SRect& targetScreen);
    HandleType hitTestHandle(float mx, float my);

    void startResize(HandleType type, const SPoint& mousePos);
    void updateResize(const SPoint& mousePos);
    void endResize();

    void startDrag(const SPoint& mousePos);
    void updateDrag(const SPoint& mousePos);
    void endDrag();

    void setResizeCursor(HandleType type);

    void drawHandle(const SRect& rect, HandleType type, bool active);
    void drawSelectionBox(const SRect& targetScreen);
    void drawMoveHandle(const SRect& targetScreen);
};

class HandleControlBuilder {
private:
    shared_ptr<HandleControl> m_handle;
public:
    HandleControlBuilder();
    HandleControlBuilder& setTarget(shared_ptr<Control> target);
    HandleControlBuilder& setHandleSize(float size);
    HandleControlBuilder& setMinSize(float w, float h);
    HandleControlBuilder& setHandleColor(SColor fill, SColor border);
    HandleControlBuilder& setActiveColor(SColor color);
    HandleControlBuilder& setSelectionColor(SColor color);
    HandleControlBuilder& setCornerHandlesVisible(bool show);
    HandleControlBuilder& setEdgeHandlesVisible(bool show);
    HandleControlBuilder& setMoveHandleVisible(bool show);
    HandleControlBuilder& setSelectionBoxVisible(bool show);
    shared_ptr<HandleControl> build();
};

#endif // HandleControlH
