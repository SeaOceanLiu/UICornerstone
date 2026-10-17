# P093_094_DragDrop_Design — 通用拖拽框架（Drag & Drop）+ TabControl 运行时拖动排序

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-12 追加：P0-94 中、P0-93 中——P0-93 并入本框架）
> 前置：P0-92 批已实施并同步
> 状态：**已放行，已实施**（2026-10-12；Q-1~Q-6 确认；**采纳复核 §6 建议：Phase 1+2 合并一批**；补充 1~4 全部落实；Phase 3 归设计器）
> 复核：`requirements/P093_094_复核意见.md`（通过）
> 实施备注：① **Phase 1 核心**：实例级 `DragManager`（`UIContext.dragState`；Idle→Pending(阈值)→Dragging→Drop/Abort）；事件循环前置钩子 `DragPreHandleEvent`（`eventLoopEntry` 内 watcher 之前；拖动期消费 Move/Up/Esc；跨子视口经 `resolveMouseTarget` + 深命中）；注册键 `draggable/droppable/drag-threshold/drag-visual/drag-data/drag-userdata`（通用通道 + JSON common）；载荷 `UIDragPayload`（tree 节点/list 行/tab 页自动填充 + userData 透传；**MouseDown 捕获、拖动期不变**——补充 1）；落点 `UIDropHint`（before/after/into、row、index、inside）；事件 `drag-start/drag-over/drop/drag-end`（`drag-over` 内 `SetBool("drag-accept",0)` 拒绝；**目标/落点变化重置接受态**——补充 1）；默认视觉（ghost + 指示线，主题三色取自源控件，`drag-visual` 可关）；`drag-active` 只读（补充 4；`drag-hint` 快照未做，注记后续）；② **Phase 2 内置**：`reorderable`（tree/list/tab）——Tab 换序（moveTab + `tab-reordered`）、Tree 换序/换父（`moveNode` 完整子树 + `reordered`）、List 行换序（`moveRow` + `reordered`）；**MoveNode/MoveRow 同步公开**（CABI `TreeViewMoveNode`/`ListViewMoveRow` + Binding——补充 2）；③ **附带引擎修复（重要）**：`fireCCallback` 首参句柄原为 `ControlImpl*` 强转——`ControlImpl` **virtual 继承** `Control` 子对象地址不同（探针实测偏移 0x488），宿主回调内用该句柄调 CABI 会崩溃（本批探针抓到 `SetBool("drag-accept")` 段错误）→ 修为 `static_cast<Control*>(this)`（**修复影响全部既有回调**）；另修 manager ListView `hitTestRow` 需内容局部 y 的坐标域；④ 验证：**P0-93/94 探针 8/8**——阈值内点击不触发/核心拖动（载荷 kind=tree 节点、over INSIDE、drop、end dropped=1、drag-active=1）/拒绝（accept=0 → 无 drop）/Esc 中止/tab 换序 BCA+事件/list 换序 r2 首+事件/公开 move API/tree 换序 n1 首+事件；⑤ 全量构建 0 错误、回归仅 4 项预存；⑥ 用户手册：capi/binding/properties/declarative-syntax 已刷新；schema common 增 5 键；⑦ **宿主侧**：设计器 Phase 3（控件树/画布/结构行拖动）后续批；跨实例（工具箱→画布）宿主自管（框架 `drag-active` 查询）

---

## 1. 现状核实

