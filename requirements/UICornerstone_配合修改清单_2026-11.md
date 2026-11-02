# UICornerstone 配合修改清单（2026-11）

> **用途**：设计器 → 引擎的当月新需求/缺陷提交清单。
> **归档规则（2026-11-01 起）**：每月新需求单独成档，命名 `UICornerstone_配合修改清单_<YYYY-MM>.md`。
> **编号规则**：`CR<YYYYMMDD><NNN>`，当日序号从 `001` 开始，最大 `999`；例：`CR20261101001`。
> **状态规则**：提交后设计器/引擎两项均保持 `open`；**设计器只能修改「设计器」项、引擎只能修改「引擎」项**，各自验证通过后才置 `closed`，不得代改对方项。
> **历史**：P0-108/P0-109/P0-110 交付记录见 `Temp/UICornerstone_配合修改清单_第三批.md`。
> **详细设计**：ListView/TreeView 就地编辑与列宽拖拽手势见 `Temp/引擎ListView_TreeView编辑模式_设计建议.md`。
> **编号映射**：原 P0-111 → `CR20261101001`，原 P0-112 → `CR20261101002`，原 P0-113 → `CR20261101003`。

## CR20261101001 ListView 单元格双击编辑——引擎清单

### 现状
ListView 单元格仅文本渲染，没有提供控件内编辑的机制，宿主可能需自行在单元格屏幕位置叠 EditBox 来模拟控件内编辑；滚动、列宽变化、行重排或数据刷新时容易错位或残留。

### CR类型：requirement

### 修订记录

#### 2026-11-01 初次提交CR

**建议引擎提供**：①属性/API `editable-columns`（列索引集合或 bool）；②双击单元格 → 引擎在单元格 rect 上创建内部 EditBox（覆盖层，非单元格子控件），预填当前文本；③提交：Enter/失焦（可配置）；取消：Esc；④滚动/列宽变化/行重排/数据刷新时立即提交或取消并销毁覆盖层（避免错位 ✗）；⑤事件 `cell-edit-begin`/`cell-edited`/`cell-edit-cancel`（载荷 row-id、column、text）；⑥可配置只读列/校验回调（可选）。**验收**：双击编辑、Enter/失焦提交、Esc 取消、滚动不残留、事件 payload 正确、C ABI/binding 可用 ✓。设计器事件页为自定义行，不依赖此项；通用 ListView 单元格编辑接入引擎 ✓。

#### 2026-11-01 补充建议：属性、事件前后值、API 与生命周期

**属性/API**：
- `cell-editing`（bool，默认 false）+ `editable-columns`（逗号串；空=全部）；
- 双击可编辑单元格 → 覆盖 EditBox，预填当前文本；
- Enter/失焦提交、Esc 取消；
- 滚动、列宽变化、行增删移、排序、清空、刷新 → **取消并销毁**；
- 程序化 `BeginEditCell/CommitEdit/CancelEdit/IsEditing`。

**事件**（载荷复用 `GridPayload{row,col}`）：
- `cell-edit-begin`：仅 `oldText`；
- `cell-edited`：`oldText` + `newText`；
- `cell-edit-cancel`：仅 `oldText`。

**前后值双通道**：
1. `GridPayload` 追加 `oldText/newText`（C 字符串，仅回调期间有效，宿主须拷贝；绑定层加 `GetEditOldText/GetEditNewText`）；
2. 只读属性 `edit-old-text`/`edit-new-text`（begin/cancel 时 `edit-new-text` 为空）。

**验收**：双击编辑；Enter/失焦提交；Esc 取消；滚动/刷新不残留；begin/cancel 仅 old、edited old+new；C ABI/binding/schema/文档同步；版本不匹配时功能禁用/安全失败（不做回退）。

**待引擎确认**：Q1 全选策略、Q2 滚动取消、Q4 双击/单击/拖拽顺序、Q5 校验钩子、Q6 Tab/方向键、Q7 部分可见、Q8 前后值载荷形式。**Q3 只读语义已确认（见下）。**

**详细设计**：`Temp/引擎ListView_TreeView编辑模式_设计建议.md`。

#### 2026-11-01 只读语义确认（用户要求）
- ListView 只读行/列仍可进入编辑；覆盖 EditBox 只读（可选中/复制，不可改文本）。
- 命中只读项仍发 `cell-edit-begin`（`readOnly=1`）；Enter/失焦统一按 `cell-edit-cancel`（不写回、无 `cell-edited`）；Esc/生命周期取消同样 cancel。
- 新增 `read-only-columns`/`read-only-row-ids`；schema/文档/C ABI/Binding 同步。
- 该项为用户确认需求，CR 状态维持 open，待引擎实现后验证。

