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
    void draw(void) override;
};

class MenuSeparatorBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MenuSeparator> m_separator; // 菜单项对象
public:
    MenuSeparatorBuilder(float xScale=1.0f, float yScale=1.0f, Control *parent=nullptr);
    shared_ptr<MenuBase> build(void) override;
};

class MenuBar: public MenuBase {
private:
    bool m_showTopSubMenu;
public:
    MenuBar(Control *parent, MenuDirection direction=MenuDirection::Horizontal, float xScale=1.0f, float yScale=1.0f);
    void setShowTopSubMenu(bool show);
    bool getShowTopSubMenu(void) const;
    void draw(void) override;
    void setParent(Control *parent) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void addMainMenu(shared_ptr<MenuBase> item);
    void removeMainMenu(shared_ptr<MenuBase> item);
};

class MenuBarBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MenuBar> m_menuBar; // 菜单栏对象
public:
    MenuBarBuilder(Control *parent=nullptr, MenuDirection direction=MenuDirection::Horizontal, float xScale=1.0f, float yScale=1.0f);
    MenuBarBuilder& addMainMenu(shared_ptr<MenuBase> item);
    MenuBarBuilder& addBeforeEventHandlingWatcher(EventName eventName);
    shared_ptr<MenuBase> build(void) override;
};

class MainMenu: public MenuBase {
    friend class MainMenuBuilder;
public:
    MainMenu(Control *parent,
            MenuDirection direction=MenuDirection::Vertical,
            float xScale=1.0f,
            float yScale=1.0f
        );

    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void addMenuItem(shared_ptr<MenuBase> item);
    void removeMenuItem(shared_ptr<MenuBase> item);
};

class MainMenuBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MainMenu> m_mainMenu; // 菜单项对象
public:
    MainMenuBuilder(string caption, MenuDirection direction=MenuDirection::Vertical, float xScale=1.0f, float yScale=1.0f, Control *parent=nullptr);
    MainMenuBuilder& addBeforeEventHandlingWatcher(EventName eventName);
    MainMenuBuilder& addMenuItem(shared_ptr<MenuBase> item);
    shared_ptr<MenuBase> build(void) override;
};

class MenuItem: public MenuBase {
    friend class MenuItemBuilder;
public:
    MenuItem(Control *parent,
            MenuDirection direction=MenuDirection::Vertical,
            float xScale=1.0f,
            float yScale=1.0f
        );

    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void addSubMenuItem(shared_ptr<MenuBase> item);
    void removeSubMenuItem(shared_ptr<MenuBase> item);
};

class MenuItemBuilder: public MenuBaseBuilder {
private:
    shared_ptr<MenuItem> m_menuItem; // 菜单项对象
public:
    MenuItemBuilder(string caption, MenuDirection direction=MenuDirection::Vertical, float xScale=1.0f, float yScale=1.0f, Control *parent=nullptr);
    MenuItemBuilder& addBeforeEventHandlingWatcher(EventName eventName);
    MenuItemBuilder& addSubMenuItem(shared_ptr<MenuBase> item);
    shared_ptr<MenuBase> build(void) override;
};
#endif  // MenuH