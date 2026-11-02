# ListView / TreeView 就地编辑与列宽拖拽（引擎 P0-111/P0-112/P0-113）需求与设计建议

> 提出方：CornerstoneDesigner；目标：引擎提供 ListView 单元格、TreeView 节点标签的**双击就地编辑**，设计器只接入事件/属性，不自行叠加 EditBox。
> 月度需求清单：`Temp/UICornerstone_配合修改清单_2026-11.md`。
> CR 编号：`CR20261101001`（P0-111）/ `CR20261101002`（P0-112）/ `CR20261101003`（P0-113）。
> 当前设计器侧：事件页为自定义行，不依赖本功能；通用 ListView/TreeView 数据编辑尚未接入。

## 1. 现状（引擎源码事实）

- **ListView**：`ListRow.cells` 为每列文本；已有 `getCell/setCell`、`cellControls`（稀疏单元格控件，按行/列定位，`syncChildControls` 随滚动/布局定位）与 `item-click`（`GridPayload{row,col}`）事件；无双击编辑/覆盖 EditBox。
- **TreeView**：节点 `label`、`setNodeLabel`；`selectedId/selectedRow`；滚动/展开/折叠；事件 `select`（`TreeNodePayload{id}`）、`expand/collapse`（String）；无节点标签编辑控件。
- **双击**：引擎控件目前未提供统一双击事件；设计器画布自行做 450ms/4px 判定。

## 2. 目标与非目标

目标：
- 双击可编辑单元格/节点标签 → 在**该位置**临时出现 EditBox（覆盖层），预填现文本；
- Enter / 失焦提交；Esc 取消；
- 滚动、列宽变化、行/节点增删移、数据刷新、控件隐藏/禁用/销毁时**安全清理**，不残留、不越界；
- 提供事件让宿主同步数据；提供 API 供程序化开始/提交/取消；
- 与现有单双击、选择、拖拽、排序、展开/折叠语义不冲突。

非目标（本批可不做）：
- 富文本/多行单元格、公式、下拉候选；
- 行列虚拟化（沿用现有滚动实现）；
- 跨单元格区域编辑。

## 2.1 列宽拖拽（用户决策：手势归引擎，P0-113）

- ListView **multi 视图**运行时已支持列头分隔线拖拽：`m_dragCol/m_dragStartWidth` + `setColumnWidth`（`min-column-width` 钳制）；现有 `ListViewSetColumnWidth` 只能设宽，不能把**设计态手势**委托给引擎。
- 设计画布拦截鼠标（避免行选择/排序/单元格点击），因此引擎运行时拖拽在设计态不可用。**用户定稿：不在设计器重复命中/宽度计算，由引擎提供设计态手势 API，设计器只转发指针、持久化模型、记一次撤销。**
- 新增 **P0-113**：`ListViewHitTestColumnDivider` + `Begin/Update/End/CancelColumnResize`（引擎内部复用运行时拖拽路径）；`column-resize-*` 事件带 `col/oldWidth/newWidth/canceled`；详见清单。

## 2.2 P0-113 设计态列宽拖拽 API（引擎拥有手势）

- 命中：`int HitTestColumnDivider(lv, localX, localY)`；multi 视图表头区、4px 半宽、返回列索引/-1；与运行时同一判定。
- 会话：
  - `bool BeginColumnResize(lv, col, localX)`：记录 oldWidth；
  - `float UpdateColumnResize(lv, localX)`：引擎计算/钳制/relayout，返回当前宽度；
  - `bool EndColumnResize(lv, commit)`：commit=1 保留；0 恢复 oldWidth；
  - `bool CancelColumnResize(lv)`：恢复 oldWidth。
- 事件：`column-resize-begin/changed/ended`，载荷 `col/oldWidth/newWidth`（begin 仅 old；changed/ended 旧+新；ended 含 canceled）；可选只读属性 `column-resize-old-width/new-width`。
- 约束：设计态只处理表头分隔线，不触发行选择/排序/单元格编辑；事件回调仅通知，宽度由引擎写回；设计器收到后同步 model 并在 ended 记一次撤销；旧 DLL 无 API 时返回 -1/false，设计器回退属性面板。
- 验收：设计画布拖拽与预览运行时宽度/钳制/视觉一致；模型/属性面板/撤销同步；取消恢复；不产生选择/排序副作用。

## 3. 交互与生命周期

