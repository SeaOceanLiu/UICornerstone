# P038_46_VisualRound3_Design — 视觉联测第三轮问题确认与修改方案（P0-38 ~ P0-46）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-27 视觉测试第三轮，P0-38 ~ P0-46）
> 前置：P0-32~37 批已实施并同步（能力矩阵、item 级四态、pressed 链路等）
> 状态：**已放行，已实施**（2026-09-28）
> 复核：`requirements/P038_46_VisualRound3_Design_复核意见.md`（通过，放行实施；§4 七问全部确认）
> 实施：P0-38~46 全部落地 + 全量回归（仅 4 项预存失败：menu_cabi / treeview_cabi / multiviewport / multiviewport_cabi【发布版 DLL 对照证实预存】）+ 验收探针 14/14 PASS
> 实施备注：① P0-40① JSON 侧 `thickness` 同步对齐（parseScrollBar → rect 短边）；② P0-39 valueLabel 色键经属性通道（JSON slider def 不含 `label` 键，设计器经 SetColor 通道）；③ P0-45 新增样本 `layouts/p0_context_menu.json`（ctest layouts_strict 覆盖）+ schema version 1.2.0；④ P0-41 定稿仅显式 `setBorderVisible(false)`（不做重绘）

---

## 1. 问题确认（源码核实）

| # | 报告 | 核实结论 | 根因 |
|---|---|---|---|
| P0-38 | ProgressBar textLabel 只在 label 自身 hotRect 生效 | **确认**。`ProgressBar::setState` 已实现（一次性转发），但 **Label 自身可交互**（`m_clickable=true`）：其 handleEvent 在自身 hotRect 内自管 hover、离开即置 Normal → 覆盖父同步；且父态不变时不再重同步 | 事件驱动同步 + label 自管态 |
| P0-39① | shadow.offset.\* 读回缺省 0（应 2.0） | **确认**。offset 读写直连 `m_valueLabel`（未启用 show-value-label 时 label 为空 → 读回 0 / 写入丢弃） | 无独立存储，缺省值丢失 |
| P0-39② | valueLabel 拿不到滑块 hover 态 | **确认**。`Slider::setState` override **未实现**（P0-32~37 设计 §3.6 漏项，仅落了 pressed 链） | 同 P0-38 模式缺失 |
| P0-39③ | text/text-shadow 的 disabled 不生效 | **确认**（同 ②：标签态无同步） | 同上 |
| P0-40① | 厚度无法被外观手柄拖动调整 | **部分确认**：`ScrollBar::setRect` 尊重厚度（track=整 rect，按朝向取 width/height）——手柄拖动若经 SetRect 应生效；疑为父布局回写/手柄路径未走 SetRect | 待联测定位 + 补 `thickness` 属性入口 |
| P0-40② | 轨道 background 色无法呈现 | **确认现象**：控件底色（beforeDraw）被 **track/thumb 全区域自绘覆盖**；但**轨道/滑块色键已存在**（`track` / `thumb` / `thumb-hover` / `thumb-pressed`，ScrollBar.cpp:379-382） | 面板应映射 track/thumb 键；控件级 background 对全自绘控件语义弱 |
| P0-41 | WinFrame 关闭按钮自身绘制边框（小正方形） | **确认（更正）**。关闭按钮为 Button（`borderVisible=false`，代码/像素双证无自绘边框），但创建时被设为**不透明灰底**（`setTransparent(false)` + `setBackgroundStateColor` 灰 0x50/0x60/0x40）→ 在浅蓝标题栏上形成灰色小方块；且按钮紧贴窗框右上角，恒显的 WinFrame 边框（ctor `setBorderVisible(true)`）在 `afterDraw` 覆盖其顶/右边缘（实测默认 0x60 灰、设色后变边框色）→ 两因叠加呈现"小正方形围着关闭按钮" | ①按钮不透明灰底 ②窗框边框压过按钮顶/右缘 |
| P0-42 | Splitter colors.background 不受控 | **确认**。`Splitter::draw`：`beforeDraw()`（背景）后**立即用把手线色铺满整框**（Splitter.cpp:59-62）→ 背景被全覆盖 | 自绘把手线覆盖背景 |
| P0-43 | ListView 控件级 colors.border 不生效 | **确认**。ListView 缺省 `borderVisible=false`；且**单态 border 键不自动开启**（对象路径 `setBorderStateColor` 会开启，单态 `setNormalStateBDColor` 不会）→ 设色不显示 | 单态/对象路径行为不一致 |
| P0-44 | MenuBar background.hover/disabled 不生效且无法读回 | **确认双因**：① Bar 填充恒用 `m_bgColor`（未按状态解析；应：无悬停项时 hover→hoverBg、disabled→disabledBg）；② 未 override `getStateColorProperty` → `UICornerstone_GetStateColor("background")` 读回基类空值（MenuBar 专用成员未组装） | 绘制与读回均未接状态 |
| P0-45 | ContextMenu 无动态行 | **确认**。schema 无 `context-menu` def（动态区 defs.contains 失败）；解析器也无独立 `type:"context-menu"` 分支（:344 仅为控件级 context-menu 键创建路径） | 缺 def + 缺解析分支 |
| P0-46 | font 运行时接口形态 | **确认**。`font` 仅 Enum 通道（setEnumProperty）；设计器已生成 `SetString("font")` 行 → 写入失败（基类 setStringProperty 返回 0） | 缺 string 通道别名 |

