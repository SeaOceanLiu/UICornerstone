# CR20261102001 — CSS-like 主题样式表 / 类与角色（分析 + 设计）

> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261102001）
> 提出方：CornerstoneDesigner
> 状态：**已实施（2026-11-03；待设计器联验）**——核心引擎/主题运行时/C ABI/Binding/schema/文档/测试已落地；全量回归 55 OK + 4 项预存失败（基线 54+新增 test_cr_theme）。
> 兼容政策：DLL/Binding/宿主同批配套；**V1 直接支持 CSS-like 新语法**（未正式发布，不引入版本门禁）；无新语法的旧主题行为不变。

---

## 1. 需求摘要

用 **CSS-like 样式表 + 类选择器**替代/补充现有按控件类型的 theme：用户可定义任意类（`.confirm` / `.cancel`）、变量、状态规则（`:hover/:pressed/:disabled/:selected`），控件通过 `classes` 分配；运行时按主题变体切换，且不覆盖控件显式属性；设计器提供可视化编辑与类批量分配（不要求用户手写 CSS）。旧布局/旧主题零影响。

## 2. 现状与源码核实

- `Theme` 仅保存整棵 JSON，提供 `getStateColor/getColor/getFontName/getFontSize` 与 `applyCommonColors/applyFont`（`include/Theme.h:16-40`、`src/Theme.cpp:76-137`），**只按控件类型分类**（`theme.colors.<category>` / `theme.fonts.<category>`）。
- `LayoutParser::parse` 解析顶层 `theme` 后，各控件先 `applyCommonColors/applyFont`，再逐项解析显式 JSON（`src/LayoutParser.cpp:51,404-405,...`）——现有“显式属性覆盖主题”只是解析顺序，**没有显式覆盖记录**，运行时无法安全重算主题。
- `ControlImpl` 背景/边框使用 `fillRect`（`src/ControlBase.cpp:227-280`）；`RenderDevice::drawRoundRect` 已存在（`include/RenderDevice.h:52`），可扩展通用圆角。
- 无 `classes` / 样式表 / 变体 / 运行时主题 API；`theme` schema 仅 `"type":"object"`（`docs/schema/declarative-ui.schema.json:1084-1086`），校验宽松。
- 无通用 `padding` / `opacity` 属性；`border-radius` 未通用化（`kRadius` 仅用于 Shape/菜单等局部）。
- 控件类型/ID/属性键均有既有接口（`getControlType`、`id`、`PropertyNames`）；焦点/布局重排入口明确。

## 3. 分析（可行性 / 风险）

1. **解析与运行时重算**是两层能力：解析期按规则应用；运行期 `ApplyTheme/SetThemeVariant` 需要重新遍历控件并应用，必须新增“显式覆盖记录”才能不覆盖控件显式属性。
2. **状态规则**不能只按当前状态应用；需把 `:hover/:pressed/:disabled/:selected` 规则写入对应 StateColor 通道，由既有状态机绘制时选择。
3. **优先级**必须固定并可测试：inline > #id > .class > 类型；同权重 source order；状态规则优于同选择器无状态；复合选择器按分量计数。
4. **声明属性范围**受引擎现状约束：字体/颜色/文字阴影/阴影偏移已有；`border-radius` 可用 `drawRoundRect` 通用化；**菜单面板已有 box shadow 实现（shadow-radius 可配、颜色/偏移固定）**，可泛化为通用 `box-shadow`；`padding`/`opacity` 无通用实现，需明确范围或降级。
5. **旧主题兼容**：无 `styles/variables/variants/classes` 的主题继续按旧 `fonts/colors` 路径解析；出现新字段即按本设计解析，不做版本切换。
6. **性能**：解析期一次性编译选择器；运行时重算仅遍历控件匹配规则，不做字符串/JSON 反复解析；控件数 × 规则数可接受（规则通常数十条）。
7. **设计器协作**：引擎只提供 JSON 语义 + C ABI/Binding；可视化规则编辑器由设计器实现；schema 需给出精确结构。

## 4. 设计

### 4.1 主题 JSON（V1 直接支持）

