# UICornerstone 配合修改清单（第二批：HandleControl 集成批）

- 提出方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-18
- 背景：设计器已将选中框/手柄/resize/光标整体迁移到引擎 `HandleControl`（自绘方案 ~200 行已删除）。集成中发现 C ABI 缺口与交互扩展需求如下。上一批（汇总版）中 P0-1~4 均已闭环，不再重复；个别项有状态更新（见文末）。

## P0 —— HandleControl 集成缺口（阻塞精确集成）

### 1. 切换 target / 分离的 C ABI

- **现状**：仅 `CreateHandleControl(instance, target, x, y, w, h)`——target 构造时绑定。设计器每次改选控件被迫 Destroy + 重建（选中新控件/取消选中/切换选中均触发销毁创建）。
- **需求**：
  - `UICornerstone_SetHandleTarget(instance, handle, target)`——切换附加目标（target 传 NULL = detach：自移出父容器+恢复光标，即 C++ `detach()` 语义）
  - 对应 binding：`Control::SetHandleTarget(Control target)`（空 Control = detach）
- **收益**：选中切换零创建开销；detach 语义（恢复光标/移出容器）由引擎保证。

### 2. 手柄命中查询 C ABI

- **现状**：设计器在 tickInput 按下沿需要区分"点击在手柄上（让给 HandleControl）"与"点击在控件本体/空白（设计器处理）"。目前只能用"选中控件 rect 外扩 10px"近似——边缘 10px 内点击无法启动本体拖动（取舍），且与引擎 `hitTestHandle`（8×8 精确判定）不一致。
- **需求**：`UICornerstone_HandleHitTest(instance, handle, x, y) -> int`（命中返回非 0，可带出 HandleType 或仅 bool）+ binding 包装。
- **用法**：设计器按下沿先查 `HandleHitTest`——命中则不处理（HandleControl 接管）；未命中且在控件本体 → 本体拖动；否则空白取消选中。像素级一致，去除近似。

### 3. 拖拽几何回调（rect filter）——吸附/对齐线的插入点

- **现状**：HandleControl 拖拽 resize/move **直改 target rect**，应用无法参与几何决策。设计器的网格吸附目前靠"轮询差异 → snap → 写回"绕行：resize 拖拽中不能写回（会漂移引擎锚定），只能**松手后收敛**（拖拽过程中无吸附反馈）；move 路径（设计器本体拖动）有吸附但与 HandleControl Move 手柄路径不一致。
- **需求**：rect 过滤回调——HandleControl 在 `m_target->setRect(...)` 前回调应用：
  ```c
  typedef int (*UIHandleRectFilter)(UIControlHandle target, float* ioX, float* ioY,
                                    float* ioW, float* ioH, void* userData);
  UICornerstone_SetHandleRectFilter(instance, handle, filter, userData);
  ```
  语义：拖拽/移动每帧计算出新 rect 后、setRect 前，调用 filter（应用可 snap/对齐修正，返回 1 用修正值、0 用原值）。
- **收益**：吸附在**拖拽过程中实时生效**（当前松手才收敛）；未来对齐线/智能参考线同点扩展；设计器 move 路径可统一交给 HandleControl（吸附由 filter 提供），删除本体拖动路径。

### 4. HandleControl 配置 C ABI（可选，按引擎判断优先级）

- `SetHandleMoveVisible(ctl, bool)`——设计器当前策略倾向关闭 Move 手柄（移动统一走本体拖动+filter 吸附）；若 3 落地则保留 Move 手柄并经 filter 吸附，此项可降级。
- `SetHandleSize/SetHandleColors/SetMinSize`——视觉与约束定制（缺省可用，非阻塞）。

## P1 —— 存量推进

| # | 项 | 说明 |
|---|---|---|
| 5 | **SetControlId 的 binding 包装** | C ABI 已有（上一批实施），binding `UICornerstone` 类与 DynamicApi 未 RESOLVE/包装——设计器暂未接入，包装后回收自存句柄映射 |
| 6 | **schema 裁剪布局语义键** | `flow-weight` 出现在 schema（label 等可编辑键清单）——它是 flow 容器的**布局属性**，对控件实例编辑无意义，设计器已 skip；建议 schema 生成时排除布局类键（flow-weight/anchor 等） |
| 7 | **字体懒加载预热**（低优推进中） | 控件类型首建 ~390ms，交互可感知 |

## 状态更新（对照上一批汇总）

| 项 | 更新 |
|---|---|
| 光标 override 栈（P1-6） | **撤销**：HandleControl 接管后应用层不再设置光标，引擎原生方向光标工作正常 |
| Event.h:28 注释 | 已修正（已确认） |
| font-size 统一 + caption-size 废弃 | 已完成并验证（Int 域归一） |

## 验收（P0 各项）

1. **需求 1**：SetHandleTarget 切换三次 target → 手柄跟随新 target；NULL → HandleControl 移出容器且光标恢复默认。
2. **需求 2**：HandleHitTest 与引擎内部 hitTestHandle 结果一致（8×8 手柄边界像素级）；非手柄处返回 0。
3. **需求 3**：filter 内 snap 修改 → 拖拽**过程中**视觉实时吸附（非松手收敛）；filter 返回 0 时行为与现状一致；move 与 resize 均经过 filter。
