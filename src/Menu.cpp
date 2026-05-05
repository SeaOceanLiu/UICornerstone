#include "Menu.h"

MenuBase::MenuBase(Control *parent,
                    MenuItemType type,
                    MenuDirection direction,
                    float xScale,
                    float yScale):
    ControlImpl(parent, xScale, yScale),
    m_parentMenu(nullptr),
    m_type(type),
    m_classLevel(MenuClassLevel::MenuItem),
    m_direction(direction),
    // m_alignedWidth(0),
    // m_alignedHeight(0),
    m_isExpanded(false),
    m_subMenuState(SubMenuState::Collapsed),
    m_expandedItem(nullptr),

    m_caption(nullptr),
    m_subMenuArrow(nullptr),
    m_captionText(""),
    m_captionSize(ConstDef::MENU_TEXT_SIZE),
    m_captionAlignment(AlignmentMode::AM_MID_LEFT),
    m_captionMargin(ConstDef::MENU_CAPTION_MARGIN),
    m_enableTextShadow(false),
    m_menuContainer(nullptr),
    m_onClick(nullptr)
{
    SDL_Log("MenuBase::MenuBase entered. direction = %d", m_direction);
    // 初始化菜单相关属性
    setRect(SRect(0, 0, 0, 0)); // 默认大小，后续可以调整
    // setBackgroundStateColor(StateColor(StateColor::Type::Background)
    //                             .setNormal({255, 255, 0, 255})
    //                             .setPressed({255, 255, 255, 255})
    // );
}

void MenuBase::draw(void)
{
    if (getVisible() == false) return;

    // 绘制菜单
    const SRect drawRect = getDrawRect();

    // 先绘制当前控件的外观
    drawBackground(&drawRect);

    // 绘制菜单标题
    if (m_caption != nullptr){
        m_caption->draw();
    }

    // 绘制子菜单箭头
    if (m_subMenuArrow != nullptr){
        m_subMenuArrow->draw();
    }

    // 最后绘制边框
    drawBorder(&drawRect);

}
void MenuBase::drawContainer(void)
{
    if (getVisible() == false) return;

    // 如果本级菜单被展开，那就绘制本级菜单项
    if (getExpanded() && m_menuContainer != nullptr) {
        m_menuContainer->draw();
    }
}

bool MenuBase::handleEvent(shared_ptr<Event> event){
    if (!getEnable() || !getVisible()) return false;

    // 如果有子菜单，则让子菜单先处理事件
    if (m_menuContainer != nullptr){
        if (m_menuContainer->handleEvent(event)) return true;
    }

    if (EventQueue::isPositionEvent(event->m_eventName)){
        if (!event->m_eventParam.has_value()) return false;
        try {
            shared_ptr<SPoint> pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            SRect drawRect = getDrawRect();
            if (drawRect.contains(pos->x, pos->y)){
                shared_ptr<MenuBase> subMenu = nullptr;
                switch(event->m_eventName){
                    case EventName::FINGER_DOWN:
                    case EventName::FINGER_MOTION:
                        if (getType() == MenuItemType::Normal && m_onClick != nullptr){
                            m_onClick(dynamic_pointer_cast<MenuBase>(this->getThis()));
                        }
                        setState(getExpanded() ? ControlState::Pressed : ControlState::Normal);
                        return true;
                    case EventName::MOUSE_LBUTTON_DOWN:
                        switch(m_type){
                            case MenuItemType::SubMenu:
                                setExpended(!getExpanded());
                                if (getExpanded() && getParentMenu() != nullptr) {
                                    getParentMenu()->setExpendedSubMenuItem(dynamic_pointer_cast<MenuBase>(getThis()));
                                }
                                break;
                            case MenuItemType::Normal:
                                setExpended(false);

                                if (m_onClick != nullptr){
                                    SDL_Log("MenuBase::handleEvent m_onClick = 0x%0X, m_state = %d", m_onClick, m_state);
                                    closeMenuIntheChain();
                                    m_onClick(dynamic_pointer_cast<MenuBase>(this->getThis()));
                                }
                                break;
                            case MenuItemType::Separator:
                            default:
                                break;
                        }
                        return true;
                    case EventName::MOUSE_LBUTTON_UP:
                    case EventName::FINGER_UP:
                        return true;
                    case EventName::MOUSE_MOVING:
                        setMenuChainState(ControlState::Hover);
                        if (getType() == MenuItemType::SubMenu && getMenuClassLevel() == MenuClassLevel::MenuItem){
                            setExpended(true);
                        }
                        if (getParentMenu() != nullptr && getParentMenu()->getExpendedSubMenuItem() != nullptr){
                            SDL_Log("MenuBase::handleEvent: expend <%s>'s sub menu", getCaption().c_str());
                            getParentMenu()->setExpendedSubMenuItem(dynamic_pointer_cast<MenuBase>(getThis()));
                        }
                        return true;
                    case EventName::MOUSE_WHEEL:
                        return false;
                    default:
                        break;
                }
                return true;
            } else {
                setState(ControlState::Normal);
            }
        } catch (...) {
            return false;
        }
    }
    if (ControlImpl::handleEvent(event)) return true;
    return false;
}
bool MenuBase::isContainsPoint(float x, float y)
{
    if (getEnable() == false || getVisible() == false) return false;

    // 先判断自己是否包含该点
    if (ControlImpl::isContainsPoint(x, y)) {
        return true;
    }
    // 再判断菜单容器是否包含该点
    if (m_menuContainer != nullptr) {
        return m_menuContainer->isContainsPoint(x, y);
    }
    // // 最后断子菜单面板是否包含该点
    // if (m_expandedItem != nullptr) {
    //     return m_expandedItem->isContainsPoint(x, y);
    // }
    return false;
}