```json
"theme": {
  "variables": { "--primary": "#2D7FF9FF", "--danger": "#E06060FF", "--radius": 6 },
  "styles": [
    { "selector": "button", "declarations": { "font-size": 16 } },
    { "selector": ".confirm", "declarations": {
        "background": "var(--primary)", "font-size": 18,
        "border-radius": "var(--radius)", "padding": "6 12" } },
    { "selector": ".confirm:hover", "declarations": { "background": "#4A90D9FF" } },
    { "selector": "#btnOk:disabled", "declarations": { "background": "#3A3A3AFF" } }
  ],
  "variants": {
    "dark": {
      "variables": { "--primary": "#5AA0FFFF" },
      "styles": [ { "selector": ".confirm", "declarations": { "border-color": "#5AA0FFFF" } } ]
    }
  }
}
```

- **无版本门禁**：`theme` 中一旦出现 `variables/styles/variants` 或控件 `classes`，即按本设计解析；未出现则完全按旧 `fonts/colors` 行为。
- 不要求 `theme.version`/`theme.features`；如 JSON 中出现这些字段，仅作为前向兼容元数据忽略（可诊断提示），不改变解析。
- `variables`：`--name` → 颜色/数值/长度/字符串；`var(--name)` 仅单层替换（支持变量引用变量，带循环检测）；未知变量/循环 → 该声明忽略 + 诊断。
- `variants.<name>`：可选 `variables`（覆盖基主题）与 `styles`（**追加到基 styles 之后**，同权重 source order 更高）；`SetThemeVariant("")` 恢复基主题。

### 4.1.1 适用范围（全部公开控件）

- **所有公开控件实例**（26 种类型及容器/复合控件本身）都参与选择器匹配与 `classes` 分配；不存在“仅按钮可主题化”的限制。
- **通用声明**对所有 `ControlImpl` 派生控件生效：`background` / `border-color` / `text-color` / `text-shadow`（四态）、`font-family/size/style`（有文本的控件）、`border-radius`（通用圆角）。
- **控件专有外观**通过该控件已支持的属性键声明（见 §4.3.1）；引擎按控件属性系统分发，未知/不支持 → 诊断并忽略，不影响其它声明。
- **内部辅助控件可判定**：`ControlImpl` 新增 `theme-eligible`（Bool，默认 true）：
  - 引擎内部创建的子控件（滚动条、ColorPicker 弹窗内部 hex/label、Dialog 内置确认/取消按钮、菜单内部面板等）创建后置 `false`；
  - 用户通过布局/Factory/C ABI/Binding 创建的控件（含放入 leadingControl/cellControl 的控件）默认为 true；
  - 主题遍历/选择器匹配仅处理 `theme-eligible==true`；**父内部控件为 false 时整个子树跳过匹配与继承**；
  - 用户 leadingControl/cellControl 默认 true；`theme-eligible` 作为通用 Bool 属性暴露时，用户置回 true 即恢复参与匹配/继承（后果：内部辅助控件也可被主题规则命中，需自担风险）；
  - `theme-eligible` 写入 PropertyNames，schema 允许控件级 Bool；内部控件设置仅引擎内部完成。
- 运行时 `ApplyTheme/SetThemeVariant` 遍历当前实例的公开控件树；复合控件内部辅助控件不单独匹配，其视觉由宿主控件样式/既有内部主题决定。

### 4.2 选择器子集（本批）

| 形式 | 示例 | 说明 |
|---|---|---|
| 类型 | `button` | 控件 `getControlType()` 的 kebab 名 |
| 类 | `.confirm` | 控件 `classes` 包含该名称 |
| ID | `#btnOk` | 控件 `id` 完全匹配 |
| 复合 | `button.confirm` / `#btnOk.confirm` | 类型/类/ID 组合（无空格） |
| 状态 | `.confirm:hover` / `#btnOk:disabled` | 仅允许最后一个简单选择器带一个状态；状态∈hover/pressed/disabled/selected |

- 本批不支持：后代/子选择器、属性选择器、`!important`、逗号分组；遇到时忽略该规则/声明并诊断（不崩溃）。
- 非法选择器：整条规则忽略 + 诊断（不崩溃）；未知声明：忽略该声明 + 诊断。

### 4.3 声明属性 → 引擎属性映射

