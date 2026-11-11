# CR20261105001_StartupPerformance_Design 二次评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105001_StartupPerformance_Design.md`（修订版，待复核）
- 前次意见：`Temp/CR20261105001_StartupPerformance_Design_评审意见.md`
- 结论：**有条件通过（B1/B3/B4 已基本闭合；B2 仍不足以实施，另需补 B5）**

## 1. 修订确认

### B1 保留 runtime/显式采集：已闭合

修订版 §3.1.2 已改为“解析期仍创建 themeRuntime 并执行 captureExplicitFromJson”，§3.1.3 增加“快速路径必须同时保留显式覆盖 map”，并在 §4 增加“无主题布局 + 后续 ApplyTheme”验收。方向正确，避免了设计器 `LoadLayout → ApplyTheme` 丢失显式值的主要回归。

仍需在实现中确认：`LayoutParser::parseLayout` 无 theme 时的 `themeRuntime->clear()` 不会清掉**本次加载前已存在控件**的显式 map；如果当前 `LoadLayout` 是增量加载而非整树替换，需要补充语义测试。此项属于实现细节，不阻塞设计。

### B3 多根 popup/menu：已闭合

§3.1.1 已明确 bench、popupPool、menuPool 都要应用，并要求容器条件按各自根评估、media 条件共享视口/缩放；§4 也补了回归。方向正确。

### B4 有效数据判定：已闭合

`hasLegacyData`/`hasThemableData` 改为非空有效数据判定，并明确 variables-only 不算可应用数据；符合快速路径的安全前提。

## 2. 仍阻塞实施的问题

### B2. 空主题 baseline 只有设计引用，没有可实施契约

修订版 §3.1.4 选择“清除当前主题并恢复 baseline/控件默认值，显式值保持不变”，并写明 baseline 复用 CR20261102001 的 `ThemeStyleState/baseline` 契约，若当前实现缺失则先补齐。但当前引擎源码并未落地该契约：

- `src/include` 中未发现 `ThemeStyleState`、`themeBaseline`、`markThemeExplicit` 等实现；
- 现有实现只有 `ThemeRuntime::m_explicit`（`ThemeStyle.h:107`）和 `replayExplicit`，没有“旧主题 + 控件默认”的 baseline 快照；
- 因而本 CR 的 P0 实际隐含了“先补齐 CR2001 baseline 机制”的范围扩张，但修订版没有给出 baseline 的数据结构、属性覆盖范围、捕获/恢复生命周期、内存与性能预算。

这还不是可以直接进入编码的设计。至少需要补充：

1. baseline 保存哪些通道（字体、四态色、padding、圆角、阴影、文字阴影等）以及状态粒度；
2. 初次布局、`ApplyTheme` 替换主题、`SetThemeVariant`、`classes` 变化、控件新增/销毁时 baseline 的捕获/失效规则；
3. baseline 与 `m_explicit` 的交互：恢复 baseline 后显式通道如何回放，避免二次写入；
4. baseline 捕获本身的耗时是否计入 P0 的 `ApplyTheme` 目标（当前 0.5–1.5s 未包含说明）；
5. popup/menu 的 baseline 生命周期；
6. 如果当前实现没有 baseline，是否改为“空主题无操作”并在后续单独提供清除 API。

**建议**：本 CR 不要强行引入完整 baseline。把 `ApplyTheme({})` 定义为“无操作/保留当前样式”，删除“有旧主题恢复 baseline”的 P0 目标；若确实需要清除主题，另立 CR 或作为 P1。这样 P0 才能真正聚焦去重和空遍历，避免把性能修复变成主题模型补全。

### B5. 多根容器条件不能只共享一个 `m_activeMedia`

§3.1.1 要求“容器条件按各自根分别评估”，但当前 `ThemeRuntime` 只有单个 `std::string m_activeMedia`（`ThemeStyle.h:114`），`applyOne` 和 `refreshResponsive` 都依赖这一全局字符串。若 bench 命中某个容器条件、popup 没有该容器，而多个根共用同一个 active 字符串，popup 会把 bench 的容器条件误当成自身命中。

需要明确：

- 每个根（bench/popup/menu）维护独立的 container active 集；media 条件可共享，container 条件不可共享；
- `refreshResponsive` 的评估/应用接口要携带 per-root 上下文，而不是只传一个 root 后复用全局 `m_activeMedia`；
- `UICornerstone_GetActiveMedia` 在多根场景返回什么（bench 的 active 集，还是当前根）必须定义；
- 增加回归：主题含 container query，bench 命中、popup 不命中；popup 不应错误应用该规则；反之亦然。

这不是单纯的实现细节：B3 要求 popup/menu 覆盖，若容器条件共享会造成新的样式串扰。

## 3. 非阻塞建议

1. §7 的 `LoadLayout 2.5–4.5s` 仍是未计时估算；建议先落 §3.4 阶段计时，再以实测替代区间，避免把估算写进验收。
2. `ApplyTheme(designer_theme)` 的 0.5–1.5s 目标应明确是否包含 baseline 捕获/重建；若包含，当前目标没有依据。
3. 无响应式数据时跳过 `refreshResponsive` 是对的；同时应保证 `themeRawJson`、`hasThemableData` 查询和空主题语义不受影响。
4. `captureExplicitFromJson` 的“无键不建 map”优化可以与 P0 同批做，但必须覆盖颜色组、字体、padding、shadow、corner-radius 等所有当前已采集键，避免漏掉显式通道。
5. 一次性遍历/规则索引属于 P1；实施时先保证 base→媒体→容器→variant→显式覆盖的层序和 source order 不变，再比较性能。
6. 日志角色由 UIContext/Bench 创建参数传入 main/viewport 是合适的；Debug 输出顺序不应作为判断依据，Release 已能区分两次 Bench。
7. 空主题老主题场景还需区分“旧主题是 CSS 主题”与“旧主题是 legacy fonts/colors”：baseline 的构建来源不同，测试应覆盖两类。

## 4. 二次验收矩阵（补充前次）

- 无主题布局 + 后续 ApplyTheme：显式 colors/font/content-padding/shadow 优先（B1）。
- `ApplyTheme({})`：无旧主题无操作；有旧主题按最终确定的语义执行；两种路径分别测视觉/字体/显式值。
- 空主题/空 fonts/colors 不触发默认字体回放。
- popup/menu：主题应用与 responsive 变化都覆盖；container 条件按根隔离（B5）。
- 主题仅含 variables 时不执行应用遍历，但不影响后续显式值。
- `GetActiveMedia` 在 bench/popup/menu 多根下的返回值有明确断言。
- baseline（若本 CR 实施）：旧 CSS 主题→新主题、旧 legacy 主题→新主题、空主题恢复、`SetThemeVariant`、classes 变更、显式覆盖保护、popup/menu、控件新增/销毁。
- Debug/Release `auto=2`、`auto=3`、`validate_layout --strict` 零回归。

## 5. 放行结论

**有条件通过**：

- B1/B3/B4 的修订方向已满足；
- **B2 仍不实施就绪**：当前源码没有 baseline，修订版只引用 CR2001 设计，未给出本 CR 的实施契约；建议改为空主题无操作，或把 baseline 作为明确范围/前置 CR 并补齐设计；
- **B5 必须补充**：per-root 容器条件状态，不能复用单个 `m_activeMedia`；
- 完成 B2/B5 后，P0 的“去重 + 无有效主题跳过遍历 + 多根应用”可以放行；P1 在一次遍历/索引前后分别做性能与语义回归。
