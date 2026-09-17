#define NOMINMAX
#include "ComboBox.h"
#include "Utility.h"
#include "MainWindow.h"
#include "EventQueue.h"
#include "PropertyNames.h"
#include "Bench.h"
#include "nlohmann/json.hpp"
#include <algorithm>

// ═══════════════════════════════════════════════════════════════
// ComboBox 构造函数
// ═══════════════════════════════════════════════════════════════
ComboBox::ComboBox(Control* parent, SRect rect, float xScale, float yScale)
    : EditBox(parent, rect, xScale, yScale)
    , m_selectedIndex(-1)
    , m_hoveredIndex(-1)
    , m_savedSelectedIndex(-1)
    , m_arrowWidth(ConstDef::COMBOBOX_DEFAULT_ARROW_WIDTH)
    , m_itemHeight(ConstDef::COMBOBOX_DEFAULT_ITEM_HEIGHT)
    , m_maxVisibleItems(ConstDef::COMBOBOX_DEFAULT_MAX_VISIBLE_ITEMS)
    , m_arrowColor(ConstDef::COMBOBOX_DEFAULT_ARROW_COLOR)
    , m_arrowHoverColor(ConstDef::COMBOBOX_DEFAULT_ARROW_HOVER_COLOR)
    , m_itemSelectedColor(ConstDef::COMBOBOX_DEFAULT_ITEM_SELECTED_COLOR)
    , m_itemHoverColor(ConstDef::COMBOBOX_DEFAULT_ITEM_HOVER_COLOR)
    , m_itemDisabledColor(ConstDef::COMBOBOX_DEFAULT_ITEM_DISABLED_COLOR)
    , m_listBgColor(ConstDef::COMBOBOX_DEFAULT_LIST_BG_COLOR)
    , m_listBorderColor(ConstDef::COMBOBOX_DEFAULT_LIST_BORDER_COLOR)
    , m_dropdownOffset(ConstDef::COMBOBOX_DROPDOWN_OFFSET)
{
    m_ctlType = ControlType::ComboBox;
    setPasswordMode(false);
    m_fontSize = ConstDef::COMBOBOX_DEFAULT_FONT_SIZE;
    m_margin.right += m_arrowWidth;
}

ComboBox::~ComboBox()
{
}

// ═══════════════════════════════════════════════════════════════
// create
// ═══════════════════════════════════════════════════════════════
void ComboBox::create()
{
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建

    EditBox::create();

    m_popup = make_shared<Popup>(nullptr, SRect(0, 0, 100, 100), m_xScale, m_yScale);
    m_popup->setRenderDevice(getRenderDevice());
    m_popup->setTextRenderer(getTextRenderer());
    m_popup->setResourceProvider(getResourceProvider());
    m_popup->setInputBackend(getInputBackend());
    // 浮层继承宿主实例上下文：Popup 以 nullptr 构造，无 setContext 传播路径
    m_popup->setContext(GET_CONTEXT);
    m_popup->setCloseOnClickOutside(true);
    m_popup->setCloseOnEsc(false);
    m_popup->setBorderVisible(false);
    m_popup->setTransparent(false);
    m_popup->setOnClose([this](shared_ptr<Popup> popup, DialogResult result) {
        if (result == DialogResult::Cancelled) {
            // 只读模式：取消时恢复打开前的选择；
            // 可编辑模式：保留当前输入内容（包括未匹配文本与选中状态）
            if (!m_editable) {
                restorePreviousSelection();
            }
        }
    });
    m_popup->create();
    m_popup->setVisible(false);

    m_listPanel = make_shared<ComboBoxListPanel>(nullptr,
        SRect(0, 0, 100, 100), 1.0f, 1.0f);
    m_listPanel->setOwner(this);
    m_listPanel->setRenderDevice(getRenderDevice());
    m_listPanel->setTextRenderer(getTextRenderer());
    m_listPanel->setResourceProvider(getResourceProvider());
    m_listPanel->setInputBackend(getInputBackend());
    // 列表面板同浮层：以 nullptr 构造，需继承宿主实例上下文
    m_listPanel->setContext(GET_CONTEXT);
    m_listPanel->create();

    m_scrollBar = make_shared<ScrollBar>(m_popup.get(),
        SRect(0, 0, ConstDef::SCROLLBAR_WIDTH, 100),
        ScrollBarOrientation::Vertical, 1.0f, 1.0f);
    m_scrollBar->setRenderDevice(getRenderDevice());
    m_scrollBar->setTextRenderer(getTextRenderer());
    m_scrollBar->setResourceProvider(getResourceProvider());
    m_scrollBar->setInputBackend(getInputBackend());
    m_scrollBar->setVisible(false);
    m_scrollBar->setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float, float, float) {
        syncListFromScroll();
    });
    m_scrollBar->create();

    m_popup->addControl(m_listPanel);
    m_popup->addControl(m_scrollBar);

    // 展开态事件 watcher：必须在此注册（早于任何 popup->open() 注册的
    // clickOutside watcher），保证点击 combo 自身区域时先处理收起逻辑，
    // 避免 popup 先关闭后 handleEvent 的 togglePopup 又重新打开
    m_context->eventQueue->addBeforeEventHandlingWatcher(
        EventType::KeyDown, getThis());
    m_context->eventQueue->addBeforeEventHandlingWatcher(
        EventType::MouseDown, getThis());
    m_watcherRegistered = true;
}

