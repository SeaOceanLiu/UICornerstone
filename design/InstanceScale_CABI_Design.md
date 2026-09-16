# InstanceScale_CABI_Design — 实例缩放 C ABI（画布缩放语义）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_需求_实例缩放CABI.md`（2026-09-15，P1）
> 复核意见：`requirements/InstanceScale_CABI_Design_复核意见.md`（2026-09-16，**通过——放行实施**；方案 A 接受，4 项确认；附 2 项补充建议）
> 状态：**已放行，实施中**

## 1. 问题与目标

设计器中央画布为**子视口**（viewport instance），画布缩放（工具栏 -/+ 档位）期望：
- 控件 rect 按比例跟随（视觉位置/大小变化）
- 文本字号同步跟随（重新光栅化，非位图拉伸）
- 控件**逻辑 rect 不变**（模型存逻辑坐标，缩放是视图变换）

引擎已原生支持该语义（`Bench::setScaleX/Y` + `refreshScaleWith` 链 + Label `scaledSize = m_fontSize × getScaleXX()`），**缺口仅是 C ABI/Binding 暴露**。设计器当前被迫用"像素坐标 + 手动同步 rect/字号"旁路（每类控件字号属性名不统一，维护成本高）。

目标：`SetInstanceScale` 一行调用完成整树缩放，设计器删除旁路代码。

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| 根缩放入口 | `Bench::setScaleX/setScaleY`（Bench.cpp:212-225）：置 m_xScale/m_xxScale 后整树 `refreshScaleWith` | 已存在；**无幂等判断**（每次全量递归）；两连调=两次 O(n) |
| 复合快照读 | `getScaleXX()/getScaleYY()`（ControlBase.h:440 override） | 已存在 |
| 逻辑坐标渲染 | `Bench::getDrawRect() = rect × scale`（Bench.cpp:229） | 已存在 |
| 字号复合 | `Label::refreshScaleWith` 重建文本（scaledSize） | 已存在（test_viewport_scale T11 已验深层链+非树成员收口） |
| C ABI 暴露 | UICornerstoneAPI.h 无任何 setScale 接口；binding 同无 | **缺口（本需求）** |
| 视口缩放模式重算 | `recomputeViewportTransform`（Bench.cpp:139-176）：**off 分支强制 `setScaleX(1)/setScaleY(1)`**；fit/stretch 按视口/画布比算 | **与手动缩放的交互风险（见 §4）** |
| 触发重算时机 | `resized`（窗口/子视口 resize、SetViewport、CreateViewport 时）→ recompute | 设计器 splitter 拖动画布即触发 |

## 3. 架构选择与关键设计决策

### 3.1 合并入口（决策：Bench 新增幂等 `setScale(x, y)`）

`setScaleX`/`setScaleY` 各自整树刷新，两连调=两次 O(n)。新增合并入口：

```cpp
// Bench.h / Bench.cpp
void setScale(float xScale, float yScale);   // 幂等：与 m_xScale/m_yScale 均相等时快速返回
```

- 幂等判断 `xScale == m_xScale && yScale == m_yScale` 相等则直接 return（不触发 refresh）。
- `setScaleX(v)` 改为 `setScale(v, m_yScale)`、`setScaleY(v)` 改为 `setScale(m_xScale, v)`——内部既有调用点（recompute 的 fit/stretch/off 分支、测试 T11 等）行为不变，且天然获得幂等（T11 `setScaleX(2)` 后再 `setScaleX(2)` 不再重复刷新）。
- 风险：`setScaleX(2)` 后 `setScaleY(2)`（y 已=2 时）会 return——语义正确（终态相同）。

### 3.2 C ABI + Binding（决策：透传 bench->setScale + getScaleXX/YY）

```c
UICORNERSTONE_API int UICornerstone_SetInstanceScale(UIInstance instance, float xScale, float yScale);
UICORNERSTONE_API int UICornerstone_GetInstanceScale(UIInstance instance, float* outX, float* outY);
```

