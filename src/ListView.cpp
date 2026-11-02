// ============================================================================
// ListView.cpp -- 列表控件实现（design/ListView_Design.md）
// multi=Report（列头/多列/排序/网格线） / single=List（单列，ListBox 替代）
// 可见行窗口化渲染 O(可见行×可见列)；垂直行级滚动 + 水平整行平移。
// ============================================================================
#include "ListView.h"
#include "TextDraw.h"
#include "ScrollBar.h"
#include "PropertyNames.h"
#include "RenderDevice.h"
#include "TextRenderer.h"
#include "ResourceProvider.h"
#include "ConstDef.h"
#include "InlineEditBox.h"
#include "FocusManager.h"
#include "PlatformUtils.h"
#include "Cursor.h"

#include <algorithm>
#include <cmath>

using std::max;
using std::min;

namespace {
void parseIndexSet(const std::string& s, std::set<int>& out);
void parseIdSet(const std::string& s, std::set<std::string>& out);
}

namespace {
constexpr float kScrollbarW = 14.0f;     // 与 ConstDef::SCROLLBAR_WIDTH 对齐的兜底
constexpr float kTextLeftPad   = 6.0f;   // 单元格/表头文字左距
constexpr float kSortArrowW    = 10.0f;  // 排序箭头宽
constexpr float kSortArrowHalf = 5.0f;   // 排序箭头半高/半宽
constexpr float kSortArrowTip  = 3.0f;   // 排序箭头尖偏移
constexpr float kSortArrowRightGap = 14.0f; // 箭头距列右缘
constexpr float kHeaderLineInset = 2.0f; // 表头分隔线上下内缩
constexpr float kIconSlotInset   = 4.0f; // 行图标槽内缩
constexpr float kColResizeHitW   = 8.0f; // 列宽拖拽分隔线命中半宽（CR005）
constexpr float kDefaultColumnWidth = 100.0f; // 无列定义时单元格兜底宽
constexpr float kScrollbarStep   = 20.0f; // 水平滚动条步长
}

// ── 构造 ──
ListView::ListView(Control* parent, const SRect& rect, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
{
    m_ctlType = ControlType::ListView;
    m_rect = rect;
    m_textColor.setNormal(SColor(235, 235, 235));   // P0-26：原自有缺省色迁入基类 StateColor
    setTransparent(true);                           // P0-26：缺省无底色；显式设背景色自动取消透明
}

// ── 字体 ──
void ListView::ensureFont() {
    if (m_font) return;
    TextRenderer* renderer = getTextRenderer();
    ResourceProvider* provider = getResourceProvider();
    if (!renderer || !provider) return;
    auto it = ConstDef::fontFiles.find(m_fontName);
    if (it == ConstDef::fontFiles.end()) return;
    string fontPath = it->second;   // P0-54：统一相对路径约定（与 Label/Actor 一致）
    auto data = provider->readFile(fontPath);
    if (!data || data->empty()) return;
    int scaledSize = static_cast<int>(m_fontSize * getScaleXX());
    m_font = renderer->loadFontFromMemoryWithText(data->data(), data->size(), scaledSize, "W");
}

void ListView::setFont(FontName fontName) {
    if (m_fontName == fontName && m_font) return;
    m_fontName = fontName;
    m_font.reset();
    m_fontCache.clear();  // 逐格字体缓存随控件级字体失效
    if (m_isCreated) ensureFont();
}

void ListView::setFontSize(int size) {
    if (m_fontSize == size) return;
    m_fontSize = size;
    m_font.reset();
    m_fontCache.clear();  // 逐格字体缓存随控件级字号失效
    if (m_isCreated) ensureFont();
}

SharedFont ListView::fontFor(FontName name, int size) {
    if (size <= 0 || (name == m_fontName && size == m_fontSize)) { ensureFont(); return m_font; }
    const unsigned long long key =
        (static_cast<unsigned long long>(name) << 16) | static_cast<unsigned>(size);
    auto it = m_fontCache.find(key);
    if (it != m_fontCache.end()) return it->second;

    TextRenderer* renderer = getTextRenderer();
    ResourceProvider* provider = getResourceProvider();
    if (!renderer || !provider) return nullptr;
    auto fit = ConstDef::fontFiles.find(name);
    if (fit == ConstDef::fontFiles.end()) return nullptr;
    string fontPath = fit->second;   // P0-54：统一相对路径约定（与 Label/Actor 一致）
    auto data = provider->readFile(fontPath);
    if (!data || data->empty()) return nullptr;
    int scaledSize = static_cast<int>(size * getScaleXX());
    SharedFont f = renderer->loadFontFromMemoryWithText(data->data(), data->size(), scaledSize, "W");
    m_fontCache[key] = f;
    return f;
}

// ── 引擎：创建（滚动条，仿 TreeView::create）──
void ListView::create(void) {
    if (m_isCreated) return;
    ControlImpl::create();

    if (m_scrollBarV) removeControl(m_scrollBarV);
    m_scrollBarV = ScrollBarBuilder(this,
        SRect(m_rect.width - kScrollbarW, 0, kScrollbarW, m_rect.height),
        ScrollBarOrientation::Vertical, 1.0f, 1.0f)
        .setStepSize(m_rowHeight)
        .setPageSize(m_rect.height)
        .setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float, float, float) {
            if (!m_scrollBarV) return;
            const int nv = static_cast<int>(m_scrollBarV->getValue());
            if (nv == m_scrollOffsetV) return;   // setValue 会在每帧 update 时重复通知
            cancelEditForMutation();
            cancelColumnResizeInternal(m_apiResizeActive);
            m_scrollOffsetV = nv;
        })
        .build();
    addControl(m_scrollBarV);

    if (m_scrollBarH) removeControl(m_scrollBarH);
    m_scrollBarH = ScrollBarBuilder(this,
        SRect(0, m_rect.height - kScrollbarW, m_rect.width, kScrollbarW),
        ScrollBarOrientation::Horizontal, 1.0f, 1.0f)
        .setStepSize(kScrollbarStep)
        .setPageSize(m_rect.width)
        .setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float, float, float) {
            if (!m_scrollBarH) return;
            const float nv = m_scrollBarH->getValue();
            if (fabs(nv - m_hScrollOffset) <= 0.01f) return;
            cancelEditForMutation();
            cancelColumnResizeInternal(m_apiResizeActive);
            m_hScrollOffset = nv;
        })
        .build();
    addControl(m_scrollBarH);

    ensureFont();
    rebuildLayout();
}

// ── 布局 ──
float ListView::columnX(int index) const {
    if (index < 0 || index >= static_cast<int>(m_colX.size())) return 0.f;
    return m_colX[index] - m_hScrollOffset;
}

float ListView::totalContentWidth() const {
    if (m_viewMode == Mode::Single) return m_rect.width;
    return m_contentWidth;
}

int ListView::visibleStartRow() const {
    if (m_rowHeight <= 0.f) return 0;
    return static_cast<int>(m_scrollOffsetV / m_rowHeight);
}

int ListView::visibleEndRow() const {
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    const float dataH = max(0.f, m_rect.height - dataY);
    if (m_rowHeight <= 0.f) return 0;
    const int n = static_cast<int>(std::ceil(dataH / m_rowHeight)) + 1;
    return min(getRowCount(), visibleStartRow() + n);
}

void ListView::updateScrollBars() {
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    const float dataH = max(0.f, m_rect.height - dataY);
    // 垂直滚动条：仅内容超高时显示
    if (m_scrollBarV) {
        const float range = max(0.f, getRowCount() * m_rowHeight - dataH);
        m_scrollBarV->setRange(0.f, range);
        m_scrollBarV->setPageSize(dataH);
        m_scrollBarV->setVisible(range > 0.f);          // 内容不超高 → 隐藏
        m_scrollOffsetV = static_cast<int>(std::clamp(
            static_cast<float>(m_scrollOffsetV), 0.f, range));
        m_scrollBarV->setValue(static_cast<float>(m_scrollOffsetV));
    }
    // 水平滚动条：仅内容超宽时显示
    const float hRange = max(0.f, totalContentWidth() - m_rect.width);
    if (m_scrollBarH) {
        m_scrollBarH->setRange(0.f, hRange);
        m_scrollBarH->setPageSize(m_rect.width);
        m_scrollBarH->setVisible(hRange > 0.f);          // 内容不超宽 → 隐藏
        m_hScrollOffset = std::clamp(m_hScrollOffset, 0.f, hRange);
        m_scrollBarH->setValue(m_hScrollOffset);
    }
}

void ListView::rebuildLayout() {
    // single 模式：单列铺满控件宽；multi：Σ列宽
    m_colX.clear();
    m_contentWidth = 0.f;
    if (m_viewMode == Mode::Single) {
        m_colX.push_back(0.f);
        m_contentWidth = m_rect.width;
    } else {
        float acc = 0.f;
        m_colX.push_back(0.f);
        for (const auto& c : m_columns) {
            acc += c.width;
            m_colX.push_back(acc);
        }
        m_contentWidth = acc;
    }
    updateScrollBars();
    syncChildControls();
}

