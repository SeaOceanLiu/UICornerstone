# P104_MenuPanelResidue_Design — 含菜单栏工程重建后的菜单面板残留（P0-104 分析）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-18 追加：P0-104 中）
> 前置：P0-69（RemoveMenu 保活池）/ P0-71①（弹层 bench 挂载）/ P0-88（ownerBar 兜底）已实施
> 状态：**已闭环**（2026-10-20 设计器确认全链闭环）
> 结案结论（2026-10-20）：① **R1/R2** 真实缺陷已实施并同步，作为**防御性加固保留** ✓（设计器全量回归绿）；② 闪动定案 = **采集时序伪影 + DWM 末次呈现帧** ✓——设计器按引擎 §2.5 将采集移至 `Render+RenderOverlays` 后/`Present` 前（v6）→ **渲染目标无任何面板帧** ✓，引擎判定成立；用户视觉闪动 = 模态关闭瞬间 DWM 露出的"末次呈现帧"（点击菜单项时菜单仍开启的旧画面）✗；③ **设计器侧修复**（构建 13:18:31）：菜单动作统一入口执行前先 Clear/Render/RenderViewport/RenderOverlays/Present 一帧干净画面 → 模态关闭后露出干净帧 ✓；④ **引擎侧无进一步动作** ✓（状态/渲染一致性已证：11 场景探针 A–K 全 0 异常 + 正确时序采集无面板帧）。待用户视觉复测。
> 实施备注（2026-10-18）：① **R1 落地**：`MenuBar::detachMenu` 开启态移除 → 完整 `detachMenuPanel`（hide + bench 摘除 + 父链还原 + watcher 注销）+ `m_activeIndex=-1` + `menuMode/hovered` 复位 + **索引前移修正**（`activeIndex>idx` 减一）；② **R2 落地**：`~MenuBar` 遍历 entries 兜底摘除仍挂 bench 的面板（不依赖 activeIndex）；③ 探针 `temp_p104_probe` **8 场景全过**（A 主路径 / B 开启态移除——新断言 active=-1 + 无残留 / C 销毁栏无残留 / D 子菜单链 / E 忙等模态 / **H 真实 Win32 模态（MessageBoxTimeoutW）+ MouseUp 延迟到达 + 重建 + 重开**）；④ **闪动仍未复现**（含真实模态）——按复核 §4 流程：设计器复测手动流程；若仍闪，由设计器提供"真实对话框 + 重建"录屏帧序列再深入；⑤ P0-93/94、96/97、98/99/100、101、102、103 探针全过；全量回归 51 OK + 仅 4 项预存；Debug/Release 零警告；⑥ **帧级探针（2026-10-19，响应设计器帧级证据）**：场景 H（真实模态）与场景 I（设计器流程镜像：主栏开启→点项→延迟回调内真实模态+子视口画布菜单重建→队列事件→**连续 12 帧逐帧扫描**）均 **0 异常**（回调执行经 `[I] callback ran/done` 验证）——引擎侧流程镜像仍不复现"2 个整片重绘过渡帧"。**请设计器补充**：(a) 其诊断构建导出**这 2 个过渡帧的 BMP**（引擎按像素比对判定是否确为菜单面板及所属）；或 (b) 最小复现工程；引擎可扩展场景按帧序列回放鼠标轨迹；
> **第三轮（2026-10-19 晚，BMP 已收）**：引擎像素判读确认 f118 = **设计器 File 面板**（左缘与 文件(F) 对齐、含 Ctrl+N/O/S 与导出项——设计器清单答复一致）且 文件 项高亮；f117 为正常界面（画布内另有工程菜单栏）。**场景 K**（轨迹回放+全宽栏+大窗口+真实模态）仍 **0 异常**；`GetOpenFileNameW` 自动化关闭不可行（已列尝试）。**请设计器配合（引擎无法远程观测的点）**：①诊断构建同时记录 **`hovered-index`**（区分 f118 的 文件 高亮=hover 还是 active——若 hover 则说明鼠标曾在 File 上）；②在闪动帧直接读回**其 File 面板句柄的 `visible`**（通用属性已可用）+ 面板 parent 归属；③若其 File 面板句柄不可得，可临时用引擎 `Debug_ValidateHandle`/遍历 dump；④提供**最小复现工程**或在其诊断构建中**保留 hovered-index/面板 visible 逐帧日志**供引擎分析。⑦ **精确时序镜像（场景 J，2026-10-19 第二轮）**：按设计器提示"模态在 flush 内打开"实现——菜单项点击（延迟）→ 下一帧 flush 内真实模态（`MessageBoxTimeoutW`）→ 回调内重建 + 模态期队列保真事件（MouseUp/Move/FocusLost/Gained）→ **回调内三点检测**（pre-modal/post-modal/post-rebuild 面板可见性/捕获）+ 12 帧逐帧扫描 → **全 0 异常**（仍不复现）；**唯一状态分歧**：模态期间 `mouseButtonDown`（P0-67 捕获）残留 `cap=1`（队列 MouseUp 晚于 flush 处理所致；下一帧恢复）——**请设计器核对真实流程中 MouseUp 与模态的先后**（通常 MouseUp 先于模态处理，该残留或不存在）；⑧ **新假设待设计器验证**：帧级 bbox 第 2 帧从 **y=28** 起（菜单条行未变）与"画布左上紧邻菜单栏下方"位置吻合——闪动内容是否实为**画布菜单栏控件的面板**（其菜单项若恰为 新建/打开项目/保存 形似 File 菜单）？请核对自测工程画布菜单项文本/像素颜色，或提供 BMP 定案。

