// 由AI(GLM 5.1)生成，可能不完整或有错误，请自行检查和修改
// Menu.cpp - VSCode风格菜单控件实现
// 重构说明：去除内嵌 Label 控件，文本直接经 TextRenderer 绘制（同 TreeView 模式），
// 字体/尺寸状态为实例成员，hover 统一由 MenuPanel 管理。

#include "Menu.h"
#include "TextDraw.h"
#include "PropertyNames.h"
#include "Bench.h"

// ==================== VSCode Dark主题颜色 ====================
namespace MenuColors {
    // 菜单栏
    constexpr SColor BAR_BG(60, 60, 60, 255);
    constexpr SColor BAR_TEXT(204, 204, 204, 255);
    constexpr SColor BAR_HOVER_BG(80, 80, 80, 255);
    constexpr SColor BAR_ACTIVE_BG(9, 71, 113, 255);

    // 下拉面板
    constexpr SColor PANEL_BG(37, 37, 38, 255);
    constexpr SColor PANEL_BORDER(69, 69, 69, 255);
    constexpr SColor PANEL_SHADOW(0, 0, 0, 102);

    // 菜单项
    constexpr SColor ITEM_TEXT(204, 204, 204, 255);
    constexpr SColor ITEM_HOVER_BG(9, 71, 113, 255);
    constexpr SColor ITEM_DISABLED(90, 90, 90, 255);
    constexpr SColor SHORTCUT_TEXT(133, 133, 133, 255);
    constexpr SColor ARROW_COLOR(204, 204, 204, 255);

    // 分隔线
    constexpr SColor SEPARATOR(69, 69, 69, 255);

    // 尺寸（基于字体大小 × 比例系数计算，见 MenuPanel::m_fontSize / m_heightRatio）
    constexpr float DEFAULT_TEXT_SIZE = 20.0f;
    constexpr float DEFAULT_HEIGHT_RATIO = 1.6f;
    constexpr float MIN_HEIGHT_RATIO = 1.0f;
    constexpr float MAX_HEIGHT_RATIO = 3.0f;
    constexpr float ITEM_LEFT_PADDING   = 20.0f;  // icon 区左 padding（面板左缘 → icon 区起点，决策 v3/v4）
    constexpr float ITEM_RIGHT_PADDING  = 20.0f;
    constexpr float ICON_AREA_WIDTH     = 20.0f;
    constexpr float SHORTCUT_MIN_WIDTH  = 60.0f;
    constexpr float ARROW_AREA_WIDTH    = 20.0f;
    constexpr float SEPARATOR_HEIGHT    = 1.0f;
    constexpr float SEPARATOR_MARGIN    = 10.0f;
    constexpr float PANEL_RADIUS        = 5.0f;
    constexpr float PANEL_SHADOW_OFFSET = 3.0f;
    constexpr float PANEL_SHADOW_BLUR   = 8.0f;
    constexpr FontName MENU_FONT        = FontName::MapleMono_NF_CN_Regular;
}

namespace {

// 加载菜单字体（同 TreeView::ensureFont 模式）：从资源读取字体文件并经 TextRenderer 创建
SharedFont loadMenuFont(Control* ctl, FontName fontName, float fontSize) {
    TextRenderer* renderer = ctl->getTextRenderer();
    if (!renderer) return nullptr;
    ResourceProvider* provider = ctl->getResourceProvider();
    if (!provider) return nullptr;

    auto it = ConstDef::fontFiles.find(fontName);
    if (it == ConstDef::fontFiles.end()) return nullptr;
    string fontPath = ConstDef::pathPrefix.string() + "/" + it->second;
    auto data = provider->readFile(fontPath);
    if (!data || data->empty()) return nullptr;

    int scaledSize = static_cast<int>(fontSize * ctl->getScaleXX());
    return renderer->loadFontFromMemoryWithText(data->data(), data->size(), scaledSize, "W");
}

} // namespace

// ==================== MenuItem 实现 ====================

MenuItem::MenuItem(Control *parent, MenuItemType type, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_type(type)
    , m_caption("")
    , m_shortcut("")
    , m_checked(false)
    , m_onClick(nullptr)
    , m_subMenu(nullptr)
    , m_font(nullptr)
    , m_fontSize(MenuColors::DEFAULT_TEXT_SIZE)
{
    m_ctlType = ControlType::MenuItem;
    setRect(SRect(0, 0, 0, 0));
}

MenuItem::~MenuItem() = default;

void MenuItem::create() {
    ControlImpl::create();
}

void MenuItem::setMenuFont(SharedFont font, float fontSize) {
    m_font = std::move(font);
    m_fontSize = fontSize;
}

void MenuItem::setCaption(const string& caption) {
    m_caption = caption;
}

void MenuItem::setShortcut(const string& shortcut) {
    m_shortcut = shortcut;
}

void MenuItem::setChecked(bool checked) {
    m_checked = checked;
}

void MenuItem::setSubMenu(shared_ptr<MenuPanel> panel) {
    m_subMenu = panel;
    m_type = MenuItemType::SubMenu;
    if (panel) {
        panel->setParent(this);
        if (getContext()) panel->setContext(getContext());
    }
}

void MenuItem::draw() {
    if (!getVisible()) return;
    if (m_type == MenuItemType::Separator) return; // 分隔线由MenuPanel绘制

    SRect drawRect = getDrawRect();

    // Menu 增强：icon 区布局参数（面板实例状态 + 自身行高/空隙）
    auto* panel = dynamic_cast<MenuPanel*>(getParent());
    float iconAreaW = panel ? panel->getIconAreaWidth() : MenuColors::ICON_AREA_WIDTH;
    float rowH = panel ? panel->itemRowHeight(this) : m_fontSize * MenuColors::DEFAULT_HEIGHT_RATIO;
    float gap = leading ? leading->getGap() : 8.0f;

    TextRenderer* renderer = getTextRenderer();
    int fontHeight = (renderer && m_font) ? renderer->getFontHeight(m_font.get()) : 0;

    // 前置控件容器（icon 区）：正方形边长 = 字体高度（无字体回退 icon 区宽），
    // icon 区内水平居中、行内垂直居中（各行独立，行高不同则中心线自然错开）。
    // 几何/对齐/命中统一由 LeadingControlSlot 组件承载（见 LeadingControlSlot.h）
    if (leading && leading->hasControl()) {
        float slotSize = leading->getSlotHeight((float)fontHeight);
        float slotLeft = MenuColors::ITEM_LEFT_PADDING + (iconAreaW - slotSize) / 2.0f;
        auto ctl = leading->getControl();
        ctl->setRect(leading->layout(0.0f, rowH, slotLeft, 0.0f, 0.0f, 1.0f, 1.0f, (float)fontHeight));
        ctl->draw();
    } else if (m_checked) {
        // 勾选标记：icon 区中心（有容器时不绘制，勾选视觉由容器内 CheckBox 承担）；行内垂直居中
        GET_RENDERDEVICE->setDrawColor(MenuColors::ITEM_TEXT);
        float cx = drawRect.left + (MenuColors::ITEM_LEFT_PADDING + iconAreaW / 2.0f) * getScaleXX();
        float cy = drawRect.top + drawRect.height / 2.0f;
        GET_RENDERDEVICE->drawLine(cx - 4, cy, cx - 1, cy + 3);
        GET_RENDERDEVICE->drawLine(cx - 1, cy + 3, cx + 4, cy - 3);
    }

    if (renderer && m_font) {
        float textY = drawRect.top + (drawRect.height - fontHeight) / 2;
        const float sx = getScaleXX();
        // P0-26：面板文本色（normal/disabled）与阴影（面板级配置）
        if (panel && !getEnable()) {   // P0-33③：禁用项底色（三态之 disabled）
            GET_RENDERDEVICE->setDrawColor(panel->getItemDisabledBgColor());
            GET_RENDERDEVICE->fillRect(drawRect);
        }
        const bool hovered = panel && panel->isItemHovered(this);
        SColor itemColor = (!getEnable())
            ? (panel ? panel->getItemDisabledTextColor() : MenuColors::ITEM_DISABLED)
            : (hovered && panel ? panel->getItemHoverTextColor()
                                : (panel ? panel->getItemTextColor() : MenuColors::ITEM_TEXT));
        SColor shadowColor = panel
            ? (!getEnable() ? panel->getItemTextShadowState().getDisabled()
                            : (hovered ? panel->getItemTextShadowState().getHover()
                                       : panel->getItemTextShadowState().getNormal()))
            : SColor(0, 0, 0, 120);
        const bool shadowEnabled = panel ? panel->isTextShadowEnabled() : false;
        SPoint shadowOffset = panel ? panel->getTextShadowOffset() : SPoint(1, 1);

        // 标题（所有行文字起点统一：无 leadingControl 时从左 padding 起，不预留 icon 区）
        if (!m_caption.empty()) {
            float textX = drawRect.left + (MenuColors::ITEM_LEFT_PADDING
                + (panel && panel->hasLeadingControl() ? (iconAreaW + gap) : 0)) * sx;
            TextDraw::withShadow(renderer, m_font.get(), m_caption,
                                 textX, textY, textX + shadowOffset.x * sx, textY + shadowOffset.y * sx,
                                 itemColor, shadowEnabled, shadowColor);
        }

        // 快捷键（右侧右对齐，箭头区域之前）
        if (!m_shortcut.empty()) {
            SSize sz = renderer->measureText(m_font.get(), m_shortcut);
            float shortcutWidth = sz.width / getScaleXX();
            float right = drawRect.right();
            if (m_type == MenuItemType::SubMenu) right -= MenuColors::ARROW_AREA_WIDTH;
            float textX = right - MenuColors::ITEM_RIGHT_PADDING - shortcutWidth;
            TextDraw::withShadow(renderer, m_font.get(), m_shortcut,
                                 textX, textY, textX + shadowOffset.x * sx, textY + shadowOffset.y * sx,
                                 MenuColors::SHORTCUT_TEXT, shadowEnabled, shadowColor);
        }
    }

    // 子菜单箭头（最右侧，图形绘制不依赖字体字形）
    if (m_type == MenuItemType::SubMenu) {
        float cx = drawRect.right() - MenuColors::ARROW_AREA_WIDTH / 2.0f;
        float cy = drawRect.top + drawRect.height / 2.0f;
        float size = 4.0f;
        GET_RENDERDEVICE->drawTriangle(
            cx - size * 0.577f, cy - size,
            cx - size * 0.577f, cy + size,
            cx + size * 0.577f * 2, cy,
            MenuColors::ARROW_COLOR);
    }
}

bool MenuItem::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;
    if (m_type == MenuItemType::Separator) return false;

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) { mx = event->mousePos.x; my = event->mousePos.y; gotPos = true; }
    else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }
    if (gotPos) {
        SRect drawRect = getDrawRect();
        if (drawRect.contains(mx, my)) {
            // 前置控件命中（先于 onClick 判断，覆盖 MouseMove/MouseDown/MouseUp）：
            // 不触发 item onClick、不关闭菜单链，转子控件分发完成控件自身交互
            // （CheckBox 在 MouseUp 翻转勾选，须经此路径送达；返回子控件分发结果）
            if (leading && leading->containsPoint(mx, my)) {
                return ControlImpl::handleEvent(event);
            }
            if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
                if (m_type == MenuItemType::Normal && m_onClick) {
                    closeMenuChain();
                    m_onClick(dynamic_pointer_cast<MenuItem>(getThis()));
                    fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
                }
                return true;
            }
            return true;
        }
    }
    return false;
}