- Set：`instance->bench->setScale(xScale, yScale)`；参数校验（instance 有效、bench 非空）；**x/y ≤ 0 拒绝**（返回 0）。
- Get：`*outX = bench->getScaleXX(); *outY = bench->getScaleYY()`。
- 子视口：`instance->bench` 即子视口 bench，行为与主实例一致 ✓（需求 2.2-4）。
- Binding：`bool SetInstanceScale(float x, float y); float GetInstanceScaleX() const; float GetInstanceScaleY() const;`（DynamicApi 指针 + RESOLVE）。

### 3.3 与 viewport-scale-mode 的交互（决策：off 分支尊重手动缩放，需评审确认）

**冲突**：off 模式下 `recomputeViewportTransform` 强制 `setScale(1,1)`——设计器画布（子视口默认 off）+ `SetInstanceScale(1.5)` 后，任何 resize（splitter 拖动画布）→ resized → recompute → **缩放重置为 1**，zoom 丢失。

三个方案：

| 方案 | 内容 | 评估 |
|---|---|---|
| **A（推荐）** | Bench 新增 `m_scaleOverride` 标志：`SetInstanceScale` 置 true；recompute 的 off 分支改为 `if (!m_scaleOverride) setScale(1,1)`（fit/stretch 分支仍按视口算，且切到 fit/stretch 时**清除** override 标志——引擎自动缩放接管）；新增 `ClearInstanceScaleOverride`（或 SetInstanceScale 无法清除的约定：**设计器切回 100% 时手动 SetInstanceScale(1,1) 即回到默认**，override 保持 true 但值=1 无副作用） | 改动小（off 分支一行+标志），不破坏现有 off 语义（未手动时仍 1）；需同步 ViewportScale_Design/手册 |
| B（否决） | 设计器侧绕开（画布不触发 resized） | splitter 拖动画布必然 resize，绕不开 |
| C（否决） | 设计器画布改 fit/stretch 模式 | fit/stretch 的 scale 由引擎按视口算，设计器无法自由 zoom |

**方案 A 细节**：
- `m_scaleOverride` 初值 false（现有 off 行为不变：resize 重置 1）。
- `SetInstanceScale` → `bench->setManualScale(x, y)`（置 override + setScale）。
- recompute：off 分支 `if (!m_scaleOverride) setScale(1,1)`；fit/stretch 分支 `m_scaleOverride = false` 后按视口算（模式切换时引擎接管，覆盖手动值——语义清晰）。
- 既有测试（T0 off 复合=1、T11 手动 setScaleX 后 getScaleXX=2）：T0 无 override ✓；T11 用 `bench->setScaleX`（直接成员 API，不置 override）→ recompute off 仍会重置——但 T11 后测试切回（153-154 setScaleX/Y(1)）✓ 不破坏。
- 需要新增测试：子视口 SetInstanceScale(1.5) → resize（SetViewport 模拟）→ scale 仍 1.5。

### 3.4 语义边界（写入文档）

- `GetInstanceScale` 返回**复合快照**（getScaleXX）——fit/stretch 下即引擎自动值；off 下即手动值。
- 逻辑 rect 不变：缩放只影响 drawRect（渲染/命中），`getRect()` 不变（需求验收 1 的"再次 SetInstanceScale(1,1) 后 GetRect 一致"）。
- 幂等：SetInstanceScale 相同值快速返回（合并入口幂等）。

## 4. API 设计

### 4.1 核心库

```cpp
// Bench.h
void setScale(float xScale, float yScale);              // 幂等合并入口
void setManualScale(float xScale, float yScale);        // 置 override 标志 + setScale
float getManualScaleOverride() const { return m_scaleOverride; }
// 成员：bool m_scaleOverride = false;
```

### 4.2 C ABI（UICornerstoneAPI.h + .cpp）

```c
UICORNERSTONE_API int UICornerstone_SetInstanceScale(UIInstance instance, float xScale, float yScale);
UICORNERSTONE_API int UICornerstone_GetInstanceScale(UIInstance instance, float* outX, float* outY);
```

### 4.3 Binding（UICornerstone.h/.cpp + DynamicApi.h/.cpp）