#### 2026-11-01 设计复核：修订稿通过（设计层面）
- B1/B2/B3 已闭环；R2 版本配套政策已由设计器同步；R4 Tab 语义确认。
- Tab=失焦提交 + Bench 焦点切换；只读语义已由用户确认。
- 版本政策：DLL/Binding/宿主同批配套；旧 DLL/旧宿主不混用；功能不可用时提示/禁用，不做回退。
- 状态维持 open，待引擎实施与双方验证。

### CR状态
#### 设计器：closed
#### 引擎：closed

## CR20261101002 TreeView 节点标签双击编辑——引擎清单

### 现状
TreeView 节点标签仅文本渲染，没有提供控件内编辑的机制，宿主无法就地编辑；滚动、展开/折叠、节点重排或数据刷新时自行叠加 EditBox 容易错位或残留。

### CR类型：requirement

### 修订记录

#### 2026-11-01 初次提交CR

与 CR20261101001 同类：①属性/API `editable-labels`；②双击节点标签 → 引擎在节点文本位置创建内部 EditBox 覆盖层（预填标签）；③Enter/失焦提交，Esc 取消；④滚动/展开/折叠/节点重排/数据刷新时提交或取消并销毁覆盖层；⑤事件 `node-edit-begin`/`node-edited`/`node-edit-cancel`（载荷 node-id、text）；⑥可选只读/校验。**验收**：双击编辑、Enter/失焦提交、Esc 取消、滚动/展开不残留、事件 payload 正确、C ABI/binding 可用 ✓。

#### 2026-11-01 补充建议：属性、事件前后值、API 与生命周期

**属性/API**：
- `label-editing`（bool，默认 false）；
- 双击节点标签 → 覆盖 EditBox，预填当前 label；
- Enter/失焦提交、Esc 取消；
- 滚动、展开/折叠、节点增删移、刷新 → **取消并销毁**；
- 程序化 `BeginEditNode/CommitEdit/CancelEdit/IsEditing`。

**事件**（载荷复用 `TreeNodePayload{id}`）：
- `node-edit-begin`：仅 `oldText`；
- `node-edited`：`oldText` + `newText`；
- `node-edit-cancel`：仅 `oldText`。

**前后值双通道**：
1. `TreeNodePayload` 追加 `oldText/newText`（C 字符串，仅回调期间有效，宿主须拷贝；绑定层加 `GetEditOldText/GetEditNewText`）；
2. 只读属性 `edit-old-text`/`edit-new-text`（begin/cancel 时 `edit-new-text` 为空）。

**验收**：双击编辑；Enter/失焦提交；Esc 取消；滚动/展开/刷新不残留；begin/cancel 仅 old、edited old+new；C ABI/binding/schema/文档同步；版本不匹配时功能禁用/安全失败（不做回退）。

**待引擎确认**：与 CR20261101001 的 Q1/Q2/Q4~Q8 同口径；Q3 只读语义已确认（见下）；补充节点展开/折叠期间编辑的取消时机。

**详细设计**：`Temp/引擎ListView_TreeView编辑模式_设计建议.md`。

#### 2026-11-01 只读语义确认（用户要求）
- TreeView 只读节点仍可进入编辑；覆盖 EditBox 只读（可选中/复制，不可改 label）。
- 命中只读节点仍发 `node-edit-begin`（`readOnly=1`）；Enter/失焦统一按 `node-edit-cancel`（不写回、无 `node-edited`）；Esc/生命周期取消同样 cancel。
- 新增 `read-only-ids`；schema/文档/C ABI/Binding 同步。
- 该项为用户确认需求，CR 状态维持 open，待引擎实现后验证。

#### 2026-11-01 设计复核：修订稿通过（设计层面）
- B1/B2/B3 已闭环；R3 空 id 语义、R6 横向完整可见已定稿；R2 版本配套政策已由设计器同步。
- Tab=失焦提交 + Bench 焦点切换；只读语义已由用户确认。
- 版本政策：DLL/Binding/宿主同批配套；旧 DLL/旧宿主不混用；功能不可用时提示/禁用，不做回退。
- 状态维持 open，待引擎实施与双方验证。

### CR状态
#### 设计器：closed
#### 引擎：closed

