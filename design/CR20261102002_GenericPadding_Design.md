# CR20261102002 — 通用控件内边距（content-padding）（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261102002）
> 提出方：CornerstoneDesigner
> 状态：**已实施（2026-11-03；待设计器联验）**——核心引擎/主题运行时/C ABI/Binding/schema/文档/测试已落地；全量回归 55 OK + 4 项预存失败（基线 54+新增 test_cr_theme）。
> 关联：CR20261102001 的 CSS `padding` 映射依赖本 CR 的通用属性。

---

## 1. 需求摘要

为全部公开控件提供统一内容内边距（`content-padding`，left/top/right/bottom），使 JSON、CSS-like 主题（CSS 名仍为 `padding`）与 C ABI/Binding 使用同一语义：内容按 contentRect 排布/裁剪；容器内边距作用于子控件；默认 0，旧布局零变化；与现有 `layout.padding`/Dialog 旧 `padding` 不冲突、不重复叠加。

## 2. 现状与源码核实

- `ControlImpl` 已有 `m_margin`/`setMargin/getMargin/getMarginedRect`（`include/ControlBase.h:283,343,485-487`；`src/ControlBase.cpp:541-558`），**没有通用内容内边距**。
- 容器布局（HFlow/VFlow/Grid/Anchor）已有 `layout.padding`，在 `apply*` 中从容器 rect 扣除（`src/LayoutEngine.cpp:50-64,120-149`）；`Panel::reflowChildren` 直接传 `m_rect`（`src/Panel.cpp:75-82`）。
- 文本控件各自用内部偏移绘制文本；`Label::setMargin` 同时影响文本布局（`src/Label.cpp:515-520`）。
- ListView/TreeView 使用 `m_rect` 计算表头/行/滚动条（如 `src/ListView.cpp` columnX/visibleStartRow；`src/TreeView.cpp` draw/hit）；无内容区内边距。
- **`PropertyNames::kPadding = "padding"` 已存在且 Dialog 以 Float 使用**（`setPadding(float)`，JSON 数字，`src/Dialog.cpp`）；ColorPicker 弹窗内部有独立 padding 常量。
- 结论：通用键必须**避开 `padding`**，否则与 Dialog 类型/语义冲突（复核 B）。

## 3. 分析（可行性 / 风险）

1. **属性键隔离**：通用内边距使用 `content-padding`（四边），Dialog 保留 `padding`（Float）不变；CSS-like 的 `padding` 声明在主题层映射到 `content-padding`，用户 CSS 名称不变。
2. **几何统一**：以 `contentRect = rect 内缩 content-padding` 作为内容/子控件布局基准；背景/边框仍绘制外框，命中范围保持外框。
3. 控件绘制路径差异大：文本、列表、树、页签、图像需要分别接入 contentRect。
4. **容器叠加**：控件 `content-padding` 为外层，`layout.padding` 为内层（Panel 传 contentRect 给布局引擎，布局引擎再扣自身 padding），不重复。
5. **Dialog 兼容**：Dialog 旧 `padding` JSON/API/行为完全不变；Dialog 的通用 `content-padding` 独立，默认 0，不自动等同于旧 `padding`；若宿主同时设置，两者叠加（Dialog 内容先扣旧 padding，再由通用 contentRect 裁剪/布局，见 Q9）。
6. **滚动/命中**：ListView/TreeView 滚动条保持外缘；内容区（表头/行/节点）用 contentRect；行命中与 `GetNodeAt` 同步扣 content-padding。
7. **超尺寸/负值**：负值 clamp 0；超过尺寸时 contentRect 归零并诊断，外框不变。
8. **旧布局零回归**：默认 0；旧 JSON 不产生 content-padding；Dialog 旧 padding 不变。

## 4. 设计

### 4.1 属性/JSON/API

- `ControlImpl` 新增 `content-padding`（四边 float，默认 0；复用 `Margin` 结构）。
- JSON（仅通用控件 **新键**；`padding` 仍归 Dialog 旧语义，不参与通用解析）：
  ```json
  "content-padding": { "left": 12, "top": 6, "right": 12, "bottom": 6 }
  "content-padding": 6                 // 四边相同
  "content-padding": "6 12"            // v h
  "content-padding": "6 12 6 12"       // t r b l
  ```
  - 数字/字符串解析；`px` 后缀可选；负值 → 0 + 诊断；非法格式 → 该键失败（诊断，保持默认）。
- 属性系统：键 `content-padding`（`kContentPadding`）：
  - `GetString("content-padding")` 返回规范化 `"t r b l"`；`SetString` 解析；
  - 通用 `SetFloat("content-padding-left/top/right/bottom")` 可选（新增 PropertyNames）。
- C ABI：
  ```c
  int UICornerstone_SetContentPadding(UIInstance, UIControlHandle,
                                      float left, float top, float right, float bottom);
  int UICornerstone_GetContentPadding(UIInstance, UIControlHandle,
                                      float* left, float* top, float* right, float* bottom);
  ```
