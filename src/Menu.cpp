// 由AI(GLM 5.1)生成，可能不完整或有错误，请自行检查和修改
// Menu.cpp - VSCode风格菜单控件实现

#include "Menu.h"

// ==================== VSCode Dark主题颜色 ====================
namespace MenuColors {
    // 菜单栏
    constexpr SDL_Color BAR_BG          = {60, 60, 60, 255};      // #3C3C3C
    constexpr SDL_Color BAR_TEXT        = {204, 204, 204, 255};   // #CCCCCC
    constexpr SDL_Color BAR_HOVER_BG    = {80, 80, 80, 255};     // #505050
    constexpr SDL_Color BAR_ACTIVE_BG   = {9, 71, 113, 255};     // #094771

    // 下拉面板
    constexpr SDL_Color PANEL_BG        = {37, 37, 38, 255};     // #252526
    constexpr SDL_Color PANEL_BORDER    = {69, 69, 69, 255};     // #454545
    constexpr SDL_Color PANEL_SHADOW    = {0, 0, 0, 102};        // rgba(0,0,0,0.4)

    // 菜单项
    constexpr SDL_Color ITEM_TEXT       = {204, 204, 204, 255};   // #CCCCCC
    constexpr SDL_Color ITEM_HOVER_BG   = {9, 71, 113, 255};     // #094771
    constexpr SDL_Color ITEM_DISABLED   = {90, 90, 90, 255};     // #5A5A5A
    constexpr SDL_Color SHORTCUT_TEXT   = {133, 133, 133, 255};  // #858585
    constexpr SDL_Color ARROW_COLOR     = {204, 204, 204, 255};  // #CCCCCC

    // 分隔线
    constexpr SDL_Color SEPARATOR       = {69, 69, 69, 255};     // #454545

    // 尺寸（基于字体大小比例计算）
    inline float g_menuTextSize = 20.0f;
    constexpr float DEFAULT_HEIGHT_RATIO = 1.6f;
    inline float g_heightRatio = DEFAULT_HEIGHT_RATIO;
    constexpr float MIN_HEIGHT_RATIO = 1.0f;
    constexpr float MAX_HEIGHT_RATIO = 3.0f;
    inline float getItemHeight() { return g_menuTextSize * g_heightRatio; }
    inline float getBarHeight() { return g_menuTextSize * g_heightRatio; }
    constexpr float ITEM_LEFT_PADDING   = 28.0f;
    constexpr float ITEM_RIGHT_PADDING  = 20.0f;
    constexpr float ICON_AREA_WIDTH     = 20.0f;
    constexpr float SHORTCUT_MIN_WIDTH  = 60.0f;
    constexpr float ARROW_AREA_WIDTH    = 20.0f;
    constexpr float SEPARATOR_HEIGHT    = 1.0f;
    constexpr float SEPARATOR_MARGIN    = 10.0f;
    constexpr float PANEL_RADIUS        = 5.0f;
    constexpr float PANEL_SHADOW_OFFSET = 3.0f;
    constexpr float PANEL_SHADOW_BLUR   = 8.0f;
    constexpr FontName MENU_FONT         = FontName::MapleMono_NF_CN_Regular;
}

// ==================== MenuItem 实现 ====================