## CR20261101003 ListView 列宽拖拽（设计态 API + 运行时体验）——引擎清单

### 现状
ListView multi 视图运行时已支持拖拽列头分隔线调整列宽（`setColumnWidth` + `min-column-width` 钳制）；但设计画布拦截鼠标，运行时拖拽在设计态不可用。用户要求由引擎拥有设计态手势，设计器不重复命中/宽度计算。

### CR类型：requirement

### 修订记录

#### 2026-11-01 初次提交CR

**建议引擎提供**：
- `int ListViewHitTestColumnDivider(instance, lv, float localX, float localY)`：multi 表头区、4px 半宽；返回列索引或 -1；与运行时同一判定；
- `bool ListViewBeginColumnResize(instance, lv, int col, float localX)`：记录 oldWidth；
- `float ListViewUpdateColumnResize(instance, lv, float localX)`：引擎计算/钳制/relayout，返回当前宽度；
- `bool ListViewEndColumnResize(instance, lv, int commit)`：commit=1 保留；0 恢复 oldWidth；
- `bool ListViewCancelColumnResize(instance, lv)`：恢复 oldWidth。

#### 2026-11-01 补充建议：事件、约束与验收

**事件**：`column-resize-begin/changed/ended`；`begin` 仅 `col/oldWidth`，`changed/ended` 带 `col/oldWidth/newWidth`，`ended` 另带 `canceled`。设计器在 `ended` 同步一次撤销。

**约束**：
- 设计态只处理表头分隔线，不触发行选择/排序/单元格编辑；
- 宽度写回/钳制/relayout 由引擎；设计器只持久化 model、刷新属性面板、记一次撤销；
- 与预览运行时宽度/钳制/视觉一致；取消恢复原宽；
- 版本不匹配（缺入口）→ Binding 返回 -1/false，设计器提示/禁用功能，不回退属性面板。

**验收**：设计画布拖拽列头分隔线与运行时一致；取消恢复；不产生选择/排序副作用；头文件/schema/文档/binding 同步；Auto 测试覆盖命中、begin/update/end、cancel、min clamp。

#### 2026-11-01 设计复核：修订稿通过（设计层面）
- B1/B2/B3 已闭环；R1 内部写宽路径、R5 隐藏/禁用/未 create 前置校验已定稿；R2 版本配套政策已由设计器同步；R4 A 方案确认。
- A 方案=引擎拥有命中/宽度/session，设计器转发指针并持久化模型。
- 版本政策：DLL/Binding/宿主同批配套；旧 DLL/旧宿主不混用；功能不可用时提示/禁用，不做回退。
- 状态维持 open，待引擎实施与双方验证。

#### 2026-11-01 合并 CR20261101005：运行时列宽拖拽可发现性

原 CR20261101005 与 CR003 同属 ListView 列宽拖拽，现合并为 CR003 的运行时体验修订，CR005 不再单列。

**现状**：运行时及 `ListViewHitTestColumnDivider` 只认列头分隔线 ±4px，且无 resize 光标；`lvResource` 宽 240，最后列分隔线在右边界/滚动条附近，拖动表头正文会排序而不是改宽，用户表现为“无法拖动列宽”。

**补充要求**：
- 命中区从 ±4px 扩大到 **±6~8px**（仅表头；分隔线优先于排序）；
- 分隔线命中区 hover 时**必须**使用 `SystemCursorType::EW_Resize`（左右双箭头）；拖动期间保持，离开/结束/隐藏/禁用/销毁恢复默认；
- 光标由引擎统一管理，设计器/宿主不自行设置；复用引擎 Cursor 工厂，避免每帧创建/销毁；
- 最后一列分隔线在右边界/滚动条附近仍可命中（必要时保留内缩可拖区）；
- 运行时与 `ListViewHitTestColumnDivider`/CR003 设计态 API 使用同一命中区与光标规则；表头正文排序不受影响。

**验收补充**：
- `lvResource` 名称/路径列及最后一列均可拖宽；
- hover 分隔线显示左右双箭头，离开/结束恢复，拖动中不闪烁；
- 不误触发表头排序；API 与运行时命中一致；
- 回归：表头正文排序、滚轮、行选择、单元格编辑不受影响。

**待引擎确认**：命中半宽 6/8px、最后一列/滚动条边缘方案、SDL3 cursor 工厂（其他后端缺失时保留默认并在文档注明）。

