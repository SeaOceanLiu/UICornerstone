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

## CR20261102001 主题样式表（CSS-like style sheet / 类与角色）——引擎清单

### 现状
引擎 `theme` 仅支持按**控件类型**的 `fonts` / `colors`（button/label/…），不支持“确认按钮/取消按钮”这类**语义样式组**；控件无法声明所属角色，导出后也不能运行时切换角色风格。设计器当前只有 JSON 编辑，没有主题 UI，也不能把一组控件批量分配到同一角色。

### CR类型：requirement

### 修订记录

#### 2026-11-02 初次提交CR

**目标**：支持用户定义任意命名的主题角色/样式组，并把控件（可按选择集批量）分配到角色；导出后仍可按主题变体切换角色风格。角色覆盖：字体、颜色四态（文字/背景/边框/文字阴影）、阴影（颜色/偏移/半径）、圆角、内边距/间距等常见视觉属性。

**建议模型**：
```json
"theme": {
  "roles": {
    "confirm": {
      "base": "button",
      "font": { "name": "HarmonyOS_Sans_SC_Regular", "size": 18, "style": "normal" },
      "colors": {
        "background": { "normal": "#2D7FF9FF", "hover": "#4A90D9FF", "pressed": "#1F6FE0FF", "disabled": "#3A3A3AFF" },
        "text": { "normal": "#FFFFFFFF" },
        "border": { "normal": "#2D7FF9FF" }
      },
      "shadow": { "color": "#00000080", "offset-x": 0, "offset-y": 2, "radius": 6 },
      "corner-radius": 6,
      "padding": { "left": 12, "top": 6, "right": 12, "bottom": 6 }
    },
    "cancel": { "...": "..." }
  }
}
```
- 控件声明 `"theme-role": "confirm"`；角色可按控件类型限定（`base`）或跨类型通用；
- 优先级：**控件显式属性 > theme-role > 控件类型默认 > 全局 theme 默认 > 引擎内置默认**；
- 角色属性只覆盖其声明的键；未声明键继续回退；
- 角色可继承（`base-role`，可选），并支持主题变体（见下）。

**运行时切换**：
- 支持主题变体：`theme.variants` 下按变体覆盖角色样式；提供 C ABI/Binding：`SetThemeVariant(instance, name)` / `GetThemeVariant` / `ReloadTheme`（或等价 API）；
- 切换时只刷新角色派生属性，保留控件显式覆盖；重新应用不应触发布局抖动/焦点丢失；
- 若引擎更倾向直接 API `SetRoleStyle`/`ApplyTheme`，请在设计中定稿；但必须支持导出后运行时切换组风格。

**设计器 UI（配套）**：
- 主题管理器：角色列表（新增/复制/删除/重命名）、base 类型、字体、颜色四态、阴影/圆角/间距表单；
- 选中控件属性面板增加“主题角色”下拉（多选批量赋值），并可清空；
- 主题变体切换下拉；预览/导出保持一致。

**验收**：
- 定义 confirm/cancel 两组，分别赋给多个按钮；字体/四态色/阴影/圆角/间距正确；
- 运行时切换主题变体，两类按钮同步变化，显式覆盖的控件不受影响；
- 保存/打开/导出/导入/预览全链；非法角色/变体/属性失败语义；
- C ABI/Binding/schema/文档/测试同步。

**待引擎确认**：
- 角色是否按控件类型限定；跨类型角色的属性适用范围；
- 变体 vs 直接 SetRoleStyle 的 API 形态；
- 显式覆盖与角色派生属性的判定方式（建议稀疏覆盖位）；
- 圆角/间距的引擎支持范围（按钮/复选框/面板等）；
- 角色继承与循环检测。

#### 2026-11-02 设计方向调整：CSS-like 样式表

语义角色不足以表达“确认/取消”等组合与状态；改为 **CSS-like 样式表 + 类选择器**，底层可层叠，设计器 UI 提供可视化规则编辑与类分配。

**建议 JSON（引擎解析，设计器/导出一致）**：
```json
"theme": {
  "variables": { "--primary": "#2D7FF9FF", "--danger": "#E06060FF" },
  "styles": [
    { "selector": "button", "declarations": { "font-size": 16 } },
    { "selector": ".confirm",
      "declarations": { "background": "var(--primary)", "font-size": 18,
                        "border-radius": 6, "padding": "6 12" } },
    { "selector": ".confirm:hover", "declarations": { "background": "#4A90D9FF" } },
    { "selector": ".cancel", "declarations": { "background": "#3A3A3AFF" } },
    { "selector": "#btnOk:disabled", "declarations": { "background": "#3A3A3AFF" } }
  ]
}
```
- 控件声明 `"classes": ["confirm"]`（或 `"style-class"` 字符串/数组）；保留 `theme-role` 作为 `classes` 的兼容别名。
- 选择器子集（本批）：类型、`.class`、`#id`、复合、状态 `:hover/:pressed/:disabled/:selected`；后代/子选择器、属性选择器、`!important` 暂不做（后续另议）。
- 声明属性：`font-family/font-size/font-style`、`text-color/background/border-color/text-shadow`（含状态由伪类表达）、`border-radius`、`padding`、`shadow-offset-x/y`、`opacity` 等；映射到引擎既有属性。
- 变量：`variables` + `var(--name)`（仅颜色/数值/长度，暂不做 calc）；未知变量→回退/警告。
- 层叠与优先级：控件显式属性（inline） > #id > .class > 类型；同优先级按 source order；状态规则高于无状态同选择器；字体继承沿用引擎父链。
- 运行时：`SetThemeVariant`/`ApplyTheme` 重新解析/重算样式，仅刷新样式派生属性，保留控件显式覆盖；提供 C ABI + Binding。
- 设计器 UI：主题管理器提供规则列表（选择器 + 可视化属性表单：颜色四态、字体、圆角、间距、阴影），类分配给选中控件（多选批量），变量/变体管理；高级用户可切到 CSS-like 文本编辑。**不要求用户手写 CSS**。

**验收**：`.confirm`/`.cancel` 赋给不同按钮；`:hover/:disabled` 正确；变量改为新主色后两类组同步；运行时切换变体；显式覆盖不被覆盖；保存/打开/导出/导入/预览全链。

**待引擎确认**：选择器子集、状态优先级、变量/长度语法、类字段名、变体/重载 API、与显式覆盖的判定。

#### 2026-11-02 设计评审：暂不通过
- B1 `ThemeStyleState` 把所有 set* 当显式会误锁内部/主题派生；需区分 JSON/公开 API/主题/内部同步来源。
- B2 公共控件与内部辅助控件无判定机制；需 theme-eligible/internal 标记，避免误染。
- B3 阴影仅 text-shadow/offset，未见 box shadow 颜色/半径；需收窄或补实现。
- B4 padding 仅 Label/Button/CheckBox；需确认是否满足“间距”需求。
- 版本门禁 CR 与设计冲突；还需 classes 变更/继承/显式覆盖测试。
- 详见 `Temp/CR20261102001_设计评审_2026-11-02.md`。

#### 2026-11-02 版本门禁撤销（用户确认）
- 当前未正式发布，V1 直接支持 CSS-like 新语法；取消 `theme.version`/`theme.features` 要求，旧主题无需版本字段。
- 旧主题/旧布局行为不变；旧引擎 + 新主题仍不承诺（同批配套），但引擎内部对未知字段忽略并诊断。
- 评审中“版本门禁冲突”关闭，不作为阻塞项。

#### 2026-11-02 复核：修订稿有条件通过
- B1 显式来源三分 + ClearThemeOverrides、B2 theme-eligible、B3 box-shadow、B4 padding（Label/Button/CheckBox）通过；V1 无版本门禁确认。
- 实施前补 R1：ApplyTheme 替换主题后必须按新主题重建 baseline，再应用 CSS；SetThemeVariant 同理。
- 补 theme-eligible 内部子树/用户 cell/leading 边界、非支持 padding 诊断、classes/box-shadow 回归测试。
- 详见 `Temp/CR20261102001_复核_2026-11-02.md`。

