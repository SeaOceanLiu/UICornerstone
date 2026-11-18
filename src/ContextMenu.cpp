// ============================================================================
// ContextMenu.cpp -- 右键上下文菜单（Popup 浮层 + MenuPanel 内容）
// ============================================================================
#include "ContextMenu.h"
#include "MainWindow.h"
#include "Bench.h"   // P0-71①：BENCH 宏类型完整（getDrawRect/getScaleXX 换算）
#include "PlatformUtils.h"

static void logPositionModeIgnored(const char* api) {
    static bool logged = false;
    if (!logged) {
        Platform::Log("ContextMenu: %s ignored; use setRect/show", api);
        logged = true;
    }
}

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

ContextMenu::~ContextMenu() {
    if (m_menuPanel) m_menuPanel->setOnSizeChanged(nullptr);
}

void ContextMenu::ensureSizeCallback() {
    if (m_sizeCallbackRegistered || !m_menuPanel) return;
    // 构造期间 make_shared 尚未完成，getThis() 为空 → 静默跳过；
    // 首次公开入口（addItem/show/create/syncSize/setRect）时再懒注册。
    auto self = dynamic_pointer_cast<ContextMenu>(getThis());
    if (!self) return;
    m_menuPanel->setOnSizeChanged([weak = weak_ptr<ContextMenu>(self)](const SRect& r) {
        if (auto cm = weak.lock()) cm->applySyncedSize(r.width, r.height);
    });
    m_sizeCallbackRegistered = true;
}

void ContextMenu::applySyncedSize(float w, float h) {
    if (w <= 0.0f || h <= 0.0f) return;
    if (m_rect.width == w && m_rect.height == h) return;
    // 受控路径：只更新外框宽高，避开 Panel::resized 的布局链/重入。
    ControlImpl::resized(SRect(m_rect.left, m_rect.top, w, h));
}

bool ContextMenu::syncSize() {
    ensureSizeCallback();
    if (!m_menuPanel) return false;
    m_menuPanel->recalculateSize();
    if (m_menuPanel->getItems().empty()) return false;
    applySyncedSize(m_menuPanel->getRect().width, m_menuPanel->getRect().height);
    return true;
}

SRect ContextMenu::getMenuPanelRect() const {
    if (!m_menuPanel || m_menuPanel->getItems().empty()) return SRect(0, 0, 0, 0);
    return m_menuPanel->getRect();
}

void ContextMenu::create() {
    ensureSizeCallback();
    Popup::create();
    applyChromeSuppression();   // Popup::create 重置了 chrome，恢复抑制
    ensureSizeCallback();
}

void ContextMenu::beforeDraw() {
    if (!getVisible()) return;
    // CR20261112001：ContextMenu 的视觉（圆角/边框/阴影）全部由内部 MenuPanel
    // 负责；Popup 自身不得再画矩形 box-shadow，否则会出现“底层矩形”。
    m_frameDrawRect = getDrawRect();
    m_frameDrawRectValid = true;
}

void ContextMenu::draw() {
    if (!getVisible()) return;
    // Panel::draw 内部是限定调用 ControlImpl::beforeDraw()，不会走本类 override；
    // 这里显式跳过 Popup 背景/阴影，只绘制内容（MenuPanel）。
    m_frameDrawRect = getDrawRect();
    m_frameDrawRectValid = true;
    ControlImpl::draw();
    afterDraw();
}

void ContextMenu::show(float x, float y) {
    ensureSizeCallback();
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

    // P0-71①（复核 Q-1）：ContextMenu 父级为 bench（画布局部域）——Popup::open 内
    // computeTargetRect 的正确换算被此处覆盖；钳制仍在视口/全局域进行，落到 setRect 前
    // 换算回 bench 局部（anchor + 根复合反查）；主实例 anchor=0/scale=1 时行为不变
    SRect benchDraw = BENCH ? BENCH->getDrawRect() : SRect(0, 0, 0, 0);
    float bsx = BENCH ? BENCH->getScaleXX() : 1.0f;
    float bsy = BENCH ? BENCH->getScaleYY() : 1.0f;
    float lx = (bsx != 0.0f) ? (px - benchDraw.left) / bsx : px;
    float ly = (bsy != 0.0f) ? (py - benchDraw.top) / bsy : py;
    setRect(SRect(lx, ly, w, h));
    m_menuPanel->setRect(SRect(0, 0, w, h));
    m_menuPanel->layoutItems();
}

void ContextMenu::setRect(SRect rect) {
    ensureSizeCallback();
    m_hasExplicitRect = true;
    Popup::setRect(rect);
    if (m_menuPanel) {
        m_menuPanel->setRect(SRect(0, 0, rect.width, rect.height));
        m_menuPanel->recalculateSize();
    }
}