void MenuBase::setParent(Control *parent){
    ControlImpl::setParent(parent);
    SRect parentRect = getParent()->getRect();
    parentRect.left = 0 - ConstDef::MENU_ITEM_MARGIN.left;  // 子菜单做了偏移处理，为了对齐边线，这里需要调整回来
    parentRect.top = 0 - ConstDef::MENU_ITEM_MARGIN.top;    // 子菜单做了偏移处理，为了对齐边线，这里需要调整回来

    // auto menuParent = dynamic_cast<MenuBase*>(parent);
    // if (menuParent) {
        SDL_Log("MenuBase(%s)::setParent: do nothing !!!! (%f, %f)", getCaption().c_str(), parentRect.left, parentRect.top);

        // if (getDirection() == MenuDirection::Horizontal){
        //     parentRect.height = ConstDef::MENU_TEXT_SIZE + ConstDef::MENU_ITEM_MARGIN.top + ConstDef::MENU_ITEM_MARGIN.bottom;
        // } else {
        //     parentRect.width = ConstDef::MENU_TEXT_SIZE + ConstDef::MENU_ITEM_MARGIN.left + ConstDef::MENU_ITEM_MARGIN.right;
        // }
        // SDL_Log("MenuBase(%s)::setParent: no menuContainer, set rect(%f, %f, %f, %f)", getCaption().c_str(),
        //     parentRect.left, parentRect.top, parentRect.width, parentRect.height);
        // setRect(parentRect); // 重新设置矩形区域，以便调整菜单项位置
    // } else {
    //     SDL_Log("MenuBase(%s)::setParent:: Parent is NOT a MenuBase type (should be MenuContainer).", getCaption().c_str());

    // }
}
void MenuBase::setRect(SRect rect)
{
    ControlImpl::setRect(rect);

    // 调整菜单标题和子菜单箭头的位置
    // 先调整标题位置和大小，以便子菜单箭头能根据标题的实际宽度来调整位置
    if (m_caption != nullptr){
        SDL_Log("MenuBase(%s)::call m_caption->setRect: rect(0, 0, %f, %f)", getCaption().c_str(), m_rect.width, m_rect.height);
        m_caption->setRect({0, 0, m_rect.width, m_rect.height});
    }

    // 再调整子菜单箭头的位置
    if (m_subMenuArrow != nullptr){
        SDL_Log("MenuBase(%s)::call m_subMenuArrow->setRect: rect(%f, %f, %f, %f)", getCaption().c_str(),
            m_subMenuArrow->getRect().left, m_subMenuArrow->getRect().top, m_subMenuArrow->getRect().width, m_subMenuArrow->getRect().height);
        m_subMenuArrow->setLeft(getRect().width - m_subMenuArrow->getRect().width - ConstDef::MENU_ITEM_MARGIN.right);
        SDL_Log("MenuBase(%s)::call m_subMenuArrow->setRect: rect(%f, %f, %f, %f)", getCaption().c_str(),
            m_subMenuArrow->getRect().left, m_subMenuArrow->getRect().top, m_subMenuArrow->getRect().width, m_subMenuArrow->getRect().height);
    }
}
void MenuBase::addItem(shared_ptr<MenuBase> item)
{
    SDL_Log("MenuBase(%s)::addItem(%s) entered.", getCaption().c_str(), item->getCaption().c_str());
    if (!item) {
        SDL_Log("MenuBase(%s)::addItem(%s) item is nullptr", getCaption().c_str(), item->getCaption().c_str());
        throw std::runtime_error("MenuBase::addItem item is nullptr");
    }

    if (m_menuContainer == nullptr) {
        // 保护一下
        SDL_Log("MenuBase(%s)::addItem(%s): m_menuContainer is nullptr, creating new MenuContainer.", getCaption().c_str(), item->getCaption().c_str());
        m_menuContainer = make_shared<MenuContainer>(this);
    }
    item->setParent(dynamic_cast<Control*>(this));
    item->setParentMenu(dynamic_pointer_cast<MenuBase>(this->getThis()));
    m_menuContainer->addItem(item);
}
void MenuBase::removeItem(shared_ptr<MenuBase> item){
    SDL_Log("MenuBase(%s)::removeItem(%s) entered.", getCaption().c_str(), item->getCaption().c_str());
    if (!item) {
        SDL_Log("MenuBase(%s)::removeItem item is nullptr", getCaption().c_str());
        throw std::runtime_error("MenuBase::removeItem item is nullptr");
    }

    if (m_menuContainer == nullptr) return;

    m_menuContainer->removeItem(item);
    if (m_menuContainer->isEmpty()) {
        m_menuContainer = nullptr;
    }
}

void MenuBase::onMouseEnter(float x, float y)
{
    setState(ControlState::Hover);
}

void MenuBase::onMouseLeave(float x, float y)
{
    setState(ControlState::Normal);
}
void MenuBase::setTextStateColor(StateColor stateColor){
    ControlImpl::setTextStateColor(stateColor);
    if (m_caption != nullptr){
        m_caption->setTextStateColor(m_textColor);
    }
}
void MenuBase::setTextShadowStateColor(StateColor stateColor){
    ControlImpl::setTextShadowStateColor(stateColor);
    m_textShadowColor = stateColor;
    if (m_caption != nullptr){
        m_caption->setTextShadowStateColor(m_textShadowColor);
    }
}

void MenuBase::setTextShadowEnable(bool enable){
    m_enableTextShadow = enable;
    if (m_caption != nullptr){
        m_caption->setShadow(enable);
    }
}