#### 2026-11-02 设计复核：通过（文档小修）
- R1 baseline、显式来源、theme-eligible、box-shadow、CR2002 padding 委托、V1 无版本门禁均通过。
- 小修：测试 5/8d 的旧 “padding 仅支持控件” 改为 CR2002 交付后通用；§10 Q4 标注已转 CR2002。

#### 2026-11-02 二次复核：通过（文档小修）
- content-padding 基座/委托/测试已对齐；仅需把 `ThemeStyleState` 的 `padding` 位改名为 `content-padding` 位，§10 Q4 标为已确认。

### CR状态
#### 设计器：closed
#### 引擎：closed（引擎实施完成；2026-11-11 用户确认）
#### 2026-11-02 兼容性补充：增量可选、版本标记、旧引擎行为

- **旧布局/旧主题不受影响**：未使用 `classes`/`.class`/后代/属性选择器/`!important` 的现有布局，在新引擎中解析与视觉必须保持现状；类型/类/ID/状态与显式属性的既有优先级不变。
- **新能力全部 opt-in**：没有对应规则/类时零影响；后代选择器、属性选择器、`!important` 仅在被使用时参与匹配。
- **已发布应用**：继续使用原引擎/资源即可；只有新项目使用这些语法时才要求配套新版引擎（沿用“DLL/Binding/宿主同批”政策）。
- **旧引擎 + 新主题**：视为版本不匹配；旧引擎应忽略未知规则/字段并给出诊断，**不崩溃、不覆盖控件显式样式**；不承诺旧引擎支持新选择器。
- **无版本门禁（2026-11-02 用户确认）**：当前尚未正式发布，V1 直接支持 CSS-like 语法；不要求 `theme.version`/`theme.features`。旧主题仍按 fonts/colors 解析；若未来出现版本字段，仅作为前向元数据忽略。
- **新控件字段**（如 `classes`）只在新项目使用；旧布局不产生该字段。新主题/新布局在旧 schema 下可能严格校验失败，属于版本不匹配，需升级引擎/schema。
- **验收补充**：旧主题回归（无新语法→视觉/显式覆盖不变）；旧引擎加载新主题不崩溃并告警；新引擎加载 v1 主题行为不变；含后代/属性/`!important` 的主题仅在声明版本/特性时启用。
- **实现约束**：`!important` 不得改变默认层叠；仅作为显式 opt-in 覆盖；新增选择器必须保证同优先级 source order 与状态优先级不漂移。


## CR20261102002 通用控件内边距（generic padding）——引擎清单

### 现状
引擎目前只有**容器布局 padding**（HFlow/VFlow/Grid/Anchor 的 Margin）和 Dialog/ColorPicker 的局部 padding；没有面向所有控件的通用内容内边距。CR20261102001 本批仅支持 Label/Button/CheckBox 的文本内缩，Panel/List/Tree/Tab/EditBox/ComboBox/NumericUpDown 等无法统一设置 `padding`。

### CR类型：requirement

### 修订记录

#### 2026-11-02 初次提交CR

**目标**：为全部公开控件提供通用 `padding`（Margin：left/top/right/bottom），使主题 CSS `padding`、控件 JSON `padding`、C ABI/Binding `SetPadding` 使用同一语义。

**建议语义**：
- `ControlImpl` 新增通用 `padding` 属性（默认 0，旧布局零变化）；
- **contentRect = drawRect 内缩 padding（并扣除 border，如适用）**；控件文本/图像/内容按 contentRect 排布或裁剪；
- **容器类**：padding 作为布局子控件的内边距；与现有 `layout.padding` 统一/避免叠加（建议控件级 padding 为外层，layout.padding 为内层，或明确单一来源）；
- **文本类**：Label/Button/CheckBox/EditBox/TextArea/ComboBox/NumericUpDown 等文本在 contentRect 内排布；
- **ListView/TreeView**：padding 作用于整体内容区（含表头/行/节点区域），行/列内部 padding 不作为本批；
- **TabControl**：作用于页内容区；页签条 padding 另议；
- **Image/Actor/Shape**：默认不改绘制内容；如需内缩需明确裁剪/缩放语义；
- **裁剪/命中**：内容裁剪到 contentRect，控件自身命中范围不变；子控件/滚动条不得越界；
- **最小尺寸**：padding 不得超过控件尺寸；异步/运行时设置时钳制，必要时诊断；
- **CSS-like 主题**：`padding: 6` / `"6 12"` / `"t r b l"` 映射到通用属性（CR20261102001 的 padding 限制在新 CR 交付后解除）；
- **优先级**：控件显式 padding > 主题规则；状态伪类不适用 padding。

**API/JSON**：
- JSON `"padding": {"left":..., "top":..., "right":..., "bottom":...}` 或数字/字符串简写；
- C ABI `SetPadding/GetPadding`（或通用 SetString 解析）；Binding 同步；
- schema/文档/测试同步。

**验收**：
- 对 Button/Label/EditBox/ComboBox/NumericUpDown/Panel/ListView/TreeView/TabControl 等分别设置 padding，文本/内容/子控件位置正确；
- 旧布局默认 0 无回归；padding 过大/负值失败或钳制；
- 主题 CSS 与显式属性一致；运行时修改不破布局/焦点；
- 容器 padding 与 layout.padding 不重复叠加；滚动/裁剪/命中正常。

**待引擎确认**：
- 控件级 padding 与现有布局 padding 的合成顺序；
- ListView/TreeView 是整体内容区还是行/单元格级；
- Image/Actor/Shape 是否支持；
- 负值/超尺寸策略；
- 是否与 border 盒模型（content-box/border-box）对齐。

#### 2026-11-02 设计评审：暂不通过
- 阻塞：通用 `padding`（四边/对象）与既有 Dialog `padding`（float、kPadding）同键冲突；schema/SetFloat/SetString/旧 JSON 兼容均未定稿。
- 建议改用 `content-padding`（CSS 仍映射 padding），或 Dialog `dialog-padding` + 兼容别名；需补绝对布局 contentRect、Label margin 叠加、ListView/TreeView hit padding 回归。
- 详见 `Temp/CR20261102001-003_设计复核_2026-11-02.md`。

#### 2026-11-02 二次复核：通过（文档小修）
- `content-padding` 与 Dialog 旧 `padding` 隔离，旧行为零变化；Q9 双 padding 并存语义明确。
- 小修：“Dashboard 新键”改为通用新键；补 Dialog 双 padding 回归；API 名称固定 SetContentPadding/GetContentPadding。

### CR状态
#### 设计器：closed
#### 引擎：closed（引擎实施完成；2026-11-11 用户确认）
- 实施落地：通用键 `content-padding`（Dialog 旧 `padding` 不变）；JSON number/string/object；C ABI/Binding `SetContentPadding/GetContentPadding`；CSS `padding` 映射；文档 `docs/theme.html`。

## CR20261102003 响应式主题（media queries / 响应式变量）——引擎清单

### 现状
CSS-like 主题目前是静态样式表：规则随主题变体切换，但不会根据窗口尺寸、方向、DPI/缩放等条件重新匹配；没有 `@media` / media condition 语法，也没有“主题随窗口变化自动重算”的机制。设计器预览/导出应用在不同窗口尺寸下无法使用同一套响应式主题。

### CR类型：requirement

### 修订记录

#### 2026-11-02 初次提交CR

**目标**：主题支持响应式条件（媒体查询），在窗口尺寸/方向/缩放变化时自动重新计算并应用样式；覆盖主题变量、样式规则和主题变体，且不覆盖控件显式属性。