| # | 事实 | 依据 |
|---|---|---|
| ① 无通用拖拽 | `draggable/droppable/reorderable/DragPayload/onDrop` 全库零命中；现有拖拽均为控件私有：Splitter/Slider/ScrollBar（beforeEventHandlingWatcher 模式）、WinFrame 标题栏拖动、TabControl 页签拖动为**宿主侧实现** | grep / src/Splitter.cpp:341-347 |
| ② 可复用基建 | 事件队列 + `beforeEventHandlingWatcher`（Splitter 三事件注册模式）；**P0-67 鼠标捕获**（`UIContext.mouseButtonDown`/`activeViewport`）；坐标路由 `resolveMouseTarget`（含浮层优先/子视口换算）；控件几何（`getDrawRect`/`hitTestTab` 等） | UICornerstoneAPI.cpp:320-395 |
| ③ 事件 ABI 可扩展 | `UIEventData` union 现含 float/int/str/ptr/selection/treeNode/color/grid——**新增 `drag` 成员为向后兼容扩展** | include/UICornerstoneAPI.h:813-827 |
| ④ P0-93 现状 | TabControl 无任何运行时拖拽代码（仅按下选中/悬停）；设计器宿主侧实现（TabGetItemRect+TabMovePage）✓；导出应用无换序能力 ✗ | src/TabControl.cpp |
| ⑤ 数据变更 API | Tab：`moveTab` ✓（P0-91）；TreeView：Add/Remove/Clear 节点 ✓、**无 MoveNode**；ListView：Add/Insert/Remove 行 ✓、**无 MoveRow**（Phase 2 需引擎 helper） | API 头 |

## 2. 架构（Phase 1：核心框架）

### 2.1 DragManager（实例级）
- 挂 `UIContext`（owner 实例一份）；状态机：`Idle → Pending（按下候选，等阈值）→ Dragging → Drop/Abort`；
- **启动**：源控件（`draggable=true`）在 MouseDown 命中后进入 Pending，注册 owner 队列 Move/Up watcher；Move 超过**拖动阈值**（默认 4px，属性可调）→ Dragging；
- **单实例（含子视口）**：拖动开始后 Move/Up 由 **owner 层**统一接管——owner watcher 处理；子视口内事件经 P0-67 捕获仍进 vp 队列 → vp `eventLoopEntry` 未消费回退链转发 owner（既有机制）→ 目标解析用 `resolveMouseTarget(owner, x, y)`（跨视口坐标域既有换算）；
- **Abort**：Esc（KeyDown watcher）或窗口失焦 → 发 `drag-end(dropped=false)`。

### 2.2 注册与载荷
- 属性（CABI/Binding 通用 SetBool/GetBool；JSON 键入 common）：
  - `draggable`（bool；缺省 false）——**引擎自动载荷**：按控件类型填充（TreeView 命中节点/TreeNode、ListView 命中行/ListRow、TabControl 命中页/TabPage）；通用控件填充 Control（自身句柄）；
  - `droppable`（bool；缺省 false）；
  - `drag-threshold`（float，缺省 4）；
  - HostData：`drag-data`（string）+ `drag-userdata`（ptr，经 SetPtr）——宿主自定义载荷透传。
- **载荷 ABI**：
  ```c
  typedef struct {
      int kind;              /* UIDragKind: Control/TreeNode/ListRow/TabPage/Text/File/HostData */
      const char* id;        /* 元素 id（引擎缓冲，回调期内有效） */
      int index;             /* 元素索引（行/页） */
      const char* text;      /* 文本（可选） */
      void* userData;        /* 元素 userData / 宿主指针（透传） */
      UIControlHandle source;/* 源控件句柄 */
  } UIDragPayload;
  ```

### 2.3 语义化落点（UIDropHint）
| 目标类型 | 落点语义 |
|---|---|
| TreeView | `before / after / into`（+ 目标节点 id；按命中行内上/中/下三带） |
| ListView | `row`（行索引） |
| TabControl | `index`（页索引） |
| 通用控件 | `inside` |

`UIDropHint { int kind; int index; const char* targetId; }`——宿主收语义而非像素。

