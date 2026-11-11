# CR20261102003 — 响应式主题（media queries / container queries）（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261102003）
> 提出方：CornerstoneDesigner
> 状态：**已实施（2026-11-03；待设计器联验）**——核心引擎/主题运行时/C ABI/Binding/schema/文档/测试已落地；全量回归 55 OK + 4 项预存失败（基线 54+新增 test_cr_theme）。
> 关联：CR20261102001（CSS-like 主题基座）、CR20261102002（通用 padding）

---

## 1. 需求摘要

在 CSS-like 主题上增加响应式条件：视口媒体查询（窗口/视口尺寸、方向、缩放）与容器查询（Splitter/容器尺寸），尺寸变化时自动重算并应用；覆盖变量、样式、变体；不覆盖控件显式属性；保留 rect/焦点/滚动；旧主题零变化。

## 2. 现状与源码核实

- CR20261102001 已定义 `variables/styles/variants/classes`、选择器/层叠、ThemeStyleState（显式位）、baseline、`ApplyTheme/SetThemeVariant/GetThemeVariant`；**无 media/container 条件，无尺寸变化重算**。
- `UIContext` 持有视口尺寸/缩放（`viewport`、scale）；`WindowResize/ViewportScale` 事件已有（`EventType::WindowResize`、`SetViewportScaleMode`）。
- `ControlImpl::setRect/resized/moved` 是几何变化统一入口；`Panel::reflowChildren` 在布局后更新子 rect（`src/Panel.cpp:75-82`）；Splitter 拖拽改变面板/自身 rect（`src/Splitter.cpp`）。
- CR20261102002 将提供通用 `content-padding`/contentRect；容器查询的“可用尺寸”基于 contentRect。
- 现有布局 padding 在 LayoutEngine 内部，容器 rect 变化会经 reflow 更新。

## 3. 分析（可行性 / 风险）

1. **查询上下文**：媒体查询按视口逻辑尺寸（非设计画布）；容器查询按最近匹配祖先的 contentRect；二者都使用**逻辑布局尺寸**，scale/DPI 作为独立条件。
2. **重算触发**：窗口 resize、视口缩放/DPI、Splitter 拖拽、flow/grid/anchor reflow、控件/容器 rect 变化均需标记 dirty；必须合并到帧末/下一帧一次，避免逐帧重算/抖动。
3. **稳定重算**：复用 CR2001 的 baseline + explicit 位；媒体/容器只改变“命中规则集合/变量环境”，显式值、rect、焦点、滚动不变，不重建布局。
4. **层叠顺序**：普通规则 → 视口媒体 → 容器查询 → 当前变体（按 CR 提议）；媒体/容器本身不增加选择器特异性；同层按 source order。
5. **容器查询语义**：V1 仅支持 Splitter 所在的**直接布局容器/面板**（最近匹配祖先）；嵌套容器/更深祖先作为后续扩展。需定义容器选择器与查询对象，避免与 CSS 标准歧义。
6. **与 CR2001/2002 的接口**：共享 ThemeSheet/ThemeRuntime/JSON schema/API，避免两套主题模型；CR2003 只增加条件层与 dirty/recompute，不重复 `ApplyTheme/SetThemeVariant`。
7. **风险**：resize 期间频繁重算性能；容器查询循环（container size 受子内容影响）；设计器与运行时尺寸口径；旧布局零回归。

## 4. 设计

### 4.1 主题 JSON（在 CR2001 基础上扩展）

```json
"theme": {
  "variables": { "--gap": 8 },
  "styles": [ { "selector": ".card", "declarations": { "padding": "var(--gap)" } } ],
  "media": [
    { "id": "tablet",
      "query": "(min-width: 800px) and (max-width: 1199px)",
      "variables": { "--gap": 12 },
      "styles": [ { "selector": ".card", "declarations": { "padding": "var(--gap)" } } ] },
    { "id": "landscape",
      "query": "(orientation: landscape)",
      "styles": [ { "selector": ".toolbar", "declarations": { "font-size": 14 } } ] }
  ],
  "containerQueries": [
    { "id": "panel-narrow",
      "container": ".panel",                    // 祖先容器选择器（type/.class/#id，单个简单选择器）
      "query": "(max-width: 599px)",
      "variables": { "--gap": 6 },
      "styles": [ { "selector": ".card", "declarations": { "padding": "var(--gap)" } } ] }
  ],
  "variants": {
    "dark": { "variables": { "--primary": "#5AA0FFFF" }, "media": [...], "containerQueries": [...] }
  }
}
```