// ═══════════════════════════════════════════════════════════════
// draw
// ═══════════════════════════════════════════════════════════════
void ComboBox::draw()
{
    if (!m_visible) return;   // 可见性守卫
    EditBox::draw();

    SRect dr = getDrawRect();
    float sx = getScaleXX();

    float arrowRight  = dr.right() - ConstDef::COMBOBOX_ARROW_MARGIN * sx;
    float arrowLeft   = arrowRight - m_arrowWidth * sx;
    float arrowCenterX = (arrowLeft + arrowRight) / 2.0f;
    float arrowCenterY = dr.top + dr.height / 2.0f;

    float arrowMax = min(m_arrowWidth * sx, dr.height);
    float halfW = arrowMax * ConstDef::COMBOBOX_ARROW_WIDTH_RATIO;
    float halfH = arrowMax * ConstDef::COMBOBOX_ARROW_HEIGHT_RATIO;

    SColor arrowColor = m_arrowHovered ? m_arrowHoverColor : m_arrowColor;
    GET_RENDERDEVICE->setDrawColor(arrowColor);

    if (isPopupOpen()) {
        GET_RENDERDEVICE->drawTriangle(
            arrowCenterX - halfW, arrowCenterY + halfH,
            arrowCenterX + halfW, arrowCenterY + halfH,
            arrowCenterX,         arrowCenterY - halfH,
            arrowColor);
    } else {
        GET_RENDERDEVICE->drawTriangle(
            arrowCenterX - halfW, arrowCenterY - halfH,
            arrowCenterX + halfW, arrowCenterY - halfH,
            arrowCenterX,         arrowCenterY + halfH,
            arrowColor);
    }
}

// ═══════════════════════════════════════════════════════════════
// handleEvent
// ═══════════════════════════════════════════════════════════════
bool ComboBox::handleEvent(shared_ptr<Event> event)
{
    if (!m_enable || !m_visible) return false;

    // 只读模式：拦截文本输入，防止落到 EditBox::handleEvent 写入文字
    // （EditBox 仅检查 m_focused，不检查可编辑性）
    if (!m_editable && event->m_type == EventType::TextInput) {
        return true;
    }

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        // 只读模式：点击控件任意区域（箭头或正文）都打开下拉；
        // 编辑模式：仅点击箭头打开下拉，点正文进入文本编辑
        if (isContainsPoint(event->mouseButton.x, event->mouseButton.y) &&
            (!m_editable || isInArrowArea(event->mouseButton.x))) {
            togglePopup();
            return true;
        }
    }

    if (event->m_type == EventType::MouseMove) {
        if (isContainsPoint(event->mousePos.x, event->mousePos.y)) {
            bool inArrow = isInArrowArea(event->mousePos.x);
            if (inArrow != m_arrowHovered) {
                m_arrowHovered = inArrow;
            }
        } else {
            m_arrowHovered = false;
        }
    }

    if (!isPopupOpen() && getFocused()) {
        if (event->m_type == EventType::KeyDown) {
            int delta = 0;
            if (event->keyEvent.keycode == KeyCode::Down) delta = 1;
            else if (event->keyEvent.keycode == KeyCode::Up) delta = -1;

            if (delta != 0 && !m_items.empty()) {
                cycleSelection(delta);
                return true;
            }

            // 编辑模式：输入文字后回车 → 选中匹配项；
            // 无匹配 → 输入内容保留（视为无选中项）
            if (m_editable &&
                (event->keyEvent.keycode == KeyCode::Return ||
                 event->keyEvent.keycode == KeyCode::KPEnter)) {
                int idx = findItemByText(m_text);
                if (idx >= 0) {
                    selectItem(idx);
                } else {
                    m_selectedIndex = -1;
                }
                return true;
            }
        }

        // 编辑模式：输入文字 → 写入编辑框并高亮首个匹配项（type-ahead）
        if (m_editable && event->m_type == EventType::TextInput) {
            EditBox::handleEvent(event);
            if (!m_items.empty()) {
                int idx = findItemByText(m_text);
                if (idx >= 0) m_hoveredIndex = idx;
            }
            return true;
        }

        if (event->m_type == EventType::MouseWheel) {
            // Only cycle when mouse is over the combobox (not just focused)
            if (isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)) {
                int delta = (event->mouseWheel.scrollY > 0) ? -1 : 1;
                if (!m_items.empty()) {
                    cycleSelection(delta);
                    return true;
                }
            }
        }
    }

    return EditBox::handleEvent(event);
}

