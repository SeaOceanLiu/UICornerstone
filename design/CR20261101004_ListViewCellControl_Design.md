# CR20261101004 — ListView 通用单元格控件（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261101004）
> 提出方：CornerstoneDesigner
> 状态：**已闭环（closed）**（2026-11-02；设计器联测通过，CR「设计器/引擎」均为 closed）
> 兼容政策：引擎 DLL / Binding / 宿主必须同批配套；不支持旧 DLL/旧宿主混用；缺入口=功能不可用，不做回退。

---

## 1. 需求摘要

ListView 需要通用 per-cell 控件能力（而非 action-column 专用），使设计器事件面板能把"代码/删除"按钮组合（Panel）放入单元格。要求：`FILL`（覆盖单元格内容区）/`LEADING`（既有图标槽）两种布局；每格一个控件；引擎接管所有权；滚动/列宽/行排序移动/增删/列增删/隐藏/禁用/销毁时定位正确或安全移除；控件消费自身事件，未消费时行选择/`item-click` 照旧；C ABI/Binding 同步；本期不强制 JSON。

## 2. 现状与源码核实

- `ListRow.cellControls`：稀疏 `map<int, shared_ptr<Control>>`（`include/ListView.h:84`）；`syncChildControls()` 以**图标槽**定位：`x=columnX(col)+kIconSlotInset`、`w=min(rowHeight-2*inset, colW-2*inset)`、`h=rowHeight-2*inset`（`src/ListView.cpp:225-245`）。
- C ABI 仅有 `UICornerstone_ListViewSetCellLeadingControl`（`:2606`）→ `setCellLeadingControl`（`src/ListView.cpp:455-462`）；无 layout 参数、无 Clear/Get API。
- 绘制：行内容先画、`popClipRect()` 后 `ControlImpl::draw()` 画子控件（`src/ListView.cpp:856-861`）——子控件不按内容区裁剪；部分可见行可能越界（与 P0-111 B2 同类）。
- 事件：`handleEvent` 先处理滚动条/键盘/数据区行选择，命中行即 `selectNode` 等并 `return true`（`src/ListView.cpp:940-1000` 附近）——**子控件当前无法先消费鼠标**；因此现有 cellControls 只适合非交互图标。
- 生命周期：`removeRow` 移除并 `removeControl`；`insertColumn/removeColumn` 迁移/删除 `cellControls` 键；`setColumns` 仅截断 cells，**未清理被删列控件**（需在 CR 中补齐）；排序 `sortByColumn` 按 `ListRow` 整体移动（控件跟随）。
- 焦点：控件 `setContext` 后注册 FocusManager；Tab 可达；但 ListView 行选择先拦截点击，需调整分发顺序。
- Binding：`ui->ListViewSetCellLeadingControl` 已有；无通用 cell control。

## 3. 分析（可行性 / 风险）

1. `cellControls` 基础设施可复用；需增加 **layout 类型**（FILL/LEADING）与几何同步分支。
2. **所有权**必须明确：引擎接管控件（shared_ptr + addControl），宿主不得再 Destroy；Clear/删除时由引擎释放。Binding 代理失效规则需文档化。
3. **事件分发**需改：鼠标事件先命中已挂 cell control（按 z 逆序/contains），被消费则不选行/不触发 `item-click`/不进入编辑；未消费才走既有逻辑。键盘仅在控件聚焦时由子控件消费（ListView 键盘导航仅在 ListView 自身 focused 时生效）。
4. **裁剪**：FILL 控件必须避免越出内容区/表头/滚动条。方案：**仅单元格完整可见时显示 FILL/LEADING 控件**（隐藏部分可见），与 P0-111 可见性规则一致；不做子控件裁剪。滚动时隐藏/重现，不残留。
5. 行/列变更：排序/移动行整体携带；删除行列/clear/`setColumns` 缩列时移除并销毁越界控件；列插入索引迁移已有。
6. 与单元格编辑冲突：同一格存在 FILL 控件时，双击由控件消费（按钮/Panel 内控件优先）；未消费时保持原有双击编辑；LEADING 不遮挡文本区，不改变编辑入口。
7. JSON 暂不新增；既有 `cellControls` JSON 语义保持 LEADING（向后兼容），新 API 默认 FILL。

## 4. 设计