| CSS-like 声明 | 引擎目标 | 本批支持 |
|---|---|---|
| `font-family` | `FontName`（枚举/字体名） | ✅ |
| `font-size` | 控件字号（int） | ✅ |
| `font-style` | 字体样式（normal/bold/italic/…；复用现有解析） | ✅（控件支持时） |
| `background` / `text-color` / `border-color` / `text-shadow` | 对应 `StateColor` 通道（无状态规则写 normal；伪类写对应态） | ✅ |
| `shadow-offset-x/y` | `kShadowOffsetX/Y`（**文字**阴影偏移，控件支持时） | ✅ |
| `box-shadow`（颜色/偏移/半径） | 通用 `box-shadow-color/offset-x/offset-y/radius`（`drawRoundedRect` 扩展近似；无高斯模糊） | ✅ **本批支持**（含 Menu/ContextMenu，复用既有菜单阴影路径） |
| `border-radius` | 新增通用 `corner-radius`（`drawRoundRect` 填充/边框） | ✅（全部 ControlImpl 派生） |
| `padding` | 通用内容内边距（由 **CR20261102002** 提供） | ✅ CSS 声明名 `padding` 映射到通用 **`content-padding`** 属性；语义/支持范围以 CR20261102002 为准 |
| `opacity` | 控件整体 alpha | ⚠️ 本批不做（RenderDevice 无全局 alpha；颜色乘 alpha 会破坏状态色），诊断忽略；后续另立 |

- 颜色值支持 `var(--name)`；状态规则只允许颜色类声明（`background/text-color/border-color/text-shadow`），其它声明带状态 → 诊断忽略。
- `box-shadow` 语法（对象，避免 CSS 简写歧义）：`{ "color": "#00000066", "offset-x": 0, "offset-y": 2, "radius": 6 }`；`radius` 为扩展/圆角近似半径（与既有 MenuPanel 阴影一致，不做高斯模糊）；`radius<=0` 或 `alpha=0` 不绘制。
  - 通用化：MenuPanel 现有 `shadow-radius` 作为 `box-shadow-radius` 的兼容别名；新增 `box-shadow-color/offset-x/offset-y` 控件属性（默认沿用 MenuPanel 既有 `PANEL_SHADOW*` 常量，菜单视觉不变）；
  - 所有 `ControlImpl` 派生控件可用（背景之后、内容之前绘制）；Menu/ContextMenu 与其它控件同一路径，避免重复投影。
- `padding` 语法：数字（四边）或 `"v h"` / `"t r b l"`（px 单位可省略；仅数字/`px`）；映射目标为 `content-padding`。
- **padding 依赖 CR20261102002**：本 CR 只负责将 CSS `padding` 映射到通用控件 **`content-padding`**（Dialog 旧 `padding` Float 保留，不冲突）；通用内边距几何/控件支持/布局语义由 CR20261102002 定义。若 CR20261102002 未交付，`padding` 声明诊断忽略。

#### 4.3.1 控件专有声明（全部控件可用其已支持属性键）

- 除通用声明外，`declarations` 的键可直接使用控件属性键（`PropertyNames` 列出的颜色/浮点/整数键），引擎按控件类型调用对应 `set*Property` 分发；示例：
  - Slider/ScrollBar：`track`、`track-fill`、`thumb`、`thumb-border`、`thumb-hover`、`tick`（状态子键按现有属性系统）；
  - ListView：`header-background`、`header-text`、`item-background`、`item-text`、`item-hover-background`（item-id 定向键不参与 CSS，需行级 API）；
  - TreeView：`hover`、`selected`、`item-background`、`item-text`、`item-shadow-*`（节点级定向键同 ListView 限制）；
  - TabControl：`tab-background`、`tab-hover-background`、`tab-selected-background`、`tab-indicator-color`、`tab-content-background`；
  - MenuBar/MenuPanel/StatusBar：`item-*` 系列键（控件级；item 定向需运行时属性 API）；
  - Splitter/Shape：`line`/`fill`/`stroke`/`radius` 等控件属性键（按其支持范围）。
- 状态伪类与控件专有颜色键：若控件属性键有对应状态族（如 `item-hover-background`），CSS 可用 `:hover` + 基础键或直接写状态键；无状态族的键只支持无状态声明（带伪类 → 诊断忽略）。
- 属性键与声明键同名冲突时，以显式 CSS 声明为准（inline 优先级仍最高）；未知键/类型不匹配 → 诊断忽略。

### 4.4 层叠/优先级

1. 控件显式属性（JSON 显式键或运行时 Set*，**inline**）最高；
2. `#id`（按 ID 数）；
3. `.class`（按类数）；
4. 类型（按类型数）；
5. 同权重：**source order** 后者胜；状态规则高于同选择器无状态规则；复合选择器按 (id,class,type) 元组比较。
- 例：`.confirm` 与 `button.confirm` → 后者胜；`#btnOk` 胜 `.confirm`；`.confirm:hover` 胜 `.confirm`。
- 字体/文字颜色继承：见 §4.4.1；CSS 规则只写命中控件，不新增“inherit 关键字”。

