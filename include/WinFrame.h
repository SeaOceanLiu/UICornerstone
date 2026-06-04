// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#ifndef WinFrameH
#define WinFrameH
#include <memory>
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "GraphTool.h"
#include "ConstDef.h"
#include "Cursor.h"

using GraphTool::SColor;

class WinFrame : public Panel
{
    friend class WinFrameBuilder;

    static constexpr uint8_t kLeft   = 0x01;
    static constexpr uint8_t kRight  = 0x02;
    static constexpr uint8_t kTop    = 0x04;
    static constexpr uint8_t kBottom = 0x08;
    static constexpr float MIN_WIDTH   = 100.0f;
    static constexpr float MIN_HEIGHT  = 60.0f;

private:
    shared_ptr<Panel>  m_titleBar;
    shared_ptr<Label>  m_titleLabel;
    shared_ptr<Button> m_closeButton;
    shared_ptr<Panel>  m_clientPanel;
    string m_title;

    bool    m_dragging;
    SPoint  m_dragOffset;

    bool    m_resizing;
    uint8_t m_resizeFlags;
    SRect   m_startScreenRect;
    SRect   m_startLocalRect;
    SPoint  m_resizeStartMouse;

    float   m_edgeMargin;
    bool    m_resizable;
    uint8_t m_lastEdgeFlags;

    Cursor *m_cursorDefault;
    Cursor *m_cursorSizeWE;
    Cursor *m_cursorSizeNS;
    Cursor *m_cursorSizeNWSE;
    Cursor *m_cursorSizeNESW;

    void bringToFront();
    SPoint screenToLocal(float screenX, float screenY);
    void setResizeCursor(uint8_t flags);

public:
    WinFrame(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    ~WinFrame();

    bool handleEvent(shared_ptr<Event> event) override;
    void show(void) override;
    void setRect(SRect rect) override;

    shared_ptr<Panel>  getTitleBar()    const { return m_titleBar; }
    shared_ptr<Label>  getTitleLabel()  const { return m_titleLabel; }
    shared_ptr<Button> getCloseButton() const { return m_closeButton; }
    shared_ptr<Panel>  getClientPanel() const { return m_clientPanel; }

    void setWinFrameBGColor(const SColor& color);
    void setWinFrameBorderColor(const SColor& color);
    void setTitleBarBGColor(const SColor& color);
    void setTitleTextColor(const SColor& color);

    void setEdgeMargin(float margin) { m_edgeMargin = margin; }
    float getEdgeMargin() const { return m_edgeMargin; }

    void setTitle(const string& title);
    string getTitle() const { return m_title; }
    void addToClient(shared_ptr<Control> control);

    void setResizable(bool resizable) { m_resizable = resizable; }
    bool isResizable() const { return m_resizable; }
};

class WinFrameBuilder
{
private:
    shared_ptr<WinFrame> m_winFrame;
public:
    WinFrameBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);

    WinFrameBuilder& setWinFrameBGColor(const SColor& color);
    WinFrameBuilder& setWinFrameBorderColor(const SColor& color);
    WinFrameBuilder& setTitleBarBGColor(const SColor& color);
    WinFrameBuilder& setClientBGColor(const SColor& color);

    WinFrameBuilder& setTitle(const string& title);
    WinFrameBuilder& setTitleFont(FontName font);
    WinFrameBuilder& setTitleFontSize(int size);
    WinFrameBuilder& setTitleTextColor(const SColor& color);
    WinFrameBuilder& setTitleAlignment(AlignmentMode align);

    WinFrameBuilder& setEdgeMargin(float margin);
    WinFrameBuilder& setResizable(bool resizable);

    WinFrameBuilder& addToClient(shared_ptr<Control> control);

    using OnClickHandler = std::function<void(shared_ptr<Control>)>;
    WinFrameBuilder& setOnClose(OnClickHandler handler);

    shared_ptr<WinFrame> build(void);
};
#endif // WinFrameH
