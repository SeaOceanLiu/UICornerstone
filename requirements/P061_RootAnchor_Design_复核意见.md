# P061_RootAnchor_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-03
- 对象：`design/P061_RootAnchor_Design.md`（P0-61，状态：待评审）
- 结论：**通过，放行实施**（附 2 项注意：§2.4 根尺寸语义措辞需对齐设计器最新"设计画布尺寸"；applyCanvasSize 过渡可简化）

## 1. 评审要点

### 现状核实（§1）✓（源码抽验一致）

- ① C ABI/Binding 无 `GetRoot`/`GetBench` ✓；**"Bench 继承 Panel + `validateControl` 接受 bench → 直接暴露句柄即可复用既有 child-id/anchor 链路"**——判断正确（抽验 `instanceHoldsControl` 首层 `cur == instance->bench`）✓。
- ② `Bench::resized`（Bench.cpp:123-130）无 reflow ✓。
- ③ `SetCanvasSize`（UICornerstoneAPI.cpp:639-652）off 分支仅记录、依赖下次 recompute ✓。
- **前置 P0-60 已实施并同步抽验** ✓：`hasAnchor`（LayoutEngine.h:20）/ `applyAnchor` 稀疏跳过（LayoutEngine.cpp:312）/ setter 即时重排（Panel.cpp:138-156）/ 读回未设返回 0（:230）——设计器随批已删 `forceReflow`（本批完成）。

### 方案（§2）✓

- 2.1 `GetRoot` + Binding `Root()`：最小暴露、复用 P0-59/60 属性链（`child-id`/`anchor`/`anchor-offset-x/y`/`layout`）✓。
- 2.2 `Bench::resized` 有引擎即 reflow（fit/stretch 幂等）✓。
- 2.3 `SetCanvasSize` 全模式统一 `setRect + recompute + reflow`——与 off 分支"显式画布优先"语义一致，仅把"下次生效"改即时 ✓。
- 2.4 语义方向正确，**措辞需更新**（见实施注意 1）。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | API 形态 `UICornerstone_GetRoot(inst)` + Binding `Root()`？ | **确认**（GetRoot 与根/画布语义一致；子视口实例返回其自身 bench 正确） |
| 2 | `SetCanvasSize` 全模式即时应用（off 由"下次 recompute"改即时；fit/stretch 不变）？ | **确认** |
| 3 | `Bench::resized` 有引擎即重排（fit/stretch 幂等）？ | **确认** |
| 4 | 根尺寸语义：显式画布优先（缺省=窗口视口），设计器负责声明？ | **确认**，但措辞按实施注意 1 更新（设计器声明的是**设计画布尺寸**，非可见逻辑区） |
| 5 | 设计器删 `forceReflow`（P0-60 已可删）+ 根父级解析支持"根"？ | **确认**（forceReflow **已删**——本批完成；根父级解析随 P0-61 同步实现） |

## 3. 实施注意（放行附项）

1. **§2.4 措辞对齐"设计画布尺寸"（重要）**：设计器在 P0-61 立案后已实现**设计画布尺寸（基准窗体）**功能——工具栏档位（1024×768/1280×800/1440×900/1920×1080/自定义）+ `SetCanvasSize` 声明的是**设计尺寸**（缺省 1280×800），**不是**可见逻辑区（contentW/zoom）；溢出由画布平移（中键/空格+左键）与视口让条滚动条处理。因此：
   - 锚定目标 = **设计画布边界**（缩放/窗口变化下稳定——固定窗体语义；导出应用里根面板随窗口 → 锚定跟随窗口）；
   - 请把 §2.4 与 declarative-syntax 6.5 的"设计器按可见逻辑区声明"改为"按设计画布尺寸声明"。
2. **applyCanvasSize 过渡可简化（可选）**：P0-61③ 后 `SetCanvasSize` 即时应用——设计器 `applyCanvasSize` 里的"`SetViewport` 同值强制 recompute"变为冗余（保留无害，顺带刷新视口 rect；实施同步后可视情况简化）。
3. **设计器随批计划（同步后）**：顶层控件锚定行父级解析 → `m_viewport->Root()`；首次设置顶层锚定时自动 `SetEnum(root,"layout","anchor")`（未锚定叠加物/弹窗经 P0-60 稀疏保持原位）；删 `forceReflow`（已删）。

## 4. 放行

**结论：放行实施**。三点现状核实与方案（GetRoot 暴露/重排时机/画布即时应用）质量高，"暴露句柄复用既有属性链"的路径最小；附 2 项注意（根尺寸语义措辞对齐设计画布尺寸/applyCanvasSize 过渡简化）。实施完成后同步 subModules，设计器随批：根父级解析 + 首次锚定自动切根布局，端到端联测（顶层 StatusBar 底拉伸贴设计画布底 + 窗口 resize 稳定）。