void MenuItem::closeMenuChain() {
    // 向上查找MenuPanel或MenuBar，关闭整个菜单链
    Control* p = getParent();
    while (p) {
        auto panel = dynamic_cast<MenuPanel*>(p);
        if (panel) {
            panel->closeWithChildren();
            // 继续向上查找MenuBar
            p = panel->getParent();
            continue;
        }
        auto bar = dynamic_cast<MenuBar*>(p);
        if (bar) {
            bar->closeAllMenus();
            bar->exitMenuMode();
            return;
        }
        p = p->getParent();
    }
}

// Menu 增强：前置控件统一由 LeadingControlSlot 组件承载（挂载/对齐/间隙/命中）
void MenuItem::setLeadingControl(shared_ptr<Control> ctl) {
    ensureLeading();
    if (leading->getControl() == ctl) return;
    leading->detachFrom(this);
    leading->setControl(ctl);
    leading->attachTo(this);
}

void MenuItem::ensureLeading() {
    if (!leading) {
        leading = make_shared<LeadingControlSlot>();
        leading->setFallbackSize(MenuColors::ICON_AREA_WIDTH);
    }
}

// fontSize>0 时按自身 fontName/fontSize 重建字体；加载失败保持现状（回退注入字体）
void MenuItem::ensureOwnFont() {
    if (fontSize <= 0) return;
    SharedFont f = loadMenuFont(this, fontName, (float)fontSize);
    if (f) {
        m_font = std::move(f);
        m_fontSize = (float)fontSize;
    }
}

// ==================== MenuPanel 实现 ====================