### 4.4.1 容器内控件的继承关系

- **选择器/类不继承**：`.card` 只命中声明该类的容器本身；子控件不会自动带父类的规则（本批无后代/子选择器）。子控件需各自声明 `classes`，或使用变量/变体统一控制。
- **可继承属性（主题运行时按父链向下传递）**：
  - `font-family` / `font-size` / `font-style`：沿用引擎既有 `resolveFontInheritance`；父控件由主题规则设置字体时写入 font context，子控件无显式字体/无匹配规则时继承最近祖先；
  - `text-color` 的 **normal** 通道：**仅父控件由主题规则派生**的 text-color normal 向子控件继承（子无显式/无匹配规则时）；父控件 JSON/运行时**显式**设置的 text-color **不自动继承**（保持引擎既有行为）；状态色（hover/pressed/disabled）不继承；
  - 其余声明（background、border-color、text-shadow、border-radius、padding、shadow-offset、opacity、控件专有键）**不继承**；需要在子控件规则中显式声明。
- **优先级**：子控件匹配规则/显式 inline > 继承值；父控件规则不会覆盖子控件显式值或子规则。
- **复合控件内部辅助控件**（滚动条、ColorPicker 内部 hex/label、Dialog 按钮等）不参与匹配，也不接收上述继承；其视觉由宿主控件/内部既有样式决定。
- **运行时重算**：`ApplyTheme/SetThemeVariant` 按控件树自上而下计算继承值（font/text-color normal），再对每个控件应用自身匹配规则；不改变 rect/焦点。
- 示例：`.card { background:#202020; text-color:#FFFFFF; font-size:14 }` → 容器背景仅容器；子 Label（无显式色/规则）文字白、字号 14；子控件背景仍默认/自身规则。

### 4.4.2 控件 JSON 显式字体与主题规则的关系

控件自身在 JSON 中声明的字体属性（`font` 对象、`font-family`/`font-size`/`font-style`、`font-file`、`font-resource`）属于 **inline 显式值**，优先级最高：

| 来源 | 优先级 | 说明 |
|---|---|---|
| 控件 JSON 显式字体 / 运行时 `Set*` | 最高 | 逐属性生效；主题规则、父继承均不得覆盖 |
| 控件自身匹配的 CSS 规则 | 次之 | 仅写未被显式覆盖的字体通道 |
| 父链字体继承 | 最低 | 仅当该控件无显式字体且无匹配规则时生效 |

- **逐属性判定**：JSON 只写 `font-size` 时，字号被锁定；主题仍可写 `font-family`（若未被显式覆盖）。JSON 只写字体名时，字体名锁定，主题 `font-size` 仍可生效。
- **`font-file`/`font-resource`**：与字体枚举互斥的显式来源；一旦声明，主题 `font-family` 对该控件忽略（字号仍可来自主题/显式）；保持引擎既有互斥优先级。
- **与继承交互**：控件显式字体（含仅字号）按引擎现有 `fontContext` 语义阻止父字体继承；未显式属性由主题规则补足；无规则时按既有默认/继承规则。此行为与旧布局一致，不因主题功能改变。
- **运行时**：加载后调用字体相关 Set* 即记为显式，后续 `ApplyTheme/SetThemeVariant` 不覆盖；清除显式覆盖的能力本批不提供（需重新加载布局）。
- 示例：`{ "type":"book", "font-size":20, "classes":["confirm"] }` + `.confirm { font-family: ..., font-size: 18 }` → 字号 20（inline），字体名取主题；同级若还有父规则，父规则不覆盖。

### 4.5 显式覆盖记录（运行时重算的关键）

