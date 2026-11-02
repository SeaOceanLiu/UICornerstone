# CR20261101001 — ListView 单元格就地编辑（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261101001，原 P0-111）
> 提出方：CornerstoneDesigner
> 状态：**已闭环（closed）**（2026-11-02；设计器联测通过，CR「设计器/引擎」均为 closed）
> **兼容政策（2026-11-01 用户确认）**：引擎 DLL、Binding、宿主必须**同批配套**；不支持旧 DLL/旧宿主混用（与 P108 变体 A 一致）。以下“旧 DLL”条目均改为“版本不匹配 = 功能不可用”，不承诺功能回退。设计器 CR 原文中的旧 DLL 回退条款由**设计器同步修订**（引擎不代改）。
> 参考：`requirements/引擎ListView_TreeView编辑模式_设计建议.md`

---

## 1. 需求摘要

ListView 打开 `cell-editing` 后，双击可编辑单元格 → 在单元格内容区上叠加内部 EditBox（预填当前文本）；**只读行/列仍可进入编辑，EditBox 以只读态展示（可选中/复制）**；Enter/失焦提交、Esc 取消；滚动/列宽变化/行增删移/排序/清空/刷新/隐藏/禁用/销毁时取消并清理；事件 `cell-edit-begin` / `cell-edited` / `cell-edit-cancel`；提供 C ABI + Binding 的程序化 Begin/Commit/Cancel/IsEditing；JSON/schema/用户手册同步。

## 2. 现状与源码核实

- `ListView` 数据：`ListColumn/ListRow`，`getCell/setCell`（`include/ListView.h:130-134`）；行命中 `hitTestRow`（`include/ListView.h:110`）；列命中 `hitTestColumn`（`include/ListView.h:227`）；列左缘 `columnX`（`include/ListView.h:228`）。
- 现有"单元格控件"机制：`cellControls` + `syncChildControls()`（`include/ListView.h:135-140,226`），随滚动/布局定位；**不包含编辑用 EditBox**。
- 现有事件：`item-click`（`GridPayload{row,col}`，`src/ListView.cpp:915`）、`selection-changed`/`column-sort`；无双击事件/无编辑状态。
- 运行时列宽拖拽已存在：`m_dragCol/m_dragStartWidth` + `setColumnWidth`（`src/ListView.cpp:880-886` 附近；`min-column-width` 钳制）。
- EditBox：`setOnEnter`、`onFocusLost()`（`src/EditBox.cpp:704`）、IME/剪贴板/密码/只读能力齐备；**不处理 Esc**。
- 焦点：`FocusManager` 单焦点；点击其他控件会造成 EditBox 失焦（可用于"失焦提交"）。
- C ABI 风暴：`UICornerstone_ListViewAddRow/SetCellText/...`（`include/UICornerstoneAPI.h:588-620`）；事件结构 `UIEventData.data.grid{row,col,asc}`（`:895`）。
- Binding：`ui->ListViewAddRow/...`（`binding/include/UICornerstone.h:268+`）；Event 访问器现状见 `binding/include/Event.h`。

## 3. 分析（可行性 / 风险）

1. 覆盖层用 **ListView 子控件 EditBox** 实现（而非宿主叠层）：按 B2 仅完整可见单元格进入，**不依赖子控件裁剪**；需要新的"内容区定位"而非 `cellControls` 的图标槽定位。
2. 双击识别：引擎内自做（450ms / 4px，与设计器画布一致），在现有 MouseUp（`item-click` 触发点）之后判定；不新增公共双击事件（本 CR 仅用于编辑开启）。
3. 生命周期取消点较多（滚动/列宽/行变更/排序/刷新/隐藏/禁用/销毁），必须集中到 `cancelEditInternal()` 单一出口，避免残留/悬垂焦点。
4. 事件前后值：设计器要求 begin/cancel 仅 oldText、edited 带 oldText+newText。C ABI 目前 `UIEventData.data.grid` 无字符串；按评审 B1 扩展现有 union 成员（见 §4.5，**结构体尺寸不变**）。
5. 只读模型（按 2026-11-01 评审反馈修订）：仍按 `editable-columns` 决定**能否进入编辑**；新增稀疏只读声明 `read-only-columns`（列索引）与 `read-only-row-ids`（行 id）——命中的单元格**照常进入编辑**，但覆盖 EditBox `setReadOnly(true)`，不写回数据（见 Q3）。
6. 与现有交互冲突：双击前的单击会触发行选择/`item-click`（保持既有语义，Q4）；拖拽/列头分隔线优先于双击。
7. 版本配套：新 Binding 调新入口，`Dyn::API` 缺失视为版本不匹配（安全失败，不做功能回退）。