**建议语法（V1）**：
```json
"theme": {
  "variables": { "--gap": 8 },
  "styles": [
    { "selector": ".card", "declarations": { "padding": "var(--gap)" } }
  ],
  "media": [
    {
      "query": "(min-width: 800px) and (max-width: 1199px)",
      "variables": { "--gap": 12 },
      "styles": [
        { "selector": ".card", "declarations": { "padding": "var(--gap)" } }
      ]
    },
    {
      "query": "(orientation: landscape)",
      "styles": [ { "selector": ".toolbar", "declarations": { "font-size": 14 } } ]
    }
  ]
}
```
- V1 支持媒体特性：`min-width/max-width/min-height/max-height`、`orientation: portrait|landscape`、`min-scale/max-scale`（或 `min-resolution`，映射实例 DPI/视口缩放）；`and` 组合；`not`/逗号 `or` 暂不做；
- 媒体条件基于**实际视口/窗口像素**（预览/导出应用），不是设计画布逻辑尺寸；设计器需明确设计时预览使用哪个尺寸；
- 媒体块的 `variables` 在基变量上覆盖；`styles` 追加到基样式之后；媒体匹配时按“基主题 → 媒体变量/规则 → 变体”顺序重算；
- 媒体规则与普通规则：先匹配选择器/特异性/source order，再应用所有**当前命中媒体块**；不改变选择器特异性；
- 显式控件属性仍最高；主题媒体重算不得覆盖；
- 变体与媒体可叠加：基主题媒体 + 当前变体媒体；两者冲突时变体后应用（source order），需明确。

**运行时**：
- 监听窗口 resize / 视口缩放 / DPI 变化，重新评估媒体条件并应用（合并到帧末/下一帧，避免每帧重算/抖动）；
- 仅在命中媒体集合或变量变化时重算；保留 rect/焦点/滚动位置，不重建布局；
- 提供 C ABI/Binding：`ApplyTheme`（解析 media）、`SetThemeVariant`、可选 `GetActiveMedia`/`GetResponsiveThemeState`；必要时 `RefreshResponsiveTheme`；
- 主题切换、变体切换、`classes` 变更、运行时 Set* 均与媒体重算正确交互；
- 旧主题/无 media 时零行为变化；旧引擎忽略 media 并诊断，不崩溃。

**设计器 UI（配套）**：
- 主题管理器支持媒体条件列表、断点预设（如 640/800/1200）、变量/规则编辑；
- 预览窗口尺寸变化可触发；设计画布需明确“按设计尺寸/按窗口”；
- 规则列表显示当前生效媒体，避免用户误判。

**验收**：
- 同一主题在窄/宽窗口、横/竖方向、不同缩放档下应用不同变量/样式；
- resize 后样式重新计算，显式覆盖不变，焦点/布局稳定，无逐帧抖动；
- 变体 + 媒体组合；旧主题零回归；非法 query/变量诊断不崩；
- 保存/打开/导出/导入/预览全链；C ABI/Binding/schema/文档/测试同步。

**待引擎确认**：
- V1 媒体特性集合/单位（px 逻辑像素、scale）；
- 是否支持容器查询（container queries）后续版本；
- 媒体 + 变体的精确优先级/合成；
- resize 重算的节流/事件模型；
- 设计器设计尺寸与运行时窗口尺寸的口径。

#### 2026-11-02 补充：Splitter/容器尺寸变化与查询上下文

- **必须触发重算**：Splitter 拖动改变分栏比例、容器/面板布局重排、anchor reflow、控件/容器 rect 变化、窗口 resize、DPI/视口缩放变化，都要重新评估响应式主题；可在帧末/下一帧合并一次，避免拖动中每帧抖动。
- **查询上下文**：
  - `@media` 继续针对实际视口/窗口；
  - Splitter 改变的是**容器可用尺寸**而不是窗口尺寸，因此还需要**容器查询**（`@container`，或等价的 `container-width/height` 条件）作用于最近的布局容器/父容器；否则“重算”不会产生样式变化；
  - V1 至少支持 Splitter 所在的直接容器/面板宽度/高度查询；嵌套容器和更深层祖先作为后续扩展。
- **合成/优先级**：普通规则 → 视口媒体 → 容器查询 → 当前变体（同选择器/同特异性按 source order）；显式控件属性仍最高。
- **Splitter 拖动期间**：建议实时预览但节流；释放时确保一次最终重算。重算不得重建布局/丢焦点/改变滚动位置。
- **设计器**：预览/设计画布的 Splitter 调整均触发；主题管理器显示当前命中的视口/容器条件。
- **验收补充**：拖 Splitter 让面板跨越 640/800/1200 等断点时，内部控件样式切换；窗口 resize 与 Splitter 同时变化时只应用最终组合；嵌套容器行为待确认。
- **待引擎确认**：`@container` 语法/查询上下文、嵌套容器、Splitter 连续拖动的节流策略。

#### 2026-11-02 设计复核：通过（条件）
- media/container、Splitter 触发、dirty 合并、循环防护、API、旧主题兼容通过。
- 条件：依赖 CR2002 最终 `padding` key/contentRect；确认变体/媒体层顺序；补 Splitter 实际拖动与嵌套容器测试。

#### 2026-11-02 二次复核：通过（文档小修）
- media/container、Splitter 触发、dirty、循环防护、变体层序通过；依赖 content-padding/contentRect。
- 小修：§4.7 API 表的 SetPadding/GetPadding 改为 SetContentPadding/GetContentPadding；补 Splitter 连续拖动/嵌套容器实测。

### CR状态
#### 设计器：closed
#### 引擎：closed（引擎实施完成；2026-11-11 用户确认）
- 实施落地：`theme.media/containerQueries`、dirty 帧内合并、`RefreshResponsiveTheme/GetActiveMedia`；层序含变体最后（Q3）。

## CR20261105001 启动性能：LoadLayout / ApplyTheme 耗时与初始化资源日志

### 现状
CornerstoneDesigner 当前启动到首帧约 10.7s（Debug/Release 相近）。测量拆解：
- 主实例 `UICornerstone::Create`：约 0.2–0.5s；期间引擎打印两次 `Loading resources...` / `Loading finished, waiting user starting game...`。
- 主布局 `LoadLayoutFromFile(main_layout.json)`：约 5.8–6.8s。
- 设计器主题 `ApplyTheme(designer_theme.json)`：约 4–5s；将主题临时改为空对象后仍约 5.5s，说明耗时主要来自引擎应用/遍历路径，而不是主题 JSON 内容。
- 两条 Loading 日志不是设计器重复加载布局，也不是日志本身导致慢；重点是布局解析与主题应用。

### CR类型：bug

### 修订记录

#### 2026-11-05 初次提交CR

**建议引擎提供/优化**：
- `ApplyTheme` 空规则/空主题快速路径：无可应用规则时直接返回，避免全量遍历/重建；非空主题尽量一次遍历完成，避免逐控件重复查询。
- `LoadLayout` 性能分析：1920×1080 主布局解析 + 控件创建 + 首次 reflow 分阶段计时；减少重复 schema/属性查找和不必要的布局计算。可在 Release 下保留轻量可关闭的诊断计时。
- 初始化资源日志：确认主实例创建阶段为何出现两次资源加载；若为正常两阶段初始化，请在日志中加阶段前缀区分，避免宿主误判为重复加载；若为重复加载，消除重复。
- 设计器侧当前保持自动应用（用户选择），因此本 CR 不要求设计器改行为；引擎优化后设计器直接受益。

**接口**：不新增公开 API；现有 `LoadLayout`/`ApplyTheme` 行为保持不变。可选诊断 API/日志字段由引擎决定。

**测试**：
- 以 `layouts/main_layout.json` + `layouts/designer_theme.json` 在 Debug/Release 测量：主实例创建、LoadLayout、ApplyTheme、首帧各阶段耗时；目标为启动总耗时显著下降，首帧可交互。
- 空主题、空布局、单控件布局基线；主题规则数量递增（0/10/100/1000）对比；旧主题/旧布局零回归。
- 验证 `auto=2`（含 272 项 IO 自测）、`auto=3`、`validate_layout --strict`；两条初始化日志语义明确，不出现宿主误判。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-09 性能目标达成；P1 未实施）
- 分析/设计：`design/CR20261105001_StartupPerformance_Design.md`（修订版，待复核）。
- 评审意见：`requirements/CR20261105001_StartupPerformance_Design_评审意见.md`。
- 二次评审意见：`requirements/CR20261105001_StartupPerformance_Design_评审意见_v2.md`。
- 三次评审意见：`requirements/CR20261105001_StartupPerformance_Design_评审意见_v3.md`。
- 四次评审意见：`requirements/CR20261105001_StartupPerformance_Design_评审意见_v4.md`。
- 联测遗漏修复：classes/theme-role per-root 重算；`hasResponsiveData` 无响应式快路径；Release 已同步。

