# CR20261101002 — TreeView 节点标签就地编辑（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261101002，原 P0-112）
> 提出方：CornerstoneDesigner
> 状态：**已闭环（closed）**（2026-11-02；设计器联测通过，CR「设计器/引擎」均为 closed）
> **兼容政策（2026-11-01 用户确认）**：引擎 DLL、Binding、宿主必须**同批配套**；不支持旧 DLL/旧宿主混用（与 P108 变体 A 一致）。以下“旧 DLL”条目均改为“版本不匹配 = 功能不可用”，不承诺功能回退。设计器 CR 原文中的旧 DLL 回退条款由**设计器同步修订**（引擎不代改）。
> 参考：`requirements/引擎ListView_TreeView编辑模式_设计建议.md`；共用机制见 `design/CR20261101001_ListViewCellEdit_Design.md`

---

## 1. 需求摘要

TreeView 打开 `label-editing` 后，双击节点标签 → 在节点文本位置叠加内部 EditBox（预填 label）；**只读节点仍可进入编辑，EditBox 以只读态展示（可选中/复制）**；Enter/失焦提交、Esc 取消；滚动/展开/折叠/节点增删移/刷新/隐藏/禁用/销毁时取消并清理；事件 `node-edit-begin` / `node-edited` / `node-edit-cancel`；C ABI + Binding 程序化 Begin/Commit/Cancel/IsEditing；JSON/schema/手册同步。

## 2. 现状与源码核实

- 节点：`TreeNode{id,label,expanded,children,...}`（`include/TreeView.h:18-38`）；`setNodeLabel(id,label)`（`include/TreeView.h:181`）。
- 行/命中：`hitTestRow` / `hitTestRowInContent`（`include/TreeView.h:158-160`，P0-107）；缩进/行高/箭头间距：`m_indentWidth/m_rowHeight/m_arrowGap`（`:116-119`）；滚动偏移与内容区裁剪见 `src/TreeView.cpp` 绘制/滚动实现。
- 事件：`select`（`TreeNodePayload{id,userData}`）、`expand`/`collapse`（String）、`reordered`；无编辑状态/无双击识别。
- EditBox：同 CR-001（`setOnEnter`、`onFocusLost` 可重写、原生不处理 Esc）。
- C ABI：`UICornerstone_TreeViewAddNode/SetNodeLabel/...`（`include/UICornerstoneAPI.h:431+`）；事件 `UIEventData.data.treeNode`（`:893`）。
- 行命中已扣除滚动条/表头（P0-107 的 `TreeViewGetNodeAt` 同口径），可直接复用缩进文本起点计算。

## 3. 分析（可行性 / 风险）

1. 覆盖层沿用 CR-001 的 EditBox 子控件方案；按 B2/R6 仅**完整可见**节点进入，**不依赖裁剪**；TreeView 需按"缩进后的文本起点 + 行 top - scrollOffset"定位。
2. 双击区域必须排除展开箭头/前置控件/滚动条；叶子/分支节点均可进入编辑；新增稀疏只读声明 `read-only-ids`（节点 id）——命中节点仍进入编辑但 EditBox 只读（见 Q3）。
3. 展开/折叠会改变行位置与可见性 → 必须立即取消会话；节点重排（P0-94）/增删移/刷新同理。
4. 事件载荷：复用 `TreeNodePayload` 并扩展 old/new；公共 C ABI 与 CR-001 一致采用 **union `data.treeNode` 扩展**（oldText/newText/editReadOnly，`UIEventData` 尺寸不变）。
5. 与现有单击选择/展开/拖拽排序的顺序需固定（Q4）。

## 4. 设计

### 4.1 属性 / JSON / Schema

| 属性键 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `label-editing` | Bool | false | 开启节点标签双击编辑；置 0 立即取消并清理 |
| `read-only-ids` | String | "" | 逗号分隔节点 id；命中节点仍可进入编辑，但 EditBox 只读、不写回 |