MenuPanel::MenuPanel(Control *parent, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_fontSize(MenuColors::DEFAULT_TEXT_SIZE)
    , m_heightRatio(MenuColors::DEFAULT_HEIGHT_RATIO)
    , m_itemHeight(m_fontSize * m_heightRatio)
    , m_iconAreaWidth(MenuColors::ICON_AREA_WIDTH)
    , m_hasLeadingControl(false)
    , m_shortcutAreaWidth(0)
    , m_arrowAreaWidth(MenuColors::ARROW_AREA_WIDTH)
    , m_hoveredIndex(-1)
    , m_visible(false)
    , m_openSubMenu(nullptr)
    , m_font(nullptr)
    , m_fontName(MenuColors::MENU_FONT)
    , m_bgColor(MenuColors::PANEL_BG)
    , m_borderColor(MenuColors::PANEL_BORDER)
    , m_hoverColor(MenuColors::ITEM_HOVER_BG)
    , m_disabledBgColor(MenuColors::PANEL_BG)      // 缺省与常态同色（视觉不变）
    , m_hoverBorderColor(MenuColors::PANEL_BORDER)
    , m_disabledBorderColor(MenuColors::PANEL_BORDER)
    , m_separatorColor(MenuColors::SEPARATOR)
    , m_textColor(MenuColors::ITEM_TEXT)
    , m_hoverTextColor(MenuColors::ITEM_TEXT)
    , m_disabledTextColor(MenuColors::ITEM_DISABLED)
    , m_shadowRadius(MenuColors::PANEL_SHADOW_BLUR)
{
    m_ctlType = ControlType::MenuPanel;
    setRect(SRect(0, 0, 0, 0));
    setBorderVisible(true);
}

MenuPanel::~MenuPanel() = default;

void MenuPanel::setContext(UIContext* ctx) {
    ControlImpl::setContext(ctx);
    if (ctx) {
        // 挂树前 addItem 时 context 未就绪，ensureFont 加载失败（m_font 为空），
        // 菜单文字不显示、面板宽度按空文本计算过窄；context 就绪后补加载并重算
        ensureFont();
        updateItemsFont();
        recalculateSize();
    }
    // 菜单项由 addItem 挂入 m_items（不在 m_children 中），需手动传播 context，
    // 否则 MenuItem 的 m_context 为空，绘制时 GET_RENDERDEVICE 解引用崩溃
    for (auto& item : m_items) {
        item->setContext(ctx);
        if (item->m_subMenu) item->m_subMenu->setContext(ctx);
    }
    if (m_openSubMenu) m_openSubMenu->setContext(ctx);
}

void MenuPanel::ensureFont() {
    if (m_font) return;
    m_font = loadMenuFont(this, m_fontName, m_fontSize);
}

// 父链缩放变更时重建共享字体并刷新布局；子菜单面板不在 m_children 中，
// 与 setContext 手动传播对齐，直接下发复合缩放（面板字体随自身缩放重建）
void MenuPanel::refreshScaleWith(float parentXX, float parentYY) {
    float oldScaleX = getScaleXX();
    float oldScaleY = getScaleYY();
    ControlImpl::refreshScaleWith(parentXX, parentYY);
    if (oldScaleX != getScaleXX() || oldScaleY != getScaleYY()) {
        m_font.reset();
        if (m_isCreated) {
            ensureFont();
            updateItemsFont();
        }
        recalculateSize();
    }
    for (auto& item : m_items) {
        // items 不在 m_children 中（面板手动管理），父链缩放须显式下发
        item->refreshScaleWith(m_xxScale, m_yyScale);
        if (item->m_subMenu) item->m_subMenu->refreshScaleWith(m_xxScale, m_yyScale);
    }
    if (m_openSubMenu) m_openSubMenu->refreshScaleWith(m_xxScale, m_yyScale);
}

void MenuPanel::updateItemsFont() {
    for (auto& item : m_items) {
        if (item->fontSize > 0) item->ensureOwnFont();
        else item->setMenuFont(m_font, m_fontSize);
    }
}

void MenuPanel::setFontSize(float size) {
    if (m_fontSize == size) return;
    m_fontSize = size;
    m_itemHeight = m_fontSize * m_heightRatio;
    m_font.reset();
    if (m_isCreated) {
        ensureFont();
        updateItemsFont();
    }
    recalculateSize();
}

void MenuPanel::setItemHeightRatio(float ratio) {
    if (ratio < MenuColors::MIN_HEIGHT_RATIO) ratio = MenuColors::MIN_HEIGHT_RATIO;
    if (ratio > MenuColors::MAX_HEIGHT_RATIO) ratio = MenuColors::MAX_HEIGHT_RATIO;
    if (m_heightRatio == ratio) return;
    m_heightRatio = ratio;
    m_itemHeight = m_fontSize * m_heightRatio;
    recalculateSize();
}

void MenuPanel::setFontName(FontName fontName) {
    if (m_fontName == fontName && m_font) return;
    m_fontName = fontName;
    m_font.reset();
    if (m_isCreated) {
        ensureFont();
        updateItemsFont();
    }
}

void MenuPanel::addItem(shared_ptr<MenuItem> item) {
    if (!item) return;
    ensureFont();
    item->setParent(this);
    // 覆盖项先建自身字体再测量（否则 recalculateSize 首测用面板字体，icon 区宽/行高/面板宽偏差）
    if (item->fontSize > 0) item->ensureOwnFont();
    else item->setMenuFont(m_font, m_fontSize);
    m_items.push_back(item);
    if (getContext()) item->setContext(getContext());
    recalculateSize();
}

void MenuPanel::addSeparator() {
    auto sep = make_shared<MenuItem>(this, MenuItemType::Separator);
    m_items.push_back(sep);
    recalculateSize();
}

void MenuPanel::removeItem(shared_ptr<MenuItem> item) {
    auto it = std::find(m_items.begin(), m_items.end(), item);
    if (it != m_items.end()) {
        m_items.erase(it);
        recalculateSize();
    }
}

// 变行高统一 helper（recalculateSize/layoutItems/hitTest 三处共用，防漂移）：
// Separator = 分隔线高；其余 = 生效字号（注入或自身覆盖）× heightRatio
float MenuPanel::itemRowHeight(MenuItem* item) const {
    if (item->getType() == MenuItemType::Separator)
        return MenuColors::SEPARATOR_HEIGHT + 2 * MenuColors::SEPARATOR_MARGIN;
    return item->m_fontSize * m_heightRatio;
}

void MenuPanel::show() {
    m_visible = true;
    setVisible(true);
}

void MenuPanel::hide() {
    m_visible = false;
    setVisible(false);
    if (m_openSubMenu) {
        m_openSubMenu->hide();
        m_openSubMenu = nullptr;
    }
    m_hoveredIndex = -1;
}

void MenuPanel::setPosition(float x, float y) {
    setRect(SRect(x, y, getRect().width, getRect().height));
    layoutItems();
}

void MenuPanel::recalculateSize() {
    if (m_items.empty()) {
        setRect(SRect(getRect().left, getRect().top, 0, 0));
        return;
    }

    // 计算最大宽度（需要容纳最长的标题+快捷键+箭头）
    float maxCaptionWidth = 0;
    float maxShortcutWidth = 0;
    bool hasSubMenu = false;
    float maxGap = 0;          // 各行 leadingGap 最大值（面板宽度用）
    float maxIconFontH = 0;    // 各行字体高度最大值（icon 区宽 = max(...,20)）

    TextRenderer* renderer = getTextRenderer();
    m_hasLeadingControl = false;
    for (auto& item : m_items) {
        if (item->getType() == MenuItemType::Separator) continue;
        if (item->leading && item->leading->hasControl()) m_hasLeadingControl = true;
        if (item->hasSubMenu()) hasSubMenu = true;
        if (item->leading && item->leading->getGap() > maxGap) maxGap = item->leading->getGap();

        // 获取标题宽度
        if (renderer && item->m_font) {
            float fontH = (float)renderer->getFontHeight(item->m_font.get());
            if (fontH > maxIconFontH) maxIconFontH = fontH;
            if (!item->m_caption.empty()) {
                float w = renderer->measureText(item->m_font.get(), item->m_caption).width / getScaleXX();
                if (w > maxCaptionWidth) maxCaptionWidth = w;
            }
            // 获取快捷键宽度
            if (!item->m_shortcut.empty()) {
                float w = renderer->measureText(item->m_font.get(), item->m_shortcut).width / getScaleXX();
                if (w > maxShortcutWidth) maxShortcutWidth = w;
            }
        }
    }

    // icon 区宽 = max(各行字体高度, 20)（实例状态，随逐项字体变化）
    m_iconAreaWidth = (std::max)(maxIconFontH, MenuColors::ICON_AREA_WIDTH);
    m_shortcutAreaWidth = (std::max)(maxShortcutWidth, MenuColors::SHORTCUT_MIN_WIDTH);

    // 面板宽度：icon 区左 padding + icon 区宽 + 最大 gap + 标题 + 快捷键 + 箭头 + 右 padding
    // （面板内无 leadingControl 时不预留 icon 区与 gap 空间）
    float panelWidth = MenuColors::ITEM_LEFT_PADDING
                     + (m_hasLeadingControl ? (m_iconAreaWidth + maxGap) : 0)
                     + maxCaptionWidth
                     + (m_shortcutAreaWidth > 0 ? m_shortcutAreaWidth + 10.0f : 0)
                     + (hasSubMenu ? m_arrowAreaWidth : 0)
                     + MenuColors::ITEM_RIGHT_PADDING;

    // 面板高度（变行高：逐行累加）
    float panelHeight = 0;
    for (auto& item : m_items) {
        panelHeight += itemRowHeight(item.get());
    }

    setRect(SRect(getRect().left, getRect().top, panelWidth, panelHeight));
    layoutItems();
}

void MenuPanel::layoutItems() {
    float y = 0;
    for (auto& item : m_items) {
        if (item->getType() == MenuItemType::Separator) {
            item->setRect(SRect(0, y, getRect().width,
                MenuColors::SEPARATOR_HEIGHT + 2 * MenuColors::SEPARATOR_MARGIN));
            y += item->getRect().height;
        } else {
            float rowH = itemRowHeight(item.get());
            item->setRect(SRect(0, y, getRect().width, rowH));
            y += rowH;
        }
    }
}

int MenuPanel::hitTest(float x, float y) {
    // 输入为全局绘制（像素）坐标；itemY/itemRowHeight 为逻辑坐标 → mapViewportToCanvas 换算（÷复合缩放）
    SPoint local = mapViewportToCanvas({x, y});

    float itemY = 0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        float itemHeight = itemRowHeight(item.get());

        if (local.y >= itemY && local.y < itemY + itemHeight) {
            return (item->getType() == MenuItemType::Separator) ? -1 : (int)i;
        }
        itemY += itemHeight;
    }
    return -1;
}

shared_ptr<MenuItem> MenuPanel::getItemAt(float x, float y) {
    int index = hitTest(x, y);
    if (index >= 0 && index < (int)m_items.size()) {
        return m_items[index];
    }
    return nullptr;
}

void MenuPanel::setOpenSubMenu(shared_ptr<MenuPanel> panel) {
    if (m_openSubMenu == panel) return;

    // 关闭之前的子菜单
    if (m_openSubMenu) {
        m_openSubMenu->hide();
    }
    m_openSubMenu = panel;
    if (m_openSubMenu) {
        m_openSubMenu->show();
    }
}