void ContextMenu::setCentered() {
    logPositionModeIgnored("setCentered");
}

int ContextMenu::setColorProperty(const char* prop, SColor color) {
    int ok = Popup::setColorProperty(prop, color);
    int si = -1, fi = -1;
    bool shadowKey = strcmp(prop, PropertyNames::kBoxShadowColor) == 0 ||
                     (ControlImpl::parseBoxShadowStateKey(prop, si, fi) && fi == 0);
    if (ok && shadowKey) {
        static bool logged = false;
        if (!logged) { Platform::Log("ContextMenu: box-shadow is not drawn (visual owned by MenuPanel)"); logged = true; }
    }
    return ok;
}

int ContextMenu::setFloatProperty(const char* prop, float value) {
    int ok = Popup::setFloatProperty(prop, value);
    int si = -1, fi = -1;
    bool shadowKey = strcmp(prop, PropertyNames::kBoxShadowOffsetX) == 0 ||
                     strcmp(prop, PropertyNames::kBoxShadowOffsetY) == 0 ||
                     strcmp(prop, PropertyNames::kBoxShadowRadius) == 0 ||
                     strcmp(prop, PropertyNames::kBoxShadowSpread) == 0 ||
                     (ControlImpl::parseBoxShadowStateKey(prop, si, fi) && fi >= 1);
    if (ok && shadowKey) {
        static bool logged = false;
        if (!logged) { Platform::Log("ContextMenu: box-shadow is not drawn (visual owned by MenuPanel)"); logged = true; }
    }
    return ok;
}

void ContextMenu::setAnchored(Control* anchor, const SRect& offset) {
    (void)anchor;
    (void)offset;
    logPositionModeIgnored("setAnchored");
}

void ContextMenu::applyOpenRect(const SRect& target) {
    ensureSizeCallback();
    if (!m_menuPanel) return;
    if (!m_menuPanel->isCreated() && GET_CONTEXT) {
        m_menuPanel->create();   // open 已挂树：补 create 后字体/行高才可测（幂等）
    }
    if (m_hasExplicitRect) {
        setRect(target);
        return;
    }

    if (m_menuPanel->getItems().empty()) {
        // 空菜单保持当前外框尺寸（默认 10×10），仅按 Centered/Anchored 应用位置。
        SRect pos = computeTargetRect();
        Panel::setRect(pos);
        m_menuPanel->setRect(SRect(0, 0, pos.width, pos.height));
        return;
    }

    // 无显式位置：先按内容测出最终尺寸，再重新计算居中/锚定目标，
    // 避免 Popup::open() 用旧的 10×10 尺寸算完 target 后内容才变大。
    m_menuPanel->recalculateSize();
    float finalW = m_menuPanel->getRect().width;
    float finalH = m_menuPanel->getRect().height;
    Panel::setRect(SRect(m_rect.left, m_rect.top, finalW, finalH));
    SRect pos = computeTargetRect();
    Panel::setRect(pos);   // 不改变 m_anchorMode
    m_menuPanel->setRect(SRect(0, 0, pos.width, pos.height));
    m_menuPanel->recalculateSize();   // 幂等；确保回调/布局稳定
}

void ContextMenu::close(DialogResult result) {
    if (m_menuPanel) m_menuPanel->hide();
    Popup::close(result);
}

void ContextMenu::addItem(shared_ptr<MenuItem> item) {
    ensureSizeCallback();
    if (!item || !m_menuPanel) return;
    auto orig = item->getOnClick();
    // P0-103：菜单点击回调延迟一帧执行——捕获 weak（菜单可能先于回调析构；同步时代裸 this 安全，延迟后悬垂）
    weak_ptr<ContextMenu> self = dynamic_pointer_cast<ContextMenu>(getThis());
    item->setOnClick([self, orig](shared_ptr<MenuItem> it) {
        if (orig) orig(it);
        if (auto s = self.lock()) s->close();
    });
    m_menuPanel->addItem(item);
}

void ContextMenu::addItem(const string& caption, ItemClickHandler onClick) {
    ensureSizeCallback();
    auto item = make_shared<MenuItem>(m_menuPanel.get(), MenuItemType::Normal, 1.f, 1.f);
    item->setCaption(caption);
    weak_ptr<ContextMenu> self = dynamic_pointer_cast<ContextMenu>(getThis());   // P0-103：同上
    if (onClick) {
        item->setOnClick([self, onClick](shared_ptr<MenuItem> it) {
            onClick(it);
            if (auto s = self.lock()) s->close();
        });
    } else {
        item->setOnClick([self](shared_ptr<MenuItem>) { if (auto s = self.lock()) s->close(); });
    }
    m_menuPanel->addItem(item);
}

void ContextMenu::addSeparator() {
    ensureSizeCallback();
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