// 子控件挂树管理（防重复 addControl；定位随滚动/布局更新）
void ListView::syncChildControls() {
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    for (int i = 0; i < getRowCount(); ++i) {
        ListRow& row = m_rows[i];
        const float y = dataY + (i * m_rowHeight - m_scrollOffsetV);

        auto attach = [&](shared_ptr<Control> ctl, float x, float y2, float w, float h) {
            if (!ctl) return;
            if (m_attachedChildren.find(ctl.get()) == m_attachedChildren.end()) {
                addControl(ctl);
                ctl->setContext(getContext());
                ctl->create();
                m_attachedChildren.insert(ctl.get());
            }
            ctl->setVisible(y2 >= dataY - h && y2 < m_rect.height);   // 视口外隐藏
            ctl->setRect(SRect(x, y2, w, h));
        };

        const float slot = m_rowHeight - 2.f * kIconSlotInset;
        if (row.leadingControl && getColumnCount() > 0)
            attach(row.leadingControl, columnX(0) + kIconSlotInset, y + kIconSlotInset, slot, slot);
        const int colCountEff = (m_viewMode == Mode::Single) ? 1 : getColumnCount();
        for (auto& [col, ctl] : row.cellControls) {
            if (col < 0 || col >= colCountEff) continue;
            const float cw = (m_viewMode == Mode::Single) ? m_rect.width : m_columns[col].width;
            auto lit = row.cellControlLayouts.find(col);
            const bool fill = (lit == row.cellControlLayouts.end()) ? false
                              : (lit->second == (uint8_t)CellControlLayout::Fill);
            if (fill) {
                const bool fully = cellFullyVisible(i, col);
                if (!fully && ctl && getMouseCapturedChild() == ctl.get()) clearMouseCapture(true);
                if (!fully && ctl) focusHandoffFromChild(ctl.get());
                attach(ctl, columnX(col) + 2.f, y + 1.f,
                       max(0.f, cw - 4.f), max(0.f, m_rowHeight - 2.f));
                ctl->setVisible(fully);   // FILL：仅完整可见显示（不让子控件越界）
            } else {
                attach(ctl, columnX(col) + kIconSlotInset, y + kIconSlotInset, min(slot, cw - 2.f * kIconSlotInset), slot);
            }
        }
    }
    // 列头图标（multi）
    if (m_viewMode == Mode::Multi) {
        for (int c = 0; c < getColumnCount(); ++c) {
            auto& lc = m_columns[c].leadingControl;
            if (!lc) continue;
            const float hs = m_headerHeight - 8.f;
            if (m_attachedChildren.find(lc.get()) == m_attachedChildren.end()) {
                addControl(lc);
                lc->setContext(getContext());
                lc->create();
                m_attachedChildren.insert(lc.get());
            }
            lc->setRect(SRect(columnX(c) + kTextLeftPad, kIconSlotInset, hs, hs));
        }
    }
}

// ── 数据 API：补足/截断工具 ──
static vector<string> normalizeCells(vector<string> cells, int colCount) {
    if (colCount <= 0) return cells;   // single 无列定义时不截断
    cells.resize(max(0, colCount), string());
    return cells;
}

int ListView::addRow(const string& id, const vector<string>& cells) {
    return insertRow(getRowCount(), id, cells);
}
int ListView::insertRow(int index, const string& id, const vector<string>& cells) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    ListRow row;
    row.id = id;
    row.cells = normalizeCells(cells, getColumnCount());
    index = std::clamp(index, 0, getRowCount());
    m_rows.insert(m_rows.begin() + index, std::move(row));
    rebuildLayout();
    return index;
}
void ListView::removeRow(int index) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    if (index < 0 || index >= getRowCount()) return;
    ListRow& row = m_rows[index];
    if (row.leadingControl) { removeControl(row.leadingControl); m_attachedChildren.erase(row.leadingControl.get()); }
    for (auto& [c, ctl] : row.cellControls) { removeControl(ctl); m_attachedChildren.erase(ctl.get()); }
    m_rows.erase(m_rows.begin() + index);
    clampSelectionToCount();
    rebuildLayout();
}
void ListView::removeRowById(const string& id) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    for (int i = 0; i < getRowCount(); ++i)
        if (m_rows[i].id == id) { removeRow(i); return; }
}

bool ListView::moveRow(int from, int to) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    const int n = getRowCount();
    if (from < 0 || from >= n || to < 0 || to >= n || from == to) return false;
    ListRow row = m_rows[from];
    m_rows.erase(m_rows.begin() + from);
    m_rows.insert(m_rows.begin() + to, row);
    clampSelectionToCount();
    rebuildLayout();
    return true;
}
ListRow& ListView::getRow(int index) { return m_rows[index]; }
ListRow* ListView::getRowById(const string& id) {
    for (auto& r : m_rows) if (r.id == id) return &r;
    return nullptr;
}
vector<string> ListView::getRowCells(int index) const {
    if (index < 0 || index >= getRowCount()) return {};
    return m_rows[index].cells;
}
void ListView::setRowCells(int index, const vector<string>& cells) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    if (index < 0 || index >= getRowCount()) return;
    m_rows[index].cells = normalizeCells(cells, getColumnCount());
}
shared_ptr<Control> ListView::getRowLeadingControl(int index) {
    if (index < 0 || index >= getRowCount()) return nullptr;
    return m_rows[index].leadingControl;
}
void ListView::setRowLeadingControl(int index, shared_ptr<Control> ctl) {
    if (index < 0 || index >= getRowCount()) return;
    m_rows[index].leadingControl = std::move(ctl);
    syncChildControls();
}

// ── 列头 ──
ListColumn& ListView::getColumn(int index) { return m_columns[index]; }
void ListView::setColumns(const vector<ListColumn>& columns) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    m_columns = columns;
    for (auto& r : m_rows) r.cells = normalizeCells(r.cells, getColumnCount());
    for (int i = 0; i < getRowCount(); ++i) {
        vector<int> drop;
        for (auto& [c, ctl] : m_rows[i].cellControls) if (c >= getColumnCount()) drop.push_back(c);
        for (int c : drop) removeCellControlAt(i, c, true);
    }
    clampSelectionToCount();
    rebuildLayout();
}
void ListView::setColumnTitle(int index, const string& title) {
    if (index >= 0 && index < getColumnCount()) { m_columns[index].title = title; }
}
void ListView::setColumnSortable(int index, bool sortable) {
    if (index >= 0 && index < getColumnCount()) m_columns[index].sortable = sortable;
}
void ListView::setColumnWidth(int index, float width) {
    if (index < 0 || index >= getColumnCount()) return;
    cancelEditForMutation();
    if (m_apiResizeActive) cancelColumnResizeInternal(true);   // 外部变更：取消会话并通知
    applyColumnWidthInternal(index, width);
}
void ListView::setColumnLeadingControl(int index, shared_ptr<Control> ctl) {
    if (index < 0 || index >= getColumnCount()) return;
    m_columns[index].leadingControl = std::move(ctl);
    syncChildControls();
}
void ListView::setColumnHeaderStyle(int index, const HeaderStyle& style) {
    if (index >= 0 && index < getColumnCount()) m_columns[index].style = style;
}
HeaderStyle ListView::getColumnHeaderStyle(int index) const {
    if (index < 0 || index >= getColumnCount()) return HeaderStyle{};
    return m_columns[index].style;
}
void ListView::setColumnHeaderBackground(int index, SColor color) {   // P0-56
    if (index < 0 || index >= getColumnCount()) return;
    m_columns[index].style.background = color;
    m_columns[index].style.hasBackground = true;
}
void ListView::setColumnHeaderShadow(int index, SColor color, float offsetX, float offsetY) {   // P0-56
    if (index < 0 || index >= getColumnCount()) return;
    auto& st = m_columns[index].style;
    st.shadowColor = color;
    st.hasShadow = true;
    st.shadowOffset = SPoint(offsetX, offsetY);
}

// ── 列 ──
int ListView::addColumn(const string& title, float width, bool sortable) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    return insertColumn(getColumnCount(), title, width, sortable);
}
int ListView::insertColumn(int index, const string& title, float width, bool sortable) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    ListColumn col;
    col.title = title; col.width = width; col.sortable = sortable;
    index = std::clamp(index, 0, getColumnCount());
    m_columns.insert(m_columns.begin() + index, col);
    // 所有行同步插空串；cellControls/cellStyles 列索引迁移（≥index 的 +1）
    for (auto& r : m_rows) {
        r.cells.insert(r.cells.begin() + index, string());
        map<int, shared_ptr<Control>> nc;
        for (auto& [c, v] : r.cellControls) nc[c < index ? c : c + 1] = v;
        r.cellControls = std::move(nc);
        map<int, uint8_t> nl;
        for (auto& [c, v] : r.cellControlLayouts) nl[c < index ? c : c + 1] = v;
        r.cellControlLayouts = std::move(nl);
        map<int, CellStyle> ns;
        for (auto& [c, v] : r.cellStyles) ns[c < index ? c : c + 1] = v;
        r.cellStyles = std::move(ns);
    }
    rebuildLayout();
    return index;
}
void ListView::removeColumn(int index) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    if (index < 0 || index >= getColumnCount()) return;
    m_columns.erase(m_columns.begin() + index);
    for (int i = 0; i < getRowCount(); ++i) {
        ListRow& r = m_rows[i];
        removeCellControlAt(i, index, true);
        if (index < static_cast<int>(r.cells.size())) r.cells.erase(r.cells.begin() + index);
        map<int, shared_ptr<Control>> nc;
        for (auto& [c, v] : r.cellControls) { if (c == index) continue; nc[c > index ? c - 1 : c] = v; }
        r.cellControls = std::move(nc);
        map<int, uint8_t> nl;
        for (auto& [c, v] : r.cellControlLayouts) { if (c == index) continue; nl[c > index ? c - 1 : c] = v; }
        r.cellControlLayouts = std::move(nl);
        map<int, CellStyle> ns;
        for (auto& [c, v] : r.cellStyles) { if (c == index) continue; ns[c > index ? c - 1 : c] = v; }
        r.cellStyles = std::move(ns);
    }
    if (m_sortColumn == index) { m_sortColumn = -1; }
    else if (m_sortColumn > index) { --m_sortColumn; }
    rebuildLayout();
}
vector<string> ListView::getColumnValues(int colIndex) const {
    vector<string> out;
    out.reserve(getRowCount());
    for (int i = 0; i < getRowCount(); ++i) out.push_back(getCell(i, colIndex));
    return out;
}
void ListView::setColumnValues(int colIndex, const vector<string>& values) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    const int n = min(static_cast<int>(values.size()), getRowCount());
    for (int i = 0; i < n; ++i) setCell(i, colIndex, values[i]);
}

// ── 单元格 ──
string ListView::getCell(int row, int col) const {
    if (row < 0 || row >= getRowCount() || col < 0 || col >= static_cast<int>(m_rows[row].cells.size()))
        return string();
    return m_rows[row].cells[col];
}
void ListView::setCell(int row, int col, const string& text) {
    if (row < 0 || row >= getRowCount() || col < 0) return;
    auto& cells = m_rows[row].cells;
    if (col >= static_cast<int>(cells.size())) cells.resize(col + 1, string());
    cells[col] = text;
}
shared_ptr<Control> ListView::getCellLeadingControl(int row, int col) {
    if (row < 0 || row >= getRowCount()) return nullptr;
    auto it = m_rows[row].cellControls.find(col);
    return it != m_rows[row].cellControls.end() ? it->second : nullptr;
}
void ListView::setCellLeadingControl(int row, int col, shared_ptr<Control> ctl) {
    if (!ctl) { clearCellControl(row, col); return; }
    setCellControl(row, col, std::move(ctl), CellControlLayout::Leading);
}

