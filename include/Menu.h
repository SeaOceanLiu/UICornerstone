#ifndef MenuH
#define MenuH

#include <memory>
#include <vector>
#include <functional>

#include <SDL3/SDL.h>

#include "ControlBase.h"
#include "Label.h"

using namespace std;

// 菜单项类型枚举
enum class MenuItemType {
    Normal,     // 普通菜单项，即无下级菜单，点击后执行相应的操作
    Separator,  // 分隔符，用于分隔菜单项
    SubMenu     // 子菜单项，即有下级菜单，点击后展开子菜单，绘制竖向菜单时要画出右侧的箭头
};
enum class MenuDirection {
    Horizontal,  // 水平菜单
    Vertical     // 垂直菜单
};
enum class MenuClassLevel {
    MenuBar,    // 菜单栏
    MainMenu,   // 主菜单
    MenuItem    // 菜单项
};
enum class SubMenuState {
    Collapsed,  // 子菜单折叠
    Expanded    // 子菜单展开
};

// 前向声明
class MenuBase;
class MenuItem;
class MainMenu;
class MenuBar;

class MenuContainer: public ControlImpl {
public:
    MenuContainer(MenuBase *parent, float xScale=1.0f, float yScale=1.0f);
    virtual ~MenuContainer() {}

    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override; // 检查菜单容器是否包含指定点
    void addItem(shared_ptr<MenuBase> item, bool intoMenuList=true);
    void removeItem(shared_ptr<MenuBase> item);
    bool isEmpty(void) const { return m_items.empty(); }
    shared_ptr<MenuBase> getSubMenuAtPoint(float x, float y);
protected:
    float m_alignedWidth;
    float m_alignedHeight;
    vector<shared_ptr<MenuBase>> m_items;

    MenuDirection getDirection(void);
    string getMountMenuCaption(void);
};


