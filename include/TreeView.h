#ifndef TreeViewH
#define TreeViewH

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

#include "ConstDef.h"
#include "ScrollBar.h"
#include "RenderDevice.h"
#include "Label.h"
#include "EventTypes.h"
#include "TextRenderer.h"
#include "ResourceProvider.h"

struct TreeNode {
    std::string id;
    std::string label;
    bool expanded = false;
    std::vector<std::shared_ptr<TreeNode>> children;
    void* userData = nullptr;
    bool userDataOwned = false;  // 节点销毁时是否由 TreeView 清理 userData（LayoutParser 创建的堆对象置 true；外部 CABI 置入的指针由调用方管理）
    // TreeView 增强：Item 前置控件容器 + 逐 Item 字体（见 design/TreeView_Enhancement_Design.md）
    std::shared_ptr<Control> leadingControl;  // 前置控件（CheckBox/Actor/Image 等）；高度自适应行高
    float leadingGap = 6.0f;                  // 控件与文字之间的空隙（局部 px，可调）
    AlignmentMode leadingAlign = AlignmentMode::AM_MID_LEFT;  // 槽位对齐（复用 Label 9 宫格；水平分量忽略，槽位贴文本起点）
    FontName fontName = FontName::HarmonyOS_Sans_SC_Regular;  // 与 TreeView 默认一致；仅 fontSize>0 时生效
    int fontSize = 0;                         // 0 = 继承 TreeView 级字号
    StateColor textShadowColor{StateColor::Type::TextShadow};   // P032：item 文本阴影四态
    bool   shadowEnabled = false;
    float  shadowOffsetX = 1.0f;
    float  shadowOffsetY = 1.0f;
    // P032：item 级四态（设置任一即 hasStyle=true，行绘制改用 item 样式）
    StateColor bgColor{StateColor::Type::Background};
    StateColor borderColor{StateColor::Type::Border};
    StateColor textColor{StateColor::Type::Text};
    bool borderVisible = false;
    bool disabled = false;
    bool hasStyle = false;
};

inline std::shared_ptr<TreeNode> makeNode(
    const std::string& id,
    const std::string& label,
    bool expanded = false,
    std::vector<std::shared_ptr<TreeNode>> children = {}) {
    auto node = std::make_shared<TreeNode>();
    node->id = id;
    node->label = label;
    node->expanded = expanded;
    node->children = std::move(children);
    return node;
}

inline std::shared_ptr<TreeNode> cloneNode(
    const std::shared_ptr<TreeNode>& src,
    const std::string& idPrefix = "",
    const std::string& labelPrefix = "") {
    std::vector<std::shared_ptr<TreeNode>> clonedChildren;
    for (const auto& child : src->children)
        clonedChildren.push_back(cloneNode(child, idPrefix, labelPrefix));
    auto node = std::make_shared<TreeNode>();
    node->id = idPrefix + src->id;
    node->label = labelPrefix + src->label;
    node->expanded = src->expanded;
    node->children = std::move(clonedChildren);
    node->userData = src->userData;
    node->fontName = src->fontName;
    node->fontSize = src->fontSize;
    node->leadingGap = src->leadingGap;
    node->leadingAlign = src->leadingAlign;
    // leadingControl 必须置空：项目无控件深拷贝（共享实例会 getParent 冲突/双树串扰），
    // 调用方按需对新树重新赋值（见 TreeView_Enhancement_Design.md 4.1）
    return node;
}

class TreeView : public ControlImpl {
    friend class TreeViewBuilder;
public:
    using OnSelectHandler = std::function<void(shared_ptr<TreeView>, const std::string& nodeId)>;
    using OnSelectDataHandler = std::function<void(shared_ptr<TreeView>, const std::string& nodeId, void* userData)>;
    using OnExpandHandler = std::function<void(shared_ptr<TreeView>, const std::string& nodeId)>;
    using OnCollapseHandler = std::function<void(shared_ptr<TreeView>, const std::string& nodeId)>;
    using OnClearNodeHandler = std::function<void(shared_ptr<TreeView>, void* userData)>;

private:
    std::vector<std::shared_ptr<TreeNode>> m_rootItems;
    std::unordered_map<std::string, std::shared_ptr<TreeNode>> m_nodeMap;

    struct FlatRow {
        std::shared_ptr<TreeNode> node;
        int depth;
    };
    std::vector<FlatRow> m_flatRows;

    std::string m_selectedId;
    std::string m_itemTargetId;   // CABI "item-id" 定位：item 级属性（leadingGap/font/size/leadingControl）的作用目标
    int m_selectedRow = -1;
    int m_hoveredRow = -1;
    int m_pressedRow = -1;    // P032：item pressed 态（按下行）

    std::shared_ptr<ScrollBar> m_scrollBar;
    float m_scrollOffset = 0;
    std::shared_ptr<ScrollBar> m_hScrollBar;
    float m_hScrollOffset = 0;
    float m_contentWidth = 0;