### 2.4 事件（C ABI 回调名 / JSON events 键同族）
| 事件 | 侧 | 载荷 |
|---|---|---|
| `drag-start` | 源 | payload |
| `drag-over` | 目标 | payload + hint（每次目标/落点变化触发；用于高亮） |
| `drop` | 目标 | payload + hint |
| `drag-end` | 源 | payload + dropped 标志 + 最终 hint |
- **接受/拒绝**：Q-2 二选一——(a) `drag-over` 回调内 `SetBool(ctl, "drag-accept", 0/1)`（零 ABI 变更，建议）；(b) 新增带返回值回调类型。默认接受；
- JSON：`events.onDragStart/onDragOver/onDrop/onDragEnd`（RegisterAction 同族）。

### 2.5 视觉
- 引擎默认：拖动预览 ghost（载荷文本/类型图标，跟随光标）+ 落点指示（TreeView 插入线/`into` 高亮、ListView 行线、Tab 页签插入标记、通用外框高亮）；
- 主题化：`drag-ghost-background` / `drag-ghost-text` / `drag-indicator-color`（单色，属性键；Q-5）；
- 宿主覆盖：Phase 1 提供"关闭默认视觉"属性（`drag-visual` false）即可，自绘能力 Phase 3 另议。

### 2.6 跨实例边界
- Phase 1 **仅单实例**（含子视口）；跨实例（工具箱→画布）保持宿主自管；框架提供 `IsDragging` 查询（只读属性 `drag-active`）。

## 3. Phase 2：内置行为（opt-in，零配置可用）
- **TreeView**：节点换序/换父（引擎内部 MoveNode helper：Remove+Insert 保持展开/选中状态；落点 before/after/into 语义执行）；事件 `reordered`（from/to 语义）；
- **ListView**：行换序（MoveRow helper）；
- **TabControl（P0-93）**：`reorderable` 属性（bool，缺省 false）→ 引擎内置拖动（页签按下→阈值→目标位高亮→释放 `moveTab`）+ 事件 **`tab-reordered`**（from/to）——宿主据此同步模型；
- 内置行为均发**语义事件**，数据迁移由引擎执行；宿主只处理模型/撤销。

## 4. Phase 3（设计器接入，后续批次）
- 控件树拖动换父/换序、画布内控件拖动迁移到统一通道、结构行（menus/tabs/items）拖动编辑；设计器保留编辑语义（模型/撤销/树同步）。

## 5. 测试计划
- **Phase 1**：探针——注册/阈值（未超阈值=点击语义不冲突）/载荷自动填充（tree 节点/list 行/tab 页含 userData 透传）/跨子视口拖动与落点/目标拒绝（drag-accept=0 不发 drop）/Esc 与失焦 Abort/默认视觉像素（ghost+指示线）/`drag-active` 查询；
- **Phase 2**：三控件换序探针（数据顺序断言 + 语义事件 from/to）+ P0-93（`reorderable` 拖动 moveTab + `tab-reordered`）+ 回归；
- 全量回归（仅 4 项预存）+ 设计器联测（Phase 1 后评估宿主拖动迁移）。

## 6. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 本批范围：**仅 Phase 1 核心**；P0-93/Phase 2 下一批（框架先行稳定）？或 Phase 1 末尾并入 Tab 内置换序？ | **Phase 1 独立批次**（框架 ABI 先经复核/联测；P0-93 随 Phase 2 首个消费者落地） |
| Q-2 | 接受/拒绝机制：`drag-over` 内 `SetBool("drag-accept")`（零 ABI 变更）vs 带返回值回调 | 前者（建议） |
| Q-3 | 载荷 id/text 生命周期：引擎实例缓冲（回调期有效）vs 调用方拷贝 | 引擎缓冲（与 GetString 同款语义） |
| Q-4 | JSON 键：`draggable`/`droppable`（common）+ Phase 2 `reorderable`（tab/list/tree 各自 def） | 确认 |
| Q-5 | 默认视觉主题键：`drag-ghost-background`/`drag-ghost-text`/`drag-indicator-color` | 确认 |
| Q-6 | Phase 1 跨实例不支持（仅单实例含子视口）；`drag-active` 只读查询 | 确认 |

---

