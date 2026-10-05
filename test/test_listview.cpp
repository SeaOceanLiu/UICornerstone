// ============================================================================
// test_listview.cpp -- ListView 列表控件测试
// 断言：数据模型（补足/列对齐反例/同步迁移）/ 选择钳制 / 排序跟随 / 四层属性回环 / CABI 抽查
// 可视化：multi 三列表（列头/排序/网格线/选中） + single 单列（ListBox 替代视觉）
// ============================================================================
#include <iostream>
#include <memory>
#include <cmath>
#include "ListView.h"
#include "Shape.h"
#include "Label.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "TestUtils.h"
#include "TestInstance.h"
#include "UICornerstoneAPI.h"

using namespace std;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { ++g_pass; TestUtil::log("OK   %s", msg); } \
    else      { ++g_fail; TestUtil::log("FAIL %s", msg); } \
} while (0)

static shared_ptr<ListView> g_probe;   // 数据模型断言探针（不挂树）

static void runAssertions() {
    TestUtil::log("---- ListView assertions ----");

    // ── 列定义 ──
    g_probe->addColumn(u8"名称", 160.f, true);
    g_probe->addColumn(u8"类型", 90.f);
    g_probe->addColumn(u8"大小", 70.f, true);
    CHECK(g_probe->getColumnCount() == 3, "addColumn x3");

    // P0-56：控件级表头样式（属性系统回环）
    {
        SColor hc;
        CHECK(g_probe->setColorProperty(PropertyNames::kHeaderText, SColor(10, 20, 30, 255)) == 1, "P0-56 set header-text");
        CHECK(g_probe->getColorProperty(PropertyNames::kHeaderText, hc) == 1 && hc.redByte() == 10, "P0-56 get header-text");
        CHECK(g_probe->setColorProperty(PropertyNames::kHeaderBackground, SColor(40, 50, 60, 255)) == 1, "P0-56 set header-background");
        CHECK(g_probe->getColorProperty(PropertyNames::kHeaderBackground, hc) == 1 && hc.greenByte() == 50, "P0-56 get header-background");
        CHECK(g_probe->setColorProperty(PropertyNames::kHeaderShadow, SColor(70, 80, 90, 255)) == 1, "P0-56 set header-shadow");
        CHECK(g_probe->getColorProperty(PropertyNames::kHeaderShadow, hc) == 1 && hc.blueByte() == 90, "P0-56 get header-shadow");
        CHECK(g_probe->setFloatProperty(PropertyNames::kHeaderShadowOffsetX, 3.0f) == 1, "P0-56 set shadow-offset-x");
        float hoff = 0.f;
        CHECK(g_probe->getFloatProperty(PropertyNames::kHeaderShadowOffsetY, hoff) == 1 && hoff == 1.0f, "P0-56 get shadow-offset-y default");
    }

    // P0-56：per-column 稀疏扩展
    g_probe->setColumnHeaderBackground(0, SColor(1, 2, 3, 255));
    g_probe->setColumnHeaderShadow(1, SColor(4, 5, 6, 255), 2.0f, 3.0f);
    {
        HeaderStyle hs0 = g_probe->getColumnHeaderStyle(0);
        HeaderStyle hs1 = g_probe->getColumnHeaderStyle(1);
        HeaderStyle hs2 = g_probe->getColumnHeaderStyle(2);
        CHECK(hs0.hasBackground && hs0.background.blueByte() == 3, "P0-56 column bg set");
        CHECK(hs1.hasShadow && hs1.shadowColor.greenByte() == 5 && hs1.shadowOffset.x == 2.0f && hs1.shadowOffset.y == 3.0f, "P0-56 column shadow set");
        CHECK(!hs2.hasBackground && !hs2.hasShadow, "P0-56 unset column stays sparse");
    }

    // ── 行补足语义（§5.0.3）：不足补空 ──
    g_probe->addRow("r1", {"A"});
    CHECK(g_probe->getRowCells(0).size() == 3 && g_probe->getCell(0, 1).empty(),
          "addRow pads cells to column count");

    // ── 反例核对：["C"] 落 A 列（位置映射），不是 C 列 ──
    g_probe->addRow("r2", {"C"});
    auto colA = g_probe->getColumnValues(0);
    CHECK(colA.size() == 2 && colA[1] == "C", "[\"C\"] lands in column A (position mapping)");
    auto colC = g_probe->getColumnValues(2);
    CHECK(colC.size() == 2 && colC[0].empty() && colC[1].empty(),
          "getColumnValues(C) empty for tail-omitted rows");

    // ── getColumnValues 恒 = 行数（对齐语义）──
    g_probe->setCell(1, 2, "512 B");
    CHECK(g_probe->getColumnValues(2).size() == 2 &&
          g_probe->getColumnValues(2)[1] == "512 B", "getColumnValues aligned to row count");

    // ── setCell 越界自动扩 ──
    g_probe->setCell(0, 5, "X");
    CHECK(g_probe->getCell(0, 4).empty() && g_probe->getCell(0, 5) == "X",
          "setCell beyond columns auto-expands row");

    // ── insertColumn 同步迁移 cellControls；removeColumn 还原 ──
    auto cb = make_shared<Label>(nullptr, SRect(0, 0, 40, 20));
    g_probe->setCellLeadingControl(0, 1, cb);
    CHECK(g_probe->getCellLeadingControl(0, 1) == cb, "setCellLeadingControl");
    g_probe->insertColumn(0, u8"插入", 60.f);
    CHECK(g_probe->getColumnCount() == 4 && g_probe->getRowCount() == 2,
          "insertColumn shifts all rows");
    CHECK(g_probe->getCellLeadingControl(0, 2) == cb,
          "cellControls migrate col1 -> col2 after insertColumn(0)");
    g_probe->removeColumn(0);
    CHECK(g_probe->getColumnCount() == 3 && g_probe->getCellLeadingControl(0, 1) == cb,
          "removeColumn restores & migrates back");

    // ── removeRow 后选中钳制 ──
    g_probe->setSelectedRow(1);
    CHECK(g_probe->getSelectedRow() == 1, "setSelectedRow");
    g_probe->removeRow(1);
    CHECK(g_probe->getSelectedRow() == -1, "selection clamped after removeRow");

    // ── 排序：字典序升序 + 稳定 + 选中按 id 跟随 ──
    g_probe->addRow("aaa", {"aaa", "x", "1"});
    g_probe->addRow("ccc", {"ccc", "y", "3"});
    const int n = g_probe->getRowCount();                 // r1, aaa, ccc → 排序后 aaa,r1,ccc
    g_probe->setSelectedRow(n - 1);                       // 选中 "ccc"
    g_probe->sortByColumn(0, true);
    bool sortedAsc = true;
    for (int i = 1; i < n; ++i)
        if (g_probe->getRowCells(i)[0] < g_probe->getRowCells(i - 1)[0]) sortedAsc = false;
    CHECK(sortedAsc, "sortByColumn lexicographic ascending");
    CHECK(g_probe->getRowCells(g_probe->getSelectedRow())[0] == "ccc",
          "selection follows row by id after sort");

    // ── 自定义排序回调（数值比较降序）──
    g_probe->setColumnSorter(2, [](const string& a, const string& b) {
        return atof(a.c_str()) < atof(b.c_str());
    });
    g_probe->sortByColumn(2, false);
    CHECK(atof(g_probe->getRowCells(0)[2].c_str()) >=
          atof(g_probe->getRowCells(n - 1)[2].c_str()), "custom numeric comparator descending");
    g_probe->clearColumnSorter(2);

    // ── 四层属性回环 ──
    CHECK(g_probe->setEnumProperty("mode", "single") == 1 &&
          g_probe->getMode() == ListView::Mode::Single, "SetEnum(mode)=single");
    g_probe->setEnumProperty("mode", "multi");
    g_probe->setBoolProperty("gridlines", 0);
    int bi = 1;
    CHECK(g_probe->getBoolProperty("gridlines", bi) == 1 && bi == 0, "gridlines roundtrip false");
    g_probe->setFloatProperty("row-height", 30.f);
    float f = 0.f;
    CHECK(g_probe->getFloatProperty("row-height", f) == 1 && f == 30.f, "row-height roundtrip 30");
    g_probe->setIntProperty("selected-index", 0);
    int ii = -2;
    CHECK(g_probe->getIntProperty("selected-index", ii) == 1 && ii == 0, "selected-index roundtrip 0");

    // cycle-navigation 属性回环（C ABI/Binding 属性系统通道）
    g_probe->setBoolProperty("cycle-navigation", 1);
    int cn = -1;
    CHECK(g_probe->getBoolProperty("cycle-navigation", cn) == 1 && cn == 1, "cycle-navigation roundtrip true");
    g_probe->setBoolProperty("cycle-navigation", 0);
    CHECK(g_probe->getBoolProperty("cycle-navigation", cn) == 1 && cn == 0, "cycle-navigation roundtrip false");

    // font/font-size 属性系统回环（int 通道，与 Label 等控件规范一致；C ABI GetInt 可读）
    g_probe->setIntProperty("font-size", 20);
    int fs = 0;
    CHECK(g_probe->getIntProperty("font-size", fs) == 1 && fs == 20, "font-size roundtrip 20 (int channel)");
    const char* fname = nullptr;
    g_probe->setEnumProperty("font", "harmonyos-sans-sc-regular");
    CHECK(g_probe->getEnumProperty("font", fname) == 1 && fname &&
          strcmp(fname, "harmonyos-sans-sc-regular") == 0, "font enum roundtrip");

    // ── 事件：selection-changed / item-click / column-sort（C ABI 回调链路）──
    static int gEvRow = -9, gEvCol = -9, gEvAsc = -9, gEvSeen = 0;
    static char gEvName[64] = "";
    auto onEvent = [](void*, const void* raw, void*) {
        const UIEventData* ev = static_cast<const UIEventData*>(raw);
        strncpy(gEvName, ev->eventName ? ev->eventName : "", sizeof(gEvName) - 1);
        gEvRow = ev->data.grid.row;
        gEvCol = ev->data.grid.col;
        gEvAsc = ev->data.grid.asc;
        ++gEvSeen;
    };
    using CbFn = void(*)(void*, const void*, void*);
    g_probe->setCallbackProperty(PropertyNames::kEventListSelectionChanged, CbFn(onEvent), nullptr);
    g_probe->setCallbackProperty(PropertyNames::kEventItemClick, CbFn(onEvent), nullptr);
    g_probe->setCallbackProperty(PropertyNames::kEventColumnSort, CbFn(onEvent), nullptr);
    g_probe->setCallbackProperty(PropertyNames::kEventAnimationEnded, CbFn(onEvent), nullptr);

    // selection-changed：setSelectedRow 触发，grid{row=选中行, col=选中数}
    gEvSeen = 0; gEvRow = gEvCol = -9;
    const int rowCount = g_probe->getRowCount();
    g_probe->setSelectedRow(rowCount - 1);
    CHECK(gEvSeen == 1 && strcmp(gEvName, PropertyNames::kEventListSelectionChanged) == 0,
          "selection-changed fired");
    CHECK(gEvRow == rowCount - 1 && gEvCol == 1, "selection payload row/count");

    // column-sort：列头（Multi 模式 headerHeight 内）点击可排序列 0 -> grid{row=col, col=asc}
    g_probe->setVisible(true);
    gEvSeen = 0;
    auto hdDown = make_shared<Event>(EventType::MouseDown);
    hdDown->mouseButton = { 20.f, 14.f, MouseButton::Left };
    g_probe->handleEvent(hdDown);
    CHECK(gEvSeen >= 1 && strcmp(gEvName, PropertyNames::kEventColumnSort) == 0,
          "column-sort fired on header click");
    CHECK(gEvRow == 0 && gEvCol == 1, "column-sort payload col/asc");

    // item-click：数据区第 0 行点击 -> grid{row, col}
    gEvSeen = 0;
    auto rowDown = make_shared<Event>(EventType::MouseDown);
    rowDown->mouseButton = { 20.f, 40.f, MouseButton::Left };
    g_probe->handleEvent(rowDown);
    CHECK(gEvSeen >= 1 && strcmp(gEvName, PropertyNames::kEventItemClick) == 0,
          "item-click fired on row click");
    CHECK(gEvRow == 0 && gEvCol == 0, "item-click payload row/col");


    // P0-62①：单元格文字色（hasTextColor 稀疏：仅设背景不变黑字）
    {
        CellStyle bgOnly;
        bgOnly.bgColor = SColor(10, 20, 30, 255);
        g_probe->setCellStyle(0, 0, bgOnly);
        CHECK(!g_probe->getCellStyle(0, 0).hasTextColor, "P0-62a cell style bg-only hasTextColor=false");
        g_probe->setCellTextColor(0, 0, SColor(200, 30, 30, 255));
        CellStyle cs = g_probe->getCellStyle(0, 0);
        CHECK(cs.hasTextColor && cs.textColor.redByte() == 200 && cs.bgColor.blueByte() == 30,
              "P0-62a cell text color set (hasTextColor + color kept)");
        g_probe->clearCellStyle(0, 0);
    }

    // P0-64②③：控件级 hover/selected 键 + 单元格显式 hover 背景
    {
        SColor hc;
        CHECK(g_probe->setColorProperty(PropertyNames::kTreeHover, SColor(11, 22, 33, 255)) == 1, "P0-64b set hover key");
        CHECK(g_probe->getColorProperty(PropertyNames::kTreeHover, hc) == 1 && hc.blueByte() == 33, "P0-64b get hover key");
        CHECK(g_probe->setColorProperty(PropertyNames::kTreeSelected, SColor(44, 55, 66, 255)) == 1, "P0-64b set selected key");
        CHECK(g_probe->getColorProperty(PropertyNames::kTreeSelected, hc) == 1 && hc.greenByte() == 55, "P0-64b get selected key");
        g_probe->setCellHoverBackgroundColor(0, 0, SColor(200, 100, 50, 255));
        CellStyle cs2 = g_probe->getCellStyle(0, 0);
        CHECK(cs2.hasHoverBg && cs2.hoverBgColor.redByte() == 200, "P0-64③ cell hover bg set");
        CHECK(!cs2.hasBg, "P0-65① hover-only cell style hasBg=false (常态不填)");
        g_probe->clearCellStyle(0, 0);
    }

    // P0-63②③：字体名 API + hasFontName 稀疏（既有 SetCellStyle 不隐式覆盖字体）
    {
        CellStyle bg2;
        bg2.bgColor = SColor(9, 9, 9, 255);
        g_probe->setCellStyle(0, 0, bg2);
        CHECK(!g_probe->getCellStyle(0, 0).hasFontName, "P0-63b cell style bg-only hasFontName=false");
        g_probe->setCellFontName(0, 0, FontName::MapleMono_NF_CN_Regular);
        CHECK(g_probe->getCellStyle(0, 0).hasFontName
              && g_probe->getCellStyle(0, 0).fontName == FontName::MapleMono_NF_CN_Regular,
              "P0-63b cell font name set");
        g_probe->setColumnHeaderFontName(1, FontName::Quando_Regular);
        CHECK(g_probe->getColumnHeaderStyle(1).hasFontName
              && g_probe->getColumnHeaderStyle(1).fontName == FontName::Quando_Regular,
              "P0-63b column header font name set");
        g_probe->clearCellStyle(0, 0);
    }

    TestUtil::log("---- assertions done: pass=%d fail=%d ----", g_pass, g_fail);
}