## CR20261105002 控件构造与主题显式回放重复资源重建性能优化

### 现状

CR20261105001 落地后设计器 Release 启动约 4.92s：
- `LoadLayout(main_layout.json)`：约 3.81s；
- `ApplyTheme(designer_theme.json)`：约 0.91s；
- JSON 解析已不是瓶颈：LayoutParser JSON 约 1.3ms，主题 JSON 解析 + `setTheme` <1ms。

临时插桩分析（`Temp/CR20261105001_启动性能分析.md`）：
- `LoadLayout`：控件树解析/构造约 2.9–3.2s，字体继承约 0.20s；Label 样本约 40–54ms（构造约 10ms、旧主题字体/颜色约 10ms、通用属性约 10–21ms、caption/布局约 10–12ms）；Button 约 20–22ms，Combo/TextArea 约 80ms。
- `ApplyTheme`：base 规则应用约 0.76–0.80s；其中 CSS `applyValue` 接近 0ms，**`replayExplicit` 占绝大部分**；Label 约 9.7ms、Button 约 9.5ms、Combo/Edit 约 19ms、MenuBar 约 50ms。`main_layout.json` 中显式 `font {name,size}` 被重复回放。
- `Label::setFont`/`setFontSize`/`setCaption` 等 setter 会调用 `recreate()`；布局解析和显式回放过程中同一控件被多次重建，字体资源反复释放/加载。

结论：当前 4.92s 主要不是 JSON 解析，而是**控件构造/属性写入阶段重复 create/字体资源重建**。JSON 预编译不能解决该问题；本 CR 聚焦资源生命周期与批量应用。

### CR类型：requirement

### 修订记录

#### 2026-11-05 初次提交CR

**建议引擎优化**：

1. **解析/主题应用批处理**：为 LayoutParser 与 ApplyTheme 增加内部批量作用域（例如 `BeginBatch/EndBatch` 或 RAII），期间合并 `recreate()`、字体释放、reflow 和文本布局；一次遍历结束后统一刷新。
2. **setter 去重**：`setFont`/`setFontSize`/`setCaption`/颜色/`content-padding` 等值未变化时直接返回；不要无条件 `releaseFont()`/`recreate()`。
3. **`replayExplicit` 跳过等值写入**：回放前读取当前值，显式值与运行时相同则跳过；尤其 font/font-size/content-padding，避免 ApplyTheme 每次重新触发字体重建。
4. **字体/文本资源缓存**：按 FontName + size + style + renderer 共享字体句柄或字形缓存；同字体控件不重复加载；控件销毁时仅释放引用。
5. **LayoutParser 两阶段化**（如可行）：先构造控件/解析属性，再统一 `create()`/reflow；避免构造、默认主题、通用属性、显式属性各自触发一次重建。
6. **可选诊断**：保留默认关闭的阶段计时（已有 CR20261105001 诊断可复用），报告 parse/tree/inherit/theme/replay 分项；不改变公开 API。

**不建议**：为当前 148 控件、145KB 布局做 JSON 预编译/二进制布局缓存；JSON 解析约 1.3ms，收益可忽略。若未来布局规模增长，另行评估编译缓存。

**接口**：不新增公开 API；批量作用域、去重、字体缓存均为引擎内部实现。诊断可沿用日志/环境变量。

**测试/验收**：
- 同一 Release、同一 `main_layout.json`/`designer_theme.json` 复测：建议目标 `LoadLayout` ≤2.8s、`ApplyTheme` ≤0.4s、启动到首帧 ≤3.5s；若达不到需给出分项数据及后续方案。
- 显式字体/颜色/padding 仍优先于主题；空主题 no-op、变体、media/container、popup/menu 行为零回归。
- 字体资源缓存不得泄漏；控件重复销毁/重建、切换项目、预览窗口、多次 ApplyTheme 均稳定。
- 回归：Debug/Release `auto=2`（272/0）、`auto=3`、`validate_layout --strict`、`test_cr_theme`；启动耗时不以单次抖动判定。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-06 联测通过）
- 分析/设计：`design/CR20261105002_ResourceRebuild_Design.md`（修订版，待复核）。
- 评审意见：`requirements/CR20261105002_ResourceRebuild_Design_评审意见.md`。
- 二次复核意见：`requirements/CR20261105002_ResourceRebuild_Design_复核意见_v2.md`。
- 三次复核意见（通过）：`requirements/CR20261105002_ResourceRebuild_Design_复核意见_v3.md`。
- 实施（2026-11-06）：P0 等值/replay + Label 延迟重建；探针 LoadLayout≈1.76s、ApplyTheme≈0.11s；完整通用批处理待后续。
- 回归修复：Label 构造期 setFont 短路需保留 m_fontFile；测试日志无 `loadFromResource: Error: ''`。
- 设计器分析：`requirements/CR20261105001_启动性能分析.md`（由设计器 Temp 镜像）。
- 交付说明（2026-11-06）：设计器 Release 启动约 3.11s；LoadLayout≈2.44s（目标≤2.8s）；ApplyTheme≈0.067s（目标≤0.4s）。
- 实际实现范围：等值短路、`replayExplicit` 规范化跳过、Label 解析期延迟 recreate；未新增通用 batch 符号，性能目标已由定向优化达成。
## CR20261105003 窗口可见性 API：创建期隐藏与运行期显示/隐藏

### 现状

设计器启动 Launcher 需要主窗口在 ready 前隐藏：

- 当前 `UICornerstone_GetNativeWindowHandle()` 仅在 SDL3 后端直接返回 `SDL_Window*`；SFML/raylib 不是 SDL_Window，不能作为跨后端契约。
- 引擎没有创建期隐藏标志；宿主只能在 `Create` 后调用 `SDL_HideWindow`，可能出现一帧窗口闪烁、抢 Launcher 焦点。
- 引擎没有通用的 `SetWindowVisible`；非 SDL3 后端无法在不重建实例的情况下显示/隐藏窗口。

设计文档：`design/09_启动Launcher设计.md`（v0.2）；引擎评审：`requirements/09_启动Launcher设计_引擎评审_2026-11-05.md`。

### CR类型：requirement

### 修订记录

#### 2026-11-05 初次提交CR

**建议引擎提供**：

1. `UIWindowFlags::Hidden`（对齐 SDL_WINDOW_HIDDEN；SFML/raylib 按后端能力实现或明确返回不支持），用于 `Create` 时直接隐藏窗口，避免首帧闪烁和焦点抢占。
2. `UICornerstone_SetWindowVisible(UIInstance instance, int visible)`：
   - 1=显示，0=隐藏；成功 1 / 失败 0；
   - 不重建实例/后端/控件，不改变焦点以外的布局；
   - headless/无窗口实例返回 0；
   - SDL3/SFML/raylib 各自实现，无法实现时返回 0 并诊断。
3. Binding：`UICornerstone::SetWindowVisible(bool)`（或等价封装），并在头文件/手册标注 backend 限制。
4. 可选：`GetWindowVisible` 查询，便于启动流程断言。

**接口**：新增窗口标志与 C ABI/Binding API；不改变现有 `Create`/`Show` 语义；旧代码不传 Hidden 时行为不变。

**测试/验收**：
- `Create(WithWindowFlags(Hidden))` 首帧不可见，无闪烁/抢焦点；
- `SetWindowVisible(0/1)` 显示/隐藏往返，控件、焦点、布局、主题状态保持；
- SDL3 全链路；SFML/raylib 按实现返回结果并诊断；
- headless 返回 0；非法实例不崩；
- Launcher ready 前隐藏、ready 后显示；Designer 直接启动（不传 Hidden）行为不变；
- Binding/API/文档/schema 同步；回归 `auto=2`、`auto=3`、`validate_layout --strict`。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-06 联测通过）
- 分析/设计：`design/CR20261105003_WindowVisibility_Design.md`（已实施，待设计器联测）。
- 评审意见：`requirements/CR20261105003_WindowVisibility_Design_评审意见.md`。
- 二次复核意见：`requirements/CR20261105003_WindowVisibility_Design_复核意见_v2.md`。
- 三次复核意见（通过）：`requirements/CR20261105003_WindowVisibility_Design_复核意见_v3.md`。
- 三次复核意见（通过）：`requirements/CR20261105003_WindowVisibility_Design_复核意见_v3.md`。