// ═══════════════════════════════════════════════════════════════
// beforeEventHandlingWatcher — 展开态键盘导航 + 点击收起
// ═══════════════════════════════════════════════════════════════
bool ComboBox::beforeEventHandlingWatcher(shared_ptr<Event> event)
{
    if (!isPopupOpen()) return false;

    // 展开态点击控件自身区域 → 收起：
    // 编辑模式仅箭头区（正文点击用于聚焦编辑），只读模式任意区域
    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left &&
        isContainsPoint(event->mouseButton.x, event->mouseButton.y) &&
        (!m_editable || isInArrowArea(event->mouseButton.x))) {
        closePopup();
        return true;
    }

    if (event->m_type == EventType::KeyDown) {
        switch (event->keyEvent.keycode) {
        case KeyCode::Down:
            if (m_hoveredIndex < (int)m_items.size() - 1) {
                int newIdx = m_hoveredIndex + 1;
                while (newIdx < (int)m_items.size() && m_items[newIdx].disabled)
                    newIdx++;
                if (newIdx < (int)m_items.size()) {
                    m_hoveredIndex = newIdx;
                    scrollToItem(m_hoveredIndex);
                }
            }
            return true;

        case KeyCode::Up:
            if (m_hoveredIndex > 0) {
                int newIdx = m_hoveredIndex - 1;
                while (newIdx >= 0 && m_items[newIdx].disabled)
                    newIdx--;
                if (newIdx >= 0) {
                    m_hoveredIndex = newIdx;
                    scrollToItem(m_hoveredIndex);
                }
            }
            return true;

        case KeyCode::Return:
        case KeyCode::KPEnter:
            if (m_hoveredIndex >= 0 && m_hoveredIndex < (int)m_items.size()
                && !m_items[m_hoveredIndex].disabled) {
                selectItem(m_hoveredIndex);
                closePopup(DialogResult::Confirmed);
            } else {
                closePopup();
            }
            return true;

        case KeyCode::Escape:
            closePopup();
            return true;

        case KeyCode::PageUp:
            m_hoveredIndex = max(0, m_hoveredIndex - m_maxVisibleItems);
            while (m_hoveredIndex < (int)m_items.size() && m_items[m_hoveredIndex].disabled)
                m_hoveredIndex++;
            if (m_hoveredIndex >= (int)m_items.size())
                m_hoveredIndex = findLastEnabled();
            scrollToItem(m_hoveredIndex);
            return true;

        case KeyCode::PageDown:
            m_hoveredIndex = min((int)m_items.size() - 1,
                                 m_hoveredIndex + m_maxVisibleItems);
            while (m_hoveredIndex >= 0 && m_items[m_hoveredIndex].disabled)
                m_hoveredIndex--;
            if (m_hoveredIndex < 0)
                m_hoveredIndex = findFirstEnabled(0);
            scrollToItem(m_hoveredIndex);
            return true;

        case KeyCode::Home:
            m_hoveredIndex = findFirstEnabled(0);
            scrollToItem(m_hoveredIndex);
            return true;

        case KeyCode::End:
            m_hoveredIndex = findLastEnabled();
            scrollToItem(m_hoveredIndex);
            return true;

        default:
            return false;
        }
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════
// setRect
// ═══════════════════════════════════════════════════════════════
void ComboBox::setRect(SRect rect)
{
    EditBox::setRect(rect);
}

// ═══════════════════════════════════════════════════════════════
// update
// ═══════════════════════════════════════════════════════════════
void ComboBox::update()
{
    if (!getEnable()) return;   // enable 守卫（disabled 时滚动条不更新）
    EditBox::update();

    if (m_scrollBar && m_scrollBar->getVisible()) {
        m_scrollBar->update();
    }
}

// ═══════════════════════════════════════════════════════════════
// ── Popup 控制 ──
// ═══════════════════════════════════════════════════════════════
void ComboBox::openPopup()
{
    if (isPopupOpen() || m_items.empty()) return;

    m_savedSelectedIndex = m_selectedIndex;
    m_hoveredIndex = (m_selectedIndex >= 0) ? m_selectedIndex : findFirstEnabled(0);

    rebuildPopupContent();

    SRect popupRect = computePopupRect();
    if (popupRect.width <= 0 || popupRect.height <= 0)
        return;

    // 绝对坐标（computePopupRect 基于 getDrawRect）转父（bench）相对本地坐标。
    // 弹层复合恒 1（物理像素面板），但位置经 getDrawRect 乘父复合 → 需除根复合反查
    SRect br = BENCH ? BENCH->getDrawRect() : SRect(0, 0, 0, 0);
    float bsx = BENCH ? BENCH->getScaleXX() : 1.0f;
    float bsy = BENCH ? BENCH->getScaleYY() : 1.0f;
    popupRect.left = (popupRect.left - br.left) / bsx;
    popupRect.top = (popupRect.top - br.top) / bsy;
    m_popup->setAbsolute(popupRect);
    m_popup->open();
}

void ComboBox::closePopup(DialogResult result)
{
    if (!isPopupOpen()) return;

    m_popup->close(result);
}

bool ComboBox::isPopupOpen() const
{
    return m_popup && m_popup->getVisible();
}

void ComboBox::togglePopup()
{
    if (isPopupOpen())
        closePopup();
    else
        openPopup();
}

// ═══════════════════════════════════════════════════════════════
// computePopupRect — 弹窗定位
// ═══════════════════════════════════════════════════════════════
SRect ComboBox::computePopupRect()
{
    SRect dr = getDrawRect();
    float sx = getScaleXX();
    float sy = getScaleYY();

    int visibleCount = min((int)m_items.size(), m_maxVisibleItems);
    float pw = dr.width;
    float fullPh = visibleCount * m_itemHeight * sy;

    // 视口绝对钳制（多视口场景下拉列表按视口区域钳制；dr 为窗口绝对
    // 坐标，钳制边界须为 vp.left/top + 尺寸）
    SRect vp = GET_CONTEXT ? GET_CONTEXT->viewport : SRect(0, 0, 1024, 768);
    float screenH = vp.top + vp.height;

    float x = dr.left;
    float bestY = dr.bottom() + m_dropdownOffset * sy;
    float bestPh = fullPh;
    bool found = false;

    if (bestY + fullPh <= screenH) {
        found = true;
    } else {
        float yAbove = dr.top - fullPh - m_dropdownOffset * sy;
        if (yAbove >= vp.top) {
            bestY = yAbove;
            found = true;
        } else {
            float spaceBelow = screenH - dr.bottom() - m_dropdownOffset * sy;
            float spaceAbove = dr.top - m_dropdownOffset * sy - vp.top;

            if (spaceBelow >= spaceAbove) {
                bestPh = max(0.0f, spaceBelow);
                bestY  = dr.bottom() + m_dropdownOffset * sy;
            } else {
                bestPh = max(0.0f, spaceAbove);
                bestY  = dr.top - bestPh - m_dropdownOffset * sy;
            }

            float oneItemH = m_itemHeight * sy;
            if (bestPh < oneItemH)
                return SRect();

            int adjustedCount = (int)(bestPh / oneItemH);
            if (adjustedCount <= 0)
                return SRect();
            bestPh = adjustedCount * oneItemH;
            found = true;
        }
    }

    if (!found) return SRect();

    float screenW = vp.width;
    if (x + pw > screenW) x = screenW - pw;
    if (x < 0) x = 0;

    return SRect(x, bestY, pw / sx, bestPh / sy);
}

// ═══════════════════════════════════════════════════════════════
// rebuildPopupContent — 重建列表内容
// ═══════════════════════════════════════════════════════════════
void ComboBox::rebuildPopupContent()
{
    SRect popupRect = computePopupRect();
    if (popupRect.width <= 0 || popupRect.height <= 0)
        return;

    float listW = popupRect.width;
    float listH = popupRect.height;

    int totalItems = (int)m_items.size();
    bool needScroll = totalItems > m_maxVisibleItems;

    if (needScroll) {
        float sbW = ConstDef::SCROLLBAR_WIDTH;
        m_listPanel->setRect(SRect(0, 0, listW - sbW, listH));
        m_scrollBar->setRect(SRect(listW - sbW, 0, sbW, listH));
        updateScrollBar();
    } else {
        m_listPanel->setRect(SRect(0, 0, listW, listH));
        m_scrollBar->setVisible(false);
        m_scrollBar->setRect(SRect(0, 0, 0, 0));
    }

    m_listPanel->setScrollOffset(0);
}

void ComboBox::updateScrollBar()
{
    if (!m_scrollBar) return;
    int totalItems = (int)m_items.size();
    if (totalItems <= m_maxVisibleItems) {
        m_scrollBar->setVisible(false);
        return;
    }
    m_scrollBar->setVisible(true);
    // Save intended offset BEFORE setRange/setPageSize callbacks
    // (their notifyPositionChanged → syncListFromScroll would reset it to 0)
    int intendedOffset = m_listPanel->getScrollOffset();
    m_scrollBar->setRange(0.0f, (float)(totalItems - m_maxVisibleItems));
    m_scrollBar->setPageSize((float)m_maxVisibleItems);
    m_scrollBar->setStepSize(1.0f);
    m_scrollBar->setValue((float)intendedOffset);
}

void ComboBox::syncListFromScroll()
{
    if (!m_scrollBar) return;
    int newOffset = (int)(m_scrollBar->getValue());
    m_listPanel->setScrollOffset(newOffset);
}

// ═══════════════════════════════════════════════════════════════
// ── 选择 ──
// ═══════════════════════════════════════════════════════════════
void ComboBox::selectItem(int index)
{
    if (index < 0 || index >= (int)m_items.size()) return;
    if (m_items[index].disabled) return;

    m_selectedIndex = index;
    m_text = m_items[index].label;
    m_cursorPosition = (int)m_text.length();
    clearSelection();
    updateTextOffset();

    if (m_onSelectionChanged)
        m_onSelectionChanged(std::dynamic_pointer_cast<ComboBox>(getThis()),
                             index, m_items[index].value);

    SelectionPayload sel = { m_selectedIndex, m_items[m_selectedIndex].label.c_str() };
    fireCCallback(PropertyNames::kEventSelectionChanged, CCallbackData::Selection, &sel);
}

void ComboBox::restorePreviousSelection()
{
    if (m_savedSelectedIndex >= 0 && m_savedSelectedIndex < (int)m_items.size()) {
        m_selectedIndex = m_savedSelectedIndex;
        m_text = m_items[m_selectedIndex].label;
    } else {
        m_selectedIndex = -1;
        m_text.clear();
    }
    m_cursorPosition = (int)m_text.length();
    clearSelection();
    updateTextOffset();
}

void ComboBox::cycleSelection(int direction)
{
    if (m_items.empty()) return;
    int n = (int)m_items.size();
    if (n <= 1) return;

    int newIdx = m_selectedIndex;
    if (newIdx < 0) {
        newIdx = (direction > 0) ? findFirstEnabled(0) : findLastEnabled();
        if (newIdx >= 0)
            selectItem(newIdx);
        return;
    }

    int attempts = 0;
    do {
        if (m_cycleEnabled) {
            newIdx = (newIdx + direction + n) % n;
        } else {
            int next = newIdx + direction;
            if (next < 0 || next >= n) return;
            newIdx = next;
        }
        attempts++;
    } while (attempts < n && m_items[newIdx].disabled);

    if (attempts >= n) return;

    if (newIdx != m_selectedIndex)
        selectItem(newIdx);
}

int ComboBox::findFirstEnabled(int start) const
{
    for (int i = start; i < (int)m_items.size(); ++i) {
        if (!m_items[i].disabled) return i;
    }
    return -1;
}

int ComboBox::findLastEnabled() const
{
    for (int i = (int)m_items.size() - 1; i >= 0; --i) {
        if (!m_items[i].disabled) return i;
    }
    return -1;
}

// 输入文字匹配：先精确匹配（忽略大小写），再前缀匹配
int ComboBox::findItemByText(const string& text) const
{
    if (text.empty()) return -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i].disabled && _stricmp(m_items[i].label.c_str(), text.c_str()) == 0)
            return (int)i;
    }
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i].disabled &&
            _strnicmp(m_items[i].label.c_str(), text.c_str(), text.length()) == 0)
            return (int)i;
    }
    return -1;
}