class MenuBase: public ControlImpl {
    friend class MenuBaseBuilder;
public:
    using OnClickHandler = std::function<void (shared_ptr<MenuBase>)>;
public:
    MenuBase(Control *parent,
            MenuItemType type=MenuItemType::Normal,
            MenuDirection direction=MenuDirection::Vertical,
            float xScale=1.0f,
            float yScale=1.0f
        );
    void setParentMenu(shared_ptr<MenuBase> parentMenu) { m_parentMenu = parentMenu; }
    shared_ptr<MenuBase> getParentMenu(void) const { return m_parentMenu; }
    //void update(void) override;
    void draw(void) override;
    void drawContainer(void);
    bool handleEvent(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override; // 检查菜单栏是否包含指定点
    void setParent(Control *parent) override; // 当被添加到其它控件容器中时，当前控件会被调用这个函数用于设置父控件，此时需要重新设置一下菜单项的位置
    void setRect(SRect rect) override;  // 重载，以便于设置菜单项的矩形区域后，能同步调整菜单标题和子菜单项的位置

    void addItem(shared_ptr<MenuBase> item);
    void removeItem(shared_ptr<MenuBase> item);

    // 鼠标进入/退出处理
    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    // 重载字体颜色设置相关函数，以同步调用Caption相关设置接口
    void setTextStateColor(StateColor stateColor) override;
    void setTextShadowStateColor(StateColor stateColor) override;
    void setTextShadowEnable(bool enable);
    void setCaption(string caption);
    string getCaption(void) const;
    void setCaptionSize(float size);
    uint32_t getCaptionSize(float size) const;
    void setCaptionAlignment(AlignmentMode alignment);
    AlignmentMode getCaptionAlignment(void) const;
    void setCaptionMargin(Margin margin);
    Margin getCaptionMargin(void) const;
    SRect getCaptionRect(void) const;
    void setMenuChainState(ControlState state);
    void closeMenuIntheChain(void); // 关闭当前菜单及其所有父菜单
    shared_ptr<MenuBase> getSubMenuAtPoint(float x, float y); // 获取指定坐标下的子菜单项
    void setType(MenuItemType type) { m_type = type; }
    MenuItemType getType(void) const { return m_type; }
    void setDirection(MenuDirection direction) { m_direction = direction; }
    MenuDirection getDirection(void) const { return m_direction; }
    void setExpended(bool isExpanded) {
        m_isExpanded = isExpanded;
        // setExpendedSubMenuItem(nullptr);
        if (m_menuContainer) {
            if (m_isExpanded) {
                m_menuContainer->show();
            } else {
                m_menuContainer->hide();
            }
        }
    }
    bool getExpanded(void) const { return m_isExpanded; }
    void setExpendedSubMenuItem(shared_ptr<MenuBase> item) {
        if (m_expandedItem == item) return; // 如果展开的子菜单项与要设置的子菜单项相同，则直接返回

        if (m_expandedItem) {
            m_expandedItem->setExpended(false);
        }
        m_expandedItem = item;
        SDL_Log("MenuBase::set <%s>'s expanded item to: 0x%0X", getCaption().c_str(), m_expandedItem);
        if (m_expandedItem) {
            m_expandedItem->setExpended(true);
        }
    }
    shared_ptr<MenuBase> getExpendedSubMenuItem(void) const { return m_expandedItem; }
    void setOnClick(OnClickHandler handler) { m_onClick = handler; }
    shared_ptr<MenuContainer> getMenuContainer(void) const { return m_menuContainer; }

    void setMenuClassLevel(MenuClassLevel level) { m_classLevel = level; }
    MenuClassLevel getMenuClassLevel(void) const { return m_classLevel; }

    void createSubMenuArrowLabel(void); // 创建子菜单箭头
    void destroySubMenuArrowLabel(void); // 销毁子菜单箭头

    // shared_ptr<Label> m_subMenuArrow; // 子菜单箭头
    // shared_ptr<Label> m_caption;
protected:
    shared_ptr<MenuBase> m_parentMenu;
    MenuItemType m_type;
    MenuClassLevel m_classLevel;
    MenuDirection m_direction;
    bool m_isExpanded; // 是否展开子菜单
    SubMenuState m_subMenuState; // 子菜单状态
    // vector<shared_ptr<MenuBase>> m_items;
    shared_ptr<MenuBase> m_expandedItem; // 展开的子菜单项

    shared_ptr<Label> m_caption;
    shared_ptr<Label> m_subMenuArrow; // 子菜单箭头
    // shared_ptr<Label> m_shotcutKey; // 快捷键

    string m_captionText;
    float m_captionSize;
    AlignmentMode m_captionAlignment;
    Margin m_captionMargin;

    bool m_enableTextShadow;

    shared_ptr<MenuContainer> m_menuContainer;

    OnClickHandler m_onClick;

};

class MenuBaseBuilder {
protected:
    shared_ptr<MenuBase> m_menu; // 菜单项对象
public:
    MenuBaseBuilder(shared_ptr<MenuBase> menu,
            MenuItemType type=MenuItemType::Normal,
            MenuDirection direction=MenuDirection::Vertical,
            float xScale=1.0f,
            float yScale=1.0f):
        m_menu(menu)
        {
        SDL_Log("MenuBaseBuilder: Creating MenuBase");
        if (!m_menu) {
            SDL_Log("MenuBaseBuilder::MenuBaseBuilder(): ERROR: input menu is null!");
            throw std::runtime_error("Failed to create MenuBase");
        }
    }
    // virtual MenuBaseBuilder& addBeforeEventHandlingWatcher(EventName eventName) {
    //     EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_menu);
    //     return *this;
    // }
    virtual MenuBaseBuilder& setBackgroundStateColor(StateColor stateColor) {
        m_menu->setBackgroundStateColor(stateColor);
        return *this;
    }
    virtual MenuBaseBuilder& setBorderStateColor(StateColor stateColor) {
        m_menu->setBorderStateColor(stateColor);
        return *this;
    }
    virtual MenuBaseBuilder& setTextStateColor(StateColor stateColor) {
        m_menu->setTextStateColor(stateColor);
        return *this;
    }
    virtual MenuBaseBuilder& setTextShadowStateColor(StateColor stateColor) {
        m_menu->setTextShadowStateColor(stateColor);
        return *this;
    }
    virtual MenuBaseBuilder& setTextShadowEnable(bool enable) {
        m_menu->setTextShadowEnable(enable);
        return *this;
    }
    virtual MenuBaseBuilder& setCaption(string caption) {
        m_menu->setCaption(caption);
        return *this;
    }
    virtual MenuBaseBuilder& setCaptionSize(float size) {
        m_menu->setCaptionSize(size);
        return *this;
    }
    virtual MenuBaseBuilder& setCaptionAlignment(AlignmentMode alignment) {
        m_menu->setCaptionAlignment(alignment);
        return *this;
    }
    virtual MenuBaseBuilder& setCaptionMargin(Margin margin) {
        m_menu->setCaptionMargin(margin);
        return *this;
    }
    virtual MenuBaseBuilder& setOnClick(MenuBase::OnClickHandler handler) {
        m_menu->setOnClick(handler);
        return *this;
    }
    virtual shared_ptr<MenuBase> build(void) {
        m_menu->create();
        return m_menu;
    }
};

class MenuSeparator: public MenuBase {
    friend class MenuSeparatorBuilder;
public:
    MenuSeparator(MenuBase *parent, float xScale=1.0f, float yScale=1.0f):
        MenuBase(parent, MenuItemType::Separator, MenuDirection::Horizontal, xScale, yScale)
    {}