## 4. 设计

### 4.1 属性 / JSON / Schema

| 属性键 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `cell-editing` | Bool | false | 开启单元格双击编辑；动态置 0 立即取消并清理 |
| `editable-columns` | String | "" | 逗号分隔列索引；空=全部列；非法项忽略；single 视图仅列 0 生效（只决定**能否进入**编辑） |
| `read-only-columns` | String | "" | 逗号分隔列索引；命中的列仍可进入编辑，但 EditBox 只读、不写回 |
| `read-only-row-ids` | String | "" | 逗号分隔行 id；命中的行仍可进入编辑，但 EditBox 只读、不写回 |

- `PropertyNames`：`kCellEditing`、`kEditableColumns`、`kReadOnlyColumns`、`kReadOnlyRowIds`；
- `LayoutParser::parseListView` 增四键解析（`cell-editing`/`editable-columns`/`read-only-columns`/`read-only-row-ids`）；`schema $defs/list-view` 同步四键（`additionalProperties:false`）；
- 只读属性：`edit-old-text` / `edit-new-text`（String；begin/cancel 时 new 为空；回调内可读，会话结束后清空），`PropertyNames` 增 `kEditOldText/kEditNewText`（若评审认为可省略只读属性则删除此项，事件载荷仍保留）。

### 4.2 交互与生命周期

- 进入：`cell-editing=1` 且双击命中的单元格列在 `editable-columns` 内 → 创建/复用内部 EditBox（只读行/列不阻止进入）：
  - 覆盖单元格内容矩形：`columnX(col)+cellPaddingX`，行 `top = headerHeight + (row - firstVisibleRow)*rowHeight - scrollOffsetV`，宽=列宽-2*padding，高=rowHeight；
  - **可见性策略（评审 B2 定稿）**：仅**完全可见**的单元格可进入编辑（行完整在内容区、列完整在可视宽内；部分可见/完全不可见双击不进入）。覆盖层因此不会越出表头/滚动条/控件边界，无需额外子控件裁剪；
  - 文本=`getCell(row,col)`；字体/字号/文字色继承现有单元格/行绘制口径；`selectAll()`；焦点移至 EditBox；
  - 只读判定：列 ∈ `read-only-columns` 或行 id ∈ `read-only-row-ids` → `EditBox::setReadOnly(true)`（仍可聚焦/全选/复制，不可改文本）；
  - 事件顺序：状态就绪并聚焦 → 记录 oldText（稳定成员）→ 发 `cell-edit-begin`。
- 提交：Enter 或失焦 → **只读会话统一按取消处理**（不写回，发 `cell-edit-cancel`，old 仅 + `readOnly=1`）；可写会话 `setCell(row,col,newText)`（空串合法）→ 销毁覆盖层 → 发 `cell-edited`（old+new，`readOnly=0`）；
- 取消：Esc、**只读会话 Enter/失焦**、生命周期取消（见下）→ 不写数据 → 销毁 → 发 `cell-edit-cancel`（仅 old；`readOnly` 随会话）；
- **取消并销毁**：滚动（滚轮/滚动条/程序化）、列宽变化、列增删、行增删移、排序、清空、刷新、控件隐藏/禁用、`cell-editing` 置 0 → 发 `cell-edit-cancel`；
- **控件销毁**：只清理会话与覆盖层，**不回调**（避免把失效句柄交给宿主）；
- **焦点（评审补充）**：键盘 Enter/Esc 结束后焦点还给 ListView（或进入编辑前的焦点控件）；点击其他控件导致的失焦提交不抢回焦点；销毁时 `FocusManager` 不留下悬垂指针；
- **Tab（评审补充）**：`Bench` 在子控件前拦截 Tab 做焦点遍历；本设计将 Tab 定义为"失焦提交 + 焦点切换"（不尝试在 EditBox 内消费全局 Tab），与 Q6 定稿一致；
- 双击与单击/拖拽：第一击按现有语义选中并触发 `item-click`；第二击（阈值内）在列可编辑时进入编辑；**双击仅命中单元格内容区（文本区）**——列头/表头分隔线、滚动条、行 leadingControl、拖拽阈值命中均优先，不进入编辑；第二击不重复触发行拖动（Q4）；
- 失焦提交需区分"点击覆盖层内部"（EditBox 自身）与真正失焦；EditBox 子类 `onFocusLost` 上报宿主；Esc 由子类拦截处理（EditBox 原生不处理 Esc）。