## 7. 开发参考（实现细节，2026-10-15 补充）

> 面向后续开发/维护/扩展的落地参考；与源码一一对应（文件:行区间以当前实现为准）。

### 7.1 模块与文件地图
| 文件 | 内容 |
|---|---|
| `include/DragDrop.h` | 引擎内部钩子声明：`DragPreHandleEvent` / `DragRenderOverlay` / `DragRelease` / `DragQueryActive` / `DragSetAccept`（前向声明 `UIContext`/`Control`/`Event`） |
| `src/UICornerstoneAPI.cpp`（"P0-94 通用拖拽框架"块，位于 `UICornerstone_RenderOverlays` 之前） | `struct DragState` + 全部管理器逻辑（见 7.3）；`RenderOverlays` 首行调 `DragRenderOverlay`；`DestroyInstance` 调 `DragRelease` |
| `include/UIContext.h` | `void* dragState`（owner 持有，惰性创建；子视口经 `owner` 解析） |
| `include/ControlBase.h` / `src/ControlBase.cpp` | 控件标志成员（`m_draggable/m_droppable/m_reorderable/m_dragVisual/m_dragThreshold`）+ 只读访问器（`isDraggable()` 等）；`setBoolProperty/getBoolProperty`（拖拽键）、`setFloatProperty/getFloatProperty`（`drag-threshold`）、`setColorProperty/getColorProperty`（视觉三色）；`CCallbackData::Drag` + `DragEventBundle`；`fireCCallback` Drag 分支；**`eventLoopEntry` 首行前置钩子** |
| `include/UICornerstoneAPI.h` | ABI：`UI_DRAG_KIND_*` / `UI_DROP_HINT_*` / `UIDragPayload` / `UIDropHint` / `UIEventData.data.drag`；换序公开 API 声明 |
| `include/PropertyNames.h` | 属性/事件/主题常量（7.5） |
| `src/LayoutParser.cpp` | `parseCommonProperties` 通用键解析（draggable/droppable/reorderable/drag-threshold/drag-visual） |
| `docs/schema/declarative-ui.schema.json` | common 5 键（draggable/droppable/reorderable/drag-threshold/drag-visual） |
| `include/TreeView.h` / `src/TreeView.cpp` | `hitTestRow`（公共，屏幕坐标）、`getNodeAtRow`、`getParentId`、`getChildIndex`、`moveNode`（完整子树迁移 + flat 重建 + 选中行重定位） |
| `include/ListView.h` / `src/ListView.cpp` | `hitTestRow`（公共，**内容局部 y**）、`moveRow`（vector 迁移 + clampSelection + rebuildLayout） |
| `include/TabControl.h` / `src/TabControl.cpp` | `moveTab`（P0-91；完整 TabPage 迁移 + currentIndex 重映射） |
| `binding/`（DynamicApi/UICornerstone.h/.cpp） | `TreeViewMoveNode` / `ListViewMoveRow` / `TabMovePage` / `TabGetItemRect` 封装 |

### 7.2 状态机与数据流
```
MouseDown（命中 draggable 祖先）
  → Idle→Pending：记录 source/起点/阈值/视觉开关；captureDragPayload（**此刻捕获，拖动期不变**）；不消费
MouseMove
  ├─ Pending 且 dist<阈值：不消费（点击语义保留）
  └─ 跨阈值 → Dragging：fire drag-start(源)
       → findDropTarget(owner, x, y)（resolveMouseTarget 跨子视口 + deepHitControl 深命中 + 向上找 droppable）
       → computeDropHint（按目标类型：tree 三分带 / list 行 / tab 插入位 / 通用 INSIDE）
       → 目标或落点变化：accept=true 重置 + fire drag-over(目标)
       → 消费 Move
MouseUp
  ├─ Pending：取消（不消费 → 普通点击）
  └─ Dragging：accept 且 target → fire drop(目标) → applyBuiltInReorder → dropped=1
       → fire drag-end(源, dropped) → 状态复位；消费
Esc（KeyDown）/ FocusLost：Dragging → fire drag-end(dropped=0) → 复位（Esc 消费）
```
- **钩子位置**：`ControlBase.cpp::TopControl::eventLoopEntry` 事件弹出后、`notifyBeforeEventHandlingWatchers` **之前**（拖动期优先于控件 watcher，如 ScrollBar/Splitter 不再收到拖动期 Move/Up）；owner/vp 两队列均调用，状态按 `owner = instance->owner ? owner : instance` 归并。
- **坐标域**：事件坐标为窗口全局；`resolveMouseTarget(owner,…)` 返回目标实例（owner/vp），`deepHitControl(inst->bench,…)` 逆序命中（含遮挡）；**TreeView::hitTestRow 收屏幕坐标**，**ListView::hitTestRow 收内容局部 y**（管理器已换算，勿混用）。