    // void update(void) override;
    void draw(void) override {
        if (getVisible() == false) return;
        if (getParent() == nullptr) return;

        SDL_Renderer* renderer = getRenderer();
        if (!renderer) return;

        MenuBase::draw();

        SRect drawRect = getDrawRect();

        // 绘制分隔线，使用边框颜色
        SDL_SetRenderDrawColor(renderer,
            m_borderColor.getNormal().r,
            m_borderColor.getNormal().g,
            m_borderColor.getNormal().b,
            m_borderColor.getNormal().a);
        SDL_FRect lineRect;
        if (getDirection() == MenuDirection::Vertical) {
            // 竖向菜单，绘制横向分隔线
            lineRect = {drawRect.left - ConstDef::MENU_ITEM_MARGIN.left,
                                    drawRect.top + drawRect.height/2,
                                    drawRect.width + ConstDef::MENU_ITEM_MARGIN.left + ConstDef::MENU_ITEM_MARGIN.right,
                                    1
                                };
        } else {
            // 水平菜单，绘制纵向分隔线
            lineRect = {drawRect.left + drawRect.width/2,
                                    drawRect.top - ConstDef::MENU_ITEM_MARGIN.top,
                                    1,
                                    drawRect.height + ConstDef::MENU_ITEM_MARGIN.top + ConstDef::MENU_ITEM_MARGIN.bottom
                                };
        }
        SDL_RenderFillRect(renderer, &lineRect);
    }
};

class MenuSeparatorBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MenuSeparator> m_separator; // 菜单项对象
public:
    MenuSeparatorBuilder(float xScale=1.0f, float yScale=1.0f, Control *parent=nullptr):
        MenuBaseBuilder(
            // 通过lambda表达式创建MenuSeparator实例，保证parent类型正确
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
            MenuItemType::Separator, MenuDirection::Horizontal, xScale, yScale){
        m_separator = dynamic_pointer_cast<MenuSeparator>(m_menu);
    }
    shared_ptr<MenuBase> build(void) {
        m_separator->create();
        return m_separator;
    }
};

