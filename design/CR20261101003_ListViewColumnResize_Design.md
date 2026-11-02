# CR20261101003 — ListView 设计态列宽拖拽手势委托（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261101003，原 P0-113）
> 提出方：CornerstoneDesigner
> 状态：**已闭环（closed）**（2026-11-02；设计器联测通过，CR「设计器/引擎」均为 closed）
> **兼容政策（2026-11-01 用户确认）**：引擎 DLL、Binding、宿主必须**同批配套**；不支持旧 DLL/旧宿主混用（与 P108 变体 A 一致）。以下“旧 DLL”条目均改为“版本不匹配 = 功能不可用”，不承诺功能回退。设计器 CR 原文中的旧 DLL 回退条款由**设计器同步修订**（引擎不代改）。
> 参考：`requirements/引擎ListView_TreeView编辑模式_设计建议.md` §2.1/§2.2

---

## 1. 需求摘要

ListView（multi 视图）运行时已支持列头分隔线拖拽调宽；设计画布拦截鼠标导致运行时手势不可用。用户定稿：**手势归引擎**，引擎提供设计态命中/会话 API（Begin/Update/End/Cancel），设计器只转发指针、持久化模型、记一次撤销；引擎发 `column-resize-*` 事件带 `col/oldWidth/newWidth/canceled`。

## 2. 现状与源码核实

- 运行时拖拽：`ListView::handleEvent` 列头区分隔线命中（`fabs(lx - right) <= kColResizeHitW`）→ `m_dragCol/m_dragStartX/m_dragStartWidth`；`MouseMove` 调 `setColumnWidth`；`MouseUp` 结束（`src/ListView.cpp:880-940` 附近）。
- 宽度写回/钳制：`setColumnWidth`（`include/ListView.h:125`；`min-column-width` 钳制/relayout）。
- 列几何：`columnX(col)`、`hitTestColumn(x)`、`totalContentWidth()`（`include/ListView.h:227-229`）。
- 现有 C ABI：`UICornerstone_ListViewSetColumnWidth`（可用但仅写宽，不提供手势会话/旧宽/取消恢复）。
- 现有事件：`column-sort`；无 resize 事件。
- 表头命中常量：`kColResizeHitW`（`src/ListView.cpp` 文件内常量）；命中/钳制需与运行时**同一判定**。

## 3. 分析

1. 设计态画布拦截鼠标，不能依赖 `ListView::handleEvent`；需要**纯函数式会话 API**（宿主传入局部坐标），引擎内部复用运行时几何/钳制/relayout。
2. Begin 记录 `oldWidth`（会话唯一状态）；Update 返回当前宽度；End(commit)/Cancel 恢复或保留；任何失败不得改变列宽。
3. 与选择/排序/编辑互斥：这些 API 只处理表头分隔线，不产生行选择/排序/`item-click`/单元格编辑副作用。
4. 模型来源：设计器持久化宽度；引擎不提供 JSON 运行时键（本 CR 无新增声明式属性）。
5. 版本政策：DLL/Binding/宿主必须配套；入口缺失 = 版本不匹配，设计器提示/禁用功能，**不做回退**。
6. 事件：begin 仅 `col/oldWidth`；changed/ended 带 `col/oldWidth/newWidth`；ended 另带 `canceled`。公共载荷采用 **union `data.resize` 扩展**（评审 B1，`UIEventData` 尺寸不变）。
7. 归属定义（评审 B3）：**A 方案**——引擎拥有命中/宽度计算/会话，设计器拥有鼠标转发与模型持久化（不做设计态鼠标路由）。

## 4. 设计

### 4.1 C ABI（命令式，不做鼠标捕获）