void MenuBase::setCaption(string caption){
    m_captionText = caption;

    if (m_caption != nullptr){
        m_caption.reset();
        m_caption = nullptr;
    }
    if (m_captionText.length() > 0) {
        m_caption = LabelBuilder(this, {0, 0, m_rect.width, m_rect.height})
                            .setFont(FontName::HarmonyOS_Sans_SC_Regular)
                            .setAlignmentMode(m_captionAlignment)
                            .setMargin(m_captionMargin)
                            .setFontSize((int)m_captionSize)
                            .setCaption(m_captionText)
                            .setTextStateColor(m_textColor)
                            .setTextShadowStateColor(m_textShadowColor)
                            .setShadow(m_enableTextShadow)
                            .setEnableExpand(true)
                            .build();

        setRect({getRect().left, getRect().top,
            ConstDef::MENU_ITEM_MARGIN.left + ConstDef::LABEL_CAPTION_MARGIN.left +
                m_caption->getHotRect().width + ConstDef::MENU_ITEM_MARGIN.right + ConstDef::LABEL_CAPTION_MARGIN.right,
            ConstDef::MENU_ITEM_MARGIN.top + ConstDef::LABEL_CAPTION_MARGIN.top +
                m_caption->getHotRect().height + ConstDef::MENU_ITEM_MARGIN.bottom + ConstDef::LABEL_CAPTION_MARGIN.bottom}); // 根据文本内容调整菜单项大小
        SDL_Log("MenuBase(%s)::setCaption(%s): rect(%f, %f, %f, %f)", getCaption().c_str(),
            m_captionText.c_str(), getRect().left, getRect().top, getRect().width, getRect().height);
    }
    createSubMenuArrowLabel();
}
string MenuBase::getCaption(void) const{
    return m_captionText;
}
void MenuBase::createSubMenuArrowLabel(void){
    if (getType() == MenuItemType::SubMenu && m_caption != nullptr){
        // 创建子菜单箭头
        if (m_subMenuArrow != nullptr){
            m_subMenuArrow.reset();
            m_subMenuArrow = nullptr;
        }
        m_subMenuArrow = LabelBuilder(this, {0, 0, 0, m_rect.height})    // 宽度设置为0，Label会根据内容自适应宽度
                                .setFont(FontName::Quando_Regular)  // 使用直角符号作为箭头显示
                                .setAlignmentMode(AlignmentMode::AM_CENTER)
                                .setMargin(m_captionMargin)
                                .setFontSize((int)m_captionSize)
                                .setCaption(">") // 使用一个简单的箭头符号表示子菜单
                                .setTextStateColor(m_textColor)
                                .setTextShadowStateColor(m_textShadowColor)
                                .setShadow(m_enableTextShadow)
                                .setEnableExpand(true)
                                .build();
        m_subMenuArrow->setWidth(m_subMenuArrow->getHotRect().width); // 根据内容调整箭头宽度
        // 调整菜单项宽度以容纳箭头
        setWidth(getRect().width + m_subMenuArrow->getHotRect().width);
    }
}
void MenuBase::destroySubMenuArrowLabel(void){
    if (m_subMenuArrow != nullptr){
        // 调整菜单项宽度以去除箭头
        setRect({getRect().left, getRect().top,
            getRect().width - m_subMenuArrow->getRect().width,
            getRect().height});
        // setWidth(getRect().width - m_subMenuArrow->getRect().width);

        m_subMenuArrow.reset();
        m_subMenuArrow = nullptr;
    }
}
void MenuBase::setCaptionSize(float size){
    m_captionSize = size;
    if (m_caption != nullptr){
        m_caption->setFontSize((int)m_captionSize);
    }
}
uint32_t MenuBase::getCaptionSize(float size) const{
    return static_cast<uint32_t>(m_captionSize);
}

void MenuBase::setCaptionAlignment(AlignmentMode mode){
    m_captionAlignment = mode;
    if (m_caption != nullptr){
        m_caption->setAlignmentMode(mode);
    }
}
AlignmentMode MenuBase::getCaptionAlignment(void) const{
    return m_captionAlignment;
}
void MenuBase::setCaptionMargin(Margin margin){
    m_captionMargin = margin;
    if (m_caption != nullptr){
        m_caption->setMargin(margin);
    }
}
Margin MenuBase::getCaptionMargin(void) const{
    return m_captionMargin;
}
SRect MenuBase::getCaptionRect(void) const{
    return m_caption != nullptr ? m_caption->getRect() : SRect(0, 0, 0, 0);
}

void MenuBase::setMenuChainState(ControlState state){
    if (getEnable() == false) return;

    if (getType() != MenuItemType::Separator && getState() != ControlState::Pressed) setState(state);

    shared_ptr<MenuBase>parentMenu = getParentMenu();
    if (parentMenu == nullptr) return;
    parentMenu->setMenuChainState(state); // 递归设置父控件状态
}
void MenuBase::closeMenuIntheChain(void){
    if (getEnable() == false) return;
    setState(ControlState::Normal);
    if (getParent() == nullptr) return;

    SDL_Log("MenuBase(%s)::closeMenuIntheChain", getCaption().c_str());
    if (getParentMenu() != nullptr) // 判断parentMenu不为空是为了保障MainMenu始终显示
    {
        setExpended(false); // 关闭当前菜单Container
    }

    if(getExpendedSubMenuItem() != nullptr){
        getExpendedSubMenuItem()->setExpended(false);   // 关闭子菜单
        setExpendedSubMenuItem(nullptr);
    }

    shared_ptr<MenuBase>parentMenu = getParentMenu();
    if (parentMenu == nullptr /*|| parentMenu->getParentMenu() == nullptr*/) return; // 这里保证MainMenu是常驻的，所以不需要关闭MainMenu
    parentMenu->closeMenuIntheChain(); // 递归关闭父菜单
}
shared_ptr<MenuBase> MenuBase::getSubMenuAtPoint(float x, float y) {
    if (isContainsPoint(x, y)) return dynamic_pointer_cast<MenuBase>(getThis());

    if (m_menuContainer == nullptr) return nullptr;
    if (m_expandedItem != nullptr && m_expandedItem->isContainsPoint(x, y)) {
        return m_expandedItem;
    } else {
        return m_menuContainer->getSubMenuAtPoint(x, y);
    }

    return nullptr;
}