- `PropertyNames`：`kLabelEditing`、`kReadOnlyIds`；`parseTreeView` 增两键（`label-editing`/`read-only-ids`）；`schema $defs/tree-view` 同步两键（`additionalProperties:false`）；
- 只读属性 `edit-old-text`/`edit-new-text`（与 CR-001 同口径；待评审决定是否保留）。

### 4.2 交互与生命周期

- 进入：`label-editing=1` 且双击命中节点文本区域（排除箭头/前置控件/滚动条）→ 创建/复用内部 EditBox：
  - 文本 rect：行 `top = contentTop + (row - firstVisibleRow)*stride - scrollOffset`；`left = indent*(depth+1) + arrowGap + icon/leading 宽度`；宽=内容区右界-左界；
  - **可见性策略（评审 B2 + 复核 R6 定稿）**：仅节点行**纵向完整可见**、且**节点文本 rect 横向完整落在内容区**时进入编辑；部分可见/横向越界/不可见双击不进入（覆盖层不越出内容区，无需额外子控件裁剪）；
  - 预填 `node.label`；继承节点字体/字号/文字色（与绘制一致）；全选；焦点切换；
  - 只读判定：节点 id ∈ `read-only-ids` → `EditBox::setReadOnly(true)`（仍可聚焦/全选/复制，不可改文本）；
  - 记录 oldText → 发 `node-edit-begin`。
- 提交：Enter/失焦 → **只读会话统一按取消处理**（不写回，发 `node-edit-cancel`，old 仅 + `readOnly=1`）；可写会话 `setNodeLabel(id,new)` → 销毁 → 发 `node-edited`（old+new，`readOnly=0`）；
- 取消：Esc、**只读会话 Enter/失焦**、生命周期取消（见下）→ 不写数据 → 销毁 → 发 `node-edit-cancel`（仅 old；`readOnly` 随会话）；
- **取消并销毁**：滚动、展开/折叠、节点增删移、重排、刷新、隐藏/禁用、`label-editing` 置 0 → 发 `node-edit-cancel`；**控件销毁只清理不回调**；
- **焦点/Tab/销毁（与 CR-001 统一）**：Enter/Esc 后焦点回 TreeView（或原焦点）；点击其他控件失焦提交不抢回；Tab=失焦提交+焦点切换；销毁不留悬垂焦点；
- 与单击/拖拽：单击选择沿用；双击在第一击选择后进入编辑；拖拽/换序优先（Q4）。

### 4.3 C ABI / Binding

```c
int UICornerstone_TreeViewBeginEditNode(UIInstance, UIControlHandle tree, const char* id);
int UICornerstone_TreeViewCommitEdit(UIInstance, UIControlHandle tree);
int UICornerstone_TreeViewCancelEdit(UIInstance, UIControlHandle tree);
int UICornerstone_TreeViewIsEditing(UIInstance, UIControlHandle tree);           /* 1/0 */
int UICornerstone_TreeViewGetEditingId(UIInstance, UIControlHandle tree,
                                       char* out, int cap, int* outReadOnly);
```

