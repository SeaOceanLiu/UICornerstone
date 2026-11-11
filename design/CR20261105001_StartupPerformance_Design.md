# CR20261105001 — 启动性能：LoadLayout / ApplyTheme 耗时与初始化资源日志（分析 + 设计）

> 状态：**P0 已实施（2026-11-05；补齐 classes/per-root 与无响应式快路径；待设计器联测；P1 未实施）**（2026-11-05；按设计器评审 B1~B4 修订）
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261105001）
> 关联：CR20261102001（主题运行时）、CR20261102003（响应式重算）

## 1. 现象与基线

设计器实测（Debug/Release 接近）：

| 阶段 | 耗时 |
|------|------|
| 主实例 Create | 0.2–0.5s |
| `LoadLayoutFromFile(main_layout.json)` | 5.8–6.8s |
| `ApplyTheme(designer_theme.json)` | 4–5s |
| `ApplyTheme({})` | 约 5.5s |

关键信号：**空主题仍约 5.5s**，说明主要成本不在规则数量，而在固定遍历/重复应用；两条 `Loading resources...` 日志来自两个 Bench 实例（主 UI + CanvasPane 子视口），不是同一实例重复加载。

## 2. 代码路径与热点

### 2.1 ApplyTheme 至少遍历两遍

`UICornerstone_ApplyTheme`（`src/UICornerstoneAPI.cpp:1601`）：

1. `applyAll(bench/popup/menu)`；
2. 随后 `refreshResponsive(...)`，而 `refreshResponsive` 末尾再次 `applyAll(root)`（`src/ThemeStyle.cpp:742`）。

因此每次 `ApplyTheme` 至少执行 **两遍** `applySet + inheritPass`，且中间还解析/重算响应式条件。

### 2.2 空主题没有快速路径，并且会重置字体

`ThemeRuntime::applyAll`（`src/ThemeStyle.cpp:385`）无条件：

- `applySet(base)`；
- 有变体再 `applySet(variant)`；
- `inheritPass(root)`。

`applyOne`（`src/ThemeStyle.cpp:404`）即使没有任何规则，仍会：

- 对每个控件调用旧主题逻辑；
- `m_legacy` 在 `setTheme` 中总是创建（空对象也非空指针），随后按 `themeCategory` 调用 `applyCommonColors` / `getFontSize` / `applyFontToControl`；
- 对 Label/EditBox/Button 等重新设置字体，可能触发布局/字体资源重建。

这能解释空主题仍 5.5s，并可能改变已加载布局的字体 baseline。

### 2.3 LoadLayout 强制主题遍历 + dirty 再应用

`UICornerstone_LoadLayout`（`src/UICornerstoneAPI.cpp:1659`）在布局无主题时仍：

- 创建 `themeRuntime`；
- 尾部 `applyAll` + `markResponsiveDirty`；
- 下一次 `UICornerstone_Update` 消费 dirty，`refreshResponsive` 再次 `applyAll`（`src/UICornerstoneAPI.cpp:1002`）。

即使用户不应用主题，空 runtime 也会无条件扫描整棵控件树两遍。无 media/container 时 dirty 重算没有必要。

### 2.4 显式覆盖采集在解析期无条件做

`LayoutParser::parseCommonProperties` 对每个控件调用
`captureExplicitFromJson`（`src/ThemeStyle.cpp:581`）：

- 为每个控件创建/复制 `json` 显式对象；
- 解析颜色组、字体、padding、shadow 等并转字符串；
- 即使当前布局没有主题、且宿主尚未 ApplyTheme。

在 1920×1080 大型布局上，这是解析期固定成本；同时显式 map 生命周期贯穿实例，增加内存。

### 2.5 规则匹配为 O(控件 × 规则)

`applyOne` 对每个控件遍历 `set->rules`（以及 media/container 规则）；每次匹配都做类型/类/ID 判定。
规则数量增长时会放大；空规则时只剩遍历成本，但非空主题（designer_theme）会走完整匹配。

### 2.6 继承遍历无条件执行

