# P075_ResourcePipeline_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-08
- 对象：`design/P075_ResourcePipeline_Design.md`（P0-74⑥ / P0-75 / P0-78 / P0-79，状态：待评审）
- 结论：**通过，放行实施**（四项根因核实与设计器实测一致；落地后设计器可回收 4 项绕过）

## 1. 评审要点

### 2.1 P0-74⑥ `viewport.id`——✓
- 方案（解析 `id` → `controlsById[bench]`）✓；设计器已随导出携带 `viewport.id`（Bench 编辑"画布 id"行，模型/元数据自持、唯一性校验已做）→ 引擎落地后宿主 `FindControl(id)` 定位根画布 ✓。
- `GetRoot`/Binding `Root()` 已暴露的备注 ✓——设计器 Bench 编辑可用（bench 透明/背景直连需求见 P076 Q-3，非阻塞）。

### 2.2 P0-75 items 独立 `hover-background-color`——✓（Q-1 答复见下）
- 两控件补独立键 + tree 侧 `bgColor.setHover + hasHoverBg`（**不置 bgMask/hasBgStyle**——与 P0-65② 解耦语义一致 ✓，仅 hover 不引常态落色 ✓）；status-bar 侧 `setStatusItemHoverBackgroundColor` ✓。
- schema/docs 同步 ✓。
- **设计器回收**：引擎落地后移除导出侧对象形态归一（`serializeLayoutJson` 的 mergeHover——模型 1:1 直出独立键）；**导入侧拆分归一保留**（兼容已导出的旧文件对象形态）。设计器现网导出两种形态均可用，无阻塞。

### 2.3 P0-78 顶层 provider + delegate 回退——✓（本批对设计器收益最大）
- `setFilesystemFallback` + readFile/exists 末尾回退 ✓；MainWindow 换 MemoryResourceProvider + 回退 ✓——顶层 `resourceProviders` 挂载（memory 型判定）生效 ✓。
- **设计器回收**：①`RegisterResource` 探针（不再需要）；②`RegisterFontsTo`（delegate 回退承接 `fonts/...`）；③`RegisterAnimationFramesTo`（回退承接 `animations/...` 帧）——三项一并删除。
- Q-2（回退顺序）：同意"精确 → 目录前缀 → assets 剥离 → delegate 回退"；绝对路径经 `operator/` 直读 ✓（设计器用户 provider 配置存在绝对目录场景，需保持 ✓）。
- Q-3（provider 对象语义）：同意保持 `getResourceProvider()` 返回同一对象；**建议明确**：`mountPath` 懒建 delegate 与 `setFilesystemFallback` 幂等共存（先建者生效，basePath 一致——均为 resourceRoot）。

### 2.4 P0-79 动画描述经 provider——✓（用户"动画不显示"最终修复点）
- `loadAniDesc` 先 provider（含 provider: 剥离后的资源 ID）→ 保留 fopen 回退（Q-4 同意）✓；覆盖**延迟补读（:558）**入口 ✓（易漏点，确认到位）。
- **设计器回收**：`ResolvePreviewAnimationPaths` + 编辑态 writeStringPropTo 的动画 path 解析——两项删除（provider: 原值直通）；属性行读回特例（显示模型原值）可一并简化。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | 75：独立键与对象 {normal,hover} 优先级 | 同意"独立键后解析覆盖（独立键优先）"+文档注明；设计器导入侧两种都归一，导出侧引擎落地后统一为独立键 |
| Q-2 | 78：回退顺序/绝对路径 | 同意（见 §2.3）；绝对路径必须可读（用户 provider 配置实测为绝对目录） |
| Q-3 | 78：MainWindow 换 provider 语义 | 同意；建议文档注明 delegate 幂等共存与 basePath 一致性 |
| Q-4 | 79：provider 未命中回退 fopen | 同意保留（文件路径动画兼容） |

## 3. 设计器联测计划（同步后）

1. 删除 4 项绕过（探针/字体注册/帧注册/动画路径解析）+ 复测：预览 provider 图片/动画加载、`playing/loop=1`、字体正常；
2. P0-75 导出切换独立键 + 旧文件（对象形态）导入回归；
3. `viewport.id` → 预览实例 `FindControl(画布 id)` 探针；
4. 全量：auto=1/2/3、strict、矩阵。

## 4. 放行

**结论：放行实施**。四项根因与设计器实测一致；P0-78/79 为"导出产物"级必需修复（绕过仅救设计器预览）；落地后设计器按 §3 回收并回归。