### 4.1 C ABI（命名沿用既有 `ListView<Action>` 风格）

```c
typedef enum {
    UI_LIST_CELL_LAYOUT_FILL = 0,     /* 单元格内容区（默认） */
    UI_LIST_CELL_LAYOUT_LEADING = 1   /* 既有图标槽 */
} UIListCellControlLayout;

int UICornerstone_ListViewSetCellControl(UIInstance, UIControlHandle lv,
                                         int row, int col, UIControlHandle ctl, int layout);
int UICornerstone_ListViewClearCellControl(UIInstance, UIControlHandle lv, int row, int col);
UIControlHandle UICornerstone_ListViewGetCellControl(UIInstance, UIControlHandle lv, int row, int col);
```

- 返回：Set/Clear 1=成功 0=失败（越界/非 list-view/句柄无效/生命周期中）；Get 返回句柄或 NULL；
- `ctl=NULL` 等价 Clear；
- `layout` 非法值 → 失败；默认由调用方显式传 `UI_LIST_CELL_LAYOUT_FILL`；
- 旧 `ListViewSetCellLeadingControl` 保留，等价 `SetCellControl(..., LEADING)`（兼容既有 JSON/宿主）。

**所有权/Binding 语义（复核 B2 定稿：借用语义，不承诺自动失效）**：
- `SetCellControl` 仅接受**同一实例、尚未挂父**的新控件（`getParent()==nullptr` 且 `validateControl` 通过）；已有父容器/属于另一实例 → 返回 0，不自动 detach/reparent；
- 成功后引擎为唯一所有者（`map` + `addControl`）；传入的 `Control&` 代理与 `GetCellControl` 句柄均为**借用**，宿主不得 `Destroy`/`RemoveChild`；Clear/替换/删除/销毁后**不得再使用**；
- **不承诺** `IsValid()==false`：Binding 代理可能仍持有共享状态（取决于宿主其它引用），引擎无法回收/置空 C++ 侧代理；以“借用生命周期结束即失效、不得访问”为契约，测试按此语义；
- `Clear`/替换/行删除/列删除/`setColumns` 缩列/ListView 销毁 → 引擎摘树并释放；若控件（或后代）持有焦点，先 `focusControl(ListView)` 交还焦点，再摘除/释放（先交还、后移除）；
- 宿主自行保留 C++ `shared_ptr`：引擎释放后控件可能脱离树继续存活，行为不受支持（文档禁止）；
- `GetCellControl` 返回只读借用；移除后返回 NULL/空 Control，测试以“返回空 + 不得使用”为准。


### 4.2 Binding

- `ui->ListViewSetCellControl(Control& lv, int row, int col, Control& ctl, int layout = UI_LIST_CELL_LAYOUT_FILL)`;
- `ui->ListViewClearCellControl(...)`;
- `Control ui->ListViewGetCellControl(...)`（无效返回空 `Control`）；
- 版本不匹配（缺入口）→ 安全失败（false/空），设计器提示/禁用功能。

### 4.3 几何/布局

- **FILL**：`x = columnX(col) + fillPaddingX`，`w = colW - 2*fillPaddingX`，`y = rowTop + fillPaddingY`，`h = rowHeight - 2*fillPaddingY`；`fillPaddingX=2`、`fillPaddingY=1`（常量入 ConstDef/PropertyNames 不需要；内部常量并写入文档）；single 模式列 0 铺满。
- **LEADING**：保持既有图标槽：`x=columnX+4`、`y=rowTop+4`、`w=h=rowHeight-8`（并受列宽上限钳制）。
- 文本绘制：既有单元格文本仍绘制；FILL 控件覆盖其上（宿主可用透明 Panel 叠加）；不改变文本 API。
- 可见性：
  - **FILL**：仅单元格**完整可见**时显示——纵向行完整在数据区、横向列完整在内容区且不被垂直/水平滚动条覆盖；部分可见隐藏（不可见、不可点击）；滚动/列宽/行高/模式变化即时重算；
  - **LEADING**：保持既有图标槽可见性规则（不改变既有行为，兼容现有 JSON/宿主）；仅 FILL 采用"完整可见"策略；
  - 若隐藏时控件（或后代）持有焦点 → 焦点交还 ListView；重新可见时不自动抢焦点。