### 3.1 ListView
- 开启：`cell-editing=true`；`editable-columns` 为允许编辑的列索引（逗号串；**空=全部列**）。
- 双击命中单元格（非列头/非滚动条/非 leadingControl）→ 若该列可编辑且非只读行/单元格 → 创建/定位编辑 EditBox：
  - 覆盖单元格内容矩形（`columnX(col)+padding`，行可见区域内；上下/左右裁剪）；
  - 文本=当前 cell 文本；字体/字号/颜色继承单元格/行/列样式（与绘制一致）；
  - 光标：建议全选或末尾（待定 Q1）；
  - 覆盖层位于行的绘制之上、ListView 裁剪区内。
- 提交：Enter 或失焦 → `setCell(row,col,text)`（更新显示与数据）→ `cell-edited`；
- 取消：Esc → 不写数据 → `cell-edit-cancel`；
- **滚动（滚轮/滚动条）**：立即取消并销毁（避免覆盖层位置漂移；Q2）；
- 列宽/列增删/行插入删除/排序/清空/刷新：立即取消并销毁；
- 控件隐藏/禁用/销毁：取消并销毁；
- 只读列/行/单元格：双击不进入编辑（是否需要事件通知？Q3）。
- 与单击/选择：双击不改变既有多选语义（或先按现有 item-click 选中该行；Q4）。

### 3.2 TreeView
- 开启：`label-editing=true`。
- 双击节点标签（非展开箭头/非图标区域）→ 覆盖 EditBox：
  - 覆盖节点文本矩形（含缩进后的文本起点；随 `scrollOffset`/缩进）；
  - 预填当前 label；字体/颜色继承节点样式；
  - 位于绘制之上、TreeView 裁剪区内。
- 提交：Enter/失焦 → `setNodeLabel(id,text)` → `node-edited`；
- 取消：Esc → `node-edit-cancel`；
- 滚动/展开/折叠/节点增删移/刷新/隐藏/禁用/销毁：取消并销毁；
- 只读节点（如分组/根）是否允许编辑：Q3。

## 4. 事件与数据契约（建议）

### 4.1 属性 / JSON
```json
// ListView
"cell-editing": true,
"editable-columns": "0,2"     // 逗号串；空=全部；运行时 SetString 同口径
// TreeView
"label-editing": true
```
- schema `$defs` 增键；解析/导出 round-trip；默认 false。
- 动态关闭（false）时立即取消当前编辑并清理。

### 4.2 事件
| 控件 | 事件 | 载荷 | 前值/新值 |
|---|---|---|---|
| ListView | `cell-edit-begin` | `GridPayload{row,col}` | **仅** `edit-old-text`=当前文本（new 无/空） |
| ListView | `cell-edited` | 同上 | `edit-old-text`=编辑前文本；`edit-new-text`=提交文本（数据已写回） |
| ListView | `cell-edit-cancel` | 同上 | **仅** `edit-old-text`=编辑前文本（new 无/空） |
| TreeView | `node-edit-begin` | `TreeNodePayload{id}` | **仅** `edit-old-text`=当前 label（new 无/空） |
| TreeView | `node-edited` | 同上 | `edit-old-text`=编辑前 label；`edit-new-text`=提交 label（已更新） |
| TreeView | `node-edit-cancel` | 同上 | **仅** `edit-old-text`=编辑前 label（new 无/空） |

- **前值/新值按事件按需**：begin/cancel 只带 oldText；edited 带 oldText+newText。双通道：
  1. `GridPayload`/`TreeNodePayload` **追加** `oldText`/`newText`（C 字符串，**仅回调期间有效**、宿主须拷贝；非编辑事件为 nullptr；绑定层加 `GetEditOldText/GetEditNewText`）；
  2. 同时提供只读字符串属性 `edit-old-text`/`edit-new-text`（begin/cancel 时 `edit-new-text` 为空；回调内 `GetString` 可读）。
- 不新增 CCallbackData 类型，复用现有 Grid/TreeNode（结构体追加字段；宿主/绑定同批重编）。
- 事件顺序：begin → 用户输入（不逐键发）→ edited 或 cancel；不再额外发 item-click/select（或保留既有语义，Q4）。
- 数据写回：edited 前已写 cells/label（宿主可读回；如需校验/回滚，Q5）。

### 4.3 程序化 API（C ABI + Binding）
```c
int UICornerstone_ListView_BeginEditCell(UIInstance, UIControlHandle, int row, int col);
int UICornerstone_ListView_CommitEdit(UIInstance, UIControlHandle);
int UICornerstone_ListView_CancelEdit(UIInstance, UIControlHandle);
int UICornerstone_ListView_IsEditing(UIInstance, UIControlHandle);   // + GetEditingRow/Col
int UICornerstone_TreeView_BeginEditNode(UIInstance, UIControlHandle, const char* id);
int UICornerstone_TreeView_CommitEdit(UIInstance, UIControlHandle);
int UICornerstone_TreeView_CancelEdit(UIInstance, UIControlHandle);
int UICornerstone_TreeView_IsEditing(UIInstance, UIControlHandle);   // + GetEditingId
```
- Binding 对应 `ListView::BeginEditCell/CommitEdit/CancelEdit/IsEditing` 等；
- 失败返回 0（越界/只读/未开启/不存在）。