```c
/* 命中：multi 表头分隔线（8px 半宽，与运行时同判定）；返回列索引，未命中 -1 */
int UICornerstone_ListViewHitTestColumnDivider(UIInstance, UIControlHandle lv,
                                               float localX, float localY);
/* 会话：localX 为 ListView 局部逻辑坐标 */
int   UICornerstone_ListViewBeginColumnResize(UIInstance, UIControlHandle lv,
                                              int col, float localX);   /* 1/0，记录 oldWidth */
float UICornerstone_ListViewUpdateColumnResize(UIInstance, UIControlHandle lv,
                                               float localX);           /* 返回当前宽度；无会话抛回原宽 */
int   UICornerstone_ListViewEndColumnResize(UIInstance, UIControlHandle lv,
                                            int commit);                /* 1=保留 0=恢复 oldWidth */
int   UICornerstone_ListViewCancelColumnResize(UIInstance, UIControlHandle lv); /* 恢复 oldWidth */
/* 显式光标控制（设计态；纯查询 API 不改变光标）：active=1 → EW_Resize；0 → 默认（仅当前 owner 生效） */
int   UICornerstone_ListViewSetColumnResizeCursor(UIInstance, UIControlHandle lv, int active);
```

- 失败/无会话：`HitTest` 返回 -1；`Update` 返回 -1.0f；`End/Cancel` 返回 0。
- `Begin` 前置校验（复核 R5）：multi 视图、列索引有效、表头存在、控件可见/启用/已 create/未销毁；否则 0；`HitTest` 同样对隐藏/禁用/未 create/销毁中返回 -1。
- 宽度钳制：`max(min-column-width, proposed)`，与运行时一致；relayout 在 Update/End 内完成。
- 并发：单一会话；重复 Begin 返回 0；`m_dragCol >= 0`（运行时列头拖拽中）时 Begin 返回 0；API 会话活跃时运行时列头拖拽拒绝进入（`handleEvent` 守卫）。
- **外部变更清理矩阵**：会话中**外部调用** `setColumnWidth`/`setColumns`/列增删/滚动/`setRect`/resized/隐藏/禁用 → 取消会话并发 `ended{canceled=1}`（宽度恢复 oldWidth）；**控件销毁 → 静默清理，不回调**。
- **内部写宽路径（复核 R1）**：`UpdateColumnResize` 必须走私有 `applyColumnWidthInternal(col,width)`（或 `m_apiResizeActive` 守卫）完成钳制/relayout，**不得触发“外部变更取消”**；只有宿主/其它代码调用公开 `setColumnWidth` 才算外部变更。连续 `Update` 不产生 cancel/ended，session 保持。
- **事件恰好一次**：成功 Begin 后，End(commit=1/0) 或 Cancel 必发且仅发一次 `ended`；失败调用不发事件。`changed` 仅在宽度实际变化时发（同坐标重复 Update 去重）。
- **坐标口径**：`localX/localY` 为 ListView 局部逻辑坐标（与 `columnX` 同域；已含水平滚动偏移，不含 DPI/窗口变换）。设计器换算：`local = (screen - getDrawRect().left/top) / getScaleX/Y()`；写入手册示例。

### 4.2 Binding

- `Control`/`UICornerstone` 封装：`ListViewHitTestColumnDivider(lv, x, y)`、`ListViewBeginColumnResize(lv, col, x)`、`Update...`、`End...(commit)`、`Cancel...`、`ListViewSetColumnResizeCursor(lv, active)`；
- 入口缺失（版本不匹配）→ Binding 防御性返回 `-1`/`false`/`-1.0f`，设计器提示/禁用功能（不承诺回退）；
- 事件访问器：`IsColumnResizeBegin/Changed/Ended`、`GetResizeCol/GetResizeOldWidth/GetResizeNewWidth/GetResizeCanceled`。

### 4.3 事件与载荷（union 扩展，UIEventData 尺寸不变）

- 事件名/常量：`column-resize-begin` / `column-resize-changed` / `column-resize-ended`
  （`kEventColumnResizeBegin/kEventColumnResizeChanged/kEventColumnResizeEnded`）。
- `UIEventData.data` 新增 union 成员 `resize`（评审 B1 定稿；`UIEventData` 尺寸不变）：
  ```c
  struct { int col; float oldWidth; float newWidth; int canceled; } resize;
  ```
  begin：`canceled=0`、`newWidth=oldWidth`；changed：`canceled=0`（仅实际变化时发）；ended：commit=1 → `canceled=0`，commit=0 → `canceled=1` 且 `newWidth=oldWidth`；外部变更取消 → `ended{canceled=1}`。