- 与编辑覆盖层：同一单元格编辑会话期间隐藏 cell control；结束时恢复（若仍有）。

### 4.4 生命周期/所有权矩阵

| 触发 | 行为 |
|---|---|
| SetCellControl | 仅无父控件；引擎 addControl + setContext/create + 持有 shared_ptr；替换旧控件：若有焦点先交还 ListView，再移除释放 |
| ClearCellControl | 移除并释放（若宿主仍持有共享引用则仅摘树） |
| 行删除/清空 | 移除该行所有 cell controls 并释放 |
| 列删除/`setColumns` 缩列 | 移除越界列 cell controls 并释放（补齐现有缺口） |
| 行排序/移动 | 控件随 `ListRow` 整体迁移，不重建 |
| 列插入/删除（索引迁移） | 已迁移索引；删除列执行上一条 |
| 滚动/列宽/行高/模式切换 | 重新定位；部分可见隐藏；不销毁 |
| 控件隐藏/禁用/ListView 销毁 | 子控件随父隐藏/销毁；ListView 析构不回调事件 |
| 宿主 DestroyControl | **禁止**（引擎持有）；文档标注为未定义 |
| 移除/隐藏时焦点 | 焦点交还 ListView，不留悬垂 |

### 4.5 事件/焦点

**通用子控件派发与捕获（三次复核 B1/B2 定稿）**：现有 `ControlImpl::handleEvent` 只返回 bool，无法定位消费子控件。新增受保护 helper：

```cpp
// 逆序派发子控件；MouseDown 消费时记录 captured；MouseUp/Move 优先派发 captured；
// MouseUp/取消/隐藏/禁用/移除时清理 captured。Panel 递归同样使用，天然支持复合控件。
bool ControlImpl::dispatchToChild(shared_ptr<Event> event, Control** consumedChild = nullptr);
```

- `dispatchToChild` 内部维护 `Control* m_capturedChild`：
  - MouseDown：先清空旧 capture；逆序子控件派发，首个返回 true 的子控件设为 captured（Panel 内部同样递归设置其 captured，叶子按钮可收到后续 Up/Move）；
  - MouseUp/Move：若 captured 可见/启用 → **先直接派发 captured**（即使指针已移出/在其它控件上），未消费再派发其它子控件；MouseUp 后清空 captured；
  - MouseWheel：按逆序正常派发，不改变 captured；
  - captured 隐藏/禁用/移除/滚出视口：派发前检测并清空 capture；`removeControl` 若移除 captured 同步清空并复位其 pressed 态；`FocusLost` 清空 capture；
- ListView 的 `handleEvent` 开头调用 `dispatchToChild`；返回 true → 直接返回，**不选行/不触发 `item-click`/不进入编辑/不重复派发**。Viewport 只调用父控件，子控件派发统一由该 helper 完成。

| 事件 | 顺序（优先级从高到低） |
|---|---|
| MouseDown | ① `dispatchToChild`（EditBox/滚动条/cell control，逆 z；Panel 内按钮可成为 captured 叶子）→ 消费：返回（不选行/不编辑）；未消费 → ② 表头分隔线/排序 → ③ 数据区行选择 + `item-click` + 双击计时 |
| MouseUp | ① `dispatchToChild`（captured 优先，指针移出仍派发；未消费再派发其它子控件）→ 消费：返回；未消费 → ② 结束列宽拖拽 → ③ 双击判定（FILL 屏蔽/LEADING/无控件）→ ④ 清 pressed |
| MouseWheel | ① `dispatchToChild`（含 FILL Panel，消费则停止）→ ② 滚动 ListView |
| MouseMove | ① `dispatchToChild`（captured 优先；其它子控件 hover）→ 消费：返回；未消费 → ② 列宽拖拽/行 hover/光标 |
| 双击 | 仅当子控件未消费：命中单元格存在 **FILL** 控件 → 不进入 P0-111 编辑（编辑屏蔽）；LEADING/无控件按既有规则 |
| 拖拽/排序 | 子控件未消费且位于表头 → 分隔线/排序；数据区无行拖动语义，FILL 空白未消费时按行选择处理 |

**captured 清理矩阵**：