### 7.3 管理器关键函数（`src/UICornerstoneAPI.cpp`）
| 函数 | 职责 | 注意点 |
|---|---|---|
| `dragOwnerOf/dragStateOf` | owner 归并 + 惰性状态 | 子视口销毁时 `DragRelease` 若源属该视口则复位（防悬垂） |
| `deepHitControl` | 逆序深命中（visible+contains，先到先得=上层优先） | 与 `ControlImpl::handleEvent` 的遮挡语义一致 |
| `captureDragPayload` | 自动载荷：tree 节点 / list 行 / tab 页；`drag-data/drag-userdata` 覆盖为 HOST | 指针指向 `DragState` 内 `std::string`（回调期有效） |
| `computeDropHint` | 语义落点 | tree 三分带用 `fmod(localY, rowHeight)` **近似**（未含滚动/头偏移；如需精确可改按行 rect） |
| `findDropTarget` | 目标解析（跨视口） | 命中后向上找最近 `droppable`；允许 target==source（内置换序） |
| `fireDragEvent` | 组 `DragEventBundle` → `fireCCallback(CallbackData::Drag)` | 事件名常量 `kEventDrag*` |
| `applyBuiltInReorder` | Phase 2 内置：tab→`moveTab`+`tab-reordered`；tree→`moveNode`（hint→parent/index 映射）+`reordered`；list→`moveRow`+`reordered` | 门槛：source==target 且 `reorderable`；事件载荷 from=`payload.index`、to=`hint.index` |
| `DragRenderOverlay` | ghost（源主题三色 + 惰性 14px 字体）+ 指示线/高亮 | 仅 owner 渲染；`drag-visual=false` 跳过；指示线用光标位置近似 |

### 7.4 hint → 内置操作映射
| 目标 | hint | 操作 |
|---|---|---|
| TreeView | BEFORE/AFTER | `parent = getParentId(targetId)`；`idx = childIndex(target) + (AFTER?1:0)`；**同父且源在目标之前 → 再 -1**（摘除源位移补偿，P0-96 修正）→ `moveNode(id, parent, idx)`；自落点（target==源）无操作 |
| TreeView | INTO | `moveNode(id, targetId, -1)`（追加） |
| ListView | ROW | `moveRow(from, hint.index)`（to=最终索引） |
| TabControl | INDEX | `hint.index`=摘除前插入槽；`to = (from < ins) ? ins-1 : ins`（摘除后最终索引，P0-96 修正），clamp `[0, n-1]`；`moveTab(from, to)`；`from==to` 不动作 |
| 通用 | INSIDE | 仅发 drop（宿主自理） |