void MenuPanel::setHoveredIndex(int index) {
    if (m_hoveredIndex == index) return;

    m_hoveredIndex = index;

    // 如果hover到子菜单项，展开子菜单
    if (m_hoveredIndex >= 0 && m_hoveredIndex < (int)m_items.size()) {
        auto& item = m_items[m_hoveredIndex];
        if (item->hasSubMenu()) {
            auto subMenu = item->getSubMenu();
            // 子菜单的 parent 是 MenuItem，setPosition 使用相对 MenuItem 的坐标：
            // x = item 完整宽度（紧贴右缘），y = 0（与 item 顶部对齐）。
            // 传入绝对坐标（itemRect.right/top）会被父链偏移二次叠加导致错位
            subMenu->setPosition(item->getRect().width, 0);
            setOpenSubMenu(subMenu);
        } else {
            setOpenSubMenu(nullptr);
        }
    } else {
        setOpenSubMenu(nullptr);
    }
}

int MenuPanel::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kHoveredIndex) == 0) { setHoveredIndex(value); return 1; }
    if (strcmp(prop, PropertyNames::kLabelFontSize) == 0) { setFontSize((float)value); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemFontSize) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item) return 0;
        item->setOwnFontSize(value);
        if (value > 0) item->ensureOwnFont();
        else item->setMenuFont(m_font, m_fontSize);  // 0 = 恢复继承面板字体
        recalculateSize();
        return 1;
    }
    return ControlImpl::setIntProperty(prop, value);
}

int MenuPanel::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kHoveredIndex) == 0) { out = m_hoveredIndex; return 1; }
    if (strcmp(prop, PropertyNames::kLabelFontSize) == 0) { out = (int)m_fontSize; return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemFontSize) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item) return 0;
        out = item->getOwnFontSize();
        return 1;
    }
    return ControlImpl::getIntProperty(prop, out);
}

int MenuPanel::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kTreeItemId) == 0) { m_itemTargetId = value ? value : ""; return 1; }
    return ControlImpl::setStringProperty(prop, value);
}

int MenuPanel::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kTreeItemId) == 0) { out = m_itemTargetId.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}

// P0-33③：对象路径（colors.background/border）→ 专用成员（三态 N/H/D）
void MenuPanel::setBackgroundStateColor(StateColor stateColor) {
    m_bgColor = stateColor.getNormal();
    m_hoverColor = stateColor.getHover();
    m_disabledBgColor = stateColor.getDisabled();
}
void MenuPanel::setBorderStateColor(StateColor stateColor) {
    m_borderColor = stateColor.getNormal();
    m_hoverBorderColor = stateColor.getHover();
    m_disabledBorderColor = stateColor.getDisabled();
    setBorderVisible(true);
}

// P0-26：JSON colors.text 三态（normal/hover/disabled）→ 面板 item 角色色
void MenuPanel::setTextStateColor(StateColor stateColor) {
    m_textColor = stateColor.getNormal();
    m_hoverTextColor = stateColor.getHover();
    m_disabledTextColor = stateColor.getDisabled();
}
void MenuPanel::setTextShadowStateColor(StateColor stateColor) {
    m_textShadowColor = stateColor;
}

bool MenuPanel::isItemHovered(const MenuItem* item) const {
    return m_hoveredIndex >= 0 && m_hoveredIndex < (int)m_items.size() &&
           m_items[m_hoveredIndex].get() == item;
}

int MenuPanel::setColorProperty(const char* prop, SColor color) {   // P0-26：面板/条目颜色属性化
    if (strcmp(prop, PropertyNames::kBackground) == 0)  { m_bgColor = color;        return 1; }
    if (strcmp(prop, PropertyNames::kStateHover) == 0)  { m_hoverColor = color;     return 1; }
    if (strcmp(prop, PropertyNames::kStateDisabled) == 0){ m_disabledBgColor = color; return 1; }
    if (strcmp(prop, PropertyNames::kBorder) == 0)      { m_borderColor = color;    return 1; }
    if (strcmp(prop, PropertyNames::kBorderHover) == 0) { m_hoverBorderColor = color; return 1; }
    if (strcmp(prop, PropertyNames::kBorderDisabled) == 0){ m_disabledBorderColor = color; return 1; }
    if (strcmp(prop, PropertyNames::kText) == 0)        { m_textColor = color;      return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)   { m_hoverTextColor = color; return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0){ m_disabledTextColor = color; return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)  { m_textShadowColor.setNormal(color); return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

int MenuPanel::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kBackground) == 0)  { out = m_bgColor;           return 1; }
    if (strcmp(prop, PropertyNames::kStateHover) == 0)  { out = m_hoverColor;        return 1; }
    if (strcmp(prop, PropertyNames::kBorder) == 0)      { out = m_borderColor;       return 1; }
    if (strcmp(prop, PropertyNames::kText) == 0)        { out = m_textColor;         return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)   { out = m_hoverTextColor;    return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0){ out = m_disabledTextColor; return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)  { out = m_textShadowColor.getNormal(); return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int MenuPanel::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { m_shadowEnabled = (value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}

int MenuPanel::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { out = m_shadowEnabled ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int MenuPanel::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { m_shadowOffset.x = value; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { m_shadowOffset.y = value; return 1; }
    if (strcmp(prop, PropertyNames::kItemHeightRatio) == 0) { setItemHeightRatio(value); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingGap) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item) return 0;
        item->setLeadingGap(value);
        recalculateSize();
        return 1;
    }
    return ControlImpl::setFloatProperty(prop, value);
}

int MenuPanel::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_shadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_shadowOffset.y; return 1; }
    if (strcmp(prop, PropertyNames::kItemHeightRatio) == 0) { out = m_heightRatio; return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingGap) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item) return 0;
        out = item->getLeadingGap();
        return 1;
    }
    return ControlImpl::getFloatProperty(prop, out);
}

int MenuPanel::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        if (!value) return 0;
        setFontName(FontNameFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kTreeItemFont) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item || !value) return 0;
        item->setFontName(FontNameFromString(value));
        if (item->getOwnFontSize() > 0) item->ensureOwnFont();
        recalculateSize();
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

int MenuPanel::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_fontName); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemFont) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item) return 0;
        out = FontNameToString(item->getFontName());
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}

int MenuPanel::setPtrProperty(const char* prop, void* value) {
    if (strcmp(prop, PropertyNames::kTreeItemLeadingControl) == 0) {
        // 前置控件容器（借用语义，生命周期由调用方保证，与 TreeView 同约定）：
        // 无删除器包装，避免 MenuPanel 误删外部控件；挂/摘由 setLeadingControl（addControl/removeControl）完成
        auto item = getItemById(m_itemTargetId);
        if (!item) return 0;
        if (value) {
            item->setLeadingControl(shared_ptr<Control>(static_cast<Control*>(value), [](Control*) {}));
        } else {
            item->setLeadingControl(nullptr);   // NULL = 解除容器并摘树
        }
        recalculateSize();
        return 1;
    }
    return ControlImpl::setPtrProperty(prop, value);
}

int MenuPanel::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kTreeItemLeadingControl) == 0) {
        auto item = getItemById(m_itemTargetId);
        if (!item || !item->getLeadingControl()) return 0;
        out = item->getLeadingControl().get();
        return 1;
    }
    return ControlImpl::getPtrProperty(prop, out);
}

shared_ptr<MenuItem> MenuPanel::getItemById(const string& id) const {
    if (id.empty()) return nullptr;
    for (auto& item : m_items) {
        if (item->getItemId() == id) return item;
    }
    return nullptr;
}

void MenuPanel::closeWithChildren() {
    if (m_openSubMenu) {
        m_openSubMenu->closeWithChildren();
    }
    hide();
}

void MenuPanel::drawShadow() {
    SRect drawRect = getDrawRect();

    // 使用GraphTool绘制阴影
    GraphTool::DrawingContext dc(getRenderDevice());
    dc.setFillColor(GraphTool::SColor(
        MenuColors::PANEL_SHADOW.red(),
        MenuColors::PANEL_SHADOW.green(),
        MenuColors::PANEL_SHADOW.blue(),
        MenuColors::PANEL_SHADOW.alpha()));

    // 绘制阴影矩形（偏移+模糊区域）
    float offset = MenuColors::PANEL_SHADOW_OFFSET;
    float blur = MenuColors::PANEL_SHADOW_BLUR;
    dc.drawRoundedRect(
        ::SRect(drawRect.left + offset - blur/2,
                        drawRect.top + offset - blur/2,
                        drawRect.width + blur,
                        drawRect.height + blur),
        MenuColors::PANEL_RADIUS + blur/2, true);
}

