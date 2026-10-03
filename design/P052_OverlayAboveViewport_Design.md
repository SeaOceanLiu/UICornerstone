# P052_OverlayAboveViewport_Design — 浮层恒在子视口之上（渲染通道 + 事件路由）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-02 追加：P0-52①/②）
> 背景：画布工具栏 ComboBox 打开下拉后被中央画布（子视口）整片盖住；点击下拉项/外点关闭均失效
> 状态：**已放行，已实施**（2026-10-02）
> 复核：`requirements/P052_OverlayAboveViewport_Design_复核意见.md`（通过，放行实施；主方案/类型集合/路由策略确认；附 3 项实施注意已落实）
> 实施备注：① `UICornerstone_RenderOverlays` + Binding `RenderOverlays()`（仅 bench 顶层可见浮层，含子树/z-order/viewport 裁剪）；② 鼠标四类路由前置 `resolveMouseTarget`（浮层优先 → owner 独占；Down/Up 回收子视口焦点，Esc 等键盘通路修正）；③ 新增 `UICornerstone_Debug_RouteMouseTarget` 探针（复核附项 1：注入通路不经坐标路由，路由改探针验收）；④ **联测发现并修复预存 UAF**：C ABI 创建的 ContextMenu 唯一持有者为 bench，`Popup::close` 摘树后对象销毁、close 尾部（m_onClose/回调）访问悬空——修复：`CreateContextMenu` 纳入 `popupPool` 保活 + `Popup::close` 防御性 `selfKeepAlive`；⑤ 新聚焦测试 `test_overlay_viewport_cabi` 12/12 PASS（路由开关/像素级覆盖/无浮层零副作用）；⑥ 回归仅 4 项预存失败；⑦ 文档：CABI/Binding 速查表 + multiwindow 帧序/路由要点

---

## 1. 现状核实（源码事实）

| 项 | 现状 | 位置 |
|---|---|---|
| 主实例渲染 | `UICornerstone_Render`：`pushClipRect(viewport)` → viewport 背景 → `bench->draw()`（含全部子控件与浮层）→ pop | UICornerstoneAPI.cpp:861-873 |
| 子视口渲染 | 各子视口实例独立 `vp->Render()`，在同一帧缓冲上后绘于其 viewport 区域 → **覆盖先绘的浮层**（根因） | 同上 |
| 调换顺序不可行 | 主 bench 背景不透明（`DEFAULT_NORMAL_COLOR=(23,23,24,255)`）+ 根面板不透明（#202020）→ 先画布后主实例会把画布整片盖掉（设计器实测结论一致） | 设计器 §背景 |
| 浮层挂载 | Popup/ConfirmPopup/Dialog：`show()` 时 `BENCH->addControl(self)`；MenuPanel：MenuBar 打开时 `bench->addControl` 追加末尾、关闭时摘除；ContextMenu（类型=Popup）；ComboBox 下拉=内部 Popup（含 ComboBoxListPanel/ScrollBar 子树）；ColorPicker 弹窗=Dialog → **浮层 = bench 顶层可见、类型 ∈ {Popup, ConfirmPopup, Dialog, MenuPanel}** | Dialog.cpp:143 / Menu.cpp:1148,1177 |
| Z-order | `stabilizeTopmostChildren`（alwaysOnTop 置尾）；浮层按 bench 子序绘制 | ControlBase.cpp:400-410 |
| 事件路由 | `pumpInstanceEvents` 鼠标四类（Move/Down/Up/Wheel）**先** `findViewportByCoord`（落在子视口 rect 内 → 无条件发子视口）；仅在 owner 空白区才回 owner bench → **浮层压画布时点不中/外点不关**（根因） | UICornerstoneAPI.cpp:277-286 / 737-760 |
| 半透明元素 | `MenuPanel::drawShadow()`：圆角阴影（PANEL_SHADOW alpha + blur）为半透明；主体背景不透明 | Menu.cpp:796-816 |

## 2. 修改方案（采纳设计器主方案；引擎细节与边界如下）

### 2.1 P0-52① 浮层渲染通道
- 新增 C ABI `void UICornerstone_RenderOverlays(UIInstance)`（Binding `void RenderOverlays()`）：
  ```cpp
  // 实现要点：仅重绘 bench 顶层可见浮层（含子树），按 bench 子序（z-order），
  // 裁剪同 Render（instance->viewport）；不填充背景；无浮层时零开销。
  instance->renderDevice->pushClipRect(instance->viewport);
  for (auto& c : instance->bench->getChildren()) {
      if (isOverlayControl(c.get())) c->draw();   // 可见 + 类型 ∈ {Popup, ConfirmPopup, Dialog, MenuPanel}
  }
  instance->renderDevice->popClipRect();
  ```