class MenuBar: public MenuBase {
private:
    bool m_showTopSubMenu; // 是否显示首层子菜单
public:
    MenuBar(Control *parent, MenuDirection direction=MenuDirection::Horizontal, float xScale=1.0f, float yScale=1.0f):
        MenuBase(parent, MenuItemType::Normal, direction, xScale, yScale),
        m_showTopSubMenu(false)
    {
        setCaption(""); // 对于MenuBar来说，设置标题为空，不显示
        if (parent != nullptr) {
            setRect(SRect(0, 0, parent->getRect().width, 30)); // 默认宽与父对像相同，高30
        }
        setBorderVisible(true);
        setExpended(true); // 默认显示菜单栏上的菜单项
        setMenuClassLevel(MenuClassLevel::MenuBar);
    }
    void setShowTopSubMenu(bool show) {
        m_showTopSubMenu = show;
        if (show == false) {
            setExpendedSubMenuItem(nullptr); // 隐藏首层子菜单
        }
    }
    bool getShowTopSubMenu(void) const {
        return m_showTopSubMenu;
    }
    void draw(void) override {
        MenuBase::draw();
        drawContainer();
    }
    void setParent(Control *parent) override {
        MenuBase::setParent(parent);
        if (parent != nullptr) {
            setRect(SRect(0, 0, parent->getRect().width, getRect().height)); // 宽度调整为与父对象相同，高度保持不变
        }
    }
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override {
        shared_ptr<SPoint> pos;
        SRect drawRect;
        switch(event->m_eventName){
            case EventName::MOUSE_LBUTTON_DOWN:
                pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (!isContainsPoint(pos->x, pos->y)) {
                    // 点击了菜单栏以外的区域，收起菜单
                    if(getExpendedSubMenuItem() != nullptr){
                        getExpendedSubMenuItem()->setExpended(false);
                        setExpendedSubMenuItem(nullptr);
                    }
                }
                break;
            case EventName::MOUSE_MOVING:
                drawRect = getDrawRect(); // 获取当前控件的绘制区域
                pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (!drawRect.contains(pos->x, pos->y)){
                    // 移动到菜单栏外，状态变更为Normal
                    setState(ControlState::Normal);
                }
                break;
            default:
                break;
        }
        return false;
    }
    void addMainMenu(shared_ptr<MenuBase> item) {
        if (!item) {
            SDL_Log("MenuBar::addMainMenu item is nullptr");
            throw std::runtime_error("MenuBar::addMainMenu item is nullptr");
        }

        if (m_menuContainer == nullptr) {
            setType(MenuItemType::SubMenu); // 有子菜单时，类型改为SubMenu
            m_menuContainer = make_shared<MenuContainer>(this); // 创建子菜单容器
            // m_menuContainer->setVisible(true);
            m_menuContainer->create(); // 创建子菜单容器
            if (getDirection() == MenuDirection::Horizontal) {
                // 水平菜单，子菜单容器为水平
                m_menuContainer->setRect({0, 0, 0, 0});
            } else {
                // 竖向菜单，子菜单容器为竖向
                SDL_Log("MainMenu::addMenuItem getRect().height = %f", getRect().height);
                m_menuContainer->setRect({0, 0, 0, 0});
            }
        }
        addItem(item);
        // 修改MenuBar的大小与其MenuContainer一致，避免显示时出现空白
        setRect({getRect().left, getRect().top, m_menuContainer->getRect().width, m_menuContainer->getRect().height});
        SDL_Log("MenuBar::addMainMenu rect = {%f, %f, %f, %f}", getRect().left, getRect().top, getRect().width, getRect().height);
    }
    void removeMainMenu(shared_ptr<MenuBase> item){
        if (!item) {
            SDL_Log("MenuBar::removeMainMenu item is nullptr");
            throw std::runtime_error("MenuBar::removeMainMenu item is nullptr");
        }
        removeItem(item);
        if (m_menuContainer == nullptr) {
            setType(MenuItemType::Normal); // 没有子菜单时，类型改为Normal
            if(getParent() != nullptr)
                setRect({getRect().left, getRect().top, getParent()->getRect().width, getParent()->getRect().height});
        } else {
            // 修改MenuBar的大小与其MenuContainer一致，避免显示时出现空白
            setRect({getRect().left, getRect().top, m_menuContainer->getRect().width, m_menuContainer->getRect().height});
        }
    }
};

class MenuBarBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MenuBar> m_menuBar; // 菜单栏对象
public:
    MenuBarBuilder(Control *parent=nullptr, MenuDirection direction=MenuDirection::Horizontal, float xScale=1.0f, float yScale=1.0f):
        MenuBaseBuilder(
            make_shared<MenuBar>(parent, direction, xScale, yScale),
            MenuItemType::Normal, direction, xScale, yScale)
    {
        m_menuBar = dynamic_pointer_cast<MenuBar>(m_menu);
        m_menuBar->setCaption("MenuBar");
        SDL_Log("MenuBarBuilder: Created MenuBar with rect {%f, %f, %f, %f}", m_menuBar->getRect().left, m_menuBar->getRect().top, m_menuBar->getRect().width, m_menuBar->getRect().height);
    }
    MenuBarBuilder& addMainMenu(shared_ptr<MenuBase> item) {
        m_menuBar->addMainMenu(item);
        return *this;
    }
    MenuBarBuilder& addBeforeEventHandlingWatcher(EventName eventName) {
        EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_menuBar);
        return *this;
    }
    shared_ptr<MenuBase> build(void) {
        m_menuBar->create();
        return m_menuBar;
    }
};

