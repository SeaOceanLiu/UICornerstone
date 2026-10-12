# ScrollBarDragCapture_Design — ScrollBar 拖拽释放丢失修复（跨子视口鼠标捕获 + 同实例拖拽 watcher）

> 提出方：用户（实测缺陷）
> 现象：在滑块上按下左键拖动可滚动，**松开左键后 ScrollBar 未检测到释放**，鼠标移动仍持续拖动滑块，直到在滑块上再次"按下+松开"才复位。
> 前置：P0-52（子视口事件路由/浮层优先）、P0-60（Panel 布局）等批次已实施
> 状态：**已放行，已实施**（2026-10-05；四问全部确认，Q-3 采纳注入路由对齐）
> 复核：`requirements/ScrollBarDragCapture_Design_复核意见.md`（通过；设计器侧确认子视口画布场景成立）
> 实施备注：① **跨子视口捕获**（主修）：`UIContext.mouseButtonDown` + 统一路由 `dispatchMouseByRouting`（捕获 > 浮层优先 > 坐标路由；Down 置位/Up 复位；按下期间 Move/Up 锁定 activeViewport）；② **ScrollBar watcher 兜底**：懒注册 Move/Up `beforeEventHandlingWatcher`（同实例拦截亦复位；释放消费防穿透；`dragTo/endDragState` 抽取，handleEvent 同源）；③ **注入路由对齐（Q-3）**：queued 鼠标事件与轮询共用统一路由（Down/Up 坐标改用 mouseButton 字段）——跨视口场景可自动化探针；④ 验证：VP 探针（视口内拖动/视口外释放 → NO BUG）、独立探针（释放/复位回归 OK）、静态探针（CORE LOGIC OK + **WATCHER OK**：watcher 拖动消费/释放消费/复位）；⑤ 全量回归仅 4 项预存失败（注入路由对齐与捕获无副作用）；⑥ 2026-10-05 **设计器联测通过**（设计态/预览态拖出视口释放 + 画布滚动条回归）

---

## 1. 问题确认（源码路径分析 + 复现尝试）

| 路径 | 结论 | 证据 |
|---|---|---|
| **A. 跨子视口释放丢失（主因，症状完全吻合）** | **确认（源码路径成立）** | `pumpInstanceEvents`（UICornerstoneAPI.cpp:737-795）鼠标事件**按坐标路由**：`resolveMouseTarget` → `findViewportByCoord`，仅当光标落在子视口 rect 内才发 vp。拖动中光标一旦移出视口 rect（拖到边缘/快速移动），**MouseUp 发给 owner**，vp 内 ScrollBar 收不到 → `m_dragging` 卡住；光标移回视口后 MouseMove 继续拖（m_dragging=true）→ 完全吻合"松开后移动仍作用在滑块上，直到再次点击"；设计器画布即子视口结构。另：命中矩形随窗口/视口重算是 vp 本地，跨视口无捕获语义 |
| **B. 同实例无拖拽捕获兜底** | **确认** | ScrollBar 仅依赖 `handleEvent`（ScrollBar.cpp:176-258）收 MouseUp；若同实例内有控件/叠加层先行消费 MouseUp（返回 true）或路由未达，`m_dragging` 同样卡住。Splitter 已有 `beforeEventHandlingWatcher` 模式（Splitter.cpp:333-347）可循，ScrollBar 未用 |
| 复现说明 | 注入通路（PushUIEvent）**不经坐标路由**（P0-52 已知：queued 事件直接 `dispatchToBench(instance)`），无法用现有探针自动复现跨视口丢失；独立/嵌入式（同实例）探针实测**释放正常**（含控件外释放——bench 子级无条件投递 Reset），故主因指向跨视口与同实例拦截两类路径 | 探针：`Temp/temp_scrollbug_probe.cpp`（同实例 NO BUG）、`Temp/temp_scrollbug_vp.cpp`（注入不进坐标路由，无法覆盖） |

## 2. 修改方案