## 5. 引擎实现建议

- **ListView**：复用现有 `cellControls` 的定位/滚动同步机制（或独立 overlay 但复用同一坐标/裁剪计算）；编辑 EditBox 作为临时控件挂到 ListView，`syncChildControls` 时只定位当前编辑；注意其尺寸不是图标 slot，而是单元格内容区（需要新增编辑专用定位）。
- **TreeView**：新增内部 EditBox 覆盖层（或复用 `cellControls` 思路），按节点行/缩进计算文本 rect；节点重排/展开时取消销毁。
- **双击识别**：引擎内做 (时间/距离阈值)，避免宿主各写一套；与单击选择/拖拽的先后顺序要定义（Q4）。
- **键盘**：覆盖 EditBox 内 Enter=提交、Esc=取消；Tab/上下左右不切换单元格（Q6）；失焦提交；IME/剪贴板走 EditBox 现有能力。
- **裁剪**：覆盖层必须在 ListView/TreeView 的 clip 内；单元格部分可见时建议只在完全/足够可见时允许编辑或裁剪显示（Q7）。
- **销毁/清理**：所有取消路径必须 `removeChild/Destroy` 覆盖层并清 editing 状态；不得留下焦点/悬停。
- **性能**：覆盖层按需创建、复用；不参与行绘制；不增加每帧分配。

## 6. 设计器接入（引擎交付后）

1. 事件面板 `kEventSpecs` 增加 6 个事件（类型 list-view/tree-view）；handler 绑定/导出/代码编辑沿用现有 F4 管线。
2. 属性面板增加 `cell-editing`（bool）、`editable-columns`（字符串行）、`label-editing`（bool）；导出/schema/保存打开。
3. 预览：双击编辑由引擎处理；设计器不叠加 EditBox；事件在预览/导出宿主生效。
4. 设计器自身的**事件页**仍为自定义行，不使用 ListView 覆盖编辑。

## 7. 验收矩阵

### ListView
1. `cell-editing=1`、`editable-columns="1,2"`：双击列 1→EditBox；Enter→cell 文本更新+`cell-edited`（Grid+`edit-text`）；Esc→不写+`cancel`；
2. 列 0 只读：双击不进入；
3. 失焦（点其它单元格/控件）→提交；
4. 滚动/列宽/行增删/排序/清空 → 取消并销毁，无覆盖层残留；
5. 空单元格/长文本/UTF-8/IME/剪贴板；全选或光标策略；
6. 控件隐藏/禁用/销毁 → 清理；
7. JSON/schema round-trip；C ABI/binding API；旧 DLL 空实现回退；
8. 原单击选择/多选/拖拽/列排序不回归。

### TreeView
1. `label-editing=1`：双击叶子/分支节点标签→EditBox；Enter→label 更新+`node-edited`（TreeNode+`edit-text`）；Esc→`cancel`；
2. 展开/折叠/滚动/节点增删移/刷新/隐藏/销毁 → 取消并销毁；
3. 缩进/图标/展开箭头不误触；根/只读节点策略；
4. 长文本/UTF-8/IME；C ABI/binding；JSON/schema；
5. 原选择/展开/拖拽排序不回归。

## 8. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q1 | 进入编辑时光标/全选 | 全选（便于替换） |
| Q2 | 滚动/列宽/行变更时提交还是取消 | **取消**（避免隐式写数据）；失焦/Enter 才提交 |
| Q3 | 只读行/节点/列 | 双击不进入；不额外发事件 |
| Q4 | 双击与单击选择/排序的先后 | 双击先按现有单击选中，再进入编辑；拖拽优先 |
| Q5 | 宿主校验/拒绝 | 本批先写回+事件；需要时后续加 validating 事件 |
| Q6 | Tab/方向键 | 不切换单元格；Tab 可作为失焦提交（或忽略） |
| Q7 | 部分可见单元格 | 建议允许编辑但覆盖层裁剪；或要求完整可见 |
| Q8 | 文本载荷 | begin/cancel **只带 oldText**；edited 带 oldText+newText。Grid/TreeNode 追加字段 + 只读 `edit-old-text`/`edit-new-text` 属性（双通道；绑定加访问器） |
