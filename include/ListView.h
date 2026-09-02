// ============================================================================
// ListView.h -- 列表控件（multi=Report 多列详情 / single=List 单列，ListBox 替代）
// 设计：design/ListView_Design.md（决策点 0-10 已拍板，v10）
// 数据模型：变长行主序 + 写 API 强制补足（§5.0.3）；单元格控件/样式随行（稀疏）。
// ============================================================================
#pragma once

#include "ControlBase.h"
#include "SColor.h"
#include "ConstDef.h"
#include "Font.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class ScrollBar;
class TextRenderer;

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;

// 列头文本样式（每列一份；缺省 = 继承控件列头默认）
struct HeaderStyle {
    SColor textColor;                                          // 缺省占位黑；稀疏语义由列存在性判定
    FontName fontName = FontName::HarmonyOS_Sans_SC_Regular;
    int fontSize = 0;                                          // 0 = 继承控件列头字号
};

// 单元格样式（稀疏；缺省字段 = 继承控件默认；"缺省继承"由 map 存在性判定）
struct CellStyle {
    SColor bgColor;
    SColor textColor;
    FontName fontName = FontName::HarmonyOS_Sans_SC_Regular;
    int fontSize = 0;
};

struct ListColumn {
    string title;                                // 列头文本
    float width = 100.0f;                        // 列宽（局部 px）；single 模式自动铺满
    bool sortable = false;
    shared_ptr<Control> leadingControl;          // 列头图标（标题前，可空）
    HeaderStyle style;                           // 列头文本样式
};

struct ListRow {
    string id;
    vector<string> cells;                        // 每列文本（位置映射 + 尾部省略；写 API 强制补足）
    shared_ptr<Control> leadingControl;          // 行图标（首列前，可空）
    void* userData = nullptr;
    map<int, shared_ptr<Control>> cellControls;  // 单元格级控件（稀疏；排序/移动/删除跟随行）
    map<int, CellStyle> cellStyles;              // 单元格级样式（稀疏）
};

class ListView : public ControlImpl {
public:
    enum class Mode { Multi, Single };           // multi=Report / single=List（ListBox 替代）
    using SortComparator = std::function<bool(const string&, const string&)>;

    // ── 事件回调（四层同步一期）──
    using OnSelectionChangedHandler = std::function<void(shared_ptr<ListView>)>;
    using OnItemClickHandler        = std::function<void(shared_ptr<ListView>, int row, int col)>;
    using OnColumnSortHandler       = std::function<void(shared_ptr<ListView>, int col, bool ascending)>;

    ListView(Control* parent, const SRect& rect, float xScale = 1.0f, float yScale = 1.0f);

    // ── 行 API（§5.0.3；写 API 自动补足/截断到列数）──
    int  addRow(const string& id, const vector<string>& cells = {});
    int  insertRow(int index, const string& id, const vector<string>& cells = {});
    void removeRow(int index);
    void removeRowById(const string& id);
    int  getRowCount() const { return static_cast<int>(m_rows.size()); }
    ListRow& getRow(int index);
    ListRow* getRowById(const string& id);
    vector<string> getRowCells(int index) const;
    void setRowCells(int index, const vector<string>& cells);
    shared_ptr<Control> getRowLeadingControl(int index);
    void setRowLeadingControl(int index, shared_ptr<Control> ctl);

    // ── 列头 ──
    ListColumn& getColumn(int index);
    const vector<ListColumn>& getColumns() const { return m_columns; }
    void setColumns(const vector<ListColumn>& columns);          // 批量重建；所有行同步补足/截断
    void setColumnTitle(int index, const string& title);
    void setColumnSortable(int index, bool sortable);
    void setColumnWidth(int index, float width);                 // 钳制 minColumnWidth；relayout
    void setColumnLeadingControl(int index, shared_ptr<Control> ctl);
    void setColumnHeaderStyle(int index, const HeaderStyle& style);
    HeaderStyle getColumnHeaderStyle(int index) const;

    // ── 列 ──
    int  addColumn(const string& title, float width, bool sortable = false);
    int  insertColumn(int index, const string& title, float width, bool sortable = false);
    void removeColumn(int index);                                // 所有行同步删除该列
    int  getColumnCount() const { return static_cast<int>(m_columns.size()); }
    vector<string> getColumnValues(int colIndex) const;          // 恒 = 行数，缺失空串
    void setColumnValues(int colIndex, const vector<string>& values);

