#ifndef SplitterH
#define SplitterH

#include <functional>
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Cursor.h"

class Splitter : public ControlImpl {
    friend class SplitterBuilder;
public:
    using OnSplitterMovedHandler = std::function<void(shared_ptr<Splitter>, float ratio)>;

private:
    bool m_orientation;
    weak_ptr<Control> m_firstWeak;
    weak_ptr<Control> m_secondWeak;
    Control* m_first;
    Control* m_second;

    float m_thickness;
    float m_minFirst;
    float m_minSecond;

    float m_splitRatio;

    bool   m_dragging;
    bool   m_dragWatcherRegistered;
    float m_dragStartRatio;
    SPoint m_dragStartMousePos;
    float m_dragStartScreenPos;
    float m_dragStartLocalPos;
    float m_dragStartSegW   = -1.0f;   // 引擎模式拖拽起始前段宽（-1=未初始化）
    float m_dragStartRearW  = -1.0f;   // 引擎模式拖拽起始后段宽    // 鎷栨嫿璧峰鏃?Splitter 鐨勫眬閮ㄥ潗鏍囦綅缃?   // 鎷栨嫿璧峰鏃?Splitter 鐨勫睆骞曚綅缃紙x 鎴?y锛?
    uint64_t m_lastClickTime;

    SColor m_colorNormal;
    SColor m_colorHover;
    SColor m_colorDrag;
    SColor m_colorDisabled;              // P0-42：colors.background.disabled -> 把手线色（缺省=normal）
    bool   m_lineNormalSet;              // P0-42：line 专用键优先标记（显式专用 > 通用组）
    bool   m_lineHoverSet;
    bool   m_hovered;

    Cursor* m_cursorResize;
    Cursor* m_cursorDefault;

    SRect  m_lastRect;

    OnSplitterMovedHandler m_onSplitterMoved;

public:
    Splitter(Control* parent, const SRect& rect,
             float xScale = 1.0f, float yScale = 1.0f);
    ~Splitter() override;

    void create() override;
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void  setOrientation(bool horizontal);
    bool  isHorizontal() const { return m_orientation; }

    void  setLinkedControls(shared_ptr<Control> first, shared_ptr<Control> second);
    void  setFirstControl(shared_ptr<Control> first);
    void  setSecondControl(shared_ptr<Control> second);
    Control* getFirstControl() const { return m_first; }
    Control* getSecondControl() const { return m_second; }
    void  clearLinkedControls();

    void  setMinSize(float firstMin, float secondMin);
    float getMinFirst() const { return m_minFirst; }
    float getMinSecond() const { return m_minSecond; }

    void  setThickness(float px);
    float getThickness() const { return m_thickness; }

    void  setSplitRatio(float ratio);
    float getSplitRatio() const { return m_splitRatio; }

    void  setColor(SColor normal, SColor hover, SColor drag);
    SColor getColorNormal() const { return m_colorNormal; }
    SColor getColorHover() const { return m_colorHover; }
    SColor getColorDrag() const { return m_colorDrag; }

    void setOnSplitterMoved(OnSplitterMovedHandler handler);

    // ── Property system overrides ──
    void setBackgroundStateColor(StateColor stateColor) override;   // P0-42：通用组 -> 把手线色
    StateColor getBackgroundStateColor(void) override;              // P0-42：读回（有效线色）
    int setColorProperty(const char* prop, SColor color) override;
    int setBoolProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setPtrProperty(const char* prop, void* value) override;
    int getColorProperty(const char* prop, SColor& out) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getPtrProperty(const char* prop, void*& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;

private:
    bool ensureControls();
    void startDrag(const SPoint& mousePos);
    void updateDrag(const SPoint& mousePos);
    void endDrag();
    void handleKeyEvent(shared_ptr<Event> event);
    void applySplitRatio(float ratio);
    void handleDoubleClick();
    // 布局引擎容器内的 Splitter：拖拽改为"改前段弹性权重 + 引擎重排"（见 Splitter_Design §8.3/8.4）
    bool isEngineManaged();
    void updateEngineDrag(float delta);
    bool m_applyingRatio = false;   // applySplitRatio 重入保护（引擎 reflow 写回时）
    void ensureCursors();
    void cleanupCursors();
    void updateCursor(bool inside);
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
};

class SplitterBuilder {
private:
    shared_ptr<Splitter> m_splitter;
public:
    SplitterBuilder(Control* parent, const SRect& rect,
                    float xScale = 1.0f, float yScale = 1.0f);

    SplitterBuilder& setOrientation(bool horizontal);
    SplitterBuilder& setLinkedControls(shared_ptr<Control> first, shared_ptr<Control> second);
    SplitterBuilder& setMinSize(float firstMin, float secondMin);
    SplitterBuilder& setThickness(float px);
    SplitterBuilder& setSplitRatio(float ratio);
    SplitterBuilder& setColor(SColor normal, SColor hover, SColor drag);
    SplitterBuilder& setOnSplitterMoved(Splitter::OnSplitterMovedHandler handler);
    SplitterBuilder& setBackgroundStateColor(StateColor sc);
    SplitterBuilder& setBorderStateColor(StateColor sc);
    SplitterBuilder& setId(int id);

    shared_ptr<Splitter> build();
};

#endif
