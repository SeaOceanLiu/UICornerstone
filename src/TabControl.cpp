// ============================================================================
// TabControl.cpp -- 选项卡控件（四方向页签条 + 内容区，一体自绘）
// ============================================================================
#include "TabControl.h"
#include "TextDraw.h"
#include "RenderDevice.h"
#include "TextRenderer.h"
#include "ResourceProvider.h"
#include "ConstDef.h"
#include "PropertyNames.h"
#include "EventTypes.h"
#include "FocusManager.h"
#include "UIContext.h"

// ── 局域常量（design-rules §2：业务常量具名，控件局域文件级 constexpr）──
static constexpr SColor kSelTabColor     = SColor(45, 45, 52);     // 选中页签底
static constexpr SColor kHoverTabColor   = SColor(60, 60, 70);     // hover 页签底
static constexpr SColor kNormTabColor    = SColor(37, 37, 42);     // 常态页签底
static constexpr SColor kIndicatorColor  = SColor(0, 122, 204);    // 选中指示条
static constexpr SColor kSelTextColor    = SColor(235, 235, 235);  // 选中页签文字
static constexpr SColor kNormTextColor   = SColor(180, 180, 185);  // 常态页签文字
static constexpr SColor kContentBgColor  = SColor(50, 50, 56);     // 内容区底色
static constexpr float kBarFontRatio     = 1.4f;   // 条厚/图标槽 = 字号×1.4
static constexpr float kMinBarWidth      = 40.0f;  // Left/Right 条宽下限
static constexpr float kTextEstRatio     = 0.6f;   // 字体未就绪字宽估算系数
static constexpr float kIndicatorH       = 3.0f;   // 指示条厚（×scale）
static constexpr float kIconTextGap      = 4.0f;   // 图标→文字间距

TabControl::TabControl(Control* parent, const SRect& rect, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
{
    m_ctlType = ControlType::TabControl;
    m_rect = rect;
    m_textColor.setNormal(kNormTextColor);         // P0-26：常态页签缺省色（原常量迁入四态）
    m_textShadowColor.setNormal(SColor(0, 0, 0, 120));
    setFocusable(true);
    // 焦点作用域边界（FocusSystem_Design §4.3 / TabControl_Design §3.6）：
    // Tab 在本控件页内循环，Ctrl+Tab 跨作用域
    setFocusBoundary(true);
    relayout();
}

void TabControl::ensureFont() {
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

int TabControl::addTab(const string& title, shared_ptr<Control> page) {
    TabPage tp;
    tp.title = title;
    tp.page = page;
    m_tabs.push_back(std::move(tp));
    int idx = static_cast<int>(m_tabs.size()) - 1;
    if (page) {
        page->setParent(this);
        addControl(page);
    }
    relayout();
    if (m_currentIndex < 0) setCurrentIndex(0);
    return idx;
}

void TabControl::insertTab(int index, const string& title, shared_ptr<Control> page) {
    TabPage tp;
    tp.title = title;
    tp.page = page;
    if (index < 0) index = 0;
    if (index > static_cast<int>(m_tabs.size())) index = static_cast<int>(m_tabs.size());
    m_tabs.insert(m_tabs.begin() + index, std::move(tp));
    if (page) {
        page->setParent(this);
        addControl(page);
    }
    if (m_currentIndex >= index) ++m_currentIndex;
    relayout();
    if (m_currentIndex < 0) setCurrentIndex(0);
}

void TabControl::removeTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (m_tabs[index].page) removeControl(m_tabs[index].page);
    m_tabs.erase(m_tabs.begin() + index);
    if (m_currentIndex > index) --m_currentIndex;
    else if (m_currentIndex == index) {
        m_currentIndex = m_tabs.empty() ? -1 : std::min(m_currentIndex, static_cast<int>(m_tabs.size()) - 1);
    }
    relayout();
    applyCurrentPage();
}

void TabControl::setCurrentIndex(int index) {
    if (m_tabs.empty()) { m_currentIndex = -1; return; }
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (index == m_currentIndex) return;
    m_currentIndex = index;
    applyCurrentPage();
    if (m_onTabChange) m_onTabChange(std::static_pointer_cast<TabControl>(shared_from_this()), m_currentIndex);
    fireCCallback(PropertyNames::kEventTabChanged, CCallbackData::Int, &m_currentIndex);
}

void TabControl::applyCurrentPage() {
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        auto& tp = m_tabs[i];
        if (!tp.page) continue;
        tp.page->setRect(SRect(m_contentRect.left, m_contentRect.top,
                               m_contentRect.width, m_contentRect.height));
        tp.page->setVisible(i == m_currentIndex);
    }
}

