# CR20261117002 — TreeView 行内编辑器控件（引擎侧分析 + 设计）

> 状态：**已实施并交付（设计 v4 通过；引擎项 closed；设计器联调待完成）**
> 来源：`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_2026-11.md` → CR20261117002
> 关联：`TreeView`（`TreeNode` / `leadingControl` / 行布局 / 滚动 / 命中）、`LayoutParser`、C ABI/Binding
> 边界：本文档只做引擎侧分析与设计；设计器属性面板迁移不在本文范围，评审通过后实施。

## 1. 现状与约束

- `TreeNode` 已有 `leadingControl`（行前置控件）与 `leadingGap/leadingAlign`，但**没有行尾/行内编辑控件槽**；
- 所有行使用统一 `m_rowHeight`，`getStride()` 等步长计算贯穿绘制/命中/滚动/`ensureSelectedVisible`/`nodeLabelRect`；
- `m_scrollOffset` 为像素，但行位置按 `row * stride` 计算，无法支持逐节点行高；
- 行控件集合 `m_rowControls` 负责绘制排除与挂/摘；节点删除/清空时统一摘除；
- 标签绘制无列宽概念，文本直接绘制；无截断/列裁剪；
- 事件路径：行前置控件命中时选中行并返回 false，让子控件分发处理；双击编辑需避开前置控件。

CR 目标要求“行内编辑控件槽 + 逐节点行高 + 动态增删不丢状态”，必须先把行几何从“统一步长”改为“逐行 top/height 缓存”。

## 2. 目标与非目标

### 2.1 目标

1. `TreeNode` 支持 `editorControl`（行内编辑控件，借用语义，树不拥有）；
2. 支持 `editorWidth` / `editorGap` / `editorAlign`；
3. TreeView 支持 `labelWidth`（0=按行自适应，>0=固定标签列）；
4. `TreeNode::rowHeight` 逐节点行高（0=继承 TreeView `row-height`）；
5. 绘制/滚动/命中/选中可见/双击编辑/`GetNodeRect` 全部使用逐行几何；
6. 编辑器控件随行绘制、随树内容区裁剪；折叠/隐藏时摘除，展开时恢复；
7. 点击编辑器不触发行选中/展开/折叠/双击编辑，事件优先给编辑器；
8. 未使用 `editorControl` / `rowHeight` 时视觉与行为完全兼容；
9. 不新增 JSON 持久化格式（V1 编辑器控件由宿主/C ABI 挂接）。

### 2.2 非目标

- 不实现属性面板/PropertyGrid 控件；
- 不把编辑器控件写入 JSON/`schema`（V1）；
- 不做单元格合并、多列编辑器、列宽拖拽；
- 不改变 TreeView 现有标签编辑（`beginEditNode`）语义；
- 不实现虚拟化行池/增量重排（V1 全量 `rebuildFlatRows` 可接受）。

## 3. 设计

### 3.1 数据模型

`TreeNode` 新增（默认值保证旧行为）：

```cpp
std::shared_ptr<Control> editorControl;     // 行内编辑控件；借用（C ABI 用 no-op deleter 包装），树不拥有
                                            // cloneNode 不复制；宿主销毁前必须先解绑；节点删除只摘除
float editorWidth = 0.0f;                   // 0=占满剩余宽度；>0=固定宽（局部 px）
float editorGap = 6.0f;                     // 标签列与编辑器之间
AlignmentMode editorAlign = AlignmentMode::AM_MID_LEFT; // 水平分量：left/center/right；垂直恒居中
float rowHeight = 0.0f;                     // 0=继承 TreeView row-height
```

TreeView 新增：

```cpp
float m_labelWidth = 0.0f;                  // 0=按行自适应；>0=固定标签列宽
std::vector<float> m_rowTops;               // 每行 top（内容坐标，不含滚动）
std::vector<float> m_rowHeights;            // 每行有效高度
float m_contentHeight = 0.0f;               // 累计到末行行底（不含末行后 spacing）
```

- `editorControl` 生命周期与 `leadingControl` 一致：树只 `addControl` 挂接/摘除，不销毁；节点删除/清空时先摘除；
- 同一节点同时有 `leadingControl` 与 `editorControl`：leading 在标签左，editor 在标签右，互不重叠；
- `editorControl == nullptr` 时布局与现状完全一致。