// ═══════════════════════════════════════════════════════════════
// ── 文本辅助 ──
// ═══════════════════════════════════════════════════════════════
float ComboBox::getStringWidth(const string& text)
{
    if (!m_font || text.empty()) return 0;
    SSize size = getTextRenderer()->measureText(m_font.get(), text);
    return size.width / getScaleXX();
}

string ComboBox::getTruncatedText(const string& text, float maxWidth)
{
    return ::truncateText(text, maxWidth,
        [this](const string& s) { return getStringWidth(s); });
}

// ═══════════════════════════════════════════════════════════════
// ── 事件辅助 ──
// ═══════════════════════════════════════════════════════════════
bool ComboBox::isInArrowArea(float x)
{
    SRect dr = getDrawRect();
    float arrowStartX = dr.right() - m_arrowWidth * getScaleXX();
    return x >= arrowStartX && x <= dr.right();
}

void ComboBox::scrollToItem(int index)
{
    if (!m_listPanel) return;
    int offset = m_listPanel->getScrollOffset();
    int visibleEnd = offset + m_maxVisibleItems;
    if (index < offset) {
        m_listPanel->setScrollOffset(index);
        updateScrollBar();
    } else if (index >= visibleEnd) {
        m_listPanel->setScrollOffset(index - m_maxVisibleItems + 1);
        updateScrollBar();
    }
}