- `media[].id`/`containerQueries[].id`：可选稳定标识（设计器展示/`GetActiveMedia`）；缺省用索引（`media:0` / `container:0`）。
- `container`：祖先容器选择器（单简单选择器，不支持复合/后代）；V1 匹配最近祖先。
- 变体内可含 `variables/media/containerQueries`（与基主题同构）。

### 4.2 查询条件（V1）

| 类型 | 支持特性 | 说明 |
|---|---|---|
| 媒体 | `min-width` / `max-width` / `min-height` / `max-height` | 视口**逻辑尺寸**（px，数字可省略 px） |
| 媒体 | `orientation: portrait|landscape` | 宽>=高=landscape |
| 媒体 | `min-scale` / `max-scale` | 实例有效渲染缩放（window/logical，浮点） |
| 容器 | `min-width` / `max-width` / `min-height` / `max-height` | 最近匹配祖先的 **contentRect** 逻辑尺寸（CR2002） |
| 组合 | `and` | 括号包裹每项；`not`/`,`（or）不做，整块诊断禁用 |

- 查询字符串语法：`(min-width: 800px) and (orientation: landscape)`；单位仅 px（可省略）；未知特性/非法表达式 → 该块不匹配 + 诊断，不影响其它块。
- `min-resolution` 不作为 V1；后续可用 `min-scale` 等价表达。

### 4.3 查询上下文口径

- **媒体**：当前控件所在实例/视口的逻辑尺寸 `UIContext.viewport.width/height`；多视口各自独立；预览/导出的窗口尺寸即运行尺寸；设计画布需明确“按画布尺寸/按窗口”，由设计器设置预览视口（引擎不猜测）。
- **容器**：从控件向上找最近一个匹配 `container` 选择器且 `theme-eligible` 的祖先；查询尺寸 = 该祖先 `contentRect`（若 CR2002 未交付则为其 rect）；V1 仅直接布局容器/面板（最近祖先），嵌套容器作为后续扩展（诊断提示）。
- 显式 `container` 选择器命中自身祖先；查询结果只影响该祖先的后代匹配规则（不在祖先之外的控件）。
- 不基于 Splitter 自身 rect 查询（Splitter 是分隔条，不是容器）；Splitter 拖动引起面板/容器 contentRect 变化 → 触发容器查询重算。

### 4.4 层叠/变体/变量合成

对每个目标控件：

1. 解析匹配选择器的规则（CR2001 的 specificity/source order）；
2. 应用层顺序（同权重按 source order，后层覆盖前层）：
   - 基主题普通规则 → 基主题匹配的 `media` 块（source order）→ 基主题匹配的 `containerQueries` 块（source order）→
   - 当前变体普通规则 → 变体匹配 `media` → 变体匹配 `containerQueries`；
3. 媒体/容器条件**不改变选择器特异性**；变体层整体在基主题媒体/容器之后（按 CR 提议，若需媒体优先于变体可后续调整）；
4. 变量环境按同一层顺序覆盖：基 variables → 基媒体 variables → 基容器 variables → 变体 variables → 变体媒体/容器 variables；声明 `var()` 用最终环境解析；未知/循环变量诊断忽略该声明；
5. 显式 inline（CR2001 ThemeStyleState Explicit 位）**始终最高**，任何层不得覆盖；baseline 重建规则见 CR2001 R1（ApplyTheme/变体切换按新层集合重建）。

- 同源规则重复命中：同层同选择器 source order 后者胜；媒体/容器层内同理。
- 媒体块与容器块同时命中：容器块在媒体块之后（见层顺序）。

### 4.5 重算触发与节流