### 3.2 行几何与滚动

`rebuildFlatRows()` 末尾统一计算：

```
top = 0
for each row:
    h = node->rowHeight > 0 ? max(node->rowHeight, minRowHeight) : m_rowHeight
    m_rowTops[i] = top
    m_rowHeights[i] = h
    top += h + m_lineSpacing
m_contentHeight = max(0, top - m_lineSpacing)
```

- `viewH` 为逻辑高度（与 `m_rect.height` 一致）；`m_scrollOffset` 保持像素语义，范围 `max(0, m_contentHeight - viewH)`；
- 每行**槽高** `slotH[i] = rowHeight[i] + lineSpacing`（末行不含 lineSpacing）；背景/命中/首行判定使用 slot，标签/编辑器使用实际行高；
- 首行（B1）：`it = upper_bound(tops, offset)`；
  - `it == begin` → 第 0 行；
  - 否则 `cand = it - 1`；若 `offset < tops[cand] + slotH[cand]` → cand，否则 `it`（offset 落在行尾 spacing）；
  - **不能直接返回 upper_bound**（offset=0 会跳过第 1 行、行内会跳过当前行）；
- 命中（B1）：同一算法 + 必须包含在 `[top, top+slotH)`；落在 spacing 时归属上一行，保证与旧版“gap 可选中”一致；
- **坐标口径（B6）**：`m_rowTops/m_rowHeights/m_scrollOffset/m_contentHeight/m_labelWidth/editorX` 均为**逻辑坐标**；`cr`/`drawRect` 为设备坐标。
- 绘制：行设备 y = `cr.top + (m_rowTops[i] - m_scrollOffset) * scaleY`；
- 行设备 x = `cr.left`；行内元素设备 x = `cr.left + (logicalX - m_hScrollOffset) * scaleX`；
- `ensureSelectedVisible`：用 `m_rowTops[row]` 与 `m_rowHeights[row]`；
- `nodeLabelRect` / `GetNodeRect` 使用同一缓存；
- `updateScrollBar`：`contentH = m_contentHeight`；
- 键盘 Home/End/PageUp/PageDown 等导航继续按行索引，滚动改用 helper `ensureRowVisible(index)`。

`getStride()` 保留但仅用于“无 editor/无逐节点行高”的兼容路径；新代码不再用它计算行位置。

### 3.3 标签列与编辑器槽布局

设内容区 `cr`、行 top `y`、有效行高 `h`、行字体高 `fontH`。

1. **标签起点**：与现状一致（`LEFT_PADDING + depth*indent + arrow + arrowGap + leading 槽宽 + leadingGap`）。
2. **标签列宽（B2 定案）**：
   - `m_labelWidth == 0`：每行按实际文本测量宽度（`labelW`），编辑器紧跟文本 + `editorGap`；
   - `m_labelWidth > 0`：**全局列**，从内容区逻辑左边起算，忽略 depth/arrow/leading：
     - 重建时按内容逻辑坐标（不含 hScroll）计算 `maxLabelStart = max(rows, labelX)`；
     - `editorColumnX = maxLabelStart + m_labelWidth + editorGap`；
     - 标签裁剪到 `[0, maxLabelStart + m_labelWidth)`；
     - 编辑器设备 x = `cr.left + (editorColumnX - m_hScrollOffset) * scaleX`；
   - `labelWidth` 是**标签文本列宽**（不是“列 x 坐标”）：最深缩进行仍获得完整 `labelWidth` 文本空间，所有行共用同一编辑列（B7/T21）；
   - 浅层行可用文本空间 = `maxLabelStart - labelX + labelWidth`（全局裁剪到编辑列前），V1 不强制每行只 `labelWidth`；文档/T3 注明；
   - 固定列宽小于文本宽度时：硬裁剪（V1 不省略号），文档注明。