- Binding：`Control::SetContentPadding(left,top,right,bottom)` / `GetContentPadding(float& ...)`；同时支持 `SetString("content-padding","6 12")`。
- CSS-like（CR20261102001）：声明名仍为 `padding`，映射到通用 `content-padding`；状态伪类不适用（诊断忽略）。
- **Dialog**：旧 `padding`（Float）不变；`SetPadding/GetPadding` 与 `"padding":28` 行为保持；通用 `content-padding` 默认 0，不自动复用旧值。
- **唯一通用命名**：C ABI/Binding/JSON 均为 `SetContentPadding/GetContentPadding`、`content-padding`；CR2001/CR2003 统一引用此名。

### 4.2 几何模型

- **contentRect（局部逻辑坐标）**：`left=content-padding.left, top=content-padding.top, width=max(0, w-left-right), height=max(0, h-top-bottom)`；基于 `m_rect`（不含缩放）。
- **border-box 近似**：边框绘制外缘，contentRect 只扣 content-padding，不额外扣边框。
- 背景/边框/box-shadow 以**外框**绘制；文字/图像/内容/子控件布局/裁剪以 **contentRect**。
- 控件命中（鼠标进入控件）仍用外框；内容级命中（List 行/表头、Tree 节点、EditBox 光标）用 contentRect；Button/Label 整框（含 content-padding）可点击。
- 子控件/滚动条不得越出 contentRect（容器布局传入 contentRect；ListView/TreeView 滚动条贴外框，内容区再应用 content-padding）。

### 4.3 控件接入矩阵（本批）

| 控件 | content-padding 语义 | 说明 |
|---|---|---|
| Panel / Bench / WinFrame | 子控件布局内边距（外层） | `Panel::reflowChildren` 传 contentRect；`layout.padding` 内层，二者不互相覆盖 |
| Label / Button / CheckBox | 文本内容内缩 | 文本/图标按 contentRect 排布；点击区域仍整框 |
| EditBox / TextArea | 文本编辑区/滚动内容内缩 | 光标/选区/滚动按 contentRect；滚动条贴外框 |
| ComboBox / NumericUpDown | 文本/按钮内容内缩 | 子按钮/输入区在 contentRect 内 |
| ListView | 内容区（表头/行）内缩 | 滚动条贴外框；行命中扣除 content-padding；不做行/单元格级 |
| TreeView | 内容区（节点行）内缩 | 滚动条贴外框；`TreeViewGetNodeAt` 同步扣 content-padding |
| TabControl | 页内容区内缩 | 页签条不受影响（本批；另议） |
| StatusBar / MenuBar / MenuPanel | 条内容/菜单面板内容内缩 | item 内部 padding 不做 |
| Slider / ProgressBar | 轨道/填充内容内缩 | 标签/游标随 contentRect |
| ScrollBar | 轨道/滑块内容内缩 | 窄控件超尺寸按 0 处理 |
| Splitter | 线条/手柄内容内缩 | 命中仍外框 |
| Image / Actor / LuotiAni / Shape | 内容裁剪到 contentRect | 默认不缩放/不移动；Shape 图元坐标仍相对控件，超出裁剪 |
| **Dialog / ConfirmPopup** | 通用 `content-padding` 额外内缩；旧 `padding` 语义不变 | 旧 JSON/API 零变化；两者同时设置时见 Q9 |
| ColorPicker 本体 | 关闭态 swatch/label 内缩 | 弹窗内部 padding 不受影响 |
| HandleControl | 无内容，忽略 | 属性可读写 |

- 绝对布局（无 layout engine）不自动移动子控件；contentRect 仅用于裁剪/可用区域（Q2）。

### 4.4 最小尺寸/钳制/运行时

- 负值 → clamp 0；
- `left+right >= width` 或 `top+bottom >= height` → contentRect 宽/高 0，保留外框，诊断（不崩溃）；
- 运行时 SetContentPadding：容器重新 reflow；非容器重绘/重算内容；不修改外框 rect；焦点保持；
- 旧布局默认 0，不触发 reflow 变化。

### 4.5 与现有 padding/margin 关系

- `margin`（布局外边距）不变；`layout.padding` 继续作为布局引擎内层 padding；
- 控件 `content-padding` 为**外层**（先扣 content-padding，再交给 layout engine 扣 layout.padding）；
- **Dialog 旧 `padding`**：继续按现有 `m_padding` 作用于内容/按钮布局；通用 `content-padding` 独立（默认 0），不合并、不迁移；两者并存时语义见 Q9；
- Label 现有 `margin` 文本内缩保留；Label 同时设置 content-padding 时两者叠加（Q6），默认 0 零回归。

## 5. Q/待评审