### 4.3 C ABI（命名沿用既有 `ListView<Action>` 风格）

```c
int UICornerstone_ListViewBeginEditCell(UIInstance, UIControlHandle lv, int row, int col);
int UICornerstone_ListViewCommitEdit(UIInstance, UIControlHandle lv);
int UICornerstone_ListViewCancelEdit(UIInstance, UIControlHandle lv);
int UICornerstone_ListViewIsEditing(UIInstance, UIControlHandle lv);           /* 1/0 */
int UICornerstone_ListViewGetEditingCell(UIInstance, UIControlHandle lv,
                                         int* outRow, int* outCol,
                                         int* outReadOnly);                    /* 1=编辑中；outReadOnly 可空 */
```

失败语义：未开启（`cell-editing=0`）/隐藏/禁用/未 create/越界/列不可编辑/行不存在 → Begin=0；Commit/Cancel=0=无会话；GetEditingCell 输出仅在返回 1 时有效。

### 4.4 Binding

- `Control` 或 `UICornerstone` 侧封装：`ListViewBeginEditCell/CommitEdit/CancelEdit/IsEditing/GetEditingCell`（随既有风格，`Control& lv` 参数）；
- `Event` 访问器：`IsCellEditBegin/IsCellEdited/IsCellEditCancel`、`GetEditRow/GetEditCol/GetEditOldText/GetEditNewText`、`IsEditReadOnly`；
- 动态库缺少新入口 = **版本不匹配**（禁止混用）；Binding 判空防御性返回失败，设计器提示/禁用功能，不做功能回退。

### 4.5 事件与载荷（union 扩展，**UIEventData 尺寸不变**）

- 事件名/常量：`cell-edit-begin` / `cell-edited` / `cell-edit-cancel`（`kEventCellEditBegin/kEventCellEdited/kEventCellEditCancel`）。
- 语义：`begin` 开启会话；`edited` 仅**可写会话**提交（数据已写回）；`cancel` = Esc / **只读会话 Enter/失焦** / 生命周期取消（未写回）。
- 内部：扩展 `GridPayload` 为 `{int row,col,asc; const char* oldText; const char* newText; int readOnly;}`（零初始化；非编辑事件为 nullptr/0；文本存会话成员，生命周期覆盖回调期）。
- 公共 C ABI（评审 B1 定稿，**不增加顶层字段**）：扩展 union 的 `grid` 成员（保持结构体尺寸不变）：
  ```c
  struct {
      int row;
      int col;
      int asc;
      const char* oldText;   /* begin/cancel/edited 有 */
      const char* newText;   /* 仅 edited 有 */
      int editReadOnly;      /* begin/cancel 的只读标志；edited 恒 0 */
  } grid;
  ```
  `fireCCallback(CallbackData::Grid)` 同步把内部 `GridPayload` 拷入该成员；`UIEventData` 尺寸由既有 `drag` 成员保证不变（实现加 `static_assert`/测试校验）。
- 只读属性双通道：保留 `GetString("edit-old-text")` / `GetString("edit-new-text")`；begin/cancel 的 new 为空，会话结束后清空；与事件载荷同源。
- 版本：**必须配套发布**（用户 2026-11-01 确认）；不依赖旧 DLL 兼容；union 扩展保持尺寸不变（实测 `sizeof(UIEventData)=80`，扩展后仍 80）为附加收益。

### 4.6 声明式事件接线

- 新增 `kEventKeyCellEditBegin/onCellEdited/onCellEditCancel`（`onCellEditBegin`/`onCellEdited`/`onCellEditCancel`）；
- `LayoutParser::parseEvents` ListView 分支接线到 `setOnCellEditBegin/Edited/Cancel`（签名 `void(shared_ptr<ListView>, int row, int col, const string& oldText, const string& newText, bool readOnly)`，begin/cancel 的 new 为空串）；
- 文档：声明式事件键表 + ListView 页。