## 2. 修改方案

### 2.1 P0-38 / P0-39②③：内部 Label 态同步（决策：非交互 + 持续同步）
- `ProgressBar`：`createTextLabel` 后 `m_textLabel->setClickable(false)`（Label::handleEvent 对非 clickable 直接 return，不再自管态）；`ProgressBar::update`（或 draw 前）**每帧** `m_textLabel->setState(getState())`（保留 setState override 即时同步）。
- `Slider`：新增 `void setState(ControlState) override` → `m_valueLabel->setState(state)`；valueLabel 创建后 `setClickable(false)`；`Slider::update` 每帧同步。
- 效果：hover/pressed/disabled 三态由控件（滑块）统一驱动，任意鼠标位置一致（P0-38/P0-39②③ 一并解决）。

### 2.2 P0-39①：Slider 阴影偏移独立存储
- 新增成员 `float m_shadowOffsetX = 2.0f; float m_shadowOffsetY = 2.0f;`（缺省对齐 Label 语义 2.0）；
- `setFloatProperty(kShadowOffsetX/Y)` → 更新成员 + 若有 label 转发；`getFloatProperty` → 返回成员（label 有无均可读回）；
- valueLabel 创建时应用成员值（`setShadowOffset`）。

### 2.3 P0-40：ScrollBar 两处
- ① 新增 `thickness`（Float）属性：垂直 → `m_rect.width`、水平 → `m_rect.height`（含 `calculateTrackRect/ThumbRect` 重算）；供外观手柄绑定；同时**联测定位**手柄路径（若父布局回写需设计器配合）。
- ② 面板映射既有 `track` / `thumb` / `thumb-hover` / `thumb-pressed` 色键（引擎无需新增）；文档注明"ScrollBar 为全自绘控件，外观以色键 track/thumb 为准，控件级 background 仅提供底板"。

### 2.4 P0-41：WinFrame 关闭按钮去边框（按评审意见定稿）
- **只做一处**：关闭按钮创建处（WinFrame.cpp:60 后）显式 `m_closeButton->setBorderVisible(false)` —— 消除按钮自身的边框线，仅余灰底 + 黑十字 + 窗框边线。
- **不做**常态透明/去灰底；**不做**按钮重绘于边框之上（评审明确：窗框边框本就应该压过按钮顶/右边缘，保持现状）。
- hover/pressed 状态图（红底方块）保持不变；不改素材。

### 2.5 P0-42：Splitter 背景驱动把手线（决策方案 a）
- `colors.background` 三态映射为**把手线颜色**：normal→`m_colorNormal`、hover→`m_colorHover`、disabled→新增 `m_colorDisabled`（缺省=normal）；
- 专用键 `line` / `line-hover` / `line-drag` 保留且**优先**（显式专用键 > 通用组）；
- `draw` 的线色解析顺序：dragging→`m_colorDrag`；disabled→`m_colorDisabled`；hovered→`m_colorHover`；else `m_colorNormal`；
- 备选方案 b：把手线缩进（上下/左右各留 2px）露出背景——观感变化大，不推荐。
- border ⊘ 不变（schema 已不声明）。

### 2.6 P0-43：单态边框键自动开启 borderVisible（全局对齐）
- `ControlImpl::setNormalStateBDColor/Hover/Pressed/Disabled` 统一追加 `setBorderVisible(true)`（与对象路径 `setBorderStateColor` 一致，"设色即显示"）；
- 影响面：所有控件（Label/CheckBox 等缺省无边框控件若显式设 border 色将显示边框——语义一致）；需像素回归确认既有用例未受影响。