    // ── 单元格 ──
    string getCell(int row, int col) const;
    void setCell(int row, int col, const string& text);
    shared_ptr<Control> getCellLeadingControl(int row, int col);
    void setCellLeadingControl(int row, int col, shared_ptr<Control> ctl);

    // ── 单元格样式（稀疏）──
    void setCellStyle(int row, int col, const CellStyle& style);
    CellStyle getCellStyle(int row, int col) const;              // 未设置 → 默认
    void clearCellStyle(int row, int col);

    // ── 属性 setter（全部触发 relayout/重绘；四层见设计文档 §5.6）──
    void setFont(FontName fontName);                             // 控件级字体（字体继承链目标）
    void setFontSize(int size);
    int  getFontSize() const { return m_fontSize; }
    FontName getFontName() const { return m_fontName; }
    void setMode(Mode mode);
    void setMultiSelect(bool on);
    void setSelectedRow(int index);                              // -1 = 清除选中
    void setRowHeight(float px);
    void setHeaderHeight(float px);
    void setGridlines(bool on);
    void setHorizontalGridlines(bool on);
    void setHoverHighlight(bool on);
    void setMinColumnWidth(float px);
    void setCycleNavigation(bool on);
    void setSortColumn(int colIndex);                            // -1 = 清除排序；触发重排
    void setSortAscending(bool ascending);
    // single 模式便捷层（ListBox 替代）：addItem(id,text) ≡ addRow(id,{text})
    int  addItem(const string& id, const string& text);

    // ── 排序（§5.4.1：缺省字典序 + 用户回调；stable_sort；选中按 id 跟随）──
    void setColumnSorter(int columnIndex, SortComparator cmp);
    void clearColumnSorter(int columnIndex);
    void sortByColumn(int colIndex, bool ascending);             // 内部/显式触发

    // ── 查询 ──
    Mode getMode() const { return m_viewMode; }
    int  getSelectedRow() const;                                 // 多选时返回首个；无 -1
    const std::set<int>& getSelectedRows() const { return m_selectedRows; }
    float getRowHeight() const { return m_rowHeight; }
    float getHeaderHeight() const { return m_headerHeight; }

    // ── 事件注册 ──
    void setOnSelectionChanged(OnSelectionChangedHandler h) { m_onSelectionChanged = std::move(h); }
    void setOnItemClick(OnItemClickHandler h)               { m_onItemClick = std::move(h); }
    void setOnColumnSort(OnColumnSortHandler h)             { m_onColumnSort = std::move(h); }