`inheritPass`（`src/ThemeStyle.cpp:540`）无论是否存在字体声明/父链字体上下文都递归全树，并对每个控件查询字体属性。
在没有字体相关规则时，这一遍可以短路。

### 2.7 LoadLayout 解析/首帧固定成本未拆分

`LoadLayout` 内部还包含：

- JSON 解析、组件展开、控件构造（字体/纹理延迟初始化）；
- 各容器 `resolveChildPercentages` / `reflowChildren`；
- 动画 `prepareLayoutAnis`；
- `controlsById` 注册、popup/menu 挂载；
- 主题应用（上述）。

目前只有整体日志，没有分阶段耗时；无法区分“解析/控件创建”与“主题遍历”的真实占比，需要轻量阶段计时后再确定第二阶段优化重点。

## 3. 优化设计

### 3.1 P0：去掉重复应用与空主题遍历

1. **ApplyTheme 只应用一次，且覆盖全部根**：
   - 将 `refreshResponsive` 拆为“评估条件”和“应用”两步，或在 `ApplyTheme` 中只调用一次统一的“评估 + 多根应用”入口；删除 `applyAll` + `refreshResponsive` 的双调用。
   - 必须继续覆盖 `bench`、`popupPool`、`menuPool`；不能只对 bench 调一次 `refreshResponsive`。
   - **B5：容器条件必须 per-root 隔离**：`media` 条件共享视口/缩放；`container` 命中集不能复用单个全局 `m_activeMedia`。评估接口需返回 `ActiveConditions { mediaIds, containerIds }`，每个根分别计算 `containerIds`，再将该上下文传入应用遍历。
   - `UICornerstone_GetActiveMedia` 明确定义为返回 **bench（主根）** 的 active 集；popup/menu 的内部应用使用各自 per-root 上下文，不进入该字符串。
### 3.1.1a B5：per-root ActiveConditions 的上下文契约

- `ThemeRuntime` 不再用单个全局 `m_activeMedia` 表示容器命中；定义：

```
struct ActiveConditions {
    std::string mediaIds;       // 视口/缩放，所有根共享
    std::string containerIds;   // 当前根专属
};
```

- 每个根（bench/popup/menu）独立计算 `containerIds`；`mediaIds` 共享。
- `applyAll/applySet/applyOne/inheritPass` 必须显式接收 `ActiveConditions`，`applyOne` 不再读取全局 `m_activeMedia`。
- `applyControl`（classes 变更/`ClearThemeOverrides`）需先解析控件所属根，使用该根的 `ActiveConditions`；不能在 popup/menu 内复用 bench 的容器命中。
- 所有重新应用入口统一走同一上下文：
  - `ApplyTheme`；
  - `SetThemeVariant`；
  - `RefreshResponsiveTheme`；
  - `UICornerstone_Update` 的 dirty 消费；
  - `ClearThemeOverrides` / `classes` 变更 / 创建后重算。
- `GetActiveMedia` 返回 **bench 主根** 的 `mediaIds + containerIds`（保持现有逗号分隔格式；不返回 popup/menu 的容器 ID）。
- `hasResponsiveData` 必须检查 base **和当前 variant** 的 media/container；只查 base 会漏 variant 条件。
- 回归：bench 命中 container、popup 不命中及反向；变体切换、Refresh、Update、ClearOverrides 后 popup/menu 仍保持各自条件。

2. **LoadLayout 保留 runtime 与显式采集，只跳过无主题应用**：
   - 解析期仍创建 `themeRuntime` 并执行 `captureExplicitFromJson`，保证“先 LoadLayout、后 ApplyTheme”时 JSON 显式颜色/字体/圆角/阴影仍然优先；
   - 仅当布局无有效主题数据时跳过 `applyAll` 和 `markResponsiveDirty`；
   - 不采用“无主题不建 runtime”的方案（会造成 B1 语义回归）。