// ── 单元格样式 ──
void ListView::setCellStyle(int row, int col, const CellStyle& style) {
    if (row < 0 || row >= getRowCount() || col < 0) return;
    CellStyle s = style;
    s.hasBg = true;   // P0-65①：仅常态背景显式入口置位（hover-only 条目常态不填）
    m_rows[row].cellStyles[col] = std::move(s);
}
void ListView::setCellTextColor(int row, int col, SColor color) {   // P0-62①
    if (row < 0 || row >= getRowCount() || col < 0) return;
    CellStyle& cs = m_rows[row].cellStyles[col];
    cs.textColor = color;
    cs.hasTextColor = true;
}
void ListView::setCellHoverBackgroundColor(int row, int col, SColor color) {   // P0-64③
    if (row < 0 || row >= getRowCount() || col < 0) return;
    CellStyle& cs = m_rows[row].cellStyles[col];
    cs.hoverBgColor = color;
    cs.hasHoverBg = true;
}
void ListView::setCellBackgroundColor(int row, int col, SColor color) {   // P0-66③
    if (row < 0 || row >= getRowCount() || col < 0) return;
    CellStyle& cs = m_rows[row].cellStyles[col];
    cs.bgColor = color;
    cs.hasBg = true;
}
void ListView::setCellFontSize(int row, int col, int size) {   // P0-66③
    if (row < 0 || row >= getRowCount() || col < 0) return;
    m_rows[row].cellStyles[col].fontSize = size;
}
void ListView::setCellShadow(int row, int col, SColor color, float offsetX, float offsetY) {   // P0-66③
    if (row < 0 || row >= getRowCount() || col < 0) return;
    CellStyle& cs = m_rows[row].cellStyles[col];
    cs.textShadowColor = color;
    cs.shadowEnabled = color.alpha() > 0;
    cs.shadowOffsetX = offsetX;
    cs.shadowOffsetY = offsetY;
}
void ListView::setCellFontName(int row, int col, FontName name) {   // P0-63②
    if (row < 0 || row >= getRowCount() || col < 0) return;
    CellStyle& cs = m_rows[row].cellStyles[col];
    cs.fontName = name;
    cs.hasFontName = true;
}
void ListView::setColumnHeaderFontName(int col, FontName name) {    // P0-63③
    if (col < 0 || col >= getColumnCount()) return;
    m_columns[col].style.fontName = name;
    m_columns[col].style.hasFontName = true;
}
CellStyle ListView::getCellStyle(int row, int col) const {
    if (row < 0 || row >= getRowCount()) return CellStyle{};
    auto it = m_rows[row].cellStyles.find(col);
    return it != m_rows[row].cellStyles.end() ? it->second : CellStyle{};
}
void ListView::clearCellStyle(int row, int col) {
    if (row < 0 || row >= getRowCount()) return;
    m_rows[row].cellStyles.erase(col);
}

// ── 属性 setter ──
void ListView::setMode(Mode mode) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    if (m_viewMode == mode) return;
    m_viewMode = mode;
    if (mode == Mode::Single) m_gridlines = false;   // 单列恒关（显示语义）
    // single 模式无列定义时不截断 cells（保留 addItem 的单列数据）
    if (getColumnCount() > 0)
        for (auto& r : m_rows) r.cells = normalizeCells(r.cells, getColumnCount());
    rebuildLayout();
}
void ListView::setMultiSelect(bool on) { m_multiSelect = on; if (!on) setSelectedRow(getSelectedRow()); }
void ListView::setSelectedRow(int index) {
    m_selectedRows.clear();
    if (index >= 0 && index < getRowCount()) m_selectedRows.insert(index);
    fireSelectionChanged();
}
void ListView::setRowHeight(float px)  {    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
 if (px > 0 && m_rowHeight != px) { m_rowHeight = px; rebuildLayout(); } }
void ListView::setHeaderHeight(float px) {    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
 if (px > 0 && m_headerHeight != px) { m_headerHeight = px; rebuildLayout(); } }
void ListView::setGridlines(bool on)           { m_gridlines = on; }
void ListView::setHorizontalGridlines(bool on) { m_horizontalGridlines = on; }
void ListView::setHoverHighlight(bool on)      { m_hoverHighlight = on; }
void ListView::setMinColumnWidth(float px) {
    m_minColumnWidth = max(1.f, px);
    for (auto& c : m_columns) c.width = max(c.width, m_minColumnWidth);
    rebuildLayout();
}
void ListView::setCycleNavigation(bool on) { m_cycleNavigation = on; }
void ListView::setSortColumn(int colIndex) {
    m_sortColumn = colIndex;
    sortByColumn(colIndex, m_sortAscending);
}
void ListView::setSortAscending(bool ascending) {
    m_sortAscending = ascending;
    if (m_sortColumn >= 0) sortByColumn(m_sortColumn, ascending);
}
int ListView::addItem(const string& id, const string& text) {
    return addRow(id, {text});
}

// ── 排序（stable_sort；选中集合按 id 跟随）──
void ListView::setColumnSorter(int columnIndex, SortComparator cmp) {
    m_columnSorters[columnIndex] = std::move(cmp);
}
void ListView::clearColumnSorter(int columnIndex) { m_columnSorters.erase(columnIndex); }

void ListView::sortByColumn(int colIndex, bool ascending) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    if (colIndex < 0 || colIndex >= getColumnCount()) return;
    m_sortColumn = colIndex;
    m_sortAscending = ascending;

    // 记录选中 id（排序后按 id 重映射，资源管理器风格）
    std::set<string> selectedIds;
    for (int i : m_selectedRows)
        if (i >= 0 && i < getRowCount()) selectedIds.insert(m_rows[i].id);

    SortComparator cmp = std::less<string>();
    auto it = m_columnSorters.find(colIndex);
    if (it != m_columnSorters.end()) cmp = it->second;

    const bool asc = ascending;
    std::stable_sort(m_rows.begin(), m_rows.end(),
        [&](const ListRow& a, const ListRow& b) {
            const string& sa = colIndex < static_cast<int>(a.cells.size()) ? a.cells[colIndex] : string();
            const string& sb = colIndex < static_cast<int>(b.cells.size()) ? b.cells[colIndex] : string();
            return asc ? cmp(sa, sb) : cmp(sb, sa);
        });

    // 选中重映射
    m_selectedRows.clear();
    for (int i = 0; i < getRowCount(); ++i)
        if (selectedIds.count(m_rows[i].id)) m_selectedRows.insert(i);

    fireSelectionChanged();
}

// ── 选择辅助 ──
int ListView::getSelectedRow() const {
    return m_selectedRows.empty() ? -1 : *m_selectedRows.begin();
}
void ListView::fireSelectionChanged() {
    if (m_onSelectionChanged) {
        if (auto self = std::dynamic_pointer_cast<ListView>(
                std::dynamic_pointer_cast<Control>(getThis())))
            m_onSelectionChanged(self);
    }
    GridPayload sel{getSelectedRow(), static_cast<int>(m_selectedRows.size()), 0};
    fireCCallback(PropertyNames::kEventListSelectionChanged, CCallbackData::Grid, &sel);
}
void ListView::clampSelectionToCount() {
    for (auto it = m_selectedRows.begin(); it != m_selectedRows.end();) {
        if (*it >= getRowCount()) it = m_selectedRows.erase(it);
        else ++it;
    }
}
void ListView::ensureRowVisible(int index) {
    if (index < 0) return;
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    const float dataH = max(0.f, m_rect.height - dataY);
    const float top = index * m_rowHeight;
    const float bottom = top + m_rowHeight;
    if (top < m_scrollOffsetV) { m_scrollOffsetV = static_cast<int>(top); }
    else if (bottom > m_scrollOffsetV + dataH) {
        m_scrollOffsetV = static_cast<int>(bottom - dataH);
    }
    if (m_scrollBarV) m_scrollBarV->setValue(static_cast<float>(m_scrollOffsetV));
}

// ── 命中测试（局部坐标）──
int ListView::hitTestRow(float y) const {
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    if (y < dataY || m_rowHeight <= 0.f) return -1;
    const int row = static_cast<int>((y - dataY + m_scrollOffsetV) / m_rowHeight);
    return (row >= 0 && row < getRowCount()) ? row : -1;
}
int ListView::hitTestColumn(float x) const {
    for (int c = 0; c < getColumnCount(); ++c) {
        const float x0 = columnX(c);
        const float w = (m_viewMode == Mode::Single) ? m_rect.width : m_columns[c].width;
        if (x >= x0 && x < x0 + w) return c;
    }
    return -1;
}