MenuContainer::MenuContainer(MenuBase *parent, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale),
    m_alignedWidth(0),
    m_alignedHeight(0)
{
    setBorderVisible(true); // 默认显示边框
    setVisible(false); // 默认隐藏
}

void MenuContainer::draw(void){
    if (getVisible() == false) return;

    SRect drawRect = getDrawRect();
    // 先绘制当前控件的外观

    // 绘制菜单容器背景
    drawBackground(&drawRect);

    // 所有下级菜单项标题
    for (auto& item : m_items){
        item->draw();
    }
    drawBorder(&drawRect);

    // 绘制子菜单
    for (auto& item : m_items){
        item->drawContainer();
    }
}

bool MenuContainer::handleEvent(shared_ptr<Event> event){
    if (!getEnable() || !getVisible()) return false;

    // 传递给各子菜单项优先处理事件
    for (auto& item : m_items){
        if (item->handleEvent(event)) return true;
    }

    // 下面处理鼠标在Container中移动的逻辑，使得鼠标在两个菜单项之间的空隙间时，仍能正确显示各级菜单的Hover状态
    if (EventQueue::isPositionEvent(event->m_eventName)){
        if (!event->m_eventParam.has_value()) return false;
        try {
            shared_ptr<SPoint> pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            SRect drawRect = getDrawRect();
            if (drawRect.contains(pos->x, pos->y)){
                shared_ptr<MenuBase> subMenu = nullptr;
                switch(event->m_eventName){
                    case EventName::MOUSE_MOVING:
                        dynamic_cast<MenuBase *>(getParent())->setMenuChainState(ControlState::Hover);
                        return true;
                    case EventName::MOUSE_WHEEL:
                        return false;
                    default:
                        break;
                }
                return true;
            }
        } catch (...) {
            return false;
        }
    }
    return false;
}
bool MenuContainer::isContainsPoint(float x, float y){
    if (getEnable() == false || getVisible() == false) return false;

    // 先判断自己是否包含该点
    if (ControlImpl::isContainsPoint(x, y)) {
        return true;
    }
    // 再判断子菜单是否包含该点
    for (auto& item : m_items) {
        if (item->isContainsPoint(x, y)) {
            return true;
        }
    }
    return false;
}
void MenuContainer::addItem(shared_ptr<MenuBase> item, bool intoMenuList){
    SDL_Log("MenuContainer(%s)::addItem(%s) entered.", getMountMenuCaption().c_str(), item->getCaption().c_str());
    item->setParent(this);

    float targetWidth = 0;
    float targetHeight = 0;

    if (item->getType() == MenuItemType::Separator) {
        // 分隔符菜单项，按分隔符默认大小设置默认大小
        item->setDirection(getDirection()); // 分隔符菜单项方向自动与菜单容器一致
        SDL_Log("MenuContainer(%s)::addItem(%s): item is separator, set item direction = %d, by parent direction = %d",
            getMountMenuCaption().c_str(), item->getCaption().c_str(), item->getDirection(), getDirection());
        if (getDirection() == MenuDirection::Horizontal){
            item->setRect(SRect(ConstDef::MENU_ITEM_MARGIN.left + m_items.back()->getRect().right(),
                ConstDef::MENU_ITEM_MARGIN.top,
                ConstDef::MENU_SEPARATOR_WIDTH,
                getRect().height - ConstDef::MENU_ITEM_MARGIN.top - ConstDef::MENU_ITEM_MARGIN.bottom));

            // 调整菜单容器大小以适应分隔符
            setRect(SRect(getRect().left,
                            getRect().top,
                            item->getRect().right() + ConstDef::MENU_ITEM_MARGIN.right,
                            ConstDef::MENU_ITEM_MARGIN.top + m_alignedHeight + ConstDef::MENU_ITEM_MARGIN.bottom));
            SDL_Log("MenuContainer(%s)::addItem(%s): reset container rect: %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(),
                    getRect().left,
                    getRect().top,
                    getRect().width,
                    getRect().height);
        } else {    // MenuDirection::Vertical
            item->setRect(SRect(ConstDef::MENU_ITEM_MARGIN.left,
                ConstDef::MENU_ITEM_MARGIN.top + m_items.back()->getRect().bottom(),
                getRect().width - ConstDef::MENU_ITEM_MARGIN.left - ConstDef::MENU_ITEM_MARGIN.right,
                ConstDef::MENU_SEPARATOR_HEIGHT));

            // 调整菜单容器大小以适应分隔符
            setRect(SRect(getRect().left,
                            getRect().top,
                            m_alignedWidth + ConstDef::MENU_ITEM_MARGIN.left + ConstDef::MENU_ITEM_MARGIN.right,
                            item->getRect().bottom() + ConstDef::MENU_ITEM_MARGIN.bottom));
            SDL_Log("MenuContainer(%s)::addItem(%s): reset container rect: %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(),
                    getRect().left,
                    getRect().top,
                    getRect().width,
                    getRect().height);
        }
        if (intoMenuList) {
            m_items.push_back(item);
        }
        return; // 分隔符菜单项不需要再调整菜单容器中其它菜单的大小
    } else {
        SDL_Log("MenuContainer(%s)::addItem(%s): alignedWidth=%f, alignedHeight=%f", getMountMenuCaption().c_str(), item->getCaption().c_str(), m_alignedWidth, m_alignedHeight);
        // 非分隔符菜单项，按菜单文字设置默认大小
        item->setRect(SRect(ConstDef::MENU_ITEM_MARGIN.left,
                            ConstDef::MENU_ITEM_MARGIN.top,
                            item->getCaptionRect().width,
                            item->getCaptionRect().height));
        SDL_Log("MenuContainer(%s)::addItem(%s): caption size: %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(), item->getCaptionRect().width, item->getCaptionRect().height);

        targetWidth = max(m_alignedWidth, item->getRect().width);
        targetHeight = max(m_alignedHeight, item->getRect().height);

        SDL_Log("MenuContainer(%s)::addItem(%s): calculated target size: %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(), targetWidth, targetHeight);
        SDL_Log("MenuContainer(%s)::addItem(%s): item rect = %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(), item->getRect().left,
                item->getRect().top,
                item->getRect().width,
                item->getRect().height);
    }


    SDL_Log("MenuContainer(%s)::addItem(%s) %d: targetWidth=%f, targetHeight=%f",
        getMountMenuCaption().c_str(), item->getCaption().c_str(), intoMenuList, targetWidth, targetHeight);

    // 将新菜单项添加到菜单项列表中
    if (intoMenuList) {
        m_items.push_back(item);
    }

    SDL_Log("MenuContainer(%s)::addItem(%s) total items = %d", getMountMenuCaption().c_str(), item->getCaption().c_str(), m_items.size());

    // 计算菜单项的位置
    if (getDirection() == MenuDirection::Vertical) {
        SDL_Log("MenuContainer(%s)::addItem(%s): direction is vertical.", getMountMenuCaption().c_str(), item->getCaption().c_str());
        // 当新菜单项宽高度大于已有菜单项宽高度时，调整已有菜单项宽高度，使之保持一致
        if (targetWidth != m_alignedWidth || targetHeight != m_alignedHeight) {
            float targetLeft = ConstDef::MENU_ITEM_MARGIN.left;
            float targetTop = ConstDef::MENU_ITEM_MARGIN.top;

            for (size_t i = 0; i < m_items.size(); ++i) {
                auto& existingItem = m_items[i];
                if (existingItem->getType() != MenuItemType::Separator) {
                    existingItem->setRect(SRect(targetLeft,
                                                targetTop,
                                                targetWidth,
                                                targetHeight));
                    targetTop += targetHeight + ConstDef::MENU_ITEM_MARGIN.bottom + ConstDef::MENU_ITEM_MARGIN.top;
                    SDL_Log("MenuContainer(%s)::addItem(%s): adjust item[%d] rect: %f, %f, %f, %f",getMountMenuCaption().c_str(), item->getCaption().c_str(), i,
                            existingItem->getRect().left,
                            existingItem->getRect().top,
                            existingItem->getRect().width,
                            existingItem->getRect().height);
                } else { // Separator
                    existingItem->setRect(SRect(targetLeft,
                                                targetTop,
                                                targetWidth,
                                                ConstDef::MENU_SEPARATOR_HEIGHT));
                    targetTop += ConstDef::MENU_SEPARATOR_HEIGHT + ConstDef::MENU_ITEM_MARGIN.bottom + ConstDef::MENU_ITEM_MARGIN.top;
                }
            }
        } else {
            // 宽高度无变化时，直接在最后添加菜单项
            m_items.back()->setRect(SRect(ConstDef::MENU_ITEM_MARGIN.left,
                                            m_items[m_items.size() - 2]->getRect().bottom() + ConstDef::MENU_ITEM_MARGIN.bottom + ConstDef::MENU_ITEM_MARGIN.top,
                                            targetWidth,
                                            targetHeight));
            SDL_Log("MenuContainer(%s)::addItem(%s): %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(),
                    m_items.back()->getRect().left,
                    m_items.back()->getRect().top,
                    m_items.back()->getRect().width,
                    m_items.back()->getRect().height);
        }
        // 调整当前菜单的宽高度以适应新增菜单项
        setRect(SRect(getRect().left,
                        getRect().top,
                        targetWidth + ConstDef::MENU_ITEM_MARGIN.left + ConstDef::MENU_ITEM_MARGIN.right,
                        m_items.back()->getRect().bottom() + ConstDef::MENU_ITEM_MARGIN.bottom));
        SDL_Log("MenuContainer(%s)::addItem(%s): reset container rect: %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(),
                getRect().left,
                getRect().top,
                getRect().width,
                getRect().height);
    } else { // Horizontal
        SDL_Log("MenuContainer(%s)::addItem(%s): direction is horizontal.", getMountMenuCaption().c_str(), item->getCaption().c_str());
        // 对于水平菜单，只要求菜单项高度一致，所以在新菜单项高度大于已有菜单项高度时，调整已有菜单项高度
        if (targetHeight != m_alignedHeight) {
            float targetLeft = ConstDef::MENU_ITEM_MARGIN.left;
            float targetTop = ConstDef::MENU_ITEM_MARGIN.top;

            SDL_Log("MenuContainer(%s)::addItem(%s): targetHeight changed, adjust existing items. targetHeight=%f, alignedHeight=%f", getMountMenuCaption().c_str(), item->getCaption().c_str(), targetHeight, m_alignedHeight);
            for (size_t i = 0; i < m_items.size(); ++i) {
                auto& existingItem = m_items[i];
                if (existingItem->getType() != MenuItemType::Separator) {
                    existingItem->setRect(SRect(targetLeft,
                                                targetTop,
                                                existingItem->getRect().width,
                                                targetHeight));
                    targetLeft += existingItem->getRect().width + ConstDef::MENU_ITEM_MARGIN.right + ConstDef::MENU_ITEM_MARGIN.left;
                    SDL_Log("MenuContainer(%s)::addItem(%s): adjust item[%d] rect: %f, %f, %f, %f",getMountMenuCaption().c_str(), item->getCaption().c_str(), i,
                            existingItem->getRect().left,
                            existingItem->getRect().top,
                            existingItem->getRect().width,
                            existingItem->getRect().height);
                } else { // Separator
                    existingItem->setRect(SRect(targetLeft,
                                                targetTop,
                                                ConstDef::MENU_SEPARATOR_WIDTH,
                                                targetHeight));
                    targetLeft += ConstDef::MENU_SEPARATOR_WIDTH + ConstDef::MENU_ITEM_MARGIN.right + ConstDef::MENU_ITEM_MARGIN.left;
                }
            }
        } else {
            // 高度无变化时，直接在最后添加菜单项
            if(m_items.size() == 1) {
                m_items.back()->setRect(SRect(ConstDef::MENU_ITEM_MARGIN.left,
                                                ConstDef::MENU_ITEM_MARGIN.top,
                                                targetWidth,
                                                targetHeight));

            } else if (m_items.size() > 1) {
                m_items.back()->setRect(SRect(m_items[m_items.size() - 2]->getRect().right() + ConstDef::MENU_ITEM_MARGIN.right + ConstDef::MENU_ITEM_MARGIN.left,
                                                ConstDef::MENU_ITEM_MARGIN.top,
                                                m_items.back()->getRect().width,
                                                targetHeight));
            }
            SDL_Log("MenuContainer(%s)::addItem(%s): %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(),
                    m_items.back()->getRect().left,
                    m_items.back()->getRect().top,
                    m_items.back()->getRect().width,
                    m_items.back()->getRect().height);
        }
        // 调整当前菜单的宽高度以适应新增菜单项
        setRect(SRect(getRect().left,
                        getRect().top,
                        m_items.back()->getRect().right() + ConstDef::MENU_ITEM_MARGIN.right,
                        ConstDef::MENU_ITEM_MARGIN.top + targetHeight + ConstDef::MENU_ITEM_MARGIN.bottom));
        SDL_Log("MenuContainer(%s)::addItem(%s): reset container rect: %f, %f, %f, %f", getMountMenuCaption().c_str(), item->getCaption().c_str(),
                getRect().left,
                getRect().top,
                getRect().width,
                getRect().height);
    }

    // 更新菜单项的宽高对齐值
    m_alignedWidth = targetWidth;
    m_alignedHeight = targetHeight;
}
void MenuContainer::removeItem(shared_ptr<MenuBase> item){
    // 重置宽高对齐值和菜单项位置
    m_alignedWidth = 0;
    m_alignedHeight = 0;
    setRect(SRect(getRect().left, getRect().top, 0, 0));

    // 从菜单项列表中移除指定菜单项
    auto it = m_items.begin();
    while (it != m_items.end()) {
        if (*it == item) {
            it = m_items.erase(it);
        } else {
            addItem(*it, false); // 重新布局剩余菜单项
            ++it;
        }
    }

}
MenuDirection MenuContainer::getDirection(void) {
    return dynamic_cast<MenuBase*>(getParent())->getDirection();
}
string MenuContainer::getMountMenuCaption(void) {
    return dynamic_cast<MenuBase*>(getParent())->getCaption();
}

shared_ptr<MenuBase> MenuContainer::getSubMenuAtPoint(float x, float y) {
    if (!isContainsPoint(x, y)) return nullptr;

    for (auto& item : m_items) {
        if (item->isContainsPoint(x, y)) {
            return item;
        }
    }
    return nullptr;
}

void MenuSeparator::draw(void) {
    if (getVisible() == false) return;
    if (getParent() == nullptr) return;

    SDL_Renderer* renderer = getRenderer();
    if (!renderer) return;

    MenuBase::draw();

    SRect drawRect = getDrawRect();

    SDL_SetRenderDrawColor(renderer,
        m_borderColor.getNormal().r,
        m_borderColor.getNormal().g,
        m_borderColor.getNormal().b,
        m_borderColor.getNormal().a);
    SDL_FRect lineRect;
    if (getDirection() == MenuDirection::Vertical) {
        lineRect = {drawRect.left - ConstDef::MENU_ITEM_MARGIN.left,
                        drawRect.top + drawRect.height/2,
                        drawRect.width + ConstDef::MENU_ITEM_MARGIN.left + ConstDef::MENU_ITEM_MARGIN.right,
                        1
                    };
    } else {
        lineRect = {drawRect.left + drawRect.width/2,
                        drawRect.top - ConstDef::MENU_ITEM_MARGIN.top,
                        1,
                        drawRect.height + ConstDef::MENU_ITEM_MARGIN.top + ConstDef::MENU_ITEM_MARGIN.bottom
                    };
    }
    SDL_RenderFillRect(renderer, &lineRect);
}

MenuSeparatorBuilder::MenuSeparatorBuilder(float xScale, float yScale, Control *parent):
    MenuBaseBuilder(
        [parent, xScale, yScale]() -> shared_ptr<MenuBase> {
            if (parent) {
                auto menuParent = dynamic_cast<MenuBase*>(parent);
                if (!menuParent) {
                    SDL_Log("MenuSeparatorBuilder: Parent must be a MenuBase type or nullptr");
                    throw std::runtime_error("Parent must be a MenuBase type or nullptr");
                }
            }
            return make_shared<MenuSeparator>(dynamic_cast<MenuBase*>(parent), xScale, yScale);
        }(),
        MenuItemType::Separator, MenuDirection::Horizontal, xScale, yScale)
{
    m_separator = dynamic_pointer_cast<MenuSeparator>(m_menu);
}

shared_ptr<MenuBase> MenuSeparatorBuilder::build(void) {
    m_separator->create();
    return m_separator;
}

MenuBar::MenuBar(Control *parent, MenuDirection direction, float xScale, float yScale):
    MenuBase(parent, MenuItemType::Normal, direction, xScale, yScale),
    m_showTopSubMenu(false)
{
    setCaption("");
    if (parent != nullptr) {
        setRect(SRect(0, 0, parent->getRect().width, 30));
    }
    setBorderVisible(true);
    setExpended(true);
    setMenuClassLevel(MenuClassLevel::MenuBar);
}

void MenuBar::setShowTopSubMenu(bool show) {
    m_showTopSubMenu = show;
    if (show == false) {
        setExpendedSubMenuItem(nullptr);
    }
}

bool MenuBar::getShowTopSubMenu(void) const {
    return m_showTopSubMenu;
}

void MenuBar::draw(void) {
    MenuBase::draw();
    drawContainer();
}

void MenuBar::setParent(Control *parent) {
    MenuBase::setParent(parent);
    if (parent != nullptr) {
        setRect(SRect(0, 0, parent->getRect().width, getRect().height));
    }
}

bool MenuBar::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    shared_ptr<SPoint> pos;
    SRect drawRect;
    switch(event->m_eventName){
        case EventName::MOUSE_LBUTTON_DOWN:
            pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!isContainsPoint(pos->x, pos->y)) {
                if(getExpendedSubMenuItem() != nullptr){
                    getExpendedSubMenuItem()->setExpended(false);
                    setExpendedSubMenuItem(nullptr);
                }
            }
            break;
        case EventName::MOUSE_MOVING:
            drawRect = getDrawRect();
            pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!drawRect.contains(pos->x, pos->y)){
                setState(ControlState::Normal);
            }
            break;
        default:
            break;
    }
    return false;
}