```cpp
bool  SetInstanceScale(float xScale, float yScale);
float GetInstanceScaleX() const;
float GetInstanceScaleY() const;
```

## 5. 实现要点与验收（对照需求 §3）

| 验收 | 实现 | 测试 |
|---|---|---|
| 1. 放置后缩放：GetRect×1.5、字号×1.5、逻辑 rect 不变 | setManualScale → refreshScaleWith 链（既有） | test_viewport_scale 新用例：子视口 C ABI SetInstanceScale(1.5) → Label/Button/EditBox 各抽一 drawRect×1.5、字号×1.5；SetInstanceScale(1,1) 后 getRect 恢复 |
| 2. 字号重建（重新光栅化） | Label::refreshScaleWith 既有 | 抽验 getScaleXX 后 Label 内部 font 重建（m_fontScaleDirty 路径） |
| 3. 幂等无害 | setScale 合并入口幂等 | 相同值重复调用：断言无 refresh（可经计数器或行为断言——退化为仅断言返回值 1 与 scale 不变） |
| 4. 复合控件（Button/CheckBox/ComboBox/NumericUpDown） | refreshScaleWith 各覆写已有 | 抽验四类 drawRect/字号 |
| 5. 回归：主实例不调用时行为一致 | off 默认 override=false | test_viewport_scale 既有全绿 |

## 6. 影响面与风险

- **Bench::setScaleX/Y 委托化**：调用点（recompute fit/stretch/off、测试）行为等价；幂等化后重复调用不再刷新——纯收益。
- **off 分支条件重置**：一行改动 + 标志，未手动时行为不变；需同步 ViewportScale_Design.md 与用户手册 5.6 画布缩放章节（off 语义扩展说明）。
- **fit/stretch 覆盖**：模式切换时清除 override（引擎接管），设计文档标注"fit/stretch 下 SetInstanceScale 会被下一次 recompute 覆盖，仅 off 模式适用手动缩放"——设计器画布应使用 off + 手动。
- **recompute 触发时机（复核 §3.2 确认）**：仅 `resized` / `SetViewport` / `CreateViewport` / `setViewportScaleMode` 触发；**放置/删除控件、SetRect 等内容变更不触发 recompute**（源码层面已核）。在 ViewportScale_Design.md 同步说明中加约束：新增触发点须重新评估 override 语义。
- **覆盖层二次缩放（复核 §3.1 应用侧注意）**：视口内除被设计控件外，设计器自绘覆盖层同为 bench 子控件，`refreshScaleWith` 会同样刷新：

| 覆盖层 | 现状 | bench scale 影响 | 设计器对策（迁移 scale 语义时） |
|---|---|---|---|
| 网格瓦片 Image | rect=视口物理 + cellPx×zoom 重建瓦片 | drawRect×scale → 网格密度不再随 zoom（tile 按纹理原尺寸平铺，缩放只扩大范围） | 瓦片纹理仍按 cellPx×zoom 重建（保持密度）+ **rect 除以 scale 抵消**（rect=(W/s,H/s)→drawRect=W） |
| 选中框 Shape（8 向控点） | 图元坐标按逻辑×zoom 手算 | ×scale 双重缩放（框/控点错位） | 图元坐标改**逻辑坐标直绘**（不再×zoom）；线宽/控点尺寸常量除 scale 或接受视觉缩放 |
- **三后端无涉**：纯逻辑层改动，不触碰渲染。

## 7. 待审核问题

1. **方案 A（off 分支尊重手动缩放 override）是否接受**？若不接受（严格保持 off=无缩放），则设计器画布需引擎提供"手动缩放模式"新概念（改动更大），请裁决。
2. `SetInstanceScale(1,1)` 语义：置 override 且值=1（后续 resize 仍保持 1，等同默认）——无需单独"清除"接口，确认？
3. GetInstanceScale 返回复合快照（fit/stretch 下为引擎自动值）而非"手动值"——确认？
4. Binding 用 `GetInstanceScaleX/Y` 两个 getter（而非 std::pair）——确认？