3. **有效数据判定（B4）**：
   - `hasLegacyData`：`fonts`/`colors` 为**非空对象**且包含有效类别/颜色键，不能只看键存在；
   - `hasThemableData`：非空 `styles` 规则，或非空 media/container，或非空变体规则，或有效 legacy 数据；
   - 仅 `variables` 而没有任何规则/有效 legacy 时不算可应用数据；no-op 时 `themeRawJson` 保留旧值，不由本 API 改写；
   - 快速路径必须同时保留显式覆盖 map 与后续 ApplyTheme 语义；
   - **LoadLayout 语义定论**：本 CR 维持现有**叠加加载**行为，不引入整树替换；`themeRuntime->clear()` 只清理主题编译态，不清理已有控件的 `m_explicit`。不承诺多次 LoadLayout 之间合并主题/显式状态；需要整树替换的调用方应使用新实例。
4. **空主题语义（B2）——本 CR 定为“无操作”**：
   - `ApplyTheme({})` 不改变当前主题/控件样式/字体，仅解析成功返回；不再触发全树遍历/旧字体重放。
   - 本 CR **不实现“清除主题并恢复 baseline”**；若宿主需要清除主题，另立 CR 定义 baseline/清除 API，避免性能修复隐式扩张为 CR2001 主题模型补全。
   - 空 `fonts:{}` / `colors:{}` 也不触发默认字体回放；`hasLegacyData` 按非空有效键判定。
   - 验收覆盖：无旧主题/有旧 CSS 主题/有旧 legacy 主题三种场景下 `ApplyTheme({})` 均为 no-op，显式值不变。
5. **空主题 no-op 的状态保护（B2 实现约束）**：
   - 必须在调用 `setTheme` **之前**用解析后的 JSON 计算 `hasThemableData`；false 时直接成功返回；
   - 不修改 `themeRawJson`、`m_variant`、`m_activeMedia`、per-root active 状态、`m_explicit`；
   - 仅 `variables`、空 `styles`、空 `fonts/colors`、legacy 空对象全部归入 no-op，不能出现“JSON 已替换、运行时未应用”的半状态；
   - 无旧主题/旧 CSS 主题/旧 legacy 主题三种场景均 no-op；
   - 测试覆盖三种输入 × 三种旧主题：字体、颜色、显式值、变体均不变，`themeRawJson` 保持旧值。
6. **旧主题数据判定**：
   - `setTheme` 记录 `legacyData = 有效非空 fonts/colors`；
   - 只有 `legacyData` 才执行旧字体/颜色重放，避免空 `{}` 重置字体。

### 3.2 P1：合并遍历与规则索引

1. `applyAll` 合并 base/variant/继承为一次深度遍历（携带父字体上下文与显式状态），减少 2~3 次全树扫描；
   - 合并不得改变 base → 媒体 → 容器 → 变体 → 显式覆盖的层序与 source order；
   - variant 覆盖 base、媒体与变体的组合需单独回归。
2. 主题编译期建立索引：
   - 按类型、类、ID 建立候选规则桶；
   - `applyOne` 只检查候选规则，再按原 specificity/order 合并，不把索引变成选择器语义重写；
   - 无匹配规则/无字体通道时跳过显式 replay 与继承。
3. 无响应式条件时跳过 `refreshResponsive` 和 `GetActiveMedia` 计算；有 media/container 时才进入评估/应用管线。

### 3.3 P1：解析期显式覆盖采集优化

- 保留 B1 要求的解析期采集；先做零成本优化：没有主题相关显式键时不创建 `m_explicit[ctl]` 条目；
- 进一步改为轻量 **属性键 bitset/稀疏值** 时，必须保证后续 ApplyTheme 仍尊重 JSON 显式值；不能把语义补齐留到后续阶段；
- 控件销毁/重建时清理 `m_explicit` 陈旧指针，避免内存增长和潜在悬垂键。

### 3.4 P0：阶段计时诊断

在 Release 保留默认关闭/环境变量开启的轻量计时：

```
[Perf] LoadLayout parse=... create=... apply=... reflow=... theme=... total=...
[Perf] ApplyTheme parse=... apply=... responsive=... controls=...
```