- **dirty 来源**：WindowResize、ViewportScale/DPI 变化、Splitter 拖拽（面板 rect/contentRect 变化）、Panel/flow/grid/anchor reflow、任意 `ControlImpl::setRect/resized/moved`、`classes` 变更、ApplyTheme/SetThemeVariant。
- **统一标记**：`UIContext::markThemeResponsiveDirty(reason)`；`ControlImpl::setRect/resized/moved` 在主题已安装时调用（`applyingTheme`/初始化 reflow 期间不标记，避免首帧抖动）。
- **合并执行**：每帧最多一次，在帧末/下一帧由 `UIContext` 处理（`flushThemeResponsive()`）；Splitter 拖动期间可实时预览但按帧节流，MouseUp 后强制一次最终重算。
- **快速跳过**：维护当前活动媒体/容器集合与变量环境签名；签名不变则不做样式写操作；变化时才 baseline 恢复 + 重算。
- **稳定性**：重算不调用 `setRect`/布局重建；外框、焦点、滚动位置保持；只写 CR2001 主题通道。
- **循环防护**：容器查询结果可能改变样式（如 padding/font）但不会改变容器 rect（CR2002 padding 可影响内容区，可能改变控件内部尺寸→reflow）；若重算引起 rect 变化，下一帧再评估，最多一次/帧；设置最大迭代（例如同一帧重算 1 次，下一帧允许一次，连续 3 帧变化则告警）。

### 4.6 API（C ABI + Binding）

在 CR2001 `ApplyTheme/SetThemeVariant/GetThemeVariant` 基础上新增：

```c
/* 强制立即重算（设计器/测试用；1=发生变化并应用，0=无变化/失败） */
int UICornerstone_RefreshResponsiveTheme(UIInstance instance);
/* 当前命中条件：逗号分隔 id（媒体/容器按 id 或 media:idx/container:idx）；
   P107 缓冲语义：返回完整长度，out=NULL/cap=0 仅查长度 */
int UICornerstone_GetActiveMedia(UIInstance instance, char* out, int cap);
```

- Binding：`ui->RefreshResponsiveTheme()`、`GetActiveMedia()`。
- `ApplyTheme` 解析 `media/containerQueries`；`SetThemeVariant` 选择变体后按层重新评估；`GetActiveMedia` 供设计器显示当前生效条件。
- 运行时 resize 自动到期重算；显式 Refresh 仅用于设计器预览/测试强制刷新。
- 旧主题无 media/container → 无行为变化；旧引擎忽略新字段并诊断（版本配套政策）。

### 4.7 与 CR20261102001/002 的接口（保证三方实现一致）

| 事项 | 归属/约定 |
|---|---|
| 主题 JSON 模型 | **CR2001 定义基座**（variables/styles/variants/classes）；CR2003 只追加 `media/containerQueries`（含变体内同构）；CR2002 只定义 `padding` 声明目标 |
| ThemeRuntime/层叠 | **CR2001 提供**规则编译/选择器/specificity、ThemeStyleState、baseline、ApplyTheme/SetThemeVariant；CR2003 在管线中插入 media/container 层与 dirty/recompute；不另建主题运行时 |
| `content-padding` | CR2002 提供通用属性/contentRect；CR2003 容器查询读 contentRect；CR2001 CSS `padding` 声明映射到 CR2002 `content-padding` |
| 显式覆盖 | CR2001 的 Explicit 位统一仲裁；CR2003 媒体/容器重算只能写非显式通道 |
| API | CR2001：`ApplyTheme/SetThemeVariant/GetThemeVariant`；CR2003：`RefreshResponsiveTheme/GetActiveMedia`；CR2002：`SetContentPadding/GetContentPadding`；不重复定义 |
| schema | 一个 `theme` 定义（CR2001 基座 + CR2002 `content-padding` + CR2003 media/containerQueries）；`classes`/`content-padding` 在 common；Dialog `padding` 独立；`validate_layout --strict` 统一 |
| 设计器 UI | 单一主题编辑器：CR2001 规则/变量/变体 + CR2003 media/容器条件列表 + CR2002 padding 控件；`GetActiveMedia` 显示命中项；预览尺寸/设计画布口径由设计器选择 |
| 实施顺序 | 建议 CR2001 基座 → CR2002 padding → CR2003 media/container；可同批发布。CR2001 解析器应预留 `media/containerQueries` 字段（未实现时诊断忽略），CR2003 只填管线，不改为新格式 |

### 4.8 设计器交互约定

- 主题管理器：媒体条件列表（id/query/variables/styles）、断点预设（640/800/1200）、层级顺序展示；容器条件列表（container selector/query）。
- 预览/设计画布：明确使用“画布逻辑尺寸”还是“窗口尺寸”作为媒体上下文；切换/缩放时显示 `GetActiveMedia`；Splitter 拖动触发容器查询预览。
- 预览/导出：同一主题 JSON；宿主只需实现新的 ApplyTheme（含 media）即可；旧主题无需迁移。

## 5. Q/待评审