---

## 1. 设计器现象与触发矩阵（用户实测）

- 打开含"菜单栏"控件的工程 → 设计器窗口顶部菜单区**闪动约 1–2 秒**（内容=设计器自身"文件"菜单）；无菜单栏控件的工程不闪；
- 触发矩阵（用户对照实验）：**闪 ⇔ "菜单开启过" ∧ "画布重建"**（对话框、最近菜单刷新均非必要条件）；
- 设计器侧排除：自身 `active-index` 时间线干净（`-1→0→-1`，菜单从未重开）；自动探针（auto=1）区域哈希逐帧稳定（自动流程不复现）；
- **复核更正（2026-10-18）**："可见（m_visible=true）但 active-index=-1"为设计器**推断而非观测**（引擎未暴露该读回）；实际观测仅三项：①肉眼闪动（内容=设计器自身"文件"菜单）；②`active-index` 时间线干净（引擎读回）；③触发矩阵。
- **触发条件修正（复核）**：**三方合取 = 菜单开启 ∧ 真实模态对话框 ∧ 画布重建**——合成探针（无真实模态）不复现；忙碌等待（场景 E）**不等价于真实模态**（真实 `GetOpenFileNameW`/`MessageBox` 引入 OS 消息泵接管、WM 序列、SDL 消息在模态期间的累积/丢失——尤其鼠标按钮状态残留）。

## 2. 引擎侧复现（探针 `temp_p104_probe`，5 场景）