- `ControlImpl` 新增 **ThemeStyleState**：
  - `classes`（string 集合）；
  - `applyingTheme` 标志；
  - 主题可写声明通道的稀疏显式位（**只标记显式来源**）：
    - `font-family/font-size/font-style` 位；
    - 四个颜色组的 `normal/hover/pressed/disabled` 位（background/text/border/text-shadow）；
    - `shadow-offset-x/y`、`corner-radius`、`content-padding` 位；
  - **来源三分**（B1 定稿）：
    - `Explicit`：布局 JSON 显式键 + 公开 C ABI/Binding `Set*` 入口 → 置位；
    - `Theme`：`applyingTheme=true` 的主题写 → 不置位；
    - `Internal`：引擎内部同步/控件专用内部 setter/状态刷新 → 不置位；
  - **置位入口收敛**：仅在 `LayoutParser` 的显式键应用路径与 `UICornerstone_Set*` 的公开属性入口调用 `markThemeExplicit(...)`；内部 setter 走不置位路径（或 `PropertySource` 守卫）；
  - 主题应用：仅写未显式置位的通道，应用完复位 `applyingTheme`；
  - 运行时新 Set* 置位，后续 `ApplyTheme/SetThemeVariant` 保留；
  - **基线快照（R1 修订）**：
    - **初次布局**：应用 CSS 规则前，保存“旧类型 theme + 控件默认”为 `themeBaseline`；
    - **ApplyTheme 成功替换主题后**：按**新主题的旧类型 theme + 控件默认**重建每个控件的 baseline，再应用新 CSS 规则（非显式通道必须使用新主题值，不得保留旧 baseline）；
    - **SetThemeVariant**：按 base+variant 重建 baseline（variant variables/styles 生效）后重算；
    - **classes 变更**：仅恢复当前 baseline 后重算本控件（不重建 baseline）；
    - 显式通道在任何情况下都保持不变；
  - `classes` 变更仅触发该控件基线恢复 + 规则重算（不全局重排）；
  - **清除显式覆盖**：提供 `ClearThemeOverrides(instance, ctl)`（C ABI/Binding），清除主题通道显式位并重算；未提供的其它内部属性不受影响。
- 状态位粒度：伪类规则只写对应状态位；显式 `background-hover` 只阻止主题写 background-hover，不影响 normal。
- 字体/圆角/内边距为单值通道（无状态粒度）。

### 4.6 `classes` 字段与兼容别名

- 控件 JSON：`"classes": ["confirm","primary"]` 或 `"classes": "confirm primary"`；`PropertyNames` 新增 `kClasses`。
- 属性系统：`GetString("classes")` 返回空格分隔；`SetString("classes", ...)` 更新并触发本控件基线恢复 + 样式重算（旧类派生值不残留，显式位保留）；C ABI/Binding 通用 Get/Set 即可；
- 从 `.confirm` 改为 `.cancel` 时，旧的 `.confirm` 派生 background/hover 等先回基线再应用 `.cancel`；无新规则匹配的通道回到基线值；显式 inline 不变。
- 旧 `theme-role`：若存在且无 `classes`，等价 `classes=[theme-role值]`；只映射为类名，不实现 `theme.roles` 旧模型（设计方向已切换到 CSS-like）。

### 4.7 运行时 API（C ABI + Binding）

```c
/* 应用主题 JSON；成功 1，解析/校验失败 0 并保留旧主题 */
int UICornerstone_ApplyTheme(UIInstance instance, const char* themeJson);
/* 选择变体；name=NULL/"" → 基主题；不存在/失败 0 */
int UICornerstone_SetThemeVariant(UIInstance instance, const char* name);
/* 读取当前变体；P107 缓冲语义：返回完整长度，out=NULL/cap=0 仅查长度 */
int UICornerstone_GetThemeVariant(UIInstance instance, char* out, int cap);
/* 清除控件主题通道显式覆盖并重算（1=成功；invalid/不支持=0） */
int UICornerstone_ClearThemeOverrides(UIInstance instance, UIControlHandle ctl);
```

- Binding：`ui->ApplyTheme(json)`、`SetThemeVariant(name)`、`GetThemeVariant()`。
- `ApplyTheme`：解析成功后才替换；随后遍历实例控件重算样式（跳过显式通道），不调用 `setRect`/布局重建，焦点保持；失败保留旧主题并返回 0（诊断含行号/选择器）。
- `SetThemeVariant`：等价 `ApplyTheme(base+variant 合成)` 的样式重算；变体不存在返回 0。
- 无 `ReloadTheme`（宿主可复用自身保存的 JSON 调用 ApplyTheme）；仅含旧 `fonts/colors` 的主题调用 ApplyTheme 也合法（重算旧分类样式）。

### 4.8 解析/运行时架构