### 2.7 P0-44：MenuBar 状态绘制与读回
- **绘制**：Bar 填充按状态解析——`disabled → m_disabledBgColor`；`hover 且无悬停项（m_hoveredIndex < 0）→ m_hoverBgColor`；else `m_bgColor`（悬停项时填充保持 normal，避免与 item hover 底色叠加）；底部线同样按态（已实现）。
- **读回**：MenuBar/MenuPanel 补 `getStateColorProperty`/`getStateColorProperty(kBorder)` 组装（normal/hover/disabled；pressed 取 hover 或 active 语义）；`UICornerstone_GetStateColor` 全链可读。

### 2.8 P0-45：context-menu schema def + 解析分支
- schema 新增 `context-menu` def：`type` / `visible`? / `colors:$colors-basic2`（两态）/ `border-visible` / `items`?（结构另行）——按 Popup 能力矩阵两态；
- 解析器新增 `type == "context-menu"` 分支：复用 :344 的 `make_shared<ContextMenu>(parent, 1.0f, 1.0f)` 路径，**挂父、保持隐藏**（运行时经 SetPtr("context-menu") 绑定或 show）；与控件级 context-menu 键语义一致；
- 动态区（设计器）即可由 def 驱动生成行。

### 2.9 P0-46：font string 通道别名（基类统一）
- `ControlImpl::setStringProperty`：`kFont` → 委托 `setEnumProperty(kFont, value)`；
- `ControlImpl::getStringProperty`：`kFont` → 委托 `getEnumProperty(kFont, out)`（`FontNameToString` 返回静态串，读回安全）；
- 全控件受益（已实现 Enum 通道者自动获得 string 别名）；设计器无需改动。

## 3. 验收测试

| # | 验收 | 方式 |
|---|---|---|
| 1 | ProgressBar/Slider：hover/pressed/disabled 文本态随控件（非 label hotRect）一致 | 探针（鼠标移入/按下/禁用 + 文本像素） |
| 2 | Slider shadow.offset 读回缺省 2.0；设置后读写一致；label 消失/重建不丢 | 读写回 |
| 3 | ScrollBar thickness 属性读写 + 视觉厚度；track/thumb 色键呈现 | 探针像素 + 读写回 |
| 4 | WinFrame 关闭按钮自身无边框线（灰底保留；窗框边线照常压其顶/右边缘） | 捕获像素（按钮左/下缘无独立边线） |
| 5 | Splitter background 三态驱动线色；line 专用键优先 | 探针像素 |
| 6 | 设 `border` 色即显示边框（ListView 等）；对象路径行为不变 | 探针像素 + 既有回归 |
| 7 | MenuBar background.hover（无悬停项）/disabled 呈现 + GetStateColor 读回 | 探针 + 读写回 |
| 8 | `type:"context-menu"` 布局可加载（隐藏）；schema 动态行生成 | test_layout 扩展 + validate |
| 9 | SetString("font") 写入成功、GetString("font") 读回枚举名 | 读写回 |
| 10 | 全量回归（既有预存失败除外） | 全量扫描 |

## 4. 待审核问题

1. **P0-42**：确认"colors.background 三态驱动把手线颜色、line* 专用键优先"方案（备选缩进方案不推荐）？
2. **P0-43**：确认全局"设边框色自动开启 border-visible"（含 Label/CheckBox 等缺省无边框控件；语义=设色即显示）？
3. **P0-45**：确认 context-menu def 两态 + 解析分支（独立声明挂父、隐藏）方案？`items` 结构是否本批纳入（或随结构化编辑推进）？
4. **P0-40①**：`thickness` 属性形态（Float；垂直→width / 水平→height）确认？手柄拖动若为父布局回写导致，需设计器侧配合（联测定位）。
5. **P0-41**（已按评审意见定稿）：仅显式 `setBorderVisible(false)`；保留灰底、保留窗框边框压过按钮顶/右边缘。
6. **P0-46**：确认 string 通道返回枚举名（与 GetEnum 同源）？
7. **P0-44**：确认 hover 填充规则（仅无悬停项时用 hoverBg；悬停项时 Bar 填充 normal）？

## 5. 配套改动

- schema（context-menu def）+ 手册（6.3.1 矩阵补 context-menu 行、Splitter 线色说明、ScrollBar track/thumb 说明、font string 通道说明）。
- C ABI/Binding：无新增（thickness 走通用 Float 通道；font 别名走基类）。
- 探针/回归；requirements/ 归档；复核意见归档后 make_release 同步 CornerstoneDesigner。