| 场景 | 步骤 | 结果 |
|---|---|---|
| A | 开启 → Esc 关闭 → `RemoveMenu`+新建面板+`AddMenu`（设计器 open 主路径类比） | 无可见面板残留 ✓、active=-1 ✓ |
| B | **开启态直接 RemoveMenu**（不先关闭）→ 重建 | 无可见面板 ✓，但**旧面板仍挂 bench 子级（hidden）** ✗、**bar active-index 残留 0** ✗ |
| C | 开启态移除 → **销毁旧栏**（画布重建类比）→ 扫描 | **残留面板滞留 bench**（bar 销毁后 parent 悬垂）✗（hidden，不绘制） |
| D | 子菜单链（hover 展开 → 点子项）→ 扫描 | 子链正常收起 ✓、无可见残留 ✓ |
| E | **模态模拟**（延迟回调内忙等阻塞帧循环）→ 重建 → 扫描 | 无可见面板 ✓、active=-1 ✓ |
| **H** | **真实 Win32 模态对话框**（`MessageBoxTimeoutW` 600ms 超时自动关；OS 消息泵接管）+ **MouseUp 延迟到达**（模态期间释放）+ 重建 + 重开 | 无可见面板 ✓、active=-1 ✓、**无鼠标捕获残留** ✓、重开正常 ✓ |
| **I** | **设计器流程帧级镜像**（2026-10-19）：主栏菜单开启 → 点项（延迟回调内：**真实模态 + 子视口画布菜单重建**）→ 模态期间释放/移动（队列）→ **连续 12 帧逐帧扫描**（主/画布面板可见性、双 active-index、鼠标捕获） | 回调执行已验证（`[I] callback ran/done`）；**12 帧全 0 异常**（mainVis/vpVis/vpBenchVis=0、act=vact=-1、cap=0）✓ |
| **J** | **设计器精确时序镜像**（2026-10-19 第二轮）：菜单项点击（延迟）→ **下一帧 flush 内打开真实模态** → 回调内重建 + **模态期队列保真事件**（MouseUp/Move/FocusLost/Gained）→ **回调内三点检测**（pre-modal/post-modal/post-rebuild）+ 逐帧扫描 | 三点检测与 12 帧全 0 异常 ✓；**观察：模态期间 P0-67 捕获残留 `cap=1`**（MouseUp 队列晚于 flush 处理；下一帧恢复 cap=0）——复核答复确认**此为真实行为**（MouseUp 必然晚于模态），非差异面 |
| **K** | **轨迹回放 + 形态镜像**（2026-10-19 第三轮）：1920 宽窗口 + **全宽贴顶主栏** + 设计器陈旧坐标序列回放（`(1894,0)/(1903,21)/(1906,14)` + MouseUp `(1913,0)`）+ flush 内真实模态 + 回调内重建 + 捕获/activeViewport 逐帧日志 | 12 帧全 0 异常；`cap=1`（模态+重建期间）→ 队列 MouseUp 后 `cap=0` ✓；`activeViewport=0` 全程（陈旧坐标未路由 vp）✓。**注**：真实 `GetOpenFileNameW`（COM IFileDialog）在本环境**无法程序化关闭**（WM_COMMAND/WM_CLOSE/BM_CLICK/DirectUIHWND-ESC/SendInput-ESC 均无效）→ K 用 `MessageBoxTimeoutW`（同为真实 Win32 模态、OS 泵接管）；轨迹/形态/日志全保真 |

**结论**：**9 场景（A–K，含真实模态、帧级镜像、精确时序与轨迹回放）均未复现**"可见（m_visible=true）但 active-index=-1"状态（隐藏残留面板不绘制——`MenuPanel::draw` 有 `m_visible` 守卫 ✓）；但确认两处**真实状态残留缺陷**（与设计器"重点核查①"一致）：

### 确认缺陷
- **R1：`MenuBar::detachMenu`（RemoveMenu 路径）未完整摘除**
  - 菜单**开启态**下移除：`detachMenu` 仅 `panel->hide()` ✗——未 `bench->removeControl` + `setParent(this)`（`detachMenuPanel` 的职责）、未 `unregisterMenuWatcher`、**未复位 `m_activeIndex`**（残留指向已移除条目，后续 `closeAllMenus/exitMenuMode` 会作用于错误索引条目）；
  - 复现：场景 B（残留 bench 子级 + active=0）。
- **R2：`~MenuBar` 清理不完整**
  - 析构仅 `if (m_activeIndex >= 0) closeAllMenus();` ✗——经 R1 残留（或索引失配）时，已移除但附着 bench 的面板**不会**被摘除 → 面板滞留 bench（bar 销毁后 parent 悬垂，hidden 不绘制但状态脏）。
  - 复现：场景 C（残留滞留）。

### 排除项
- `ControlImpl::addControl` **有去重**（同指针不重复入列）✓——排除"重复挂载双绘制"机制；
- `GET_CONTEXT`=控件自身 `m_context`（非全局）✓——`attachMenuPanel` 的 bench 取自菜单栏自身上下文，子视口菜单栏不会挂错 bench；
- 隐藏面板在 draw/handleEvent 均有 `m_visible` 守卫 ✓——残留本身不可见。

### 2.5 决定性证据辨析（2026-10-20，采集时序）

