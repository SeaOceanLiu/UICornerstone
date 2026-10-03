# P050_AddChildRemoveChild_Design — 容器类子控件挂载（E-4 / P0-50：Binding AddChild/RemoveChild）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-01 补充：E-4 / P0-50）
> 设计器前置：`CornerstoneDesigner/design/07_容器类子控件编辑设计.md`（已定稿：放置双通道 / WinFrame 严格 ClientPanel / 分期 P1-a→P2；P1-a 依赖本能力）
> 状态：**已放行，已实施**（2026-10-01）
> 复核：`requirements/P050_AddChildRemoveChild_Design_复核意见.md`（通过，放行实施；§4 五问全部确认）
> 实施备注：① `AddChildControl` 语义修复（任意旧父自动摘除 + 环/自身/非 Panel 守卫，签名不变）；② 新增 `UICornerstone_RemoveChild`（摘除不销毁 + `setParent(nullptr)` + `detachedControls` 保活池，AddChild/Destroy 三向联动、`destroy()` 清池）；③ Binding：`Control::AddChild` 返回 `Control` + `Control::RemoveChild` + 实例形态 `AddChild(parent,child)`/`RemoveChild(parent,child)` + DynamicApi；④ **联测发现并修复**：Debug `validateControl` 的实例持有检查未纳入新池 → RemoveChild 后句柄触发 assert（VC 运行库异常）；已把 `detachedControls`/`handleControls` 池纳入 `instanceHoldsControl`（含 treeContains 后代）；⑤ 测试：`test_property_cabi` 容器段 8 断言（130 passed）+ Binding 探针全过；⑥ 回归仅 4 项预存失败

---

## 1. 现状核实（源码事实）

| 通道 | 现状 | 位置 |
|---|---|---|
| C ABI 挂载 | `UICornerstone_AddChildControl(inst, parent, child)` **已存在**（void）：仅 `bench->removeControl(sp)` 后 `panel->addControl(sp)`；父须为 Panel（否则静默 no-op） | UICornerstoneAPI.cpp:1865-1876 |
| C ABI 销毁 | `UICornerstone_DestroyControl(inst, ctl)` **已存在**：从当前父（或 bench）摘除 → 局部 shared_ptr 归零即销毁 | UICornerstoneAPI.cpp:1892-1907 |
| C ABI 摘除 | **缺失**（无"摘除不销毁"入口） | — |
| Binding | `Control::AddChild(Control child)` void（目标=this，`fnAddChildControl`）；`Control::Destroy()`（销毁）；**无 RemoveChild**、无 parent/child 形态的 AddChild | Control.h:56 / Control.cpp:134-142 |
| 引擎 | `ControlImpl::addControl`：重复跳过 + 上下文继承 + `setParent` + 渲染设备传播；`removeControl`：仅从 children 摘除，**不重置父指针**（reparent 由新父 `addControl→setParent` 修正；纯摘除需显式 `setParent(nullptr)`） | ControlBase.cpp:379-424 |
| 保活先例 | `UIContext::menuPool` / `handleControls`（注释明确"detach 移出容器不销毁，设计器可反复 attach/detach；随实例销毁级联释放"）；`destroy()` 显式清池 | UIContext.h:78/91-93、UIContext.cpp:105-111 |
| WinFrame ClientPanel | `GetPtr("client-panel")` 可用（严格放置由设计器 07 定稿执行） | WinFrame.cpp:591 |

## 2. 问题确认

| # | 问题 | 说明 |
|---|---|---|
| C1 | **跨容器 reparent 双重挂载** | `AddChildControl` 只摘 `bench`，若 child 原属**另一容器**（Panel→Panel、设计器拖动主路径），旧父 children 仍持有 → **双重绘制 + 双重事件**（真实缺陷） |
| C2 | **摘除不销毁缺失** | 无 RemoveChild：显式移出容器（暂存/画布根回收）只能借 Destroy（销毁）或依赖下次 AddChild 自动摘除——"移出但保留"路径不可达 |
| C3 | **无守卫/无返回码** | self 挂载、祖先环（把祖先挂进后代）、非 Panel 父均静默失败；void 无法判别成功，设计器只能盲操作 |
| C4 | **Binding 形态缺口** | 设计器签名需 `Control AddChild(Control& parent, Control& child)` / `bool RemoveChild(Control& parent, Control& child)`；现有仅 `Control::AddChild`（void，目标=this），无 RemoveChild |

## 3. 修改方案

