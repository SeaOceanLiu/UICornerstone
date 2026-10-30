# P103_MenuClickDeferred_Design — 菜单点击回调统一延迟一帧执行（模态对话框不遮挡菜单关闭）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-18 追加：P0-103 中）
> 前置：P0-98（菜单点击门控/关链）已实施并同步
> 状态：**已放行，已实施**（2026-10-18；复核：`requirements/P103_复核意见.md`——通过）
> 实施备注（2026-10-18）：① `UIContext::deferredClicks`（闭包持 sender shared_ptr）+ `MenuItem::handleEvent` 关链同步、回调入队；② flush：`UICornerstone_ProcessEvents` 与 `MainWindow::processEvents` 帧起始（`FlushDeferredMenuClicks`，move-out 重入保护）；`UIContext::destroy` 清空队列；③ **实施中发现并修复（test_contextmenu 段错误定位）**：`ContextMenu::addItem` 三处内部 onClick 包装捕获裸 `this`——延迟后菜单先于回调析构即悬垂 ✗ → 改 **weak_ptr 捕获**（lock 失败安全跳过 close）；parser 路径（handler 按值持有）与 binding（Impl 注册表持有）本就安全 ✓；④ 测试更新：`test_contextmenu` 改帧序语义（断言段手动驱动一帧；帧 25 入队 → 帧 26 断言关闭）——**19/0 通过**；⑤ 探针 `temp_p103_probe` **3/3**（帧序 deferred=1、cbFrame=clickFrame+1 / 第二次点击 / 销毁安全）；P0-98 探针改延迟帧序断言后 9/9；P0-93/94、96/97、101、102 探针全过；全量回归 **51 OK + 仅 4 项预存**；Debug/Release 零警告。

---

## 1. 复现与现状核实

**复现（设计器，用户实测）**：点击菜单项（如"导出项目报告"）→ 菜单**仍呈现在窗体中**（模态目录对话框打开期间）✗——与常规应用"先见菜单关闭、再弹对话框"的体验不符。

**源码核实**：

| # | 事实 | 依据 |
|---|---|---|
| ① 关链先行（P0-98 已修） | `MenuItem::handleEvent` MouseDown：`closeMenuChain()` → `m_onClick(sender)` → `fireCCallback(kEventClick, None)`——**三者同帧同步执行** | `src/Menu.cpp`（P0-98①b 分支） |
| ② 渲染由宿主驱动 | 引擎渲染在宿主循环内 `UICornerstone_Render`（Present）执行；事件在 `UICornerstone_ProcessEvents` 内分发——**点击帧**：ProcessEvents（关链逻辑完成）→ 宿主 Render（应呈现关闭）→ 下一帧 | `UICornerstone_ProcessEvents` / `UICornerstone_Render` |
| ③ 模态阻塞 | 宿主回调（`m_onClick` 或 click 回调）内打开**模态对话框**（阻塞宿主循环）→ 点击帧的 Render **永不执行** → 屏幕上仍是上一帧（菜单可见）✗ | 设计器实测 + 宿主模态语义 |
| ④ 设计器过渡 | 宿主侧 `guard` 延迟一帧（`m_pendingActions`，主循环 Present 后执行）——**引擎实施后可回收** | 设计器清单 P0-103 附注 |

**结论**：根因=回调与关链同帧同步，而"关闭的可见性"依赖回调返回后的宿主 Render；回调阻塞（模态）即破坏该呈现。**延迟一帧执行回调**是框架层正确机制（引擎无法在回调内强制宿主重绘）。

## 2. 方案

### 2.1 延迟队列（引擎侧，per-instance）
- `UIContext` 增 `std::vector<std::function<void()>> deferredClicks;`（避免 UIContext 依赖 Menu.h；闭包在 MenuItem 侧构造）；
- `MenuItem::handleEvent`（Normal 点击分支）改为：
  1. `closeMenuChain();`（**同步不变**——菜单立即进入关闭态）；
  2. 构造闭包入队（`owner = m_context->owner ? m_context->owner : m_context`）：闭包持 **sender `shared_ptr<MenuItem>` 保活**（`dynamic_pointer_cast<MenuItem>(getThis())`）→ 执行 `m_onClick` + `fireCCallback(kEventClick, CCallbackData::None, nullptr)`（无载荷、无指针时效问题）；
- 生命周期：实例 `destroy` 时清空队列（防悬垂闭包）；项在 flush 前被重建销毁 → 闭包保活 sender（对象存活、无 UI 副作用——关链已完成）。

### 2.2 Flush 时机
- **下一次 `UICornerstone_ProcessEvents` 起始**统一 flush（此时上一帧已 Render/Present → 菜单关闭已可见；模态在 flush 内打开亦不遮挡关闭呈现）；
- **Mode 1**（`MainWindow::processEvents`）同样在起始 flush；
- **重入保护**：flush 前把队列 move 到局部再逐项执行（回调内若再次调用 ProcessEvents 不重入）；flush 中产生的新点击入新队列、下一帧执行（FIFO 语义一致）。

### 2.3 语义变化与文档
- **所有菜单项点击回调（`m_onClick` 与 `SetCallback("click")`）统一延迟一帧**——常规 UI 框架语义，风险低；
- 文档注明：API 头 Menu 三件套注释、用户手册 `docs/controls/menu.html`（click 行 + 示例注）、`design/P093_094` §7.6 补记；
- Esc/外点关闭/禁用项路径**不受影响**（不触发 click）。

### 2.4 范围界定（Q-1）
- 仅 **MenuItem**（设计器要求）；Button/Label 等其它控件回调的同类"模态遮挡"问题**不扩面**（留待按需另议）。

## 3. 验收与测试矩阵

1. **帧序断言（探针）**：合成点击（帧计数 N）→ 回调在 **N+1 次 ProcessEvents** 触发 + 触发时菜单已隐藏（`hovered-index`/面板可见性读回）；回调内记录帧号；
2. **关链即时性**：点击后（同帧）面板 `isVisible()==false`（不等回调）；
3. **回调内打开引擎 Dialog**（非阻塞）行为正常（flush 帧内执行）；
4. **多次点击 FIFO**：两项连点 → 回调顺序与次数正确；
5. **实例销毁**：入队后立即 DestroyInstance → 不崩溃（队列清理）；
6. 回归：P0-98 探针（动态 click 关链）改帧序断言后全过；全量回归仅 4 项预存。

## 4. 影响面

- 源码：`include/UIContext.h`（队列成员）、`src/Menu.cpp`（入队替代同步调用）、`src/UICornerstoneAPI.cpp`（flush 辅助 + ProcessEvents 起始调用 + destroy 清理）、`src/MainWindow.cpp`（Mode 1 起始 flush）；
- 无 C ABI/JSON schema 变更；**行为语义**：菜单点击回调延迟一帧（文档注明）；
- 设计器：同步后**回收 `m_pendingActions` 过渡**。

## 5. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 范围仅 MenuItem（不扩至其它控件） | 是（不扩面） |
| Q-2 | flush 时机=下一次 ProcessEvents 起始（Mode 1 同理） | 确认 |
| Q-3 | 多次点击 FIFO、重入保护（move-out 再执行） | 确认 |
| Q-4 | 文档明确"菜单点击回调延迟一帧"语义 | 确认 |