MenuItem::MenuItem(Control *parent, MenuItemType type, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_type(type)
    , m_caption("")
    , m_shortcut("")
    , m_checked(false)
    , m_hovered(false)
    , m_onClick(nullptr)
    , m_subMenu(nullptr)
    , m_captionLabel(nullptr)
    , m_shortcutLabel(nullptr)
    , m_arrowLabel(nullptr)
{
    setRect(SRect(0, 0, 0, 0));
}

MenuItem::~MenuItem() = default;

void MenuItem::create() {
    ControlImpl::create();
    createLabels();
}

float MenuItem::getItemHeight() {
    return MenuColors::getItemHeight();
}

void MenuItem::setCaption(const string& caption) {
    m_caption = caption;
    createLabels();
}

void MenuItem::setShortcut(const string& shortcut) {
    m_shortcut = shortcut;
    createLabels();
}

void MenuItem::setChecked(bool checked) {
    m_checked = checked;
}

void MenuItem::setSubMenu(shared_ptr<MenuPanel> panel) {
    m_subMenu = panel;
    m_type = MenuItemType::SubMenu;
    createLabels();
}

void MenuItem::createLabels() {
    // 创建标题Label
    if (!m_caption.empty() && m_type != MenuItemType::Separator) {
        m_captionLabel = LabelBuilder(this, {0, 0, 0, 0})
            .setFont(MenuColors::MENU_FONT)
            .setAlignmentMode(AlignmentMode::AM_MID_LEFT)
            .setFontSize((int)MenuColors::g_menuTextSize)
            .setCaption(m_caption)
            .setTextStateColor(StateColor(StateColor::Type::Text)
                .setNormal(MenuColors::ITEM_TEXT)
                .setHover(MenuColors::ITEM_TEXT)
                .setPressed(MenuColors::ITEM_TEXT))
            .setEnableExpand(true)
            .build();
    }

    // 创建快捷键Label
    if (!m_shortcut.empty()) {
        m_shortcutLabel = LabelBuilder(this, {0, 0, 0, 0})
            .setFont(MenuColors::MENU_FONT)
            .setAlignmentMode(AlignmentMode::AM_MID_RIGHT)
            .setFontSize((int)MenuColors::g_menuTextSize)
            .setCaption(m_shortcut)
            .setTextStateColor(StateColor(StateColor::Type::Text)
                .setNormal(MenuColors::SHORTCUT_TEXT)
                .setHover(MenuColors::SHORTCUT_TEXT)
                .setPressed(MenuColors::SHORTCUT_TEXT))
            .setEnableExpand(true)
            .build();
    }

    // 创建子菜单箭头Label
    if (m_type == MenuItemType::SubMenu) {
        m_arrowLabel = LabelBuilder(this, {0, 0, 0, 0})
            .setFont(MenuColors::MENU_FONT)
            .setAlignmentMode(AlignmentMode::AM_CENTER)
            .setFontSize((int)MenuColors::g_menuTextSize)
            .setCaption(u8"\u25B6")
            .setTextStateColor(StateColor(StateColor::Type::Text)
                .setNormal(MenuColors::ARROW_COLOR)
                .setHover(MenuColors::ARROW_COLOR)
                .setPressed(MenuColors::ARROW_COLOR))
            .setEnableExpand(true)
            .build();
    }
}

void MenuItem::updateLabelPositions() {
    if (m_type == MenuItemType::Separator) return;

    float w = getRect().width;
    float h = getRect().height;

    // 标题：左侧padding后开始
    if (m_captionLabel) {
        float captionWidth = w - MenuColors::ITEM_LEFT_PADDING - MenuColors::ITEM_RIGHT_PADDING;
        if (m_shortcutLabel) captionWidth -= MenuColors::SHORTCUT_MIN_WIDTH;
        if (m_arrowLabel) captionWidth -= MenuColors::ARROW_AREA_WIDTH;
        if (captionWidth < 0) captionWidth = 0;
        m_captionLabel->setRect(SRect(MenuColors::ITEM_LEFT_PADDING, 0, captionWidth, h));
    }

    // 快捷键：右侧箭头区域之前
    if (m_shortcutLabel) {
        float shortcutRight = w - MenuColors::ITEM_RIGHT_PADDING;
        if (m_arrowLabel) shortcutRight -= MenuColors::ARROW_AREA_WIDTH;
        float shortcutWidth = m_shortcutLabel->getHotRect().width;
        m_shortcutLabel->setRect(SRect(shortcutRight - shortcutWidth, 0, shortcutWidth, h));
    }

    // 箭头：最右侧
    if (m_arrowLabel) {
        m_arrowLabel->setRect(SRect(w - MenuColors::ARROW_AREA_WIDTH, 0,
                                     MenuColors::ARROW_AREA_WIDTH, h));
    }
}

void MenuItem::draw() {
    if (!getVisible()) return;
    if (m_type == MenuItemType::Separator) return; // 分隔线由MenuPanel绘制

    SDL_Renderer* renderer = getRenderer();
    if (!renderer) return;

    SRect drawRect = getDrawRect();

    // 绘制hover背景
    if (m_hovered) {
        SDL_SetRenderDrawColor(renderer,
            MenuColors::ITEM_HOVER_BG.r,
            MenuColors::ITEM_HOVER_BG.g,
            MenuColors::ITEM_HOVER_BG.b,
            MenuColors::ITEM_HOVER_BG.a);
        SDL_FRect bgRect = {drawRect.left, drawRect.top, drawRect.width, drawRect.height};
        SDL_RenderFillRect(renderer, &bgRect);
    }

    // 绘制勾选标记
    if (m_checked) {
        SDL_SetRenderDrawColor(renderer,
            MenuColors::ITEM_TEXT.r,
            MenuColors::ITEM_TEXT.g,
            MenuColors::ITEM_TEXT.b,
            MenuColors::ITEM_TEXT.a);
        // 绘制简单的勾选符号 "✓"
        float cx = drawRect.left + MenuColors::ICON_AREA_WIDTH / 2.0f;
        float cy = drawRect.top + drawRect.height / 2.0f;
        // 简单的勾选线条
        SDL_RenderLine(renderer, cx - 4, cy, cx - 1, cy + 3);
        SDL_RenderLine(renderer, cx - 1, cy + 3, cx + 4, cy - 3);
    }

    // 绘制标题
    if (m_captionLabel) {
        m_captionLabel->draw();
    }

    // 绘制快捷键
    if (m_shortcutLabel) {
        m_shortcutLabel->draw();
    }

    // 绘制子菜单箭头
    if (m_arrowLabel) {
        m_arrowLabel->draw();
    }
}

bool MenuItem::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;
    if (m_type == MenuItemType::Separator) return false;

    if (EventQueue::isPositionEvent(event->m_eventName)) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            SRect drawRect = getDrawRect();
            if (drawRect.contains(pos->x, pos->y)) {
                switch (event->m_eventName) {
                    case EventName::MOUSE_LBUTTON_DOWN:
                        if (m_type == MenuItemType::Normal && m_onClick) {
                            closeMenuChain();
                            m_onClick(dynamic_pointer_cast<MenuItem>(getThis()));
                        }
                        return true;
                    case EventName::MOUSE_MOVING:
                        if (!m_hovered) {
                            m_hovered = true;
                        }
                        return true;
                    default:
                        break;
                }
                return true;
            } else {
                m_hovered = false;
            }
        } catch (...) {
            return false;
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

// ==================== MenuPanel 实现 ====================

MenuPanel::MenuPanel(Control *parent, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_itemHeight(MenuColors::getItemHeight())
    , m_iconAreaWidth(MenuColors::ICON_AREA_WIDTH)
    , m_shortcutAreaWidth(0)
    , m_arrowAreaWidth(MenuColors::ARROW_AREA_WIDTH)
    , m_hoveredIndex(-1)
    , m_visible(false)
    , m_openSubMenu(nullptr)
    , m_bgColor(MenuColors::PANEL_BG)
    , m_borderColor(MenuColors::PANEL_BORDER)
    , m_hoverColor(MenuColors::ITEM_HOVER_BG)
    , m_separatorColor(MenuColors::SEPARATOR)
    , m_shadowRadius(MenuColors::PANEL_SHADOW_BLUR)
{
    setRect(SRect(0, 0, 0, 0));
    setBorderVisible(true);
}

MenuPanel::~MenuPanel() = default;

void MenuPanel::addItem(shared_ptr<MenuItem> item) {
    if (!item) return;
    item->setParent(this);
    m_items.push_back(item);
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
    // 重置所有菜单项的hover状态
    for (auto& item : m_items) {
        // MenuItem没有直接的setHovered方法，通过绘制时检查hoveredIndex来处理
    }
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

    for (auto& item : m_items) {
        if (item->getType() == MenuItemType::Separator) continue;
        if (item->hasSubMenu()) hasSubMenu = true;

        // 获取标题宽度
        if (item->m_captionLabel) {
            float w = item->m_captionLabel->getHotRect().width;
            if (w > maxCaptionWidth) maxCaptionWidth = w;
        }
        // 获取快捷键宽度
        if (item->m_shortcutLabel) {
            float w = item->m_shortcutLabel->getHotRect().width;
            if (w > maxShortcutWidth) maxShortcutWidth = w;
        }
    }

    m_shortcutAreaWidth = (std::max)(maxShortcutWidth, MenuColors::SHORTCUT_MIN_WIDTH);

    // 计算面板宽度
    float panelWidth = MenuColors::ITEM_LEFT_PADDING
                     + maxCaptionWidth
                     + (m_shortcutAreaWidth > 0 ? m_shortcutAreaWidth + 10.0f : 0)
                     + (hasSubMenu ? m_arrowAreaWidth : 0)
                     + MenuColors::ITEM_RIGHT_PADDING;

    // 计算面板高度
    float panelHeight = 0;
    for (auto& item : m_items) {
        if (item->getType() == MenuItemType::Separator) {
            panelHeight += MenuColors::SEPARATOR_HEIGHT + 2 * MenuColors::SEPARATOR_MARGIN;
        } else {
            panelHeight += m_itemHeight;
        }
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
            item->setRect(SRect(0, y, getRect().width, m_itemHeight));
            item->updateLabelPositions();
            y += m_itemHeight;
        }
    }
}

int MenuPanel::hitTest(float x, float y) {
    SRect drawRect = getDrawRect();
    // 转换为局部坐标
    float localX = x - drawRect.left;
    float localY = y - drawRect.top;

    float itemY = 0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        float itemHeight = (item->getType() == MenuItemType::Separator)
            ? MenuColors::SEPARATOR_HEIGHT + 2 * MenuColors::SEPARATOR_MARGIN
            : m_itemHeight;

        if (localY >= itemY && localY < itemY + itemHeight) {
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

    // 重置旧hover项
    if (m_hoveredIndex >= 0 && m_hoveredIndex < (int)m_items.size()) {
        m_items[m_hoveredIndex]->m_hovered = false;
    }

    m_hoveredIndex = index;

    // 设置新hover项
    if (m_hoveredIndex >= 0 && m_hoveredIndex < (int)m_items.size()) {
        auto& item = m_items[m_hoveredIndex];
        item->m_hovered = true;

        // 如果hover到子菜单项，展开子菜单
        if (item->hasSubMenu()) {
            SRect itemRect = item->getDrawRect();
            auto subMenu = item->getSubMenu();
            subMenu->setPosition(itemRect.right(), itemRect.top);
            setOpenSubMenu(subMenu);
        } else {
            setOpenSubMenu(nullptr);
        }
    } else {
        setOpenSubMenu(nullptr);
    }
}

void MenuPanel::closeWithChildren() {
    if (m_openSubMenu) {
        m_openSubMenu->closeWithChildren();
    }
    hide();
}

void MenuPanel::drawShadow() {
    SDL_Renderer* renderer = getRenderer();
    if (!renderer) return;

    SRect drawRect = getDrawRect();

    // 使用GraphTool绘制阴影
    GraphTool::DrawingContext dc(renderer);
    dc.setFillColor(GraphTool::SColor(
        MenuColors::PANEL_SHADOW.r / 255.0f,
        MenuColors::PANEL_SHADOW.g / 255.0f,
        MenuColors::PANEL_SHADOW.b / 255.0f,
        MenuColors::PANEL_SHADOW.a / 255.0f));

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

    SDL_Renderer* renderer = getRenderer();
    if (!renderer) return;

    SRect drawRect = getDrawRect();

    // 1. 绘制阴影
    drawShadow();

    // 2. 绘制背景（圆角矩形）
    {
        GraphTool::DrawingContext dc(renderer);
        dc.setFillColor(GraphTool::SColor(
            m_bgColor.r / 255.0f, m_bgColor.g / 255.0f,
            m_bgColor.b / 255.0f, m_bgColor.a / 255.0f));
        dc.drawRoundedRect(::SRect(drawRect.left, drawRect.top,
            drawRect.width, drawRect.height), MenuColors::PANEL_RADIUS, true);
    }

    // 3. 绘制边框
    {
        GraphTool::DrawingContext dc(renderer);
        dc.setPen(GraphTool::SPen(
            GraphTool::SColor(m_borderColor.r / 255.0f, m_borderColor.g / 255.0f,
                             m_borderColor.b / 255.0f, m_borderColor.a / 255.0f), 1.0f));
        dc.drawRoundedRect(::SRect(drawRect.left, drawRect.top,
            drawRect.width, drawRect.height), MenuColors::PANEL_RADIUS, false);
    }

    // 4. 绘制菜单项和分隔线
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        if (item->getType() == MenuItemType::Separator) {
            // 绘制分隔线
            SRect itemRect = item->getDrawRect();
            SDL_SetRenderDrawColor(renderer,
                m_separatorColor.r, m_separatorColor.g,
                m_separatorColor.b, m_separatorColor.a);
            SDL_FRect lineRect = {
                drawRect.left + MenuColors::SEPARATOR_MARGIN,
                itemRect.top + MenuColors::SEPARATOR_MARGIN,
                drawRect.width - 2 * MenuColors::SEPARATOR_MARGIN,
                MenuColors::SEPARATOR_HEIGHT
            };
            SDL_RenderFillRect(renderer, &lineRect);
        } else {
            // 先绘制hover背景
            if (item->m_hovered) {
                SRect itemRect = item->getDrawRect();
                SDL_SetRenderDrawColor(renderer,
                    m_hoverColor.r, m_hoverColor.g,
                    m_hoverColor.b, m_hoverColor.a);
                SDL_FRect bgRect = {drawRect.left, itemRect.top, drawRect.width, itemRect.height};
                SDL_RenderFillRect(renderer, &bgRect);
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

    if (EventQueue::isPositionEvent(event->m_eventName)) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;

            SRect drawRect = getDrawRect();
            if (drawRect.contains(pos->x, pos->y)) {
                int index = hitTest(pos->x, pos->y);
                switch (event->m_eventName) {
                    case EventName::MOUSE_MOVING:
                        setHoveredIndex(index);
                        return true;
                    case EventName::MOUSE_LBUTTON_DOWN:
                        if (index >= 0) {
                            auto& item = m_items[index];
                            if (item->getType() == MenuItemType::Normal && item->m_onClick) {
                                item->closeMenuChain();
                                item->m_onClick(dynamic_pointer_cast<MenuItem>(item->getThis()));
                                return true;
                            }
                        }
                        return true;
                    default:
                        break;
                }
                return true;
            }
        } catch (...) {
            return false;
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
    , m_barHeight(MenuColors::getBarHeight())
    , m_hoveredIndex(-1)
    , m_activeIndex(-1)
    , m_menuMode(false)
    , m_bgColor(MenuColors::BAR_BG)
    , m_textColor(MenuColors::BAR_TEXT)
    , m_hoverBgColor(MenuColors::BAR_HOVER_BG)
    , m_hoverTextColor(MenuColors::BAR_TEXT)
    , m_activeBgColor(MenuColors::BAR_ACTIVE_BG)
{
    setRect(SRect(0, 0, 0, m_barHeight));
    setBorderVisible(false);
}

MenuBar::~MenuBar() = default;

void MenuBar::setParent(Control *parent) {
    ControlImpl::setParent(parent);
    if (parent) {
        setRect(SRect(0, 0, parent->getRect().width, m_barHeight));
        layoutEntries();
    }
}

void MenuBar::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    layoutEntries();
}

void MenuBar::addMenu(const string& caption, shared_ptr<MenuPanel> panel) {
    MenuEntry entry;
    entry.caption = caption;
    entry.panel = panel;
    entry.panel->setParent(this);
    entry.panel->hide();

    // 创建菜单项Label
    entry.label = LabelBuilder(this, {0, 0, 0, m_barHeight})
        .setFont(MenuColors::MENU_FONT)
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setFontSize((int)MenuColors::g_menuTextSize)
        .setCaption(caption)
        .setTextStateColor(StateColor(StateColor::Type::Text)
            .setNormal(m_textColor)
            .setHover(m_hoverTextColor)
            .setPressed(m_hoverTextColor))
        .setEnableExpand(true)
        .build();

    m_entries.push_back(std::move(entry));
    layoutEntries();
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
    for (auto& entry : m_entries) {
        float entryWidth = 0;
        if (entry.label) {
            entryWidth = entry.label->getHotRect().width + 2 * MenuColors::ITEM_LEFT_PADDING;
        }
        entry.hitRect = SRect(x, 0, entryWidth, m_barHeight);
        if (entry.label) {
            entry.label->setRect(SRect(x, 0, entryWidth, m_barHeight));
        }
        x += entryWidth;
    }
    // 更新菜单栏宽度
    if (getParent()) {
        ControlImpl::setRect(SRect(0, 0, getParent()->getRect().width, m_barHeight));
    }
}

int MenuBar::hitTest(float x, float y) {
    SRect drawRect = getDrawRect();
    float localX = x - drawRect.left;
    float localY = y - drawRect.top;

    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].hitRect.contains(localX, localY)) {
            return (int)i;
        }
    }
    return -1;
}

void MenuBar::openMenu(int index) {
    // 关闭当前打开的菜单
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        m_entries[m_activeIndex].panel->hide();
    }

    m_activeIndex = index;
    if (index >= 0 && index < (int)m_entries.size()) {
        auto& entry = m_entries[index];
        SRect hitRect = entry.hitRect;
        SRect drawRect = getDrawRect();
        entry.panel->setPosition(drawRect.left + hitRect.left, drawRect.top + hitRect.height);
        entry.panel->show();
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
        m_entries[m_activeIndex].panel->hide();
    }
    m_activeIndex = -1;
    m_hoveredIndex = -1;
}

