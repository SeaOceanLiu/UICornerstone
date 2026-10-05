// ============================================================================
// StatusBar.cpp -- 状态栏控件实现（design/StatusBar_Design.md）
// left/right 两组布局；点击 item 向上弹出共享 MenuPanel；外部点击关闭。
// ============================================================================
#include "StatusBar.h"
#include "TextDraw.h"
#include "Menu.h"
#include "PropertyNames.h"
#include "RenderDevice.h"
#include "TextRenderer.h"

using std::string;
using std::vector;
using std::shared_ptr;
using std::function;

// ── 局域常量（design-rules §2：业务常量具名，控件局域文件级 constexpr）──
static constexpr SColor kVSCodeBlue      = SColor(0, 122, 204);    // 缺省底色
// P0-64②：控件级 hover 色（属性键 "hover" 可配；缺省原常量值）
static constexpr SColor kTextColor       = SColor(235, 235, 235);  // 段文字
static constexpr float kIconFontRatio    = 1.4f;   // 图标槽边长 = 字号×1.4
static constexpr float kIconLeftPad      = 4.0f;   // 段左缘→图标间距
static constexpr float kIconTextGap      = 4.0f;   // 图标→文字间距
static constexpr float kTextEstRatio     = 0.6f;   // 字体未就绪时字宽估算系数

StatusBar::StatusBar(Control* parent, const SRect& rect, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
{
    m_ctlType = ControlType::StatusBar;
    m_rect = rect;
    m_itemHeight = rect.height;
    m_textColor.setNormal(kTextColor);             // P0-26：段文字缺省色（原常量迁入四态）
    m_textShadowColor.setNormal(SColor(0, 0, 0, 120));
    setNormalStateBGColor(kVSCodeBlue);            // VSCode 状态栏蓝
}

// ── 数据操作 ──
void StatusBar::addStatusItem(const string& id, const string& text, bool rightAlign) {
    StatusItem item;
    item.id = id;
    item.text = text;
    item.rightAlign = rightAlign;
    m_items.push_back(std::move(item));
    relayout();
}
// P0-55：态位与回退链（显式设过的态 -> 该段 normal -> 控件级四态）
static uint8_t statusStateBit(ControlState s) {
    switch (s) {
        case ControlState::Hover:    return 2;
        case ControlState::Pressed:  return 4;
        case ControlState::Disabled: return 8;
        default:                     return 1;
    }
}
static SColor resolveItemTextColor(StatusItem& item, StateColor& ctrl, ControlState st) {
    if (!item.textColorMask) return ControlImpl::resolveStateColor(ctrl, st);
    if (item.textColorMask & statusStateBit(st)) {
        switch (st) {
            case ControlState::Hover:    return item.textColor.getHover();
            case ControlState::Pressed:  return item.textColor.getPressed();
            case ControlState::Disabled: return item.textColor.getDisabled();
            default:                     return item.textColor.getNormal();
        }
    }
    if (item.textColorMask & 1) return item.textColor.getNormal();   // 未设态 -> 该段 normal
    return ControlImpl::resolveStateColor(ctrl, st);                 // 该段未设色 -> 控件级
}

void StatusBar::setStatusItemTextColor(const string& id, SColor color, ControlState state) {
    for (auto& item : m_items) {
        if (item.id != id) continue;
        switch (state) {
            case ControlState::Hover:    item.textColor.setHover(color);    break;
            case ControlState::Pressed:  item.textColor.setPressed(color);  break;
            case ControlState::Disabled: item.textColor.setDisabled(color); break;
            default:                     item.textColor.setNormal(color);   break;
        }
        item.textColorMask |= statusStateBit(state);
        return;
    }
}
void StatusBar::setStatusItemFontSize(const string& id, float size) {   // P0-58
    for (auto& item : m_items) {
        if (item.id != id) continue;
        if (item.fontSize != size) { item.fontSize = size; relayout(); }
        return;
    }
}
void StatusBar::setStatusItemHoverBackgroundColor(const string& id, SColor color) {   // P0-64④
    for (auto& item : m_items) {
        if (item.id != id) continue;
        item.hoverBackground = color;
        item.hasHoverBackground = true;
        return;
    }
}
void StatusBar::setStatusItemFontName(const string& id, FontName name) {   // P0-63⑤
    for (auto& item : m_items) {
        if (item.id != id) continue;
        if (item.fontName != name || !item.hasFontName) {
            item.fontName = name;
            item.hasFontName = true;
            m_itemFonts.clear();
            relayout();
        }
        return;
    }
}
void StatusBar::setStatusItemTextShadow(const string& id, SColor color, float offsetX, float offsetY) {   // P0-58
    for (auto& item : m_items) {
        if (item.id != id) continue;
        item.shadowColor = color;
        item.hasShadow = true;
        item.shadowOffset = SPoint(offsetX, offsetY);
        return;
    }
}

void StatusBar::setStatusItemBackgroundColor(const string& id, SColor color) {
    for (auto& item : m_items) {
        if (item.id != id) continue;
        item.background = color;
        item.hasBackground = true;
        return;
    }
}

void StatusBar::updateStatusItemText(const string& id, const string& text) {
    for (auto& item : m_items) {
        if (item.id == id) {
            if (item.text != text) { item.text = text; relayout(); }   // P0-53：段宽随文本变化
            return;
        }
    }
}
void StatusBar::removeStatusItem(const string& id) {
    m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
        [&](const StatusItem& it) { return it.id == id; }), m_items.end());
    relayout();
}
void StatusBar::setStatusItemMenu(const string& id, shared_ptr<MenuPanel> panel) {
    for (auto& item : m_items) {
        if (item.id == id) { item.menuPanel = std::move(panel); return; }
    }
}
bool StatusBar::isPopupOpen() const {
    return m_popupPanel && m_popupPanel->getVisible();
}
void StatusBar::setStatusItemLeadingControl(const string& id, shared_ptr<Control> ctl) {
    for (auto& item : m_items) {
        if (item.id == id) { item.leadingControl = std::move(ctl); relayout(); return; }
    }
}
void StatusBar::setStatusItemOnClick(const string& id, function<void(shared_ptr<StatusItem>)> cb) {
    for (auto& item : m_items) {
        if (item.id == id) { item.onClick = std::move(cb); return; }
    }
}
StatusItem* StatusBar::getStatusItem(const string& id) {
    for (auto& item : m_items) if (item.id == id) return &item;
    return nullptr;
}

