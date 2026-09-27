// 由AI(GLM 5.1)生成，可能不完整或有错误，请自行检查和修改
#ifndef MenuH
#define MenuH

#include <memory>
#include <vector>
#include <functional>
#include <string>

#include "ControlBase.h"
#include "Font.h"
#include "GraphTool.h"
#include "LeadingControlSlot.h"

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
class ContextMenu;

// ==================== MenuItem 菜单项 ====================
class MenuItem : public ControlImpl {
    friend class MenuItemBuilder;
    friend class MenuPanel;
public:
    using OnClickHandler = function<void(shared_ptr<MenuItem>)>;

    MenuItem(Control *parent, MenuItemType type = MenuItemType::Normal,
             float xScale = 1.0f, float yScale = 1.0f);
    ~MenuItem() override;

    void create() override;
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
    OnClickHandler getOnClick() const { return m_onClick; }

    // 子菜单
    void setSubMenu(shared_ptr<MenuPanel> panel);
    shared_ptr<MenuPanel> getSubMenu() const { return m_subMenu; }
    bool hasSubMenu() const { return m_subMenu != nullptr; }

    MenuItemType getType() const { return m_type; }

    // 由所属 MenuPanel 注入共享字体（绘制文本用）
    void setMenuFont(SharedFont font, float fontSize);
    Font* getFont() const { return m_font.get(); }
    float getFontSize() const { return m_fontSize; }

    // Menu 增强：前置控件容器 + 逐 Item 字体
    void setLeadingControl(shared_ptr<Control> ctl);
    shared_ptr<Control> getLeadingControl() const {
        return (leading && leading->hasControl()) ? leading->getControl() : nullptr;
    }
    void setLeadingGap(float gap) { ensureLeading(); leading->setGap(gap); }
    float getLeadingGap() const { return leading ? leading->getGap() : 8.0f; }
    void setFontName(FontName fn) { fontName = fn; }
    FontName getFontName() const { return fontName; }
    void setOwnFontSize(int size) { fontSize = size; }
    int getOwnFontSize() const { return fontSize; }
    void setItemId(const string& id) { m_itemId = id; }
    const string& getItemId() const { return m_itemId; }
    // fontSize>0 时按自身 fontName/fontSize 重建字体（加载失败保持现状回退）
    void ensureOwnFont();
    // 惰性创建 leading 组件
    void ensureLeading();

    // 关闭整个菜单链
    void closeMenuChain();

    // ── Property system overrides ──
    int setBoolProperty(const char* prop, int value) override;
    int setStringProperty(const char* prop, const char* value) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;

private:
    MenuItemType m_type;
    string m_caption;
    string m_shortcut;
    bool m_checked;
    OnClickHandler m_onClick;
    shared_ptr<MenuPanel> m_subMenu;
    SharedFont m_font;
    float m_fontSize;

    // Menu 增强：前置控件统一由 LeadingControlSlot 组件承载（挂载/对齐/间隙/命中，
    // 见 LeadingControlSlot.h，供 TreeView/Menu/未来 ListView 等复用）
    shared_ptr<LeadingControlSlot> leading;
    FontName fontName = FontName::MapleMono_NF_CN_Regular; // 与面板默认一致；仅 fontSize>0 时生效
    int fontSize = 0;                                     // 0 = 继承面板级字号
    string m_itemId;                                      // CABI item-id 定位（第二期）
};

// ==================== MenuPanel 菜单面板 ====================
class MenuPanel : public ControlImpl {
    friend class MenuPanelBuilder;
    friend class MenuBar;
    friend class ContextMenu;
public:
    MenuPanel(Control *parent, float xScale = 1.0f, float yScale = 1.0f);
    ~MenuPanel() override;

    void setContext(UIContext* ctx) override;
    void setParent(Control* parent) override;
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override;
    void refreshScaleWith(float parentXX, float parentYY) override;

    // 添加菜单项
    void addItem(shared_ptr<MenuItem> item);
    void addSeparator();
    void removeItem(shared_ptr<MenuItem> item);

    // 显示/隐藏
    void show();
    void hide();
    bool isVisible() const { return m_visible; }

    // 设置位置（相对父控件坐标，由 MenuBar::openMenu 换算）
    void setPosition(float x, float y);

    // 获取菜单项
    shared_ptr<MenuItem> getItemAt(float x, float y);