#### 2026-11-01 设计复核：修订稿暂不通过
- 8px 与最后一列方向认可；但文档仍混用 4px/8px，最后一列完全不可见时缺少交集判断。
- `HitTestColumnDivider` 是否改变光标在 §7.2/§7.3 矛盾；需明确副作用/生命周期/窗口失焦恢复。
- 详见 `Temp/CR20261101003-004_设计复核_2026-11-01.md`。

#### 2026-11-01 二次复核：CR003 通过（文档小修）
- 8px、最后一列交集、HitTest 纯查询 + 显式 `SetColumnResizeCursor`、光标生命周期均已定稿；
- 小修：文档 API 计数 5 改为 6、`SetColumnResizeCursor` 失败语义、测试基线统一；不影响实施。

#### 2026-11-01 三次复核：CR003 接近通过（光标恢复语义待修）
- 8px、最后一列、纯查询 HitTest、显式光标 API、文档/测试已修；
- 阻塞：`SetColumnResizeCursor(0)` 在隐藏/禁用时被拒绝且不恢复默认，会导致 EW 光标卡死；`active=0` 应始终恢复默认，仅 `active=1` 受隐藏/禁用约束。

#### 2026-11-01 四次复核：设计通过
- `SetColumnResizeCursor(0)` 隐藏/禁用也恢复默认；`active=1` 才受隐藏/禁用/未 create 约束；
- 8px、最后一列、纯查询 HitTest、光标生命周期、文档/测试均通过，可实施。

#### 2026-11-01 联测缺陷：全局 EW 光标被非当前 ListView 重置
- 现象：设计器同时存在 `lvResource` 与事件页 ListView；hover 分隔线时 EW 光标一闪而过，拖动不稳定。
- 根因（引擎）：`ListView::setVisible(false)`/`setEnable(false)` 无条件调用 `setColumnResizeCursor(false)`；隐藏的另一 ListView（如未激活 Tab 页）会把当前 ListView 的全局 EW 光标恢复默认。`FocusLost/MouseLeave` 也存在同类“非 owner 重置”。
- 要求：只有当前 EW owner（`m_overDivider`/`m_dragCol>=0`/`m_apiResizeActive` 等等价状态）在隐藏/禁用/失焦/离开/销毁时才恢复默认；非 owner 的隐藏/销毁 ListView 不得触碰全局光标。`~ListView` 同样不得无条件 `Cursor::setCurrent(default)`。
- 联测补充：splitter 有时无 resize 光标但仍可拖动，同一根因——其他 ListView 隐藏/销毁把全局光标重置；需由引擎统一 owner 仲裁。
- **owner 仲裁仍不完整（2026-11-02 复核）**：`Cursor::setCurrent` 会把 `g_currentOwner` 清空；`Label`、`Slider`、`HandleControl`、`WinFrame` 仍在用 `Cursor::setCurrent`（含 createSystem），即使 ListView/Splitter 已改用 owner API，也会被这些控件的 MouseMove/MouseLeave 清掉。要求所有 cursor 写入统一走 `setCurrentWithOwner`/`restoreDefault`，或引入全局 hover-cursor 仲裁；否则 splitter/ListView 光标仍会闪/丢。
- 验收：两个 ListView，A hover 分隔线显示 EW；隐藏/切换 B 时 EW 保持；离开 A 恢复；A 隐藏/禁用恢复；非 owner FocusLost/MouseLeave 不影响。
- 状态：CR003 保持 open；此条为实现缺陷修订，不另开 CR。

#### 2026-11-02 引擎交付：全控件 cursor owner 迁移
- 新版引擎已把 Label/Slider/HandleControl/WinFrame 等 cursor 写入统一迁移到 `setCurrentWithOwner`/`restoreDefault`；非 owner 控件不再清空全局 owner。
- 交付验证：Debug/Release 构建通过；`auto=2` 272/0；`auto=3` 通过；`strict` PASS。
- 设计器侧 `lvResource` 可拖、事件页操作按钮已接；cursor/splitter 待用户人工复测。

### CR状态
#### 设计器：closed
#### 引擎：closed

## CR20261101004 ListView 通用单元格控件（cell control）——引擎清单

### 现状
ListView 内部已有稀疏的 `cellControls` 与 leadingControl 定位机制，但 C ABI/Binding 只暴露图标槽定位（`ListViewSetCellLeadingControl`），没有通用的单元格控件 API。设计器事件面板目前只能使用自定义 Panel 行；每行“代码/删除”按钮无法放入 ListView 单元格，导致事件面板无法迁移到 ListView。