// ── 绘制 ──
void ListView::draw(void) {
    cleanupPendingEdit();
    // 可见性守卫（同 TreeView/ControlImpl 语义）：TabControl 切页隐藏后不再绘制
    if (!m_visible) return;
    RenderDevice* dev = getRenderDevice();
    TextRenderer* renderer = getTextRenderer();
    if (!dev || !renderer) return;
    ControlImpl::beforeDraw();   // P0-26：背景（四态经基类状态机）
    ensureFont();
    if (!m_font) return;



    const float fontH = renderer->getFontHeight(m_font.get());
    const bool multi = (m_viewMode == Mode::Multi);
    const float dataY = multi ? m_headerHeight : 0.f;

    // 缩放：本地布局坐标 × scale，绘制原点取 getDrawRect（scale 生效）
    const SRect dr = getDrawRect();
    const float sx = getScaleXX(), sy = getScaleYY();
    const float ox = dr.left, oy = dr.top;
    const float headerH = m_headerHeight * sy;
    const float rowH = m_rowHeight * sy;
    dev->pushClipRect(SRect(ox, oy, dr.width, dr.height));

    // ── 列头行（仅 multi）──
    if (multi) {
        dev->setDrawColor(m_headerBgColor);   // P0-56：控件级 header-background（缺省内部常量）
        dev->fillRect(SRect(ox, oy, dr.width, headerH));

        for (int c = 0; c < getColumnCount(); ++c) {
            const float x = ox + columnX(c) * sx;
            const float w = m_columns[c].width * sx;
            if (x + w < ox || x > ox + dr.width) continue;

            const HeaderStyle& st = m_columns[c].style;
            SharedFont hf = fontFor(st.hasFontName ? st.fontName : m_fontName,   // P0-63③：字体名稀疏
                                    st.fontSize > 0 ? st.fontSize : m_fontSize);
            if (!hf) hf = m_font;
            const float hfh = renderer->getFontHeight(hf.get());

            // 标题文本（leadingControl 槽让位） + P0-56：per-column 背景/文字色优先
            float tx = x + kTextLeftPad * sx;
            if (m_columns[c].leadingControl) tx += headerH - kIconSlotInset * sx;
            dev->pushClipRect(SRect(x, oy, w, headerH));
            if (st.hasBackground) {
                dev->setDrawColor(st.background);
                dev->fillRect(SRect(x, oy, w, headerH));
            }
            const SColor headerColor = st.hasTextColor ? st.textColor : m_headerTextColor;
            const bool useShadow = st.hasShadow || m_headerShadowEnabled;
            if (useShadow) {
                const SColor sc = st.hasShadow ? st.shadowColor : m_headerShadowColor;
                const SPoint so = st.hasShadow ? st.shadowOffset : m_headerShadowOffset;
                const float ty = oy + (headerH - hfh) / 2.f;
                TextDraw::withShadow(renderer, hf.get(), m_columns[c].title, tx, ty,
                                     tx + so.x * sx, ty + so.y * sy, headerColor, true, sc);
            } else {
                renderer->drawText(hf.get(), m_columns[c].title, tx,
                                   oy + (headerH - hfh) / 2.f, headerColor);
            }
            // 排序箭头（sortable 且当前排序列）
            if (m_columns[c].sortable && c == m_sortColumn) {
                const float ax = x + w - kSortArrowRightGap * sx, ay = oy + headerH / 2.f;
                if (m_sortAscending)
                    dev->drawTriangle(ax, ay + kSortArrowHalf * sx, ax + kSortArrowW * sx, ay + kSortArrowHalf * sx, ax + kSortArrowHalf * sx, ay - kSortArrowTip * sx, headerColor);
                else
                    dev->drawTriangle(ax, ay - kSortArrowHalf * sx, ax + kSortArrowW * sx, ay - kSortArrowHalf * sx, ax + kSortArrowHalf * sx, ay + kSortArrowTip * sx, headerColor);
            }
            dev->popClipRect();
            // 分隔线
            dev->setDrawColor(m_gridlineColor);
            dev->drawLine(x + w - 1.f, oy + kHeaderLineInset * sy, x + w - 1.f, oy + headerH - kHeaderLineInset * sy);
        }
    }

    // ── 数据区 ──
    const int start = visibleStartRow();
    const int end = visibleEndRow();
    const int visColFrom = 0, visColTo = getColumnCount();   // 列裁剪经 x 越界跳过



    for (int i = start; i < end; ++i) {
        const float y = oy + dataY * sy + (i * rowH - m_scrollOffsetV * sy);
        ListRow& row = m_rows[i];   // P032：行级样式需非 const 访问（resolveStateColor）
        const bool selected = m_selectedRows.count(i) != 0;

        // P032：行状态（disabled > pressed > hover > normal）
        const ControlState rowSt = row.style.disabled ? ControlState::Disabled
                                 : (i == m_pressedRow) ? ControlState::Pressed
                                 : (i == m_hoveredRow) ? ControlState::Hover
                                                       : ControlState::Normal;
        // 行背景：item 样式（优先）> 选中 > hover
        if (row.hasStyle) {
            dev->setDrawColor(ControlImpl::resolveStateColor(row.style.bgColor, rowSt));
            dev->fillRect(SRect(ox, y, dr.width, rowH));
            if (row.style.borderVisible) {
                dev->setDrawColor(ControlImpl::resolveStateColor(row.style.borderColor, rowSt));
                dev->drawRect(SRect(ox, y, dr.width, rowH));
            }
        } else if (selected) {
            dev->setDrawColor(m_selectedColor);
            dev->fillRect(SRect(ox, y, dr.width, rowH));
        }
        else if (i == m_hoveredRow && m_hoverHighlight) {
            dev->setDrawColor(m_hoverColor); dev->fillRect(SRect(ox, y, dr.width, rowH));
        }

        // 单元格
        const int cols = getColumnCount();
        const int effCols = (m_viewMode == Mode::Single) ? 1 : cols;
        for (int c = 0; c < effCols; ++c) {
            const float x = columnX(c) * sx;
            const float w = (m_viewMode == Mode::Single) ? dr.width
                            : (c < cols ? m_columns[c].width * sx : kDefaultColumnWidth * sx);
            if (x + w < 0 || x > dr.width) continue;

            // cellStyle 背景（覆盖高亮之上，差异着色可见）
            auto styleIt = row.cellStyles.find(c);
            const bool cellRowDisabled = row.style.disabled;
            const bool cellHovered = (i == m_hoveredRow && m_hoverHighlight) && !selected && !cellRowDisabled;
            if (styleIt != row.cellStyles.end()) {
                const CellStyle& csC = styleIt->second;
                const bool hasBase = csC.hasBg && csC.bgColor.alpha() > 0;   // P0-65①：hover-only 条目常态不填
                if (hasBase) {
                    dev->setDrawColor(csC.bgColor);
                    dev->fillRect(SRect(ox + x, y, w, rowH));
                }
                if (csC.hasHoverBg && cellHovered) {
                    // P0-64③：显式 hover 背景（优先；不叠加）——无常态 bg 也可用
                    dev->setDrawColor(csC.hoverBgColor);
                    dev->fillRect(SRect(ox + x, y, w, rowH));
                } else if (hasBase && !cellRowDisabled) {
                    // P0-63①：cell bg 之上以半透明高亮叠加（selected 优先 hover），
                    // 保证 per-cell 着色与交互反馈并存；文字/网格线绘制仍在叠加之后
                    const SColor* tint = nullptr;
                    if (selected) tint = &m_selectedColor;
                    else if (i == m_hoveredRow && m_hoverHighlight) tint = &m_hoverColor;
                    if (tint) {
                        dev->setDrawColor(SColor(tint->red(), tint->green(), tint->blue(),
                                                 ConstDef::LIST_HIGHLIGHT_OVERLAY_ALPHA));
                        dev->fillRect(SRect(ox + x, y, w, rowH));
                    }
                }
            }

            // 文本（首列让位 leadingControl/cellControl 槽）
            auto clit = row.cellControlLayouts.find(c);
            const bool leadingCellCtl = (row.cellControls.count(c) != 0) &&
                (clit == row.cellControlLayouts.end() || clit->second == (uint8_t)CellControlLayout::Leading);
            const bool hasCtl = (c == 0 && row.leadingControl) || leadingCellCtl;
            float tx = ox + x + kTextLeftPad * sx + (hasCtl ? rowH - kIconSlotInset * sx : 0.f);
            const string text = (c < static_cast<int>(row.cells.size())) ? row.cells[c] : string();
            if (text.empty()) continue;

            FontName fn = m_fontName; int fs = m_fontSize;
            SColor tc = row.hasStyle
                      ? ControlImpl::resolveStateColor(row.style.textColor, rowSt)   // P032：行文本四态
                      : ControlImpl::resolveStateColor(m_textColor, getState());     // P0-26：文本四态
            const CellStyle* csPtr = nullptr;
            if (styleIt != row.cellStyles.end()) {
                const CellStyle& cs = styleIt->second;
                if (cs.fontSize > 0) fs = cs.fontSize;
                if (cs.hasTextColor) tc = cs.textColor;   // P0-62①：未设走行/控件级链
                if (cs.hasFontName) fn = cs.fontName;     // P0-63②：字体名稀疏（未设 → 控件级）
                csPtr = &cs;
            }
            SharedFont cf = fontFor(fn, fs);
            if (!cf) cf = m_font;
            const float fh = renderer->getFontHeight(cf.get());
            // P032：阴影优先级 cell > row
            const bool shadowEnabled = csPtr ? csPtr->shadowEnabled : row.style.shadowEnabled;
            const SColor shadowColor = csPtr ? csPtr->textShadowColor
                                             : ControlImpl::resolveStateColor(row.style.textShadowColor, rowSt);
            const float offX = csPtr ? csPtr->shadowOffsetX : row.style.shadowOffsetX;
            const float offY = csPtr ? csPtr->shadowOffsetY : row.style.shadowOffsetY;

            dev->pushClipRect(SRect(ox + x, y, w, rowH));
            TextDraw::withShadow(renderer, cf.get(), text,
                                 tx, y + (rowH - fh) / 2.f,
                                 tx + offX, y + (rowH - fh) / 2.f + offY,
                                 tc, shadowEnabled, shadowColor);   // P0-26：cell 阴影
            dev->popClipRect();
        }

        // 网格线
        if (multi && m_gridlines) {
            dev->setDrawColor(m_gridlineColor);
            for (int c = 0; c < cols; ++c) {
                const float gx = ox + (columnX(c) + ((c < cols) ? m_columns[c].width : 0.f)) * sx - 1.f;
                if (gx >= ox && gx <= ox + dr.width)
                    dev->drawLine(gx, y, gx, y + rowH);
            }
        }
        if (m_horizontalGridlines) {
            dev->setDrawColor(m_gridlineColor);
            dev->drawLine(ox, y + rowH - 1.f, ox + dr.width, y + rowH - 1.f);
        }
    }

    dev->popClipRect();

    // 子控件（leadingControl/cellControls/滚动条）最后绘制：
    // 必须位于行背景/表头背景之上（此前在函数开头绘制会被选中/hover 行背景覆盖）
    ControlImpl::draw();
    afterDraw();  // P0-26：边框 + 焦点环（统一基类路径，替换原手动 drawFocusRing）
}