// ═══════════════════════════════════════════════════════════════
// ── 选项管理 ──
// ═══════════════════════════════════════════════════════════════
void ComboBox::setItems(const vector<ComboBoxItem>& items)
{
    m_items = items;
    if (m_selectedIndex >= (int)m_items.size())
        m_selectedIndex = m_items.empty() ? -1 : 0;
    if (m_selectedIndex >= 0 && !m_items.empty()) {
        m_text = m_items[m_selectedIndex].label;
        m_cursorPosition = (int)m_text.length();
        clearSelection();
        updateTextOffset();
    }
}

void ComboBox::addItem(const string& label, const string& value, bool disabled)
{
    ComboBoxItem item;
    item.label = label;
    item.value = value;
    item.disabled = disabled;
    m_items.push_back(item);
}

void ComboBox::clearItems()
{
    m_items.clear();
    m_selectedIndex = -1;
    m_hoveredIndex = -1;
    m_text.clear();
    m_cursorPosition = 0;
    clearSelection();
    updateTextOffset();
}

void ComboBox::removeItem(int index)
{
    if (index < 0 || index >= (int)m_items.size()) return;
    m_items.erase(m_items.begin() + index);
    if (m_selectedIndex == index) {
        m_selectedIndex = -1;
        EditBox::setText("");
    } else if (m_selectedIndex > index) {
        m_selectedIndex--;
    }
}

