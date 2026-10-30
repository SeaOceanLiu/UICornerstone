# P107_TreeRowHit_Design — TreeView 行命中 API（右键操作光标行）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-21 追加：P0-107 中）
> 前置：P0-94（`hitTestRow` 坐标域修正）/ P0-105（CABI 暴露批次）已实施
> 状态：**已放行，已实施**（2026-10-21；复核：`requirements/P107_TreeRowHit_Design_复核意见.md`——可批，附实现约束）
> 实施备注（2026-10-21）：① **共享 helper** `TreeView::hitTestRowInContent`（内容区扣除滚动条、与绘制裁剪 `cr` 同口径）——MouseDown 已切换共用（防口径漂移）；② **缓冲语义**：CABI 返回所需长度（含 NUL，snprintf 式；outBuf=NULL 仅查询；`>outSize-1` 表示截断）；Binding **先查后取**（null 长度查询 → 分配 → 取值），`fnTreeViewGetNodeAt` 缺失（旧 DLL）返回空串不崩溃；③ 纯查询（不改选中/悬停/展开）；④ 验证：探针 `temp_p107_probe` **9/9**——基本命中（含缩进区 x）/ 行下方空白未命中 / 纯查询保持选中 / 折叠·展开后正确 / **滚动条区域未命中** / 滚轮滚动后顶部行变化 / **缩放 1.5 命中** / 空树+非法句柄 / **cap 过小返回所需长度 44 且前缀 NUL 不静默截断**；P0-93/94~105 探针全过；全量回归 51 OK + 仅 4 项预存；Debug/Release 零警告。

---

## 1. 需求与现状核实

**需求**：设计器控件树右键菜单（删除/上移/下移/移到画布根）需作用于**光标所在行**；当前引擎 `TreeView` 仅处理左键 MouseDown 选中，右键不选中且无行命中查询 → 设计器只能用"上一次选中项"，右键另一行时动作落错控件。

**核实**：

| # | 事实 | 依据 |
|---|---|---|
| ① TreeView 仅左键选中 | `TreeView::handleEvent` MouseDown 判定 `MouseButton::Left`；右键（Right）无选中逻辑 | `src/TreeView.cpp:446` |
| ② 行命中 C++ 已具备 | `hitTestRow(mx, my)`：**窗口全局坐标**，含滚动偏移（`+ m_scrollOffset`）与行距（stride）；`getNodeAtRow(row)` 取节点（含折叠/缩进后的 flat 行） | `src/TreeView.cpp`（hitTestRow/getNodeAtRow） |
| ③ C ABI 无行命中 | 仅 `TreeViewSelectNode` / `GetSelectedId` / MoveNode 等；无"坐标→节点"查询 ✗ | `include/UICornerstoneAPI.h` |

## 2. 方案（建议 ①）

### 2.1 新增 CABI（只读查询）
```c
/* P0-107：按窗口坐标查行节点 id（含滚动/缩进/折叠态；行间空白返回 0）。
   outId 缓冲（建议 128B，超长截断且保证 NUL）；返回 1=命中 0=未命中/参数非法。 */
UICORNERSTONE_API int UICornerstone_TreeViewGetNodeAt(UIInstance instance, UIControlHandle tree,
    float x, float y, char* outId, int cap);
```
- 引擎 impl：`validateControl` → `TreeView*` → `hitTestRow(x, y)`（窗口坐标，与 P0-94 口径一致）→ `getNodeAtRow(row)` → `id` 拷贝（截断+NUL）；
- Binding：`std::string TreeViewGetNodeAt(Control& tree, float x, float y)`（未命中返回空串；动态 API 解析 + 封装）。

### 2.2 右键选中（Q-1，建议本批不做）
- 备选 ②：引擎右键 MouseDown 直接选中行（语义变更，影响所有 TreeView 右键场景）；建议**不做**——设计器可用 `GetNodeAt` 获取光标行后按需 `TreeViewSelectNode`（选中语义显式、可控）✓。

### 2.3 事件/选择语义
- `GetNodeAt` **不改变选中**（纯查询）；右键菜单流程 = `GetNodeAt(光标)`（得到目标 id）→ 显示菜单 → 动作作用于该 id ✓。

## 3. 验收与测试矩阵

1. **基本命中**：右键第 N 行 → `GetNodeAt` 返回该行节点 id（N=0/中间/最后一行）；
2. **滚动后命中**：设置 scroll-offset 后，同一点返回当前可见行（含 `m_scrollOffset`）✓；
3. **缩进/折叠**：折叠子节点后其行消失（返回相邻可见行）；展开后恢复 ✓；
4. **空白/越界**：行间空白（树内无行处）/ 树外坐标 → 0（binding 空串）；`cap` 过小 → 截断且 NUL；非法句柄 → 0；
5. Binding 封装回归；全量回归仅 4 项预存。

## 4. 影响面

- 源码：`include/UICornerstoneAPI.h`（声明）、`src/UICornerstoneAPI.cpp`（impl）、`binding/include/UICornerstone.h` + `binding/src/UICornerstone.cpp` + `binding/src/DynamicApi.*`（封装）；
- 无 JSON/schema 变更；`TreeViewGetNodeAt` 为只读查询（不影响既有交互语义）；
- 文档：capi/binding 速查（新增行）、用户手册 treeview 页（右键命中示例）、API_Mapping_Table 补行；
- 设计器：随批实施右键菜单（重命名/删除/复制/粘贴/上移/下移/移到根）。

## 5. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 方案 ①（只读 GetNodeAt）vs ②（右键自动选中） | **①**（非侵入；选中可用现成 `TreeViewSelectNode` 显式完成） |
| Q-2 | 坐标口径：窗口全局坐标（与 `hitTestRow`/P0-94 一致） | 确认 |
| Q-3 | `outId` 缓冲约定（128B 建议、截断 + NUL） | 确认 |