// ── 事件 ──
bool ListView::handleEvent(shared_ptr<Event> event) {
    cleanupPendingEdit();
    if (event && (event->m_type == EventType::FocusLost || event->m_type == EventType::MouseLeave)) {
        m_overDivider = false;
        setColumnResizeCursor(false);
        clearMouseCapture(true);
    }
    if (!m_enable || !m_visible) return false;
    // P0-114：子控件统一派发（EditBox/滚动条/cell control；含 captured）
    if (dispatchToChild(event)) return true;

    // 命中测试：屏幕坐标逆变换到本地布局空间（scale 生效）
    const SRect hitDr = getDrawRect();
    const float hitSx = getScaleXX() != 0.f ? getScaleXX() : 1.f;
    const float hitSy = getScaleYY() != 0.f ? getScaleYY() : 1.f;
    const float lx = (event->mouseButton.x - hitDr.left) / hitSx;
    const float ly = (event->mouseButton.y - hitDr.top) / hitSy;
    const bool inside = isContainsPoint(event->mouseButton.x, event->mouseButton.y);

    // ── 键盘导航（决策点 5 一期）──
    if (event->m_type == EventType::KeyDown && getFocused()) {
        KeyCode kc = event->keyEvent.keycode;
        const int cur = getSelectedRow();
        if (kc == KeyCode::Up) {
            if (cur > 0) { setSelectedRow(cur - 1); ensureRowVisible(cur - 1); }
            else if (m_cycleNavigation && getRowCount() > 0) { setSelectedRow(getRowCount() - 1); ensureRowVisible(getRowCount() - 1); }
            return true;
        }
        if (kc == KeyCode::Down) {
            if (cur >= 0 && cur + 1 < getRowCount()) { setSelectedRow(cur + 1); ensureRowVisible(cur + 1); }
            else if (m_cycleNavigation && getRowCount() > 0) { setSelectedRow(0); ensureRowVisible(0); }
            return true;
        }
        if (kc == KeyCode::Home && getRowCount() > 0) { setSelectedRow(0); ensureRowVisible(0); return true; }
        if (kc == KeyCode::End  && getRowCount() > 0) { setSelectedRow(getRowCount() - 1); ensureRowVisible(getRowCount() - 1); return true; }
    }

    // ── MouseDown ──
    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left && inside) {
        if (!getFocused()) setFocused(true);

        const float dataYl = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
        if (m_viewMode == Mode::Multi && ly < m_headerHeight) {
            if (m_apiResizeActive) return true;   // P0-113：API 会话期间运行时列头交互拒绝
            // 列头区：分隔线拖拽（含最后一列可见边界）or 排序点击
            {
                const int hitCol = hitTestColumnDivider(lx, ly);
                if (hitCol >= 0) {
                    m_dragCol = hitCol; m_dragStartX = event->mouseButton.x;
                    m_dragStartWidth = m_columns[hitCol].width;
                    setColumnResizeCursor(true);
                    return true;
                }
            }
            const int col = hitTestColumn(lx + m_hScrollOffset);
            if (col >= 0 && m_columns[col].sortable) {
                const bool asc = (m_sortColumn == col) ? !m_sortAscending : true;
                sortByColumn(col, asc);
                if (m_onColumnSort) {
                    if (auto self = std::dynamic_pointer_cast<ListView>(
                            std::dynamic_pointer_cast<Control>(getThis())))
                        m_onColumnSort(self, col, asc);
                }
                GridPayload sort{col, asc ? 1 : 0, 0};
                fireCCallback(PropertyNames::kEventColumnSort, CCallbackData::Grid, &sort);
                return true;
            }
            return true;   // 列头其余区域消费（不透传）
        }

        // 数据区：选择
        const int row = hitTestRow(ly);
        if (row >= 0) {
            m_pressedRow = row;   // P032：行 pressed 态
            KeyMod mod = KeyMod::None;
        if (auto* ib = getInputBackend()) mod = ib->getModState();
        const bool ctrl = (mod == KeyMod::LCtrl || mod == KeyMod::RCtrl ||
                           mod == static_cast<KeyMod>(0x0040 | 0x0080));
            if (m_multiSelect && ctrl) {
                if (m_selectedRows.count(row)) m_selectedRows.erase(row);
                else m_selectedRows.insert(row);
                fireSelectionChanged();
            } else {
                setSelectedRow(row);
            }
            ensureRowVisible(row);
            const int col = hitTestColumn(lx + m_hScrollOffset);
            if (m_onItemClick) {
                if (auto self = std::dynamic_pointer_cast<ListView>(
                        std::dynamic_pointer_cast<Control>(getThis())))
                    m_onItemClick(self, row, max(0, col));
            }
            GridPayload click{row, max(0, col), 0};
            fireCCallback(PropertyNames::kEventItemClick, CCallbackData::Grid, &click);
            return true;
        }
    }

    // ── MouseUp：结束拖拽 + P0-111 双击进入编辑 ──
    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left && inside) {
        const bool wasDraggingCol = (m_dragCol >= 0);
        m_dragCol = -1;
        const float upLy = (event->mouseButton.y - getDrawRect().top) / (getScaleYY() != 0.f ? getScaleYY() : 1.f);
        const float upLx = (event->mouseButton.x - getDrawRect().left) / (getScaleXX() != 0.f ? getScaleXX() : 1.f);
        const bool over = (upLy >= 0.f && upLy < m_headerHeight) && hitTestColumnDivider(upLx, upLy) >= 0;
        if (over != m_overDivider) { m_overDivider = over; setColumnResizeCursor(over); }
        m_pressedRow = -1;   // P032：抬起清除行 pressed
        if (!wasDraggingCol && m_cellEditing && !m_editing) {
            const float dataYl = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
            if (ly >= dataYl) {
                int row = hitTestRow(ly);
                int col = hitTestColumn(lx + m_hScrollOffset);
                if (row >= 0 && col >= 0) {
                    bool blocked = false;
                    ListRow& lrow = m_rows[row];
                    if (lrow.leadingControl && lrow.leadingControl->isContainsPoint(event->mouseButton.x, event->mouseButton.y))
                        blocked = true;
                    auto it = lrow.cellControls.find(col);
                    auto lit = lrow.cellControlLayouts.find(col);
                    const bool fillCtl = (lit != lrow.cellControlLayouts.end() &&
                                          lit->second == (uint8_t)CellControlLayout::Fill);
                    if (!blocked && fillCtl && it != lrow.cellControls.end() && it->second)
                        blocked = true;   // P0-114：FILL 区域双击屏蔽编辑
                    if (!blocked && it != lrow.cellControls.end() && it->second &&
                        it->second->isContainsPoint(event->mouseButton.x, event->mouseButton.y))
                        blocked = true;
                    if (!blocked) {
                        const uint64_t now = Platform::GetTicks();
                        const float dx = event->mouseButton.x - m_lastClickX;
                        const float dy = event->mouseButton.y - m_lastClickY;
                        if (row == m_lastClickRow && col == m_lastClickCol &&
                            (now - m_lastClickTick) <= 450 && fabs(dx) <= 4.f && fabs(dy) <= 4.f) {
                            beginEditCell(row, col);
                            m_lastClickTick = 0;
                            m_lastClickRow = m_lastClickCol = -1;
                        } else {
                            m_lastClickTick = now;
                            m_lastClickX = event->mouseButton.x;
                            m_lastClickY = event->mouseButton.y;
                            m_lastClickRow = row;
                            m_lastClickCol = col;
                        }
                    }
                }
            }
        }
    }

    // ── MouseMove：hover / 列宽拖拽 ──
    if (event->m_type == EventType::MouseMove) {
        const SRect hDr = getDrawRect();
        const float hSx = getScaleXX() != 0.f ? getScaleXX() : 1.f;
        const float hSy = getScaleYY() != 0.f ? getScaleYY() : 1.f;
        const float mx = (event->mousePos.x - hDr.left) / hSx;
        const float my = (event->mousePos.y - hDr.top) / hSy;
        const float hdr = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
        m_hoveredRow = (isContainsPoint(event->mousePos.x, event->mousePos.y) && my >= hdr)
                       ? hitTestRow(my) : -1;
        if (m_dragCol >= 0 && m_dragCol < getColumnCount()) {
            setColumnResizeCursor(true);
            const float dx = event->mousePos.x - m_dragStartX;
            setColumnWidth(m_dragCol, m_dragStartWidth + dx);
            return true;
        }
        if (isContainsPoint(event->mousePos.x, event->mousePos.y) && my < hdr) {
            const bool over = hitTestColumnDivider(mx, my) >= 0;
            if (over != m_overDivider) { m_overDivider = over; setColumnResizeCursor(over); }
        } else if (m_overDivider) {
            m_overDivider = false;
            setColumnResizeCursor(false);
        }
    }

    // ── MouseWheel：滚动（无滚动余量时透传，不空转消费）──
    if (event->m_type == EventType::MouseWheel && inside) {
        cancelEditForMutation();
        cancelColumnResizeInternal(m_apiResizeActive);
        const float vMax = max(0.f, getRowCount() * m_rowHeight -
            (m_rect.height - ((m_viewMode == Mode::Multi) ? m_headerHeight : 0.f)));
        const float hMax = max(0.f, totalContentWidth() - m_rect.width);
        if (vMax <= 0.f && hMax <= 0.f) return false;
        if (event->mouseWheel.scrollY != 0.f && m_scrollBarV) {
            const float nv = std::clamp(
                static_cast<float>(m_scrollOffsetV) - event->mouseWheel.scrollY * m_rowHeight * 3.f, 0.f, vMax);
            m_scrollOffsetV = static_cast<int>(nv);
            m_scrollBarV->setValue(nv);
            syncChildControls();
        }
        if (event->mouseWheel.scrollX != 0.f && m_scrollBarH) {
            const float nh = std::clamp(
                m_hScrollOffset - event->mouseWheel.scrollX * 40.f, 0.f, hMax);
            m_hScrollOffset = nh;
            m_scrollBarH->setValue(nh);
            syncChildControls();
        }
        return true;
    }

    return false;
}

// ── 几何变化 ──
void ListView::setRect(SRect rect) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    ControlImpl::setRect(rect);
    rebuildLayout();
}
void ListView::resized(SRect newRect) {
    cancelEditForMutation();
    cancelColumnResizeInternal(m_apiResizeActive);
    ControlImpl::resized(newRect);
    rebuildLayout();
}

// ── 属性系统 override（通用属性键；数据/对象类走 CABI 专用函数）──

int ListView::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kMode) == 0 && value) {
        if (strcmp(value, PropertyNames::kModeSingle) == 0)      { setMode(Mode::Single);  return 1; }
        if (strcmp(value, PropertyNames::kModeMulti) == 0)       { setMode(Mode::Multi);   return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kFont) == 0 && value) {
        setFont(FontNameFromString(value));
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}
int ListView::findRowById(const std::string& id) const {
    for (size_t i = 0; i < m_rows.size(); ++i)
        if (m_rows[i].id == id) return static_cast<int>(i);
    return -1;
}