void MenuBar::addMainMenu(shared_ptr<MenuBase> item) {
    if (!item) {
        SDL_Log("MenuBar::addMainMenu item is nullptr");
        throw std::runtime_error("MenuBar::addMainMenu item is nullptr");
    }

    if (m_menuContainer == nullptr) {
        setType(MenuItemType::SubMenu);
        m_menuContainer = make_shared<MenuContainer>(this);
        m_menuContainer->create();
        if (getDirection() == MenuDirection::Horizontal) {
            m_menuContainer->setRect({0, 0, 0, 0});
        } else {
            SDL_Log("MainMenu::addMenuItem getRect().height = %f", getRect().height);
            m_menuContainer->setRect({0, 0, 0, 0});
        }
    }
    addItem(item);
    setRect({getRect().left, getRect().top, m_menuContainer->getRect().width, m_menuContainer->getRect().height});
    SDL_Log("MenuBar::addMainMenu rect = {%f, %f, %f, %f}", getRect().left, getRect().top, getRect().width, getRect().height);
}

void MenuBar::removeMainMenu(shared_ptr<MenuBase> item){
    if (!item) {
        SDL_Log("MenuBar::removeMainMenu item is nullptr");
        throw std::runtime_error("MenuBar::removeMainMenu item is nullptr");
    }
    removeItem(item);
    if (m_menuContainer == nullptr) {
        setType(MenuItemType::Normal);
        if(getParent() != nullptr)
            setRect({getRect().left, getRect().top, getParent()->getRect().width, getRect().height});
    } else {
        setRect({getRect().left, getRect().top, m_menuContainer->getRect().width, m_menuContainer->getRect().height});
    }
}