### 2.1 跨子视口鼠标捕获（主修，路由层）
- `UIContext` 增 `bool mouseButtonDown = false;`（按下状态跟踪）；
- `pumpInstanceEvents` 鼠标分支调整：
  ```cpp
  // MouseDown：按坐标路由（既有）；命中子视口 → activeViewport=target（既有）并置 mouseButtonDown=true
  // MouseMove / MouseUp：若 (mouseButtonDown && instance->activeViewport) → 强制路由到 activeViewport（捕获），
  //                      不再按坐标；MouseUp 后清 mouseButtonDown（activeViewport 保留为焦点域语义）
  ```
- 效果：**在子视口内按下后，拖动/释放始终由该视口接收**（标准鼠标捕获语义），光标离开视口 rect 亦不丢失释放；无按钮按下时路由不变（浮层优先/坐标路由与 P0-52 一致）；
- 顺带受益：子视口内的 Slider/Splitter 等拖拽控件同类问题一并消除（Splitter 的 watcher 首次能跨出视口收到 Move/Up）。

### 2.2 ScrollBar 同实例拖拽 watcher（兜底 + 防穿透）
- 抽私有方法：`void dragTo(float x, float y);`（现 MouseMove 拖拽体）与 `void endDragState();`（复位 m_dragging/m_thumbPressed/applyPressState(false)）；
- MouseDown 命中滑块时懒注册（同 Splitter 模式；不注销，自守卫防递归锁 UB）：
  ```cpp
  m_context->eventQueue->addBeforeEventHandlingWatcher(EventType::MouseMove, getThis());
  m_context->eventQueue->addBeforeEventHandlingWatcher(EventType::MouseUp, getThis());
  ```
- `beforeEventHandlingWatcher`：`m_dragging` 时 MouseMove → `dragTo(...)`；MouseUp(Left) → `endDragState()` 并 **return true**（消费释放，防止拖拽释放穿透为下层控件点击）；其余返回 false；
- `handleEvent` 的 MouseUp/MouseMove 分支改调同一 `endDragState()/dragTo()`（行为不变，单点维护）。

### 2.3 可选：注入通路路由对齐（Q-3）
- 将 queued 鼠标事件与真实事件统一走 `resolveMouseTarget + 捕获`（而非当前"直接 owner bench"）——可使跨视口场景可自动化探针验收；风险：需回归确认既有注入用例（test_combobox_cabi 单实例无 vp 不受影响；multiviewport 相关用例需核）。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | 子视口内 ScrollBar：按下拖动 → 光标移出视口 → 视口外松开 → **释放生效**（移回不再拖） | 真实鼠标联测（设计器画布）；（若 2.3 对齐）注入探针自动化 |
| 2 | 同实例：拖拽过程中另一控件/叠加层消费 MouseUp 时，ScrollBar 仍复位（watcher 兜底）且释放不穿透为点击 | 引擎探针（watcher 路径） |
| 3 | 无按钮按下时路由不变（浮层优先 / 坐标路由 / 焦点作用域语义） | 现有 multiviewport/overlay 用例回归 |
| 4 | ScrollBar 既有拖拽/点击轨道/滚轮行为不变 | test 回归 + 探针 |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **捕获语义**：子视口内按下（MouseDown）后，Move/Up 一律发该视口（直到 Up），Up 后恢复坐标路由、activeViewport 保留为焦点域——确认？
2. **ScrollBar watcher 消费 MouseUp**：拖拽结束的释放返回 true（不穿透为下层点击）——确认？（Splitter 同款语义）
3. **注入通路是否对齐坐标路由**（2.3，换取跨视口自动化验收）？或保持现状、跨视口项以真实鼠标联测验收？
4. **范围**：本期仅修 ScrollBar（2.2）+ 路由捕获（2.1）；Slider 等其它拖拽控件依赖 2.1 一并受益、不再单独加固——确认？

## 5. 配套改动

- 代码：`src/UICornerstoneAPI.cpp`（mouseButtonDown 捕获路由）、`include/UIContext.h`（flag）、`ScrollBar.h/.cpp`（dragTo/endDragState + watcher）；
- 测试：探针（同实例 watcher；若 2.3 则跨视口注入用例）+ 全量回归；
- 文档：无需用户文档变更（内部行为修复）；设计文档标注 + 复核归档 + make_release + 同步。