void MenuPanel::draw() {
    if (!m_visible) return;

    SRect drawRect = getDrawRect();

    // 1. 绘制阴影
    drawShadow();

    // 2. 绘制背景（圆角矩形）
    {
        GraphTool::DrawingContext dc(getRenderDevice());
        dc.setFillColor(GraphTool::SColor(
            m_bgColor.red(), m_bgColor.green(),
            m_bgColor.blue(), m_bgColor.alpha()));
        dc.drawRoundedRect(::SRect(drawRect.left, drawRect.top,
            drawRect.width, drawRect.height), MenuColors::PANEL_RADIUS, true);
    }

    // 3. 绘制边框（P0-26：border-visible 门控；P0-33③：三态）
    if (getBorderVisible()) {
        const SColor bc = (getState() == ControlState::Disabled) ? m_disabledBorderColor
                        : (getState() == ControlState::Hover)    ? m_hoverBorderColor
                                                                 : m_borderColor;
        GraphTool::DrawingContext dc(getRenderDevice());
        dc.setPen(GraphTool::SPen(
            GraphTool::SColor(bc.red(), bc.green(), bc.blue(), bc.alpha()), 1.0f));
        dc.drawRoundedRect(::SRect(drawRect.left, drawRect.top,
            drawRect.width, drawRect.height), MenuColors::PANEL_RADIUS, false);
    }

    // 4. 绘制菜单项和分隔线（hover 背景统一由面板管理）
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        if (item->getType() == MenuItemType::Separator) {
            SRect itemRect = item->getDrawRect();
            SRect lineRect(
                drawRect.left + MenuColors::SEPARATOR_MARGIN,
                itemRect.top + MenuColors::SEPARATOR_MARGIN,
                drawRect.width - 2 * MenuColors::SEPARATOR_MARGIN,
                MenuColors::SEPARATOR_HEIGHT
            );
            GET_RENDERDEVICE->setDrawColor(m_separatorColor);
            GET_RENDERDEVICE->fillRect(lineRect);
        } else {
            if ((int)i == m_hoveredIndex) {
                SRect itemRect = item->getDrawRect();
                SRect bgRect(drawRect.left, itemRect.top, drawRect.width, itemRect.height);
                GET_RENDERDEVICE->setDrawColor(m_hoverColor);
                GET_RENDERDEVICE->fillRect(bgRect);
            }
            item->draw();
        }
    }

    // 5. 绘制打开的子菜单
    if (m_openSubMenu) {
        m_openSubMenu->draw();
    }
}

bool MenuPanel::handleEvent(shared_ptr<Event> event) {
    if (!m_visible) return false;

    // 先让子菜单处理事件
    if (m_openSubMenu && m_openSubMenu->handleEvent(event)) return true;

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) { mx = event->mousePos.x; my = event->mousePos.y; gotPos = true; }
    else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }
    if (gotPos) {
        SRect drawRect = getDrawRect();
        if (drawRect.contains(mx, my)) {
            int index = hitTest(mx, my);
            if (event->m_type == EventType::MouseMove) {
                // 带前置控件的行先转发 MouseMove（容器 hover 态），再统一维护面板 hover 背景
                if (index >= 0) {
                    auto& item = m_items[index];
                    if (item->leading && item->leading->hasControl()) item->leading->handleEvent(event);
                }
                setHoveredIndex(index);
                return true;
            }
            if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
                if (index >= 0) {
                    auto& item = m_items[index];
                    // 命中前置控件 → 控件自身交互（不关菜单不触发 onClick，面板仍消费防穿透）；
                    // 未命中 → item onClick + 关菜单链（item 级 fireCCallback）
                    item->handleEvent(event);
                }
                return true;
            }
            if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
                // 带前置控件的行转发 MouseUp（CheckBox 在 MouseUp 翻转勾选，须送达）
                if (index >= 0) {
                    auto& item = m_items[index];
                    if (item->leading && item->leading->hasControl()) item->leading->handleEvent(event);
                }
                return true;
            }
            return true;
        }
    }
    return false;
}

bool MenuPanel::isContainsPoint(float x, float y) {
    if (!m_visible) return false;
    if (ControlImpl::isContainsPoint(x, y)) return true;
    if (m_openSubMenu && m_openSubMenu->isContainsPoint(x, y)) return true;
    return false;
}

// ==================== MenuBar 实现 ====================

MenuBar::MenuBar(Control *parent, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_barHeight(MenuColors::DEFAULT_TEXT_SIZE * MenuColors::DEFAULT_HEIGHT_RATIO)
    , m_hoveredIndex(-1)
    , m_activeIndex(-1)
    , m_menuMode(false)
    , m_bgColor(MenuColors::BAR_BG)
    , m_textColor(MenuColors::BAR_TEXT)
    , m_hoverBgColor(MenuColors::BAR_HOVER_BG)
    , m_hoverTextColor(MenuColors::BAR_TEXT)
    , m_activeBgColor(MenuColors::BAR_ACTIVE_BG)
    , m_borderColor(MenuColors::PANEL_BORDER)
    , m_hoverBorderColor(MenuColors::PANEL_BORDER)
    , m_disabledBorderColor(MenuColors::PANEL_BORDER)
    , m_disabledBgColor(MenuColors::BAR_BG)
    , m_disabledTextColor(MenuColors::ITEM_DISABLED)
    , m_itemHeightRatio(MenuColors::DEFAULT_HEIGHT_RATIO)
    , m_menuTextSize(MenuColors::DEFAULT_TEXT_SIZE)
    , m_font(nullptr)
    , m_fontName(MenuColors::MENU_FONT)
{
    m_ctlType = ControlType::MenuBar;
    setRect(SRect(0, 0, 0, m_barHeight));
    setBorderVisible(false);
}

MenuBar::~MenuBar() {
    // 若下拉面板正挂在 BENCH 顶层，析构前须摘回，避免父链/子列表悬垂；
    // 仅实例存活期执行（退出期 m_context 可能已随 Instance 释放，BENCH 宏将解引用悬垂指针）
    if (UIContext::isActive(m_context)) {
        closeAllMenus();
    }
}

void MenuBar::setContext(UIContext* ctx) {
    ControlImpl::setContext(ctx);
    if (ctx) {
        // 挂树前 addMenu 时 context 未就绪，ensureFont 加载失败，菜单栏文字不显示
        ensureFont();
    }
    // 下拉面板由 addMenu 挂入 m_entries（不在 m_children 中），需手动传播 context
    for (auto& e : m_entries) {
        e.panel->setContext(ctx);
    }
}

void MenuBar::ensureFont() {
    if (m_font) return;
    m_font = loadMenuFont(this, m_fontName, m_menuTextSize);
}

// 父链缩放变更时重建菜单栏字体并重排条目；下拉面板不在 m_children 中，
// 手动传播复合缩放（与 setContext 传播一致）
void MenuBar::refreshScaleWith(float parentXX, float parentYY) {
    float oldScaleX = getScaleXX();
    float oldScaleY = getScaleYY();
    ControlImpl::refreshScaleWith(parentXX, parentYY);
    if (oldScaleX != getScaleXX() || oldScaleY != getScaleYY()) {
        m_font.reset();
        if (m_isCreated) ensureFont();
        layoutEntries();
    }
    Control* bench = BENCH;
    for (auto& e : m_entries) {
        // 已挂到 BENCH 顶层的面板不随 MenuBar 复合刷新（其复合由 Bench 决定，
        // attach/detach 时经 setParent 重新对齐，避免双重缩放）
        if (bench != nullptr && e.panel->getParent() == bench) continue;
        e.panel->refreshScaleWith(m_xxScale, m_yyScale);
    }
}

void MenuPanel::setParent(Control* parent) {
    ControlImpl::setParent(parent);
    if (parent) {
        // items 不在 m_children 中，挂载后复合缩放须显式下发；字体按新复合重建并重排
        for (auto& item : m_items) item->refreshScaleWith(m_xxScale, m_yyScale);
        m_font.reset();
        if (m_isCreated) {
            ensureFont();
            updateItemsFont();
        }
        recalculateSize();
    }
}

void MenuBar::setParent(Control *parent) {
    ControlImpl::setParent(parent);
    if (parent) {
        // 顶层独立语义：仅挂到 Bench（画布顶层）时保持默认全宽贴顶布局；
        // 挂到普通容器（v-flow 等布局引擎容器）时自动切换为手动定位，
        // 放弃全宽重置、rect 完全由外部（布局引擎）驱动，从而实现流内重排
        if (dynamic_cast<Bench*>(parent) == nullptr) {
            m_manualPosition = true;
        }
        // 挂树时 parent 链已可解析 renderer：补加载字体（parseLayout 创建的菜单栏
        // 挂树前 ensureFont 失败，缺字体时 layoutEntries 的 hitRect 宽度为 0，
        // 导致点击菜单栏条目无法打开面板），再按父宽布局（手动定位模式跳过全宽重置）
        ensureFont();
        layoutEntries();  // 全宽重置仅非 manual 时生效（manual 由 setRect 自由定位）
    }
}

void MenuBar::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    layoutEntries();
}