- `Theme` 扩展：`ThemeSheet`（rules/variables/variants）+ 既有 `fonts/colors` 数据；选择器编译为内部结构（类型/类/ID/状态/权重/sourceOrder）。
- `UIContext` 持有 `ThemeRuntime`：当前主题、变体、控件注册（或遍历 `bench` 树）；`ApplyTheme` 重建编译结果。
- 初始布局：`LayoutParser` 解析 theme 后，对每个控件按“旧类型主题 → CSS-like 规则 → 显式 JSON”顺序应用；显式写入通过 ThemeStyleState 标记。
- 运行时重算：遍历 `bench` 控件（含 panels 子控件；弹窗/菜单按既有树），对每个控件应用匹配规则（未显式通道），无 allocation 上的逐帧开销（仅 API 调用时）。
- 诊断：解析错误/未知选择器/未知变量/不支持声明/版本不符 → 汇总到实例错误信息（`GetLastError`/日志），不崩溃。

### 4.9 兼容/版本/降级

- 旧主题（仅 `fonts/colors`）：完全走原 `Theme` 分支，视觉不变。
- 新语法（`styles/variables/variants/classes`）：V1 引擎直接解析，无版本字段要求。
- 旧引擎 + 新主题：不属于本 CR 承诺（同批配套）；引擎对未知 field/声明忽略+诊断，不崩溃。
- 不支持的选择器/声明：诊断+忽略对应规则/声明；不影响其它规则。
- 旧布局/旧主题解析与视觉回归必须保持（无 classes/新语法）。

### 4.10 Schema

- `theme` 从 `"type":"object"` 展开：`variables`(object)、`styles[]`（selector string + declarations object）、`variants`(object)；旧 `fonts/colors` 保留；不新增版本字段。
- `classes` 加入 `$defs/common`（string 或 string[]）；`theme-role` 作为兼容别名（string）。
- 严格校验：declarations 采用宽松 object（键集合由引擎诊断，schema 不封闭，避免 CSS-like 扩展频繁改 schema）；selector 非空字符串。
- `validate_layout --strict` 对旧布局/新示例均通过。

## 5. Q/待评审（引擎建议）

| # | 问题 | 引擎建议 |
|---|---|---|
| Q1 | 选择器子集 | 类型/.class/#id/复合/单状态；不做后代/属性/important/逗号分组 |
| Q2 | 状态优先级 | 状态规则 > 同选择器无状态；按 (id,class,type) 权重 + source order |
| Q3 | 变量/长度语法 | `var(--name)` 单层/递归（循环检测）；`padding` 数字/px；`border-radius` 数字/px |
| Q4 | `padding` 支持范围 | **已确认**：另立 CR20261102002 通用 `content-padding`；本 CR 仅映射 CSS 声明，待其交付后全控件可用 |
| Q5 | `border-radius` | 通用 `corner-radius`（ControlImpl 背景/边框，drawRoundRect）；所有控件可用 |
| Q6 | `opacity` | 本批不做（需全局 alpha/离屏合成），诊断忽略 |
| Q7 | 类字段名 | `classes`（string/array），`theme-role` 作为兼容别名 |
| Q8 | 运行时 API | `ApplyTheme/SetThemeVariant/GetThemeVariant`；不做 ReloadTheme |
| Q9 | 显式覆盖判定 | ThemeStyleState 稀疏位（状态粒度）；主题应用跳过显式通道 |
| Q10 | 旧主题/版本 | **无版本门禁**：旧主题走旧路径；出现新字段即按本设计解析（用户确认 V1 直接支持） |
| Q11 | 适用范围 | **全部公开控件**参与匹配；通用声明全控件；专有外观用控件属性键；引擎内部辅助控件默认排除 |
| Q12 | 容器内继承 | classes/box 属性不继承；font-family/size/style 与 text-color(normal) 沿父链继承；状态色/背景/边框/圆角/内边距/专有键不继承；子显式/规则优先 |
| Q13 | JSON 显式字体 | 显式 font/font-size/font-file/font-resource 为 inline，最高；主题/父继承仅补未显式通道；font-file/resource 与字体枚举互斥优先 |
| Q14 | 显式来源/内部 setter | 仅 JSON 显式键 + 公开 C ABI/Binding Set* 置显式位；Theme/Internal 不置位；提供 ClearThemeOverrides |
| Q15 | 阴影范围 | **本批支持通用 `box-shadow`**（颜色/偏移/半径，扩展近似无模糊）；MenuPanel 既有阴影复用并保留 `shadow-radius` 别名；文字阴影独立保留 |
| Q16 | padding 范围 | **由 CR20261102002 通用 `content-padding` 提供**；本 CR 仅映射 CSS 声明，不单独限制控件范围 |
| Q17 | 内部控件判定 | `theme-eligible` 标志（默认 true，内部子控件 false）；用户 leadingControl/cellControl 为 true |

## 6. 自动化测试计划（实施时必须落地）