void StatusBar::setFontSize(float size) {
    if (size > 0 && m_fontSize != size) {
        m_fontSize = size;
        m_font.reset();          // P0-57：失效缓存字体（否则 ensureFont 早退，字号不生效）
        m_itemFonts.clear();     // P0-58：段级字体缓存随控件级字号失效
        relayout();
    }
}
void StatusBar::setItemHeight(float px) {
    if (px > 0 && m_itemHeight != px) { m_itemHeight = px; m_rect.height = px; relayout(); }
}

void StatusBar::ensureFont() {
    if (m_font) return;
    TextRenderer* renderer = getTextRenderer();
    ResourceProvider* provider = getResourceProvider();
    if (!renderer || !provider) return;
    auto it = ConstDef::fontFiles.find(m_fontName);   // P0-26：字体名可配
    if (it == ConstDef::fontFiles.end()) return;
    string fontPath = it->second;   // P0-54：统一相对路径约定（与 Label/Actor 一致）
    auto data = provider->readFile(fontPath);
    if (data && !data->empty()) {
        int scaledSize = static_cast<int>(m_fontSize * getScaleXX());
        m_font = renderer->loadFontFromMemoryWithText(data->data(), data->size(), scaledSize, "W");
    }
}

// P0-58：段级字号字体（<=0 或未就绪回退控件级 m_font；按缩放后像素字号缓存）
SharedFont StatusBar::fontForSize(const StatusItem& item) {
    const FontName name = item.hasFontName ? item.fontName : m_fontName;   // P0-63⑤
    const float size = item.fontSize > 0.0f ? item.fontSize : m_fontSize;
    if (name == m_fontName && size == m_fontSize) return m_font;           // 继承控件级
    const int px = static_cast<int>(size * getScaleXX());
    if (px <= 0) return m_font;
    const std::string key = FontNameToString(name) + std::string("#") + std::to_string(px);
    auto it = m_itemFonts.find(key);
    if (it != m_itemFonts.end()) return it->second;
    if (!m_font) return m_font;   // 控件级字体未就绪：沿用（relayout 估算路径）
    TextRenderer* renderer = getTextRenderer();
    ResourceProvider* provider = getResourceProvider();
    if (!renderer || !provider) return m_font;
    auto fit = ConstDef::fontFiles.find(name);
    if (fit == ConstDef::fontFiles.end()) return m_font;
    auto data = provider->readFile(fit->second);   // P0-54：相对路径约定
    if (!data || data->empty()) return m_font;
    SharedFont f = renderer->loadFontFromMemoryWithText(data->data(), data->size(), px, "W");
    if (!f) return m_font;
    m_itemFonts[key] = f;
    return f;
}