void ComboBox::setSelectedIndex(int index)
{
    if (index < -1 || index >= (int)m_items.size()) return;
    if (index >= 0 && m_items[index].disabled) return;
    m_selectedIndex = index;
    if (index >= 0) {
        m_text = m_items[index].label;
    } else {
        m_text.clear();
    }
    m_cursorPosition = (int)m_text.length();
    clearSelection();
    updateTextOffset();
}

void ComboBox::setSelectedValue(const string& value)
{
    for (int i = 0; i < (int)m_items.size(); ++i) {
        if (m_items[i].value == value && !m_items[i].disabled) {
            setSelectedIndex(i);
            return;
        }
    }
}

string ComboBox::getSelectedValue() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size())
        return m_items[m_selectedIndex].value;
    return "";
}

string ComboBox::getSelectedLabel() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size())
        return m_items[m_selectedIndex].label;
    return "";
}


// ═══════════════════════════════════════════════════════════════
// ComboBoxListPanel
// ═══════════════════════════════════════════════════════════════
ComboBoxListPanel::ComboBoxListPanel(Control* parent, SRect rect,
                                      float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
{
    m_visible = true;
    m_enable = true;
    m_isTransparent = false;
}

void ComboBoxListPanel::create()
{
    ControlImpl::create();
}

int ComboBoxListPanel::getVisibleEnd() const
{
    if (!m_owner) return 0;
    int maxVisible = m_owner->m_maxVisibleItems;
    return min(m_scrollOffset + maxVisible, (int)m_owner->m_items.size());
}

int ComboBoxListPanel::getItemHeight(int index)
{
    if (!m_owner) return ConstDef::COMBOBOX_DEFAULT_ITEM_HEIGHT;
    return (int)m_owner->m_itemHeight;
}

int ComboBoxListPanel::hitTest(float y)
{
    if (!m_owner) return -1;
    SRect dr = getDrawRect();
    float localY = y - dr.top;
    float sy = getScaleYY();

    int visibleStart = m_scrollOffset;
    int visibleEnd = getVisibleEnd();
    float currentY = 0;
    for (int i = visibleStart; i < visibleEnd; ++i) {
        float itemH = m_owner->m_itemHeight * sy;
        if (localY >= currentY && localY < currentY + itemH)
            return i;
        currentY += itemH;
    }
    return -1;
}

void ComboBoxListPanel::setScrollOffset(int offset)
{
    if (!m_owner) return;
    int maxOffset = max(0, (int)m_owner->m_items.size() - m_owner->m_maxVisibleItems);
    m_scrollOffset = min(max(0, offset), maxOffset);
}

int ComboBoxListPanel::getTotalItemCount() const
{
    return m_owner ? (int)m_owner->m_items.size() : 0;
}

int ComboBoxListPanel::getVisibleItemCount() const
{
    return getVisibleEnd() - getVisibleStart();
}

void ComboBoxListPanel::draw()
{
    if (!m_visible || !m_owner) return;

    ControlImpl::beforeDraw();
    SRect dr = getDrawRect();
    auto* device = GET_RENDERDEVICE;

    device->setDrawColor(m_owner->m_listBgColor);
    device->fillRect(dr);

    int start = getVisibleStart();
    int end = getVisibleEnd();
    auto& items = m_owner->m_items;

    float sx = getScaleXX();
    float sy = getScaleYY();
    TextRenderer* renderer = getTextRenderer();
    if (!renderer) return;

    bool needScroll = m_owner->m_scrollBar && m_owner->m_scrollBar->getVisible();
    float textClipRight = dr.right();
    if (needScroll) {
        textClipRight -= ConstDef::SCROLLBAR_WIDTH * sx;
    }

    for (int i = start; i < end; ++i) {
        float itemY = (float)(i - start) * m_owner->m_itemHeight * sy;
        float itemH = m_owner->m_itemHeight * sy;
        SRect itemRect(dr.left, dr.top + itemY, dr.width, itemH);

        if (i == m_owner->m_selectedIndex) {
            device->setDrawColor(m_owner->m_itemSelectedColor);
            device->fillRect(itemRect);
        } else if (i == m_owner->m_hoveredIndex) {
            device->setDrawColor(m_owner->m_itemHoverColor);
            device->fillRect(itemRect);
        }

        if (needScroll) {
            SRect textClip(dr.left, dr.top + itemY,
                           textClipRight - dr.left, itemH);
            device->pushClipRect(textClip);
        }

        SColor textColor = items[i].disabled
            ? m_owner->m_itemDisabledColor
            : m_owner->getEffectiveListTextColor();

        float textMaxWidth = textClipRight - dr.left - ConstDef::COMBOBOX_LIST_PADDING * sx;
        string displayText = m_owner->getTruncatedText(items[i].label, textMaxWidth);

        int fontSize = m_owner->getItemFontSize();
        float textY = dr.top + itemY + (itemH - fontSize * sy) / 2.0f;
        renderer->drawText(m_owner->getItemFont(), displayText,
            dr.left + ConstDef::COMBOBOX_LIST_PADDING * sx, textY, textColor);

        if (needScroll) {
            device->popClipRect();
        }
    }

    device->setDrawColor(m_owner->m_listBorderColor);
    device->drawRect(dr);

    ControlImpl::afterDraw();
}

bool ComboBoxListPanel::handleEvent(shared_ptr<Event> event)
{
    if (!m_visible || !m_enable || !m_owner) return false;

    float mx, my;
    bool gotPos = false;
    if (event->m_type == EventType::MouseMove) {
        mx = event->mousePos.x; my = event->mousePos.y; gotPos = true;
    } else if (event->m_type == EventType::MouseDown ||
               event->m_type == EventType::MouseUp) {
        mx = event->mouseButton.x; my = event->mouseButton.y; gotPos = true;
    }

    if (gotPos) {
        if (!getDrawRect().contains(mx, my))
            return false;

        if (event->m_type == EventType::MouseMove) {
            int idx = hitTest(my);
            if (idx >= 0 && (int)m_owner->m_items.size() > idx
                && m_owner->m_items[idx].disabled)
                idx = -1;
            if (idx != m_owner->m_hoveredIndex) {
                m_owner->m_hoveredIndex = idx;
            }
            return true;
        }

        if (event->m_type == EventType::MouseDown &&
            event->mouseButton.button == MouseButton::Left) {
            int idx = hitTest(my);
            if (idx >= 0 && idx < (int)m_owner->m_items.size()
                && !m_owner->m_items[idx].disabled) {
                m_owner->selectItem(idx);
                m_owner->closePopup(DialogResult::Confirmed);
                return true;
            }
            return true;
        }
    }

    if (event->m_type == EventType::MouseWheel) {
        int delta = (event->mouseWheel.scrollY > 0) ? -1 : 1;
        int newOffset = m_scrollOffset + delta;
        setScrollOffset(newOffset);
        if (m_owner)
            m_owner->updateScrollBar();
        return true;
    }

    return false;
}


// ── Property system overrides ──

int ComboBox::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kArrow) == 0)          { setArrowColor(color);      return 1; }
    if (strcmp(prop, PropertyNames::kArrowHover) == 0)     { setArrowHoverColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kItemSelected) == 0)   { setItemSelectedColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kItemHover) == 0)      { setItemHoverColor(color);  return 1; }
    if (strcmp(prop, PropertyNames::kItemDisabled) == 0)   { setItemDisabledColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kListBg) == 0)         { setListBgColor(color);     return 1; }
    if (strcmp(prop, PropertyNames::kListBorder) == 0)     { setListBorderColor(color); return 1; }
    return ControlImpl::setColorProperty(prop, color);
}