3. **编辑器区域**：
   - 可用宽 = 内容区右边界逻辑值 - 右 padding - editorColumnX（`max(0, …)`）；
   - `editorWidth == 0`：占满可用宽（忽略控件原 rect 宽）；`>0`：取 `min(editorWidth, 可用宽)`；
   - 高：控件 rect 高 >0 时 `min(rect.h, rowHeight)`，否则取 rowHeight；负值按 0；宽/高最终为 0 时跳过绘制但仍挂接；
   - 水平对齐按 `editorAlign`：`left/top-left/mid-left/bottom-left` → 左，`center/top-center/mid-center/bottom-center` → 中，`right/*-right` → 右；`editorWidth==0` 时对齐无效果；
   - 垂直居中：`y + (h - editorH)/2`；负宽/负 gap 钳制 0，行高最小 1；
   - `editorH` 取控件自身 rect 高度，钳制到 `h`；控件高 <=0 时回退行高；宽度统一由上述规则计算，不回退控件原宽；
- `labelColumnRect` 在 pushClipRect 前按 `cr`/scale 转设备坐标（逻辑列宽 * scaleX + cr.left）。
4. **横向滚动（N7）**：标签与全局编辑器列都**随内容横向滚动**（含 `-m_hScrollOffset`），不做固定列；需要固定属性面板列时由宿主关闭树横向滚动或单独布局；
5. **绘制顺序**：行背景/边框 → leading → 标签文本 → editor（在树裁剪区内）→ 其他子控件（排除行控件）；
6. **裁剪**：编辑器与标签都受树内容区 `pushClipRect(cr)` 裁剪；固定标签列额外 `pushClipRect(labelColumnRect)`。

### 3.4 挂接、折叠与生命周期

- 复用 `syncRowControls()`：收集**全部展开行**（`flatRows`，不仅是可见行）的 `leadingControl` 与 `editorControl`，统一挂/摘；滚出视口仅裁剪绘制，不摘除，避免滚动时焦点/文本状态丢失（B5）；
- 折叠/隐藏：编辑器 `removeControl` 但不销毁；若编辑器持有焦点，焦点交回 TreeView（不自动提交，值由宿主/编辑器自身维护）；展开/恢复时重新挂接；
- 同一控件被多个节点引用：按 `flatRows` 顺序首次挂接生效，后续节点跳过并 `Platform::Log` 一次性诊断；
- 编辑器与 leading 都参与 `m_rowControls` 绘制排除，防止同帧双绘；
- 节点删除/`clearItems`：先摘除编辑器再销毁节点（与 leading 一致）；
- 编辑器控件原有 parent/rect 由宿主决定；TreeView 挂接时设置局部 rect（相对树内容区），不改变其 `shared_ptr` 所有权；
- 同一控件不可同时挂到两个节点；检测到重复时保留首次挂接并一次性诊断。

### 3.5 事件路由

- `MouseDown` 命中 editor：不选中行、不切换展开、不进入双击编辑；返回 false，让 `ControlImpl::handleEvent` 分发给子控件；
- `MouseMove`：编辑器区域不改变行 hover（或保持行 hover，由编辑器自行处理 hover）；V1 保持行 hover 但点击不选中；
- `MouseUp`：双击编辑判断先排除 editor；
- 键盘焦点（B4）：编辑器控件作为树子控件参与现有焦点/Tab 链；点击 editor 时请求其焦点（若 focusable）；
  - 当 editor 持有焦点时，`Up/Down/Home/End/Delete/Enter` 等**先交给 editor**；TreeView 仅在 editor 未消费时处理行导航/展开；
  - Tab 继续走全局焦点链，不把焦点强制拉回树；
- 滚轮在 editor 上：默认交给 editor（如 ComboBox）；未消费时冒泡到 TreeView 滚动；
- 编辑器失焦/IME：TreeView 不自动提交/回滚，由宿主或编辑器自身事件处理；文档注明。

### 3.6 C ABI / Binding

新增 C ABI（均 `instance + tree + node id` 定位）：

```c
int UICornerstone_TreeViewSetNodeEditorControl(UIInstance, UIControlHandle tree,
        const char* id, UIControlHandle editor);              // NULL=解除
int UICornerstone_TreeViewSetNodeEditorLayout(UIInstance, UIControlHandle tree,
        const char* id, float width, float gap);
int UICornerstone_TreeViewSetNodeEditorAlign(UIInstance, UIControlHandle tree,
        const char* id, const char* align);                   // 9 值字符串，仅取水平分量（left/center/right）
int UICornerstone_TreeViewSetNodeRowHeight(UIInstance, UIControlHandle tree,
        const char* id, float height);                        // 0=继承
int UICornerstone_TreeViewSetLabelWidth(UIInstance, UIControlHandle tree, float width);
int UICornerstone_TreeViewGetNodeRect(UIInstance, UIControlHandle tree,
        const char* id, float* x, float* y, float* w, float* h);
```