static void addCaption(Bench* bench, int x, int y, const char* txt) {
    auto lbl = LabelBuilder(nullptr, SRect(x, y, 320, 22)).setCaption(txt).build();
    lbl->create();
    bench->addControl(lbl);
}

// ── C ABI 抽查（g_uiInstance 有效期内的可视化阶段执行）──
static void runCabiChecks() {
    TestUtil::log("---- ListView CABI checks ----");
    UIControlHandle h = UICornerstone_CreateListView(
        g_uiInstance, 700.f, 60.f, 300.f, 150.f, 1.f, 1.f);
    CHECK(h != nullptr, "CreateListView");

    const char* cols[] = {u8"名称", u8"大小"};
    CHECK(UICornerstone_ListViewAddColumn(g_uiInstance, h, cols[0], 160.f, 1) == 1 &&
          UICornerstone_ListViewAddColumn(g_uiInstance, h, cols[1], 90.f, 0) == 1,
          "ListViewAddColumn x2");

    const char* cells0[] = {"main.cpp", "2.1 KB"};
    const char* cells1[] = {"build.bat", "512 B"};
    CHECK(UICornerstone_ListViewAddRow(g_uiInstance, h, "f1", 2, cells0) == 1 &&
          UICornerstone_ListViewAddRow(g_uiInstance, h, "f2", 2, cells1) == 1,
          "ListViewAddRow x2");

    char buf[128] = "";
    CHECK(UICornerstone_ListViewGetCellText(g_uiInstance, h, 0, 0, buf, sizeof(buf)) == 1 &&
          strcmp(buf, "main.cpp") == 0, "ListViewGetCellText roundtrip");

    CHECK(UICornerstone_ListViewSetCellText(g_uiInstance, h, 1, 1, "1 KB") == 1 &&
          UICornerstone_ListViewGetCellText(g_uiInstance, h, 1, 1, buf, sizeof(buf)) == 1 &&
          strcmp(buf, "1 KB") == 0, "ListViewSetCellText roundtrip");

    CHECK(UICornerstone_ListViewSetColumnWidth(g_uiInstance, h, 0, 200.f) == 1,
          "ListViewSetColumnWidth");

    // 非 list-view 句柄拒绝
    CHECK(UICornerstone_ListViewAddRow(g_uiInstance, nullptr, "x", 0, nullptr) == 0,
          "null handle rejected");

    TestUtil::log("---- CABI checks done: pass=%d fail=%d ----", g_pass, g_fail);
}