### CR类型：requirement

### 修订记录

#### 2026-11-01 初次提交CR

**建议引擎提供（通用 per-cell control，而非 action-column 专用）**：
- `ListViewSetCellControl(instance, lv, row, col, UIControlHandle ctl, int layout)`；
- `ListViewClearCellControl(instance, lv, row, col)`；
- `ListViewGetCellControl(instance, lv, row, col)`（可选，便于宿主判断）；
- `layout` 至少支持 `FILL`（单元格内容区，按 cell padding 内缩）和 `LEADING`（现有图标槽）；默认 `FILL`；
- 每个单元格一个控件；复合操作可用一个 Panel 内含多个按钮，避免引擎做 action-column 特例；
- 所有权：调用后引擎接管控件（挂为 ListView 子控件）；宿主不得再 Destroy；Clear/行删除/列删除/清空/ListView 销毁时由引擎移除并销毁；
- 生命周期：滚动、列宽变化、行排序/移动/增删、列增删、隐藏/禁用/销毁时保持定位正确或安全移除；不残留、不越界；
- 事件/焦点：单元格控件消费自己的鼠标/键盘事件；未消费时 ListView 行选择/`item-click` 照旧；单元格控件可聚焦，Tab/焦点规则明确；
- 本期运行时 API 即可，不强制新增 JSON 声明；后续可按需补 JSON。

**验收**：
- 设计器事件面板可用 ListView 呈现“控件/事件/handler/操作”四列；操作列放入一个含“代码/删除”按钮的 Panel，绑定增删、滚动、行列变化、复制/查看均正常；
- 按钮点击命中正确；不触发行选择或触发可预期；无悬垂句柄/泄漏；
- FILL 布局覆盖整个单元格内容区；LEADING 保持现有图标槽行为；
- C ABI/Binding 同步，测试覆盖行/列变更、滚动、销毁、焦点/点击。

**待引擎确认**：
- 所有权/销毁语义（引擎接管 vs 宿主保留）；
- FILL 的 padding/对齐规则；
- 行排序/移动时 cell control 跟随策略；
- 是否同时适用于 TreeView（后续可复用同一 cell-control 抽象）；
- 是否需要“多个控件/单元格”或 JSON 声明（当前建议先单控件 + Panel 组合）。

#### 2026-11-01 设计复核：初稿暂不通过
- 通用 cell-control/引擎所有权/FILL 2/1/仅完整可见方向认可。
- 需补齐：Binding 代理在 Set/Clear/Get/替换/删除后的生命周期；已有父容器/焦点清理；MouseDown/Up/Wheel/双击/拖拽的精确派发顺序；完整可见边界与编辑屏蔽；对应测试。
- 详见 `Temp/CR20261101003-004_设计复核_2026-11-01.md`。

#### 2026-11-01 二次复核：CR004 暂不通过
- 阻塞：§4.5 误述“父到子”分发；源码 `ControlImpl::handleEvent` 实际子控件先，按文档会在 ListView 内双重派发；
- 阻塞：Binding 代理 `IsValid()==false` 无法由 shared_ptr 自动保证，且与 Clear“只摘树”矛盾；
- 文档末尾有孤立的三引号、缺修订记录标题；
- 详见 `Temp/CR20261101003-004_复核2_2026-11-01.md`。

#### 2026-11-01 三次复核：CR004 暂不通过（事件捕获）
- 借用语义、FILL 2/1、仅完整可见、文档格式已通过；
- 阻塞：`ControlImpl::handleEvent` 只返回 bool，ListView 无法获知哪个子控件消费，无法记录 captured；
- 阻塞：§4.5 先调 `ControlImpl::handleEvent` 与 MouseUp/Move “captured 优先”矛盾，需定稿 captured 先派发、再其它子控件/自身，并防双重派发；
- 补 captured 移出/隐藏/删除测试；详见 `Temp/CR20261101003-004_复核3_2026-11-01.md`。

#### 2026-11-01 四次复核：设计通过
- `dispatchToChild` + captured 机制解决消费子控件识别与同/异指针 MouseUp/Move 顺序；
- 借用语义、FILL 2/1、仅完整可见、LEADING 兼容、FILL 编辑屏蔽、测试均通过；
- 实施注意：ListView 不再二次调用 `ControlImpl::handleEvent`，Panel 真正接入 helper，所有移除/隐藏/失焦清理 captured。

### CR状态
#### 设计器：closed
#### 引擎：closed