void TabControl::setTabText(int index, const string& title) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    m_tabs[index].title = title;
    relayout();
}

void TabControl::setTabPage(int index, shared_ptr<Control> page) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (m_tabs[index].page) removeControl(m_tabs[index].page);
    m_tabs[index].page = page;
    if (page) {
        page->setParent(this);
        addControl(page);
    }
    applyCurrentPage();
}

void TabControl::setTabLeadingControl(int index, shared_ptr<Control> ctl) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    m_tabs[index].leadingControl = ctl;
    relayout();
}

void TabControl::setPosition(TabPosition pos) {
    m_position = pos;
    relayout();
}

void TabControl::setFontSize(float px) {
    m_fontSize = px;
    m_font.reset();
    ensureFont();
    relayout();
}

void TabControl::relayout() {
    const float barT = m_fontSize * kBarFontRatio + 2.f * m_padding;  // Top/Bottom 厚度
    const float fullW = m_rect.width, fullH = m_rect.height;
    SRect bar, content;

    if (m_position == TabPosition::Top) {
        bar = SRect(0, 0, fullW, barT);
        content = SRect(0, barT, fullW, fullH - barT);
    } else if (m_position == TabPosition::Bottom) {
        bar = SRect(0, fullH - barT, fullW, barT);
        content = SRect(0, 0, fullW, fullH - barT);
    } else if (m_position == TabPosition::Left) {
        bar = SRect(0, 0, 0, fullH);     // 宽在下方按最大标题计算
        content = SRect(0, 0, fullW, fullH);
    } else { // Right
        bar = SRect(fullW, 0, 0, fullH);
        content = SRect(0, 0, fullW, fullH);
    }

    // 左/右侧：按最大标题宽定 bar 宽
    if (m_position == TabPosition::Left || m_position == TabPosition::Right) {
        float maxW = kMinBarWidth;
        ensureFont();
        TextRenderer* r = getTextRenderer();
        for (auto& tp : m_tabs) {
            float w = r && m_font ? r->measureText(m_font.get(), tp.title).width : tp.title.length() * m_fontSize * kTextEstRatio;
            w += 2.f * m_padding;
            if (tp.leadingControl) w += m_fontSize * kBarFontRatio + kIconTextGap;
            if (w > maxW) maxW = w;
        }
        const float tabH = m_fontSize * kBarFontRatio + 2.f * m_padding;
        if (m_position == TabPosition::Left) {
            bar = SRect(0, 0, maxW, fullH);
            content = SRect(maxW, 0, fullW - maxW, fullH);
        } else {
            bar = SRect(fullW - maxW, 0, maxW, fullH);
            content = SRect(0, 0, fullW - maxW, fullH);
        }
        // 页签竖排
        float y = m_padding;
        for (auto& tp : m_tabs) {
            tp.tabRect = SRect(bar.left, y, bar.width, tabH);
            y += tabH;
        }
    } else {
        // 上/下：页签横排
        float x = m_padding;
        ensureFont();
        TextRenderer* r = getTextRenderer();
        for (auto& tp : m_tabs) {
            float w = r && m_font ? r->measureText(m_font.get(), tp.title).width : tp.title.length() * m_fontSize * kTextEstRatio;
            w += 2.f * m_padding;
            if (tp.leadingControl) w += m_fontSize * kBarFontRatio + kIconTextGap;
            tp.tabRect = SRect(x, bar.top, w, bar.height);
            x += w;
        }
    }

    m_contentRect = content;
    applyCurrentPage();
}