不改变公开 API；宿主可用日志定位。设计器侧只需打印/收集，不需改行为。

### 3.5 日志语义

`Bench` 构造函数打印的两条日志分别对应：

- 主实例 Bench（设计器 UI）；
- CanvasPane 子视口 Bench（画布）。

建议改为：

```
[Bench] init (main instance)
[Bench] init (canvas viewport)
```

或由 UIContext 输出实例角色；不消除第二次初始化（子视口本来就需要），只消除“重复加载资源”的误导。

## 4. 验收与测试

- 空主题（B2）：无旧主题/旧 CSS 主题/旧 legacy 主题下，`ApplyTheme({})`、仅 variables、空 fonts/colors 均为 no-op；不改变字体/颜色/显式值/变体/`themeRawJson`，也不发生全树应用。
- 无主题布局 + 后续 `ApplyTheme`（B1）：控件显式 `colors`/`font`/`content-padding`/`shadow` 仍优先。
- 叠加加载（本 CR 定论）：`LoadLayout` 维持现有叠加语义，不承诺多次加载主题合并；`clear()` 保留已有控件显式 map；验证新增控件采集、旧控件显式值在后续 ApplyTheme 仍优先。
- popup/menu（B3/B5）：`ApplyTheme` 及 media/container 变化后 bench、popupPool、menuPool 都使用新主题；container 条件按根隔离（bench 命中、popup 不命中及反向），`GetActiveMedia` 只报告 bench。
- 空布局/单控件/1920×1080 主布局分阶段对比；主题规则 0/10/100/1000 递增。
- LoadLayout 无有效主题时不执行应用遍历，但仍保留显式采集；有主题时只应用一次。
- 字体 baseline：布局无主题时与“未调用 ApplyTheme”一致；空主题 no-op 不修改字体。
- 三级回归：Debug/Release `auto=2`、`auto=3`、`validate_layout --strict`。
- 旧主题/旧布局/显式颜色/字体/状态/响应式行为零回归。
- 测量目标：启动总耗时显著下降，首帧可交互；不以单次机器抖动作为结论。

## 5. 风险与待确认

- **清除旧主题语义**：本 CR 明确空主题为 no-op；真正的清除主题需要另立 CR 定义 baseline/清除 API，不能在性能 CR 中隐式引入。
- **显式覆盖延迟采集**：若 ApplyTheme 可发生在 LoadLayout 之后，必须保证显式 JSON 值不被主题覆盖；需要键级稀疏存储而非直接丢弃。
- **规则索引**：选择器状态/media/container 层组合复杂，先做缓存/索引的正确性回归，避免破坏 specificity/层序。
- **Diagnostic 计时**：Release 默认关闭，避免日志/计时影响正常启动。

## 6. 建议实施顺序

1. 先做 P0 去重/空主题/无 media dirty/legacy 判定（风险低、收益最大）；
2. 加阶段计时，确认 LoadLayout 剩余瓶颈；
3. 再做一次遍历合并与规则索引；
4. 最后评估显式覆盖采集延迟化的语义风险。

---

## 修订记录

- 2026-11-05 初稿：基于 CR20261105001 设计器实测与当前引擎代码路径分析；待设计器 + 用户评审后实施。
- 2026-11-05 按评审意见修订：B1 保留 runtime/显式采集；B2 空主题 no-op；B3 多根 popup/menu 应用；B4 有效数据判定；补阶段计时优先级与验收矩阵。
- 2026-11-05 二次评审修订：B2 不引入 baseline（no-op，清除主题另立 CR）；B5 per-root 容器条件状态；补旧主题分类/增量加载/`GetActiveMedia` 验收。
- 2026-11-05 三次评审修订：B2 no-op 在 setTheme 前判定并保护 themeRawJson/active/显式 map；B5 所有重应用入口统一使用 per-root ActiveConditions；补增量加载与 variant responsive 判定。
- 2026-11-05 P0 实施：ApplyTheme no-op/单次应用、空主题跳过、有效数据判定、LoadLayout 无主题跳过、多根 ActiveConditions；Debug 回归通过。