## CR20261105004 跨平台窗口外观 API：Borderless + AlwaysOnTop

### 现状

Launcher 需要无标题栏/关闭按钮并保持置顶：

- 当前 SDL3 `GetNativeWindowHandle()` 返回的 `SDL_Window*` 归属引擎后端上下文；宿主 Launcher 再动态加载一份 SDL3 时，`SDL_GetWindowFlags` 返回 0，`SDL_SetWindowBordered/SDL_SetWindowAlwaysOnTop` 失败。
- 引擎已有 `UIWindowFlags::Hidden` / `SetWindowVisible`（CR20261105003），但没有创建期无边框和运行期置顶能力。
- 当前 Launcher 使用 Win32 `EnumWindows/SetWindowLongPtr/SetWindowPos` 临时兜底，不能作为跨平台接口。

### CR类型：requirement

### 修订记录

#### 2026-11-06 初次提交CR

**建议引擎提供**：

1. `UIWindowFlags::Borderless`（对齐 `SDL_WINDOW_BORDERLESS`，值 `0x10`）：
   - 创建期无标题栏与关闭按钮；
   - SDL3/SFML/raylib 各自映射；无法创建期实现的后端返回/诊断明确降级。
2. `UICornerstone_SetWindowAlwaysOnTop(UIInstance, int onTop)`：
   - 运行期置顶/取消置顶；成功 1/失败 0；
   - 幂等；仅 owner 独立窗口允许，子视口/headless 返回 0；
   - 不重建实例/控件，不改变布局/焦点字段。
3. 可选 `UICornerstone_GetWindowAlwaysOnTop(UIInstance, int*)`，语义同 `GetWindowVisible`，返回引擎本地状态。
4. Binding：`UIWindowFlags::Borderless`、`SetWindowAlwaysOnTop(bool)`、可选 `IsWindowAlwaysOnTop()`。
5. 保持现有 `Hidden/SetWindowVisible` 语义与后端能力约定；不要求宿主直接使用 native handle。

**接口约束**：

- 不新增平台头文件依赖；C ABI/Binding 由引擎封装。
- 后端不支持时返回 0 并诊断，禁止假成功；旧 DLL 缺入口时 Binding 返回 false、诊断一次。
- `SetWindowAlwaysOnTop` 只影响窗口层级，不改变窗口尺寸/位置/缩放/控件树/主题。
- 线程：与现有 UI 帧循环同线程。

**测试/验收**：
- `Create(WithWindowFlags(Borderless))`：无标题栏/关闭按钮，首帧无边框闪烁；控件树/布局/主题正常。
- `SetWindowAlwaysOnTop(1/0)`：置顶往返、幂等；不影响子视口/headless；非法实例不崩。
- SDL3 全链路；SFML/raylib 按后端能力实现或明确返回 0；文档标注能力矩阵。
- Launcher 去掉 Win32 窗口样式兜底后，ready 前隐藏、ready 后显示、无标题栏、置顶、ready/失败/超时行为不变。
- Binding/API/手册/schema 同步；回归 `auto=2`、`auto=3`、`validate_layout --strict`、`test_cr_theme`。

### CR状态
#### 设计器：closed
#### 引擎：closed（SDL3 已验证；SFML/raylib 按能力矩阵/探针待补）
- 分析/设计：`design/CR20261105004_WindowAppearance_Design.md`（修订版，待复核）。
- 评审意见：`requirements/CR20261105004_WindowAppearance_Design_评审意见.md`。
- 二次复核意见：`requirements/CR20261105004_WindowAppearance_Design_复核意见_v2.md`。
- 三次复核意见：`requirements/CR20261105004_WindowAppearance_Design_复核意见_v3.md`。
- 四次复核意见（通过）：`requirements/CR20261105004_WindowAppearance_Design_复核意见_v4.md`。
- 实施（2026-11-06）：Borderless/AlwaysOnTop/Foreground + Binding + version=2 回调；test_window_visibility 通过；Release 已同步。

## CR20261108001 启动器剩余 Win32 能力封装：窗口阴影 + 宿主进程/路径

### 现状

CR20261105003 已封装窗口 Hidden/SetWindowVisible；CR20261105004 已封装 Borderless/AlwaysOnTop/Foreground。Launcher 仍直接使用以下 Windows API：

- 窗口阴影/装饰：`EnumWindows`、`SetWindowLongPtr`、`GetClassLongPtr`、`CS_DROPSHADOW`、`SetWindowRgn`、`DwmSetWindowAttribute`、`DwmExtendFrameIntoClientArea`；
- 前台/窗口查找：`GetWindowThreadProcessId`、`IsWindowVisible`、`ShowWindow`、`SetWindowPos`、`SetForegroundWindow`、`SetActiveWindow`、`BringWindowToTop`（CR05004 只能移除其中一部分）；
- 进程：`CreateProcessW`、`PROCESS_INFORMATION`、`CREATE_NO_WINDOW`、`WaitForSingleObject`、`TerminateProcess`、`CloseHandle`、Toolhelp32 `Process32First/Next`；
- 路径/错误：`GetModuleFileNameW`、`MultiByteToWideChar`、`MessageBoxW`。

其中 CR05004 明确不承诺无 DWM 阴影，因此 Windows 阴影代码仍留在 LauncherPlatform；进程启动也不是 UI 引擎职责。

### CR类型：requirement

### 修订记录

#### 2026-11-08 初次提交CR

**建议引擎分两部分处理**：

**A. 窗口阴影/非客户区能力（建议纳入窗口 API）**

1. `UIWindowFlags::NoShadow`（或 `Borderless` 的附加语义）：
   - Windows 下由引擎统一处理 `CS_DROPSHADOW`/DWM/窗口 region，宿主不再直接碰 DWM；
   - SDL3 实现跨平台可用；SFML/raylib 不支持时返回/诊断明确；
   - 与 Borderless/Hidden 组合：创建期无标题栏、无关闭按钮、无系统阴影；
   - 若无法跨后端统一，至少提供 `SetWindowShadowEnabled(bool)` 并在能力矩阵标注平台限制。
2. 保持 CR05004 的 Borderless/AlwaysOnTop/Foreground；Launcher 不再直接调用 DWM/EnumWindows/SetWindowLongPtr。

**B. 宿主进程/路径能力（建议由独立 Host/Platform 模块或引擎可选工具提供）**

3. 进程启动/等待/终止：
   - 输入 exe、args、workdir、stdout/file progress 选项；
   - 返回 PID/进程句柄；支持 wait/kill/close；
   - 跨平台实现（Win32 CreateProcess / POSIX spawn / SDL3 Process）；
   - 宿主不直接使用 `CreateProcessW`/`PROCESS_INFORMATION`/Toolhelp32。
4. 可执行文件目录/当前模块路径与 UTF-8 转换：
   - 提供宿主可用的 `GetExecutableDirectory`/`GetBasePath`（或明确推荐使用 SDL3 API）；
   - 错误提示 UI 由 Launcher 自己的 loading UI 承担，不要求引擎封装 MessageBoxW。
5. 若不希望 UI 引擎承担进程生命周期，请在设计中明确“进程/路径归 LauncherPlatform，不由引擎提供”，避免后续再按本 CR 追问。

**接口约束**：
- 不改变现有 `Create/Show/SetWindow*` 语义；旧宿主 flags=0 行为不变。
- 旧 DLL 缺新增入口时 Binding 返回 false/诊断一次。
- 线程：与现有 UI 帧循环同线程；进程回调不阻塞 UI 线程。
- 窗口阴影能力若仅 Windows 支持，必须标注 backend/平台矩阵，不假成功。

