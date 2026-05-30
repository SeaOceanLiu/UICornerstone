#include "Dialog.h"

Dialog::Dialog(Control* parent, SRect rect, float xScale, float yScale):
    Panel(parent, rect, xScale, yScale),
    m_title("Dialog"),
    m_titleBar(nullptr),
    m_okButton(nullptr),
    m_textArea(nullptr)
{
    if(m_rect.width < 200) m_rect.width = 200;
    if(m_rect.height < 100) m_rect.height = 100;

    setNormalStateBGColor({0x30, 0x30, 0x30, 0xFF});
    setBorderVisible(true);
    setBorderStateColor(StateColor({0x60, 0x60, 0x60, 0xFF}, {0x80, 0x80, 0x80, 0xFF}, {0x60, 0x60, 0x60, 0xFF}, {0x60, 0x60, 0x60, 0xFF}));

    addControl(ButtonBuilder(this, SRect(m_rect.width - ConstDef::WINDOW_TITLE_HEIGHT, 0, ConstDef::WINDOW_TITLE_HEIGHT, ConstDef::WINDOW_TITLE_HEIGHT))
                .setNormalStateActor(    make_shared<Actor>(this, ResourceLoader::RID_cross_up_png, true))
                .setHoverStateActor(     make_shared<Actor>(this, ResourceLoader::RID_cross_over_png, true))
                .setPressedStateActor(   make_shared<Actor>(this, ResourceLoader::RID_cross_down_png, true))
                .setBackgroundStateColor(StateColor({0x50, 0x50, 0x50, 0xFF}, {0x60, 0x60, 0x60, 0xFF}, {0x40, 0x40, 0x40, 0xFF}, {0x50, 0x50, 0x50, 0xFF}))
                .setOnClick(std::bind(&Dialog::onClose, this, std::placeholders::_1))
                .setTransparent(false)
                .build());

    SRect titleRect = {0, 0, m_rect.width - ConstDef::WINDOW_TITLE_HEIGHT, ConstDef::WINDOW_TITLE_HEIGHT};
    addControl(m_titleBar = PanelBuilder(this, titleRect)
                .addControl(LabelBuilder(this, titleRect)
                                .setFontSize(30)
                                .setAlignmentMode(AlignmentMode::AM_CENTER)
                                .setFont(FontName::HarmonyOS_Sans_SC_Regular)
                                .setCaption(m_title)
                                .build())
                .build());

    SSize btnSize(50, 15);
    SRect okRect(m_rect.width - btnSize.width - ConstDef::FONT_MARGIN, m_rect.height - btnSize.height - ConstDef::FONT_MARGIN, btnSize.width, btnSize.height);
    addControl(m_okButton = ButtonBuilder(this, okRect)
                .setCaption("OK")
                .setCaptionSize(8)
                .setOnClick(std::bind(&Dialog::onOk, this, std::placeholders::_1))
                .build());

    m_clientRect = {ConstDef::FONT_MARGIN,
                        titleRect.height + ConstDef::FONT_MARGIN,
                        m_rect.width - ConstDef::FONT_MARGIN * 2,
                        okRect.top - titleRect.height - ConstDef::FONT_MARGIN * 2};

    addControl(m_textArea = TextAreaBuilder(this, m_clientRect)
                    .setWordWrap(true)
                    .setFont(FontName::MapleMono_NF_CN_Regular)
                    .setFontSize(14)
                    .build());
}

void Dialog::draw(void){
    if (getVisible()){
        Panel::draw();
    }
}

bool Dialog::handleEvent(shared_ptr<Event> event){
    if(!getVisible() || !getEnable()) return false;

    ControlImpl::handleEvent(event);

    return true;
}

void Dialog::onClose(shared_ptr<Button> btn) {
    hide();
}
void Dialog::onOk(shared_ptr<Button> btn){
    hide();
}

void Dialog::show(void){
    Panel::show();
}