- Binding 封装同名方法；缺失符号时按既有 DynamicApi 规则返回失败；
- 属性系统别名（与 leadingControl 同风格）：
  - `item-editor-control`（`SetPtr/GetPtr`，配合 `item-id`）
  - `item-editor-width` / `item-editor-gap`（Float）
  - `item-editor-align`（Enum）
  - `item-row-height`（Float，0=继承）
  - `label-width`（树级 Float）
- `GetNodeRect` 语义（N5/T17/T23）：
  - 返回**整行可见矩形（不含行距）**，逻辑坐标（已含双向滚动，不含树 drawRect 偏移/缩放）：
    `x = -m_hScrollOffset`、`y = rowTop - m_scrollOffset`、`w = 内容区宽（扣纵向滚动条）`、`h = 有效行高`；
  - 设备坐标由宿主换算：`sx = cr.left + x*scaleX`、`sy = cr.top + y*scaleY`；
  - 折叠节点的子行、非法 id、`tree/id=nullptr`、空树 → 返回 0；所有非 NULL 输出置 0；
  - `x/y/w/h` 任一输出指针为 NULL → 返回 0，其余非 NULL 也置 0；
  - 滚出视口但已展开 → 仍返回矩形（y 可为负/超出内容区）；
- 所有新 C ABI：无效 instance/tree/id/越界返回 0，不崩溃；NULL editor 解除；宽度/高度/间距负值按 0 钳制，`rowHeight=0` 继承。

### 3.7 JSON / schema

- 新增 `tree-view.label-width`（Float，0=自适应）；
- `row-height` 已是现有键，逐节点 `rowHeight` 不做 JSON 持久化（编辑器控件由宿主挂接，V1 不导出）；
- 文档注明：`item-editor-*`/`item-row-height` 为运行时属性，不进入声明式 schema。

### 3.8 行几何失效与重建

| 变更 | 处理 |
|---|---|
| `setRowHeight` / `setLineSpacing` / `setIndentWidth` / 字体变化 | 重建 flatRows 几何（`rebuildRowMetrics`）并 clamp 滚动 |
| 节点 add/remove/clear、展开/折叠 | 重建 flatRows + 几何 + `syncRowControls` |
| `rowHeight`（节点） / 字体变化 | 重建行高几何（全量，V1） |
| `labelWidth` / editorWidth/Gap/Align | 只重算编辑列/编辑槽（`editorColumnX`、rect），不改行高；重绘即可；labelWidth=0↔固定列切换时重置 editor rect |
| `SetNodeLeadingControl` / leading 控件尺寸变化 | 重算 `maxLabelStart` 与 `editorColumnX`/editor rect；不改行高 |
| `moveNode` / `setItems` 替换 / 清空/空树 | 重建 flatRows + 几何 + `syncRowControls`；空树 contentHeight=0、scroll=0 |
| 树 rect/缩放变化 | 重新计算滚动条范围并 clamp；逻辑行几何不变 |
| 仅 `m_scrollOffset` 变化 | 不重建几何，只重绘/重新命中 |
| 编辑器控件自身 rect 变化 | 下一次绘制按槽位重新设置 rect |

## 4. 影响面与 ABI

| 层 | 影响 |
|---|---|
| C++ | `TreeNode` 增字段（结构体布局变化）；`TreeView` 增成员/方法 → **DLL/头文件/C++ 使用方需同批重编译**；无 vtable 变化（除新增 C ABI 无关） |
| C ABI | 新增 6 个函数；旧调用不受影响 |
| Binding | 对应 6 个封装 + DynamicApi RESOLVE；缺失符号返回 false |
| JSON/Schema | 仅 `label-width`；编辑器控件不持久化 |
| 设计器 | 属性面板按节点挂接 EditBox/ComboBox 等；无需 schema 编辑器字段 |
| 性能 | 行几何 O(n) 全量重建；绘制/命中 O(log n + 可见行)；编辑器控件数量与**展开行**相关（非仅可见行） |

## 5. 测试计划

新增 `test/test_cr_treeview_editor.cpp` + `layouts/cr_treeview_editor.json`，注册 CMake 并纳入 layouts 复制条件。