- 可选只读属性 `column-resize-old-width/new-width`：**建议不做**（事件已带值），如设计器需要再加。
- 仅由上述 API 触发；引擎不监听鼠标（A 方案：设计器转发指针并持久化模型；若后续要求引擎接管设计态鼠标需另立 CR）。

### 4.4 声明式

- 无 JSON 属性（纯设计态 API）；文档在 ListView 页"设计态协作 API"章节与 C ABI/Binding 速查中说明。

## 5. Q/待评审

| # | 问题 | 引擎建议 |
|---|---|---|
| Q-1 | 命中半宽 | **8px**（与运行时 `kColResizeHitW` 一致；±8px 区间） |
| Q-2 | 重复 Begin / 运行时拖拽中 | 均返回 0（单会话；API 活跃时运行时拖拽拒绝） |
| Q-3 | 取消/结束事件 | 取消发 `ended{canceled=1}`；不额外发 `changed` |
| Q-4 | 失败时宽度 | 保持不变（Begin/Update 失败均不改宽） |
| Q-5 | 只读属性 | 本批不做（事件载荷足够） |
| Q-6 | 事件触发时机 | `End` 内恢复/保留完成后发 `ended`（宿主可读最终宽）；成功 Begin 后 `ended` 恰好一次 |
| Q-7 | 外部变更 | 取消会话 + `ended{canceled=1}`（销毁除外：静默清理） |
| Q-8 | changed 去重 | 仅宽度实际变化时发 |

## 6. 自动化测试计划

- 新增 `test/test_listview_column_resize_cabi.cpp`：
  1. 命中边界（分隔线 ±8px、列头中心/行区/滚动条 → -1）；
  2. Begin→Update→End(commit)：宽度按 proposed+钳制、relayout、事件载荷/顺序；
  3. End(0)/Cancel：恢复 oldWidth、`canceled=1`；
  4. 无会话/无效列/重复 Begin/非 multi → 失败且宽度不变；
  5. 选择/排序/`item-click`/单元格编辑无副作用；
  6. 最小宽度钳制、极值/负宽；
  7. Binding：版本不匹配（入口缺失）安全失败（返回值 -1/false 且宽度不变；不承诺功能回退）；
  8. 并发/生命周期：API session + 运行时 `m_dragCol` 互斥；隐藏/禁用/未 create/销毁中 `HitTest/Begin` 失败；列增删/`setColumns`/外部 `setColumnWidth`/滚动/`setRect`；`changed` 去重；`ended` 恰好一次；
  8b. **连续 `Update` 不自取消**（内部写宽路径）；仅外部 `setColumnWidth` 触发一次取消 + `ended{canceled=1}`；
  9. ABI：`sizeof(UIEventData)`/union 偏移静态断言。
- 探针 `Temp/temp_cr20261101003_probe.cpp`（直接调用 API 序列，校验宽度与事件）；
- 全量回归（基线 53 OK + 4 项预存失败）。

## 7. 用户手册更新计划

- `docs/controls/listview.html`：新增"设计态列宽拖拽（引擎手势 API）"章节：命中/会话/提交/取消、事件表（载荷）、设计器接入顺序（Down→Update*→End）、版本不匹配提示；
- `docs/appendix/capi.html` / `binding.html`：新增 6 个 API（含 SetColumnResizeCursor）+ 3 个事件访问器；
- `docs/appendix/properties.html`：无需新增（本 CR 无属性）；如需只读属性另补；
- `docs/schema`：无变更；
- 示例：C ABI/Binding 示例纳入探针编译运行；事件载荷文档与实现一致。

## 8. 影响面与兼容性

- 源码：`include/ListView.h`、`src/ListView.cpp`、`include/PropertyNames.h`、`include/ControlBase.h/.cpp`、`include/UICornerstoneAPI.h`、`src/UICornerstoneAPI.cpp`；
- Binding：`Event.h/.cpp`、`UICornerstone.h/.cpp`、`DynamicApi.*`；
- ABI：union `data.resize` 扩展（`UIEventData` 尺寸不变，实测 80 bytes）；版本必须配套，不支持旧 DLL 混用；
- 运行时行为不变（设计态 API 独立）。

## 9. 验收矩阵