// ── 布局 ──
void StatusBar::relayout() {
    if (m_items.empty()) return;
    ensureFont();
    float leftX = m_padding;
    const float cy = (m_rect.height - m_itemHeight) / 2.f;
    TextRenderer* renderer = getTextRenderer();

    // 先算左组
    for (auto& item : m_items) {
        if (item.rightAlign) continue;
        float w = m_itemHeight;
        if (item.leadingControl) w += m_fontSize * kIconFontRatio;
        SharedFont f = fontForSize(item);   // P0-58/63⑤：按段级字体测量
        if (renderer && f) w += renderer->measureText(f.get(), item.text).width;
        else w += item.text.length() * (item.fontSize > 0.0f ? item.fontSize : m_fontSize) * kTextEstRatio;
        item.hitRect = SRect(leftX, cy, w + m_spacing, m_itemHeight);
        leftX += w + m_spacing;
    }

    // 右组从右向左
    float rightX = m_rect.width - m_padding;
    for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
        auto& item = m_items[i];
        if (!item.rightAlign) continue;
        float w = m_itemHeight;
        if (item.leadingControl) w += m_fontSize * kIconFontRatio;
        SharedFont f = fontForSize(item);   // P0-58/63⑤：按段级字体测量
        if (renderer && f) w += renderer->measureText(f.get(), item.text).width;
        else w += item.text.length() * (item.fontSize > 0.0f ? item.fontSize : m_fontSize) * kTextEstRatio;
        item.hitRect = SRect(rightX - w, cy, w + m_spacing, m_itemHeight);
        rightX -= w + m_spacing;
    }
}

int StatusBar::hitTestIndex(float screenX, float screenY) const {
    // 命中测试入参为屏幕坐标：逆变换到本地布局空间（drawRect 原点 + 1/scale），二维判定
    auto* self = const_cast<StatusBar*>(this);      // getDrawRect/getScaleXX 为非 const 接口
    const SRect dr = self->getDrawRect();
    const float sx = self->getScaleXX() != 0.f ? self->getScaleXX() : 1.f;
    const float sy = self->getScaleYY() != 0.f ? self->getScaleYY() : 1.f;
    const float x = (screenX - dr.left) / sx;
    const float y = (screenY - dr.top) / sy;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const auto& r = m_items[i].hitRect;
        if (x >= r.left && x < r.left + r.width &&
            y >= r.top && y < r.top + r.height) return i;
    }
    return -1;
}