// P032：item（行）级样式（item-id 定位）
int ListView::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kEditableColumns) == 0) {
        m_editableColumnsRaw = value ? value : "";
        parseIndexSet(m_editableColumnsRaw, m_editableCols);
        cancelEditForMutation();
        return 1;
    }
    if (strcmp(prop, PropertyNames::kReadOnlyColumns) == 0) {
        m_readOnlyColumnsRaw = value ? value : "";
        parseIndexSet(m_readOnlyColumnsRaw, m_readOnlyCols);
        cancelEditForMutation();
        return 1;
    }
    if (strcmp(prop, PropertyNames::kReadOnlyRowIds) == 0) {
        m_readOnlyRowIdsRaw = value ? value : "";
        parseIdSet(m_readOnlyRowIdsRaw, m_readOnlyRows);
        cancelEditForMutation();
        return 1;
    }
    if (strcmp(prop, "item-id") == 0) {
        if (!value || !value[0]) { m_itemTargetId.clear(); return 1; }
        if (findRowById(value) < 0) return 0;
        m_itemTargetId = value;
        return 1;
    }
    return ControlImpl::setStringProperty(prop, value);
}
int ListView::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kEditableColumns) == 0)  { out = m_editableColumnsRaw.c_str();  return 1; }
    if (strcmp(prop, PropertyNames::kReadOnlyColumns) == 0)  { out = m_readOnlyColumnsRaw.c_str();  return 1; }
    if (strcmp(prop, PropertyNames::kReadOnlyRowIds) == 0)   { out = m_readOnlyRowIdsRaw.c_str();   return 1; }
    if (strcmp(prop, PropertyNames::kEditOldText) == 0)      { out = m_editOldText.c_str();         return 1; }
    if (strcmp(prop, PropertyNames::kEditNewText) == 0)      { out = m_editNewText.c_str();         return 1; }
    if (strcmp(prop, "item-id") == 0) { out = m_itemTargetId.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}
int ListView::setStateColorProperty(const char* prop, StateColor stateColor) {
    if (strcmp(prop, PropertyNames::kItemBackground) == 0 ||
        strcmp(prop, PropertyNames::kItemBorder) == 0 ||
        strcmp(prop, PropertyNames::kItemText) == 0 ||
        strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0) {
        int idx = findRowById(m_itemTargetId);
        if (idx < 0) return 0;
        ListRow& row = m_rows[idx];
        if (strcmp(prop, PropertyNames::kItemBackground) == 0)      row.style.bgColor = stateColor;
        else if (strcmp(prop, PropertyNames::kItemBorder) == 0)     row.style.borderColor = stateColor;
        else if (strcmp(prop, PropertyNames::kItemText) == 0)       row.style.textColor = stateColor;
        else                                                        row.style.textShadowColor = stateColor;
        row.hasStyle = true;
        return 1;
    }
    return ControlImpl::setStateColorProperty(prop, stateColor);
}
int ListView::getStateColorProperty(const char* prop, StateColor& out) {
    int idx = findRowById(m_itemTargetId);
    if (idx < 0) return 0;
    const RowStyle& st = m_rows[idx].style;
    if (strcmp(prop, PropertyNames::kItemBackground) == 0)      { out = st.bgColor;         return 1; }
    if (strcmp(prop, PropertyNames::kItemBorder) == 0)          { out = st.borderColor;     return 1; }
    if (strcmp(prop, PropertyNames::kItemText) == 0)            { out = st.textColor;       return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0)  { out = st.textShadowColor; return 1; }
    return ControlImpl::getStateColorProperty(prop, out);
}
int ListView::setColorProperty(const char* prop, SColor color) {
    // P0-64②：控件级行 hover/选中色（复用通用键 hover/selected，与 TreeView 一致）
    if (strcmp(prop, PropertyNames::kTreeHover) == 0)    { m_hoverColor = color;    return 1; }
    if (strcmp(prop, PropertyNames::kTreeSelected) == 0) { m_selectedColor = color; return 1; }
    // P0-56：控件级表头样式
    if (strcmp(prop, PropertyNames::kHeaderText) == 0)       { m_headerTextColor = color; return 1; }
    if (strcmp(prop, PropertyNames::kHeaderBackground) == 0) { m_headerBgColor = color;   return 1; }
    if (strcmp(prop, PropertyNames::kHeaderShadow) == 0)     { m_headerShadowColor = color; m_headerShadowEnabled = true; return 1; }
    if (strcmp(prop, PropertyNames::kItemBackground) == 0 ||
        strcmp(prop, PropertyNames::kItemBorder) == 0 ||
        strcmp(prop, PropertyNames::kItemText) == 0 ||
        strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0) {
        int idx = findRowById(m_itemTargetId);
        if (idx < 0) return 0;
        ListRow& row = m_rows[idx];
        if (strcmp(prop, PropertyNames::kItemBackground) == 0)      row.style.bgColor.setNormal(color);
        else if (strcmp(prop, PropertyNames::kItemBorder) == 0)     row.style.borderColor.setNormal(color);
        else if (strcmp(prop, PropertyNames::kItemText) == 0)       row.style.textColor.setNormal(color);
        else { row.style.textShadowColor.setNormal(color); row.style.shadowEnabled = true; }
        row.hasStyle = true;
        return 1;
    }
    return ControlImpl::setColorProperty(prop, color);
}
int ListView::getColorProperty(const char* prop, SColor& out) {
    // P0-64②：控件级行 hover/选中色
    if (strcmp(prop, PropertyNames::kTreeHover) == 0)    { out = m_hoverColor;    return 1; }
    if (strcmp(prop, PropertyNames::kTreeSelected) == 0) { out = m_selectedColor; return 1; }
    // P0-56：控件级表头样式
    if (strcmp(prop, PropertyNames::kHeaderText) == 0)       { out = m_headerTextColor;  return 1; }
    if (strcmp(prop, PropertyNames::kHeaderBackground) == 0) { out = m_headerBgColor;    return 1; }
    if (strcmp(prop, PropertyNames::kHeaderShadow) == 0)     { out = m_headerShadowColor; return 1; }
    int idx = findRowById(m_itemTargetId);
    if (idx >= 0) {
        const RowStyle& st = m_rows[idx].style;
        if (strcmp(prop, PropertyNames::kItemBackground) == 0)      { out = st.bgColor.getNormal();         return 1; }
        if (strcmp(prop, PropertyNames::kItemBorder) == 0)          { out = st.borderColor.getNormal();     return 1; }
        if (strcmp(prop, PropertyNames::kItemText) == 0)            { out = st.textColor.getNormal();       return 1; }
        if (strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0)  { out = st.textShadowColor.getNormal(); return 1; }
    }
    return ControlImpl::getColorProperty(prop, out);
}

int ListView::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kCellEditing) == 0) {
        m_cellEditing = (value != 0);
        if (!m_cellEditing) cancelEditForMutation();
        return 1;
    }
    if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0 ||
        strcmp(prop, PropertyNames::kItemShadow) == 0 ||
        strcmp(prop, PropertyNames::kItemDisabled) == 0) {   // P032：item 级开关（item-id 定位）
        int idx = findRowById(m_itemTargetId);
        if (idx < 0) return 0;
        ListRow& row = m_rows[idx];
        if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0) row.style.borderVisible = (value != 0);
        else if (strcmp(prop, PropertyNames::kItemShadow) == 0)   row.style.shadowEnabled = (value != 0);
        else                                                      row.style.disabled = (value != 0);
        row.hasStyle = true;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kMultiSelect) == 0)       { setMultiSelect(value != 0);      return 1; }
    if (strcmp(prop, PropertyNames::kGridlines) == 0)          { setGridlines(value != 0);         return 1; }
    if (strcmp(prop, PropertyNames::kHorizontalGridlines) == 0){ setHorizontalGridlines(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kHoverHighlight) == 0)     { setHoverHighlight(value != 0);    return 1; }
    if (strcmp(prop, PropertyNames::kSortAscending) == 0)      { setSortAscending(value != 0);     return 1; }
    if (strcmp(prop, PropertyNames::kCycleNavigation) == 0)    { setCycleNavigation(value != 0);   return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int ListView::setFloatProperty(const char* prop, float value) {
    // P0-56：控件级表头阴影偏移
    if (strcmp(prop, PropertyNames::kHeaderShadowOffsetX) == 0) { m_headerShadowOffset.x = value; return 1; }
    if (strcmp(prop, PropertyNames::kHeaderShadowOffsetY) == 0) { m_headerShadowOffset.y = value; return 1; }
    if (strcmp(prop, PropertyNames::kRowHeight) == 0)      { setRowHeight(value);      return 1; }
    if (strcmp(prop, PropertyNames::kHeaderHeight) == 0)   { setHeaderHeight(value);   return 1; }
    if (strcmp(prop, PropertyNames::kMinColumnWidth) == 0) { setMinColumnWidth(value); return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}
int ListView::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kSelectedIndex) == 0) { setSelectedRow(value); return 1; }
    if (strcmp(prop, PropertyNames::kFontSize) == 0)      { setFontSize(value);     return 1; }
    if (strcmp(prop, PropertyNames::kSortColumn) == 0)    {
        if (value < 0) { m_sortColumn = -1; }
        else { setSortColumn(value); }
        return 1;
    }
    return ControlImpl::setIntProperty(prop, value);
}
int ListView::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kMode) == 0) { out = (m_viewMode == Mode::Single)
        ? PropertyNames::kModeSingle : PropertyNames::kModeMulti; return 1; }
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_fontName); return 1; }
    return ControlImpl::getEnumProperty(prop, out);
}
int ListView::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kCellEditing) == 0) { out = m_cellEditing ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0 ||
        strcmp(prop, PropertyNames::kItemShadow) == 0 ||
        strcmp(prop, PropertyNames::kItemDisabled) == 0) {   // P032
        int idx = findRowById(m_itemTargetId);
        if (idx < 0) return 0;
        const RowStyle& st = m_rows[idx].style;
        if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0) out = st.borderVisible ? 1 : 0;
        else if (strcmp(prop, PropertyNames::kItemShadow) == 0)   out = st.shadowEnabled ? 1 : 0;
        else                                                      out = st.disabled ? 1 : 0;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kMultiSelect) == 0)       { out = m_multiSelect ? 1 : 0;            return 1; }
    if (strcmp(prop, PropertyNames::kGridlines) == 0)          { out = m_gridlines ? 1 : 0;               return 1; }
    if (strcmp(prop, PropertyNames::kHorizontalGridlines) == 0){ out = m_horizontalGridlines ? 1 : 0;     return 1; }
    if (strcmp(prop, PropertyNames::kHoverHighlight) == 0)     { out = m_hoverHighlight ? 1 : 0;          return 1; }
    if (strcmp(prop, PropertyNames::kSortAscending) == 0)      { out = m_sortAscending ? 1 : 0;           return 1; }
    if (strcmp(prop, PropertyNames::kCycleNavigation) == 0)    { out = m_cycleNavigation ? 1 : 0;         return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}