    float m_indentWidth;
    float m_rowHeight;
    float m_lineSpacing = 0;
    float m_arrowGap;
    bool m_cycleNavigation = true;
    bool m_defaultExpand = false;

    FontName m_fontName;
    int m_fontSize;
    SharedFont m_font;

    // TreeView 增强：行控件登记与逐节点字体缓存（生命周期与节点一致）
    std::vector<std::shared_ptr<Control>> m_rowControls;               // 行控件集合（绘制排除 + 挂摘同步）
    std::unordered_map<const TreeNode*, SharedFont> m_nodeFonts;       // 逐节点字体缓存（节点销毁时同点清理）

    SColor m_bgColor = ConstDef::TREEVIEW_BG_COLOR;
    SColor m_borderColor = ConstDef::TREEVIEW_BORDER_COLOR;
    SColor m_hoverColor = ConstDef::TREEVIEW_HOVER_COLOR;
    SColor m_selectedColor = ConstDef::TREEVIEW_SELECTED_COLOR;
    // 文本色使用基类 StateColor m_textColor（P0-26 复核：原自有 SColor 遮蔽基类 → 属性路径无效）

    OnSelectHandler m_onSelect;
    OnSelectDataHandler m_onSelectData;
    OnExpandHandler m_onExpand;
    OnCollapseHandler m_onCollapse;
    OnClearNodeHandler m_onClearNode;

    void ensureFont();
    SharedFont getNodeFont(const std::shared_ptr<TreeNode>& node);  // 逐节点字体（fontSize>0 生效，否则 m_font）
    void syncRowControls();                                         // rebuildFlatRows 内调用：行控件挂/摘 + 登记
    bool toggleExpand(const string& id);
    void rebuildFlatRows();
    int hitTestRow(float mx, float my);
    bool hitTestArrow(int row, float mx);
    void updateScrollBar();
    void ensureSelectedVisible();
    void drawArrow(RenderDevice* dev, float x, float y, bool expanded);
    float calcContentWidth();
    void clearNodeRecursive(const std::shared_ptr<TreeNode>& node);
    float getStride() const { return m_rowHeight + m_lineSpacing; }
    void syncStateColor();

public:
    TreeView(Control* parent, const SRect& rect,
             float xScale = 1.0f, float yScale = 1.0f);
    ~TreeView() override;

    void create() override;
    void draw() override;
    bool handleEvent(std::shared_ptr<Event> event) override;
    void refreshScaleWith(float parentXX, float parentYY) override;

    // ── CRUD ──
    void setItems(const std::vector<std::shared_ptr<TreeNode>>& items);
    const std::vector<std::shared_ptr<TreeNode>>& getItems() const { return m_rootItems; }
    bool addChild(const std::string& parentId, std::shared_ptr<TreeNode> node);
    bool addRootItem(std::shared_ptr<TreeNode> node);
    bool removeNode(const std::string& id);
    bool setNodeLabel(const std::string& id, const std::string& label);
    bool setNodeUserData(const std::string& id, void* userData);
    void clearItems();
    std::shared_ptr<TreeNode> findNodeById(const std::string& id);

    bool expandNode(const std::string& id);
    bool collapseNode(const std::string& id);
    void expandAll();
    void collapseAll();
    bool selectNode(const std::string& id);
    void clearSelection() { m_selectedId.clear(); m_selectedRow = -1; }
    std::string getSelectedId() const { return m_selectedId; }

    void setIndentWidth(float px);
    float getIndentWidth() const { return m_indentWidth; }
    void setRowHeight(float px);
    float getRowHeight() const { return m_rowHeight; }
    void setLineSpacing(float px);
    float getLineSpacing() const { return m_lineSpacing; }
    void setArrowGap(float px);
    float getArrowGap() const { return m_arrowGap; }
    void setCycleNavigation(bool cycle) { m_cycleNavigation = cycle; }
    bool getCycleNavigation() const { return m_cycleNavigation; }
    void setDefaultExpand(bool expand) { m_defaultExpand = expand; }
    bool getDefaultExpand() const { return m_defaultExpand; }

    void setOnSelect(OnSelectHandler h) { m_onSelect = std::move(h); }
    void setOnSelectData(OnSelectDataHandler h) { m_onSelectData = std::move(h); }
    void setOnExpand(OnExpandHandler h) { m_onExpand = std::move(h); }
    void setOnCollapse(OnCollapseHandler h) { m_onCollapse = std::move(h); }
    void setOnClearNode(OnClearNodeHandler h) { m_onClearNode = std::move(h); }

    void setBgColor(const SColor& c);
    SColor getBgColor() const { return m_bgColor; }
    void setBorderColor(const SColor& c);
    SColor getBorderColor() const { return m_borderColor; }
    void setHoverColor(const SColor& c) { m_hoverColor = c; }
    SColor getHoverColor() const { return m_hoverColor; }
    void setSelectedColor(const SColor& c) { m_selectedColor = c; }
    SColor getSelectedColor() const { return m_selectedColor; }
    void setTextColor(const SColor& c) { m_textColor.setNormal(c); }
    SColor getTextColor() const { return m_textColor.getNormal(); }

