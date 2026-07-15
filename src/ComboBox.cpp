#define NOMINMAX
#include "ComboBox.h"
#include "Utility.h"
#include "MainWindow.h"
#include "EventQueue.h"
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
    EditBox::create();

    m_popup = make_shared<Popup>(nullptr, SRect(0, 0, 100, 100), m_xScale, m_yScale);
    m_popup->setRenderDevice(getRenderDevice());
    m_popup->setTextRenderer(getTextRenderer());
    m_popup->setResourceProvider(getResourceProvider());
    m_popup->setInputBackend(getInputBackend());
    m_popup->setCloseOnClickOutside(true);
    m_popup->setCloseOnEsc(false);
    m_popup->setBorderVisible(false);
    m_popup->setTransparent(false);
    m_popup->setOnClose([this](shared_ptr<Popup> popup, DialogResult result) {
        if (result == DialogResult::Cancelled) {
            restorePreviousSelection();
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
}

// ═══════════════════════════════════════════════════════════════
// draw
// ═══════════════════════════════════════════════════════════════
void ComboBox::draw()
{
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

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        if (isContainsPoint(event->mouseButton.x, event->mouseButton.y) &&
            isInArrowArea(event->mouseButton.x)) {
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
// beforeEventHandlingWatcher — 展开态键盘导航
// ═══════════════════════════════════════════════════════════════
bool ComboBox::beforeEventHandlingWatcher(shared_ptr<Event> event)
{
    if (!isPopupOpen()) return false;

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

    m_popup->setAbsolute(popupRect);
    m_popup->open();

    if (!m_watcherRegistered) {
        EventQueue::getInstance()->addBeforeEventHandlingWatcher(
            EventType::KeyDown, getThis());
        m_watcherRegistered = true;
    }
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

    SSize ws = MAINWIN->getWindowSize();
    float screenH = (float)ws.height;

    float x = dr.left;
    float bestY = dr.bottom() + m_dropdownOffset * sy;
    float bestPh = fullPh;
    bool found = false;

    if (bestY + fullPh <= screenH) {
        found = true;
    } else {
        float yAbove = dr.top - fullPh - m_dropdownOffset * sy;
        if (yAbove >= 0) {
            bestY = yAbove;
            found = true;
        } else {
            float spaceBelow = screenH - dr.bottom() - m_dropdownOffset * sy;
            float spaceAbove = dr.top - m_dropdownOffset * sy;

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

    float screenW = (float)MAINWIN->getWindowSize().width;
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
            device->setClipRect(textClip);
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
            device->clearClipRect();
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