## 7. 实施后性能收益预估（初评）

以下以设计器当前实测为基线：启动约 10.7s，`LoadLayout` 5.8–6.8s，`ApplyTheme(designer_theme)` 4–5s，空主题约 5.5s。估算是**方向性范围**，最终以同机 Release 分阶段计时为准。

| 阶段 | 当前 | P0（去重/快速路径/legacy 判定） | P0 + P1（合并遍历/索引/采集优化） |
|------|------|----------------------------------|-----------------------------------|
| `ApplyTheme({})`（任意旧主题，no-op） | 约 5.5s | **< 0.01s** | < 0.01s |
| `ApplyTheme(designer_theme)` | 4–5s | **0.5–1.5s** | **0.2–0.8s** |
| `LoadLayout(main_layout)` | 5.8–6.8s | **2.5–4.5s** | **1.5–3s** |
| 启动到首帧 | 约 10.7s | **5–7s** | **3–5s** |

依据：

- 空主题的主要耗时不是规则匹配，而是 `applyAll` 固定遍历、旧主题字体重放和重复调用；本 CR 将空主题定义为 no-op 后可直接短路，`<0.01s` 仅针对该 no-op 语义。
- 设计器主题只有少量规则，P0 后主要剩一次遍历 + 显式 replay + 继承；规则索引对规则数量增长才明显，当前主题下预计再降约 30%–50%。
- `LoadLayout` 的解析、控件创建、字体/资源初始化为不可消除的固定成本；P0 只能去掉主题重复遍历，不能承诺把 5.8s 全部消除，故给出 2.5–4.5s 区间。
- 若阶段计时显示 `LoadLayout` 解析/创建仍占 2s 以上，需要另立“解析/控件创建”优化子项，不在本 CR 首次实施中承诺。

结论：**P0 预计可把启动总耗时降低约 35%–55%（空主题 no-op 接近常数时间）；完整 P0+P1 有望降到 3–5s。** 所有数字须以同一 Release、同一 `main_layout.json`/`designer_theme.json` 的分阶段计时复测为准。

> 2026-11-05 引擎侧 Debug 探针（65 控件，非设计器最终口径）：P0 后空主题约 0.5ms、designer_theme 约 2.6s、LoadLayout 约 9.9s；设计器同机 Release 数据待补。

## 8. 设计评审响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| B1 跳过 runtime/采集 | 改为**保留 runtime 与解析期采集**，仅跳过无有效主题的应用遍历；补无主题 LoadLayout + 后续 ApplyTheme 的显式覆盖回归。 |
| B2 空主题语义 | 明确为 **no-op**（保留当前主题/样式/字体，显式值不变）；不引入 baseline；清除主题另立 CR。 |
| B3 popup/menu | ApplyTheme 改为“一次条件评估 + 对 bench/popupPool/menuPool 多根应用”；补 popup/menu 主题与响应式回归。 |
| B4 有效数据判定 | 改为非空有效 legacy、非空规则/media/container/变体判定；仅 variables 不算可应用数据；补单元测试。 |
| 分阶段计时先行 | 保留并优先实施默认关闭的诊断计时；§7 数字标注为待同机 Release 验证，不作硬目标。 |
| 显式采集低成本优化 | 无主题相关显式键时不创建 map 条目；控件销毁清理陈旧键。 |
| 无响应式条件 | `hasResponsiveData` 为假时跳过 `refreshResponsive` 扫描。 |
| 一次遍历/索引 | 明确不得改变 specificity、层序和 source order；补变体/媒体组合回归。 |
| 日志角色 | 由 UIContext/Bench 创建参数传入 main/viewport 角色，不从构造函数猜测。 |