MainMenu::MainMenu(Control *parent, MenuDirection direction, float xScale, float yScale):
    MenuBase(parent, MenuItemType::Normal, direction, xScale, yScale)
{
    SDL_Log("ManMenu::MainMenu direction = %d", getDirection());
    setCaptionAlignment(AlignmentMode::AM_CENTER);
    setMenuClassLevel(MenuClassLevel::MainMenu);
}

bool MainMenu::handleEvent(shared_ptr<Event> event) {
    shared_ptr<SPoint> pos;
    shared_ptr<MenuBar>menuBar;
    switch(event->m_eventName){
        case EventName::MOUSE_LBUTTON_DOWN:
            menuBar = dynamic_pointer_cast<MenuBar>(getParentMenu());
            if (menuBar != nullptr) {
                menuBar->setShowTopSubMenu(menuBar->getShowTopSubMenu());
            }
            break;
        default:
            break;
    }
    return MenuBase::handleEvent(event);
}

bool MainMenu::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    shared_ptr<SPoint> pos;
    SRect drawRect;
    switch(event->m_eventName){
        case EventName::MOUSE_MOVING:
            drawRect = getDrawRect();
            pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!drawRect.contains(pos->x, pos->y)){
                setState(ControlState::Normal);
            }
            break;
        default:
            break;
    }
    return false;
}

