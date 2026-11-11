# CR20261105001_StartupPerformance_Design 评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105001_StartupPerformance_Design.md`（分析/设计，待评审）
- 结论：**有条件通过（需修订 P0 后再实施）**

总体判断：热点分析方向正确，尤其是空主题仍慢、主题重复遍历、`inheritPass` 无条件执行、显式覆盖采集、阶段计时缺失，均与当前引擎源码吻合；P0/P1 分层和“先计时再优化”的顺序也合理。但 **P0 的 3.1.3 按当前写法会造成显式覆盖语义回归**，空主题语义和 popup/menu 覆盖也未闭合，不能直接放行实施。

## 1. 认可部分

### 1.1 热点定位基本准确

- `UICornerstoneAPI.cpp:1601` 的 `ApplyTheme` 确实先对 bench/popupPool/menuPool 执行 `applyAll`，随后 `refreshResponsive` 末尾再次 `applyAll(root)`；两遍遍历成立。
- `ThemeStyle.cpp:328` 的 `setTheme` 无条件创建 `m_legacy`；`ThemeStyle.cpp:413` 在空主题下仍可能执行 `applyCommonColors` + `applyFontToControl`，`Theme::getFontSize`/`getFontName` 会回落到默认 16/HarmonyOS，确实可能重置字体 baseline。
- `LayoutParser.cpp:52-55` 在布局无 `theme` 时调用 `themeRuntime->clear()`；`UICornerstoneAPI.cpp:1743-1747` 仍执行 `applyAll` 和 `markResponsiveDirty`，下一帧 `Update` 再 `refreshResponsive`，空遍历成立。
- `LayoutParser.cpp:3058` 对每个控件调用 `captureExplicitFromJson`；`ThemeStyle.cpp:581` 先执行 `m_explicit[ctl]`，即使没有主题相关键也会创建条目。固定采集成本判断成立。
- `ThemeStyle.cpp:540` 的 `inheritPass` 无条件递归；`ThemeStyle.cpp:404` 的规则匹配为全规则扫描，O(控件 × 规则) 判断成立。
- 两条 `Loading resources...` 来自两个 Bench（主实例 + CanvasPane 子视口）判断正确；Release 启动日志可清楚看到第二条出现在 `skeleton` 之后、`canvas` 之前。Debug 日志偶发两行都在 `[Startup] main` 前，属于输出顺序/测量点干扰，不应作为“同一实例重复加载”的证据。

### 1.2 分层与验收方向

- P0 先去掉空主题/重复遍历，P1 再做合并遍历和规则索引，优先级合理。
- 增加默认关闭的 Release 阶段计时，符合“先量化再优化”的原则。
- 不新增公开 API、保持 `LoadLayout`/`ApplyTheme` 行为不变的方向正确。
- 日志改为带角色前缀（main instance / canvas viewport）合理；不需要消除子视口所需的第二次初始化。

## 2. 阻塞问题（实施前必须修订）

### B1. P0 3.1.3 跳过 `themeRuntime`/显式采集会造成后续 ApplyTheme 语义回归

设计写：“LoadLayout 无主题不建 runtime、不应用”。但当前显式覆盖的保留机制依赖 `themeRuntime`：

- `LayoutParser.cpp:3058` 仅在 `m_themeRuntime` 非空时采集显式覆盖；
- 设计器启动流程正是 `LoadLayoutFromFile(main_layout.json)` 之后在 `App.cpp:420` 调用 `applyDesignerTheme()`；
- `main_layout.json` 中的显式颜色/字体/圆角/阴影依赖 `m_explicit` 在后续主题应用后回放；若加载时跳过 runtime 和采集，后续 `ApplyTheme(designer_theme)` 会用主题规则覆盖这些显式值。

这不是理论风险，而是设计器当前启动路径。**P0 不能直接跳过 runtime/采集**。

可行修订：

1. **保留 themeRuntime 和解析期采集**，仅在没有主题数据时跳过 `applyAll`/`markResponsiveDirty`；这仍能消除 LoadLayout 的空遍历成本，且保持后续 ApplyTheme 的显式覆盖语义。
2. 如果确实要跳过采集，必须先实现 P1 的“轻量显式键值存储”，并在 P0 就用它替代 runtime 采集；不能把 P1 的语义补齐留到后面的阶段。
3. 增加回归测试：布局无 `theme`，但控件有显式 `colors`/`font`/`content-padding`/`shadow`；先 `LoadLayout`，再 `ApplyTheme`；显式值必须仍然优先。

### B2. 空主题 `ApplyTheme({})` 的语义未定义，当前“快速返回”与现状不一致

设计已经识别“不能简单 early-return”，但验收仍写“空主题从 5.5s 降到接近 0”。需要先明确空主题到底表示：

- **无操作**（保留当前已应用的样式/字体）；还是
- **清除当前主题并恢复 baseline/默认值**。

当前实现并不是真正的 clear：`setTheme({})` 会清空规则，但 `m_legacy` 仍非空，`applyOne` 仍可能重置字体；旧主题已写入控件的颜色也不会仅因空主题自动恢复。若直接加空数据快速返回，会改变“空主题重置字体”的现有行为；若要求恢复默认，则需要 baseline 快照或重建机制，设计必须给出数据结构和生命周期。