| 触发 | 行为 |
|---|---|
| MouseUp（任意位置） | captured 收到 Up 后清空；pressed 由控件自身复位 |
| captured 隐藏/禁用 | 清空 capture，控件 `setState(Normal)` 复位 |
| captured 被移除/行删/列删/缩列/清空/ListView 销毁 | 父容器 `removeControl` 清空 capture 并复位；不回调悬垂指针 |
| 滚动导致 cell control 隐藏 | ListView 隐藏子控件后，下一次派发检测隐藏并清空 capture/复位 |
| FocusLost/窗口失焦 | 清空 capture，复位 pressed |
| 控件被替换 | 先清 capture/焦点，再摘除旧控件 |

- FILL 透明 Panel：空白区 MouseDown 未消费 → 回落行选择/`item-click`；按钮消费则停止；FILL 区域双击禁止进入编辑；
- 键盘：cell control 可聚焦；Tab 遍历注册；ListView 仅在自身 focused 时处理行导航；控件聚焦时方向键/Enter 由其处理；
- 不新增 cell-control 专属事件（宿主用控件自身回调）。

### 4.6 JSON

- 本期不新增 JSON key；既有 `cellControls` 数组语义固定为 **LEADING**；`FILL` 仅程序化 API。后续如需 JSON：`cellControls[].layout: "fill"|"leading"`，另立 CR。

## 5. Q/待评审

| # | 问题 | 引擎建议 |
|---|---|---|
| Q1 | 所有权 | **引擎接管**（Set 后宿主不得 Destroy；Clear/删除由引擎释放） |
| Q2 | FILL padding/对齐 | 2px 横 / 1px 纵（内容区居中铺满）；如需自定义布局用 Panel 内部自排 |
| Q3 | 行排序/移动跟随 | 随 `ListRow` 整体迁移，不重建 |
| Q4 | TreeView 复用 | 本 CR 仅 ListView；TreeView 现有 leadingControl 机制独立，后续如需要另立 CR |
| Q5 | 多控件/单元格 | 单控件；复合用 Panel 组合（设计器已按此方案） |
| Q6 | 部分可见/裁剪 | 仅完整可见显示（不做子控件裁剪），与 P0-111 一致 |
| Q7 | 与单元格编辑冲突 | FILL 命中由控件消费则不进编辑；LEADING/空白行为不变 |
| Q8 | JSON | 本期不新增；后续另立 |
| Q9 | Get 返回句柄 | 只读借用，不得 Destroy；移除后失效 |
| Q10 | 已有父控件 | Set 失败（不自动 detach） |
| Q11 | FILL 与编辑 | FILL 区域双击不进入编辑（编辑屏蔽）；LEADING/无控件不变 |

## 6. 自动化测试计划（实施时必须落地）

1. Set FILL Panel（两按钮）→ 点击按钮/空白：按钮回调正确、不触发行选择/`item-click`；点击单元格文本区（无控件覆盖）仍选行；
2. LEADING 兼容旧 `SetCellLeadingControl`/JSON cellControls 行为；
3. 滚动/列宽/行高/模式切换：定位正确；部分可见时隐藏且不可点击，完全可见恢复；
4. 行排序/移动、行增删、列增删、`setColumns` 缩列、清空/销毁：控件跟随或释放、无崩溃/泄漏；借用结束后不再访问（不作为 `IsValid` 断言）；
5. 焦点/Tab：按钮可聚焦、Tab 切换、不误触发行键盘导航（控件聚焦时）；
6. 与 P0-111 编辑互斥：FILL 双击不进编辑；LEADING/无控件双击仍可编辑；
7. C ABI/Binding 失败语义（越界/无效句柄/非法 layout/版本不匹配）；
8. Binding 借用语义：Set 后代理可用（借用）；Clear/替换/行删除/列删除/`setColumns` 缩列/销毁后**不得再使用**（不承诺 `IsValid`）；`GetCellControl` 移除后返回 NULL/空；已挂父控件/跨实例 Set 返回 0；
9. 焦点：cell control 聚焦时行删除/隐藏/滚出视口 → 焦点交还 ListView，不崩溃；
10. 事件派发逐项：按钮、FILL 空白、双击（编辑屏蔽）、滚轮、拖拽/排序、表头优先；
11. 32px 行高下 FILL Panel 内按钮可点击（探针/人工）；
12. captured：按下 cell Button → 指针移出 → MouseUp：按钮释放/pressed 复位、不选行；
13. captured：MouseDown 消费后指针移到另一 cell control → MouseUp 只派发 captured；
14. captured 控件滚出内容区/隐藏/删除/缩列：capture 清空、无崩溃；
15. FILL Panel 空白区 MouseDown 未消费 → 行选择；按钮消费 → 不选行；
16. MouseWheel 覆盖 cell control 时消费/不消费两条路径；
17. 探针 `Temp/temp_cr20261101004_probe.cpp`；全量回归（基线 53 OK + 4 项预存失败）。

