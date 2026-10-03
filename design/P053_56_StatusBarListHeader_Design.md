# P053_56_StatusBarListHeader_Design — StatusBar 重排/字体路径 + 分段着色 + ListView 表头样式

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-02 追加：P0-53 / P0-54 / P0-55 / P0-56）
> 前置：P0-52 批已实施并同步（设计器已接线 RenderOverlays，本批闭环）
> 状态：**已放行，已实施**（2026-10-02）
> 复核：`requirements/P053_56_StatusBarListHeader_Design_复核意见.md`（通过，放行实施；五问全确认 + 3 项实施注意已落实）
> 实施备注：① P0-53 文本变更 relayout（hitRect 宽度实测增长）；② P0-54 六处调用点统一相对路径 + `MemoryResourceProvider` 绝对前缀剥离回退（readFile/exists，单测 PASS）；③ P0-55 StatusItem 四态 mask + 背景（回退链=显式态→段 normal→控件级，未设置继承）+ ABI/Binding/JSON（字符串/四态对象）+ schema（$ref color）；④ P0-56 ListView 控件级 5 键 + per-column 稀疏扩展（hasTextColor 修复既有 per-column 文字色被忽略问题）+ ABI/Binding/JSON/schema；⑤ 测试：test_statusbar 13/0、test_listview 40/0、像素探针 11/11（段背景/表头 per-column 背景精确色值）、样例 `layouts/p0_statusbar_list.json`（ctest strict）；⑥ 回归仅 4 项预存失败；⑦ 文档：CABI/Binding 速查表 + statusbar/listview 键表 + binding 样例

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-53 | updateStatusItemText 不触发 relayout，段宽不随文本变化 | **确认** | `StatusBar::updateStatusItemText`（StatusBar.cpp:46-50）仅 `item.text = text; return;`；`addStatusItem`/`removeStatusItem`/`setStatusItemSize` 均 `relayout()`（:44/:54/:80-83），唯独文本更新遗漏 → hitRect 宽度仍按旧文本，长文本压邻段（字体首次就绪的 draw 内自愈重排偶发修正） |
| P0-54 | 字体路径两套约定：内存 provider 下部分控件无文字 | **确认** | 相对约定：Label（`provider->readFile(m_fontFile.string())`，值= `fonts/…`）、Actor（Actor.cpp:142）✓。绝对约定：StatusBar.cpp:93 / Menu.cpp:63 / TabControl.cpp:50 / TreeView.cpp:95,117 / ListView.cpp:53,88 —— `ConstDef::pathPrefix.string() + "/" + rel`（`GetBasePath()+"assets" + "/fonts/…"`）。`MemoryResourceProvider::readFile`（ResourceProvider.cpp:117-141）仅 `provider:` 前缀剥离后**精确键匹配**（含 `exists`）→ 绝对路径在按相对键注册的子视口 provider 下读不到 → 状态栏/列表列头/菜单/页签/树节点无文字 |
| P0-55 | StatusBar 分段着色缺失 | **确认** | `StatusItem`（StatusBar.h:22-30）无颜色字段；draw 文本恒用控件级四态色（`resolveStateColor(m_textColor, getState())`，StatusBar.cpp:188），无段背景绘制；仅增量 API `AddItem/SetItemText/SetItemMenu/SetItemIcon`（ABI:593-601），无颜色入口 |
| P0-56 | ListView 表头背景/文字阴影/控件级表头文字色缺失 | **确认** | 内部常量：`m_headerBgColor{45,45,52}`、`m_headerTextColor{200,200,205}`（ListView.h:252/256；背景填充 ListView.cpp:568）——无属性/API/schema 键；`HeaderStyle`（ListView.h:30-34）仅 textColor/fontName/fontSize，无背景/阴影字段；`ListViewSetCellShadow` 仅单元格（ABI:581）；控件级表头文字色无入口（仅 per-column `ListViewSetColumnHeaderStyle`，ABI:583）；schema list-view def 有 `header-height`，无 `header-text/background/shadow`；columns[] items 仅 {icon,title,width,sortable} |

## 2. 修改方案

### 2.1 P0-53 文本更新触发重排（单行修复）
- `updateStatusItemText`：命中段设置文本后，**文本实际变化时** `relayout()`（与 add/remove 语义对齐；同文本写入不重排）。
- 设计器过渡代码（临时增删段）同步后可删。

### 2.2 P0-54 字体路径约定统一（主修 + 防御回退）
- **主修（统一为相对约定）**：6 处调用点改为直接读相对键（`it->second` / `fit->second`，与 Label/Actor 一致）：
  StatusBar.cpp:93、Menu.cpp:63、TabControl.cpp:50、TreeView.cpp:95,117、ListView.cpp:53,88。
  文件系统 provider（`basePath / path`）与内存 provider（相对键精确匹配）均正常。