// ── 可视化矩阵 ──
static void testListViewVisualize(Bench* bench) {
    g_probe = make_shared<ListView>(nullptr, SRect(0, 0, 100, 100));   // 断言探针不挂树
    runAssertions();

    addCaption(bench, 30, 30, "multi (Report): header/sort/gridlines/selection");
    {
        auto lv = make_shared<ListView>(nullptr, SRect(30, 60, 420, 240));
        lv->addColumn(u8"名称", 180.f, true);
        lv->addColumn(u8"类型", 120.f);
        lv->addColumn(u8"大小", 90.f, true);
        lv->addRow("f1", {"main.cpp", u8"C++ 源文件", "2.1 KB"});
        lv->addRow("f2", {"build.bat", u8"批处理", "512 B"});
        lv->addRow("f3", {"logo.svg", u8"矢量图", "8 KB"});
        lv->addRow("f4", {"readme.md", u8"文档", "3 KB"});
        lv->addRow("f5", {"app.exe", u8"可执行", "96 KB"});
        // leadingControl 视觉：列头图标 + 行首图标（几何 Shape，attach 机制自动挂树定位）
        {
            auto colIcon = make_shared<Shape>(nullptr, SRect(0, 0, 12, 12));
            colIcon->setShape(ShapeType::Circle);
            colIcon->setFillColor(SColor(96, 165, 250));
            lv->setColumnLeadingControl(0, colIcon);

            auto rowIcon0 = make_shared<Shape>(nullptr, SRect(0, 0, 12, 12));
            rowIcon0->setShape(ShapeType::FilledRect);
            rowIcon0->setFillColor(SColor(74, 222, 128));
            lv->setRowLeadingControl(0, rowIcon0);

            auto rowIcon1 = make_shared<Shape>(nullptr, SRect(0, 0, 12, 12));
            rowIcon1->setShape(ShapeType::Circle);
            rowIcon1->setFillColor(SColor(251, 146, 60));
            lv->setRowLeadingControl(1, rowIcon1);
        }
        lv->setSelectedRow(0);
        lv->create();
        bench->addControl(lv);
    }

    addCaption(bench, 480, 30, "single (= ListBox)");
    {
        auto lv = make_shared<ListView>(nullptr, SRect(480, 60, 220, 200));
        lv->setMode(ListView::Mode::Single);
        lv->addItem(u8"选项 一", u8"选项 一");
        lv->addItem(u8"选项 二", u8"选项 二");
        lv->addItem(u8"选项 三", u8"选项 三");
        lv->addItem(u8"选项 四", u8"选项 四");
        lv->setSelectedRow(1);
        lv->create();
        bench->addControl(lv);
        // DEBUG: 验证选中状态
        TestUtil::log("DEBUG single: selected=%d rows=%d mode_multi=%s",
            lv->getSelectedRow(), lv->getRowCount(),
            lv->getMode()==ListView::Mode::Multi ? "yes" : "no");
    }

    // 缩放可视化：2.0x 列表（ListViewBuilder 路径；getDrawRect = rect × scale）
    // 独立行绘制：避开 (700,60) 的 CABI 1x 实例
    addCaption(bench, 760, 430, "scale 2.0x (builder)");
    {
        auto lv = ListViewBuilder(nullptr, SRect(760, 460, 280, 180), 2.0f, 2.0f)
                      .addColumn(u8"名称", 160.f, true)
                      .addColumn(u8"大小", 90.f)
                      .addRow("s1", {"scaled.cpp", "12 KB"})
                      .addRow("s2", {"note.md", "3 KB"})
                      .setSelectedRow(0)
                      .build();
        bench->addControl(lv);
        CHECK(fabs(lv->getDrawRect().width - 560.f) < 0.01f, "scaled listview drawRect = rect*2.0");
        CHECK(fabs(lv->getDrawRect().height - 360.f) < 0.01f, "scaled listview drawRect.height = h*2.0");
    }

    runCabiChecks();
}

class ListViewApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_listview");
        BENCH->setOnInitial([](shared_ptr<Bench> b) { testListViewVisualize(b.get()); });
        return true;
    }
    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }
    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(40, 40, 44));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
        // 视觉验证：帧内读回（Render 后、Present 前）
        if (++m_frames == 30 && !m_saved) {
            static uint8_t pixels[1400 * 900 * 4];
            int w = 0, h = 0;
            const int cap = UICornerstone_CaptureViewport(g_uiInstance, pixels, &w, &h);
            const std::string out = "Temp/listview_capture.bmp";
            const int saved = cap ? UICornerstone_SavePixelsToFile(pixels, w, h, out.c_str()) : 0;
            TestUtil::log("capture: cap=%d saved=%d (%dx%d) -> %s", cap, saved, w, h,
                          saved ? out.c_str() : "FAILED");
            m_saved = true;
        }
    }
    void onQuit() override { TestUtil::log("ListView test quit"); }

private:
    int m_frames = 0;
    bool m_saved = false;
};

int main(int argc, char* argv[]) {
    return TestRunMain<ListViewApp>(argc, argv);
}