    // ── 引擎接口 ──
    void create(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;
    void resized(SRect newRect) override;

    // ── 属性系统 override（通用属性键；数据类走专用 CABI 不在此）──
    int setEnumProperty(const char* prop, const char* value) override;   // "mode"
    int setBoolProperty(const char* prop, int value) override;           // multi-select/gridlines/…
    int setFloatProperty(const char* prop, float value) override;        // row-height/header-height/min-column-width
    int setIntProperty(const char* prop, int value) override;            // selected-index/sort-column
    int getEnumProperty(const char* prop, const char*& out) override;
    int getBoolProperty(const char* prop, int& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getIntProperty(const char* prop, int& out) override;

private:
    friend class ListViewBuilder;

    // ── 内部工具 ──
    void rebuildLayout();                         // 列 x 偏移缓存 + 滚动范围 + 子控件定位
    void updateScrollBars();                      // 垂直/水平滚动条范围与可见性
    void syncChildControls();                     // leadingControl/cellControls 定位与挂摘
    int  hitTestRow(float y) const;               // 内容区行命中（含滚动）；-1 未命中
    int  hitTestColumn(float x) const;            // 列命中（含水平滚动）；-1 未命中
    float columnX(int index) const;               // 列左缘 x（含水平滚动偏移）
    float totalContentWidth() const;              // Σ列宽（single 模式 = 控件宽）
    int  visibleStartRow() const;
    int  visibleEndRow() const;                   // 含夹取
    void ensureRowVisible(int index);             // 键盘移动后滚动跟随
    void clampSelectionToCount();                 // 行删除后清理越界选中
    void fireSelectionChanged();

    // ── 成员 ──
    vector<ListColumn> m_columns;
    vector<ListRow> m_rows;
    std::set<int> m_selectedRows;                 // 选择集合（前瞻 Grid：(row,col) 扩展零重构）
    int  m_sortColumn = -1;
    bool m_sortAscending = true;
    map<int, SortComparator> m_columnSorters;
    Mode m_viewMode = Mode::Multi;

    float m_rowHeight = 24.0f;                    // ConstDef::TREEVIEW_DEFAULT_ROW_HEIGHT 同款
    float m_headerHeight = 28.0f;
    bool  m_gridlines = true;                     // multi 缺省开；single 恒关
    bool  m_horizontalGridlines = false;
    bool  m_hoverHighlight = true;
    float m_minColumnWidth = 20.0f;
    bool  m_cycleNavigation = true;
    bool  m_multiSelect = true;

    int   m_hoveredRow = -1;
    int   m_scrollOffsetV = 0;                    // 垂直滚动偏移（px，行级步进）
    float m_hScrollOffset = 0.f;                  // 水平滚动（px，整行平移）
    int   m_dragCol = -1;                         // 列宽拖拽中的列索引；-1 非拖拽
    float m_dragStartX = 0.f;
    float m_dragStartWidth = 0.f;

    shared_ptr<ScrollBar> m_scrollBarV;
    shared_ptr<ScrollBar> m_scrollBarH;

    // ── 字体（仿 TreeView：控件级缺省 + 逐格覆盖缓存）──
    FontName m_fontName = FontName::HarmonyOS_Sans_SC_Regular;
    int     m_fontSize = 14;
    SharedFont m_font;
    std::unordered_map<unsigned long long, SharedFont> m_fontCache; // key=(fontName<<16)|fontSize
    vector<float> m_colX;                         // 列左缘 x 累计缓存（含 0 起点）
    float m_contentWidth = 0.f;                   // Σ列宽缓存

    // ── 配色（一期固定；颜色属性键与 bool "hover" 同串冲突，暂不暴露）──
    SColor m_textColor{235, 235, 235};
    SColor m_headerTextColor{200, 200, 205};
    SColor m_hoverColor{60, 60, 70};
    SColor m_selectedColor{59, 130, 246};
    SColor m_gridlineColor{55, 55, 62};
    SColor m_headerBgColor{45, 45, 52};

    // ── 内部绘制辅助 ──
    void ensureFont();
    SharedFont fontFor(FontName name, int size);  // 缓存化逐格字体
    void drawHeaderText(const ListColumn& col, float x, float w);
    TextRenderer* renderer() { return getTextRenderer(); }

    OnSelectionChangedHandler m_onSelectionChanged;
    OnItemClickHandler        m_onItemClick;
    OnColumnSortHandler       m_onColumnSort;

    std::set<Control*> m_attachedChildren;   // 子控件挂树去重（leadingControl/cellControls）
};

// ── 声明式 Builder（LabelBuilder 同款惯例）──
class ListViewBuilder {
private:
    std::shared_ptr<ListView> m_lv;
public:
    ListViewBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    ListViewBuilder& setMode(ListView::Mode mode);
    ListViewBuilder& setMultiSelect(bool on);
    ListViewBuilder& setSelectedRow(int index);
    ListViewBuilder& setRowHeight(float px);
    ListViewBuilder& setHeaderHeight(float px);
    ListViewBuilder& setGridlines(bool on);
    ListViewBuilder& setHorizontalGridlines(bool on);
    ListViewBuilder& setHoverHighlight(bool on);
    ListViewBuilder& setCycleNavigation(bool on);
    ListViewBuilder& setMinColumnWidth(float px);
    ListViewBuilder& addColumn(const std::string& title, float width, bool sortable = false);
    ListViewBuilder& addRow(const std::string& id, const std::vector<std::string>& cells = {});
    ListViewBuilder& setOnSelectionChanged(ListView::OnSelectionChangedHandler h);
    ListViewBuilder& setOnItemClick(ListView::OnItemClickHandler h);
    ListViewBuilder& setOnColumnSort(ListView::OnColumnSortHandler h);
    std::shared_ptr<ListView> build(void);
};