1. 设计画布拖拽列头分隔线与运行时宽度/钳制/视觉一致；
2. Begin/Update/End/Cancel 完整会话；取消恢复；重复/无效调用无副作用；
3. 事件序列与载荷正确（begin 仅 old；changed/ended 带 new；ended 带 canceled）；
4. 不产生选择/排序/编辑副作用；版本不匹配时安全失败/功能禁用；Binding/C ABI 可用；
5. 新测试 + 探针 + 全量回归；手册示例可运行。

## 10. 待评审确认

- API 命名（沿用既有 `UICornerstone_ListView<Action>` 风格，无下划线）；
- 重复 Begin 行为（返回 0 vs 自动取消旧会话）；
- 是否需要只读属性通道（当前建议不做）；
- 已按 2026-11-01 评审修订：B1 union `data.resize`、B3 归属采用 A 方案、session/运行时互斥与外部清理矩阵、坐标口径、事件恰好一次/去重；
- A 方案（设计器转发指针）已经设计器复核确认（2026-11-01）；若产品改为引擎接管设计态鼠标事件，另立 CR。

## 修订记录

- 2026-11-01 初稿。
- 2026-11-01 按设计器评审修订：B1 `data.resize` union 扩展（尺寸不变）、B3 归属采用 A 方案、session/运行时互斥、外部变更清理矩阵、坐标口径、`ended` 恰好一次与 `changed` 去重。
- 2026-11-01 用户确认**版本配套政策**（不支持旧 DLL/旧宿主混用）；相关回退条目改为“版本不匹配安全失败/功能禁用”，union 尺寸不变仅作附加收益。
- 2026-11-01 复核：R1 内部 `applyColumnWidthInternal` 守卫（Update 不自取消）；R5 补齐隐藏/禁用/未 create/销毁中前置校验；R4 A 方案口径经设计器确认。
- 2026-11-01 复核（R2）：设计器 CR 原文的旧 DLL 回退条款由设计器同步修订，引擎不代改。
- 2026-11-01 复核（终）：设计通过；按 R7 清理文档表述（仅完整可见、不依赖裁剪；删除 A 方案待确认项），进入实施。

## 实施记录（2026-11-01）

- 列宽手势 API/session/事件/外部清理；测试 test_cr_cells；探针 cr_cells/cr_binding。
- 全量回归：53 OK + 4 项预存失败；探针 18 组全过；Schema strict 63 示例 0 失败；`UIEventData` 尺寸保持 80。
- 发布并同步 `subModules/UICornerstone`（make_release + robocopy /MIR）。

---

## 7. 合并 CR20261101005：运行时列宽拖拽可发现性（2026-11-01）

> 原 CR20261101005 与 CR-003 同属 ListView 列宽拖拽，已合并为本 CR 的运行体验修订；CR-005 不再单列。

### 7.1 现状问题

- 修订前运行时与 `ListViewHitTestColumnDivider` 仅识别列头分隔线 ±4px，命中区偏窄且无 resize 光标；本次统一为 **±8px** 并增加光标；
- `lvResource`（宽 240）名称/路径列及最后一列分隔线靠近右边界/滚动条，用户容易点在表头正文触发排序而不是改宽，表现为"无法拖动列宽"。

### 7.2 设计决策（2026-11-01 复核修订）

1. **命中半宽统一为 8px**（原 `kColResizeHitW=4`）：运行时与设计态 API 共用同一常量/函数；分隔线优先于排序，表头正文排序不受影响。
2. **最后一列可见性条件与命中线**：
   - 坐标口径：`localX/localY` 为 ListView 局部逻辑坐标；`columnX(c)` 已含 `-m_hScrollOffset`；`contentRight = m_rect.width - (垂直滚动条可见 ? 滚动条宽 : 0)`；`contentY` 表头区为 `0 <= localY < m_headerHeight`。
   - 最后一列参与命中必须与内容区有交集：`lastLeft < contentRight && lastRight > 0`；否则返回 -1（完全滚出左/右均不命中）。
   - 可见分隔线：若 `lastRight > contentRight - 1`（右缘被裁剪）→ `hitX = contentRight - 1`；否则 `hitX = lastRight`；命中条件 `|localX - hitX| <= 8 && 0 <= localX < contentRight`。
   - 非最后一列：`hitX = columnX(c)+width`，仅当 `0 <= hitX < contentRight` 时参与命中。