设计器第三/四轮决定性证据（`AA.csd` 仅 animation 亦复现；f118 面板像素 + hovered/active **全程 -1**）经引擎源码核对：
- 设计器诊断 `CaptureBench`（`CornerstoneDesigner/src/App.cpp:1558`）位于 `m_ui->ProcessEvents()`（:1359）之后、**`m_ui->Render()`（:1681）/`Present()`（:1684）之前** ✗——采集读的是**上一帧**的渲染目标内容；
- 因此 f118 的"菜单面板像素"= **上一帧**（用户点击菜单项时菜单仍合法开启的帧）的渲染 ✗；而 `hovered/active=-1` 是**当前帧**状态 ✓ —— **"可见但无 active-index"为采集时序伪影，非引擎状态分歧** ✓（与引擎 11 场景探针 A–K 全 0 异常一致）；
- `AA.csd`（仅 animation）触发：animation 逐帧变化使诊断的帧间哈希持续出现差异（变化集中在动画区），与菜单状态无共享路径（引擎场景 I–K 含 animation 无关状态均 0 异常）；
- **建议设计器**：将诊断采集移至 `Present()` 之后（或 `Render()` 之后、`Present()` 前），重跑用户流程——预期**无面板帧**；若正确时序下仍出现面板像素，再按"真实渲染异常"深入（附正确时序的 PNG/逐帧）。用户视觉"闪动"若仍存在，属**模态期间窗口末次呈现帧的时序观感**（宿主渲染节奏；引擎状态与渲染内容均已证明一致），可在 Present 后采集确认。

## 3. 修复方案（消除状态分歧面）

### 3.1 R1：`detachMenu` 完整摘除（src/Menu.cpp）
```cpp
shared_ptr<MenuPanel> MenuBar::detachMenu(const string& caption) {
    auto it = std::find_if(...);
    if (it == m_entries.end()) return nullptr;
    const int idx = (int)(it - m_entries.begin());
    auto panel = it->panel;
    // 开启态移除：走完整 detach（hide + bench 摘除 + 父链还原 + watcher 注销）
    if (idx == m_activeIndex) {
        detachMenuPanel(m_activeIndex);
        m_activeIndex = -1;
        m_menuMode = false;
    }
    m_entries.erase(it);
    if (panel) panel->hide();   // 保留（非开启态防残留）
    // 索引前移修正：activeIndex > idx 时减一（防后续操作错位）
    if (m_activeIndex > idx) m_activeIndex--;
    layoutEntries();
    return panel;
}
```

### 3.2 R2：`~MenuBar` 兜底摘除
- 析构时遍历 `m_entries`：凡 `panel->getParent() == bench` 者执行 detach 语义（removeControl + setParent + hide + unregister），**不依赖 `m_activeIndex` 条件**；随后 `m_entries.clear()`。

### 3.3 防御（不改行为，仅核查结论）
- `attachMenuPanel` 的 `getParent() != bench` 检查 + `addControl` 去重已防重复挂载 ✓ 不动；
- `menuPool` 回收：R1/R2 修复后，池内面板均为"已 detach（parent=bar 或 null）"状态 ✓。

## 4. 未复现部分的处理（Q-4）

探针（含**真实 Win32 模态**场景 H）均未能复现"可见无 active-index"状态。按复核 §4 流程：
1. **R1/R2 已实施**（真实缺陷、防御价值；但复核判定大概率不解释闪动）；
2. 设计器**复测**手动流程（R1/R2 同步后）；
3. 若仍闪动：设计器提供"**真实对话框 + 重建**"的**录屏帧序列**（含区域/帧号/鼠标轨迹）再深入；引擎侧 H 场景探针保留可扩展（如按帧序列回放鼠标轨迹）。

## 5. 验收与测试矩阵

1. 探针场景 A–E 转正（B 新增断言：`active==-1` + **bench 无残留面板**；C 新增：bar 销毁后 bench 无该栏面板）；
2. 新增 F：**RemoveMenu 后再次 AddMenu 复用同一面板句柄**（模型复用路径）→ 开启/关闭/重建全链正常、无残留；
3. 新增 G：双菜单栏/多条目下移除中间菜单（activeIndex 索引前移修正验证）；
4. P0-98/99/100、103 探针回归（菜单交互全链）；`test_contextmenu`/`test_menu` 回归；全量回归仅 4 项预存。

## 6. 影响面

- 源码：`src/Menu.cpp`（detachMenu / ~MenuBar，局部）；无 ABI/JSON 变更；
- 文档：用户手册 menu 页如有必要补"移除菜单自动关闭"语义（可选）；
- 设计器：R1/R2 同步后按 §4 复测。

## 7. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | `detachMenu` 开启态移除=完整 detach + activeIndex 复位 + menuMode 复位 + 索引前移修正 | 采纳 |
| Q-2 | `~MenuBar` 遍历 entries 兜底摘除（不依赖 activeIndex） | 采纳 |
| Q-3 | `attachMenuPanel`/`addControl` 去重已防护，不额外改动 | 确认 |
| Q-4 | 先修 R1/R2 → 设计器复测；仍闪再补证据深入 | 采纳 |
