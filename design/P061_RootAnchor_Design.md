# P061_RootAnchor_Design — 根级锚定（Bench 布局引擎支持）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-03 追加：P0-61；用户拍板走 Bench 路线）
> 前置：P0-60（锚点即时重排 + 稀疏语义 + hasAnchor）**已实施并同步**——根上叠加物（网格/参考线/框选/手柄）不再被塌
> 状态：**已放行，已实施**（2026-10-03）
> 复核：`requirements/P061_RootAnchor_Design_复核意见.md`（通过，放行实施；五问全确认 + 2 项注意）
> 实施备注：① `UICornerstone_GetRoot` + Binding `Root()`（子视口返回自身 bench；复用 child-id/layout/anchor 属性链）；② `Bench::resized` 补 `reflowChildren()`；③ `SetCanvasSize` 全模式即时 `setRect + recompute + reflow`；④ 措辞对齐"设计画布尺寸"（§2.4 + declarative-syntax 6.5）；⑤ 测试：test_property_cabi 168/0（GetRoot/画布即时/根锚定跟随）；⑥ 回归仅 4 项预存失败

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-61① | 设计器拿不到 bench 句柄，顶层控件 `child-id`+`anchor` 无处可写 | **确认** | C ABI/Binding 均无 `GetRoot`/`GetBench`（grep 零命中）。Bench 继承 Panel（`class Bench : public Panel, public TopControl`，Bench.h:10），`m_layoutEngine`/`m_anchorItemProps`/`reflowChildren` 天然可用；`validateControl` 对 bench 自身有效（`instanceHoldsControl` 首层即 `cur == instance->bench`）→ 直接暴露句柄即可复用既有 child-id/anchor 属性链 |
| P0-61② | `Bench::resized` 不触发 `reflowChildren()` | **确认** | `Bench::resized`（Bench.cpp:123-130）：Off → `Panel::resized(newRect)` + `recomputeViewportTransform()`；fit/stretch → 仅 recompute。**无 reflow** → 窗口 resize（MainWindow.cpp:140 `bench->resized(...)`）后锚定子控件不跟随 |
| P0-61③ | off 模式 `SetCanvasSize` 仅记录不即时应用 | **确认** | `UICornerstone_SetCanvasSize`（UICornerstoneAPI.cpp:639-652）：`canvasWidth/Height` 记录后，仅 `mode != Off` 时 `setRect + recompute`；Off 分支依赖"下一次 recompute"才把 bench rect 置为显式画布。注：`recomputeViewportTransform` 的 off 分支（Bench.cpp:169-174）**确实使用显式画布**（`canvasW/H` 优先）→ 语义上 Off 也应即时应用（仅时机差异） |

**前置确认**：P0-60 稀疏语义（`applyAnchor` 未显式锚定即 continue + `hasAnchor`）已实施 → 根切换到 anchor 布局时，设计器在 bench 上的叠加物（网格/参考线/框选/手柄/弹窗）保持自身 rect，不被塌左上。

## 2. 修改方案

### 2.1 P0-61① 暴露根句柄
- C ABI：`UICornerstone_GetRoot(UIInstance instance) → UIControlHandle`（返回本实例 bench 句柄；子视口实例返回其自身 bench；非法/销毁中返回 NULL）；
- Binding：`Control UICornerstone::Root()`（内部 GetRoot + 既有 `FromHandle` 包装，共享同一代理状态）；
- 设计器即可：`SetString(root,"child-id",id)` + `SetEnum(root,"anchor",…)` / `SetFloat(root,"anchor-offset-x/y",…)` / `SetEnum(root,"layout","anchor")`——全部复用 P0-59/60 既有链路；
- 读回同链（`GetEnum(root,"layout")` 等）。

### 2.2 P0-61② Bench::resized 补重排
- `Bench::resized` 末尾：`if (m_layoutEngine) reflowChildren();`（Off 分支 rect 变化 → 锚定子控件跟随窗口；fit/stretch 分支 rect 不变，重排为幂等再应用）。

### 2.3 P0-61③ SetCanvasSize 即时应用
- `UICornerstone_SetCanvasSize`：**所有模式**统一 `setRect(SRect(0,0,w,h))` + `recomputeViewportTransform()` + `bench->reflowChildren()`（有引擎时）——与 `recomputeViewportTransform` 的 off 分支语义一致（显式画布优先），仅将"下一次 recompute 生效"改为即时；
- 解析路径（LayoutParser.cpp:61 直写 `m_viewportTarget->canvasWidth`）保持现状（parse 后 create/recompute 统一应用）。

### 2.4 坐标/尺寸语义（文档注明）
- 根尺寸 = **显式设计画布尺寸**（`SetCanvasSize`，设计器按设计画布档位声明：1024×768/1280×800/…/自定义，缺省=窗口视口）→ 锚定目标=**设计画布边界**（缩放/窗口变化下稳定，固定窗体语义；导出应用里根面板随窗口 → 锚定跟随窗口）；
- 首次设置顶层锚定时由设计器确保根为 anchor 布局（`SetEnum(root,"layout","anchor")`，引擎已支持）；
- 未显式锚定的根子控件（叠加物/弹窗）保持自由布局（P0-60 稀疏）。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | `GetRoot` 返回非空且等于实例 bench（GetRect 与 SetCanvasSize 一致）；子视口实例返回自身 bench | test_property_cabi / 探针 |
| 2 | 根切 anchor + 对顶层控件设 anchor/offset：即时重排（P0-60 已通）；`SetCanvasSize` 后**立即**重排（根 rect 变化 + 锚定子控件跟随） | test_property_cabi（GetRect 前后对比） |
| 3 | 未锚定的根子控件在 `SetCanvasSize`/窗口 resize 后保持 rect | 同上（稀疏断言） |
| 4 | 窗口 resize 路径（Off 模式 `bench->resized`）触发锚定重排 | 断言 via SetCanvasSize 等价路径 + 代码走查；真实 resize 人工/视觉 |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **API 形态**：C ABI `UICornerstone_GetRoot(inst)` + Binding `Root()` 返回 `Control`——确认？（备选：`GetBench` 命名；建议 GetRoot 与"根/画布"语义一致）
2. **SetCanvasSize 全模式即时应用**（Off 由"下次 recompute 生效"改为即时；fit/stretch 行为不变）——确认？
3. **Bench::resized 重排条件**：有引擎即重排（fit/stretch 下幂等）——确认？
4. **根尺寸语义**：显式画布优先（缺省=窗口视口），设计器负责按可见逻辑区声明——确认并写入文档？
5. **设计器过渡**：删 `forceReflow`（P0-60 已可删）+ 根父级解析支持"根"——确认？

## 5. 配套改动

- 代码：`include/UICornerstoneAPI.h` + `src/UICornerstoneAPI.cpp`（GetRoot + SetCanvasSize 即时）、`src/Bench.cpp`（resized 重排）、`binding/*`（Root + DynamicApi）；
- 测试：test_property_cabi 扩展（GetRoot/SetCanvasSize 即时/稀疏）+ 回归；
- 文档：declarative-syntax 6.5（根级锚定语义）、CABI/Binding 速查表（GetRoot/Root）；设计文档标注 + 复核归档 + make_release + 同步。
