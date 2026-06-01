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

void Panel::removeAllControls() {
    m_flowItemProps.clear();
    m_anchorItemProps.clear();
    m_gridItemProps.clear();
    m_children.clear();
}

void Panel::reflowChildren() {
    if (!m_layoutEngine) return;
    string type = m_layoutEngine->getType();
    if (type == "Grid") {
        m_layoutEngine->applyGrid(m_rect, m_children, m_gridItemProps);
    } else if (type == "Anchor") {
        m_layoutEngine->applyAnchor(m_rect, m_children, m_anchorItemProps);
    } else {
        m_layoutEngine->apply(m_rect, m_children, m_flowItemProps);
    }
}

void Panel::resolveChildPercentages() {
    for (auto& child : m_children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();
        childRect.resolve(m_rect.width, m_rect.height);
        child->setRect(childRect);
    }
}

void Panel::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    resolveChildPercentages();
    if (m_layoutEngine) {
        reflowChildren();
    }
}

void Panel::resized(SRect newRect) {
    ControlImpl::resized(newRect);
    resolveChildPercentages();
    reflowChildren();
}

// *********************************************************************************************
PanelBuilder::PanelBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_panel(nullptr)
{
    m_panel = make_shared<Panel>(parent, rect, xScale, yScale);
}
PanelBuilder& PanelBuilder::setBGColor(SColor color){
    m_panel->setNormalStateBGColor(color);
    return *this;
}
PanelBuilder& PanelBuilder::setBorderColor(SColor color){
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