| # | 用例 | 断言 |
|---|---|---|
| T1 | 单节点 editor | 编辑器位于标签右侧、垂直居中；点击编辑器不选中行/不展开 |
| T2 | editorWidth=0 / >0 / editorAlign | 宽度与对齐正确；不超出内容区 |
| T3 | labelWidth=0 / 固定 | 自适应/固定列；固定列标签被裁剪不覆盖编辑器 |
| T4 | 逐节点行高 | 普通行 26、TextArea 行 80；绘制/命中/滚动范围正确 |
| T5 | 层级展开/折叠 | 3 层节点展开/折叠后编辑器位置正确，折叠时摘除、展开恢复 |
| T6 | 动态增删 | 增删节点/编辑器后其他行编辑器状态、焦点、滚动位置不丢 |
| T7 | ensureSelectedVisible + 滚轮 | 选中末尾行滚动到可见；滚轮范围按 `m_contentHeight` |
| T8 | 双击编辑/箭头 | 编辑器区域不触发行双击编辑/展开；箭头仍可切换 |
| T9 | GetNodeRect | 与绘制行矩形一致（含滚动偏移） |
| T10 | C ABI/Binding | 6 个新函数 + 属性别名读写；NULL 解除；无效 id 安全 |
| T11 | 兼容 | 未设置 editor/rowHeight 时绘制、命中、滚动与旧版一致 |
| T12 | 三后端 | SDL3/SFML/raylib 像素/矩形一致（边界容差 1px） |
| T13 | 缩放 | 树/编辑器 1.5×/2× 下行矩形与编辑器宽高正确 |
| T14 | 生命周期 | 节点删除/清空先摘编辑器，控件保活可复用 |
| T15 | 键盘焦点（B4） | 编辑器聚焦后 Up/Down/Home/End/Delete/Enter 由编辑器消费，树不抢；未消费时树才导航 |
| T16 | 编辑中滚动/折叠（B5） | 滚出视口编辑器仍挂接、焦点/文本保持；折叠时摘除、焦点回树、值不丢 |
| T17 | GetNodeRect 边界 | 折叠子行/非法 id/NULL/滚出视口/缩放坐标语义；失败置 0 |
| T18 | 跨层级固定列（B2） | labelWidth>0 时 depth 0/1/2 的编辑器左边界一致；标签裁剪不越列 |
| T19 | 尺寸钳制 | 负 width/gap/rowHeight/labelWidth、0 宽 editor、行高小于字体高时的表现 |
| T20 | 同控件多节点 | 首次挂接生效、后续诊断、不双绘；解绑后可挂到另一节点 |
| T21 | 深缩进/leading 列（B7） | 最深行标签仍至少有 `labelWidth` 文本宽（非零可见）；编辑器不压箭头/leading；各行同一列 |
| T22 | 横向滚动（N7） | 标签与编辑器列随内容滚动，行矩形/命中/GetNodeRect 含 hScrollOffset 一致 |
| T23 | 缩放一致性（B6） | 1.5×/2× 下绘制 y/x、命中、GetNodeRect 换算一致；无半行偏移 |

## 6. 用户手册与文档更新计划

| 文档 | 更新 |
|---|---|
| `docs/controls/treeview.html` | 行内编辑器槽、label-width、逐节点行高、事件路由、C ABI/Binding 示例 |
| `docs/appendix/declarative-syntax.html` | `tree-view.label-width` |
| `docs/appendix/properties.html` | `item-editor-*` / `item-row-height` / `label-width` 行 |
| `docs/appendix/capi.html` / `binding.html` | 6 个新函数 |
| `docs/schema/declarative-ui.schema.json` | `tree-view.label-width` |
| `README.md` | 特性/测试列表 |
| `design/API_Mapping_Table.md` | CR20261117002 行 |

截图：`treeview-inline-editor.png`（节点行 + EditBox/ComboBox/TextArea 编辑器、不同行高），由 `manual_shots` 布局生成。

## 7. 风险与回退