void MenuBar::closeAllMenus() {
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        m_entries[m_activeIndex].panel->closeWithChildren();
        m_entries[m_activeIndex].panel->hide();
    }
    m_activeIndex = -1;
}

void MenuBar::setBarHeight(float height) {
    m_barHeight = height;
    setRect(SRect(getRect().left, getRect().top, getRect().width, m_barHeight));
    layoutEntries();
}

void MenuBar::setItemHeightRatio(float ratio) {
    if (ratio < MenuColors::MIN_HEIGHT_RATIO) ratio = MenuColors::MIN_HEIGHT_RATIO;
    if (ratio > MenuColors::MAX_HEIGHT_RATIO) ratio = MenuColors::MAX_HEIGHT_RATIO;
    MenuColors::g_heightRatio = ratio;
}

void MenuBar::setFontSize(float size) {
    MenuColors::g_menuTextSize = size;
}

float MenuBar::getFontSize() {
    return MenuColors::g_menuTextSize;
}

void MenuBar::draw() {
    if (!getVisible()) return;

    SDL_Renderer* renderer = getRenderer();
    if (!renderer) return;

    SRect drawRect = getDrawRect();

    // 1. 绘制菜单栏背景
    SDL_SetRenderDrawColor(renderer, m_bgColor.r, m_bgColor.g, m_bgColor.b, m_bgColor.a);
    SDL_FRect bgRect = {drawRect.left, drawRect.top, drawRect.width, drawRect.height};
    SDL_RenderFillRect(renderer, &bgRect);

    // 2. 绘制菜单项
    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto& entry = m_entries[i];
        SRect hitRect = entry.hitRect;

        // 绘制hover/active背景
        if ((int)i == m_activeIndex) {
            SDL_SetRenderDrawColor(renderer,
                m_activeBgColor.r, m_activeBgColor.g,
                m_activeBgColor.b, m_activeBgColor.a);
            SDL_FRect itemBg = {drawRect.left + hitRect.left, drawRect.top,
                                hitRect.width, hitRect.height};
            SDL_RenderFillRect(renderer, &itemBg);
        } else if ((int)i == m_hoveredIndex) {
            SDL_SetRenderDrawColor(renderer,
                m_hoverBgColor.r, m_hoverBgColor.g,
                m_hoverBgColor.b, m_hoverBgColor.a);
            SDL_FRect itemBg = {drawRect.left + hitRect.left, drawRect.top,
                                hitRect.width, hitRect.height};
            SDL_RenderFillRect(renderer, &itemBg);
        }

        // 绘制文字
        if (entry.label) {
            entry.label->setRect(SRect(drawRect.left + hitRect.left, drawRect.top,
                                       hitRect.width, hitRect.height));
            entry.label->draw();
        }
    }

    // 3. 绘制底部分隔线
    SDL_SetRenderDrawColor(renderer, MenuColors::PANEL_BORDER.r,
        MenuColors::PANEL_BORDER.g, MenuColors::PANEL_BORDER.b,
        MenuColors::PANEL_BORDER.a);
    SDL_RenderLine(renderer, drawRect.left, drawRect.top + drawRect.height - 1,
                   drawRect.left + drawRect.width, drawRect.top + drawRect.height - 1);

    // 4. 绘制打开的下拉菜单
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        m_entries[m_activeIndex].panel->draw();
    }
}

