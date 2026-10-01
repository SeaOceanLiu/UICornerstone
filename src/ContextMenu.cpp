// ============================================================================
// ContextMenu.cpp -- 右键上下文菜单（Popup 浮层 + MenuPanel 内容）
// ============================================================================
#include "ContextMenu.h"
#include "MainWindow.h"

// 视口兜底（context 未就绪时的假想视口，仅参与钳制）
static const SRect kFallbackViewport(0, 0, 1024, 768);   // SRect 构造非 constexpr

ContextMenu::ContextMenu(Control* parent, float xScale, float yScale)
    : Popup(parent, SRect(0, 0, 10, 10), xScale, yScale)
{
    m_ctlType = ControlType::Popup;
    // 子面板 scale 固定 1：挂树后经 setParent 继承父复合（避免 xScale 双重叠加）
    m_menuPanel = make_shared<MenuPanel>(this, 1.0f, 1.0f);
    setContent(m_menuPanel);
    applyChromeSuppression();
}

// 菜单视觉由 MenuPanel 全权负责。Popup::create()（含 setContext→recreate 链）
// 会无条件重置为 Dialog 默认视觉（transparent=false / borderVisible=true），
// 其默认边框色 (83,83,90) 与 MenuPanel 圆角描边 (69,69,69) 在底/右边错开 1px
// 形成双框线——每次创建/显示后须重新抑制。
void ContextMenu::applyChromeSuppression() {
    setTransparent(true);
    setBorderVisible(false);
    // focusFirstContent 兜底会聚焦 Popup 自身 → 焦点环方框与圆角边框叠加；
    // 菜单是瞬态浮层，焦点环无意义，关闭
    setShowFocusRing(false);
}

void ContextMenu::create() {
    Popup::create();
    applyChromeSuppression();   // Popup::create 重置了 chrome，恢复抑制
}

void ContextMenu::show(float x, float y) {
    // 确保上下文就绪（未挂树的浮层借最近实例）
    if (GET_CONTEXT == nullptr) {
        UIContext* ctx = UIContext::getLastInstance();
        if (ctx) setContext(ctx);
    }
    if (!m_menuPanel) return;

    if (!m_menuPanel->isCreated()) m_menuPanel->create();
    m_menuPanel->recalculateSize();     // 第一遍（可能走兜底估算宽度）

    Popup::open();                      // 挂树 + 创建 + 布局 + 显示 + 注册 watcher
    applyChromeSuppression();           // setContext→recreate→Popup::create 重置后恢复

    // 第二遍：context 就绪后按真实字体重算尺寸，并据视口钳制
    m_menuPanel->recalculateSize();
    float w = m_menuPanel->getRect().width;
    float h = m_menuPanel->getRect().height;

    SRect vp = GET_CONTEXT ? GET_CONTEXT->viewport : kFallbackViewport;
    float px = x, py = y;
    if (px + w > vp.left + vp.width)  px = vp.left + vp.width  - w;
    if (py + h > vp.top + vp.height)  py = vp.top + vp.height - h;
    if (px < vp.left) px = vp.left;
    if (py < vp.top)  py = vp.top;

    setRect(SRect(px, py, w, h));
    m_menuPanel->setRect(SRect(0, 0, w, h));
    m_menuPanel->layoutItems();
}

void ContextMenu::setRect(SRect rect) {
    Panel::setRect(rect);
    if (m_menuPanel) {
        m_menuPanel->setRect(SRect(0, 0, rect.width, rect.height));
        m_menuPanel->recalculateSize();
    }
}

void ContextMenu::close(DialogResult result) {
    if (m_menuPanel) m_menuPanel->hide();
    Popup::close(result);
}

void ContextMenu::addItem(shared_ptr<MenuItem> item) {
    if (!item || !m_menuPanel) return;
    auto orig = item->getOnClick();
    item->setOnClick([this, orig](shared_ptr<MenuItem> it) {
        if (orig) orig(it);
        this->close();
    });
    m_menuPanel->addItem(item);
}

void ContextMenu::addItem(const string& caption, ItemClickHandler onClick) {
    auto item = make_shared<MenuItem>(m_menuPanel.get(), MenuItemType::Normal, 1.f, 1.f);
    item->setCaption(caption);
    if (onClick) {
        item->setOnClick([this, onClick](shared_ptr<MenuItem> it) {
            onClick(it);
            this->close();
        });
    } else {
        item->setOnClick([this](shared_ptr<MenuItem>) { this->close(); });
    }
    m_menuPanel->addItem(item);
}

void ContextMenu::addSeparator() {
    if (m_menuPanel) m_menuPanel->addSeparator();
}

bool ContextMenu::isContainsPoint(float x, float y) {
    if (ControlImpl::isContainsPoint(x, y)) return true;
    if (m_menuPanel && m_menuPanel->isContainsPoint(x, y)) return true;
    return false;
}

void ContextMenu::layoutContent() {
    if (!m_menuPanel) return;
    m_menuPanel->setRect(SRect(0, 0, m_rect.width, m_rect.height));
    m_menuPanel->layoutItems();
    m_menuPanel->show();
}

// ── ContextMenuBuilder（声明式构建，LabelBuilder 同款惯例） ──

ContextMenuBuilder::ContextMenuBuilder(float xScale, float yScale)
    : m_menu(nullptr)
{
    m_menu = std::make_shared<ContextMenu>(nullptr, xScale, yScale);
}
ContextMenuBuilder& ContextMenuBuilder::addItem(const std::string& caption, ContextMenu::ItemClickHandler onClick) {
    m_menu->addItem(caption, std::move(onClick)); return *this;
}
ContextMenuBuilder& ContextMenuBuilder::addItem(std::shared_ptr<MenuItem> item) {
    m_menu->addItem(std::move(item)); return *this;
}
ContextMenuBuilder& ContextMenuBuilder::addSeparator() {
    m_menu->addSeparator(); return *this;
}
std::shared_ptr<ContextMenu> ContextMenuBuilder::build(void) {
    m_menu->create();
    return m_menu;
}