| # | 问题 | 引擎建议 |
|---|---|---|
| Q1 | 控件 content-padding 与 layout.padding 合成 | content-padding 外层，layout.padding 内层；不重复 |
| Q2 | 绝对布局子控件 | 不自动移动；contentRect 仅用于裁剪/可用区域 |
| Q3 | ListView/TreeView 范围 | 整体内容区（表头/行/节点）；不做行/单元格级 |
| Q4 | Image/Actor/Shape | 支持：内容裁剪到 contentRect；默认不缩放/不移动 |
| Q5 | 负值/超尺寸 | clamp 0；超尺寸 content 0 + 诊断；外框不变 |
| Q6 | Label margin 与 content-padding | 保留 margin；两者叠加，文档说明；默认 0 回归 |
| Q7 | border 盒模型 | border-box 近似：只扣 content-padding，不额外扣边框 |
| Q8 | TabControl | 仅页内容区；页签条不受影响 |
| Q9 | Dialog 旧 padding | **完全保留** `padding` Float/JSON/API；通用 `content-padding` 独立默认 0；两者并存时先旧 padding 布局内容，再由 contentRect 进一步裁剪/内缩（不迁移） |
| Q10 | item 级 padding | 本批不做 |
| Q11 | CSS 映射 | CR20261102001 中 `padding` 声明 → `content-padding`；状态伪类不适用 |

## 6. 自动化测试计划

1. 各控件（Button/Label/EditBox/ComboBox/NumericUpDown/Panel/ListView/TreeView/TabControl/Slider/ProgressBar）设置 content-padding：文本/内容/子控件位置正确，外框不变；
2. 容器：content-padding + layout.padding 组合不重复；absolute 行为符合 Q2；
3. ListView/TreeView 表头/行/节点与 hitTest/GetNodeAt 坐标一致；滚动条贴外框；
4. Image/Actor/Shape 裁剪；超尺寸/负值 clamp 与诊断；
5. 运行时 SetContentPadding（属性/C ABI/Binding/SetString）生效，焦点/外框/滚动不破；
6. 旧布局默认 0 回归；**Dialog 旧 `padding` JSON/SetFloat 行为/视觉不变**，content-padding 独立；补 Dialog 同时 `padding=28` + `content-padding` 回归（旧 padding 布局 + 额外内缩）；
7. CSS `padding` 与 CR20261102001 联动（CR2001 交付后）；
8. 新测试 `test_content_padding_cabi.cpp` + Binding 探针；全量回归（基线 54 OK + 4 项预存失败）。

## 7. 用户手册更新计划

- 各控件属性表新增 `content-padding`；Dialog 页保留旧 `padding` 并说明差异；
- 新增“内容内边距（content-padding）”章节（或 theme 页内）：数字/字符串/对象、contentRect、与 layout.padding/margin 关系、控件矩阵、超尺寸诊断；
- `docs/appendix/properties.html` 新增属性；capi/binding 新增 API；
- schema common 新增 `content-padding`；Dialog `padding` Float 保持；示例 strict/探针；旧布局回归。

## 8. 影响面/兼容性

- 源码：`include/ControlBase.h`、`src/ControlBase.cpp`、`src/Panel.cpp`、各控件 draw/layout、`src/LayoutParser.cpp`、`include/PropertyNames.h`、`UICornerstoneAPI.cpp`；
- Binding：`Control.h/.cpp`、`UICornerstone.h/.cpp`、`DynamicApi.*`；
- 兼容：默认 0 零回归；Dialog 旧 `padding` 不变；同批配套。

## 9. 验收矩阵

1. 全控件 content-padding 矩阵（文本/内容/子控件）正确；
2. 容器嵌套/absolute/滚动/裁剪/命中；
3. 超尺寸/负值诊断；运行时修改不破焦点/外框；
4. 旧布局零回归、Dialog 旧 padding 不变；CSS 映射一致；schema/手册/测试通过。

## 10. 待评审确认

- Q1~Q11（重点 Q2 绝对布局、Q6 Label margin、Q9 Dialog 双 padding 并存语义）；
- 是否接受本批 item 级 content-padding 不做。

## 11. 与 CR20261102001 / CR20261102003 的接口（三方一致）

- **CR20261102001**：CSS-like 的 `padding` 声明映射到本 CR 的 `content-padding`；本 CR 负责属性/JSON/C ABI/Binding/几何/控件矩阵。
- **CR20261102003**：容器查询读取容器 **contentRect**（本 CR 定义）；媒体/容器重算不改变外框；content-padding 修改引起 reflow 由 CR2003 dirty 合并。
- **共享模型**：`content-padding` 作为 common 属性；`padding`（Dialog）保留独立；schema 同文件不同定义。
- **实施顺序**：CR2001 基座 → 本 CR → CR2003；CR2003 依赖本 CR 的键名/contentRect 口径。
- 2026-11-02 二次复核小修：措辞改“通用控件新键”；明确 content-padding 唯一命名。
- 2026-11-03 实施：主题运行时（styles/variables/variants/media/containerQueries）、通用 content-padding、响应式 API 与测试；`test_cr_theme` 通过，回归基线不变。