| 风险 | 处理 |
|---|---|
| 统一步长假设遗漏 | 全量替换为 `m_rowTops/m_rowHeights`；T1~T7 覆盖绘制/命中/滚动 |
| 行高变化导致滚动跳变 | 重建后 clamp `m_scrollOffset` 并按选中行重算；T7 |
| 编辑器焦点/事件抢行点击 | MouseDown 先判 editor；T1/T8 |
| 折叠摘除销毁控件 | 只 `removeControl` 不 reset；T5/T14 |
| 同一控件挂多节点 | 首次挂接 + 诊断；测试 |
| labelWidth 固定列裁剪文本 | 文档注明 V1 无省略号；T3 |
| C++ 结构布局变化 | 同批重编译声明 |
| 旧布局回归 | 未使用新字段走原逻辑；T11 |

## 8. 验收对照

| CR 验收项 | 设计落点 |
|---|---|
| 行内编辑控件槽 | §3.1/§3.3/T1~T2 |
| 标签列宽可配 | §3.3/T3 |
| 逐节点行高 | §3.2/§3.3/T4 |
| 动态增删不丢状态 | §3.4/T6/T14 |
| 滚动/展开/缩放跟随 | §3.2/§3.4/T5/T7/T13 |
| 点击编辑器不触发行操作 | §3.5/T1/T8 |
| 不破坏现有语义 | §2.2/§3.7/T11 |
| C ABI/Binding | §3.6/T10 |

## 9. 实施顺序

1. `TreeNode`/`TreeView` 数据结构 + 行几何缓存 + 滚动/命中改造；
2. `labelWidth`/`rowHeight` + 编辑器槽布局/绘制/裁剪；
3. 事件路由 + 挂接/折叠/生命周期；
4. C ABI/Binding + 属性别名；
5. 测试/布局样本/CMake；
6. 文档/截图/README/API 映射；
7. 三后端验证 → 设计器联调 → 关闭引擎项。

## 10. 待评审确认

- `labelWidth`（B2）采用“全局列”：从内容区左边起算，忽略 depth/arrow/leading；labelWidth=0 仍逐行自适应；
- 固定标签列超宽文本 V1 硬裁剪（不做省略号）；
- 编辑器控件不进入 JSON/schema，V1 仅运行时挂接；`item-editor-*`/`item-row-height` 为运行时别名，不导出；
- `editorWidth=0` 占满剩余宽度，此时 `editorAlign` 无效果；align 支持 9 值字符串，仅取水平分量；
- 行几何以 slot（行高+lineSpacing）参与滚动/命中，B1 首行/命中用 upper_bound 回退/包含判断；
- 6 个 C ABI 与 Binding 封装，返回语义见 §3.6；
- 坐标口径（B6）：行几何/滚动/列宽均为逻辑坐标，绘制时乘 scaleX/scaleY；GetNodeRect 输出逻辑坐标（含双向滚动）；
- `labelWidth` 为标签文本列宽：`editorColumnX = maxLabelStart + labelWidth + gap`；最深行标签仍完整可见（B7）；
- `editorWidth=0` 占满可用宽，**不**回退控件原宽；高度 0/负值回退行高（N4）；
- 标签/编辑器列随横向滚动（N7）。

## 11. 实施记录（2026-11-19）

### 11.1 落地文件

- `include/TreeView.h` / `src/TreeView.cpp`：
  - `TreeNode`：`editorControl/editorWidth/editorGap/editorAlign/rowHeight`；
  - TreeView：`m_labelWidth`、`m_rowTops/m_rowHeights/m_contentHeight/m_maxLabelStart`；
  - `rebuildRowMetrics`、`firstVisibleRow`（upper_bound 回退）、`rowAtOffset`、`editorColumnX`、`layoutRowEditor`；
  - 绘制/命中/滚动/`ensureSelectedVisible`/`nodeLabelRect`/`GetNodeRect` 改用逐行几何；`drawArrow` 按行高居中；
  - 编辑器槽绘制（树裁剪区内）、点击不选中/不展开、双击编辑排除编辑器；折叠摘除、展开恢复；
  - 属性别名：`item-editor-control/width/gap/align`、`item-row-height`、`label-width`。
- `include/UICornerstoneAPI.h` / `src/UICornerstoneAPI.cpp`：6 个新函数（SetNodeEditorControl/Layout/Align、SetNodeRowHeight、SetLabelWidth、GetNodeRect），NULL/非法语义按设计。
- `binding/`：DynamicApi + UICornerstone 封装 6 个方法。
- `src/LayoutParser.cpp`：`tree-view.label-width`。
- `docs/schema/declarative-ui.schema.json`：`tree-view.label-width`。
- `test/test_cr_treeview_editor.cpp` + `test/CMakeLists.txt`；`docs/assets/controls/treeview-inline-editor.png`。
- 文档：treeview.html 4.25.8、declarative-syntax/properties/capi/binding、README、API 映射。

