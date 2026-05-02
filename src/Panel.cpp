#include "Panel.h"

Panel::Panel(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale)
{
    m_rect = rect;
}

void Panel::update(void){
    if (!getEnable()) return;

    // 如果有子控件，这里需要更新子控件
    ControlImpl::update();
}
void Panel::draw(void){
    ControlImpl::preDraw();

    if (!getVisible()) return;

    // SRect drawRect = getDrawRect();

    // // 1. 先绘制当前控件的外观
    // if(!m_isTransparent) {
    //     if(!SDL_SetRenderDrawColor(getRenderer(), m_bgColor.getNormal().r, m_bgColor.getNormal().g,
    //                                 m_bgColor.getNormal().b, m_bgColor.getNormal().a)){
    //         SDL_Log("Panel failed to set background color: %s", SDL_GetError());
    //     }
    //     if (!SDL_RenderFillRect(getRenderer(), drawRect.toSDLFRect())){
    //         SDL_Log("Panel failed to fill render rect: %s", SDL_GetError());
    //     }
    // }

    // 2. 接着绘制子控件
    ControlImpl::draw();

    // // 5. 最后绘制边框
    // if(!getTransparent() && getBorderVisible()) {
    //     if(!SDL_SetRenderDrawColor(getRenderer(), m_borderColor.getNormal().r, m_borderColor.getNormal().g,
    //                                 m_borderColor.getNormal().b, m_borderColor.getNormal().a)){
    //         SDL_Log("Panel fFailed to set border color: %s", SDL_GetError());
    //     }
    //     if(!SDL_RenderRect(getRenderer(), drawRect.toSDLFRect())){
    //         SDL_Log("Panel failed to draw border: %s", SDL_GetError());
    //     }
    // }
}

bool Panel::handleEvent(shared_ptr<Event> event){
    return ControlImpl::handleEvent(event);

}

void Panel::addControl(shared_ptr<Control> control){
    ControlImpl::addControl(control);
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