## 7. 用户手册更新计划（含示例可运行性）

- `docs/controls/listview.html`：新增"单元格控件（cell control）"章节：FILL/LEADING、所有权、生命周期、事件/焦点、C ABI/Binding 示例；
- `docs/appendix/capi.html` / `binding.html`：新增 3 个 API/包装；`ListViewSetCellLeadingControl` 标注等价 LEADING；
- `docs/appendix/properties.html`：无新增属性（本期 JSON 不变）；
- `docs/schema`：无变更；
- 示例验证：JSON 示例 strict、C ABI/Binding 示例进入探针编译运行；审计脚本 0 失败；
- 文档注明：FILL 内缩不改变单元格文本位置（文本仍绘制，透明 Panel 可透出）；Binding 代理借用/失效语义；LEADING 保持既有可见性。

## 8. 影响面与兼容性

- 源码：`include/ListView.h`、`src/ListView.cpp`、`include/UICornerstoneAPI.h`、`src/UICornerstoneAPI.cpp`、`include/PropertyNames.h`（枚举/常量如需）；
- Binding：`binding/include/UICornerstone.h`、`binding/src/UICornerstone.cpp`、`DynamicApi.*`；
- 兼容：旧 `ListViewSetCellLeadingControl`/JSON `cellControls` 保持 LEADING 语义；版本必须配套。

## 9. 验收矩阵

1. 设计器事件面板可迁移到 ListView：四列 + 操作列 Panel（代码/删除按钮）可用；
2. 按钮点击正确、行选择/排序/滚动不误触；行/列增删移/清空/销毁无残留；
3. FILL 覆盖完整单元格；LEADING 保持既有槽位；
4. C ABI/Binding 可用；探针 + 新测试 + 全量回归；手册示例可运行。

## 10. 待评审确认

- 所有权/Binding 借用/失效、Get 只读、已有父控件失败、焦点交还；
- FILL padding（2/1px）与"仅完整可见显示"（LEADING 保持既有）；
- FILL 双击编辑屏蔽、事件派发表；
- 本期仅 ListView、仅单控件、不新增 JSON。

## 修订记录

- 2026-11-01 按设计复核修订：所有权/Binding 代理/Get/Clear/重父/焦点语义；事件派发顺序表；FILL/LEADING 可见性边界；FILL 编辑屏蔽；测试计划补齐。
- 2026-11-01 按二次复核修订：事件分发改为“子控件先/ListView 不重复派发”并明确 captured；Binding 借用语义（不承诺 IsValid）；清理格式并补测试项。
- 2026-11-01 按三次复核修订：新增 `ControlImpl::dispatchToChild` + 通用 captured 机制；重写事件顺序与 captured 清理矩阵；补 captured/FILL/Wheel 测试。

## 实施记录（2026-11-01）

- 新增 `ControlImpl::dispatchToChild` + `m_capturedChild` 捕获机制（Panel 递归），ListView 开头统一派发，避免双重派发；
- `ListViewSetCellControl/ClearCellControl/GetCellControl`（FILL 2/1px、LEADING 兼容、引擎所有权、借用语义）；FILL 仅完整可见显示；FILL 双击编辑屏蔽；行/列删除/缩列/销毁清理与焦点交还；
- 测试 `test/test_cr_cellcontrol.cpp`（FILL 按钮/空白/双击/捕获移出/LEADING/行删/CR005）+ Binding 探针；全量回归 54 OK + 4 项预存失败。

## 验收闭环（2026-11-02）

- 设计器联测通过；CR 状态「设计器：closed」「引擎：closed」。
- 相关设计/复核文档按归档规则可归档（文件保留在 `design/` 与 `requirements/`，不另移动）。
