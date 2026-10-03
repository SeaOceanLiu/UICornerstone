# P052_OverlayAboveViewport_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-02
- 对象：`design/P052_OverlayAboveViewport_Design.md`（P0-52①/②，状态：待评审）
- 结论：**通过，放行实施**（主方案/类型集合/路由策略确认；附 3 项实施注意——路由验收探针、键盘边缘说明、文档措辞，无需回炉设计）

## 1. 评审要点

### 现状核实（§1）✓（源码抽验一致）

- **浮层类型集合抽验属实**：Popup（Dialog.cpp:14）/ConfirmPopup（:268）/Dialog（:362）/ContextMenu（ContextMenu.cpp:13，归 Popup）/MenuPanel（Menu.cpp:319）；MenuBar（Menu.cpp:956）不在集合——正确（MenuBar 是主 UI，非浮层）。
- **挂载点抽验**：ComboBox 下拉=内部 Popup 挂 BENCH（ComboBox.cpp:51-72，open :375）；ColorPicker 弹窗=Dialog 且 `open()` 时挂 BENCH（ColorPicker.cpp:106/:312）——均在集合内。
- **事件侧根因属实**：`pumpInstanceEvents` 鼠标分支先 `findViewportByCoord`（UICornerstoneAPI.cpp:737）；且 `triggerEvent` → `m_eventQueueInstance->pushEventIntoQueue`（ControlBase.cpp:792）——事件发子视口即进**子实例队列**，owner 浮层 watcher（Dialog.cpp:112-118 挂 owner 队列）不可见。设计描述准确。
- **调换顺序不可行再确认**：child bench 同样不透明（Bench::initial `setTransparent(false)` + `DEFAULT_NORMAL_COLOR`，Bench.cpp:10）——先画布后主实例必被盖掉。

### 方案（§2）✓

- **2.1 RenderOverlays**：仅遍历 bench 顶层、类型命中且可见者 `draw()`；不填背景、裁剪同 Render——正确的最小通道。`getChildren()` 公开可用（ControlBase.h:449）。
- **2.2 路由**：`hasVisibleOverlay` 前置、owner 独占（不穿透）——与"点击 owner 区域→owner"既有口径一致；外点关闭/下拉滚动/菜单关闭全部回归 owner 队列 watcher，方案自洽。
- **备选评估合理**：bench 内联子视口渲染重构面大（多实例渲染管线 + 变换/裁剪语义），不采纳 ✓。

### 双绘分析（Q-1）——接受 (a)，且影响比设计描述更小

- **画布区域内无二次加深**：pass1 浮层像素被 `vp.Render` 的 child bench 不透明背景整片重绘擦除，pass2 单次混合；
- 仅浮层压在**主 UI 区域**的部分（弹层顶部阴影带/工具栏上方等）pass1+pass3 二次混合 → 轻微加深；
- (b) 否决理由成立：`Render` 跳过浮层会使未接入 `RenderOverlays` 的既有宿主（samples/测试）浮层整体消失，破坏兼容。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 双绘阴影：接受轻微加深 vs 新增跳过开关 | **接受 (a)**；(b) 兼容风险不可取，(c) 过度复杂 |
| 2 | 穿透策略：owner 独占 vs 按 close-on-click-outside 细分 | **确认 owner 独占（不穿透）**；浮层期间画布 hover 冻结属预期（下拉项 hover 正常），细分留待真实需求 |
| 3 | 类型集合是否纳入 alwaysOnTop 等 | **确认现集合即可**：alwaysOnTop 是树内 z 序标记、非浮层挂载点——纳入会把主 UI 控件错误二次绘到画布上。StatusBar 内嵌 popupPanel（挂 StatusBar 自身，StatusBar.cpp:208-211）不在本通道覆盖内——设计器未用（statusBar 无菜单项），列为已知限制即可 |
| 4 | 子视口浮层无需宿主额外调用 | **确认**；z 序为 vp 浮层 < owner 浮层（owner.RenderOverlays 后绘）。§2.1 "位于 owner.RenderOverlays 之前，仍在最顶"措辞易误读，建议改为"在自身区域内最顶，整体位于 owner 浮层之下" |
| 5 | API 形态 `UICornerstone_RenderOverlays` + Binding `RenderOverlays()` | **确认**；建议 API 注释写明帧序（`Clear → owner.Render → vp.Render → RenderOverlays → Present`）与"须在 Present 前调用" |

## 3. 实施注意（放行附项）

1. **路由验收探针（重要）**：注入通路（`PushUIEvent` → ProcessEvents）鼠标事件**不经坐标路由**（UICornerstoneAPI.cpp:845-847 直接 `dispatchToBench(instance)`）——验收 #3 用注入无法覆盖 P0-52②（注入 owner 恒进 owner bench；注入 vp 不涉浮层）。建议：抽 `resolveMouseTarget(owner,x,y)` 并加 Debug 探针（如 `Debug_RouteMouseTarget(instance,x,y)`，返回 1=owner/0=vp），以"开浮层→断言 owner；关浮层→断言 vp"的注入用例闭环；否则 #3 标注为"浮层交互覆盖"，路由改人工（真实鼠标）验证。
2. **键盘边缘（低危）**：`activeViewport` 非空时 KeyDown/TextInput 发子视口（:766-770），owner 浮层 watcher/焦点域不生效——§2.2 "Esc 关闭由 Popup watcher 已有"仅在 `activeViewport==nullptr` 成立。设计器实际流：点工具栏（owner 区）已清 activeViewport（:740-744）→ 键盘正常；仅程序化打开浮层时存在边缘。建议：浮层可见分支在 MouseDown/Up 时按同口径清子视口焦点/activeViewport（一行），或在文档注明该限制。
3. **验收补充**：① #1 加"浮层压主 UI 区域无视觉回归"（双绘阴影）；② #3 加"浮层可见时 MouseMove 归 owner（下拉项 hover）"；③ #4 加"关闭后首个 Move 恢复子视口 hover"。

## 4. 设计器随批（引擎同步后）

1. `App.cpp` 帧序：`Clear → owner.Render → vp.Render → owner.RenderOverlays → Present`（Binding `RenderOverlays()`）；
2. 联测：cb_gridcell/tb_align 下拉完整显示于画布之上、下拉项可点选、外点关闭、画布 hover/拖动恢复；菜单栏下拉同验。

## 5. 放行

**结论：放行实施**。主方案（最小渲染通道 + owner 独占路由）正确，类型集合与兼容策略经源码抽验无误；附 3 项实施注意（路由验收探针/键盘边缘/文档措辞）。实施完成后同步 subModules，设计器随批端到端联测。