int TabControl::hitTestTab(float screenX, float screenY) const {
    // 命中测试入参为屏幕坐标：逆变换到本地布局空间
    auto* self = const_cast<TabControl*>(this);     // getDrawRect/getScaleXX 为非 const 接口
    const SRect dr = self->getDrawRect();
    const float sx = self->getScaleXX() != 0.f ? self->getScaleXX() : 1.f;
    const float sy = self->getScaleYY() != 0.f ? self->getScaleYY() : 1.f;
    const float x = (screenX - dr.left) / sx;
    const float y = (screenY - dr.top) / sy;
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        const auto& r = m_tabs[i].tabRect;
        if (x >= r.left && x < r.left + r.width &&
            y >= r.top && y < r.top + r.height)
            return i;
    }
    return -1;
}

void TabControl::drawTabBar() {
    RenderDevice* dev = getRenderDevice();
    if (!dev) return;
    const SRect dr = getDrawRect();            // 缩放后绘制区
    const float sx = getScaleXX(), sy = getScaleYY();
    const float ox = dr.left, oy = dr.top;
    ensureFont();
    TextRenderer* r = getTextRenderer();

    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        const auto& tp = m_tabs[i];
        const SRect& tr = tp.tabRect;           // 本地布局坐标
        const bool sel = (i == m_currentIndex);
        const bool hov = (i == m_hoveredTab);
        const float rx = ox + tr.left * sx, ry = oy + tr.top * sy;
        const float rw = tr.width * sx, rh = tr.height * sy;

        // 页签底色
        if (sel) dev->setDrawColor(kSelTabColor);
        else if (hov) dev->setDrawColor(kHoverTabColor);
        else dev->setDrawColor(kNormTabColor);
        dev->fillRect(SRect(rx, ry, rw, rh));

        // 选中指示条（贴页签条内侧；厚度随 scale）
        if (sel) {
            const float ind = kIndicatorH * sx;
            dev->setDrawColor(kIndicatorColor);
            if (m_position == TabPosition::Top)
                dev->fillRect(SRect(rx, ry + rh - ind, rw, ind));
            else if (m_position == TabPosition::Bottom)
                dev->fillRect(SRect(rx, ry, rw, ind));
            else if (m_position == TabPosition::Left)
                dev->fillRect(SRect(rx + rw - ind, ry, ind, rh));
            else // Right
                dev->fillRect(SRect(rx, ry, ind, rh));
        }

        // 图标（leadingControl 未挂 tab 子树 → 无父复合）→【绝对坐标】
        if (tp.leadingControl) {
            float isz = m_fontSize * kBarFontRatio;
            tp.leadingControl->setRect(SRect(
                ox + (tr.left + m_padding) * sx,
                oy + (tr.top + (tr.height - isz) / 2.f) * sy,
                isz * sx, isz * sy));
            tp.leadingControl->draw();
        }
        // 文字（字体已随 scale 加载，位置按本地 × scale）
        if (r && m_font) {
            const float tx = ox + (tr.left + m_padding
                                   + (tp.leadingControl ? m_fontSize * kBarFontRatio + kIconTextGap : 0.f)) * sx;
            const float ty = oy + (tr.top + (tr.height - m_fontSize) / 2.f) * sy;
            SColor tabTextColor = sel ? m_selectedTextColor
                                      : ControlImpl::resolveStateColor(m_textColor, getState());   // P0-26：常态四态 + selected-text
            TextDraw::withShadow(r, m_font.get(), tp.title,
                                 tx, ty, tx + m_shadowOffset.x * sx, ty + m_shadowOffset.y * sy,
                                 tabTextColor, m_shadowEnabled,
                                 ControlImpl::resolveStateColor(m_textShadowColor, getState()));
        }
    }
}

void TabControl::draw(void) {
    if (!m_visible) return;   // 可见性守卫（切页场景：页面 setVisible 为 false 时不被绘制）
    ControlImpl::beforeDraw();   // 整体背景 + 边框
    RenderDevice* dev = getRenderDevice();
    if (dev) {
        // 内容区底色（浅色）：本地内容区 × scale
        const SRect dr = getDrawRect();
        dev->setDrawColor(kContentBgColor);
        dev->fillRect(SRect(dr.left + m_contentRect.left * getScaleXX(),
                            dr.top + m_contentRect.top * getScaleYY(),
                            m_contentRect.width * getScaleXX(),
                            m_contentRect.height * getScaleYY()));
    }
    drawTabBar();
    ControlImpl::draw();         // 子控件（当前页）
    ControlImpl::afterDraw();
}