- 宿主帧序（设计器随批）：`Clear → owner.Render → vp.Render → owner.RenderOverlays → Present`；**基础 `Render` 行为不变**（未接入宿主不受影响）。
- 子视口实例自身的浮层：仍由 `vp.Render` 在其区域内绘制（**在自身区域内最顶，整体位于 owner 浮层之下**），**无需**对每个子视口额外调用。
- **双绘影响（须评审决策，Q-1）**：浮层已在 `Render` 绘制一次，overlay pass 二次绘制——主体不透明无差，但 **MenuPanel/弹窗阴影等半透明像素会二次混合加深**（仅浮层边缘阴影带，程度轻微）。选项：
  - (a) 接受（推荐：改动最小、Render 行为零变化）；
  - (b) 新增可选开关（如 `SetOverlaySkipInRender`）：`Render` 跳过顶层浮层、宿主全权 overlay pass——**未调用 RenderOverlays 的应用浮层将不可见**，集成风险与 API 面积增加；
  - (c) overlay pass 中绕过阴影绘制——需要浮层绘制分级接口，复杂，不推荐。

### 2.2 P0-52② 浮层优先事件路由
- 在 `pumpInstanceEvents` 鼠标四类（`MouseMove/Down/Up/Wheel`）分支内、`findViewportByCoord` **之前**插入：
  ```cpp
  if (hasVisibleOverlay(instance)) { dispatchToBench(instance, evt); break; }
  ```
  判定：bench 顶层存在 `isOverlayControl` 的可见子控件（与 2.1 同一判定）。
- **穿透策略：不穿透**（浮层可见 = owner 独占鼠标输入；与既有"点击 owner 区域→owner"口径一致；外点关闭由浮层 watcher 处理、滚轮归浮层/下拉列表滚动）。若后续需要，可细化"未命中浮层且 close-on-click-outside=false → 放行子视口"（本期不做）。
- 键盘：`activeViewport` 焦点路由不变（浮层内键盘经其焦点域/watcher；Esc 关闭由 Popup watcher 已有）。
- 生命周期无状态：浮层不可见后 `hasVisibleOverlay=false`，路由自动恢复原逻辑（子视口 hover/点击恢复）。

### 2.3 备选评估
- **bench 内联子视口渲染**（子视口内容并入主实例绘制序、浮层自然置顶）：需重构多实例渲染管线与 viewport 变换/裁剪语义，风险高、跨后端一致性成本大 → 不采纳；
- 浮层延迟渲染通道：与主方案等价，无额外收益。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | 浮层（ComboBox 下拉/MenuBar 菜单/弹窗）跨子视口可见，浮层覆盖区不被视口覆盖 | 多视口像素探针（CaptureRect：浮层区域色值） |
| 2 | 浮层仍按实例 viewport 正确裁剪（超出实例边界不绘制） | 像素探针（越界区域） |
| 3 | 浮层可见时：点击画布区域内的下拉项选中；外点关闭；滚轮作用于下拉列表 | 事件注入测试（`UICornerstone_PushUIEvent`） |
| 4 | 浮层关闭后：画布子视口 hover/点击/滚轮恢复 | 事件注入测试 |
| 5 | 无浮层时 `RenderOverlays` 零开销；未接入宿主行为与现状一致 | 回归 + 代码走查 |
| 6 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **双绘阴影加深**：接受轻微加深（推荐，Render 零改动）？或新增"Render 跳过浮层"可选开关？
2. **穿透策略**：确认"浮层可见期间 owner 独占鼠标（不穿透）"？或需按 `close-on-click-outside` 细分？
3. **浮层类型集合**：`{Popup, ConfirmPopup, Dialog, MenuPanel}`（ContextMenu 归 Popup；ComboBox 下拉为其内部 Popup 子树）——是否还需纳入 `alwaysOnTop` 控件或其它类型？
4. **子视口实例浮层**：宿主无需对每个子视口调用 `RenderOverlays`（`vp.Render` 已绘制其浮层于自身区域）——确认？
5. **API 形态**：名称/签名 `UICornerstone_RenderOverlays(inst)` + Binding `RenderOverlays()`——确认？

## 5. 配套改动

- 代码：`src/UICornerstoneAPI.cpp`（`isOverlayControl`/`hasVisibleOverlay` 助手 + RenderOverlays + 鼠标路由前置判定）、`include/UICornerstoneAPI.h`、`binding/*`（DynamicApi + UICornerstone 封装）；
- 测试：多视口浮层像素 + 事件注入用例（`test_multiviewport_visual_cabi` 或新增聚焦用例，注意避让既有预存失败项）；
- 文档：CABI/Binding 速查表补行；多视口/集成章节补"宿主帧序（overlay pass）与浮层优先输入"说明；设计文档标注 + 复核归档 + make_release + 同步。