    void setFont(FontName fontName);

    // Property system override
    int setColorProperty(const char* prop, SColor color) override;
    int setBoolProperty(const char* prop, int value) override;
    int setIntProperty(const char* prop, int value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setStringProperty(const char* prop, const char* value) override;
    int setPtrProperty(const char* prop, void* value) override;
    int setEnumProperty(const char* prop, const char* value) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getIntProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getColorProperty(const char* prop, SColor& out) override;   // P0-26：item 阴影色读回
    int setStateColorProperty(const char* prop, StateColor stateColor) override;   // P032：item 色四态对象
    int getStateColorProperty(const char* prop, StateColor& out) override;         // P032
    int getEnumProperty(const char* prop, const char*& out) override;
    int setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) override;
    int getStringProperty(const char* prop, const char*& out) override;
    int getPtrProperty(const char* prop, void*& out) override;

    static constexpr float LEFT_PADDING = 4.0f;
    static constexpr float RIGHT_GAP = 4.0f;
    void setFontSize(int size);
    int getFontSize() const { return m_fontSize; }
    FontName getFontName() const { return m_fontName; }
    Font* getFont() const { return m_font.get(); }

    std::shared_ptr<ScrollBar> getScrollBar() const { return m_scrollBar; }
    std::shared_ptr<ScrollBar> getHScrollBar() const { return m_hScrollBar; }
    int getFlatRowCount() const { return (int)m_flatRows.size(); }
    int getSelectedRow() const { return m_selectedRow; }
};

class TreeViewBuilder {
private:
    std::shared_ptr<TreeView> m_treeView;
public:
    TreeViewBuilder(Control* parent, const SRect& rect,
                    float xScale = 1.0f, float yScale = 1.0f)
        : m_treeView(std::make_shared<TreeView>(parent, rect, xScale, yScale)) {}

    TreeViewBuilder& setItems(const std::vector<std::shared_ptr<TreeNode>>& items) {
        m_treeView->setItems(items); return *this;
    }
    TreeViewBuilder& setIndentWidth(float px) {
        m_treeView->setIndentWidth(px); return *this;
    }
    TreeViewBuilder& setRowHeight(float px) {
        m_treeView->setRowHeight(px); return *this;
    }
    TreeViewBuilder& setLineSpacing(float px) {
        m_treeView->setLineSpacing(px); return *this;
    }
    TreeViewBuilder& setArrowGap(float px) {
        m_treeView->setArrowGap(px); return *this;
    }
    TreeViewBuilder& setCycleNavigation(bool cycle) {
        m_treeView->setCycleNavigation(cycle); return *this;
    }
    TreeViewBuilder& setDefaultExpand(bool expand) {
        m_treeView->setDefaultExpand(expand); return *this;
    }
    TreeViewBuilder& setOnSelect(TreeView::OnSelectHandler h) {
        m_treeView->setOnSelect(h); return *this;
    }
    TreeViewBuilder& setOnSelectData(TreeView::OnSelectDataHandler h) {
        m_treeView->setOnSelectData(h); return *this;
    }
    TreeViewBuilder& setOnExpand(TreeView::OnExpandHandler h) {
        m_treeView->setOnExpand(h); return *this;
    }
    TreeViewBuilder& setOnCollapse(TreeView::OnCollapseHandler h) {
        m_treeView->setOnCollapse(h); return *this;
    }
    TreeViewBuilder& setOnClearNode(TreeView::OnClearNodeHandler h) {
        m_treeView->setOnClearNode(h); return *this;
    }
    TreeViewBuilder& setFont(FontName fontName) {
        m_treeView->setFont(fontName); return *this;
    }
    TreeViewBuilder& setFontSize(int size) {
        m_treeView->setFontSize(size); return *this;
    }
    TreeViewBuilder& setBackgroundStateColor(StateColor sc) {
        m_treeView->setBackgroundStateColor(sc); return *this;
    }
    TreeViewBuilder& setBorderStateColor(StateColor sc) {
        m_treeView->setBorderStateColor(sc); return *this;
    }
    TreeViewBuilder& setBgColor(const SColor& c) {
        m_treeView->setBgColor(c); return *this;
    }
    TreeViewBuilder& setBorderColor(const SColor& c) {
        m_treeView->setBorderColor(c); return *this;
    }
    TreeViewBuilder& setHoverColor(const SColor& c) {
        m_treeView->setHoverColor(c); return *this;
    }
    TreeViewBuilder& setSelectedColor(const SColor& c) {
        m_treeView->setSelectedColor(c); return *this;
    }
    TreeViewBuilder& setTextColor(const SColor& c) {
        m_treeView->setTextColor(c); return *this;
    }
    TreeViewBuilder& setId(int id) {
        m_treeView->setId(id); return *this;
    }

    std::shared_ptr<TreeView> build() {
        m_treeView->create();
        return m_treeView;
    }
};

#endif
