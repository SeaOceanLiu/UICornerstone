# PanelReflow_PropertySymmetry_Clip_Design — 容器 reflow / 属性对称 / Panel 裁剪

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_需求_容器reflow_属性对称_Panel裁剪.md`（2026-09-16，P1）
> 复核意见：`requirements/PanelReflow_PropertySymmetry_Clip_Design_复核意见.md`（2026-09-16，**通过——放行实施**）
> 状态：**已放行，实施中**（顺序：二 A → 一 → 三 → 二 B P0；二 B P1 随后）

## 0. 三项总览

| 项 | 性质 | 现状缺口 | 设计 |
|---|---|---|---|
| 一 | 容器行为一致性 | `Panel::addControl` 不 reflow（编程式挂入叠 (0,0)） | addControl 含 layoutEngine 时 reflowChildren |
| 二 | 属性读写对称 + schema 键名桥接 | set/get 不对称（Button caption-size 读失败）；schema camelCase↔kebab 无映射 | getter 补齐 + 键名桥接（方案 a/b 二选一） |
| 三 | Panel 裁剪能力 | Panel 渲染不裁剪子项（滚动容器体验差） | clip-children 属性 + draw pushClipRect |

## 1. 需求一：Panel::addControl 触发 reflow

### 1.1 现状核实

- `Bench::addControl`（Bench.cpp:241-254）：`Panel::addControl` + resolveChildPercentages + 挂入的是 Panel 时递归 resolve/reflow——**Bench 排版、普通 Panel 不排**。
- `Panel::addControl`（Panel.cpp:29-31）：仅 `ControlImpl::addControl` 挂树，**不 reflow** ✓ 需求属实。
- 编程式 `AddChildControl` 挂入 v-flow/h-flow 的子控件叠 (0,0)，直到全局重排才修正（设计器动态属性行初始位置错误的根因）。

### 1.2 方案

```cpp
void Panel::addControl(shared_ptr<Control> control) {
    ControlImpl::addControl(control);
    if (m_layoutEngine) reflowChildren();   // 编程式挂入即排（Bench 同语义）
}
```

- **reflow 幂等性**：`reflowChildren()` 依据 `m_layoutEngine->getType()` 分派，逐子 setRect——重复调用（Parser 显式 reflow + 本入口）结果一致（同输入同布局），无累计副作用 ✓。
- **removeControl 场景（验收 3）**：`ControlImpl::removeControl` 不触发 reflow——挂入后移除的布局残留（被移除控件的 rect 仍占位）。**需补**：`Panel::removeControl` override 同样 `if (m_layoutEngine) reflowChildren()`（对称）。

#### 1.2.1 Bench 复用边界（评审确认）

`Bench : public Panel`，且 `addControl` 为虚（ControlBase.h:206 纯虚 → Panel override → Bench override）。`Bench::addControl`（Bench.cpp:241-254）：

```cpp
void Bench::addControl(shared_ptr<Control> control) {
    Panel::addControl(control);            // ① 基类入口：挂树 + 若 Bench 有引擎则 reflow
    resolveChildPercentages();             // ② Bench 自身百分比子项解析
    auto panel = dynamic_pointer_cast<Panel>(control);
    if (panel) {                           // ③ 挂入的是容器 → 递归 resolve + reflow 子项
        panel->resolveChildPercentages();
        if (panel->getLayoutEngine()) panel->reflowChildren();
    }
}
```

**复用结论（两层语义互补，不冲突不重复）**：
- ① 复用新分支：`Bench::addControl` 内部显式调用 `Panel::addControl`（基类实现），新加 `if (m_layoutEngine) reflowChildren()` 对 Bench **自动生效**——若 Bench 被显式设了 `setLayoutEngine`（Panel 公有方法，Bench 继承可得），挂入即排自身子项，语义正确。
- ②③ 保留不重复：Bench 作为画布根**一般无 layoutEngine**（`if (m_layoutEngine)` 对 Bench 为 false，新分支不触发），② 的 `resolveChildPercentages` 与 ③ 的"挂入子容器时递归 reflow"处理的是**另一层语义**（Bench 自身百分比 + 子项为容器），与 Panel 新分支（自身是容器→排自身子项）**互补**，不会双重 reflow。
- **不建议进一步复用**（把 ②③ 提炼进 Panel::addControl）：会改变"任意 Panel 挂容器子项"的既有行为（当前仅 Bench 特殊处理），影响面扩大——保守保留 Bench 后半段，二者自然协同。

### 1.3 验收

1. v-flow Panel 编程式挂 N 控件 → 各 rect 正确排布（不叠 (0,0)）；
2. LoadLayout 回归无损（Parser 显式 reflow 幂等）；
3. removeControl 后布局无残留（reflow 重排余下项）。

## 2. 需求二：属性系统读写对称 + schema 键名桥接

### 2.1 现状核实

- **A 不对称**：`Button::getCaptionSize(float) const`（Button.h:58）——带参未用、返回 `uint32_t`（截断）——确为笔误，应为 `float getCaptionSize() const`；Button 属性系统**无 getFloatProperty 重载** → `caption-size` 写成功读失败。
- **B 键名差异**：schema camelCase（`captionSize`/`checkColor`/`boxBorderColor`…）↔ 运行时 kebab（`caption-size`/`check`/`box-border`…）；`checkColor→check`、`crossColor→cross`、`indeterminateColor→indeterminate`、`boxBorderColor→box-border` 为**非机械转换特例**；`text` 键对 label/button = `caption`、对 edit-box/text-area = `text`（语义差异）。

### 2.2 方案 A：getter 补齐

**范围**：逐一核对 22 类控件 set/get 对称性（重点高频编辑项）：
- **P0（需求点名 + 设计器高频）**：Button `caption-size`（补 `getFloatProperty` 分发 + 修 `getCaptionSize()` 签名）、各类型 `check-state`（CheckBox getBoolProperty 是否已有？核对）；
- **P1**：其余 set 有 get 无项（脚本扫描 set*Property 与 get*Property 常量差集产出清单，逐项补）。

**风险**：改动面大（多文件 getter），需回归。按 P0/P1 分批。

### 2.3 方案 B：键名统一（命名对齐，评审确认方向——取代桥接）

**核心思路**：统一到运行时属性名（kebab-case）。**改数据的成本远小于改代码**——属性系统 kebab 名是接口契约（PropertyNames.h + 各 set*Property 的 strcmp + 主题/测试），动它回归面大；schema/布局文件是数据，改键名是机械迁移。设计器侧收益：删除 `jsonKeyToProp`（规则+特例表）与 `textIsCaption` 特判，动态面板直接用 schema 键名调 `Set*/Get*`。

**现状数据面（已核实）**：
- schema 176 可编辑键，其中 **92 个 camelCase**（`captionSize`/`checkColor`/`boxBorderColor`…）——机械 camel→kebab 可转，但有 4 个运行时**简写特例**（`check`/`cross`/`indeterminate`/`box-border`）不成规则；
- `text` 键语义差异：Label(44)/Button(118)/CheckBox(181) 运行时是 `caption`，EditBox(198)/TextArea(656)/ComboBox(311)/ProgressBar(448) 是 `text`；
- 4 个简写属性在引擎内**仅常量引用**（`kCheck`/`kCross`/`kIndeterminate`/`kBoxBorder` 的 strcmp，CheckBox.cpp:650-694），无字符串字面量散落；`kStyleCross`（style 枚举 `"cross"`）与颜色键 `kCross` **不冲突**（不同分发域）。

**实施清单（引擎侧一次性）**：

| # | 项 | 说明 | 影响面 |
|---|---|---|---|
| 1 | PropertyNames 特例改名 | `kCheck "check"→"check-color"`、`kCross "cross"→"cross-color"`、`kIndeterminate "indeterminate"→"indeterminate-color"`、`kBoxBorder "box-border"→"box-border-color"`（含常量名与 CheckBox.cpp 分发）；`kStyleCross` 不动 | 低（常量引用自动跟；7.3 速查/文档需同步） |
| 2 | schema 键名对齐 | 92 个 camelCase 键改 kebab（`captionSize→caption-size` 等）；`text` 语义键：Label/Button/CheckBox/… → `caption`，EditBox/TextArea/ComboBox → 保持 `text`（与运行时一致） | 数据文件 |
| 3 | layouts/*.json 迁移 | 机械键名替换（test_layout.json / test_layout_advanced.json / all_controls.json 等） | 数据文件 |
| 4 | LayoutParser kJsonXxx 合并 | 双常量体系（kJson* 244 个 + 使用 890 处）改用属性名常量，长期规则化 | **大**（890 处引用；可分批，不阻塞 1-3） |
| 5 | 动画 jsonc（属布局体系）同批 | 若含键名同步迁移 | 数据文件 |
| 6 | version 字段 bump | schema/布局 version 递增（破坏性键名变更） | — |

**决策**：以**方案 1+2+3+6 为 P0**（命名对齐落地，设计器收益即得），**方案 4（LayoutParser kJson 合并）为 P1 重构**（不阻塞，长期规则化）；动画 jsonc（5）按属布局体系与否确认。**getter 补齐（§2.2）仍必须做**——与命名无关（Button 读不回 caption-size 是 getter 缺失，非键名问题）。

**风险与回归**：
- 键名变更破坏既有布局/测试/主题 JSON——需全量迁移 + 回归；
- `kJson*` 双常量合并在 P1 分批推进，期间 kJson* 与新属性名常量并存（Parser 内部读取不受影响）；
- 文档（4.x 控件页/schema 说明/7.3 速查）同步。

### 2.4 验收

1. Button 放置后 `GetFloat("caption-size")` 返回真实值（写 24 → 读 24）；
2. **命名统一**：schema 键名 == 运行时属性名（kebab）——`checkColor→check-color` 等 4 特例对齐后，camel→kebab 规则转换零特例；`text` 语义键按控件类型正确（Label/Button→caption，Edit/TextArea→text）；
3. 既有布局/测试 JSON 全量迁移后回归无损（键名变更不破坏解析）；
4. `Set*/Get*` 直接用 schema 键名（kebab）命中运行时属性（设计器侧删除 jsonKeyToProp/textIsCaption）。

## 3. 需求三：Panel 子项裁剪（clipChildren）

### 3.1 现状核实

- `RenderDevice::pushClipRect/popClipRect`（纯虚，EditBox/ListView/TreeView 内部自裁剪在用）；
- `Panel::draw`（Panel.cpp:17-23）：beforeDraw + draw + afterDraw，**无裁剪** ✓ 需求属实。

### 3.2 方案

- **属性**：`kClipChildren = "clip-children"`（Bool），`ControlImpl`（或 Panel）成员 `m_clipChildren = false`（默认关，保持兼容）；`setBoolProperty/getBoolProperty` 分发 + Binding `SetBool("clip-children", b)`。
- **渲染**（Panel::draw）：`m_clipChildren` 时 `pushClipRect(内容 rect)` 包住 `ControlImpl::draw()`（子控件递归在其内），`popClipRect()` 收尾。内容 rect = 自身绘制区（考虑边框/缩放，与 EditBox 同口径）。嵌套 Panel 均开启时 clipStack 相交（渲染器已有栈语义）✓。
- 与滚动容器配合：设计器滚动容器开 clip-children，子项超出即裁剪（不再"超界隐藏"模拟）。

### 3.3 验收

1. clip-children=true：子项超界部分不绘制；false：现状一致；
2. 嵌套裁剪（Panel⊂Panel 均开启）clipStack 相交正确；
3. 滚动行半可见被正确裁剪。

## 4. 实施顺序（按需求优先级建议）

| 序 | 项 | 范围 | 风险 |
|---|---|---|---|
| 1 | 需求二 A（getter 补齐 P0） | Button caption-size + 高频项 | 低-中 |
| 2 | 需求一（Panel reflow + removeControl 对称） | Panel.cpp + 测试 | 低 |
| 3 | 需求三（clip-children） | ControlImpl/Panel + 测试 | 低 |
| 4 | 需求二 B（键名桥接 b1） | ControlImpl 别名解析 + 特例/语义表 | 中（影响面全属性系统） |

## 5. 影响面与风险

- 需求一：`Panel::addControl` 行为变化（含 engine 时 reflow）——现有编程式挂入到普通 v-flow Panel 的代码（若依赖"手动 reflow"）会提前布局；设计器是目标受益方。removeControl 对称补齐。
- 需求二 A：getter 补齐全文件改动，需逐控件回归；`getCaptionSize` 签名修正（`uint32_t(float)`→`float()`）可能破坏既有调用（grep 调用点评估，几乎无外部调用）。
- 需求二 B（b1）：别名解析在属性系统顶层加一层——需保证 kebab 路径零行为变化（未命中别名时才走转换）。特例/语义表随 schema 演进维护。
- 需求三：Panel draw 加条件裁剪——默认 false 零影响；true 时依赖渲染器 clip 正确（三后端已有，EditBox 在用）。

## 6. 待审核问题

1. **需求二 B 方案**：已按评审方向改为「命名统一到运行时属性名（kebab-case）」——P0（特例改名 + schema/layouts 键名迁移 + version bump）先行、P1（LayoutParser kJson 体系合并 890 处）重构。**确认 P0/P1 分批范围**？
2. **getter 补齐范围**：P0（需求点名 Button caption-size + check-state 等高频项）先行，P1 全量扫描补齐分批——同意？
3. **需求一 removeControl 对称**：补 `Panel::removeControl` 也 reflow——确认？
4. **clip-children 归属层**：ControlImpl（全控件可裁剪）还是仅 Panel？需求只要 Panel——建议 Panel 层实现，ControlImpl 留作后续。
5. **命名统一的破坏性键名变更**（`check`→`check-color` 等 4 项 + 92 camelCase 键）：需全量迁移既有布局/测试/主题 JSON——**设计器 main_layout.json 同步配合**（需求侧已列出）。schema `text` 语义键对 Label/Button 改 `caption` 是否也接受（破坏性）？
6. **动画 jsonc（需求清单 5）**：是否属布局体系（含可编辑键）？若动画键名独立于属性系统，可不同批迁移——确认范围。