- 失败：未开启（`label-editing=0`）/隐藏/禁用/未 create/节点不存在/会话不存在/**节点 id 为空**（复核 R3：不允许空 id 进入编辑，避免与“无会话”混淆）→ 0。
- `GetEditingId` 缓冲语义**统一采用 P107 风格**：返回所需完整字节长（`out=NULL/cap=0` 仅查长度），`out` 非空时截断 + NUL；`outReadOnly` 可空，仅在返回 >0 时有效（与 ListView `GetEditingCell` 口径一致）。**返回 0 恒表示“无会话”**（空 id 已被 Begin 拒绝），`IsEditing()` 可作权威判断。
- Binding：`TreeViewBeginEditNode/CommitEdit/CancelEdit/IsEditing/GetEditingId`（含 `outReadOnly`）；Event 访问器 `IsNodeEditBegin/IsNodeEdited/IsNodeEditCancel`、`GetEditNodeId/GetEditOldText/GetEditNewText`、`IsEditReadOnly`。
- 版本不匹配（缺入口）→ Binding 防御性失败，设计器提示/禁用功能，不做回退。

### 4.4 事件与载荷

- 常量：`node-edit-begin` / `node-edited` / `node-edit-cancel`（`kEventNodeEditBegin/kEventNodeEdited/kEventNodeEditCancel`）。
- 语义：`begin` 开启会话；`edited` 仅**可写会话**提交（已写回）；`cancel` = Esc / **只读会话 Enter/失焦** / 生命周期取消（未写回）。
- 内部 `TreeNodePayload` 扩展 `{const char* id; void* userData; const char* oldText; const char* newText; int readOnly;}`。
- 公共 C ABI（评审 B1 定稿，与 CR-001 同方案、**不增加顶层字段**）：扩展 union `treeNode` 成员（`id/userData/oldText/newText/editReadOnly`），`UIEventData` 尺寸不变；`fireCCallback` 同步拷贝；文本存会话成员，仅回调期有效。
- 只读属性双通道同 CR-001；指针仅回调期有效。
- 声明式：`onNodeEditBegin`/`onNodeEdited`/`onNodeEditCancel` 接线到 `setOnNodeEditBegin/Edited/Cancel`（签名含 `bool readOnly`；begin/cancel 的 new 为空串）。

## 5. Q1~Q8 引擎答复

与 CR-001 同口径（详见 `CR20261101001_ListViewCellEdit_Design.md` §5），差异：

| # | TreeView 补充 |
|---|---|
| Q2 | 滚动/展开/折叠/增删移/重排 → **取消并发 `node-edit-cancel`**；可写 Enter/失焦提交；只读 Enter/失焦=cancel |
| Q3 | **已确认：只读节点仍可进入编辑，EditBox 只读（可选中/复制）**；`read-only-ids` 稀疏声明；Enter/失焦统一 `node-edit-cancel`（不写回、`readOnly=1`） |
| Q4 | 第一击沿用选择；双击排除箭头/图标区域；拖拽换序优先 |
| Q7 | **按评审 B2 + 复核 R6 定稿：节点行纵向完整可见且文本 rect 横向完整落在内容区才进入编辑**；部分可见/横向越界/不可见双击不进入 |
| 附加 | 展开/折叠与编辑冲突时立即取消并**发 `node-edit-cancel`**（不发 edited）；Tab/焦点/销毁与 CR-001 统一；部分可见行不进入编辑（B2） |

## 6. 自动化测试计划（实施时必须落地）

- 新增 `test/test_treeview_node_edit_cabi.cpp`：
  1. `label-editing`/`read-only-ids` JSON/属性 round-trip；
  1b. 只读节点：进入编辑、EditBox 只读、全选/复制；Enter/失焦 → `node-edit-cancel`（不写回、`readOnly=1`，无 `node-edited`）；
  1c. ABI：`sizeof(UIEventData)`/union 偏移静态断言（尺寸保持 80）；
  1e. `GetEditingId`：空/非空 id、无会话/编辑中、`outReadOnly` 一致性；空 id Begin 失败；
  1d. 覆盖层边界（首/末/部分可见/横向不完整/滚动附近）；不可见或横向不完整不进入；焦点恢复/Tab/销毁不崩溃；
  2. Begin（叶子/分支/含缩进与前置控件）→ Commit：label 更新、`node-edited` old/new；
  3. Cancel/Esc：不写、`node-edit-cancel` 仅 old；
  4. 滚动/展开/折叠/增删移/重排/刷新/隐藏/禁用/销毁 → 取消且 `IsEditing==0`；
  5. 命中排除：箭头/图标/滚动条/行外；纵向完整可见 + 文本横向完整可见才进入；部分可见/横向不完整/不可见不进入；
  6. 双击阈值与拖拽换序优先；
  7. API 失败语义；UTF-8/长文本/空串；
  8. 版本不匹配防御：入口缺失时 Binding 安全失败（不承诺功能回退）。
- Binding 测试 `test/test_treeview_node_edit_binding.cpp`；探针 `Temp/temp_cr20261101002_probe.cpp`（合成事件，复用 P107/P110 探针骨架）；
- 全量回归（基线 51 OK + 4 项预存失败）。

## 7. 用户手册更新计划

- `docs/controls/treeview.html`：新增"节点标签就地编辑"章节（属性/事件/JSON/C ABI/Binding 示例）；
- `docs/appendix/declarative-syntax.html`：TreeView 示例加 `label-editing`/`read-only-ids`；事件键表加 3 键；
- `docs/appendix/properties.html` / `capi.html` / `binding.html`：属性、5 个 API、事件访问器（含 `editReadOnly`/`IsEditReadOnly`）；
- `docs/schema/declarative-ui.schema.json`：同步 + `--strict`；
- 示例运行验证：JSON strict + 探针；审计脚本 0 失败。

## 8. 影响面与兼容性

- 源码：`include/TreeView.h`、`src/TreeView.cpp`、`PropertyNames.h`、`LayoutParser.cpp`、`ControlBase.h/.cpp`（payload/UIEventData）、`UICornerstoneAPI.h/.cpp`；
- Binding：`Event.h/.cpp`、`UICornerstone.h/.cpp`、`DynamicApi.*`；
- ABI：与 CR-001 共用 **union 成员扩展（`data.treeNode`），`UIEventData` 尺寸不变**（实测 80 bytes）；版本必须配套，不支持旧 DLL 混用。

## 9. 验收矩阵

1. 开启/关闭、双击进入（仅完整可见）、Enter/失焦提交、Esc 取消；只读节点进入编辑且 EditBox 只读，Enter/失焦以 `node-edit-cancel` 结束、不写回、`readOnly=1`；
2. 滚动/展开/折叠/增删移/重排/刷新/隐藏/禁用/销毁清理；
3. 命中排除与可见性（仅完整可见进入）；事件载荷 old/new；API/Binding；
4. 选择/展开/拖拽排序不回归；探针+新测试+全量回归；手册示例可用。

## 10. 待评审确认

- 与 CR-001 共用的 Q1/Q6/Q8 答复；
- 已按 2026-11-01 评审修订：B1 union 扩展、B2 仅完整可见进入、`GetEditingId` 增加 `outReadOnly` 并统一 P107 缓冲语义、焦点/Tab/销毁/展开取消规则；
- 只读属性 `edit-old-text/edit-new-text` 保留（与事件同源）。

## 修订记录

- 2026-11-01 初稿。
- 2026-11-01 按设计器评审修订：B1 `data.treeNode` union 扩展（尺寸不变）、B2 仅完整可见进入、`GetEditingId` 增加 `outReadOnly` 并统一 P107 缓冲语义、焦点/Tab/销毁/展开取消规则。
- 2026-11-01 用户确认**版本配套政策**（不支持旧 DLL/旧宿主混用）；相关回退条目改为“版本不匹配安全失败/功能禁用”，union 尺寸不变仅作附加收益。
- 2026-11-01 复核：R3 空 id 禁止编辑（`GetEditingId` 0=无会话）；R6 增加文本 rect 横向完整可见要求；R4 Tab/焦点规则与 CR-001 统一。
- 2026-11-01 复核（R2）：设计器 CR 原文的旧 DLL 回退条款由设计器同步修订，引擎不代改。
- 2026-11-01 复核（终）：设计通过；按 R7 清理文档表述（仅完整可见、不依赖裁剪；删除 A 方案待确认项），进入实施。

## 实施记录（2026-11-01）

- TreeView 节点编辑、空 id/横向可见性、展开取消；测试 test_cr_cells / test_cr_ui；探针 cr_cells/cr_ui/cr_binding。
- 全量回归：53 OK + 4 项预存失败；探针 18 组全过；Schema strict 63 示例 0 失败；`UIEventData` 尺寸保持 80。
- 发布并同步 `subModules/UICornerstone`（make_release + robocopy /MIR）。

## 验收闭环（2026-11-02）

- 设计器联测通过；CR 状态「设计器：closed」「引擎：closed」。
- 相关设计/复核文档按归档规则可归档（文件保留在 `design/` 与 `requirements/`，不另移动）。