void MenuBar::addMenu(const string& caption, shared_ptr<MenuPanel> panel) {
    ensureFont();
    MenuEntry entry;
    entry.caption = caption;
    entry.panel = panel;
    entry.panel->setParent(this);
    entry.panel->setFontSize(m_menuTextSize);
    entry.panel->setItemHeightRatio(m_itemHeightRatio);
    entry.panel->hide();
    if (getContext()) entry.panel->setContext(getContext());

    m_entries.push_back(std::move(entry));
    layoutEntries();
}

shared_ptr<MenuPanel> MenuBar::getMenuPanel(int index) const {
    if (index < 0 || index >= (int)m_entries.size()) return nullptr;
    return m_entries[index].panel;
}

void MenuBar::removeMenu(const string& caption) {
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
        [&caption](const MenuEntry& e) { return e.caption == caption; });
    if (it != m_entries.end()) {
        m_entries.erase(it);
        layoutEntries();
    }
}

void MenuBar::layoutEntries() {
    float x = 0;
    TextRenderer* renderer = getTextRenderer();
    for (auto& entry : m_entries) {
        float entryWidth = 0;
        if (renderer && m_font) {
            float textWidth = renderer->measureText(m_font.get(), entry.caption).width / getScaleXX();
            entryWidth = textWidth + 2 * MenuColors::ITEM_LEFT_PADDING;
        }
        entry.hitRect = SRect(x, 0, entryWidth, m_barHeight);
        x += entryWidth;
    }
    // 更新菜单栏宽度（手动定位模式跳过：setRect 自由生效，条目 hitRect 仍按 0 起点排布）
    if (!m_manualPosition && getParent()) {
        ControlImpl::setRect(SRect(0, 0, getParent()->getRect().width, m_barHeight));
    }
}

int MenuBar::hitTest(float x, float y) {
    // 输入为全局绘制（像素）坐标；hitRect 为逻辑坐标 → mapViewportToCanvas 换算（÷复合缩放）
    SPoint local = mapViewportToCanvas({x, y});
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].hitRect.contains(local.x, local.y)) {
            return (int)i;
        }
    }
    return -1;
}

void MenuBar::openMenu(int index) {
    // 关闭当前打开的菜单
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        detachMenuPanel(m_activeIndex);
    }

    m_activeIndex = index;
    if (index >= 0 && index < (int)m_entries.size()) {
        attachMenuPanel(index);
    }
}

void MenuBar::switchMenu(int index) {
    if (m_activeIndex == index) return;
    openMenu(index);
}

void MenuBar::enterMenuMode(int index) {
    m_menuMode = true;
    openMenu(index);
}

void MenuBar::exitMenuMode() {
    m_menuMode = false;
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        detachMenuPanel(m_activeIndex);
    }
    m_activeIndex = -1;
    m_hoveredIndex = -1;
}

void MenuBar::closeAllMenus() {
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        m_entries[m_activeIndex].panel->closeWithChildren();
        detachMenuPanel(m_activeIndex);
    }
    m_activeIndex = -1;
}

void MenuBar::attachMenuPanel(int index) {
    auto& entry = m_entries[index];
    Control* bench = BENCH;
    if (bench != nullptr) {
        // 与 Popup::open 同款语序：先挂树（继承根复合）再计算位置——
        // 弹层作为画布内容，复合 = 面板自身缩放 × 根变换；挂树后
        // getScaleXX 才能给出目标绝对矩形所需的复合基准
        if (entry.panel->getParent() != bench) {
            entry.panel->setParent(bench);
            bench->addControl(entry.panel);  // 追加到 Bench 子列表末尾 = 绘制/事件最顶层
        }
        // recalc 在挂树后运行：内部 measureText ÷ getScaleXX（面板复合），
        // 产出的即"逻辑（无缩放）尺寸"；显示尺寸 = 逻辑 × 面板复合，
        // 与挂 MenuBar 子级时代一致，此处不再乘复合（否则双重缩放）
        entry.panel->recalculateSize();
        SRect barDraw = getDrawRect();
        float sx = getScaleXX(), sy = getScaleYY();
        SRect logicSize = entry.panel->getRect();
        // 绝对坐标 = MenuBar 绘制矩形 + hitRect×复合缩放（hitRect 为菜单栏局部逻辑坐标）
        entry.panel->setRect(SRect(
            barDraw.left + entry.hitRect.left * sx,
            barDraw.top + (entry.hitRect.top + entry.hitRect.height) * sy,
            logicSize.width,
            logicSize.height));
        entry.panel->show();
        registerMenuWatcher();
    } else {
        // 无实例（离线/单测）：退化为 MenuBar 子级局部定位，由 MenuBar::draw 手动绘制
        entry.panel->setPosition(entry.hitRect.left, entry.hitRect.top + entry.hitRect.height);
        entry.panel->show();
    }
}

void MenuBar::detachMenuPanel(int index) {
    auto& entry = m_entries[index];
    entry.panel->hide();
    Control* bench = BENCH;
    if (bench != nullptr && entry.panel->getParent() == bench) {
        bench->removeControl(entry.panel);
        entry.panel->setParent(this);  // 还原父链，供下次 attachMenuPanel 复用
    }
    unregisterMenuWatcher();
}

void MenuBar::registerMenuWatcher() {
    if (m_watcherRegistered) return;
    if (!m_context || !m_context->eventQueue) return;
    m_context->eventQueue->addBeforeEventHandlingWatcher(EventType::KeyDown, getThis());
    m_context->eventQueue->addBeforeEventHandlingWatcher(EventType::MouseDown, getThis());
    m_watcherRegistered = true;
}

void MenuBar::unregisterMenuWatcher() {
    if (!m_watcherRegistered) return;
    if (m_context && m_context->eventQueue) {
        m_context->eventQueue->removeBeforeEventHandlingWatcher(EventType::KeyDown, getThis());
        m_context->eventQueue->removeBeforeEventHandlingWatcher(EventType::MouseDown, getThis());
    }
    m_watcherRegistered = false;
}

bool MenuBar::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    // 事件分发入口的最前置拦截：打开菜单期间，任何位于本 MenuBar
    // （含打开的面板/子菜单区域）之外的点击/ESC 都先关闭——事件随后仍
    // 正常流向目标控件（其它 MenuBar 可打开自己的菜单）。
    if (m_activeIndex < 0 || m_activeIndex >= (int)m_entries.size()) return false;

    if (event->m_type == EventType::KeyDown && event->keyEvent.keycode == KeyCode::Escape) {
        exitMenuMode();
        return true;
    }
    if (event->m_type != EventType::MouseDown) return false;

    SPoint mp(event->mouseButton.x, event->mouseButton.y);
    if (ControlImpl::isContainsPoint(mp.x, mp.y)) return false;   // 本栏内正常交互
    if (m_entries[m_activeIndex].panel->isContainsPoint(mp.x, mp.y)) return false;  // 面板/子菜单内
    exitMenuMode();                                               // 其它位置：先关，放行事件
    return false;
}

void MenuBar::setBarHeight(float height) {
    m_barHeight = height;
    setRect(SRect(getRect().left, getRect().top, getRect().width, m_barHeight));
    layoutEntries();
}