3. **光标策略（复核定稿）**：
   - `HitTestColumnDivider` 为**纯查询，不改变光标**；`Begin/Update/End/Cancel` 也不隐式改变光标；
   - 新增显式 API `ListViewSetColumnResizeCursor(lv, active)`：`1` → `SystemCursorType::EW_Resize`（成为 owner），`0` → 若自身为当前 owner 则恢复默认（非 owner 为 no-op，不覆盖其它控件光标）；设计器在 hover 命中/离开、窗口失焦时调用；
   - 运行时自动管理：MouseMove 在表头分隔线命中 → EW，离开分隔线/内容区 → 默认；拖动期间保持 EW，结束/取消后按当前 hover 重新判定；
   - 光标为进程级全局状态：使用 **owner token 仲裁**（`Cursor::setCurrentWithOwner/restoreDefault`）；只有当前 EW owner 能恢复默认；非 owner 的 ListView/Splitter/控件发生 MouseLeave/FocusLost/隐藏/禁用/销毁时**不得覆盖**其它控件光标；多个 ListView/预览窗口由各自事件驱动，owner 随最后一次成功设置转移；
   - 引擎新增内部 `EventType::MouseLeave`（SDL3 映射 `SDL_EVENT_WINDOW_MOUSE_LEAVE`；缺失后端以 FocusLost/下一次 MouseMove 兜底），ListView 收到后恢复默认光标；
   - 工厂缺失（其他后端）→ 保持默认光标，不崩溃；SDL3/raylib/SFML 均支持 EW。
   - `SetColumnResizeCursor` 失败/owner 语义（联测修定稿）：`active` 非 0/1 → 返回 0；`active=1` 在隐藏/禁用/未 create/销毁中 → 返回 0 且不改变光标；**`active=0` 始终允许调用**——若自身为当前 EW owner 则恢复默认（即使控件隐藏/禁用/未 create），非 owner 为 no-op（返回 1，不覆盖其它 ListView/Splitter 光标）；句柄无效无法执行时返回 0；
   - 截图/人工验收记录路径写入测试记录。
4. **事件/API**：`column-resize-begin/changed/ended` 与既有 5 个 API 签名不变；新增第 6 个 `ListViewSetColumnResizeCursor`；`changed` 仅实际变宽时发，`ended` 恰好一次。新增光标 API 仅为设计态显式控制。
5. **不改运行时排序/选择/编辑语义**：命中分隔线优先拦截；表头正文仍排序；单元格双击编辑、行选择、滚轮不受影响。

### 7.3 光标生命周期表

| 触发 | 光标 |
|---|---|
| 运行时 MouseMove 命中分隔线 | EW |
| 运行时 MouseMove 离开分隔线/内容区 | 默认 |
| 拖动 begin/move | EW（保持） |
| 拖动 end/cancel | 按当前 hover 重新判定（默认/EW） |
| 设计态 `SetColumnResizeCursor(1/0)` | 1=设为 EW（成为 owner）；0=owner 时恢复默认，非 owner no-op |
| `HitTest/Begin/Update/End/Cancel` | **不隐式改变** |
| MouseLeave / FocusLost | 默认 |
| 隐藏/禁用/销毁 | 默认（销毁静默） |
| 多 ListView/预览窗口 | 仅当前鼠标所在控件可设置；离开/失焦恢复默认；后发生效 |

### 7.4 验收/测试补充

- `lvResource` 名称/路径列与最后一列均可拖宽；分隔线左右 ±8px 命中；
- 最后一列：部分可见可命中；完全滚出左/右不命中；右缘被裁剪时以可见右边界命中；
- hover 分隔线显示左右双箭头；离开/结束/隐藏/禁用/销毁/窗口失焦恢复默认，拖动中不闪烁；截图路径记录；
- 点击表头正文仍排序、不误触改宽；API 命中与运行时一致；`HitTest` 调用不改变光标；
- 自动化：`test_cr_cellcontrol` 覆盖 ±8px、最后一列可见/不可见/部分可见、`SetCursor` 成功/失败、owner 仲裁（非 owner 隐藏/失焦不影响当前 owner；owner 隐藏恢复默认；非 owner FocusLost 无影响）；`test_cr_cells`/`test_cr_ui` 保持既有；`HitTest` 不改光标；光标视觉项人工/截图验收（记录截图路径）；手册示例可运行。

