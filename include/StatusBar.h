// ============================================================================
// StatusBar.h -- 状态栏控件（VSCode 风格底部状态栏）
// 设计：design/StatusBar_Design.md（决策点 1-6 已拍板）
// 弹窗：内嵌共享 MenuPanel,点击 item 向上弹出（MenuBar 反向）,外部点击关闭
// ============================================================================
#pragma once

#include "ControlBase.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::shared_ptr;
using std::function;

class MenuPanel;

struct StatusItem {
    std::string id;
    std::string text;
    bool rightAlign = false;
    std::shared_ptr<Control> leadingControl;
    std::function<void(std::shared_ptr<StatusItem>)> onClick;
    std::shared_ptr<MenuPanel> menuPanel;
    SRect hitRect;

    // P0-55：段着色（稀疏语义；未设置=继承控件级/不绘制）
    StateColor textColor;              // 段文本色（四态；显式设过的态生效）
    uint8_t    textColorMask = 0;      // bit0 normal / bit1 hover / bit2 pressed / bit3 disabled
    SColor     background;             // 段背景（单色，铺满 hitRect）
    bool       hasBackground = false;

    // P0-58：段级字号 / 文字阴影（稀疏语义；未设置=继承控件级）
    float      fontSize = 0.0f;        // 0 = 继承控件级
    SColor     shadowColor;
    bool       hasShadow = false;
    SPoint     shadowOffset{1.0f, 1.0f};
};

class StatusBar : public ControlImpl {
public:
    StatusBar(Control* parent, const SRect& rect, float xScale = 1.0f, float yScale = 1.0f);

    // ── 数据操作 ──
    void addStatusItem(const string& id, const string& text, bool rightAlign = false);
    void updateStatusItemText(const string& id, const string& text);
    // P0-55：段着色（未设置=继承控件级四态 / 不绘制背景）
    void setStatusItemTextColor(const string& id, SColor color, ControlState state);
    void setStatusItemBackgroundColor(const string& id, SColor color);
    // P0-58：段级字号（0=继承）/ 文字阴影（未设继承控件级）
    void setStatusItemFontSize(const string& id, float size);
    void setStatusItemTextShadow(const string& id, SColor color, float offsetX, float offsetY);
    void removeStatusItem(const string& id);
    void setStatusItemMenu(const string& id, shared_ptr<class MenuPanel> panel);
    void setStatusItemLeadingControl(const string& id, shared_ptr<Control> ctl);
    void setStatusItemOnClick(const string& id, function<void(shared_ptr<StatusItem>)> cb);

    StatusItem* getStatusItem(const string& id);
    shared_ptr<class MenuPanel> getPopupPanel() const { return m_popupPanel; }
    bool isPopupOpen() const;

    // ── 属性 setter（§5.1 矩阵,全部触发 relayout）──
    void setFontSize(float size);
    void setItemHeight(float px);

    // ── 查询 ──
    float getFontSize() const { return m_fontSize; }
    float getItemHeight() const { return m_itemHeight; }

    // ── 引擎接口 ──
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    // ── 属性系统 override ──
    void setTextStateColor(StateColor stateColor) override;      // P0-26：文本四态
    void setTextShadowStateColor(StateColor stateColor) override;  // P0-26：阴影色四态
    int setColorProperty(const char* prop, SColor color) override;   // P0-26：text/text.hover/.pressed/.disabled
    int getColorProperty(const char* prop, SColor& out) override;
    int setBoolProperty(const char* prop, int value) override;       // P0-26：shadow
    int getBoolProperty(const char* prop, int& out) override;
    int setEnumProperty(const char* prop, const char* value) override;  // P0-26：font
    int getEnumProperty(const char* prop, const char*& out) override;
    int setIntProperty(const char* prop, int value) override;    // font-size（int 通道）
    int getIntProperty(const char* prop, int& out) override;
    int setFloatProperty(const char* prop, float value) override;   // item-height / shadow-offset
    int getFloatProperty(const char* prop, float& out) override;

private:
    friend class StatusBarBuilder;

    void relayout();
    void updateItem(int index);
    void ensureFont();
    SharedFont fontForSize(float size);   // P0-58：段级字号字体（cache；<=0 回退控件级）
    int hitTestIndex(float screenX, float screenY) const;  // 屏幕→本地逆变换后按 hitRect 二维命中；-1 未命中
    void openPopup(int itemIndex);
    void closePopup();

    vector<StatusItem> m_items;
    shared_ptr<class MenuPanel> m_popupPanel;     // 共享弹窗（惰性创建）
    int m_hoveredItem = -1;
    float m_fontSize = 13.0f;
    float m_itemHeight = 24.0f;
    float m_spacing = 8.0f;
    float m_padding = 12.0f;
    SharedFont m_font;
    std::unordered_map<int, SharedFont> m_itemFonts;   // P0-58：段级字号字体缓存（键=缩放后像素字号）
    FontName m_fontName = FontName::HarmonyOS_Sans_SC_Regular;   // P0-26：字体名可配
    StateColor m_textColor;                                       // P0-26：段文字四态（ctor 设 normal 缺省）
    StateColor m_textShadowColor;                                 // P0-26：文本阴影色（四态）
    bool   m_shadowEnabled = false;
    SPoint m_shadowOffset{1, 1};
};

// ── 声明式 Builder（LabelBuilder 同款惯例）──
class StatusBarBuilder {
private:
    std::shared_ptr<StatusBar> m_bar;
public:
    StatusBarBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    StatusBarBuilder& setFontSize(float size);
    StatusBarBuilder& setItemHeight(float px);
    StatusBarBuilder& addStatusItem(const std::string& id, const std::string& text, bool rightAlign = false);
    StatusBarBuilder& setStatusItemMenu(const std::string& id, std::shared_ptr<MenuPanel> panel);
    StatusBarBuilder& setStatusItemLeadingControl(const std::string& id, std::shared_ptr<Control> ctl);
    StatusBarBuilder& setStatusItemOnClick(const std::string& id, std::function<void(std::shared_ptr<StatusItem>)> cb);
    std::shared_ptr<StatusBar> build(void);
};