int ComboBox::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kCycleEnabled) == 0) { setCycleEnabled(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kEditable) == 0)                   { setEditable(value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}

int ComboBox::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kMaxVisibleItems) == 0) { setMaxVisibleItems(value); return 1; }
    if (strcmp(prop, PropertyNames::kSelectedIndex) == 0)   { setSelectedIndex(value);   return 1; }
    return ControlImpl::setIntProperty(prop, value);
}

int ComboBox::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kArrowWidth) == 0) { setArrowWidth(value); return 1; }
    if (strcmp(prop, PropertyNames::kItemHeight) == 0) { setItemHeight(value); return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}

int ComboBox::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kItems) == 0) {
        if (!value) return 0;
        try {
            auto j = nlohmann::json::parse(value);
            vector<ComboBoxItem> items;
            for (auto& jitem : j) {
                ComboBoxItem item;
                item.label = jitem.value(PropertyNames::kJsonLabel, "");
                item.value = jitem.value(PropertyNames::kValue, item.label);
                item.disabled = jitem.value(PropertyNames::kJsonDisabled, false);
                items.push_back(item);
            }
            setItems(items);
            return 1;
        } catch (...) { return 0; }
    }
    // 编辑框文本（可编辑模式下输入的内容 / 选中后的项 label）
    if (strcmp(prop, PropertyNames::kTextContent) == 0) {
        if (value) setText(value);
        return 1;
    }
    return ControlImpl::setStringProperty(prop, value);
}

int ComboBox::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kArrow) == 0)          { out = m_arrowColor;      return 1; }
    if (strcmp(prop, PropertyNames::kArrowHover) == 0)     { out = m_arrowHoverColor; return 1; }
    if (strcmp(prop, PropertyNames::kItemSelected) == 0)   { out = m_itemSelectedColor; return 1; }
    if (strcmp(prop, PropertyNames::kItemHover) == 0)      { out = m_itemHoverColor;  return 1; }
    if (strcmp(prop, PropertyNames::kItemDisabled) == 0)   { out = m_itemDisabledColor; return 1; }
    if (strcmp(prop, PropertyNames::kListBg) == 0)         { out = m_listBgColor;     return 1; }
    if (strcmp(prop, PropertyNames::kListBorder) == 0)     { out = m_listBorderColor; return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

int ComboBox::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kCycleEnabled) == 0) { out = m_cycleEnabled ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kEditable) == 0)                   { out = m_editable ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int ComboBox::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kMaxVisibleItems) == 0) { out = m_maxVisibleItems; return 1; }
    if (strcmp(prop, PropertyNames::kSelectedIndex) == 0)   { out = m_selectedIndex;   return 1; }
    return ControlImpl::getIntProperty(prop, out);
}