bool TabControl::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (event->m_type == EventType::MouseMove) {
        m_hoveredTab = hitTestTab(event->mousePos.x, event->mousePos.y);
        return ControlImpl::handleEvent(event);
    }

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        int idx = hitTestTab(event->mouseButton.x, event->mouseButton.y);
        if (idx >= 0) {
            // 点击页签 → 聚焦本控件（键盘导航只作用于焦点实例，多实例互不干扰）
            FocusManager* fm = m_context ? m_context->focusManager : nullptr;
            if (fm) fm->focusControl(this);
            applyPressState(true);   // P032：按下切态（text/shadow 四态可达）
            setCurrentIndex(idx);
            return true;
        }
    }
    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        applyPressState(false);      // P032：抬起复位
    }

    if (event->m_type == EventType::KeyDown) {
        // 键盘导航门控：仅焦点在自身时响应（否则多实例同屏会一起切换）
        if (!getFocused()) return ControlImpl::handleEvent(event);
        int n = static_cast<int>(m_tabs.size());
        bool vertical = (m_position == TabPosition::Left || m_position == TabPosition::Right);
        auto nav = [&](int delta) {
            if (n == 0) return;
            int cur = m_currentIndex < 0 ? 0 : m_currentIndex;
            int next = (cur + delta + n) % n;
            setCurrentIndex(next);
        };
        switch (event->keyEvent.keycode) {
            case KeyCode::Left:  if (!vertical) nav(-1); break;
            case KeyCode::Right: if (!vertical) nav(1);  break;
            case KeyCode::Up:    if (vertical)  nav(-1); break;
            case KeyCode::Down:  if (vertical)  nav(1);  break;
            case KeyCode::Home:  setCurrentIndex(0); break;
            case KeyCode::End:   if (n) setCurrentIndex(n - 1); break;
            default: break;
        }
    }

    return ControlImpl::handleEvent(event);
}

void TabControl::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    relayout();
}

// P032：page-background 四态 → 转发所有页 panel 背景
int TabControl::setStateColorProperty(const char* prop, StateColor stateColor) {
    if (strcmp(prop, PropertyNames::kPageBackground) == 0) {
        m_pageBackground = stateColor;
        for (auto& t : m_tabs) {
            if (auto* impl = dynamic_cast<ControlImpl*>(t.page.get())) impl->setBackgroundStateColor(stateColor);
        }
        return 1;
    }
    return ControlImpl::setStateColorProperty(prop, stateColor);
}
int TabControl::getStateColorProperty(const char* prop, StateColor& out) {
    if (strcmp(prop, PropertyNames::kPageBackground) == 0) { out = m_pageBackground; return 1; }
    return ControlImpl::getStateColorProperty(prop, out);
}

void TabControl::setTextStateColor(StateColor stateColor) {   // P0-26：页签文字四态
    m_textColor = stateColor;
}
void TabControl::setTextShadowStateColor(StateColor stateColor) {   // P0-26：阴影色四态
    m_textShadowColor = stateColor;
}