    // 菜单项只读访问（StatusBar 弹窗补 no-op onClick 等场景）
    const vector<shared_ptr<MenuItem>>& getItems() const { return m_items; }

    // 关闭本面板及子面板
    void closeWithChildren();

    // 重新计算尺寸
    void recalculateSize();

    // 命中测试：输入绘制（像素）坐标，返回条目索引（未命中 -1）
    int hitTest(float x, float y);

    // 获取子菜单面板
    shared_ptr<MenuPanel> getOpenSubMenu() const { return m_openSubMenu; }
    void setOpenSubMenu(shared_ptr<MenuPanel> panel);

    int getHoveredIndex() const { return m_hoveredIndex; }
    void setHoveredIndex(int index);

    // 字体/尺寸（实例状态，修改后即时生效）
    void setFontSize(float size);
    float getFontSize() const { return m_fontSize; }
    Font* getFont() const { return m_font.get(); }
    void setItemHeightRatio(float ratio);
    float getItemHeightRatio() const { return m_heightRatio; }
    void setFontName(FontName fontName);

    // Menu 增强：icon 区宽（max(各行字体高度, 20)，实例状态，随逐项字体变化；
    // 面板内没有任何 leadingControl 时 = 0，不预留 icon 区空间）
    float getIconAreaWidth() const { return m_hasLeadingControl ? m_iconAreaWidth : 0; }
    // 面板内是否存在 leadingControl（决定是否预留 icon 区）
    bool hasLeadingControl() const { return m_hasLeadingControl; }
    // 行高（变行高）：Separator = 分隔线高；其余 = 生效字号 × heightRatio
    float itemRowHeight(MenuItem* item) const;
    // 按 item-id 定位（CABI 属性分发用；getItemAt 为位置定位）
    shared_ptr<MenuItem> getItemById(const string& id) const;

    // P0-26：item 文本色/阴影访问器（MenuItem::draw 读取）
    SColor getItemTextColor() const { return m_textColor; }
    SColor getItemHoverTextColor() const { return m_hoverTextColor; }
    SColor getItemDisabledTextColor() const { return m_disabledTextColor; }
    StateColor& getItemTextShadowState() { return m_textShadowColor; }
    bool   isTextShadowEnabled() const { return m_shadowEnabled; }
    SPoint getTextShadowOffset() const { return m_shadowOffset; }
    bool   isItemHovered(const MenuItem* item) const;

    // 属性系统（CABI item-id 定位 + item-leading-* 属性，TreeView v7 同模式）
    void setTextStateColor(StateColor stateColor) override;        // P0-26：文本三态（normal/hover/disabled）
    void setTextShadowStateColor(StateColor stateColor) override;  // P0-26：阴影色三态
    int setColorProperty(const char* prop, SColor color) override;
    int getColorProperty(const char* prop, SColor& out) override;
    int setBoolProperty(const char* prop, int value) override;
    int getBoolProperty(const char* prop, int& out) override;
    int setIntProperty(const char* prop, int value) override;
    int getIntProperty(const char* prop, int& out) override;
    int setStringProperty(const char* prop, const char* value) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int setFloatProperty(const char* prop, float value) override;
    int getFloatProperty(const char* prop, float& out) override;
    int setEnumProperty(const char* prop, const char* value) override;
    int getEnumProperty(const char* prop, const char*& out) override;
    int setPtrProperty(const char* prop, void* value) override;
    int getPtrProperty(const char* prop, void*& out) override;

private:
    vector<shared_ptr<MenuItem>> m_items;
    float m_fontSize;
    float m_heightRatio;
    float m_itemHeight;
    float m_iconAreaWidth;
    bool m_hasLeadingControl;
    float m_shortcutAreaWidth;
    float m_arrowAreaWidth;
    int m_hoveredIndex;
    bool m_visible;
    shared_ptr<MenuPanel> m_openSubMenu;

    // 字体（面板内所有菜单项共享）
    SharedFont m_font;
    FontName m_fontName;

    // 颜色
    SColor m_bgColor;
    SColor m_borderColor;
    SColor m_hoverColor;
    SColor m_separatorColor;
    SColor m_textColor;
    SColor m_hoverTextColor;
    SColor m_disabledTextColor;
    StateColor m_textShadowColor;
    bool   m_shadowEnabled = false;
    SPoint m_shadowOffset{1, 1};
    float m_shadowRadius;