**测试/验收**：
- SDL3 Borderless+Hidden+NoShadow（如实现）：首帧无标题栏/关闭按钮/系统阴影；Show/Hide 往返；控件/布局/主题/焦点字段不变。
- Launcher 去掉 Win32 `EnumWindows/SetWindowLongPtr/SetWindowPos/DWM` 后，ready 前隐藏、ready 后显示、无标题栏、置顶/前台、失败/超时行为不变。
- 进程 API：启动、PID 正确、wait/exit code、kill、stdout/文件进度、超时/失败；路径含空格/中文；多次启动稳定。
- SFML/raylib/headless 按能力返回；旧 DLL 降级；静态/插件模式一致。
- 回归 `test_window_visibility`、`test_window`、`auto=2`、`auto=3`、strict、`test_cr_theme`。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-09 联调通过）
- 分析/设计：`design/CR20261108001_LauncherRemainingWin32_Design.md`（修订版，待复核）。
- 评审意见：`requirements/CR20261108001_LauncherRemainingWin32_Design_评审意见.md`。
- 二次复核意见：`requirements/CR20261108001_LauncherRemainingWin32_Design_复核意见_v2.md`。
- 三次复核意见：`requirements/CR20261108001_LauncherRemainingWin32_Design_复核意见_v3.md`。
- 四次复核意见（通过）：`requirements/CR20261108001_LauncherRemainingWin32_Design_复核意见_v4.md`。
- 实施（2026-11-09）：NoShadow/SDL3 Windows DWM best-effort；SFML/raylib 诊断忽略；test_window_visibility 通过；Release 已同步。

## CR20261109001 Launcher 任务栏隐藏：引擎窗口 Utility 标志

### 现状

Launcher 已使用 `UIWindowFlags::Borderless | Hidden | NoShadow`，并通过 `SetWindowAlwaysOnTop/SetWindowForeground` 管理窗口外观；原先用于隐藏任务栏/Alt+Tab 的 Win32 `WS_EX_TOOLWINDOW` 已随 CR20261108001 的窗口样式清理移除。

当前引擎没有 Utility/SkipTaskbar 能力，Launcher 会重新出现在任务栏和窗口列表；若继续用 Win32 需重新引入 `SetWindowLongPtrW(WS_EX_TOOLWINDOW)`，与“窗口外观由引擎封装”的方向冲突。

SDL3 已提供原生 `SDL_WINDOW_UTILITY`（0x00020000），语义为“utility window，不显示在任务栏和窗口列表”；引擎只需暴露该标志并透传，Windows 下 SDL3 通常会应用 `WS_EX_TOOLWINDOW` 等价效果。**实现可行性较高，主要风险在非 SDL3 后端和平台差异。**

### CR类型：requirement

### 修订记录

#### 2026-11-09 初次提交CR

**目标**：

- 新增应用层窗口标志 `UIWindowFlags::Utility`（`UICORN_WINDOW_FLAG_UTILITY = 0x00020000`，与 `SDL_WINDOW_UTILITY` 对齐）；
- Launcher 使用 `Utility | Borderless | Hidden | NoShadow`，不在任务栏/窗口列表显示，仍可显示、置顶、前台、无标题栏/阴影；
- 不引入新的运行期 API，不改 BackendCallbacks 版本；旧宿主 flags=0 行为不变。

**接口/实现建议**：

1. `UICornerstoneAPI.h` 增加 `#define UICORN_WINDOW_FLAG_UTILITY 0x00020000u /* SDL_WINDOW_UTILITY */`；`Window.h`/Binding 暴露 `UIWindowFlags::Utility`。
2. SDL3 backend 将 Utility 原样传给 `SDL_CreateWindowAndRenderer`；仅创建期消费，不新增 SetUtility。
3. SFML/raylib：无对应跨平台能力，**mask 后忽略并诊断一次**，不得透传未知位；headless 无意义。
4. CallbackWindow/BackendCallbacks 路径保留 Utility 位给新 backend；旧 backend 忽略未知位不崩溃；核心不得在进入 backend 前把 Utility 剥掉（与 CR20261108001 的 NoShadow 处理一致）。
5. `NoShadow` 仍在首次显示前处理；`Utility` 与 `Borderless | Hidden | NoShadow` 组合必须可用；`SetWindowAlwaysOnTop/SetWindowForeground` 对 Utility 窗口继续有效。
6. 文档/能力矩阵标注：SDL3 Windows 实际支持；SDL3 其他平台按 SDL3 语义；SFML/raylib 不支持；不承诺跨后端一致。

**测试/验收**：

- Windows SDL3：`Utility | Borderless | Hidden | NoShadow` 创建；任务栏和 Alt+Tab 不出现 Launcher；窗口显示/隐藏、置顶、前台、无标题栏/阴影、焦点/控件布局正常。
- `Utility | Borderless | NoShadow`（无 Hidden）内部隐藏处理后再显示，首帧无标题栏/关闭按钮；NoShadow 仍 best-effort。
- SFML/raylib/headless：Utility 诊断忽略、窗口正常创建、不崩溃；原生 flags 不含 0x00020000。
- 旧 flags=0 行为不变；静态/插件模式；回归 `test_window_visibility`、`test_window`、`auto=2`、`auto=3`、strict、`test_cr_theme`。
- Launcher `auto=2` ready 后退出，任务栏无残留图标；失败/超时/取消路径不回归。

**边界**：

- 本 CR 只增加窗口 Utility/SkipTaskbar 能力；进程启动/路径/错误 UI 仍归 LauncherPlatform（CR20261108001 B 已明确）。
- 若 SDL3 Windows 的 Utility 仍出现在任务栏，需在设计中明确 fallback（例如 backend 内部应用 `WS_EX_TOOLWINDOW`），但不得让 Launcher 重新直接调用 Win32。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-09 联调通过；binding Utility enum 已补；任务栏/Alt+Tab 人工验收通过）
- 分析/设计：`design/CR20261109001_LauncherUtility_Design.md`（已关闭；联调通过）。
- 实施（2026-11-09）：Utility flag/SDL3 原生透传+EXSTYLE fallback/SFML+raylib mask；test_window_visibility 7 项通过；待设计器联调任务栏/Alt+Tab。
- Release（2026-11-09）：DLL SHA256 `e805073f8e2e5979256100d92c640f84dcc7b0e40c7701effc20e22d67c2bcf3`；已同步设计器子模块；引擎侧仍 open，待联调后关闭。
- 交付缺口（2026-11-09）：binding/include/UICornerstone.h 补 `UIWindowFlags::Utility`；Binding 编译通过，release/binding 与子模块已同步；DLL hash 不变，待设计器复测任务栏/Alt+Tab。

## CR20261109002 StatusBar 段 onClick handler 未绑定（空 lambda）

### 现状

设计器已支持 status-bar items 结构化编辑，引擎 `LayoutParser::parseStatusBar` 也读取 item 的 `onClick`：

```cpp
// D:/GitSpace/UIControls/src/LayoutParser.cpp:1663 附近
if (ij.contains(PropertyNames::kJsonOnClick) && ...is_string()) {
    string action = ...;
    bar->setStatusItemOnClick(id, [action](shared_ptr<StatusItem>){}); // 空回调
}
```

结果：`items[].onClick` 的 action 名被读取后丢弃，`StatusItem::onClick` 永远不会调用宿主 handler；点击状态栏段无任何效果。这是实现缺陷，不是设计器侧可绕过的问题。

### CR类型：bug

### 修订记录

#### 2026-11-09 初次提交CR

**修复建议**：

- 与 MenuItem/ListItem 事件相同：在 `m_handlers` 中按 action 查 handler；找不到时 `logMissingHandler` 诊断，找到后绑定：
  `setStatusItemOnClick(id, [handler](shared_ptr<StatusItem>){ handler(nullptr); })`；
- 不需要新增 C ABI/Binding/LayoutParser API；
- 保持旧行为：无 `onClick` / 未注册 handler 时不崩溃，仅诊断；
- 设计器后续将提供 status-bar item 事件树（`items[].events.onClick`），引擎先修复绑定。

**测试/验收**：

- `layouts/test_layout.json` 或新增测试布局含 status-bar item `onClick`，注册对应 action，点击段触发一次；
- 缺 handler 时日志诊断且不崩溃；
- 多段、右侧段、分隔/普通 item 均正常；
- 回归 `validate_layout --strict`、现有 StatusBar 字体/颜色/悬停测试。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-11 设计器联调通过；Release dab958f1...）

## CR20261109003 ContextMenu JSON items / events.onClick 支持

### 现状