int TabControl::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kPageBackground) == 0) {   // P032：页背景（单态）
        m_pageBackground.setNormal(color);
        for (auto& t : m_tabs) {
            if (auto* impl = dynamic_cast<ControlImpl*>(t.page.get())) impl->setBackgroundStateColor(m_pageBackground);
        }
        return 1;
    }
    if (strcmp(prop, PropertyNames::kText) == 0)         { m_textColor.setNormal(color);   return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)    { m_textColor.setHover(color);    return 1; }
    if (strcmp(prop, PropertyNames::kTextPressed) == 0)  { m_textColor.setPressed(color);  return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0) { m_textColor.setDisabled(color); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)   { m_textShadowColor.setNormal(color); return 1; }
    if (strcmp(prop, PropertyNames::kSelectedText) == 0) { m_selectedTextColor = color;    return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

int TabControl::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kPageBackground) == 0) { out = m_pageBackground.getNormal(); return 1; }
    if (strcmp(prop, PropertyNames::kText) == 0)         { out = m_textColor.getNormal();   return 1; }
    if (strcmp(prop, PropertyNames::kTextHover) == 0)    { out = m_textColor.getHover();    return 1; }
    if (strcmp(prop, PropertyNames::kTextPressed) == 0)  { out = m_textColor.getPressed();  return 1; }
    if (strcmp(prop, PropertyNames::kTextDisabled) == 0) { out = m_textColor.getDisabled(); return 1; }
    if (strcmp(prop, PropertyNames::kTextShadow) == 0)   { out = m_textShadowColor.getNormal(); return 1; }
    if (strcmp(prop, PropertyNames::kSelectedText) == 0) { out = m_selectedTextColor;       return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int TabControl::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { m_shadowEnabled = (value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}

int TabControl::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) { out = m_shadowEnabled ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int TabControl::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        m_fontName = FontNameFromString(value);
        m_font.reset();
        if (m_isCreated) ensureFont();
        return 1;
    }
    if (strcmp(prop, PropertyNames::kTabPosition) == 0) {
        if (strcmp(value, PropertyNames::kTabPositionTop) == 0) setPosition(TabPosition::Top);
        else if (strcmp(value, PropertyNames::kTabPositionBottom) == 0) setPosition(TabPosition::Bottom);
        else if (strcmp(value, PropertyNames::kTabPositionLeft) == 0) setPosition(TabPosition::Left);
        else if (strcmp(value, PropertyNames::kTabPositionRight) == 0) setPosition(TabPosition::Right);
        else return 0;
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}
int TabControl::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_fontName); return 1; }
    if (strcmp(prop, PropertyNames::kTabPosition) == 0) {
        switch (m_position) {
            case TabPosition::Top:    out = PropertyNames::kTabPositionTop; break;
            case TabPosition::Bottom: out = PropertyNames::kTabPositionBottom; break;
            case TabPosition::Left:   out = PropertyNames::kTabPositionLeft; break;
            case TabPosition::Right:  out = PropertyNames::kTabPositionRight; break;
        }
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}
int TabControl::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kTabCurrentIndex) == 0) { setCurrentIndex(value); return 1; }
    if (strcmp(prop, PropertyNames::kFontSize) == 0)        { setFontSize((float)value); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}
int TabControl::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kTabCurrentIndex) == 0) { out = m_currentIndex; return 1; }
    if (strcmp(prop, PropertyNames::kFontSize) == 0)        { out = (int)m_fontSize; return 1; }
    return ControlImpl::getIntProperty(prop, out);
}
int TabControl::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { m_shadowOffset.x = value; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { m_shadowOffset.y = value; return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}
int TabControl::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) { out = m_shadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) { out = m_shadowOffset.y; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

// ── TabControlBuilder（声明式构建，LabelBuilder 同款惯例） ──

TabControlBuilder::TabControlBuilder(Control* parent, SRect rect, float xScale, float yScale)
    : m_tc(nullptr)
{
    m_tc = std::make_shared<TabControl>(parent, rect, xScale, yScale);
}
TabControlBuilder& TabControlBuilder::setPosition(TabPosition pos)  { m_tc->setPosition(pos); return *this; }
TabControlBuilder& TabControlBuilder::setFontSize(float px)         { m_tc->setFontSize(px); return *this; }
TabControlBuilder& TabControlBuilder::addTab(const std::string& title, std::shared_ptr<Control> page) {
    m_tc->addTab(title, std::move(page)); return *this;
}
TabControlBuilder& TabControlBuilder::setCurrentIndex(int index)    { m_tc->setCurrentIndex(index); return *this; }
TabControlBuilder& TabControlBuilder::setOnTabChange(TabControl::OnTabChange cb) { m_tc->setOnTabChange(std::move(cb)); return *this; }
std::shared_ptr<TabControl> TabControlBuilder::build(void) {
    m_tc->create();
    return m_tc;
}
