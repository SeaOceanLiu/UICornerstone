# InstanceScale_CABI_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-16
- 对象：`design/InstanceScale_CABI_Design.md`（状态：待审核）
- 结论：**通过（方案 A 接受，4 个待审核问题全部同意）**，附 2 项补充建议（不阻塞实施）

## 1. 总体评价

设计质量高。最有价值的是 §3.3——发现了需求方（我方）未预见的 `recomputeViewportTransform` 冲突：off 分支强制 `setScale(1,1)`，设计器 splitter 拖动画布（resize → recompute）会把手动 zoom 静默重置。方案 A（`m_scaleOverride` 标志）改动小、不破坏既有语义、fit/stretch 切换时清除 override 的"引擎接管"语义清晰，接受。

## 2. 对 4 个待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 方案 A（off 分支尊重手动缩放 override） | **接受**。B/C 否决理由成立 |
| 2 | `SetInstanceScale(1,1)` = override 保持 true、值=1，无需单独清除接口 | **确认**。off 模式下 override=true/1 与 false 行为等价，省一个接口 |
| 3 | `GetInstanceScale` 返回复合快照（fit/stretch 下为引擎自动值） | **确认**。设计器画布仅 off 模式使用，无实际影响；§3.4 已标注语义即可 |
| 4 | Binding 用 `GetInstanceScaleX/Y` 两个 getter | **确认**。与现有 binding 风格一致 |

## 3. 补充建议（不阻塞，请并入设计文档"设计器侧配合改动"节）

### 3.1 覆盖层类子控件会被 bench scale 二次缩放（遗漏项）

视口内除被设计控件外，还有两类**设计器自绘覆盖层**也是 bench 子控件，`refreshScaleWith` 会同样刷新它们：

| 覆盖层 | 现状绘制方式 | bench scale 下的影响 | 设计器对策 |
|---|---|---|---|
| 网格瓦片 Image | rect=视口物理尺寸 + cellPx×zoom 重建瓦片纹理 | drawRect ×scale → 网格密度**不再随 zoom 变化**（tile 按纹理原尺寸平铺，缩放只扩大平铺范围） | 瓦片纹理仍按 cellPx×zoom 重建（保持密度语义）+ **rect 除以 scale 抵消**（rect=(W/s, H/s) → drawRect=W ✓） |
| 选中框 Shape（8 向控点） | 图元坐标按逻辑 rect×zoom 手算 | ×scale 后双重缩放（框/控点错位） | 图元坐标改为**逻辑坐标直绘**（不再 ×zoom），尺寸类常量（线宽/控点 6px）同样除 scale 或接受视觉缩放 |

建议将上表并入设计文档"影响面"或独立"应用侧注意"小节——这是从像素坐标旁路迁移到 scale 语义时最容易踩的坑（需求方自查发现，供后续同类迁移参考）。

### 3.2 recompute 触发时机确认

§2 列出触发点：resized / SetViewport / CreateViewport。请确认**放置/删除控件、SetRect 等内容变更不触发 recompute**（当前源码层面如此）。若未来新增触发点，override 语义需重新评估——建议在 ViewportScale_Design.md 同步说明中加一句约束。

## 4. 细节确认（无异议）

- §3.1 `setScaleX(v)` 委托 `setScale(v, m_yScale)`：等价委托，幂等化纯收益；`setScaleX(2)` 后 `setScaleY(2)`（y 已=2）提前 return 语义正确。
- §5 验收对照：需求放行条件 1-5 全覆盖，测试计划（test_viewport_scale 扩展 + override 新用例）完整。
- §5 条目 3 幂等断言"退化为返回值+scale 不变"：接受（行为断言工程务实）。
- §6 文档同步范围（ViewportScale_Design.md + 手册 5.6）：认可；"fit/stretch 下 SetInstanceScale 会被 recompute 覆盖"的标注请保留在用户手册。

## 5. 放行

- **结论：放行实施**（按方案 A + 4 项确认）。
- 实施完成后请同步 subModules，我方按需求 §4 清单 + 本复核 §3.1 对策迁移设计器侧（删除像素坐标旁路），并做像素级回归（放置 → SetInstanceScale(1.5) → rect/字号/网格密度/选中框对齐）。