## 9. 二次评审响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| B2 baseline 不可实施 | 采纳建议：本 CR 将 `ApplyTheme({})` 定义为 **no-op**，不引入 `ThemeStyleState/baseline`；清除主题另立 CR。P0 目标改为去重、空遍历跳过、多根应用。 |
| B5 多根 container 条件 | 增加 per-root `ActiveConditions` 设计：media 共享，container 按根评估；`GetActiveMedia` 明确返回 bench 主根；补 bench/popup 反向容器命中测试。 |
| B1 增量加载边界 | 实现时检查 `themeRuntime->clear()` 是否会清除已有控件的显式 map；补增量加载/已存在控件语义测试。 |
| 阶段计时 | 先落 §3.4，以实测替代 §7 估算；估算不作为硬验收。 |
| 空主题旧主题分类 | 验收覆盖旧 CSS 主题/旧 legacy 主题/无旧主题三种 no-op 场景。 |
| 显式采集优化 | 无键不建 map；覆盖 colors/font/content-padding/shadow/corner-radius 等全部当前采集键。 |
| 一次遍历/索引 | 保持 specificity/层序/source order，先语义回归后性能比较。 |

## 10. 三次评审响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| B2 no-op 必须早于 setTheme | 增加 3.1.4/3.1.5：先计算 `hasThemableData` 再决定；false 时完全不触碰 `setTheme`/`themeRawJson`/变体/active/显式 map；补三种输入 × 三种旧主题测试。 |
| B5 所有重应用入口 | 增加 3.1.1a：`ActiveConditions` 显式传入 applyAll/applySet/applyOne/inheritPass；ApplyTheme、SetThemeVariant、RefreshResponsiveTheme、Update dirty、ClearThemeOverrides/classes 全部使用 per-root 上下文；`applyControl` 不得读全局容器状态。 |
| `GetActiveMedia` 语义 | 固定为 bench 主根的 media+container ID，逗号分隔格式不变；popup/menu 容器 ID 不进入该返回值。 |
| `hasResponsiveData` | 明确检查 base + 当前 variant。 |
| 增量加载 | 增加验收项，要求明确整树/增量语义并测试显式 map 不被误清。 |
| 阶段计时/估算 | 先落 §3.4，用同机 Release 实测更新 §7，估算不作为验收硬目标。 |
| 日志角色 | 由 UIContext/Bench 创建参数传入；Debug 顺序不作为判断依据。 |

## 11. 四次评审小修（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| 2.1 `themeRawJson` 矛盾 | 统一为 no-op 保留旧值；删除“仅 variables 仍保存”的表述；由宿主自行保存只读配置。 |
| 2.2 LoadLayout 语义 | 明确本 CR 维持**叠加加载**，不引入整树替换；`clear()` 不清理已有控件 `m_explicit`；不承诺多次 LoadLayout 主题合并，整树替换另立需求。 |
| no-op 解析成本 | `<0.01s` 目标包含 JSON 解析，不以 `hasThemableData` 单点测量代替。 |
| GetActiveMedia | 保持 bench media+container 逗号串；no-op 时保留旧值。 |
| hasResponsiveData | base + 当前 variant 均检查；variables-only 返回 false。 |
| 显式采集键 | 无键不建 map；覆盖全部当前采集键。 |
| 阶段计时 | 先落 §3.4，再以同机 Release 数据更新 §7；估算不作为硬验收。 |

## 12. 联测遗漏修复（2026-11-05）

- **classes/theme-role 路径**：`UICornerstone_SetString` 不再调用 `applyControl(c)`（其默认使用 bench active），改走 `applyThemeControlInRoot`：先求控件所属根，再按该根 `evaluateConditions` 得到 `ActiveConditions`，最后 `applyControl(c, cond)`。popup/menu 中的类变更不会串用 bench 容器条件。
- **hasResponsiveData 快路径**：`applyThemeAllRoots` 在 `hasResponsiveData()==false` 时直接对 bench/popup/menu 使用空 `ActiveConditions` 应用一次，跳过 `evaluateConditions` 的全树容器扫描。
- 回归：`test_cr_theme`、`test_handlecontrol_cabi` 通过；全量 55 OK + 4 项预存失败。
- Release 已重建并同步；SHA256 两侧一致：`f48fcf553a3d5ae2a5ea6b84efbbcce13116bb45cbcd725234256f14ead59830`。