### 3.1 C ABI（语义修复 + 入口补齐；ABI 兼容）
- **`UICornerstone_AddChildControl`（签名不变）语义修复**：
  ```cpp
  // 守卫：空/非 Panel 父/self/child 是 parent 的祖先（环）→ 静默 no-op（void 兼容）
  // 摘旧父：任意旧父（非仅 bench）→ oldParent->removeControl(sp)
  // 挂新父：panel->addControl(sp)（内部 setParent/上下文/设备传播）
  for (Control* p = parentV; p; p = p->getParent()) if (p == childV) return;   // 环守卫
  ```
  效果：跨容器拖动 = 直接 `AddChildControl(newParent, child)`，自动从旧容器摘除，无双挂载。
- **新增 `int UICornerstone_RemoveChild(UIInstance, parent, child)`**（摘除不销毁）：
  - 校验 child 的当前父 == parent（避免误摘）；`parent->removeControl(sp)` + `sp->setParent(nullptr)`（修正父指针/复合缩放）；
  - 移入实例 **detached 保活池**（`UIContext::detachedControls`，随实例销毁级联释放；`destroy()` 显式清池）；
  - 返回 1/0（成功/非法）。
- **保活池联动**：`AddChildControl` 先 `detachPoolTake(sp)`（若在池中则移出后挂树，不销毁）；`DestroyControl` 先 `detachPoolTake(sp)`（从池中移除 → 再摘父 → 归零销毁）。
- **WinFrame**：引擎保持通用（不自动路由 ClientPanel）；设计器按 07 定稿经 `GetPtr("client-panel")` 挂载（维持引擎零类型知识）。

### 3.2 Binding（`binding/include/UICornerstone.h` / `Control.h` + 实现）
- 保留 `Control::AddChild(Control child)`（兼容；实现改经同一 ABI，自动获得修复语义）；
- 新增（与设计器签名一致，挂 `UICornerstone` 实例类）：
  ```cpp
  Control AddChild(Control& parent, Control& child);            // 返回 child（可链式；失败返回无效句柄）
  bool    RemoveChild(Control& parent, Control& child);         // 摘除不销毁（保活，可再挂）
  ```
- `Control` 类补对称便捷：`bool Control::RemoveChild(Control child)`（目标=this；可选，随批）；
- DynamicApi 增补 `fnRemoveChild` 解析（AddChildControl 已有）。

### 3.3 测试与验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | 跨容器 reparent（AddChild 直挂新容器）：旧容器不再绘制子控件、新容器绘制；无双事件 | C ABI 测试（CaptureControl 像素 + 事件回调计数） |
| 2 | RemoveChild：从容器摘除后不绘制、句柄有效（保活）、可再 AddChild 回挂 | C ABI 测试 |
| 3 | 守卫：self / 祖先环 / 非 Panel 父 → 不改变树（RemoveChild 返回 0） | C ABI 测试 |
| 4 | DestroyControl：从容器（或保活池）移除并销毁；后续句柄调用安全 | C ABI 测试 |
| 5 | Binding：`AddChild(parent, child)` 返回句柄可用；`RemoveChild` 返回 true/false | Binding 探针 |
| 6 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

### 3.4 文档
- CABI 速查表：`AddChildControl`（语义修订注记） + `RemoveChild` 行；
- Binding 速查表：`AddChild(parent, child)` / `RemoveChild(parent, child)` 行；
- 设计器 07 依赖项：P1-a 放置（AddChild 到容器/ClientPanel）即可开工；拖动即"AddChild 新容器"（自动摘旧容器）。

## 4. 待审核问题

1. **RemoveChild 语义**：摘除**不销毁**（保活池，可反复 attach/detach；销毁仍走 `DestroyControl`）——确认？
2. **AddChild 自动摘旧父**：跨容器直接 `AddChild(newParent, child)` 自动从任意旧父摘除（修复 C1，拖动零额外调用）——确认？
3. **非法关系处置**：void 的 `AddChildControl` 保持静默 no-op（ABI 兼容）；`RemoveChild` 用返回码——确认？或要求新增带返回码的 AddChild 变体？
4. **WinFrame 不自动路由**：引擎挂"给定父"；ClientPanel 由设计器经 `GetPtr("client-panel")`（07 定稿）——确认？
5. **Binding `Control::AddChild` 返回值**：是否改为返回 `Control`（源码兼容、便于链式），或保持 void？建议改返回 `Control`。

## 5. 配套改动

- 代码：`src/UICornerstoneAPI.cpp`（AddChildControl 修复 + RemoveChild + 池联动）、`include/UICornerstoneAPI.h`、`include/UIContext.h` + `src/UIContext.cpp`（detachedControls 池）、`binding/*`（AddChild/RemoveChild 封装 + DynamicApi）；
- 测试：C ABI 用例（跨容器/摘除保活/守卫/销毁）+ Binding 探针；
- 文档：CABI/Binding 速查表；设计文档标注 + 复核归档 + make_release + 同步。