bool MenuBar::handleEvent(shared_ptr<Event> event) {
    if (!getEnable() || !getVisible()) return false;

    // 先让打开的下拉菜单处理事件
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_entries.size()) {
        if (m_entries[m_activeIndex].panel->handleEvent(event)) return true;
    }

    if (EventQueue::isPositionEvent(event->m_eventName)) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;

            SRect drawRect = getDrawRect();
            int index = hitTest(pos->x, pos->y);

            switch (event->m_eventName) {
                case EventName::MOUSE_LBUTTON_DOWN:
                    if (index >= 0) {
                        if (m_menuMode && m_activeIndex == index) {
                            // 再次点击同一菜单项，退出菜单模式
                            exitMenuMode();
                        } else {
                            // 进入菜单模式
                            enterMenuMode(index);
                        }
                        return true;
                    } else if (!drawRect.contains(pos->x, pos->y)) {
                        // 点击菜单栏外部
                        bool inPanel = false;
                        if (m_activeIndex >= 0) {
                            inPanel = m_entries[m_activeIndex].panel->isContainsPoint(pos->x, pos->y);
                        }
                        if (!inPanel) {
                            exitMenuMode();
                            return false; // 让其他控件也能处理
                        }
                    }
                    break;

                case EventName::MOUSE_MOVING:
                    if (m_menuMode) {
                        // 菜单模式中，鼠标在菜单栏上移动自动切换
                        if (index >= 0 && index != m_activeIndex) {
                            switchMenu(index);
                        }
                        return true;
                    } else {
                        // 非菜单模式，仅更新hover状态
                        m_hoveredIndex = index;
                    }
                    break;

                default:
                    break;
            }

            // 检查是否在菜单区域内
            if (drawRect.contains(pos->x, pos->y)) return true;
            if (m_menuMode && m_activeIndex >= 0) {
                if (m_entries[m_activeIndex].panel->isContainsPoint(pos->x, pos->y)) {
                    return true;
                }
            }
        } catch (...) {
            return false;
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
