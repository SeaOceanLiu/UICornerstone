#ifndef DialogH
#define DialogH
#include <memory>
#include "Panel.h"
#include "Button.h"
#include "TextArea.h"

class Dialog : public Panel, public std::enable_shared_from_this<Dialog>
{
friend class DialogBuilder;
private:
    string m_title;
    SRect m_clientRect;

    shared_ptr<Panel> m_titleBar;
    shared_ptr<Button> m_okButton;
    shared_ptr<TextArea> m_textArea;
public:
    Dialog(Control* parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

    bool handleEvent(shared_ptr<Event> event) override;

    void onClose(shared_ptr<Button> btn);
    void onOk(shared_ptr<Button> btn);
    void draw(void) override;
    void show(void) override;

    void setTitle(const string& title);
    void setText(const string& text);
    void setOkBtnCaption(const string& caption);

    shared_ptr<TextArea> getTextArea(void) const { return m_textArea; }
};

class DialogBuilder
{
private:
    shared_ptr<Dialog> m_dialog;
public:
    DialogBuilder(Control* parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

    DialogBuilder& setOkBtnCaption(string okCaption);
    DialogBuilder& setTitle(string title);
    DialogBuilder& setText(string text);
    DialogBuilder& setTextFont(FontName fontName);
    DialogBuilder& setTextFontSize(int size);
    DialogBuilder& setDialogBGColor(SDL_Color color);
    DialogBuilder& setTitleBGColor(SDL_Color color);
    shared_ptr<Dialog> build(void);
};
#endif // Dialog.hpp