class MainMenu: public MenuBase {
    friend class MainMenuBuilder;
public:
    MainMenu(Control *parent,
            MenuDirection direction=MenuDirection::Vertical,
            float xScale=1.0f,
            float yScale=1.0f
        ):
        MenuBase(parent, MenuItemType::Normal, direction, xScale, yScale)
    {
        SDL_Log("ManMenu::MainMenu direction = %d", getDirection());
        setCaptionAlignment(AlignmentMode::AM_CENTER); // 设置标题居中对齐
        setMenuClassLevel(MenuClassLevel::MainMenu);
    }
    bool handleEvent(shared_ptr<Event> event) override{
        SRect drawRect;
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
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override {
        shared_ptr<SPoint> pos;
        SRect drawRect;
        switch(event->m_eventName){
            case EventName::MOUSE_MOVING:
                drawRect = getDrawRect(); // 获取当前控件的绘制区域
                pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (!drawRect.contains(pos->x, pos->y)){
                    // 移动到菜单栏外，状态变更为Normal
                    setState(ControlState::Normal);
                }
                break;
            default:
                break;
        }
        return false;
    }
    void addMenuItem(shared_ptr<MenuBase> item){
        if (m_menuContainer == nullptr) {
            setType(MenuItemType::SubMenu); // 有子菜单时，类型改为SubMenu
            m_menuContainer = make_shared<MenuContainer>(this); // 创建子菜单容器

            if (getDirection() == MenuDirection::Horizontal) {
                // 水平菜单，子菜单容器为水平
                // 子菜单做了偏移处理，为了对齐边线，这里需要调整回来
                m_menuContainer->setRect({getRect().width - ConstDef::MENU_ITEM_MARGIN.left, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
            } else {
                // 竖向菜单，子菜单容器为竖向
                SDL_Log("MainMenu::addMenuItem getRect().height = %f", getRect().height);
                // 子菜单做了偏移处理，为了对齐边线，这里需要调整回来
                m_menuContainer->setRect({0 - ConstDef::MENU_ITEM_MARGIN.left, getRect().height - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
            }
            m_menuContainer->create(); // 创建子菜单容器
            m_menuContainer->setVisible(false); // 默认隐藏子菜单，鼠标移入时显示

            SDL_Log("MainMenu::addMenuItem rect = {%f, %f, %f, %f}", getRect().left, getRect().top, getRect().width, getRect().height);
        }
        addItem(item);

    }
    void removeMenuItem(shared_ptr<MenuBase> item){
        removeItem(item);
        if (m_menuContainer == nullptr) {
            setType(MenuItemType::Normal); // 没有子菜单时，类型改为Normal
        }
    }
};

class MainMenuBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MainMenu> m_mainMenu; // 菜单项对象
public:
    MainMenuBuilder(string caption, MenuDirection direction=MenuDirection::Vertical, float xScale=1.0f, float yScale=1.0f, Control *parent=nullptr):
        MenuBaseBuilder(
            // 通过lambda表达式创建MainMenu实例，保证parent类型正确
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
    MainMenuBuilder& addBeforeEventHandlingWatcher(EventName eventName) {
        EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_mainMenu);
        return *this;
    }
    MainMenuBuilder& addMenuItem(shared_ptr<MenuBase> item) {
        m_mainMenu->addMenuItem(item);
        return *this;
    }
    shared_ptr<MenuBase> build(void) override{
        if (!m_mainMenu) {
            SDL_Log("MainMenuBuilder::build: Failed to cast MenuBase to MainMenu");
            throw std::runtime_error("MainMenu is not properly initialized");
        }
        m_mainMenu->create();
        return m_mainMenu;
    }
};

class MenuItem: public MenuBase {
    friend class MenuItemBuilder;
public:
    MenuItem(Control *parent,
            MenuDirection direction=MenuDirection::Vertical,
            float xScale=1.0f,
            float yScale=1.0f
        ):
        MenuBase(parent, MenuItemType::Normal, direction, xScale, yScale)
    {
        setMenuClassLevel(MenuClassLevel::MenuItem);
    }
    // void update(void) override;
    // void draw(void) override;
    // bool handleEvent(shared_ptr<Event> event) override; // 处理事件
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override {
        shared_ptr<SPoint> pos;
        SRect drawRect;
        switch(event->m_eventName){
            case EventName::MOUSE_MOVING:
                drawRect = getDrawRect(); // 获取当前控件的绘制区域
                pos = any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (!drawRect.contains(pos->x, pos->y)){
                    // 移动到菜单栏外，状态变更为Normal
                    setState(ControlState::Normal);
                    setExpended(false); // 收起子菜单
                }
                break;
            default:
                break;
        }
        return false;
    }
    void addSubMenuItem(shared_ptr<MenuBase> item){
        if (m_menuContainer == nullptr) {
            setType(MenuItemType::SubMenu); // 有子菜单时，类型改为SubMenu
            m_menuContainer = make_shared<MenuContainer>(this); // 创建子菜单容器

            if (getDirection() == MenuDirection::Horizontal) {
                // 水平菜单，子菜单容器为竖向，排在下方或上方
                // TODO: 这里暂时只支持下方展开，上方展开需要调整子菜单容器的位置
                // 子菜单做了偏移处理，为了对齐边线，这里需要调整回来
                m_menuContainer->setRect({0 - ConstDef::MENU_ITEM_MARGIN.left, getRect().height - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
            } else {
                // 竖向菜单，子菜单容器为水平，排在右侧或左侧
                // TODO: 这里暂时只支持右侧展开，左侧展开需要调整子菜单容器的位置
                // 子菜单做了偏移处理，为了对齐边线，这里需要调整回来
                // m_menuContainer->setRect({getRect().width - ConstDef::MENU_ITEM_MARGIN.left, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
                m_menuContainer->setRect({getRect().width, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
            }
            SDL_Log("MenuItem(%s)::new container rect = {%f, %f, %f, %f}, direction=%s", getCaption().c_str(),
                m_menuContainer->getRect().left,
                m_menuContainer->getRect().top,
                m_menuContainer->getRect().width,
                m_menuContainer->getRect().height,
                (getDirection() == MenuDirection::Horizontal) ? "Horizontal" : "Vertical"
            );
            createSubMenuArrowLabel(); // 创建子菜单箭头
            // 创建子菜单箭头后，需要再次调整子菜单容器的位置，以确保箭头和子菜单之间的间距正确
            if (getDirection() == MenuDirection::Horizontal) {
                m_menuContainer->setRect({0 - ConstDef::MENU_ITEM_MARGIN.left, getRect().height - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
            } else {
                m_menuContainer->setRect({getRect().width + ConstDef::MENU_ITEM_MARGIN.left, 0 - ConstDef::MENU_ITEM_MARGIN.top, 0, 0});
            }

            m_menuContainer->create();
            m_menuContainer->setVisible(false); // 默认隐藏子菜单，鼠标移入时显示
        }
        addItem(item);
    }
    void removeSubMenuItem(shared_ptr<MenuBase> item) {
        removeItem(item);
        if (m_menuContainer == nullptr) {
            setType(MenuItemType::Normal); // 没有子菜单时，类型改为Normal
            destroySubMenuArrowLabel(); // 销毁子菜单箭头
        }
    }
};

class MenuItemBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MenuItem> m_menuItem; // 菜单项对象
public:
    MenuItemBuilder(string caption, MenuDirection direction=MenuDirection::Vertical, float xScale=1.0f, float yScale=1.0f, Control *parent=nullptr):
        MenuBaseBuilder(
            // 通过lambda表达式创建MenuItem实例，保证parent类型正确
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
    MenuItemBuilder& addBeforeEventHandlingWatcher(EventName eventName) {
        EventQueue::getInstance()->addBeforeEventHandlingWatcher(eventName, m_menuItem);
        return *this;
    }
    MenuItemBuilder& addSubMenuItem(shared_ptr<MenuBase> item) {
        m_menuItem->addSubMenuItem(item);
        return *this;
    }
    shared_ptr<MenuBase> build(void) {
        m_menuItem->create();
        return m_menuItem;
    }
};
#endif  // MenuH