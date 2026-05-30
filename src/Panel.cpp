#include "Panel.h"

Panel::Panel(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale)
{
    m_rect = rect;
}

void Panel::update(void){
    if (!getEnable()) return;

    ControlImpl::update();
}
void Panel::draw(void){
    if (!getVisible()) return;

    ControlImpl::preDraw();
    ControlImpl::draw();
}

bool Panel::handleEvent(shared_ptr<Event> event){
    return ControlImpl::handleEvent(event);

}

void Panel::addControl(shared_ptr<Control> control){
    ControlImpl::addControl(control);
}

void Panel::reflowChildren() {
    if (!m_layoutEngine) return;
    m_layoutEngine->apply(m_rect, m_children, m_flowItemProps);
}

// *********************************************************************************************
PanelBuilder::PanelBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_panel(nullptr)
{
    m_panel = make_shared<Panel>(parent, rect, xScale, yScale);
}
PanelBuilder& PanelBuilder::setBGColor(SDL_Color color){
    m_panel->setNormalStateBGColor(color);
    return *this;
}
PanelBuilder& PanelBuilder::setBorderColor(SDL_Color color){
    m_panel->setNormalStateBDColor(color);
    return *this;
}
PanelBuilder& PanelBuilder::setTransparent(bool isTransparent){
    m_panel->setTransparent(isTransparent);
    return *this;
}
PanelBuilder& PanelBuilder::setBorderVisible(bool isBorderVisible){
    m_panel->setBorderVisible(isBorderVisible);
    return *this;
}
PanelBuilder& PanelBuilder::addControl(shared_ptr<Control> control){
    m_panel->addControl(control);
    return *this;
}
shared_ptr<Panel> PanelBuilder::build(void){
    m_panel->create();
    return m_panel;
}
