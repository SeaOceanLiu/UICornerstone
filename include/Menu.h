#ifndef MenuH
#define MenuH

#include <memory>
#include <vector>
#include <functional>
#include <string>

#include <SDL3/SDL.h>

#include "ControlBase.h"
#include "Label.h"
#include "GraphTool.h"

using namespace std;

// ==================== 菜单项类型枚举 ====================
enum class MenuItemType {
    Normal,     // 普通菜单项，点击后执行操作
    Separator,  // 分隔线
    SubMenu     // 子菜单项，有下级菜单
};

// ==================== 前向声明 ====================
class MenuItem;
class MenuPanel;
class MenuBar;

// ==================== MenuItem 菜单项 ====================
class MenuItem : public ControlImpl {
    friend class MenuItemBuilder;
    friend class MenuPanel;
public:
    using OnClickHandler = function<void(shared_ptr<MenuItem>)>;

    MenuItem(Control *parent, MenuItemType type = MenuItemType::Normal,
             float xScale = 1.0f, float yScale = 1.0f);
    ~MenuItem() override;

    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;

    // 属性设置
    void setCaption(const string& caption);
    const string& getCaption() const { return m_caption; }
    void setShortcut(const string& shortcut);
    const string& getShortcut() const { return m_shortcut; }
    void setChecked(bool checked);
    bool getChecked() const { return m_checked; }

    // 点击回调
    void setOnClick(OnClickHandler handler) { m_onClick = handler; }

    // 子菜单
    void setSubMenu(shared_ptr<MenuPanel> panel);
    shared_ptr<MenuPanel> getSubMenu() const { return m_subMenu; }
    bool hasSubMenu() const { return m_subMenu != nullptr; }

    MenuItemType getType() const { return m_type; }

    // 菜单项高度（用于布局计算）
    static float getItemHeight();

    // 关闭整个菜单链
    void closeMenuChain();

private:
    MenuItemType m_type;
    string m_caption;
    string m_shortcut;
    bool m_checked;
    bool m_hovered;
    OnClickHandler m_onClick;
    shared_ptr<MenuPanel> m_subMenu;
    shared_ptr<Label> m_captionLabel;
    shared_ptr<Label> m_shortcutLabel;
    shared_ptr<Label> m_arrowLabel;

    void createLabels();
    void updateLabelPositions();
};

// ==================== MenuPanel 菜单面板 ====================
class MenuPanel : public ControlImpl {
    friend class MenuPanelBuilder;
    friend class MenuBar;
public:
    MenuPanel(Control *parent, float xScale = 1.0f, float yScale = 1.0f);
    ~MenuPanel() override;

    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override;

    // 添加菜单项
    void addItem(shared_ptr<MenuItem> item);
    void addSeparator();
    void removeItem(shared_ptr<MenuItem> item);

    // 显示/隐藏
    void show();
    void hide();
    bool isVisible() const { return m_visible; }

    // 设置位置（屏幕坐标）
    void setPosition(float x, float y);

    // 获取菜单项
    shared_ptr<MenuItem> getItemAt(float x, float y);

    // 关闭本面板及子面板
    void closeWithChildren();

    // 重新计算尺寸
    void recalculateSize();

    // 获取子菜单面板
    shared_ptr<MenuPanel> getOpenSubMenu() const { return m_openSubMenu; }
    void setOpenSubMenu(shared_ptr<MenuPanel> panel);

    int getHoveredIndex() const { return m_hoveredIndex; }
    void setHoveredIndex(int index);

private:
    vector<shared_ptr<MenuItem>> m_items;
    float m_itemHeight;
    float m_iconAreaWidth;
    float m_shortcutAreaWidth;
    float m_arrowAreaWidth;
    int m_hoveredIndex;
    bool m_visible;
    shared_ptr<MenuPanel> m_openSubMenu;

    // 颜色
    SDL_Color m_bgColor;
    SDL_Color m_borderColor;
    SDL_Color m_hoverColor;
    SDL_Color m_separatorColor;
    float m_shadowRadius;

    void layoutItems();
    int hitTest(float x, float y);
    void drawShadow();
};

// ==================== MenuBar 菜单栏 ====================
class MenuBar : public ControlImpl {
    friend class MenuBarBuilder;
public:
    MenuBar(Control *parent, float xScale = 1.0f, float yScale = 1.0f);
    ~MenuBar() override;

    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override;
    void setParent(Control *parent) override;
    void setRect(SRect rect) override;

    // 添加顶级菜单
    void addMenu(const string& caption, shared_ptr<MenuPanel> panel);
    void removeMenu(const string& caption);

    // 关闭所有下拉菜单
    void closeAllMenus();

    // 菜单栏高度
    void setBarHeight(float height);
    float getBarHeight() const { return m_barHeight; }

    // 菜单模式
    bool isInMenuMode() const { return m_menuMode; }
    void enterMenuMode(int index);
    void exitMenuMode();

private:
    struct MenuEntry {
        string caption;
        SRect hitRect;
        shared_ptr<MenuPanel> panel;
        shared_ptr<Label> label;
    };

    vector<MenuEntry> m_entries;
    float m_barHeight;
    int m_hoveredIndex;
    int m_activeIndex;
    bool m_menuMode;

    // 颜色
    SDL_Color m_bgColor;
    SDL_Color m_textColor;
    SDL_Color m_hoverBgColor;
    SDL_Color m_hoverTextColor;
    SDL_Color m_activeBgColor;

    void layoutEntries();
    int hitTest(float x, float y);
    void openMenu(int index);
    void switchMenu(int index);
};

// ==================== Builder模式 ====================

class MenuItemBuilder {
public:
    MenuItemBuilder(const string& caption, float xScale = 1.0f, float yScale = 1.0f);
    ~MenuItemBuilder() = default;

    MenuItemBuilder& setShortcut(const string& shortcut);
    MenuItemBuilder& setOnClick(MenuItem::OnClickHandler handler);
    MenuItemBuilder& setChecked(bool checked);
    MenuItemBuilder& setSubMenu(shared_ptr<MenuPanel> panel);
    MenuItemBuilder& setEnabled(bool enabled);
    MenuItemBuilder& setBackgroundStateColor(StateColor stateColor);
    MenuItemBuilder& setTextStateColor(StateColor stateColor);

    shared_ptr<MenuItem> build();

private:
    shared_ptr<MenuItem> m_item;
};

class MenuPanelBuilder {
public:
    MenuPanelBuilder(float xScale = 1.0f, float yScale = 1.0f);
    ~MenuPanelBuilder() = default;

    MenuPanelBuilder& addItem(shared_ptr<MenuItem> item);
    MenuPanelBuilder& addSeparator();

    shared_ptr<MenuPanel> build();

private:
    shared_ptr<MenuPanel> m_panel;
    vector<shared_ptr<MenuItem>> m_items;
};

class MenuBarBuilder {
public:
    MenuBarBuilder(Control *parent = nullptr, float xScale = 1.0f, float yScale = 1.0f);
    ~MenuBarBuilder() = default;

    MenuBarBuilder& addMenu(const string& caption, shared_ptr<MenuPanel> panel);
    MenuBarBuilder& setBarHeight(float height);
    MenuBarBuilder& setBackgroundStateColor(StateColor stateColor);
    MenuBarBuilder& setTextStateColor(StateColor stateColor);

    shared_ptr<MenuBar> build();

private:
    shared_ptr<MenuBar> m_menuBar;
};

#endif // MenuH