int ComboBox::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kArrowWidth) == 0) { out = m_arrowWidth; return 1; }
    if (strcmp(prop, PropertyNames::kItemHeight) == 0) { out = m_itemHeight; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}

int ComboBox::getStringProperty(const char* prop, const char*& out) {
    // selected-label：当前选中项的 label；无选中返回空串
    if (strcmp(prop, PropertyNames::kSelectedLabel) == 0) {
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size()) {
            out = m_items[m_selectedIndex].label.c_str();
        } else {
            out = "";
        }
        return 1;
    }
    // selected-value：选中项的 value；无匹配时返回编辑框保留的输入内容
    if (strcmp(prop, PropertyNames::kSelectedValue) == 0) {
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size()) {
            out = m_items[m_selectedIndex].value.c_str();
        } else {
            out = m_text.c_str();
        }
        return 1;
    }
    // text：编辑框当前内容（输入中的文字 / 无匹配时保留的输入 / 选中后的项 label）
    if (strcmp(prop, PropertyNames::kTextContent) == 0) {
        out = m_text.c_str();
        return 1;
    }
    return ControlImpl::getStringProperty(prop, out);
}

int ComboBox::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kListPanel) == 0)     { out = m_listPanel ? static_cast<Control*>(m_listPanel.get()) : nullptr;     return 1; }
    if (strcmp(prop, PropertyNames::kListScrollBar) == 0) { out = m_scrollBar ? static_cast<Control*>(m_scrollBar.get()) : nullptr;      return 1; }
    return ControlImpl::getPtrProperty(prop, out);
}

int ComboBox::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventSelectionChanged) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

// ═══════════════════════════════════════════════════════════════
// ComboBoxBuilder
// ═══════════════════════════════════════════════════════════════
ComboBoxBuilder::ComboBoxBuilder(Control* parent, SRect rect,
                                  float xScale, float yScale)
    : m_comboBox(nullptr)
{
    m_comboBox = make_shared<ComboBox>(parent, rect, xScale, yScale);
}

ComboBoxBuilder& ComboBoxBuilder::setItems(const vector<ComboBoxItem>& items)
{ m_comboBox->setItems(items); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setSelectedIndex(int index)
{ m_comboBox->setSelectedIndex(index); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setPlaceholder(const string& text)
{ m_comboBox->setPlaceholder(text); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setArrowWidth(float width)
{ m_comboBox->setArrowWidth(width); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setItemHeight(float height)
{ m_comboBox->setItemHeight(height); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setMaxVisibleItems(int count)
{ m_comboBox->setMaxVisibleItems(count); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setOnSelectionChanged(
    ComboBox::OnSelectionChangedHandler handler)
{ m_comboBox->setOnSelectionChanged(handler); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setBackgroundStateColor(StateColor color)
{ m_comboBox->setBackgroundStateColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setBorderStateColor(StateColor color)
{ m_comboBox->setBorderStateColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setArrowColor(SColor color)
{ m_comboBox->setArrowColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setArrowHoverColor(SColor color)
{ m_comboBox->setArrowHoverColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setCycleEnabled(bool enabled)
{ m_comboBox->setCycleEnabled(enabled); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setItemSelectedColor(SColor color)
{ m_comboBox->setItemSelectedColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setItemHoverColor(SColor color)
{ m_comboBox->setItemHoverColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setItemDisabledColor(SColor color)
{ m_comboBox->setItemDisabledColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setListBgColor(SColor color)
{ m_comboBox->setListBgColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setListBorderColor(SColor color)
{ m_comboBox->setListBorderColor(color); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setText(const string& text)
{ m_comboBox->setText(text); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setFont(FontName fontName)
{ m_comboBox->setFont(fontName); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setFontSize(int size)
{ m_comboBox->setFontSize(size); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setAlignmentMode(AlignmentMode mode)
{ m_comboBox->setAlignmentMode(mode); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setId(int id)
{ m_comboBox->setId(id); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setTransparent(bool isTransparent)
{ m_comboBox->setTransparent(isTransparent); return *this; }

ComboBoxBuilder& ComboBoxBuilder::setVisible(bool visible)
{ m_comboBox->setVisible(visible); return *this; }

shared_ptr<ComboBox> ComboBoxBuilder::build(void)
{
    m_comboBox->create();
    if (!m_comboBox->getVisible())
        m_comboBox->setVisible(true);
    return m_comboBox;
}