### 11.1.1 代码 Review 修复（B8~B11）

- B8：标签裁剪宽度与 `nodeLabelRect` 列尾均扣除 `m_hScrollOffset`，横向滚动后长标签不再覆盖编辑器/双击矩形不偏宽。
- B9：`setFont/setFontSize/setArrowGap`、`item-leading-control` 设置后重建 `rebuildRowMetrics()`/滚动条，编辑列不陈旧。
- B10：PageUp/PageDown 改为按 `m_rowTops/heights + viewH` 逐行几何计算，不再用统一 `getStride()`。
- B11：行背景/选中/hover/边框使用 `rowSlotHeight(i)`（含 lineSpacing），与命中一致，消除行尾间隙无高亮。
- 测试 T24（横向滚动 GetNodeRect）/T26（变行高翻页）/T27（行槽背景）通过；三后端 29/29。

### 11.1.2 行内编辑器点击派发（CR20261119001）

- 根因：`TreeView::handleEvent` MouseDown 命中 `editorControl` 后 `return false`，事件回到父容器，父容器无法转交 TreeView 子控件。
- 修复：命中编辑器改为 `ControlImpl::handleEvent(event); return true;`；leadingControl 同样改为选中行后派发并消费。
- 测试 T28：点击 CheckBox 编辑器切换、EditBox 获得焦点、树选中行不变；三后端 29/29。

### 11.2 验证结果

- `test_cr_treeview_editor`：SDL3 / SFML / raylib 均 **29/29**（编辑器列/行高/GetNodeRect/命中/C ABI/非法 id/折叠节点）。
- `test_treeview`：Enhanced 14 / JSON 29 / Scale2x 13 全通过；SDL3 全量回归仅 4 项历史失败，无新增。
- `validate_layout --strict` PASS。
- 影响：`TreeNode`/`TreeView` 布局变化，DLL/头文件/C++ 使用方需同批重编译；C ABI 新增 6 函数，旧调用兼容。
- Release DLL SHA256：`102c260f24e5dc2867dfeabc5a52c0055eda79c232f7f98f829ef982b3d4d69e`；已同步设计器 `subModules/UICornerstone`（`/XF .git` 保护生效）。

## 修订记录

- 2026-11-19 v1：引擎侧首版设计；逐行几何缓存、编辑器槽、labelWidth/rowHeight、事件路由、6 个 C ABI；待评审。
- 2026-11-19 v2（按评审 B1~B5）：修正首行/命中 upper_bound 回退；labelWidth 定为全局列；明确借用/克隆/生命周期；键盘事件优先编辑器；全展开行挂接；补 T15~T20 与返回/钳制语义。
- 2026-11-19 v3（按复核 B6/N1~N7）：修正绘制 scaleY/scaleX 坐标口径；labelWidth 文本列宽 + maxLabelStart 防重叠；editorWidth=0 占满可用宽；GetNodeRect 逻辑坐标/NULL 输出语义；补失效表 moveNode/setItems/空树；明确横向滚动；补 T21~T23。
- 2026-11-19 v4（按复核 B7/N4）：labelWidth 定为标签文本列宽，`editorColumnX = maxLabelStart + labelWidth + gap`，最深行标签非零宽；删除“宽度为 0 回退原宽”残留；失效表补 leading 变化与列切换；T21 增加最深行标签可见断言。
- 2026-11-19 v5（设计通过）：补 labelWidth 语义/schema 文案、浅层行额外文本空间、viewH/contentHeight 定义、labelColumnRect 设备换算；进入实施。
- 2026-11-19 v7（Code Review B8~B11）：横向滚动裁剪、列重建、翻页几何、行槽背景修复；三后端 29/29；重新交付。
- 2026-11-19 v6（实施）：行几何/编辑器槽/6 C ABI/Binding/schema/文档落地；三后端 22/22；引擎项 closed。
- 2026-11-20 v8（CR20261119001）：修复行内编辑器 MouseDown 未派发；T28；三后端 29/29；重新交付。