void Dialog::setTitle(const string& title) {
    m_title = title;
    if (m_titleBar) {
        removeControl(m_titleBar);
    }
    addControl(m_titleBar = PanelBuilder(this, SRect(0, 0, m_rect.width - ConstDef::WINDOW_TITLE_HEIGHT, ConstDef::WINDOW_TITLE_HEIGHT))
                                    .addControl(LabelBuilder(m_titleBar.get(), SRect(0, 0, m_rect.width - ConstDef::WINDOW_TITLE_HEIGHT, ConstDef::WINDOW_TITLE_HEIGHT))
                                                                    .setFontSize((int)ConstDef::WINDOW_TITLE_HEIGHT - 4)
                                                                    .setAlignmentMode(AlignmentMode::AM_CENTER)
                                                                    .setFont(FontName::HarmonyOS_Sans_SC_Regular)
                                                                    .setCaption(m_title)
                                                                    .build())
                                    .setBGColor({173, 216, 230, SDL_ALPHA_OPAQUE})
                                    .setBorderVisible(false)
                                    .build());
}

void Dialog::setText(const string& text) {
    if (m_textArea) {
        m_textArea->setText(text);
    }
}

void Dialog::setOkBtnCaption(const string& caption) {
    if (m_okButton) {
        m_okButton->setCaption(caption);
    }
}


DialogBuilder::DialogBuilder(Control* parent, SRect rect, float xScale, float yScale):
    m_dialog(nullptr)
{
    m_dialog = make_shared<Dialog>(parent, rect, xScale, yScale);
}
DialogBuilder& DialogBuilder::setOkBtnCaption(string okCaption)
{
    m_dialog->m_okButton->setCaption(okCaption);
    return *this;
}

DialogBuilder& DialogBuilder::setTitle(string title){
    m_dialog->m_title = title;

    if (m_dialog->m_titleBar){
        m_dialog->removeControl(m_dialog->m_titleBar);
    }
    m_dialog->addControl(m_dialog->m_titleBar = PanelBuilder(m_dialog.get(), SRect(0, 0, m_dialog->m_rect.width - ConstDef::WINDOW_TITLE_HEIGHT, ConstDef::WINDOW_TITLE_HEIGHT))
                                                    .addControl(LabelBuilder(m_dialog->m_titleBar.get(), SRect(0, 0, m_dialog->m_rect.width - ConstDef::WINDOW_TITLE_HEIGHT, ConstDef::WINDOW_TITLE_HEIGHT))
                                                                    .setFontSize((int)ConstDef::WINDOW_TITLE_HEIGHT - 4)
                                                                    .setAlignmentMode(AlignmentMode::AM_CENTER)
                                                                    .setFont(FontName::HarmonyOS_Sans_SC_Regular)
                                                                    .setCaption(m_dialog->m_title)
                                                                    .build())
                                                    .setBGColor({173, 216, 230, SDL_ALPHA_OPAQUE})
                                                    .setBorderVisible(false)
                                                    .build());
    return *this;
}
DialogBuilder& DialogBuilder::setText(string text){
    m_dialog->m_textArea->setText(text);
    return *this;
}
DialogBuilder& DialogBuilder::setTextFont(FontName fontName){
    m_dialog->m_textArea->setFont(fontName);
    return *this;
}
DialogBuilder& DialogBuilder::setTextFontSize(int size){
    m_dialog->m_textArea->setFontSize(size);
    return *this;
}
DialogBuilder& DialogBuilder::setDialogBGColor(SDL_Color color){
    m_dialog->setNormalStateBGColor(color);
    return *this;
}
DialogBuilder& DialogBuilder::setTitleBGColor(SDL_Color color){
    if (m_dialog->m_titleBar){
        m_dialog->m_titleBar->setNormalStateBGColor(color);
    }
    return *this;
}

shared_ptr<Dialog> DialogBuilder::build(void){
    m_dialog->create();
    m_dialog->hide();
    return m_dialog;
}
