#ifndef ComboBoxH
#define ComboBoxH

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "ConstDef.h"
#include "EditBox.h"
#include "Dialog.h"
#include "ScrollBar.h"
#include "RenderDevice.h"
#include "EventTypes.h"

struct ComboBoxItem {
    std::string label;
    std::string value;
    bool disabled = false;
};

class ComboBoxListPanel;

class ComboBox : public EditBox {
    friend class ComboBoxBuilder;
    friend class ComboBoxListPanel;
public:
    using OnSelectionChangedHandler =
        std::function<void(shared_ptr<ComboBox>, int index, const string& value)>;

private:
    // ── 选项数据 ──
    vector<ComboBoxItem> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
    int m_savedSelectedIndex = -1;
    string m_placeholder;

    // ── 视觉属性 ──
    float m_arrowWidth;
    float m_itemHeight;
    int   m_maxVisibleItems;
    SColor m_arrowColor;
    SColor m_arrowHoverColor;
    SColor m_itemSelectedColor;
    SColor m_itemHoverColor;
    SColor m_itemDisabledColor;
    SColor m_listBgColor;
    SColor m_listBorderColor;

    // ── Popup ──
    shared_ptr<Popup> m_popup;
    shared_ptr<ComboBoxListPanel> m_listPanel;
    shared_ptr<ScrollBar> m_scrollBar;

    // ── 回调 ──
    OnSelectionChangedHandler m_onSelectionChanged;

    // ── 状态 ──
    float m_dropdownOffset;
    bool m_arrowHovered = false;
    bool m_watcherRegistered = false;
    bool m_cycleEnabled = true;

private:
    // ── Popup 控制 ──
    SRect computePopupRect();
    void openPopup();
    void closePopup(DialogResult result = DialogResult::Cancelled);
    bool isPopupOpen() const;
    void togglePopup();

    // ── Popup 内容构建 ──
    void rebuildPopupContent();
    void updateScrollBar();
    void syncListFromScroll();

    // ── 选择 ──
    void selectItem(int index);
    void restorePreviousSelection();
    void cycleSelection(int direction);
    int findFirstEnabled(int start) const;
    int findLastEnabled() const;

    // ── 文本辅助 ──
    float getStringWidth(const string& text);
    string getTruncatedText(const string& text, float maxWidth);

    // ── 事件 ──
    bool isInArrowArea(float x);
    void scrollToItem(int index);

public:
    SColor getEffectiveListTextColor() const { return m_textColor.getNormal(); }
    Font* getItemFont() const { return m_font.get(); }
    int getItemFontSize() const { return m_fontSize; }
    bool hasScrollBar() const { return m_scrollBar && m_scrollBar->getVisible(); }
    float getScrollBarWidth() const { return ConstDef::SCROLLBAR_WIDTH; }

public:
    ComboBox(Control* parent, SRect rect,
             float xScale = 1.0f, float yScale = 1.0f);
    ~ComboBox();
    void create(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    void update(void) override;

    // ── 选项管理 ──
    void setItems(const vector<ComboBoxItem>& items);
    void addItem(const string& label, const string& value, bool disabled = false);
    void clearItems();
    void removeItem(int index);
    const vector<ComboBoxItem>& getItems() const { return m_items; }
    int getItemCount() const { return (int)m_items.size(); }

    // ── 选中 ──
    void setSelectedIndex(int index);
    int  getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedValue(const string& value);
    string getSelectedValue() const;
    string getSelectedLabel() const;

    // ── 占位文本 ──
    void setPlaceholder(const string& text) { m_placeholder = text; }
    string getPlaceholder() const { return m_placeholder; }

    // ── 视觉配置 ──
    void setArrowWidth(float width) { m_arrowWidth = width; }
    float getArrowWidth() const { return m_arrowWidth; }
    void setItemHeight(float height) { m_itemHeight = height; }
    float getItemHeight() const { return m_itemHeight; }
    void setMaxVisibleItems(int count) { m_maxVisibleItems = count; }
    int  getMaxVisibleItems() const { return m_maxVisibleItems; }

    // ── 行为配置 ──
    void setCycleEnabled(bool enabled) { m_cycleEnabled = enabled; }
    bool isCycleEnabled() const { return m_cycleEnabled; }

    // ── 颜色配置 ──
    void setArrowColor(SColor color) { m_arrowColor = color; }
    void setArrowHoverColor(SColor color) { m_arrowHoverColor = color; }
    void setItemSelectedColor(SColor color) { m_itemSelectedColor = color; }
    void setItemHoverColor(SColor color) { m_itemHoverColor = color; }
    void setItemDisabledColor(SColor color) { m_itemDisabledColor = color; }
    void setListBgColor(SColor color) { m_listBgColor = color; }
    void setListBorderColor(SColor color) { m_listBorderColor = color; }

    // ── 事件 ──
    void setOnSelectionChanged(OnSelectionChangedHandler handler)
        { m_onSelectionChanged = handler; }

    // ── 测试辅助 ──
    shared_ptr<ComboBoxListPanel> getListPanel() { return m_listPanel; }
    shared_ptr<ScrollBar> getListScrollBar() { return m_scrollBar; }
    void openPopupForTest() { openPopup(); }
};


// ═══════════════════════════════════════════════════════════════
// ComboBoxListPanel — 下拉列表的自定义绘制面板
// ═══════════════════════════════════════════════════════════════
class ComboBoxListPanel : public ControlImpl {
    friend class ComboBox;
private:
    ComboBox* m_owner = nullptr;
    int m_scrollOffset = 0;