// ── 绘制 ──
void StatusBar::draw(void) {
    if (!m_visible) return;   // 可见性守卫（防止不可见时仍 ensureFont/relayout）
    const bool hadFont = m_font != nullptr;
    ensureFont();
    if (!hadFont && m_font && !m_items.empty()) relayout();   // 字体首次就绪→按实测宽度重排
    ControlImpl::beforeDraw();                 // 背景（蓝）/ 边框
    RenderDevice* dev = getRenderDevice();
    TextRenderer* renderer = getTextRenderer();
    const SRect dr = getDrawRect();            // 缩放后绘制区
    const float sx = getScaleXX(), sy = getScaleYY();
    const float ox = dr.left, oy = dr.top;

    if (dev) {
        // P0-64①/④：hover 处理移至段循环内（段 bg 之后，避免背景覆盖悬停反馈）
        int hoverIdx = 0;
        for (auto& item : m_items) {
            // P0-55：段背景（铺满 hitRect，VSCode 风格；未设置不绘制）
            if (item.hasBackground) {
                const auto& r = item.hitRect;
                dev->setDrawColor(item.background);
                dev->fillRect(SRect(ox + r.left * sx, oy + r.top * sy, r.width * sx, r.height * sy));
            }
            if (hoverIdx == m_hoveredItem) {   // P0-64：悬停反馈（显式 → 原色；有段 bg → 叠加；无 bg → 实色）
                const auto& r = item.hitRect;
                if (item.hasHoverBackground) {
                    dev->setDrawColor(item.hoverBackground);
                    dev->fillRect(SRect(ox + r.left * sx, oy + r.top * sy, r.width * sx, r.height * sy));
                } else if (item.hasBackground) {
                    dev->setDrawColor(SColor(m_hoverColor.red(), m_hoverColor.green(), m_hoverColor.blue(),
                                             ConstDef::LIST_HIGHLIGHT_OVERLAY_ALPHA));
                    dev->fillRect(SRect(ox + r.left * sx, oy + r.top * sy, r.width * sx, r.height * sy));
                } else {
                    dev->setDrawColor(m_hoverColor);
                    dev->fillRect(SRect(ox + r.left * sx, oy + r.top * sy, r.width * sx, r.height * sy));
                }
            }
            ++hoverIdx;
            // leadingControl 未挂 bar 子树（无父复合）→ setRect 用【绝对坐标】
            // = drawRect 原点 + 本地布局 × scale
            if (item.leadingControl) {
                // 图标框：槽内几何居中（(itemHeight-isz)/2），内容无关基准；
                // 文字型图标内容的基线偏移属内容自身特性（Label 行偏移为常量）
                const float isz = m_fontSize * kIconFontRatio;
                item.leadingControl->setRect(SRect(
                    ox + (item.hitRect.left + kIconLeftPad) * sx,
                    oy + (item.hitRect.top + (m_itemHeight - isz) / 2.f) * sy,
                    isz * sx, isz * sy));
                item.leadingControl->draw();
            }
            SharedFont itemFont = fontForSize(item);   // P0-58/63⑤：段级字体
            if (renderer && itemFont) {
                const float tx = ox + (item.hitRect.left + kIconLeftPad
                                       + (item.leadingControl ? m_fontSize * kIconFontRatio + kIconTextGap : 0.f)) * sx;
                const float hfh = static_cast<float>(renderer->getFontHeight(itemFont.get()));
                const float ty = oy + (item.hitRect.top + (m_itemHeight - hfh) / 2.f) * sy;   // P0-58：按段字体居中
                SColor itemTextColor = resolveItemTextColor(item, m_textColor, getState());   // P0-55：段色回退链
                // P0-58：阴影优先级 = 段级（hasShadow）-> 控件级
                const bool shadowOn = item.hasShadow ? true : m_shadowEnabled;
                const SPoint so = item.hasShadow ? item.shadowOffset : m_shadowOffset;
                const SColor sc = item.hasShadow ? item.shadowColor
                                                 : ControlImpl::resolveStateColor(m_textShadowColor, getState());
                TextDraw::withShadow(renderer, itemFont.get(), item.text,
                                     tx, ty, tx + so.x * sx, ty + so.y * sy,
                                     itemTextColor, shadowOn, sc);
            }
        }
    }

    ControlImpl::draw();                       // 子控件（弹窗 MenuPanel）
    ControlImpl::afterDraw();
}

// ── 弹窗 ──
void StatusBar::openPopup(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size())) return;
    auto& item = m_items[itemIndex];
    if (!item.menuPanel) return;

    // 直接挂载并显示该 item 自带的菜单面板（共享复用，免复制菜单项）
    if (m_popupPanel != item.menuPanel) {
        if (m_popupPanel) removeControl(m_popupPanel);
        m_popupPanel = item.menuPanel;
        addControl(m_popupPanel);
        m_popupPanel->setContext(getContext());
        m_popupPanel->create();
    }

    // 菜单语义：点击任意项关闭弹窗。MenuItem::handleEvent 仅在 onClick 非空时
    // 走 closeMenuChain()，故为无回调的项补 no-op onClick（用户回调不受影响）。
    for (auto& mi : item.menuPanel->getItems()) {
        if (mi && !mi->getOnClick())
            mi->setOnClick([](shared_ptr<MenuItem>) {});
    }

    item.menuPanel->recalculateSize();
    const float panelH = item.menuPanel->getRect().height;

    // 向上定位：面板底缘贴 item 顶缘。
    // 坐标系注意：m_popupPanel 是 bar 的子控件，getDrawRect() 会叠加父偏移
    // （ControlBase.cpp getDrawRect），故此处必须用【bar 相对坐标】。
    float localY = item.hitRect.top - panelH;
    // 顶部越界钳制（不出窗口顶部：相对坐标 < -bar.top 等价绝对越界）
    if (getDrawRect().top + localY * getScaleYY() < 0) localY = -getDrawRect().top / (getScaleYY() != 0.f ? getScaleYY() : 1.f);

    m_popupPanel->setPosition(item.hitRect.left, localY);
    m_popupPanel->recalculateSize();
    m_popupPanel->show();
}

void StatusBar::closePopup() {
    if (m_popupPanel) m_popupPanel->hide();
}

// ── 事件 ──
bool StatusBar::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (event->m_type == EventType::MouseMove) {
        m_hoveredItem = hitTestIndex(event->mousePos.x, event->mousePos.y);
    }

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        const int idx = hitTestIndex(event->mouseButton.x, event->mouseButton.y);
        if (idx >= 0) {
            applyPressState(true);   // P032：按下切态
            auto& item = m_items[idx];
            if (item.menuPanel) {
                openPopup(idx);
            } else {
                if (item.onClick) {
                    auto self = shared_from_this();
                    item.onClick(shared_ptr<StatusItem>(&item, [](StatusItem*){}));
                }
                fireCCallback(PropertyNames::kEventStatusItemClick, CCallbackData::Int, &idx);
            }
            return true;
        } else {
            // 点击 item 外部 → 关闭弹窗
            closePopup();
        }
    }
    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        applyPressState(false);      // P032：抬起复位
    }

    return ControlImpl::handleEvent(event);
}