- **防御回退（可选但推荐，成本极低）**：`MemoryResourceProvider::readFile`/`exists` 精确匹配失败后，若路径以 `ConstDef::pathPrefix` 开头 → 剥离前缀与紧随分隔符（`/` 或 `\`）后重试相对键（覆盖用户/第三方传入绝对路径的场景；不改变既有精确命中优先级）。
- 设计器双键注册过渡随同步删除。

### 2.3 P0-55 StatusBar 分段着色
- `StatusItem` 扩展（稀疏语义，未设置=继承/不绘制）：
  ```cpp
  StateColor textColor;   bool hasTextColor = false;   // 四态（至少 normal）
  SColor     background;  bool hasBackground = false;  // 段背景（单色，VSCode 风格）
  ```
- 引擎：`setStatusItemTextColor(id, SColor, ControlState)` / `setStatusItemBackgroundColor(id, SColor)`；
- 绘制（StatusBar::draw）：命中段先填背景（`hasBackground` 时，按 hitRect×scale），文本色 = `hasTextColor ? textColor[state] : 控件级 resolveStateColor`；
- C ABI（与既有状态栏段操作并列）：
  ```c
  int UICornerstone_StatusBarSetItemTextColor(UIInstance, UICornerstone bar, const char* id,
      UIColor color, const char* state);   // state 可 NULL=normal；"normal"/"hover"/"pressed"/"disabled"
  int UICornerstone_StatusBarSetItemBackgroundColor(UIInstance, UIControlHandle bar,
      const char* id, UIColor color);
  ```
- Binding：`StatusBarSetItemTextColor(bar, id, UIColor, const char* state = nullptr)` / `StatusBarSetItemBackgroundColor(bar, id, UIColor)`；
- JSON（status-bar items 元素宽松校验）：新增 `text-color`（字符串色 → normal；对象 `{normal,hover,…}` → 四态）与 `background-color`（字符串色）；`parseStatusBar` 解析；schema items description 同步。
- PropertyNames：新增 `kItemTextColor = "text-color"` / `kItemBackgroundColor = "background-color"`。

### 2.4 P0-56 ListView 表头样式
- **控件级键（新增，属性系统 + JSON）**：
  | 键 | 常量 | 类型 | 说明 |
  |---|---|---|---|
  | `header-text` | `kHeaderText` | Color | 表头文字色（缺省沿用内部常量 200,200,205） |
  | `header-background` | `kHeaderBackground` | Color | 表头背景色（缺省沿用 45,45,52） |
  | `header-shadow` | `kHeaderShadow` | Color | 表头文字阴影色（缺省不绘制） |
  | `header-shadow-offset-x/y` | `kHeaderShadowOffsetX/Y` | Float | 阴影偏移（缺省 1,1） |
- `HeaderStyle`（per-column 稀疏）扩展：`background/hasBackground`、`shadowColor/hasShadow/shadowOffset`；
- C ABI（与 `ListViewSetCellShadow` 同风格）：
  ```c
  int UICornerstone_ListViewSetColumnHeaderBackground(inst, lv, col, r,g,b,a);
  int UICornerstone_ListViewSetColumnHeaderShadow(inst, lv, col, r,g,b,a, float ox, float oy);
  ```
- Binding：`ListViewSetColumnHeaderBackground(lv, col, UIColor)` / `ListViewSetColumnHeaderShadow(lv, col, UIColor, ox, oy)`；
- 绘制：表头背景 = per-column 优先（`hasBackground`）→ 控件级 `m_headerBgColor`；文字色 = per-column 优先（既有）→ 控件级 `m_headerTextColor`；阴影 = per-column（`hasShadow`）→ 控件级 `header-shadow`（未设不绘制），经 `TextDraw::withShadow`；
- schema：list-view def 增 4+1 键；columns[] items 增 per-column 键（`header-text`/`header-background`/`header-shadow`/`header-shadow-offset-x`/`header-shadow-offset-y`，与控件级同名、作用域=该列）；`parseListView` columns 循环解析。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | P0-53：updateStatusItemText 长文本后段宽/hitRect 更新（不压邻段） | C ABI 测试（GetRect 或像素）+ getter 校验 |
| 2 | P0-54：内存 provider（相对键）下状态栏/列表列头/菜单/页签/树 文字正常；绝对路径回退可读 | 子视口字体探针（像素）+ 单测 |
| 3 | P0-55：per-segment 文本四态色 + 背景色（API/JSON/Binding）| C ABI 测试 + 像素 |
| 4 | P0-56：控件级/ per-column 表头背景/文字/阴影生效（API/JSON/Binding） | C ABI 测试 + 像素 |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **P0-54 修复范围**：主修（6 处统一相对路径）+ 防御回退（MemoryResourceProvider 前缀剥离）**两者都做**（推荐）？或仅主修？
2. **P0-55 text-color JSON 形态**：字符串色（normal）与对象（四态）双形态接受——确认？背景色仅单色（无态）——确认？
3. **P0-55 段背景圆角/边距**：段背景铺满 hitRect（含内边距）还是内缩？建议铺满（VSCode 风格）——确认。
4. **P0-56 控件级 `header-text` 单色**（与内部语义一致，不做四态）——确认？per-column 键与控件级**同名同义**（作用域=列）——确认？
5. **P0-56 阴影形态**：色 + 偏移两键（对齐 `SetCellShadow`/`text-shadow` 家族；不做 enabled 开关，未设色=不绘制）——确认？

## 5. 配套改动

- 代码：`StatusBar.cpp/h`（relayout + 段色）、`ListView.cpp/h`（表头样式）、`ResourceProvider.cpp`（回退）、6 处路径统一、`LayoutParser.cpp`（status-bar items / list-view columns+控件级键）、`PropertyNames.h`、`UICornerstoneAPI.h/cpp`、`binding/*`；
- schema：list-view（4+1 键 + columns items 扩展）、status-bar items description；
- 测试：C ABI 用例（重排/段色/表头样式/内存 provider 字体）+ 像素探针；文档（capi/binding 速查表 + statusbar/listview 控件页 + 声明式语法键表）；设计文档标注 + 复核归档 + make_release + 同步。