| # | 问题 | 引擎建议 |
|---|---|---|
| Q1 | V1 媒体特性/单位 | width/height px 逻辑、orientation、min/max-scale；无 min-resolution；and only |
| Q2 | 容器查询支持范围 | V1 最近匹配祖先（Splitter 直接容器/面板）；嵌套容器后续 |
| Q3 | 媒体 + 变体合成 | **已确认**：基普通→基媒体→基容器→变体普通→变体媒体→变体容器（变体最后，覆盖基主题/媒体/容器）；显式最高 |
| Q4 | resize 重算模型 | dirty 标记 + 帧末/下一帧合并；Splitter 拖动节流、释放强制一次；Refresh API |
| Q5 | 媒体尺寸口径 | 视口逻辑尺寸；预览/设计画布由设计器明确；多视口独立 |
| Q6 | 容器尺寸口径 | 最近祖先 contentRect（CR2002 `content-padding` 之后）；同批交付不建 rect fallback |
| Q7 | 设计器显示 | `GetActiveMedia` 返回命中 media/容器 id；缺省索引 |
| Q8 | 旧主题/旧引擎 | 无 media 零变化；旧引擎忽略并诊断，不崩 |
| Q9 | 响应式布局 | 媒体/容器只改样式；控件 rect/排列变化需布局层能力，另立 CR |

## 6. 自动化测试计划（实施时必须落地）

1. 媒体：窄/宽、横/竖、scale 区间切换；变量/样式正确；`GetActiveMedia` 与命中一致；
2. 容器：Splitter 拖动跨断点，面板内控件样式切换；`contentRect` 口径；嵌套容器诊断；
3. 合成：媒体+容器+变体冲突按层顺序；source order；显式覆盖不变；
4. resize/Splitter 节流：帧末合并、拖动中最多一次/帧、释放最终一次；焦点/滚动/外框稳定；无逐帧抖动；
5. ApplyTheme 替换主题后 media 重算/baseline（结合 CR2001 R1）；classes 变更；
6. 非法 query/未知特性/变量诊断；旧主题零回归；
7. `test_responsive_theme_cabi.cpp` + Binding 探针；全量回归（基线 54 OK + 4 项预存失败）。

## 7. 用户手册更新计划

- `docs/theme.html`（CR2001 独立页）新增“响应式主题”章节：media/container 语法、断点示例、层叠/变体、设计器预览口径、`GetActiveMedia/RefreshResponsiveTheme`；
- declarative-syntax 附录 theme 示例增加 media/container；capi/binding 速查新增 API；schema 同步；
- 示例验证：严格校验 + 探针运行；旧主题示例保持；文档审计 0 失败。

## 8. 影响面/兼容性

- 源码：`include/ThemeStyle.h`、`src/ThemeStyle.cpp`（条件解析）、`UIContext`（dirty/flush）、`ControlImpl`（rect 通知）、CR2001 的 ThemeRuntime、CR2002 padding contentRect；
- Binding：UICornerstone 新 API、DynamicApi；
- 兼容：无 media 零变化；旧引擎忽略并诊断；同批 DLL/Binding/宿主；
- 性能：仅尺寸/布局变化后重算，合并帧；不逐帧解析。

## 9. 验收矩阵

1. 同一主题窄/宽/横/竖/scale 应用不同变量/样式；
2. Splitter 跨断点/容器查询生效；窗口+Splitter 同时变化最终组合正确；连续拖动节流/最终一次重算；
3. 显式覆盖不变；焦点/滚动/外框稳定；无逐帧抖动；
4. 变体+媒体/容器；旧主题零回归；非法条件诊断不崩；
5. C ABI/Binding/schema/手册/测试/探针；全量回归基线。

## 10. 待评审确认

- Q1~Q9（Q3 已确认变体最后；重点 Q2 容器查询范围、Q5 设计器尺寸口径、Q6 contentRect）；
- 是否接受“媒体/容器只改样式、响应式布局另立 CR”；
- 设计器 UI 对 `GetActiveMedia` 返回格式/预览尺寸切换的需求。
- 2026-11-02 按复核修订：依赖键统一为 CR2002 `content-padding`；Q3 确认变体最后层；补 Splitter 拖动节流测试。
- 2026-11-02 二次复核小修：API 统一 `SetContentPadding/GetContentPadding`；Q6 同批始终 contentRect。
- 2026-11-03 实施：主题运行时（styles/variables/variants/media/containerQueries）、通用 content-padding、响应式 API 与测试；`test_cr_theme` 通过，回归基线不变。