### 7.5 待评审确认

- 命中半宽 8px；
- 最后一列交集/可见右边界规则；
- 新增 `ListViewSetColumnResizeCursor` 显式 API 及"HitTest 无光标副作用"；
- 内部 `EventType::MouseLeave` 的引入（不影响 C ABI）。

## 修订记录（补充）

- 2026-11-01 合并 CR20261101005：命中区 8px、EW 光标统一管理、最后一列边界、测试/手册计划。
- 2026-11-01 按设计复核修订：统一 8px（含文档/测试）；最后一列可见交集条件；HitTest 改为纯查询 + 新增显式光标 API；补光标生命周期/多实例/MouseLeave/FocusLost 与测试。
- 2026-11-01 按二次复核小修：统一 6 个 API 计数、补 SetColumnResizeCursor 失败语义、测试基线 53、光标安全调用测试项。
- 2026-11-01 按三次复核修订：`SetColumnResizeCursor(0)` 始终允许恢复默认光标；`active=1` 仅隐藏/禁用/未 create/销毁中失败；补对应测试。

## 实施记录（CR005 合并修订，2026-11-01）

- 命中半宽 8px（运行时/API 同函数）；最后一列可见交集+可见右边界命中；
- 惰性 EW 光标 + `ListViewSetColumnResizeCursor`；owner token 仲裁（仅 owner 恢复；Splitter 同接入）；运行时 hover/MouseLeave/FocusLost/隐藏/禁用/销毁仅 owner 恢复；SDL3 映射 `SDL_EVENT_WINDOW_MOUSE_LEAVE`；
- 测试 `test_cr_cells`/`test_cr_ui` + `test_cr_cellcontrol` 覆盖 8px/最后一列/光标 API；移植探针 `Temp/temp_cr4_probe.cpp`；全量回归 54 OK + 4 项预存失败。

## 8. 联测缺陷修复（2026-11-01）

- **现象**：设计器同时存在 `lvResource` 与事件页 ListView 时，hover 分隔线 EW 光标一闪而过；splitter 有时无 resize 光标但仍可拖动。
- **根因**：`ListView::setVisible(false)`/`setEnable(false)`、`~ListView`、FocusLost/MouseLeave 无条件恢复全局默认光标；隐藏的非当前 ListView 会把当前 owner 的 EW/NS 光标重置。
- **修复**：
  - `Cursor` 新增 owner 仲裁：`setCurrentWithOwner(cursor, owner)` / `restoreDefault(owner)` / `getCurrentOwner()`；`Cursor::setCurrent` 会清空 owner，仅保留给无 owner 的旧调用；
  - **所有 cursor 写入统一迁移**：ListView、Splitter、Label、Slider、HandleControl、WinFrame 全部改用 `setCurrentWithOwner`/`restoreDefault`；隐藏/禁用/失焦/离开/销毁只在自身为 owner 时恢复，非 owner no-op；
  - ListView/Splitter 的 EW/NS 光标使用 owner token（`getThis()`）；Slider 光标改为缓存复用（避免每次 MouseMove `createSystem`）；
  - 显式 `SetColumnResizeCursor(0)` 仅在自身为 owner 时恢复默认，非 owner no-op（不覆盖其它控件光标）。
- **测试**：`test_cr_cellcontrol` 新增 owner 仲裁用例：A 成为 owner 后隐藏/失焦非 owner B 不改变 owner；owner 隐藏恢复默认；非 owner FocusLost 不影响 owner；全量回归 + 全部探针通过。
- **回归**：54 OK + 4 项预存失败；全部探针通过。

## 验收闭环（2026-11-02）

- 设计器联测通过；CR 状态「设计器：closed」「引擎：closed」。
- 相关设计/复核文档按归档规则可归档（文件保留在 `design/` 与 `requirements/`，不另移动）。