- `ContextMenu` 目前是独立 `context-menu` 控件，引擎 C ABI 已提供 `CreateContextMenu`、`ContextMenuAddItemEx`、`AddSeparator`、`Show/Close`，条目可通过 `SetCallback(item,"click")` 绑定（设计器右键菜单即走此路径）。
- 但 JSON 解析 `LayoutParser::parseContextMenu` 只创建隐藏菜单，**不解析 items**；schema 的 `context-menu` 也没有 items 字段。
- 因此设计器无法把 ContextMenu 的菜单项像 MenuBar 一样保存到布局/事件页，也无法由引擎在 LoadLayout 时自动创建并绑定条目事件。

### CR类型：requirement

### 修订记录

#### 2026-11-09 初次提交CR

**目标**：让 ContextMenu 支持声明式 items 和 `events.onClick`，与 MenuBar 的菜单项事件模型一致。

**建议 JSON**：

```json
{
  "type": "context-menu",
  "id": "ctxMain",
  "items": [
    { "caption": "删除", "shortcut": "Del", "events": { "onClick": "onCtxDelete" } },
    { "type": "separator" },
    { "caption": "属性", "events": { "onClick": "onCtxProps" } }
  ]
}
```

**引擎实现**：

1. `parseContextMenu` 读取 `items[]`，逐项调用现有 `ContextMenuAddItemEx` / `AddSeparator`；
2. V1 支持 caption/shortcut/separator 与 `events.onClick`；子菜单/嵌套 items 由引擎按 ContextMenu 能力决定，如不支持明确诊断；
3. handler 通过 `m_handlers` 查表绑定（与 MenuItem 一致）；未注册仅诊断；
4. schema `context-menu` 增加 items 定义、`docs/controls/context-menu` 或菜单文档补示例；
5. 不新增 C ABI：复用 `ContextMenuAddItemEx` + `SetCallback` / 现有内部绑定。

**设计器侧（后续）**：

- ContextMenu 属性面板增加 items 结构化编辑（caption/shortcut/separator）；
- 事件页复用“item 路径 + onClick”模式（与 MenuBar 菜单树同构）；
- 导出/保存 events，宿主 `RegisterAction` 绑定。

**测试/验收**：

- 无 items 的旧 ContextMenu 行为不变；
- items 创建顺序/分隔符正确；点击触发对应 handler 一次；
- 缺 handler 诊断、不崩溃；
- schema `validate_layout --strict` 通过；
- 现有代码创建的右键菜单（工具栏/控件树）不回归。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-11 设计器联调通过；Release dab958f1...）
- 分析/设计：`design/CR20261109002_StatusBarItemClick_Design.md`（已关闭；设计器联调通过）。
- 实施（2026-11-09）：parseStatusBar events.onClick/onClick→m_handlers；test_cr_menu_item_events；回归 57 OK + 4 预存失败；Release `dab958f1...` 已同步。

- 分析/设计：`design/CR20261109003_ContextMenuItems_Design.md`（已关闭；设计器联调通过）。
- 实施（2026-11-09）：独立 context-menu items/onClick，复用 addItem 自动关闭；test_cr_menu_item_events；回归 57 OK + 4 预存失败；Release `dab958f1...` 已同步。

## CR20261110001 EditBox 初始 placeholder/文本垂直偏移未刷新（聚焦后才居中）

### 现状

设计器工具箱搜索框 `edToolSearch`（EditBox）初始显示 placeholder 时文字偏低并被裁切；鼠标点入、获得焦点后 placeholder 又正常居中。

根因在引擎 `EditBox`：

- `EditBox::updateTextOffset()` 用 `(drawRect.height - fontHeight) / 2` 计算 `m_textOffsetY`；
- 初始阶段字体尚未加载/主题字号尚未应用时，`fontHeight` 回退为 `m_fontSize * scaleY`；
- 之后字体/主题生效，但 `m_textOffsetY` 没有重新计算，直到 `onFocusGained()` 再次调用 `updateTextOffset()` 才恢复正确。

设计器已临时在 `applyDesignerTheme()` 后对 `edToolSearch` 做一次同 rect `SetRect` 强制刷新；但这只修一个控件，其它 EditBox（ComboBox/NumericUpDown 内部编辑框、对话框输入框等）同样存在潜在问题。

### CR类型：bug

### 修订记录

#### 2026-11-10 初次提交CR

**建议引擎修正**：

1. 在字体加载、主题字号/字体应用、布局 rect 完成后保证 `updateTextOffset()` 被调用；重点是“字体从 fallback 变为实际字体”后刷新；
2. `EditBox::draw()` 前可增加一次轻量校验：若当前 `fontHeight`/`drawRect` 与上次计算不一致，重算 `m_textOffsetY`（避免逐帧重算，可缓存签名）；
3. `setPlaceholder()` 也应调用 `updateTextOffset()`，避免 placeholder 单独变化时使用旧偏移；
4. 不改变输入/光标/选择/密码模式语义，不新增公共 API。

**测试/验收**：

- 新建 EditBox（placeholder、无文本）在首次 `create`/主题应用后不聚焦即垂直居中，无裁切；
- 设置 18/20 等字号、主题在创建后应用、布局 resize/锚定后 placeholder 仍居中；
- 获得焦点/失焦往返不跳位；ComboBox/NumericUpDown 内部 EditBox 同样通过；
- 回归 EditBox/ComboBox/NumericUpDown/TextArea 测试与 `auto=2`/`auto=3`/strict。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-10 联调通过；Release a3fcef00...；SetRect 兜底已移除）
- 分析/设计：`design/CR20261110001_EditBoxOffset_Design.md`（已关闭；设计器联调通过）。
- 实施（2026-11-10）：offset 刷新/签名/字体闸门；test_editbox_offset；回归 60 OK + 4 预存失败；Release `a3fcef00...` 已同步。

## CR20261110002 StatusBar events.onClick 在导出运行时未执行

### 现状

项目 `Temp/First Project.csd` 的 `status_1.items[].events.onClick` 已正确导出到 `layout.json`；导出 C++ 的 `main.cpp` 也在 `LoadLayout` 前注册了对应 action。运行时点击状态栏可以触发 C ABI `status-item-click`（探针输出 idx=0/1），但 `events.c` 的 action 回调不执行，说明点击命中正常，问题在 LayoutParser 的 StatusBar action 绑定链。

已核对：

- `LayoutParser::parseStatusBar` 源码含 `events.onClick` / `onClick` 解析与 `m_handlers` 查找；
- 导出 DLL 为 `dab958f…`；
- C ABI `status-item-click` 正常，`StatusItem::onClick` 未执行。

请引擎排查：

1. 发布 DLL 是否确实包含 CR20261109002 的 StatusBar action 绑定代码（源码存在但需确认构建产物）；
2. `parseStatusBar` 设置 `setStatusItemOnClick(id, ...)` 时，id 是否与最终 `StatusBar` 项 id 一致；
3. `parseEvents(bar,j)` 或后续 StatusBar 初始化是否覆盖/清空 item.onClick；
4. handler 名称注册表与 item action 名称是否匹配（可打印 log）。

**最小复现**：

- `Temp/First Project.csd`
- 导出 `D:/Temp/FirstProjectCpp`（layout.json + main.cpp + events.c）
- 点击状态栏 Sector1/Sector2：`status-item-click idx=0/1` 有输出，`events.c` printf 无输出。

### CR类型：bug

### 修订记录

#### 2026-11-10 初次提交CR

- 证据来自导出 C++ 工程与 C ABI 探针；设计器 layout/events/main 均正确。
- 需要引擎确认为何 StatusBar item action 未绑定/未执行。


#### 2026-11-10 根因补充：null ctl 被 Binding thunk 丢弃

进一步核对：导出 `layout.json`、`events.c`、`main.cpp` 均正常；C ABI `status-item-click` 已触发，但 action 不执行。

根因链路：

- `LayoutParser` 的 StatusBar/ContextMenu item action 均按设计调用 `handler(nullptr)`；
- C++ Binding `UICornerstone::RegisterAction` 的 thunk 当前有：
  `if (!ctl || !t) return;`
- 因此 action thunk 在调用 `events.c` 的 handler 前直接返回；C ABI 探针不受影响。

