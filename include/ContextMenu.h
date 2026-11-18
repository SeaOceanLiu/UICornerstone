// ============================================================================
// ContextMenu.h -- 右键上下文菜单控件（Popup + MenuPanel 复用浮层/菜单机制）
// 设计：design/ContextMenu_Analysis.md（决策点 1-8 已拍板）
// ============================================================================
#pragma once

#include "Dialog.h"
#include "Menu.h"

class ContextMenu : public Popup {
    friend class ContextMenuBuilder;
public:
    using ItemClickHandler = std::function<void(shared_ptr<MenuItem>)>;

    ContextMenu(Control* parent, float xScale = 1.0f, float yScale = 1.0f);
    ~ContextMenu() override;

    // ── 打开（x,y 为 bench 本地坐标，通常取右键事件的鼠标坐标）──
    void show(float x, float y);
    void draw() override;                       // CR20261112001：跳过 Popup 背景/矩形阴影，只画内部 MenuPanel
    void create() override;                     // Popup::create 后恢复 chrome 抑制
    void beforeDraw() override;                 // CR20261112001：禁止 Popup 自身矩形阴影，视觉全权由 MenuPanel 负责
    void setCentered() override;                // CR20261113001：ContextMenu 不支持运行时改定位模式
    void setAnchored(Control* anchor, const SRect& offset = {0, 0, 0, 0}) override;
    // CR20261116002：ContextMenu 不绘制 Popup box-shadow，设置时给出一次性诊断
    int setColorProperty(const char* prop, SColor color) override;
    int setFloatProperty(const char* prop, float value) override;

    // ── CR20261112002：外框尺寸随 MenuPanel 内容同步 ──
    bool syncSize();                            // 强制重算并同步；空菜单返回 false（外框保持）
    SRect getMenuPanelRect() const;             // MenuPanel 局部坐标；空菜单 (0,0,0,0)

    // ── 数据操作（包装 onClick：用户回调执行后自动关闭菜单）──
    void addItem(shared_ptr<MenuItem> item);
    void addItem(const string& caption, ItemClickHandler onClick = nullptr);
    void addSeparator();

    shared_ptr<MenuPanel> getMenuPanel() const { return m_menuPanel; }

    // ── 重写：覆盖子菜单区域，避免点击子菜单被误判为外部点击 ──
    bool isContainsPoint(float x, float y) override;
    void setRect(SRect rect) override;          // P0-37：resize 传播 menu panel

    void close(DialogResult result = DialogResult::Cancelled) override;

protected:
    void applyChromeSuppression();              // 菜单视觉全权由 MenuPanel 负责
    void layoutContent() override;
    void applyOpenRect(const SRect& target) override;   // CR20261113001

private:
    void ensureSizeCallback();
    void applySyncedSize(float w, float h);
    shared_ptr<MenuPanel> m_menuPanel;
    bool m_sizeCallbackRegistered = false;
    bool m_hasExplicitRect = false;             // CR20261113001：外部显式位置
};

// ── 声明式 Builder（LabelBuilder 同款惯例；ContextMenu 无 rect，仅 scale）──
class ContextMenuBuilder {
private:
    std::shared_ptr<ContextMenu> m_menu;
public:
    ContextMenuBuilder(float xScale = 1.0f, float yScale = 1.0f);
    ContextMenuBuilder& addItem(const std::string& caption, ContextMenu::ItemClickHandler onClick = nullptr);
    ContextMenuBuilder& addItem(std::shared_ptr<MenuItem> item);
    ContextMenuBuilder& addSeparator();
    std::shared_ptr<ContextMenu> build(void);
};
