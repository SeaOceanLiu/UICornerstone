# CR20261105001_StartupPerformance_Design 三次评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105001_StartupPerformance_Design.md`（三次修订，待复核）
- 结论：**有条件通过（可进入实施；B2/B5 的实施语义还需在两处写死）**

## 1. 已闭合

- **B1**：保留 themeRuntime 与解析期显式采集，仅跳过无有效主题的应用遍历；符合设计器 `LoadLayout → ApplyTheme` 的真实路径。
- **B2**：明确 `ApplyTheme({})` 为 no-op，不再引入 baseline/清除主题语义；避免性能 CR 扩张成 CR2001 主题模型补全。方向正确。
- **B3/B4**：多根 popup/menu 应用、非空有效主题数据判定、`GetActiveMedia` 返回 bench 主根，均已写入设计与验收。
- 阶段计时优先、规则索引不改选择器语义、空主题旧主题分类测试等补充合理。

## 2. 实施前需写死的 B2 细节

“空主题 no-op”必须在**调用 `setTheme` 之前**判定，否则会先清掉当前 `m_sheet`，再无法真正 no-op。设计需明确：

1. `ApplyTheme` 先用解析后的 JSON 计算 `hasThemableData`；false 时直接成功返回，不调用 `setTheme`、不修改 `themeRawJson`、不改 `m_variant`/`m_activeMedia`/显式 map。
2. 仅 `variables`、空 `styles`、空 `fonts/colors`、空 legacy 主题都归入 no-op；不能出现“JSON 已替换、运行时空”的半状态。
3. 无旧主题和有旧主题两种场景均 no-op；有旧 CSS/legacy 主题时保持当前样式和字体。
4. 明确 `themeRawJson` 在 no-op 时保留旧值；否则后续诊断/重放会与运行时主题不一致。
5. 增加测试：`ApplyTheme({})`、仅 variables、空 `fonts/colors` 三种输入，在“无旧主题/旧 CSS/旧 legacy”下均不改变字体、颜色、显式值和主题变体。

## 3. 实施前需补齐的 B5 路径

修订版已定义 `ActiveConditions { mediaIds, containerIds }` 和 per-root 容器评估，方向正确；但当前引擎的 `m_activeMedia` 是单个全局字符串，且多个入口仍只处理 bench：

- `UICornerstone_Update` 的 dirty 消费只对 bench 调 `refreshResponsive`；
- `UICornerstone_RefreshResponsiveTheme` 只传 bench；
- `SetThemeVariant` 内部 `setVariant` 只对 bench `applyAll`；
- `ClearThemeOverrides`/`applyControl` 仍依赖当前全局 active 状态。

设计需明确：**所有会重新应用主题的入口**都要使用 per-root `ActiveConditions`，至少 `ApplyTheme`、`SetThemeVariant`、`RefreshResponsiveTheme`、`Update` dirty 路径、`ClearThemeOverrides`/classes 变更路径；否则 popup/menu 在 resize、变体切换或显式覆盖清除后会退回旧的容器条件。

另外需定义：

- `GetActiveMedia` 当前返回值同时包含 media 和 container ID；改为 bench 语义后，是否保留 container ID、格式是否变化；
- `ActiveConditions` 在 `applyOne`、`applySet`、`inheritPass` 中如何传递；不能只给 `refreshResponsive` 增加参数而 `applyControl` 仍读全局状态；
- 新增回归：bench 命中/不命中 container，popup 反向；`SetThemeVariant`、`RefreshResponsiveTheme`、`Update` 后 popup/menu 仍正确；`GetActiveMedia` 只报告 bench。

## 4. 其他仍建议补充

1. **增量加载**：修订版只说“实现时检查 `themeRuntime->clear()`”，仍建议明确 `LoadLayout` 是否会清掉已有控件的 `m_explicit`；如果会，需补增量加载测试或明确不支持增量布局。
2. **阶段计时**：§7 的 `LoadLayout 2.5–4.5s` 仍是估算；实施顺序应先落 §3.4，再以同机 Release 数据更新目标，避免把估算当验收基线。
3. **`hasResponsiveData`**：需覆盖 base/variant 中的 media/container；仅 base 判定会漏 variant 响应式规则。
4. **显式采集优化**：`captureExplicitFromJson` 的“无键不建 map”要覆盖当前全部采集键（colors、font、font-size/style、content-padding、shadow、corner-radius），不能只覆盖其中最常用的几个。
5. **日志角色**：由 UIContext/Bench 创建参数传入 main/viewport 合理；Release 已能区分两个 Bench，Debug 输出顺序不应作为重复加载依据。

## 5. 放行结论

**有条件通过**：

- 设计已解决上一轮的 B1/B2/B3/B4 主要问题，B5 的多根容器隔离方向也正确；
- 实施前必须补写 B2 的 no-op 状态保护（尤其是 `setTheme` 之前判定与 `themeRawJson` 保留）和 B5 的全部重应用入口；
- 完成后可按 P0“去重 + 无有效主题跳过 + 多根应用”实施；P1 的一次遍历/索引按原计划先做语义回归和阶段计时。

设计器侧仍保持设计器主题自动应用；P0 完成后需用 `layouts/main_layout.json` + `layouts/designer_theme.json` 重新测量启动到首帧。