void StatusBar::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    relayout();
}

// ── 属性系统 override ──
void StatusBar::setTextStateColor(StateColor stateColor) {   // P0-26：文本四态
    m_textColor = stateColor;
}
void StatusBar::setTextShadowStateColor(StateColor stateColor) {   // P0-26：阴影色四态
    m_textShadowColor = stateColor;
}

int StatusBar::setColorProperty(const char* prop, SColor color) {   // P0-26：文本族单态键
    if (strcmp(prop, PropertyNames::kTreeHover) == 0)    { m_hoverColor = color;           return 1; }   // P0-64②：控件级悬停色（通用键 "hover"）
    if (strcmp(prop, PropertyNames::kText) == 0)         { m_textColor.setNormal(color);   return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)    { m_textColor.setHover(color);    return 1; }
    if (strcmp(prop, PropertyNames::kTextPressed) == 0)  { m_textColor.setPressed(color);  return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0) { m_textColor.setDisabled(color); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)   { m_textShadowColor.setNormal(color); return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

int StatusBar::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kTreeHover) == 0)    { out = m_hoverColor;              return 1; }   // P0-64②
    if (strcmp(prop, PropertyNames::kText) == 0)         { out = m_textColor.getNormal();   return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)    { out = m_textColor.getHover();    return 1; }
    if (strcmp(prop, PropertyNames::kTextPressed) == 0)  { out = m_textColor.getPressed();  return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0) { out = m_textColor.getDisabled(); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)   { out = m_textShadowColor.getNormal(); return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int StatusBar::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { m_shadowEnabled = (value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}

int StatusBar::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { out = m_shadowEnabled ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int StatusBar::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        m_fontName = FontNameFromString(value);
        m_font.reset();
        if (m_isCreated) { ensureFont(); relayout(); }
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

int StatusBar::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_fontName); return 1; }
    return ControlImpl::getEnumProperty(prop, out);
}

int StatusBar::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0)      { setFontSize((float)value); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}
int StatusBar::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0)  { out = (int)m_fontSize;  return 1; }
    return ControlImpl::getIntProperty(prop, out);
}
int StatusBar::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { m_shadowOffset.x = value; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { m_shadowOffset.y = value; return 1; }
    if (strcmp(prop, PropertyNames::kItemHeight) == 0)     { setItemHeight(value); return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}
int StatusBar::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_shadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_shadowOffset.y; return 1; }
    if (strcmp(prop, PropertyNames::kItemHeight) == 0) { out = m_itemHeight; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

// ── StatusBarBuilder（声明式构建，LabelBuilder 同款惯例） ──

StatusBarBuilder::StatusBarBuilder(Control* parent, SRect rect, float xScale, float yScale)
    : m_bar(nullptr)
{
    m_bar = std::make_shared<StatusBar>(parent, rect, xScale, yScale);
}
StatusBarBuilder& StatusBarBuilder::setFontSize(float size)   { m_bar->setFontSize(size); return *this; }
StatusBarBuilder& StatusBarBuilder::setItemHeight(float px)   { m_bar->setItemHeight(px); return *this; }
StatusBarBuilder& StatusBarBuilder::addStatusItem(const std::string& id, const std::string& text, bool rightAlign) {
    m_bar->addStatusItem(id, text, rightAlign); return *this;
}
StatusBarBuilder& StatusBarBuilder::setStatusItemMenu(const std::string& id, std::shared_ptr<MenuPanel> panel) {
    m_bar->setStatusItemMenu(id, std::move(panel)); return *this;
}
StatusBarBuilder& StatusBarBuilder::setStatusItemLeadingControl(const std::string& id, std::shared_ptr<Control> ctl) {
    m_bar->setStatusItemLeadingControl(id, std::move(ctl)); return *this;
}
StatusBarBuilder& StatusBarBuilder::setStatusItemOnClick(const std::string& id, std::function<void(std::shared_ptr<StatusItem>)> cb) {
    m_bar->setStatusItemOnClick(id, std::move(cb)); return *this;
}
std::shared_ptr<StatusBar> StatusBarBuilder::build(void) {
    m_bar->create();
    return m_bar;
}