    void ensureFont();
    void updateItemsFont();
    void layoutItems();
    void drawShadow();
    string m_itemTargetId;  // CABI "item-id" 定位：item 级属性的作用目标
};

// ==================== MenuBar 菜单栏 ====================
class MenuBar : public ControlImpl {
    friend class MenuBarBuilder;
public:
    MenuBar(Control *parent, float xScale = 1.0f, float yScale = 1.0f);
    ~MenuBar() override;

    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    bool isContainsPoint(float x, float y) override;
    void setParent(Control *parent) override;
    void setRect(SRect rect) override;
    void setContext(UIContext* ctx) override;
    void refreshScaleWith(float parentXX, float parentYY) override;

    // 添加顶级菜单
    void addMenu(const string& caption, shared_ptr<MenuPanel> panel);
    shared_ptr<MenuPanel> getMenuPanel(int index) const;  // 第 index 个菜单面板（越界返回 nullptr）
    void removeMenu(const string& caption);

    // 关闭所有下拉菜单
    void closeAllMenus();

    // 菜单栏高度
    void setBarHeight(float height);
    float getBarHeight() const { return m_barHeight; }

    // 手动定位模式：默认 false = 全宽布局（宽=父宽、top=0，setRect 被 layoutEntries 覆盖）；
    // 开启后 setRect 自由生效（同屏多 MenuBar / 缩放对比测试用），条目标题 hitRect 仍按
    // 0 起点排布（条目区域 = 标题宽 + 2×padding，缩放时随绘制矩形放大）
    void setManualPosition(bool on) { m_manualPosition = on; }
    bool getManualPosition() const { return m_manualPosition; }

    // 设置菜单项高度与字体大小的比例系数（范围 1.0 ~ 3.0）
    void setItemHeightRatio(float ratio);
    float getItemHeightRatio() const { return m_itemHeightRatio; }

    // 设置菜单字体大小
    void setFontSize(float size);
    float getFontSize() const { return m_menuTextSize; }
    Font* getFont() const { return m_font.get(); }

    // 菜单模式
    bool isInMenuMode() const { return m_menuMode; }
    void enterMenuMode(int index);
    void exitMenuMode();

    // 命中测试：输入绘制（像素）坐标，返回条目索引（未命中 -1）
    int hitTest(float x, float y);

private:
    struct MenuEntry {
        string caption;
        SRect hitRect;
        shared_ptr<MenuPanel> panel;
    };

    // 下拉面板顶层挂载：打开时挂 BENCH（位于所有控件之上），关闭时摘回父链
    void attachMenuPanel(int index);
    void detachMenuPanel(int index);

    // 全局 watcher：菜单打开期间注册，点击/ESC 在任何控件消费事件之前先关闭（Popup 同款）
    void registerMenuWatcher();
    void unregisterMenuWatcher();
    bool m_watcherRegistered = false;

    vector<MenuEntry> m_entries;
    float m_barHeight;
    int m_hoveredIndex;
    int m_activeIndex;
    bool m_menuMode;
    bool m_manualPosition = false;

    // 颜色
    SColor m_bgColor;
    SColor m_textColor;
    SColor m_hoverBgColor;
    SColor m_hoverTextColor;
    SColor m_activeBgColor;
    SColor m_borderColor;
    SColor m_disabledTextColor;
    StateColor m_textShadowColor;
    bool   m_shadowEnabled = false;
    SPoint m_shadowOffset{1, 1};

    float m_itemHeightRatio;
    float m_menuTextSize;
    SharedFont m_font;
    FontName m_fontName;

    void ensureFont();
    void layoutEntries();
    void openMenu(int index);
    void switchMenu(int index);
public:
    // ── Property system overrides ──
    void setTextStateColor(StateColor stateColor) override;        // P0-26：文本三态 → 自身 + 面板
    void setTextShadowStateColor(StateColor stateColor) override;  // P0-26
    int setColorProperty(const char* prop, SColor color) override;
    int getColorProperty(const char* prop, SColor& out) override;
    int setBoolProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setEnumProperty(const char* prop, const char* value) override;

    int getBoolProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getEnumProperty(const char* prop, const char*& out) override;
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
    MenuItemBuilder& setLeadingControl(shared_ptr<Control> ctl);
    MenuItemBuilder& setLeadingGap(float gap);
    MenuItemBuilder& setFontName(FontName fn);
    MenuItemBuilder& setFontSize(int size);
    MenuItemBuilder& setItemId(const string& id);

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
