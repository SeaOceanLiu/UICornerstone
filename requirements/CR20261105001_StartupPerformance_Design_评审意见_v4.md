# CR20261105001_StartupPerformance_Design 四次评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105001_StartupPerformance_Design.md`（四次修订，待复核）
- 结论：**通过（附两项实施前文字/语义修正，不阻塞进入编码）**

## 1. 已闭合

- **B1**：保留 runtime 与解析期显式采集，只跳过无有效主题的应用遍历；设计器 `LoadLayout → ApplyTheme` 的显式覆盖路径已覆盖。
- **B2**：空主题明确 no-op，并规定在 `setTheme` 之前判定；no-op 不触碰 `themeRawJson`、variant、active、显式 map；无旧主题/旧 CSS/旧 legacy 三类场景均有验收。
- **B3/B5**：bench/popup/menu 多根应用；`ActiveConditions { mediaIds, containerIds }` 显式传入 `applyAll/applySet/applyOne/inheritPass`；`SetThemeVariant`、`RefreshResponsiveTheme`、`Update`、`ClearThemeOverrides`、classes 变更均列入重应用入口；`GetActiveMedia` 固定为 bench 的 media+container 逗号串。
- **B4**：有效 legacy/主题数据判定、`hasResponsiveData` 覆盖 base + variant、阶段计时优先、规则索引保持层序等均已写明。

## 2. 实施前应修正的两处

### 2.1 `themeRawJson` 的 no-op 语义自相矛盾

- §3.1.3 写“仅 variables 时……原 JSON 仍保存在 `themeRawJson`”；
- §3.1.5 写 no-op 时“不修改 `themeRawJson`”。

两者对“仅 variables 主题”互相冲突。既然 §3.1.4/3.1.5 将无有效数据统一归入 no-op，建议明确：**no-op 时 `themeRawJson` 保留旧值**；删除 §3.1.3 的“仍保存”表述，或改为“若后续需要记录只读配置，由宿主自行保存，不由本 API 改写”。同样适用于空 `fonts/colors`。

### 2.2 `LoadLayout` 增量语义仍需定论

§4 只要求“明确整树替换还是增量；若 clear 清除已有显式 map，补测试或声明不支持”。这不是设计结论，实施者仍需猜：

- 若 `LoadLayout` 是整树替换：明确 `bench` 旧控件/旧显式 map 一并清理，并写进 API 语义；
- 若允许增量加载：当前 `LayoutParser::parseLayout` 无 theme 时调用 `themeRuntime->clear()` 会清掉已有控件显式 map，必须改为只清理本次新增控件，或明确不支持增量布局。

建议在本 CR 直接选定“整树替换”（与当前设计器用法一致），并删除“增量支持”承诺；这样 B1 的显式 map 生命周期闭合，测试范围也更可控。

## 3. 实施注意事项（非阻塞）

1. **no-op 解析成本**：`ApplyTheme({})` 仍需 JSON 解析；`<0.01s` 目标应包含解析，不要只测 `hasThemableData`。
2. **`GetActiveMedia` 格式**：bench 的 media+container 逗号串要保留现有格式；no-op 时保持旧值，空有效主题但已应用 old theme 的语义需与 no-op 一致。
3. **`hasResponsiveData`**：base + 当前 variant 均要检查；`variables`-only 明确返回 false。
4. **显式采集键覆盖**：无键不建 map 的优化要覆盖 colors、font、font-size/style、content-padding、shadow、corner-radius 等当前全部采集键。
5. **阶段计时**：先落 §3.4，再用同机 Release 实测更新 §7；`LoadLayout 2.5–4.5s` 仍只是估算，不应作为硬验收。
6. **多根容器回归**：bench 命中/popup 不命中及反向；`SetThemeVariant`、`RefreshResponsiveTheme`、`Update`、`ClearThemeOverrides` 后 popup/menu 仍保持各自条件。
7. **日志角色**：由 UIContext/Bench 创建参数传入 main/viewport；Debug 输出顺序不作为依据，Release 已能区分两个 Bench。

## 4. 放行结论

**通过**。设计已具备进入 P0 实施的条件；上述 2.1/2.2 作为实施前文字/语义修正，不改变 P0 主体方案。实施顺序仍按 §6：先 P0 去重/快速路径/多根条件，再阶段计时，最后 P1 一次遍历与索引。

设计器侧保持设计器主题自动应用；P0 完成后用 `layouts/main_layout.json` + `layouts/designer_theme.json` 复测启动首帧。