同样影响 ContextMenu item action（parser 也传 nullptr），所以右键菜单项点击也无 `events.c` printf。

建议修复（二选一）：

1. LayoutParser 传非空 Control：
   - StatusBar：把 `bar`（StatusBar 是 Control）传给 handler；
   - ContextMenu：把 `item`（MenuItem 是 Control）或 ContextMenu 控件传给 handler；
2. 或修改 Binding thunk：`ctl==nullptr` 时仍调用 `t->cb(Control())`（空 Control），不直接 return；同时保持 C ABI/Menu 现有语义兼容。

验收：

- StatusBar item action 在导出 C++ 中执行；
- ContextMenu item action 在右键菜单点击后执行；
- 现有 MenuBar/Button action 不回归；
- C ABI status-item-click 行为不变。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-10 联调通过；Release a227d811...）

## CR20261110003 Button 右键 context-menu 被 Button::handleEvent 吞掉

### 现状

导出工程已正确执行目标绑定：

```cpp
auto t = ui->FindControl("testBtn");
auto m = ui->FindControl("ctx_6");
t.SetPtr("context-menu", m.Handle());
```

但右键 `testBtn` 不弹出 ContextMenu。

根因在引擎 `Button::handleEvent`：

- 右键 MouseDown 命中 `drawRect` 后，Button 走通用分支并在末尾 `return true`；
- **没有先调用 `ControlImpl::handleEvent`**，因此 `ControlImpl` 中的 `m_contextMenu` 右键处理永远不执行；
- Label 和 Panel 等控件会先调用 `ControlImpl::handleEvent`，所以同一 ContextMenu 绑定到 Label/Panel 可以弹出。

### CR类型：bug

### 修订记录

#### 2026-11-10 初次提交CR

**建议修复**：

- `Button::handleEvent` 入口先调用 `ControlImpl::handleEvent(event)`（或至少对 `MouseDown Right + m_contextMenu` 做同样处理），再进入 Button 自己的左键状态机；
- 不改变 Button 左键/键盘/Finger 语义；
- ContextMenu 打开后 `isPopupVisible` 防重复。

**测试/验收**：

- Button 绑定 ContextMenu：右键弹出、左键仍触发 onClick；
- 未绑定 ContextMenu 的 Button 右键不应改变状态/触发 click；
- Label/Panel/StatusBar 目标不回归；
- 导出 C++ `testBtn` 目标实测通过。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-10 联调通过；Release a227d811...）
- 分析/设计：`design/CR20261110002_NullActionSender_Design.md`（已实施；待设计器联调）。
- 实施（2026-11-10）：Binding thunk 允许 null sender；test_cr_statusbar_runtime/contextmenu_dispatch；回归 59 OK + 4 预存失败；Release `a227d811...` 已同步。

- 分析/设计：`design/CR20261110003_ContextMenuRightClick_Design.md`（已实施；待设计器联调）。
- 实施（2026-11-10）：ControlImpl 右键子先/父兜底；全控件矩阵测试；回归 59 OK + 4 预存失败；Release `a227d811...` 已同步。

## CR20261110004 LuotiAni 等纯显示控件绑定 ContextMenu 后右键不弹出

### 现状

Designer 已将 ContextMenu 目标正确绑定到动画控件：

```cpp
auto t = ui->FindControl("ani_1");
auto m = ui->FindControl("ctx_6");
t.SetPtr("context-menu", m.Handle());
```

导出工程和 Designer 预览中，右键 `ani_1` 均不弹出 ContextMenu。

根因在引擎：

- `LuotiAni::isContainsPoint()` 恒返回 `false`（纯显示控件不参与事件命中）；
- `ControlImpl::tryOpenContextMenu()` 用 `isContainsPoint(mx, my)` 判断是否命中，因此动画控件永远不会进入右键菜单分支；
- `Actor::isContainsPoint()`、`HandleControl::isContainsPoint()` 同样恒为 `false`，存在同类问题。

### CR类型：bug

### 修订记录

#### 2026-11-10 初次提交CR

**建议修复**：

- ContextMenu 命中判定不要复用普通事件命中的 `isContainsPoint()`；`ControlImpl::tryOpenContextMenu()` 改用 `getDrawRect().contains(mx, my)`（或新增独立的 context-menu hit-test 入口，默认沿用 `isContainsPoint`，仅在 ContextMenu 分支使用几何范围）；
- 保持 `getVisible()` / `getEnable()` 守卫；
- 不改变 LuotiAni/Actor/HandleControl 的普通事件“不参与命中”语义，只修复右键 ContextMenu 目标路径；
- 子控件菜单优先、父菜单兜底的既有逻辑不变。

**测试/验收**：

- `ani_1` 绑定 ContextMenu 后右键弹出，菜单项 action 正常；
- 动画控件左键、普通右键（未绑定菜单）行为不变；
- Button/Label/Panel/Bench 目标 ContextMenu 不回归；
- 嵌套 Panel 内的 Animation 右键可弹出；
- 导出 C++/C 工程实测通过。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-10 联调通过；Release a3fcef00...）
- 分析/设计：`design/CR20261110004_DisplayContextMenu_Design.md`（已实施；待设计器联调）。
- 实施（2026-11-10）：isContextMenuHit/HandleControl=false；LuotiAni/Actor 测试；回归 60 OK + 4 预存失败；Release `a3fcef00...` 已同步。

## CR20261111001 Splitter 移出后鼠标仍保持双向缩放光标

### 现状

设计器主布局的 Splitter：鼠标从左侧进入 Splitter 后，光标变为左右双向箭头；继续向右移出 Splitter，进入右侧面板后光标仍保持双向箭头。

预期：离开 Splitter 的命中区域后，应恢复默认箭头；再次进入 Splitter 才重新显示双向箭头。

根因在引擎 `Splitter`：

- `Splitter::handleEvent()` 只在收到 `MouseMove` 且 `isContainsPoint()` 命中自身时调用 `updateCursor(true)`；鼠标移出后，父 Panel 的命中派发不再把 `MouseMove` 发给 Splitter；
- `ControlImpl::update()` 的 hover 检测会把 `m_mouseInside` 置 false 并调用虚函数 `onMouseLeave()`，但 `Splitter` 没有覆写 `onMouseLeave()`，因此没有调用 `Cursor::restoreDefault()`；
- 拖拽结束路径 `endDrag()` 也未在指针位于 Splitter 外部时恢复光标，拖到外面松开同样会残留。

### CR类型：bug

### 修订记录

#### 2026-11-11 初次提交CR

**建议修复**：

1. 在 `Splitter` 覆写 `onMouseLeave()`：
   ```cpp
   void Splitter::onMouseLeave(float x, float y) {
       ControlImpl::onMouseLeave(x, y);
       m_hovered = false;
       Cursor::restoreDefault(getThis().get());
   }
   ```
2. 可同时覆写 `onMouseEnter()`，在进入时调用 `ensureCursors()`/`updateCursor(true)`，不依赖首帧 MouseMove；
3. `endDrag()` 中按最终鼠标位置调用 `updateCursor(isContainsPoint(...))`，或指针在外部时 `restoreDefault`，避免拖拽释放后残留；
4. 仅恢复 Splitter 自己拥有的光标，保留 `Cursor` owner 仲裁，不影响其它控件光标。

**测试/验收**：

- 鼠标从 Splitter 左右两侧移入 → 双向箭头；移出 → 默认箭头；
- 从一侧穿过 Splitter 到另一侧，离开后不残留；
- 按住拖拽到 Splitter 外释放 → 光标恢复；
- 两个 Splitter/其它自定义光标控件相邻时不互相覆盖；
- `test_splitter`、设计器主布局 splitter1/splitter2 手工回归。

### CR状态
#### 设计器：closed
#### 引擎：closed（2026-11-11 联调通过；Release 94bf92b9...）
- 分析/设计：`design/CR20261111001_SplitterCursor_Design.md`（已关闭；设计器联调通过）。
- 实施（2026-11-11）：hover/owner 清理 + endDrag 最终坐标；test_cr_splitter_cursor；回归 61 OK + 4 预存失败；Release `94bf92b9...` 已同步。