int ListView::getFloatProperty(const char* prop, float& out) {
    // P0-56：控件级表头阴影偏移
    if (strcmp(prop, PropertyNames::kHeaderShadowOffsetX) == 0) { out = m_headerShadowOffset.x; return 1; }
    if (strcmp(prop, PropertyNames::kHeaderShadowOffsetY) == 0) { out = m_headerShadowOffset.y; return 1; }
    if (strcmp(prop, PropertyNames::kRowHeight) == 0)      { out = m_rowHeight;      return 1; }
    if (strcmp(prop, PropertyNames::kHeaderHeight) == 0)   { out = m_headerHeight;   return 1; }
    if (strcmp(prop, PropertyNames::kMinColumnWidth) == 0) { out = m_minColumnWidth; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}
int ListView::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kSelectedIndex) == 0) { out = getSelectedRow(); return 1; }
    if (strcmp(prop, PropertyNames::kSortColumn) == 0)    { out = m_sortColumn;    return 1; }
    if (strcmp(prop, PropertyNames::kFontSize) == 0)      { out = m_fontSize;      return 1; }
    return ControlImpl::getIntProperty(prop, out);
}

// ── ListViewBuilder（声明式构建，LabelBuilder 同款惯例） ──

ListViewBuilder::ListViewBuilder(Control* parent, SRect rect, float xScale, float yScale)
    : m_lv(nullptr)
{
    m_lv = std::make_shared<ListView>(parent, rect, xScale, yScale);
}
ListViewBuilder& ListViewBuilder::setMode(ListView::Mode mode)        { m_lv->setMode(mode); return *this; }
ListViewBuilder& ListViewBuilder::setMultiSelect(bool on)             { m_lv->setMultiSelect(on); return *this; }
ListViewBuilder& ListViewBuilder::setSelectedRow(int index)           { m_lv->setSelectedRow(index); return *this; }
ListViewBuilder& ListViewBuilder::setRowHeight(float px)              { m_lv->setRowHeight(px); return *this; }
ListViewBuilder& ListViewBuilder::setHeaderHeight(float px)           { m_lv->setHeaderHeight(px); return *this; }
ListViewBuilder& ListViewBuilder::setGridlines(bool on)               { m_lv->setGridlines(on); return *this; }
ListViewBuilder& ListViewBuilder::setHorizontalGridlines(bool on)     { m_lv->setHorizontalGridlines(on); return *this; }
ListViewBuilder& ListViewBuilder::setHoverHighlight(bool on)          { m_lv->setHoverHighlight(on); return *this; }
ListViewBuilder& ListViewBuilder::setCycleNavigation(bool on)         { m_lv->setCycleNavigation(on); return *this; }
ListViewBuilder& ListViewBuilder::setMinColumnWidth(float px)         { m_lv->setMinColumnWidth(px); return *this; }
ListViewBuilder& ListViewBuilder::addColumn(const std::string& title, float width, bool sortable) {
    m_lv->addColumn(title, width, sortable); return *this;
}
ListViewBuilder& ListViewBuilder::addRow(const std::string& id, const std::vector<std::string>& cells) {
    m_lv->addRow(id, cells); return *this;
}
ListViewBuilder& ListViewBuilder::setOnSelectionChanged(ListView::OnSelectionChangedHandler h) { m_lv->setOnSelectionChanged(std::move(h)); return *this; }
ListViewBuilder& ListViewBuilder::setOnItemClick(ListView::OnItemClickHandler h)               { m_lv->setOnItemClick(std::move(h)); return *this; }
ListViewBuilder& ListViewBuilder::setOnColumnSort(ListView::OnColumnSortHandler h)                 { m_lv->setOnColumnSort(std::move(h)); return *this; }
std::shared_ptr<ListView> ListViewBuilder::build(void) {
    m_lv->create();
    return m_lv;
}

// ==================== P0-111/P0-113：就地编辑与列宽手势 ====================

ListView::~ListView() {
    m_editing = false;
    m_editDestroyPending = false;
    if (m_editBox) { removeControl(m_editBox); m_editBox.reset(); }
    m_apiResizeActive = false;   // 销毁：静默清理
    Cursor::restoreDefault(static_cast<Control*>(this));   // 仅 owner 恢复，避免误重置其它 ListView/Splitter
    delete m_resizeCursor;
    m_resizeCursor = nullptr;
}

void ListView::setVisible(bool visible) {
    if (!visible) {
        m_overDivider = false;
        setColumnResizeCursor(false);
        cancelEditForMutation();
        cancelColumnResizeInternal(m_apiResizeActive);   // 隐藏：取消并通知
        cleanupPendingEdit();
    }
    ControlImpl::setVisible(visible);
}

void ListView::setEnable(bool enable) {
    if (!enable) {
        m_overDivider = false;
        setColumnResizeCursor(false);
        cancelEditForMutation();
        cancelColumnResizeInternal(m_apiResizeActive);
        cleanupPendingEdit();
    }
    ControlImpl::setEnable(enable);
}

void ListView::cleanupPendingEdit() {
    if (!m_editDestroyPending || !m_editBox) return;
    m_editDestroyPending = false;
    auto box = m_editBox;
    m_editBox.reset();
    removeControl(box);
}

void ListView::fireCellEditEvent(const char* eventName, int row, int col,
                                 const std::string& oldText, const std::string& newText, bool readOnly) {
    GridPayload p{row, col, 0};
    p.oldText = oldText.c_str();
    p.newText = newText.empty() ? nullptr : newText.c_str();
    p.readOnly = readOnly ? 1 : 0;
    fireCCallback(eventName, CCallbackData::Grid, &p);
    OnCellEditHandler* typed = nullptr;
    if (eventName == PropertyNames::kEventCellEditBegin)       typed = &m_onCellEditBegin;
    else if (eventName == PropertyNames::kEventCellEdited)     typed = &m_onCellEditEdited;
    else if (eventName == PropertyNames::kEventCellEditCancel) typed = &m_onCellEditCancel;
    if (typed && *typed) {
        if (auto self = std::dynamic_pointer_cast<ListView>(
                std::dynamic_pointer_cast<Control>(getThis())))
            (*typed)(self, row, col, oldText, newText, readOnly);
    }
}

void ListView::finishEdit(bool commit, bool fromFocusLost) {
    if (!m_editing) return;
    m_editing = false;
    const std::string oldText = m_editOldText;
    std::string newText;
    const bool ro = m_editReadOnly;
    if (commit && !ro) {
        if (m_editBox) newText = m_editBox->getText();
        if (m_editRow >= 0 && m_editCol >= 0)
            setCell(m_editRow, m_editCol, newText);
        m_editNewText = newText;
    }
    if (m_editBox) {
        m_editBox->setEditActive(false);
        m_editBox->setVisible(false);
        m_editDestroyPending = true;   // 下一帧/下一事件清理，避免回调栈内自毁
    }
    if (!fromFocusLost && getContext())
        GET_FOCUSMANAGER->focusControl(this);
    if (commit && !ro)
        fireCellEditEvent(PropertyNames::kEventCellEdited, m_editRow, m_editCol, oldText, newText, false);
    else
        fireCellEditEvent(PropertyNames::kEventCellEditCancel, m_editRow, m_editCol, oldText, "", ro);
    m_editRow = m_editCol = -1;
    m_editOldText.clear();
    m_editNewText.clear();
    m_editReadOnly = false;
}

void ListView::cancelEditForMutation() {
    if (m_editing) finishEdit(false, false);
    cleanupPendingEdit();
}

bool ListView::canEditCell(int row, int col) const {
    if (!m_cellEditing || row < 0 || row >= getRowCount()) return false;
    const int colCount = (m_viewMode == Mode::Single) ? 1 : getColumnCount();
    if (col < 0 || col >= colCount) return false;
    if (!m_editableCols.empty() && m_editableCols.count(col) == 0) return false;
    return true;
}

bool ListView::isCellReadOnly(int row, int col) const {
    if (m_readOnlyCols.count(col)) return true;
    if (row >= 0 && row < getRowCount() && m_readOnlyRows.count(m_rows[row].id)) return true;
    return false;
}

SRect ListView::cellRectLocal(int row, int col) const {
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    const float x = columnX(col);
    const float w = (m_viewMode == Mode::Single)
                    ? m_rect.width
                    : (col >= 0 && col < getColumnCount() ? m_columns[col].width : 0.f);
    const float y = dataY + row * m_rowHeight - m_scrollOffsetV;
    return SRect(x, y, w, m_rowHeight);
}

bool ListView::cellFullyVisible(int row, int col) const {
    const float hSb = (m_scrollBarH && m_scrollBarH->getVisible()) ? kScrollbarW : 0.f;
    const float vSb = (m_scrollBarV && m_scrollBarV->getVisible()) ? kScrollbarW : 0.f;
    const float availW = m_rect.width - vSb;
    const float availH = m_rect.height - hSb;
    const float dataY = (m_viewMode == Mode::Multi) ? m_headerHeight : 0.f;
    const SRect r = cellRectLocal(row, col);
    return r.left >= 0.f && r.left + r.width <= availW + 0.5f &&
           r.top >= dataY - 0.5f && r.top + r.height <= availH + 0.5f;
}

