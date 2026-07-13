#ifndef DialogH
#define DialogH

#include <functional>
#include <string>
#include "MainWindow.h"
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Panel.h"
#include "Button.h"
#include "EventQueue.h"

using namespace std;

enum class DialogResult {
    None,
    Confirmed,
    Cancelled
};

// ==================== Popup (no buttons) ====================

class Popup : public Panel {
public:
    using OnCloseHandler = std::function<void(shared_ptr<Popup>, DialogResult)>;

protected:
    shared_ptr<ControlImpl> m_content;
    bool m_closeOnClickOutside = true;
    bool m_closeOnEsc = true;
    bool m_watcherRegistered = false;
    DialogResult m_result = DialogResult::None;
    float m_padding = 10.0f;
    OnCloseHandler m_onClose;

    enum class AnchorMode { Absolute, Centered, Anchored };
    AnchorMode m_anchorMode = AnchorMode::Centered;
    Control* m_anchorControl = nullptr;
    SRect m_anchorOffset;

    SRect computeTargetRect();

protected:
    virtual void layoutContent();
    virtual void onEscPressed();
    virtual void onOutsideClicked();
    void registerWatcher();
    void focusFirstContent();

public:
    Popup(Control* parent, SRect rect,
          float xScale = 1.0f, float yScale = 1.0f);
    ~Popup();
    void create() override;

    virtual void open();
    virtual void close(DialogResult result = DialogResult::Cancelled);
    bool isPopupVisible() { return getVisible() && m_result == DialogResult::None; }
    DialogResult getResult() const { return m_result; }

    void setContent(shared_ptr<ControlImpl> content);
    shared_ptr<ControlImpl> getContent() const { return m_content; }

    void setCentered();
    void setAnchored(Control* anchor, const SRect& offset = {0, 0, 0, 0});
    void setAbsolute(const SRect& rect);

    void setCloseOnClickOutside(bool v) { m_closeOnClickOutside = v; }
    void setCloseOnEsc(bool v) { m_closeOnEsc = v; }

    void setOnClose(OnCloseHandler handler) { m_onClose = handler; }

    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
};

// ==================== ConfirmPopup (OK button) ====================

class ConfirmPopup : public Popup {
public:
    using OnConfirmHandler = std::function<void(shared_ptr<ConfirmPopup>)>;

protected:
    shared_ptr<Button> m_btnConfirm;
    string m_btnConfirmText = u8"确定";
    SRect m_btnConfirmRect;
    bool m_showConfirmButton = true;
    float m_buttonHeight = 28.0f;
    float m_buttonGap = 8.0f;
    bool m_ignoreKeyEvent = false;

    OnConfirmHandler m_onConfirm;

protected:
    virtual void createConfirmButton();
    virtual void layoutContent();
    void onEscPressed() override;
    virtual void onConfirmAction();

public:
    ConfirmPopup(Control* parent, SRect rect,
                 float xScale = 1.0f, float yScale = 1.0f);
    void create() override;
    void open() override;

    void setConfirmButtonVisible(bool v) { m_showConfirmButton = v; }
    void setConfirmButtonText(const string& text) { m_btnConfirmText = text; if (m_btnConfirm) m_btnConfirm->setCaption(text); }
    void setConfirmButtonRect(SRect rect) { m_btnConfirmRect = rect; }
    shared_ptr<Button> getConfirmButton() { return m_btnConfirm; }
    void setButtonHeight(float h) { m_buttonHeight = h; }
    void setButtonGap(float gap) { m_buttonGap = gap; }
    void setPadding(float pad) { m_padding = pad; }

    void setOnConfirm(OnConfirmHandler handler) { m_onConfirm = handler; }

    bool handleEvent(shared_ptr<Event> event) override;
};

// ==================== Dialog (OK + Cancel) ====================

class Dialog : public ConfirmPopup {
public:
    using OnCancelHandler = std::function<void(shared_ptr<Dialog>)>;

private:
    shared_ptr<Button> m_btnCancel;
    string m_btnCancelText = u8"取消";
    SRect m_btnCancelRect;

    OnCancelHandler m_onCancel;

protected:
    virtual void createCancelButton();
    void layoutContent() override;
    void onEscPressed() override;
    virtual void onCancelAction();

public:
    Dialog(Control* parent, SRect rect,
           float xScale = 1.0f, float yScale = 1.0f);
    void create() override;
    void open() override;

    void setCancelButtonText(const string& text) { m_btnCancelText = text; if (m_btnCancel) m_btnCancel->setCaption(text); }
    void setCancelButtonRect(SRect rect) { m_btnCancelRect = rect; }
    shared_ptr<Button> getCancelButton() { return m_btnCancel; }

    void recreateButtons();
    void setOnCancel(OnCancelHandler handler) { m_onCancel = handler; }

    bool handleEvent(shared_ptr<Event> event) override;
};

// ==================== Builder base ====================

class PopupBuilder {
protected:
    shared_ptr<Popup> m_popup;
public:
    PopupBuilder(Control* parent, SRect rect,
                 float xScale = 1.0f, float yScale = 1.0f);

    PopupBuilder& setContent(shared_ptr<ControlImpl> content);
    PopupBuilder& setCentered();
    PopupBuilder& setOnClose(Popup::OnCloseHandler handler);
    PopupBuilder& setCloseOnClickOutside(bool v);
    PopupBuilder& setCloseOnEsc(bool v);
    PopupBuilder& setBackgroundStateColor(StateColor stateColor);
    PopupBuilder& setBorderStateColor(StateColor stateColor);

    shared_ptr<Popup> build();
};

class ConfirmPopupBuilder : public PopupBuilder {
public:
    ConfirmPopupBuilder(Control* parent, SRect rect,
                        float xScale = 1.0f, float yScale = 1.0f);

    ConfirmPopupBuilder& setConfirmButtonText(const string& text);
    ConfirmPopupBuilder& setConfirmButtonRect(SRect rect);
    ConfirmPopupBuilder& setButtonHeight(float h);
    ConfirmPopupBuilder& setButtonGap(float gap);
    ConfirmPopupBuilder& setPadding(float pad);
    ConfirmPopupBuilder& setOnConfirm(ConfirmPopup::OnConfirmHandler handler);
    ConfirmPopupBuilder& setBackgroundStateColor(StateColor stateColor);
    ConfirmPopupBuilder& setBorderStateColor(StateColor stateColor);

    shared_ptr<ConfirmPopup> build();
};

class DialogBuilder : public ConfirmPopupBuilder {
public:
    DialogBuilder(Control* parent, SRect rect,
                  float xScale = 1.0f, float yScale = 1.0f);

    DialogBuilder& setCancelButtonText(const string& text);
    DialogBuilder& setCancelButtonRect(SRect rect);
    DialogBuilder& setOnCancel(Dialog::OnCancelHandler handler);
    DialogBuilder& setOnConfirm(ConfirmPopup::OnConfirmHandler handler);
    DialogBuilder& setBackgroundStateColor(StateColor stateColor);
    DialogBuilder& setBorderStateColor(StateColor stateColor);

    shared_ptr<Dialog> build();
};

#endif