void MenuBar::setItemHeightRatio(float ratio) {
    if (ratio < MenuColors::MIN_HEIGHT_RATIO) ratio = MenuColors::MIN_HEIGHT_RATIO;
    if (ratio > MenuColors::MAX_HEIGHT_RATIO) ratio = MenuColors::MAX_HEIGHT_RATIO;
    if (m_itemHeightRatio == ratio) return;
    m_itemHeightRatio = ratio;
    for (auto& entry : m_entries) {
        entry.panel->setItemHeightRatio(ratio);
    }
}

void MenuBar::setFontSize(float size) {
    if (m_menuTextSize == size) return;
    m_menuTextSize = size;
    m_font.reset();
    if (m_isCreated) ensureFont();
    for (auto& entry : m_entries) {
        entry.panel->setFontSize(size);
    }
    layoutEntries();
}

// P0-33③：对象路径（colors.background/border）→ 专用成员（三态 N/H/D）+ 面板传播
void MenuBar::setBackgroundStateColor(StateColor stateColor) {
    m_bgColor = stateColor.getNormal();
    m_hoverBgColor = stateColor.getHover();
    m_disabledBgColor = stateColor.getDisabled();
    for (auto& entry : m_entries) if (entry.panel) entry.panel->setBackgroundStateColor(stateColor);
}
void MenuBar::setBorderStateColor(StateColor stateColor) {
    m_borderColor = stateColor.getNormal();
    m_hoverBorderColor = stateColor.getHover();
    m_disabledBorderColor = stateColor.getDisabled();
    setBorderVisible(true);
    for (auto& entry : m_entries) if (entry.panel) entry.panel->setBorderStateColor(stateColor);
}

// P0-26：JSON colors.text 三态 → 自身 + 全部面板
void MenuBar::setTextStateColor(StateColor stateColor) {
    m_textColor = stateColor.getNormal();
    m_hoverTextColor = stateColor.getHover();
    m_disabledTextColor = stateColor.getDisabled();
    for (auto& entry : m_entries) if (entry.panel) entry.panel->setTextStateColor(stateColor);
}
void MenuBar::setTextShadowStateColor(StateColor stateColor) {
    m_textShadowColor = stateColor;
    for (auto& entry : m_entries) if (entry.panel) entry.panel->setTextShadowStateColor(stateColor);
}

// P0-26：MenuBar 颜色属性 → 自身 + 全部下拉面板（统一覆盖 bar/panel/item 角色）
int MenuBar::setColorProperty(const char* prop, SColor color) {
    bool ok = false;
    if (strcmp(prop, PropertyNames::kBackground) == 0)   { m_bgColor = color;            ok = true; }
    else if (strcmp(prop, PropertyNames::kStateHover) == 0)  { m_hoverBgColor = color;  ok = true; }
    else if (strcmp(prop, PropertyNames::kStatePressed) == 0){ m_activeBgColor = color; ok = true; }
    else if (strcmp(prop, PropertyNames::kBorder) == 0)      { m_borderColor = color;   ok = true; }
    else if (strcmp(prop, PropertyNames::kBorderHover) == 0) { m_hoverBorderColor = color; ok = true; }
    else if (strcmp(prop, PropertyNames::kBorderDisabled) == 0){ m_disabledBorderColor = color; ok = true; }
    else if (strcmp(prop, PropertyNames::kStateDisabled) == 0){ m_disabledBgColor = color; ok = true; }
    else if (strcmp(prop, PropertyNames::kText) == 0)        { m_textColor = color;     ok = true; }
    else if (strcmp(prop, PropertyNames::kTextHover) == 0)   { m_hoverTextColor = color; ok = true; }
    else if (strcmp(prop, PropertyNames::kTextDisabled) == 0){ m_disabledTextColor = color; ok = true; }
    else if (strcmp(prop, PropertyNames::kTextShadow) == 0)  { m_textShadowColor.setNormal(color); ok = true; }
    if (ok) {
        for (auto& entry : m_entries) {
            if (entry.panel) entry.panel->setColorProperty(prop, color);
        }
        return 1;
    }
    return ControlImpl::setColorProperty(prop, color);
}

int MenuBar::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kBackground) == 0)   { out = m_bgColor;            return 1; }
    if (strcmp(prop, PropertyNames::kStateHover) == 0)   { out = m_hoverBgColor;       return 1; }
    if (strcmp(prop, PropertyNames::kStatePressed) == 0) { out = m_activeBgColor;      return 1; }
    if (strcmp(prop, PropertyNames::kStateDisabled) == 0){ out = m_disabledBgColor;    return 1; }   // P0-44：补读回
    if (strcmp(prop, PropertyNames::kBorder) == 0)       { out = m_borderColor;        return 1; }
    if (strcmp(prop, PropertyNames::kBorderHover) == 0)  { out = m_hoverBorderColor;   return 1; }   // P0-44：补读回
    if (strcmp(prop, PropertyNames::kBorderDisabled) == 0){ out = m_disabledBorderColor; return 1; } // P0-44：补读回
    if (strcmp(prop, PropertyNames::kText) == 0)         { out = m_textColor;          return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)    { out = m_hoverTextColor;     return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0) { out = m_disabledTextColor;  return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)   { out = m_textShadowColor.getNormal(); return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

// P0-44：对象路径三态读回组装（UICornerstone_GetStateColor 全链可读；pressed 取 hover 语义）
int MenuBar::getStateColorProperty(const char* prop, StateColor& out) {
    if (strcmp(prop, PropertyNames::kBackground) == 0) {
        out = StateColor(m_bgColor, m_hoverBgColor, m_hoverBgColor, m_disabledBgColor);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kBorder) == 0) {
        out = StateColor(m_borderColor, m_hoverBorderColor, m_hoverBorderColor, m_disabledBorderColor);
        return 1;
    }
    return ControlImpl::getStateColorProperty(prop, out);
}

int MenuBar::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0 || strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
        (strcmp(prop, PropertyNames::kShadowOffsetX) == 0 ? m_shadowOffset.x : m_shadowOffset.y) = value;
        for (auto& entry : m_entries) if (entry.panel) entry.panel->setFloatProperty(prop, value);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kItemHeightRatio) == 0) { setItemHeightRatio(value); return 1; }
    if (strcmp(prop, PropertyNames::kValue) == 0)           { setFontSize(value);        return 1; }
    if (strcmp(prop, PropertyNames::kBarHeight) == 0)       { setBarHeight(value);       return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}

int MenuBar::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kManualPosition) == 0) { setManualPosition(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // P0-26：文本阴影开关 → 自身 + 全部面板
        m_shadowEnabled = (value != 0);
        for (auto& entry : m_entries) if (entry.panel) entry.panel->setBoolProperty(prop, value);
        return 1;
    }
    return ControlImpl::setBoolProperty(prop, value);
}

int MenuBar::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kManualPosition) == 0) { out = m_manualPosition ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kShadow) == 0) { out = m_shadowEnabled ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int MenuBar::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        FontName fn = FontNameFromString(value);
        if (m_fontName == fn) return 1;
        m_fontName = fn;
        m_font.reset();
        if (m_isCreated) ensureFont();
        for (auto& entry : m_entries) {
            entry.panel->setFontName(fn);
        }
        layoutEntries();
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

int MenuBar::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_shadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_shadowOffset.y; return 1; }
    if (strcmp(prop, PropertyNames::kItemHeightRatio) == 0) { out = m_itemHeightRatio; return 1; }
    if (strcmp(prop, PropertyNames::kValue) == 0)           { out = m_menuTextSize;    return 1; }
    if (strcmp(prop, PropertyNames::kBarHeight) == 0)       { out = m_barHeight;       return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

int MenuBar::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_fontName); return 1; }
    return ControlImpl::getEnumProperty(prop, out);
}