建议在 CR 验收中明确选择一种语义，并分别测试：

- 无旧主题时 `ApplyTheme({})`；
- 有旧主题时 `ApplyTheme({})`；
- `ApplyTheme({})` 与后续 `LoadLayout`/`ApplyTheme` 的组合。

在语义未定前，不应把 `< 0.1s` 写成无条件验收目标；最多写成“空主题无操作路径”。

### B3. 去掉第一遍 `applyAll` 后，popupPool/menuPool 没有覆盖

当前 `ApplyTheme` 对三类根分别 `applyAll`：

- `instance->bench`
- `instance->popupPool`
- `instance->menuPool`

而 `refreshResponsive` 只接收一个 `root` 并只执行 `applyAll(root)`。设计中的“只调用一次 refreshResponsive/删除首遍 applyAll”如果按字面实施，会丢失 popup/menu 的主题应用，或让它们停留在旧 `activeMedia`。

修订要求：

- 要么保留 popup/menu 的 `applyAll`，但放到 `refreshResponsive` 计算完 activeMedia 之后；
- 要么把 `refreshResponsive` 扩展为“计算一次条件 + 对多个根应用”的接口；
- 增加 popup/menu 主题应用回归，包括媒体/容器条件变化后的 popup/menu。

### B4. `hasThemableData` / `legacyData` 的判定需要精确到“有效数据”

`theme.contains("fonts") || theme.contains("colors")` 对空对象也为 true；空 `fonts: {}` 当前仍会触发默认字体回放。建议：

- 用“非空有效数据”判定 legacy，而不是仅看键存在；
- `hasThemableData` 同时覆盖：`styles` 非空规则、非空 media/container、非空变量、非空 legacy `fonts/colors`；
- 明确 `variables` 单独存在但没有规则时是否算主题；若不算，必须保证不影响 `themeRawJson`/查询语义。
- 该判定是 P0 快速路径的正确性基础，建议配单元测试。

## 3. 非阻塞建议

1. **先补阶段计时再承诺 P0 的 LoadLayout 收益**：当前 `LoadLayout` 5.8–6.8s 中解析/控件创建/字体资源准备的占比未知；P0 去主题遍历未必能把 LoadLayout 降到 2.5–4.5s。§7 的数字应标注为待计时验证，不应作为硬目标。
2. **`captureExplicitFromJson` 先做低成本优化**：没有主题相关键时不要创建 `m_explicit[ctl]`；这可以单独降低解析成本，也不影响 B1 的语义。后续再考虑 P1 稀疏存储。
3. **无响应式条件时跳过 `refreshResponsive` 扫描**：不仅是空主题，`designer_theme` 当前没有 media/container；可在 `ApplyTheme` 中判断 `hasResponsiveData()`，直接一次 `applyAll`，避免 `refreshResponsive` 的无条件树扫描。
4. **一次遍历合并必须保留层序**：base → variant → media → container → 显式覆盖的优先级与 source order 不能因合并改变；variant 覆盖 base 的现有语义要单独测试。
5. **规则索引先做候选桶，不改变匹配/排序**：按类型/类/ID 建索引后，仍需按原顺序做 specificity/order 合并；避免把“索引优化”变成“选择器语义重写”。
6. **`inheritPass` 短路**：没有字体/text 声明和显式字体时，跳过递归；但必须保留显式字体和父链继承语义。
7. **日志角色实现**：建议由 `UIContext`/Bench 创建参数传入 `main`/`viewport` 角色，而不是在 Bench 里猜测；Debug 日志顺序问题可一并通过角色消除。
8. **显式采集生命周期**：控件销毁/重建后清理 `m_explicit` 的陈旧指针；当前设计未提，但性能优化不应遗留悬垂键（即使当前只读 map 不访问，也会增加内存）。

## 4. 必需的验收矩阵

除设计 §4 外，至少增加：

- 无主题布局 + 后续 `ApplyTheme`：显式颜色/字体/圆角/阴影仍然优先（B1）。
- `ApplyTheme({})` 无旧主题/有旧主题两种语义（B2）。
- `ApplyTheme(designer_theme)` 后 popup/menu 使用新主题（B3）。
- 布局无主题时字体 baseline 与“未调用 ApplyTheme”一致。
- 主题含 media/container 时 resize、Splitter 拖动后的重算；主题不含响应式条件时不发生无意义全树扫描。
- 主题规则 0/10/100/1000 的性能曲线；空布局/单控件/主布局分别记录 parse、create、theme、reflow、total。
- Debug/Release `auto=2`、`auto=3`、`validate_layout --strict` 零回归。

## 5. 放行结论

**有条件通过**：

- 分析、热点定位和 P0/P1 方向可以接受；
- 但 P0 的 `3.1.3 跳过 runtime/采集` 必须改为“保留 runtime/采集，仅跳过无主题遍历”，或先完成等价的稀疏显式存储；
- 空主题语义（B2）、popup/menu 覆盖（B3）、有效数据判定（B4）必须在实施前写进设计并纳入验收；
- 完成 B1–B4 并补阶段计时后，可放行 P0；P1 在一次遍历/索引前后分别做性能与语义回归。

设计器侧当前保持自动应用 `designer_theme`，因此 B1 的设计器启动路径必须先验证通过，才能接受 LoadLayout 的“无主题快速路径”。