    void setOwner(ComboBox* owner) { m_owner = owner; }

    int getVisibleStart() const { return m_scrollOffset; }
    int getVisibleEnd() const;
    int getItemHeight(int index);
    int hitTest(float y);

public:
    ComboBoxListPanel(Control* parent, SRect rect,
                      float xScale = 1.0f, float yScale = 1.0f);
    void create(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;

    void setScrollOffset(int offset);
    int getScrollOffset() const { return m_scrollOffset; }
    int getTotalItemCount() const;
    int getVisibleItemCount() const;
};


// ═══════════════════════════════════════════════════════════════
// ComboBoxBuilder
// ═══════════════════════════════════════════════════════════════
class ComboBoxBuilder {
private:
    shared_ptr<ComboBox> m_comboBox;
public:
    ComboBoxBuilder(Control* parent, SRect rect,
                    float xScale = 1.0f, float yScale = 1.0f);

    ComboBoxBuilder& setItems(const vector<ComboBoxItem>& items);
    ComboBoxBuilder& setSelectedIndex(int index);
    ComboBoxBuilder& setPlaceholder(const string& text);
    ComboBoxBuilder& setArrowWidth(float width);
    ComboBoxBuilder& setItemHeight(float height);
    ComboBoxBuilder& setMaxVisibleItems(int count);
    ComboBoxBuilder& setOnSelectionChanged(ComboBox::OnSelectionChangedHandler handler);
    ComboBoxBuilder& setBackgroundStateColor(StateColor color);
    ComboBoxBuilder& setBorderStateColor(StateColor color);
    ComboBoxBuilder& setArrowColor(SColor color);
    ComboBoxBuilder& setArrowHoverColor(SColor color);
    ComboBoxBuilder& setCycleEnabled(bool enabled);
    ComboBoxBuilder& setItemSelectedColor(SColor color);
    ComboBoxBuilder& setItemHoverColor(SColor color);
    ComboBoxBuilder& setItemDisabledColor(SColor color);
    ComboBoxBuilder& setListBgColor(SColor color);
    ComboBoxBuilder& setListBorderColor(SColor color);
    ComboBoxBuilder& setText(const string& text);
    ComboBoxBuilder& setFont(FontName fontName);
    ComboBoxBuilder& setFontSize(int size);
    ComboBoxBuilder& setAlignmentMode(AlignmentMode mode);
    ComboBoxBuilder& setId(int id);
    ComboBoxBuilder& setTransparent(bool isTransparent);
    ComboBoxBuilder& setVisible(bool visible);

    shared_ptr<ComboBox> build(void);
};

#endif