### 7.5 常量速查（`include/PropertyNames.h`）
- 属性：`kDraggable="draggable"` / `kDroppable` / `kReorderable` / `kDragThreshold` / `kDragVisual` / `kDragData` / `kDragUserData` / `kDragAccept` / `kDragActive`；主题：`kDragGhostBackground` / `kDragGhostText` / `kDragIndicatorColor`
- 事件：`kEventDragStart="drag-start"` / `kEventDragOver` / `kEventDrop` / `kEventDragEnd` / `kEventReordered="reordered"` / `kEventTabReordered="tab-reordered"`
- ABI：`UI_DRAG_KIND_{CONTROL,TREE_NODE,LIST_ROW,TAB_PAGE,TEXT,FILE,HOST}`；`UI_DROP_HINT_{INSIDE,BEFORE,AFTER,INTO,ROW,INDEX}`

### 7.6 重要实现修正记录（防回归）
1. **`fireCCallback` 首参句柄**：`ControlImpl` **virtual 继承** `Control`（子对象地址不同，实测偏移 0x488）——原 `reinterpret_cast<UIControlHandle>(this)` 传出的句柄**不能**用于 CABI（宿主回调内 `SetBool` 即段错误）；已修为 `static_cast<Control*>(this)`。**影响全部既有回调**，勿回退。
2. **ListView::hitTestRow 坐标域**：收内容局部 y（含滚动、不含屏幕偏移）——管理器调用前必须 `(y - drawRect.top)/scaleY`。
3. 拖拽事件 JSON 键（`events.onDrag*`）**未实现**（CABI/Binding `SetCallback` 为主；JSON 仅注册属性键）——如宿主需 JSON 事件绑定，扩展点：`parseEvents` 增映射（可参考 `onSelect` 模式）。
4. **自目标语义（P0-96）**：`reorderable` 自含（无需 `draggable`/`droppable`）；自目标松手会同时发 `drop`（target=源自身）与 `reordered`/`tab-reordered`——宿主按 `hint` 区分，勿重复迁移数据；纯 reorderable 未命中可重排项（tab 页体/树·列表空白）不起拖。
5. **hint→索引换算（P0-96 修正）**：`moveNode`/`moveTab` 均为"摘除后插入"语义——同父源在目标前时需补偿摘除位移；自落点（BEFORE/AFTER/INTO 自身）不动作、不发事件。

### 7.7 扩展指南
- **新增内置消费者**（如"列宽/列序拖动"）：在 `applyBuiltInReorder` 增分支（门槛 `reorderable` + source==target + kind 匹配）+ 对应引擎迁移方法（参考 `moveRow` 的"先校验后变更 + 重建布局"模板）；事件复用 `reordered` 或新增专用名。
- **宿主自绘预览**：`drag-visual=false` 关闭默认视觉；宿主经 `drag-active` 轮询 + 自身状态绘制（Phase 3 设计器画布拖动迁移方案）。
- **跨实例**（工具箱→画布）：框架不支持；宿主自管（可在 `drag-start` 记录载荷，自行完成目标侧交付）；`drag-active` 仅只读查询。
- **Backlog**：`drag-hint` 只读快照（复核补充 4，未做）；ghost 图标/自定义渲染回调；tree 三分带精确行 rect；OS 文件拖入（SDL_DROPFILE 桥接，Phase 2+ 另议）。

### 7.8 测试矩阵（Temp/temp_p093_094_probe，8/8）
| # | 用例 | 断言 |
|---|---|---|
| 1 | 阈值内点击 | 无 drag-start/drop |
| 2 | 核心拖动（tree→panel） | drag-start(kind=TREE_NODE)/drag-over(INSIDE)/drop/drag-end(dropped=1)/drag-active=1 |
| 3 | 目标拒绝（drag-accept=0） | 无 drop；drag-end(dropped=0) |
| 4 | Esc 中止 | drag-end(dropped=0)，无 drop |
| 5 | Tab 内置换序 | `tab-reordered` + 顺序 BCA |
| 6 | List 内置换序 | `reordered` + 行序 r2,r3,r1 |
| 7 | 公开换序 API | `TreeViewMoveNode`=1 / 非法 `ListViewMoveRow`=0 |
| 8 | Tree 内置换序 | `reordered` + 行序变化 |
