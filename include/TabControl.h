// ============================================================================
// TabControl.h -- 选项卡控件（四方向页签条 + 内容区，一体自绘）
// 设计：design/TabControl_Analysis.md（决策点 1-8 已拍板）
// ============================================================================
#pragma once

#include "ControlBase.h"
#include <functional>

enum class TabPosition {
    Top,     // 缺省：页签条在上
    Bottom,  // 页签条在下
    Left,    // 页签条在左（竖排堆叠，文字不旋转）
    Right    // 页签条在右
};

struct TabPage {
    std::string title;
    std::shared_ptr<Control> page;            // 页面控件（任意 Control）
    std::shared_ptr<Control> leadingControl;  // 可选：页签图标
    SRect tabRect;                            // 页签命中/绘制区（相对控件原点）
};

class TabControl : public ControlImpl {
    friend class TabControlBuilder;
public:
    using OnTabChange = std::function<void(std::shared_ptr<TabControl>, int index)>;

    TabControl(Control* parent, const SRect& rect, float xScale = 1.0f, float yScale = 1.0f);

    // ── 数据操作 ──
    int  addTab(const std::string& title, std::shared_ptr<Control> page);
    void insertTab(int index, const std::string& title, std::shared_ptr<Control> page);
    void removeTab(int index);
    int  getTabCount() const { return static_cast<int>(m_tabs.size()); }
    const std::vector<TabPage>& getTabs() const { return m_tabs; }

    void setCurrentIndex(int index);
    int  getCurrentIndex() const { return m_currentIndex; }

    void setTabText(int index, const std::string& title);
    void setTabPage(int index, std::shared_ptr<Control> page);
    void setTabLeadingControl(int index, std::shared_ptr<Control> ctl);

    // ── 外观 ──
    void setPosition(TabPosition pos);
    TabPosition getPosition() const { return m_position; }
    void setFontSize(float px);
    float getFontSize() const { return m_fontSize; }

    void setOnTabChange(OnTabChange cb) { m_onTabChange = std::move(cb); }

    // ── 引擎接口 ──
    void draw(void) override;
    bool handleEvent(std::shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    // ── 属性系统 ──
    int setStateColorProperty(const char* prop, StateColor stateColor) override;   // P032：page-background
    int getStateColorProperty(const char* prop, StateColor& out) override;
    void setTextStateColor(StateColor stateColor) override;      // P0-26：文本四态
    void setTextShadowStateColor(StateColor stateColor) override;  // P0-26：阴影色四态
    int setColorProperty(const char* prop, SColor color) override;   // P0-26：text 族 + selected-text
    int getColorProperty(const char* prop, SColor& out) override;
    int setBoolProperty(const char* prop, int value) override;       // P0-26：shadow
    int getBoolProperty(const char* prop, int& out) override;
    int setEnumProperty(const char* prop, const char* value) override;
    int getEnumProperty(const char* prop, const char*& out) override;
    int setIntProperty(const char* prop, int value) override;
    int getIntProperty(const char* prop, int& out) override;
    int setFloatProperty(const char* prop, float value) override;
    int getFloatProperty(const char* prop, float& out) override;

private:
    void relayout();
    void ensureFont();
    int  hitTestTab(float x, float y) const;   // 命中页签索引；-1 未命中（绝对坐标）
    void applyCurrentPage();          // show/hide + setRect 内容区
    void drawTabBar();

    std::vector<TabPage> m_tabs;
    StateColor m_pageBackground{StateColor::Type::Background};   // P032：页背景四态（转发所有页）
    int m_currentIndex = -1;
    TabPosition m_position = TabPosition::Top;
    float m_fontSize = 13.0f;
    FontName m_fontName = FontName::HarmonyOS_Sans_SC_Regular;   // P0-26：字体名可配
    StateColor m_textColor;                                       // P0-26：页签文字四态（ctor 设 normal 缺省）
    SColor m_selectedTextColor{235, 235, 235};                    // P0-26：选中页签文字色（selected-text）
    StateColor m_textShadowColor;                                 // P0-26：阴影色（四态）
    bool   m_shadowEnabled = false;
    SPoint m_shadowOffset{1, 1};
    float m_padding = 8.0f;
    int m_hoveredTab = -1;
    SRect m_contentRect;              // 内容区（相对控件原点）
    OnTabChange m_onTabChange;
    SharedFont m_font;
};

// ── 声明式 Builder（LabelBuilder 同款惯例）──
class TabControlBuilder {
private:
    std::shared_ptr<TabControl> m_tc;
public:
    TabControlBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    TabControlBuilder& setPosition(TabPosition pos);
    TabControlBuilder& setFontSize(float px);
    TabControlBuilder& addTab(const std::string& title, std::shared_ptr<Control> page);
    TabControlBuilder& setCurrentIndex(int index);
    TabControlBuilder& setOnTabChange(TabControl::OnTabChange cb);
    std::shared_ptr<TabControl> build(void);
};