## 5. Q1~Q8 引擎答复（建议，待评审确认）

| # | 问题 | 引擎建议 |
|---|---|---|
| Q1 | 进入编辑光标/全选 | **全选**（快速替换，双击语义一致） |
| Q2 | 滚动/列宽/行变更提交还是取消 | **取消**（不隐式写数据）；可写会话仅 Enter/失焦提交；只读会话 Enter/失焦=取消 |
| Q3 | 只读行/节点/列 | **按 2026-11-01 反馈修订：只读行/列仍可进入编辑，EditBox 只读（可选中/复制）**；新增 `read-only-columns` / `read-only-row-ids` 稀疏声明；只读会话 Enter/失焦**统一按 `cell-edit-cancel` 结束**（不写回、`readOnly=1`） |
| Q4 | 双击与单击/排序顺序 | 第一击沿用现有选择+`item-click`；第二击进入编辑；拖拽/列头分隔线优先 |
| Q5 | 宿主校验/拒绝 | 本批写回后发事件；预留后续 `cell-edit-validating`（可拒绝），不阻塞本 CR |
| Q6 | Tab/方向键 | Enter=提交；Tab=失焦提交并由 `Bench` 焦点遍历切换（不在 EditBox 内消费全局 Tab）；方向键=光标移动（EditBox 原生）；Esc=取消 |
| Q7 | 部分可见单元格 | **按评审 B2 定稿：仅完全可见单元格可进入编辑**；部分可见/不可见双击不进入（后续如需裁剪编辑再单独立项） |
| Q8 | 文本载荷 | 内部 payload + **union `data.grid` 扩展**（oldText/newText/editReadOnly，尺寸不变）+ 只读属性 + Binding 访问器（双通道） |

## 6. 自动化测试计划（实施时必须落地）

- 新增 `test/test_listview_cell_edit_cabi.cpp`：
  1. 属性/JSON round-trip（四键）；ABI：`sizeof(UIEventData)`/union 偏移静态断言（尺寸保持 80）；
  1b. 覆盖层边界：首行/末行/完全可见/部分可见/横向滚动/滚动条附近；部分可见不进入编辑；不越过表头/滚动条/控件边界；点覆盖层不触发行选择；
  1c. 焦点：Enter/Esc 后焦点回 ListView（或原焦点）；点其他控件失焦提交；编辑中销毁不崩溃；Tab=失焦提交+焦点切换；
  2. Begin→文本覆盖→Commit：数据写回、`cell-edited` old/new、无 `color/select` 副作用；
  3. Begin→Cancel：数据不变、`cell-edit-cancel` 仅 old；
  4. Esc、Enter、失焦（点其他控件）路径；
  5. 滚动（滚轮/滚动条）、列宽变化、行增删、排序、清空、刷新、隐藏、禁用、销毁 → 会话取消且无覆盖层残留（`IsEditing==0`）；
  6. `editable-columns` 过滤/非法项/single 模式；
  6b. 只读行/列：仍可进入编辑、EditBox 只读、全选/复制可用；Enter/失焦 → `cell-edit-cancel`（不写回、`readOnly=1`，无 `cell-edited`）；`read-only-columns`/`read-only-row-ids` JSON/属性 round-trip；
  7. 越界/未开启/无会话的 API 失败语义；
  8. 双击阈值（450ms/4px）与拖拽/分隔线优先；
  9. UTF-8/长文本/空串；部分可见/不可见双击不进入编辑；
  10. 版本不匹配防御：入口缺失时 Binding 安全失败（不承诺功能回退）；`sizeof(UIEventData)`/偏移静态断言。
- 新增 Binding 测试 `test/test_listview_cell_edit_binding.cpp`：API、事件访问器、版本不匹配安全失败。
- 探针 `Temp/temp_cr20261101001_probe.cpp`（合成鼠标/键盘事件，参考 `temp_p110_ui_probe.cpp`），覆盖上述关键交互。
- 全量回归 `bash testing_scripts/run_regression.sh`（基线：51 OK + 4 项预存失败）。

## 7. 用户手册更新计划（含示例可运行性）