void MainMenu::addMenuItem(shared_ptr<MenuBase> item){
    if (m_menuContainer == nullptr) {
        setType(MenuItemType::SubMenu);
        m_menuContainer = make_shared<MenuContainer>(this);

        if (getDirection() == MenuDirection::Horizontal) {
            m_menuContainer->setRect({getRect().width - ConstDef::MENU_ITEM_MARGIN.left, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
        } else {
            SDL_Log("MainMenu::addMenuItem getRect().height = %f", getRect().height);
            m_menuContainer->setRect({0 - ConstDef::MENU_ITEM_MARGIN.left, getRect().height - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
        }
        m_menuContainer->create();
        m_menuContainer->setVisible(false);

        SDL_Log("MainMenu::addMenuItem rect = {%f, %f, %f, %f}", getRect().left, getRect().top, getRect().width, getRect().height);
    }
    addItem(item);
}

void MainMenu::removeMenuItem(shared_ptr<MenuBase> item){
    removeItem(item);
    if (m_menuContainer == nullptr) {
        setType(MenuItemType::Normal);
    }
}

MenuItem::MenuItem(Control *parent, MenuDirection direction, float xScale, float yScale):
    MenuBase(parent, MenuItemType::Normal, direction, xScale, yScale)
{
    setMenuClassLevel(MenuClassLevel::MenuItem);
}

bool MenuItem::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    shared_ptr<SPoint> pos;
    SRect drawRect;
    switch(event->m_eventName){
        case EventName::MOUSE_MOVING:
            drawRect = getDrawRect();
            pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!drawRect.contains(pos->x, pos->y)){
                setState(ControlState::Normal);
                setExpended(false);
            }
            break;
        default:
            break;
    }
    return false;
}

void MenuItem::addSubMenuItem(shared_ptr<MenuBase> item){
    if (m_menuContainer == nullptr) {
        setType(MenuItemType::SubMenu);
        m_menuContainer = make_shared<MenuContainer>(this);

        if (getDirection() == MenuDirection::Horizontal) {
            m_menuContainer->setRect({0 - ConstDef::MENU_ITEM_MARGIN.left, getRect().height - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
        } else {
            m_menuContainer->setRect({getRect().width, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
        }
        SDL_Log("MenuItem(%s)::new container rect = {%f, %f, %f, %f}, direction=%s", getCaption().c_str(),
            m_menuContainer->getRect().left,
            m_menuContainer->getRect().top,
            m_menuContainer->getRect().width,
            m_menuContainer->getRect().height,
            (getDirection() == MenuDirection::Horizontal) ? "Horizontal" : "Vertical"
        );
        createSubMenuArrowLabel();
        if (getDirection() == MenuDirection::Horizontal) {
            m_menuContainer->setRect({0 - ConstDef::MENU_ITEM_MARGIN.left, getRect().height - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
        } else {
            m_menuContainer->setRect({getRect().width + ConstDef::MENU_ITEM_MARGIN.left, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
        }

        m_menuContainer->create();
        m_menuContainer->setVisible(false);
    }
    addItem(item);
}

void MenuItem::removeSubMenuItem(shared_ptr<MenuBase> item) {
    removeItem(item);
    if (m_menuContainer == nullptr) {
        setType(MenuItemType::Normal);
        destroySubMenuArrowLabel();
    }
}

MenuBarBuilder::MenuBarBuilder(Control *parent, MenuDirection direction, float xScale, float yScale):
    MenuBaseBuilder(
        make_shared<MenuBar>(parent, direction, xScale, yScale),
        MenuItemType::Normal, direction, xScale, yScale)
{
    m_menuBar = dynamic_pointer_cast<MenuBar>(m_menu);
    m_menuBar->setCaption("MenuBar");
    SDL_Log("MenuBarBuilder: Created MenuBar with rect {%f, %f, %f, %f}", m_menuBar->getRect().left, m_menuBar->getRect().top, m_menuBar->getRect().width, m_menuBar->getRect().height);
}

MenuBarBuilder& MenuBarBuilder::addMainMenu(shared_ptr<MenuBase> item) {
    m_menuBar->addMainMenu(item);
    return *this;
}

MenuBarBuilder& MenuBarBuilder::addBeforeEventHandlingWatcher(EventName eventName) {
    EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_menuBar);
    return *this;
}

shared_ptr<MenuBase> MenuBarBuilder::build(void) {
    m_menuBar->create();
    return m_menuBar;
}

MainMenuBuilder::MainMenuBuilder(string caption, MenuDirection direction, float xScale, float yScale, Control *parent):
    MenuBaseBuilder(
        [parent, direction, xScale, yScale]() -> shared_ptr<MenuBase> {
            if (parent) {
                auto menuParent = dynamic_cast<MenuBase*>(parent);
                if (!menuParent) {
                    SDL_Log("MainMenuBuilder: Parent must be a MenuBase type or nullptr");
                    throw std::runtime_error("Parent must be a MenuBase type or nullptr");
                }
            }
            return make_shared<MainMenu>(parent, direction, xScale, yScale);
        }(),
        MenuItemType::Normal, direction, xScale, yScale)
{
    m_mainMenu = dynamic_pointer_cast<MainMenu>(m_menu);
    if (!m_mainMenu) {
        SDL_Log("MainMenuBuilder::MainMenuBuilder: Failed to cast MenuBase to MainMenu");
        throw std::runtime_error("Failed to cast MenuBase to MainMenu");
    }
    m_mainMenu->setCaption(caption);
}

MainMenuBuilder& MainMenuBuilder::addBeforeEventHandlingWatcher(EventName eventName) {
    EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_mainMenu);
    return *this;
}

MainMenuBuilder& MainMenuBuilder::addMenuItem(shared_ptr<MenuBase> item) {
    m_mainMenu->addMenuItem(item);
    return *this;
}

shared_ptr<MenuBase> MainMenuBuilder::build(void) {
    if (!m_mainMenu) {
        SDL_Log("MainMenuBuilder::build: Failed to cast MenuBase to MainMenu");
        throw std::runtime_error("MainMenu is not properly initialized");
    }
    m_mainMenu->create();
    return m_mainMenu;
}

MenuItemBuilder::MenuItemBuilder(string caption, MenuDirection direction, float xScale, float yScale, Control *parent):
    MenuBaseBuilder(
        [parent, direction, xScale, yScale]() -> shared_ptr<MenuBase> {
            if (parent) {
                auto menuParent = dynamic_cast<MenuBase*>(parent);
                if (!menuParent) {
                    SDL_Log("MenuItemBuilder: Parent must be a MenuBase type or nullptr");
                    throw std::runtime_error("Parent must be a MenuBase type or nullptr");
                }
            }
            return make_shared<MenuItem>(parent, direction, xScale, yScale);
        }(),
        MenuItemType::Normal, direction, xScale, yScale)
{
    m_menuItem = dynamic_pointer_cast<MenuItem>(m_menu);
    if (!m_menuItem) {
        SDL_Log("MenuItemBuilder::MenuItemBuilder: Failed to cast MenuBase to MenuItem");
        throw std::runtime_error("Failed to cast MenuBase to MenuItem");
    }
    m_menuItem->setCaption(caption);
}

MenuItemBuilder& MenuItemBuilder::addBeforeEventHandlingWatcher(EventName eventName) {
    EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_menuItem);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::addSubMenuItem(shared_ptr<MenuBase> item) {
    m_menuItem->addSubMenuItem(item);
    return *this;
}

shared_ptr<MenuBase> MenuItemBuilder::build(void) {
    m_menuItem->create();
    return m_menuItem;
}