1. 旧主题回归：无新语法的布局/主题视觉、显式覆盖、字体/颜色与现状一致；
2. 新语法选择器：类型/.class/#id/复合/状态；source order、权重、状态优先；
3. `variables`/`var()`：颜色/数值/长度、变量引用、未知/循环诊断；
4. 优先级：inline > #id > .class > 类型；运行时 Set* 后 ApplyTheme/变体不覆盖 inline；
5. 声明映射：字体、四态色、文字/盒阴影、border-radius；边框/面板投影含 MenuPanel；CSS `padding` 映射到 CR2002 `content-padding`（交付后全控件）；opacity 诊断忽略；
6. 运行时：`ApplyTheme` 成功/失败保留旧主题、`SetThemeVariant`/`GetThemeVariant`、焦点/布局不抖动、无崩溃；
7. classes 设置/清除/批量（多次 Set）触发重算；theme-role 别名；
8. 旧引擎兼容语义（忽略未知/诊断、不崩）；无版本门禁；
8b. B1 来源审计：内部状态同步/控件专用内部 setter 不置显式位；JSON/公开 Set* 置位；ClearThemeOverrides 恢复主题值；
8c. B2 theme-eligible：内部控件不匹配/不被重算；用户 leadingControl/cellControl 匹配；
8d. B3/B4：box-shadow 在 Menu/Button/Panel 生效且无重复；CR2002 未交付时 `padding` 诊断忽略，交付后映射 `content-padding` 且不影响其它声明；
9. 全控件适用性：对 Button/Label/EditBox/CheckBox/Panel/ListView/TreeView/TabControl/MenuBar/StatusBar/Slider/ScrollBar/Splitter/Shape 等分别用 `classes` + 通用/专有声明验证；引擎内部辅助控件不被误染；
9b. R1 baseline：旧主题 A → ApplyTheme 旧主题 B / CSS 主题 → 非显式通道取新主题；显式通道保持；SetThemeVariant 后 baseline 正确；
9c. R2/R3：CR20261102002 未交付时 padding 诊断忽略；theme-eligible=false 父控件的后代不匹配/不继承，用户 leading/cell 默认可匹配；
10. 容器继承：Panel `.card` 设置 font/text-color → 子 Label 继承；子显式 text-color/字体或 `.child` 规则覆盖；background/border/radius 不继承；运行时变体后继承同步；内部辅助控件不被继承；
10b. JSON 显式字体：仅 font-size / 仅 font-family / font-file / font-resource 与主题规则逐通道对照；运行时 SetFont 后 ApplyTheme 不覆盖；
11. 新测试 `test_theme_styles_cabi.cpp` + Binding 探针；全量回归（基线 54 OK + 4 项预存失败）。

## 7. 用户手册更新计划（含示例可运行性）

- **主题独立成页**：将 `docs/declarative-ui.html` 3.4.5 拆分为新页面 `docs/theme.html`（含旧 fonts/colors 与 CSS-like 语法、类/ID/状态选择器、变量、优先级、变体、运行时 API、**全部公开控件支持矩阵、容器继承规则、显式覆盖/classes 变更、不支持项诊断说明**）；`declarative-ui.html` 保留指向该页的导航链接；
- 阴影章节区分 **文字阴影** 与 **box-shadow（含菜单/面板投影）**；明确 box-shadow 为扩展近似、无高斯模糊；CSS `padding` 指向 CR20261102002 `content-padding`（Dialog 旧 `padding` 另说明）；标注 `opacity`/后代/属性/`!important`/逗号分组不支持及诊断；
- `docs/appendix/declarative-syntax.html`：theme 示例 + `classes`；状态/类示例；
- `docs/appendix/properties.html`：`classes` 属性；
- `docs/appendix/capi.html` / `binding.html`：3 个主题 API；
- `docs/schema/declarative-ui.schema.json`：同步；
- 示例验证：新 JSON 过 strict；C ABI/Binding 示例进入探针编译运行；旧主题示例保持可用；文档审计脚本 0 失败。

## 8. 影响面/兼容性

- 源码：`include/Theme.h`、`src/Theme.cpp`、`include/ThemeStyle.h`、`src/ThemeStyle.cpp`、`include/ControlBase.h`、`src/ControlBase.cpp`（圆角/显式位/classes）、`include/UIContext.h`、`src/UIContext.cpp`（ThemeRuntime）、`src/LayoutParser.cpp`、`include/PropertyNames.h`、`UICornerstoneAPI.h/.cpp`；
- Binding：`UICornerstone.h/.cpp`、`DynamicApi.*`；
- Schema/手册/API 映射同步；发布同批 DLL/Binding/宿主；
- 性能：解析期编译；运行时仅 API 调用重算，无逐帧成本。