- `docs/controls/listview.html`：新增"单元格就地编辑"章节（属性、交互、事件表含载荷、JSON 示例、C ABI/Binding 示例）；
- `docs/appendix/declarative-syntax.html`：ListView JSON 示例加 `cell-editing`/`editable-columns`/`read-only-columns`/`read-only-row-ids`；事件键表加 3 个新键；
- `docs/appendix/properties.html`：新增四属性 + 两个只读属性 + 只读标志说明；
- `docs/appendix/capi.html` / `binding.html`：新增 5 个 API 与事件访问器（含 `editReadOnly`/`IsEditReadOnly`、失败语义/版本配套要求）；
- `docs/schema/declarative-ui.schema.json`：同步并 `validate_layout --strict` 通过；
- 示例验证：新 JSON 示例过 schema；C ABI/Binding 示例进入探针编译运行；审计脚本（`Temp/audit_docs*.py`）保持 0 失败。

## 8. 影响面与兼容性

- 源码：`include/ListView.h`、`src/ListView.cpp`、`include/PropertyNames.h`、`src/LayoutParser.cpp`、`include/ControlBase.h`（payload/事件结构）、`src/ControlBase.cpp`、`include/UICornerstoneAPI.h`、`src/UICornerstoneAPI.cpp`；
- Binding：`binding/include/{UICornerstone.h,Event.h}`、`binding/src/{UICornerstone.cpp,Event.cpp,DynamicApi.*}`；
- 文档/schema/手册见 §7；发布：make_release + subModules 同步；
- ABI：**union 成员扩展（`data.grid`），`UIEventData` 尺寸不变**（实测 80 bytes）；版本必须配套，不支持旧 DLL 混用。

## 9. 验收矩阵

1. JSON/CABI/Binding 全链路开启编辑；
2. 双击命中（完全可见才进入；部分可见/不可见不进入）、列过滤、single 模式；
3. Enter/失焦提交；Esc/滚动/列宽/行变更/刷新/隐藏/禁用取消（销毁只清理不回调）；焦点恢复/Tab 语义；
4. begin/cancel 仅 old、edited old+new；只读属性与 Binding 访问器一致；只读会话以 `cancel` 结束（`readOnly=1`）、数据不变；
5. 选择/多选/拖拽/列排序/`item-click` 不回归；
6. 探针 + 新测试 + 全量回归；手册示例 strict/可运行；版本不匹配时功能禁用（不崩溃）。

## 10. 待评审确认

- 已按 2026-11-01 评审修订：B1 union 扩展、B2 仅完全可见进入、焦点/Tab/销毁规则、Q6/Q7/Q8 定稿；
- 只读属性 `edit-old-text/edit-new-text` **保留**（与事件同源）；
- 是否需要"可编辑性查询"事件：不需要（双击前不发）。

## 修订记录

- 2026-11-01 初稿。
- 2026-11-01 按设计器评审修订：B1 `data.grid` union 扩展（尺寸不变）、B2 仅完全可见进入、焦点/Tab/销毁规则、Q6/Q7/Q8 定稿、测试/手册同步。
- 2026-11-01 用户确认**版本配套政策**（不支持旧 DLL/旧宿主混用）；相关回退条目改为“版本不匹配安全失败/功能禁用”，union 尺寸不变仅作附加收益。
- 2026-11-01 复核（R4）：**Tab 语义定稿为“失焦提交 + Bench 焦点切换”**，设计器已接受；R2 设计器 CR 原文同步由其自行完成。
- 2026-11-01 复核（R2）：设计器 CR 原文的旧 DLL 回退条款由设计器同步修订，引擎不代改。
- 2026-11-01 复核（终）：设计通过；按 R7 清理文档表述（仅完整可见、不依赖裁剪；删除 A 方案待确认项），进入实施。

## 实施记录（2026-11-01）

- ListView 单元格编辑、只读语义、CR-003 互斥；测试 test_cr_cells / test_cr_ui；探针 cr_cells/cr_ui/cr_binding。
- 全量回归：53 OK + 4 项预存失败；探针 18 组全过；Schema strict 63 示例 0 失败；`UIEventData` 尺寸保持 80。
- 发布并同步 `subModules/UICornerstone`（make_release + robocopy /MIR）。

## 验收闭环（2026-11-02）

- 设计器联测通过；CR 状态「设计器：closed」「引擎：closed」。
- 相关设计/复核文档按归档规则可归档（文件保留在 `design/` 与 `requirements/`，不另移动）。