void MenuBar::draw() {
    if (!getVisible()) return;

    SRect drawRect = getDrawRect();

    // 1. 绘制菜单栏背景（P0-44：disabled/hover 态解析；悬停菜单项时保持 normal，避免与 item hover 底色叠加）
    SColor barBg = m_bgColor;
    if (getState() == ControlState::Disabled) {
        barBg = m_disabledBgColor;
    } else if (getState() == ControlState::Hover && m_hoveredIndex < 0) {
        barBg = m_hoverBgColor;
    }
    GET_RENDERDEVICE->setDrawColor(barBg);
    GET_RENDERDEVICE->fillRect(SRect(drawRect.left, drawRect.top, drawRect.width, drawRect.height));

    // 2. 绘制菜单项（背景 + 标题文本）
    TextRenderer* renderer = getTextRenderer();
    float sx = getScaleXX(), sy = getScaleYY();
    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto& entry = m_entries[i];
        SRect hitRect = entry.hitRect;

        if ((int)i == m_activeIndex) {
            SRect itemBg(drawRect.left + hitRect.left * sx, drawRect.top + hitRect.top * sy,
                         hitRect.width * sx, hitRect.height * sy);
            GET_RENDERDEVICE->setDrawColor(m_activeBgColor);
            GET_RENDERDEVICE->fillRect(itemBg);
        } else if ((int)i == m_hoveredIndex) {
            SRect itemBg(drawRect.left + hitRect.left * sx, drawRect.top + hitRect.top * sy,
                         hitRect.width * sx, hitRect.height * sy);
            GET_RENDERDEVICE->setDrawColor(m_hoverBgColor);
            GET_RENDERDEVICE->fillRect(itemBg);
        }

        if (renderer && m_font) {
            SSize sz = renderer->measureText(m_font.get(), entry.caption);
            int fontHeight = renderer->getFontHeight(m_font.get());
            float textY = drawRect.top + (drawRect.height - fontHeight) / 2;
            float textX = drawRect.left + hitRect.left * sx + (hitRect.width * sx - sz.width) / 2;
            const bool hovered = ((int)i == m_hoveredIndex);
            SColor color = hovered ? m_hoverTextColor : m_textColor;
            SColor shadowColor = hovered ? m_textShadowColor.getHover() : m_textShadowColor.getNormal();
            TextDraw::withShadow(renderer, m_font.get(), entry.caption,
                                 textX, textY,
                                 textX + m_shadowOffset.x * sx, textY + m_shadowOffset.y * sy,
                                 color, m_shadowEnabled, shadowColor);
        }
    }

    // 3. 绘制底部分隔线（P0-26：border 属性化）
    GET_RENDERDEVICE->setDrawColor(
        (getState() == ControlState::Disabled) ? m_disabledBorderColor
      : (getState() == ControlState::Hover)    ? m_hoverBorderColor
                                               : m_borderColor);   // P0-33③：三态
    GET_RENDERDEVICE->drawLine(drawRect.left, drawRect.top + drawRect.height - 1,
                               drawRect.left + drawRect.width, drawRect.top + drawRect.height - 1);

    // 4. 绘制打开的下拉菜单（已挂 BENCH 顶层的面板由 Bench 绘制，仅离线退路手动画）
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        Control* bench = BENCH;
        if (bench == nullptr || m_entries[m_activeIndex].panel->getParent() != bench) {
            m_entries[m_activeIndex].panel->draw();
        }
    }
}

bool MenuBar::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;

    // 先让打开的下拉菜单处理事件
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        if (m_entries[m_activeIndex].panel->handleEvent(event)) return true;
    }

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) { mx = event->mousePos.x; my = event->mousePos.y; gotPos = true; }
    else if (event->m_type == EventType::MouseDown || event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }
    if (gotPos) {
        SRect drawRect = getDrawRect();
        int index = hitTest(mx, my);

        if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
            if (index >= 0) {
                if (m_menuMode && m_activeIndex == index) {
                    exitMenuMode();
                } else {
                    enterMenuMode(index);
                }
                return true;
            } else if (!drawRect.contains(mx, my)) {
                bool inPanel = false;
                if (m_activeIndex >= 0) {
                    inPanel = m_entries[m_activeIndex].panel->isContainsPoint(mx, my);
                }
                if (!inPanel) {
                    exitMenuMode();
                    return false;
                }
            }
        }

        if (event->m_type == EventType::MouseMove) {
            if (m_menuMode) {
                if (index >= 0 && index != m_activeIndex) {
                    switchMenu(index);
                }
                return true;
            } else {
                m_hoveredIndex = index;
            }
        }

        if (drawRect.contains(mx, my)) return true;
        if (m_menuMode && m_activeIndex >= 0) {
            if (m_entries[m_activeIndex].panel->isContainsPoint(mx, my)) {
                return true;
            }
        }
    }
    return false;
}

bool MenuBar::isContainsPoint(float x, float y) {
    if (ControlImpl::isContainsPoint(x, y)) return true;
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        if (m_entries[m_activeIndex].panel->isContainsPoint(x, y)) return true;
    }
    return false;
}

// ── MenuItem property system ──
int MenuItem::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kChecked) == 0) { setChecked(value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int MenuItem::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kCaption) == 0)  { setCaption(value);  return 1; }
    if (strcmp(prop, PropertyNames::kShortcut) == 0) { setShortcut(value); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemId) == 0) { setItemId(value); return 1; }
    return ControlImpl::setStringProperty(prop, value);
}
int MenuItem::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kChecked) == 0) { out = m_checked ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}
int MenuItem::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCaption) == 0)  { out = m_caption.c_str();  return 1; }
    if (strcmp(prop, PropertyNames::kShortcut) == 0) { out = m_shortcut.c_str(); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemId) == 0) { out = m_itemId.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}
int MenuItem::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

// ==================== Builder实现 ====================

MenuItemBuilder::MenuItemBuilder(const string& caption, float xScale, float yScale)
    : m_item(make_shared<MenuItem>(nullptr, MenuItemType::Normal, xScale, yScale))
{
    m_item->setCaption(caption);
}

MenuItemBuilder& MenuItemBuilder::setShortcut(const string& shortcut) {
    m_item->setShortcut(shortcut);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setOnClick(MenuItem::OnClickHandler handler) {
    m_item->setOnClick(handler);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setChecked(bool checked) {
    m_item->setChecked(checked);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setSubMenu(shared_ptr<MenuPanel> panel) {
    m_item->setSubMenu(panel);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setEnabled(bool enabled) {
    m_item->setEnable(enabled);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_item->setBackgroundStateColor(stateColor);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setTextStateColor(StateColor stateColor) {
    m_item->setTextStateColor(stateColor);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setLeadingControl(shared_ptr<Control> ctl) {
    m_item->setLeadingControl(std::move(ctl));
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setLeadingGap(float gap) {
    m_item->setLeadingGap(gap);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setFontName(FontName fn) {
    m_item->setFontName(fn);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setFontSize(int size) {
    m_item->setOwnFontSize(size);
    return *this;
}

MenuItemBuilder& MenuItemBuilder::setItemId(const string& id) {
    m_item->setItemId(id);
    return *this;
}

shared_ptr<MenuItem> MenuItemBuilder::build() {
    m_item->create();
    return m_item;
}

MenuPanelBuilder::MenuPanelBuilder(float xScale, float yScale)
    : m_panel(make_shared<MenuPanel>(nullptr, xScale, yScale))
{
}

MenuPanelBuilder& MenuPanelBuilder::addItem(shared_ptr<MenuItem> item) {
    m_items.push_back(item);
    return *this;
}

MenuPanelBuilder& MenuPanelBuilder::addSeparator() {
    m_items.push_back(nullptr); // 用nullptr标记分隔线
    return *this;
}

shared_ptr<MenuPanel> MenuPanelBuilder::build() {
    m_panel->create();
    for (auto& item : m_items) {
        if (item) {
            m_panel->addItem(item);
        } else {
            m_panel->addSeparator();
        }
    }
    return m_panel;
}

MenuBarBuilder::MenuBarBuilder(Control *parent, float xScale, float yScale)
    : m_menuBar(make_shared<MenuBar>(parent, xScale, yScale))
{
}

MenuBarBuilder& MenuBarBuilder::addMenu(const string& caption, shared_ptr<MenuPanel> panel) {
    m_menuBar->addMenu(caption, panel);
    return *this;
}

MenuBarBuilder& MenuBarBuilder::setBarHeight(float height) {
    m_menuBar->setBarHeight(height);
    return *this;
}

MenuBarBuilder& MenuBarBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_menuBar->setBackgroundStateColor(stateColor);
    return *this;
}

MenuBarBuilder& MenuBarBuilder::setTextStateColor(StateColor stateColor) {
    m_menuBar->setTextStateColor(stateColor);
    return *this;
}

shared_ptr<MenuBar> MenuBarBuilder::build() {
    m_menuBar->create();
    return m_menuBar;
}