bool ListView::beginEditCell(int row, int col) {
    if (!m_visible || !m_enable || !m_isCreated || m_apiResizeActive) return false;
    if (!canEditCell(row, col) || !cellFullyVisible(row, col)) return false;
    if (m_editing) finishEdit(false, false);
    cleanupPendingEdit();

    const SRect r = cellRectLocal(row, col);
    SRect editRect(r.left + 2.f, r.top + 1.f, max(0.f, r.width - 4.f), max(0.f, r.height - 2.f));
    m_editRow = row;
    m_editCol = col;
    m_editReadOnly = isCellReadOnly(row, col);
    m_editOldText = getCell(row, col);
    m_editNewText.clear();

    auto box = make_shared<InlineEditBox>(this, editRect);
    m_editBox = box;
    addControl(box);
    box->setContext(getContext());
    box->create();
    box->setFontSize(m_fontSize);
    box->setText(m_editOldText);
    box->setReadOnly(m_editReadOnly);
    box->setOnEnter([this](shared_ptr<Control>) { commitEdit(); });
    box->setCommitHandler([this](const std::string& text) {
        m_editNewText = text;
        finishEdit(true, true);
    });
    box->setCancelHandler([this]() { finishEdit(false, false); });   // Esc：恢复焦点
    box->setEditActive(true);
    m_editing = true;
    if (getContext()) GET_FOCUSMANAGER->focusControl(box.get());

    fireCellEditEvent(PropertyNames::kEventCellEditBegin, row, col, m_editOldText, "", m_editReadOnly);
    return true;
}

bool ListView::commitEdit() {
    if (!m_editing) return false;
    finishEdit(true, false);
    return true;
}

bool ListView::cancelEdit() {
    if (!m_editing) return false;
    finishEdit(false, false);
    return true;
}

bool ListView::getEditingCell(int& row, int& col, bool& readOnly) const {
    if (!m_editing) return false;
    row = m_editRow;
    col = m_editCol;
    readOnly = m_editReadOnly;
    return true;
}

// ── P0-113：列宽手势 ──
void ListView::applyColumnWidthInternal(int index, float width) {
    if (index < 0 || index >= getColumnCount()) return;
    const float clamped = max(m_minColumnWidth, width);
    m_columns[index].width = clamped;
    rebuildLayout();
}

int ListView::hitTestColumnDivider(float localX, float localY) const {
    if (!m_visible || !m_enable || !m_isCreated || m_viewMode != Mode::Multi) return -1;
    if (localY < 0.f || localY > m_headerHeight) return -1;
    const float vSb = (m_scrollBarV && m_scrollBarV->getVisible()) ? kScrollbarW : 0.f;
    const float contentRight = m_rect.width - vSb;
    if (localX < 0.f || localX >= contentRight) return -1;
    const int last = getColumnCount() - 1;
    for (int c = 0; c < getColumnCount(); ++c) {
        const float left = columnX(c);
        const float right = left + m_columns[c].width;
        float hitX = right;
        if (c == last) {
            if (!(left < contentRight && right > 0.f)) continue;   // 完全不可见
            if (right > contentRight - 1.f) hitX = contentRight - 1.f;   // 右缘被裁剪：可见边界
        } else {
            if (right <= 0.f || right >= contentRight) continue;
        }
        if (fabs(localX - hitX) <= kColResizeHitW) return c;
    }
    return -1;
}

bool ListView::setColumnResizeCursor(bool active) {
    if (!active) {
        // owner 仲裁：仅当前 EW owner 恢复默认；非 owner 的隐藏/失焦调用不覆盖其它控件光标
        Cursor::restoreDefault(getThis().get());
        m_overDivider = false;
        return true;
    }
    if (!m_visible || !m_enable || !m_isCreated) return false;
    if (!m_resizeCursor) m_resizeCursor = Cursor::createSystem(SystemCursorType::EW_Resize);
    if (!m_resizeCursor) return false;
    Cursor::setCurrentWithOwner(m_resizeCursor, getThis().get());
    return true;
}

bool ListView::beginColumnResize(int col, float localX) {
    if (!m_visible || !m_enable || !m_isCreated) return false;
    cancelEditForMutation();   // 列 resize 会话期间取消编辑（CR 生命周期矩阵）
    if (m_viewMode != Mode::Multi || m_apiResizeActive || m_dragCol >= 0) return false;
    if (col < 0 || col >= getColumnCount()) return false;
    m_apiResizeActive = true;
    m_apiResizeCol = col;
    m_apiResizeOldWidth = m_columns[col].width;
    m_apiResizeLastWidth = m_apiResizeOldWidth;
    m_apiResizeStartX = localX;
    ResizePayload rp{col, m_apiResizeOldWidth, m_apiResizeOldWidth, 0};
    fireCCallback(PropertyNames::kEventColumnResizeBegin, CCallbackData::Resize, &rp);
    return true;
}

float ListView::updateColumnResize(float localX) {
    if (!m_apiResizeActive) return -1.0f;
    if (!m_visible || !m_enable) { cancelColumnResizeInternal(true); return -1.0f; }
    const float proposed = m_apiResizeOldWidth + (localX - m_apiResizeStartX);
    applyColumnWidthInternal(m_apiResizeCol, proposed);
    const float cur = m_columns[m_apiResizeCol].width;
    if (fabs(cur - m_apiResizeLastWidth) > 0.01f) {
        m_apiResizeLastWidth = cur;
        ResizePayload rp{m_apiResizeCol, m_apiResizeOldWidth, cur, 0};
        fireCCallback(PropertyNames::kEventColumnResizeChanged, CCallbackData::Resize, &rp);
    }
    return cur;
}

void ListView::cancelColumnResizeInternal(bool fire) {
    if (!m_apiResizeActive) return;
    if (m_apiResizeCol >= 0 && m_apiResizeCol < getColumnCount())
        applyColumnWidthInternal(m_apiResizeCol, m_apiResizeOldWidth);
    const int col = m_apiResizeCol;
    const float oldW = m_apiResizeOldWidth;
    m_apiResizeActive = false;
    m_apiResizeCol = -1;
    if (fire) {
        ResizePayload rp{col, oldW, oldW, 1};
        fireCCallback(PropertyNames::kEventColumnResizeEnded, CCallbackData::Resize, &rp);
    }
}

bool ListView::endColumnResize(bool commit) {
    if (!m_apiResizeActive) return false;
    const int col = m_apiResizeCol;
    const float oldW = m_apiResizeOldWidth;
    if (!commit) applyColumnWidthInternal(col, oldW);
    const float cur = (col >= 0 && col < getColumnCount()) ? m_columns[col].width : oldW;
    m_apiResizeActive = false;
    m_apiResizeCol = -1;
    ResizePayload rp{col, oldW, commit ? cur : oldW, commit ? 0 : 1};
    fireCCallback(PropertyNames::kEventColumnResizeEnded, CCallbackData::Resize, &rp);
    return true;
}

bool ListView::cancelColumnResize() {
    if (!m_apiResizeActive) return false;
    cancelColumnResizeInternal(true);
    return true;
}

namespace {
void parseIndexSet(const std::string& s, std::set<int>& out) {
    out.clear();
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t comma = s.find(',', pos);
        std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        size_t b = tok.find_first_not_of(" \t");
        size_t e = tok.find_last_not_of(" \t");
        if (b != std::string::npos) {
            try { out.insert(std::stoi(tok.substr(b, e - b + 1))); } catch (...) {}
        }
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
}
void parseIdSet(const std::string& s, std::set<std::string>& out) {
    out.clear();
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t comma = s.find(',', pos);
        std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        size_t b = tok.find_first_not_of(" \t");
        size_t e = tok.find_last_not_of(" \t");
        if (b != std::string::npos) out.insert(tok.substr(b, e - b + 1));
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
}
}

// ==================== P0-114/CR004：通用单元格控件 ====================

bool ListView::cellControlFullyVisible(int row, int col) const {
    return cellFullyVisible(row, col);
}

bool ListView::isFocusWithin(Control* ctl) const {
    if (!ctl) return false;
    FocusManager* fm = getContext() ? getContext()->focusManager : nullptr;
    Control* cur = fm ? fm->getCurrentFocused() : nullptr;
    while (cur) {
        if (cur == ctl) return true;
        cur = cur->getParent();
    }
    return false;
}

void ListView::focusHandoffFromChild(Control* ctl) {
    if (ctl && isFocusWithin(ctl) && getContext() && getContext()->focusManager)
        getContext()->focusManager->focusControl(this);
}

void ListView::removeCellControlAt(int row, int col, bool destroy) {
    if (row < 0 || row >= getRowCount()) return;
    ListRow& r = m_rows[row];
    auto it = r.cellControls.find(col);
    if (it != r.cellControls.end() && it->second) {
        focusHandoffFromChild(it->second.get());
        if (getMouseCapturedChild() == it->second.get()) clearMouseCapture(true);
        removeControl(it->second);
        m_attachedChildren.erase(it->second.get());
        r.cellControls.erase(it);
    }
    r.cellControlLayouts.erase(col);
    (void)destroy;
}

void ListView::removeCellControlsForColumn(int col) {
    for (int i = 0; i < getRowCount(); ++i) removeCellControlAt(i, col, true);
}

bool ListView::setCellControl(int row, int col, shared_ptr<Control> ctl, CellControlLayout layout) {
    if (row < 0 || row >= getRowCount() || col < 0 || !ctl) return false;
    const int colCountEff = (m_viewMode == Mode::Single) ? 1 : getColumnCount();
    if (col >= colCountEff) return false;
    if (ctl->getParent() != nullptr) return false;   // 只接受无父控件
    if (getMouseCapturedChild() == ctl.get()) return false;
    focusHandoffFromChild(ctl.get());
    removeCellControlAt(row, col, true);
    m_rows[row].cellControls[col] = ctl;
    m_rows[row].cellControlLayouts[col] = (uint8_t)layout;
    syncChildControls();
    return true;
}

bool ListView::clearCellControl(int row, int col) {
    const int colCountEff = (m_viewMode == Mode::Single) ? 1 : getColumnCount();
    if (row < 0 || row >= getRowCount() || col < 0 || col >= colCountEff) return false;
    removeCellControlAt(row, col, true);
    syncChildControls();
    return true;
}

shared_ptr<Control> ListView::getCellControl(int row, int col) const {
    if (row < 0 || row >= getRowCount()) return nullptr;
    auto it = m_rows[row].cellControls.find(col);
    return it != m_rows[row].cellControls.end() ? it->second : nullptr;
}
