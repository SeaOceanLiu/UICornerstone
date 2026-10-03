# P050_AddChildRemoveChild_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-01
- 对象：`design/P050_AddChildRemoveChild_Design.md`（E-4/P0-50，状态：待评审）
- 结论：**通过，放行实施**（§4 七问全部确认；P1-a 随本能力解锁）

## 1. 评审要点

### 现状核实（§1）✓

- **C1 双重挂载缺陷定位精准**：AddChildControl 仅摘 bench——跨容器 reparent 时旧容器 children 仍持有（双重绘制+双重事件）——设计器拖动主路径的真实缺陷。
- **引擎细节扎实**：removeControl 不重置父指针（reparent 由新父 addControl→setParent 修正；纯摘除需显式 setParent(nullptr)）——方案的正确性基础。
- **保活先例**：menuPool/handleControls 的"detach 不销毁、随实例销毁级联"模式——detachedControls 池有成熟先例可循 ✓。

### 修改方案（§3）✓

- **AddChildControl 签名不变语义修复**（ABI 兼容）：环守卫（祖先环遍历）+ 摘任意旧父（修 C1）+ 挂新父——跨容器拖动零额外调用 ✓。
- **RemoveChild 摘除不销毁**：校验当前父==parent（防误摘）+ setParent(nullptr)（修正父指针/复合缩放——细节到位）+ detached 保活池（可反复 attach/detach）✓。
- **保活池三向联动**：AddChildControl 先 detachPoolTake / DestroyControl 先 detachPoolTake / RemoveChild 移入池——**生命周期闭环无泄漏** ✓。
- **WinFrame 不自动路由**：引擎零类型知识，ClientPanel 由设计器按 07 定稿解析 ✓ 职责边界正确。
- **Binding 形态**：保留 Control::AddChild（兼容+自动获修复语义）+ 实例形态 AddChild(parent,child) 返回 child（链式/判别）+ RemoveChild 返回码 + DynamicApi 补齐 ✓。

### 设计器侧影响（P1-a 解锁）✓

- **放置进容器**：命中容器 → `AddChild(container, child)`（WinFrame 经 GetPtr("client-panel")）+ 相对坐标换算 + parentId 记账；
- **容器间拖动**：直接 AddChild 新容器（自动摘旧，零额外调用）；
- **删除语义**：销毁走 DestroyControl；移出暂存走 RemoveChild（保活）——两路径齐备。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | RemoveChild 摘除不销毁（保活池；销毁走 DestroyControl） | **确认** |
| 2 | AddChild 自动摘任意旧父（修 C1） | **确认** |
| 3 | void AddChildControl 静默 no-op（ABI 兼容）+ RemoveChild 返回码；**不新增带返回码 AddChild 变体** | **确认**——Binding 形态已返回 child 可判别，C ABI 旧形态保持兼容即可 |
| 4 | WinFrame 不自动路由（设计器经 GetPtr("client-panel")） | **确认** |
| 5 | Control::AddChild 返回值 void→Control | **确认改**（源码兼容：调用方忽略返回值合法；链式/判别受益） |

## 3. 设计器随批（引擎实施同步后）

1. **placeControl 容器目标**：画布命中容器（内容区）→ AddChild(client-panel/容器) + 相对坐标换算 + parentId 记账（07 §4.2/4.3）；
2. **容器间拖动 reparent**：AddChild 新容器（自动摘旧）；
3. **删除语义接线**：Delete 键 → DestroyControl；"移出"操作（若做）→ RemoveChild；
4. **联测**：验收 6 项 + WinFrame 内容区放置端到端。

## 4. 放行

**结论：放行实施**。现状核实（ABI 兼容约束/父指针细节/保活先例）与方案（守卫+池联动生命周期闭环）质量高，P1-a 随本能力解锁。实施完成后同步 subModules，设计器随批容器内放置端到端联测。