## 9. 验收矩阵

1. `.confirm`/`.cancel` 赋给不同按钮：字体/四态色/文字阴影/box-shadow/圆角/内边距正确；
2. `:hover/:disabled` 正确；变量改主色后规则同步；
3. 运行时切换变体，两类按钮同步；显式覆盖不变；
4. 保存/打开/导出/导入/预览全链；非法选择器/变量/声明诊断且不崩；
5. 旧主题/旧布局零回归；schema strict；探针/全量回归；手册示例可运行。

## 10. 待评审确认

- Q1~Q10（Q4/Q16 已确认委托 CR2002 `content-padding`；Q6 opacity 暂缓；Q10 已确认无版本门禁；Q15 已含菜单 box-shadow）；
- 是否接受 `theme-role` 仅作 classes 别名（不实现旧 roles 模型）；
- 设计器可视化编辑器需要的引擎诊断信息格式（选择器/行号/键名）。

## 修订记录

- 2026-11-02 初稿。
- 2026-11-02 按用户评审修订：V1 直接支持（移除 version/features 门禁）；主题章节从 declarative-ui.html 3.4.5 独立成 `docs/theme.html`。
- 2026-11-02 按用户确认补充：CSS-like 适用于全部公开控件（通用声明 + 控件专有属性键；内部辅助控件排除）。
- 2026-11-02 按用户确认补充：容器内选择器/类不继承；font + text-color(normal) 沿父链继承，其余不继承；内部辅助控件排除。
- 2026-11-02 按用户确认补充：JSON 显式字体为 inline 最高优先级，逐通道覆盖主题/继承；font-file/resource 互斥优先。
- 2026-11-02 按设计评审修订：B1 显式来源三分+ClearThemeOverrides；B2 theme-eligible；B3 阴影范围收窄；B4 padding 范围标注待确认；classes 变更基线恢复；父显式 text-color 不继承。
- 2026-11-02 按用户确认：B3 改为支持通用 `box-shadow`（含 Menu/ContextMenu，复用菜单阴影实现；扩展近似无模糊）。
- 2026-11-02 按复核修订：R1 ApplyTheme 重建 baseline；R2 padding 交由 CR20261102002 并补诊断；R3 theme-eligible 子树跳过/用户可置回；补 baseline/边界测试。

## 11. 与 CR20261102002 / CR20261102003 的接口（三方一致）

- **CR20261102002（通用 content-padding）**：本 CR 的 CSS `padding` 声明映射到 CR2002 的通用 `content-padding`；通用属性/contentRect 由 CR2002 定义（Dialog 旧 `padding` 保留），本 CR 不重复实现。CR2002 未交付时 `padding` 诊断忽略。
- **CR20261102003（响应式主题）**：本 CR 的 `ThemeSheet/ThemeRuntime`、选择器/层叠、ThemeStyleState/baseline、`ApplyTheme/SetThemeVariant` 作为基座；CR2003 只追加 `media/containerQueries` 层与 dirty/recompute，不另建主题模型/API。
- **JSON/schema 单一模型**：CR2001 基座（variables/styles/variants/classes）+ CR2002 `content-padding` + CR2003 `media/containerQueries` 共同组成同一 `theme`；设计器只维护一套主题 JSON/编辑器。
- **API 归属**：CR2001 定义 `ApplyTheme/SetThemeVariant/GetThemeVariant`；CR2003 增加 `RefreshResponsiveTheme/GetActiveMedia`；CR2002 增加 `SetContentPadding/GetContentPadding`，互不重复。
- **实施顺序**：CR2001 基座 → CR2002 padding → CR2003 media/container（可同批发布）；CR2001 解析器需预留 `media/containerQueries` 字段（未实现时诊断忽略），避免后续改格式。
- 2026-11-02 按复核修订：padding 通用键改为 `content-padding`（Dialog 旧 `padding` 独立）；测试/手册/交叉接口同步。
- 2026-11-02 二次复核小修：显式位改为 `content-padding`；Q4/Q16 标注确认。
- 2026-11-03 实施：主题运行时（styles/variables/variants/media/containerQueries）、通用 content-padding、响应式 API 与测试；`test_cr_theme` 通过，回归基线不变。
