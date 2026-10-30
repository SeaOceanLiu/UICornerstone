# P106_ColorStateFallback_Design — 颜色状态缺省回退（未显式设置的 hover/pressed/disabled = normal）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-21 追加：P0-106 中）
> 状态：**已放行，已实施**（2026-10-21；第二轮复核：基本通过——3 项补充全部落实后实施）
> 实施备注（2026-10-21）：① `StateColor` 增 `stateFlags` + 解析型 getter（未设态动态返回**当前 normal**）+ `clearHover/clearPressed/clearDisabled`（两态声明辅助）；1 参构造 hover/pressed/disabled 全部未置位（**disabled 不再使用内置禁用色**）；4 参构造 `stateFlags=7`；copy/move/赋值同步 flags；② 移除 Button/CheckBox/Label 构造历史缺省写入（8/8/2 行）；③ **第二轮补①**：`WinFrame::setBackgroundStateColor` 重打包改为"直接赋值 + clearPressed/clearDisabled"（审计结论：ListView/TreeView/TabControl 直接赋值 ✓、MenuPanel 自管成员模型范围外 ✓）；④ **第二轮补②**：探针含 WinFrame 对象路径 round-trip + 像素例外注明（无 disabledActor 控件）；⑤ **第二轮补③**：§5 影响面/文档章节已补；⑥ 验证：探针 `temp_p106_probe` **4/4**——矩阵 36/36（3 控件 × 4 组 × 3 态，先设自定义 normal）/ 显式态不被覆盖 / WinFrame round-trip / **像素 hover==normal (220,38,38)**；P0-93/94~105、107 探针全过；全量回归 51 OK + 仅 4 项预存；Debug/Release 零警告。
> 复核意见：`requirements/P106_ColorStateFallback_Design_复核意见.md`

---

## 0. 首版复核问题与修订对应

| 复核问题 | 本版修订 |
|---|---|
| ① `disabled` 未回退 normal（StateColor 构造显式初始化 `DEFAULT_*_DISABLED_COLOR` 灰） | §1④/§2：disabled 纳入显式位；未设 → 解析为 normal；构造不再使用内置禁用色 |
| ② hover/pressed 只回退**静态内置** normal，不跟随宿主设置的**实际** normal | §2：改为**解析型 getter**——未设态动态返回该组当前 `normal`；`setNormal` 后自动跟随 |
| ③ 验收矩阵"不设任何色 → 各态相等"在完全不修时也能通过，无法证明修复 | §4：改为**先设自定义 normal** 再断言三态 == 该色；显式态不被覆盖；读回+像素双路径 |
| ④ 需"显式设置"标记 | §2：`StateColor::stateFlags`（每色组独立，3 位） |
| ⑤ 语义边界（setNormal/加载顺序/重置/主题） | §2.3 |
| ⑥ 范围声明（WinFrame 内部子部件显式色不动） | §2.4 |
| ⑦ 兼容性（历史观感去向） | §3 / Q-B |
| Q-A/B/C | §5 答复 |
| **第二轮补①** 手工重打包位丢失（WinFrame） | §2.5：WinFrame 两态声明改为"直接赋值 + clearPressed/clearDisabled"；全库重打包点审计结论（ListView/TreeView/TabControl 直接赋值 ✓；MenuPanel 自管成员模型范围外） |
| **第二轮补②** 对象路径 round-trip 测试 | §4.8 |
| **第二轮补③** 影响面/文档章节 | §5（本版补回） |

## 1. 需求与现状核实

**需求**：控件未显式设置 `hover`/`pressed`/`disabled` 状态色时，其值应**与 `normal` 一致**（`background` / `border` / `text` / `text-shadow` 四组）；引擎在状态色解析层统一实施（一处生效覆盖全部宿主；动态行 `GetColor` 读回自动得到回退值；导出无需填充缺省态）。

| # | 事实 | 依据 |
|---|---|---|
| ① | `StateColor` 构造仅为 hover/pressed 提供**静态内置缺省**（`DEFAULT_*_NORMAL_COLOR`），**不跟随运行时 normal**；`setNormal` 不联动未设态 ✗ | `include/ControlBase.h:42-58`；`src/ControlBase.cpp:729-731` |
| ② | **三控件构造时显式写入历史缺省态色**：Button 8 行 / CheckBox 8 行 / Label 2 行（hover/pressed，bg/border/text/text-shadow） | `src/Button.cpp:20-27` / `src/CheckBox.cpp:30-37` / `src/Label.cpp:36-37` |
| ③ | 读回 `getColorProperty("background.hover")` → `bg.getHover()` **返回存储值**，无解析逻辑 ✗（仅在 normal 从未修改时碰巧成立） | `src/ControlBase.cpp:1084-1109` |
| ④ | **disabled 未回退**：`StateColor` 构造显式初始化为 `DEFAULT_DISABLED_COLOR` / `DEFAULT_BORDER_DISABLED_COLOR` / `DEFAULT_TEXT_DISABLED_COLOR` / `DEFAULT_TEXT_SHADOW_DISABLED_COLOR`（如 128,128,128,255）✗ | `include/ControlBase.h:59-63`；`src/ConstDef.cpp:13` |
| ⑤ | 绘制取态路径统一走 getter（`m_bgColor.getHover()` 等）✓——解析型 getter 可一处覆盖绘制/读回 | `src/ControlBase.cpp:241-278` |
| ⑥ | 4 参构造（`StateColor(n,h,a,d)`）用于主题应用/单色四态语义（`c,c,c,c`）——应视为**全部显式** | `src/LayoutParser.cpp:2581,2619`；`src/Menu.cpp:1520,1524`；`src/Splitter.cpp:565`；`src/TreeView.cpp:1012,1032,1076` |

## 2. 方案

### 2.1 StateColor：显式位 + 解析型 getter（核心）
- 增 `unsigned char stateFlags = 0;`（bit0=hover / bit1=pressed / bit2=disabled；**每个色组实例独立**）；
- `setHover/setPressed/setDisabled`：存值 + 置位；
- `getHover/getPressed/getDisabled`：**置位 → 返回存储值；未置位 → 返回该组当前 `normal`**（动态解析，`setNormal` 后未设态自动跟随）；
- 1 参构造：`normal` 取组内静态缺省（保留）；`hover/pressed/disabled` **不置位**、成员值占位为 normal；**disabled 不再使用 `DEFAULT_*_DISABLED_COLOR`**；
- 4 参构造：`stateFlags=7`（调用方显式提供四值——主题/单色语义）；
- copy/move ctor 与赋值：同步 `stateFlags`（防显式位丢失）。

### 2.2 移除三控件历史缺省写入
- 删除 `Button.cpp:20-27` / `CheckBox.cpp:30-37` / `Label.cpp:36-37` 的构造 `set*` 调用——未设态即回退 normal（`set*` 会置显式位，必须移除）。

### 2.3 语义边界
- `setNormal(c)` 后：未设态 getter 返回 c（动态跟随）✓；已设态保持；
- **布局加载顺序无关**：解析 getter 下 `normal` 与各状态键的应用先后不影响最终解析值（Q-C 答复）；
- 重置/主题应用：走 `set*`（显式）→ 正常覆盖；恢复缺省 = 新建 StateColor 或清位（本批不提供清位 API，Q-A）；
- 导出/保存：未设态无需填充（Q-1 确认：不保留内置缺省态色）。

### 2.4 范围声明
- 仅 `StateColor` 驱动的控件色组（background/border/text/text-shadow）；
- **WinFrame 关闭按钮/标题栏等内部子部件的显式内置色**不属本批（`src/WinFrame.cpp` 等显式设色，保持原样，避免误删）；
- 菜单栏/菜单面板的自管颜色成员（`m_hoverBgColor` 等独立体系）不在本批（其缺省体系独立，另行按需）。

### 2.5 重打包点审计（第二轮补①）
`setStateColorProperty` override 逐一核查：
| 位置 | 现状 | 结论 |
|---|---|---|
| `ControlImpl::setBackgroundStateColor` 等（ControlBase.cpp:702-709） | 直接赋值（flags 随拷贝） | ✓ 不动 |
| **`WinFrame::setBackgroundStateColor`（WinFrame.cpp:467-475）** | 用 `cp.setNormal/setHover/setPressed/setDisabled` **重建**（全部置显式位）✗ | **修**：`StateColor cp = stateColor; cp.clearPressed(); cp.clearDisabled();`（两态声明：pressed/disabled 回退 normal 且不置位） |
| `ListView::setStateColorProperty`（行样式）/ `TreeView::setStateColorProperty`（节点样式）/ `TabControl::setStateColorProperty`（页背景） | 直接赋值到成员/行/节点 | ✓ flags 保留 |
| `MenuPanel::setBackgroundStateColor` 等 | 映射到自管成员（`m_bgColor/m_hoverColor/...`，非 StateColor flag 模型） | 范围外（§2.4），行为由解析 getter 提供回退值 |

新增 `StateColor::clearHover/clearPressed/clearDisabled`（清显式位 + 值复位 normal），供"两态声明"类场景保留语义且不误置位。

## 3. 兼容性与影响

- 无主题/未设色场景：Button/CheckBox/Label 的 hover/pressed/disabled 视觉变为 ==normal（**需求目标** ✓）；引擎内置示例同类观感随之变化；
- **历史观感去向（Q-B）**：引擎当前无"默认主题"机制——按复核建议**不将历史色放回构造函数**；如需保留旧观感，由示例/布局**显式设色**承接（本批不改示例，接受观感变化）；
- 显式设色控件（设计器/导出应用/主题）行为不变 ✓；
- 设计器：引擎同步后回收导出归一化（行读回自动回退）；已保存工程中已归一化填充的显式态保持不变（无损）。

## 4. 验收与测试矩阵（按复核修正）

1. **动态跟随（核心）**：新建 Button/CheckBox/Label → `SetColor("background", 自定义色 C)` → 断言 hover/pressed/disabled 读回均 == C；四组（background/border/text/text-shadow）× 三态（hover/pressed/disabled）矩阵；
2. **显式态不被覆盖**：设置 hover=H（≠normal）→ 将 normal 改为 C2 → hover 仍 H、pressed/disabled == C2；
3. **像素双路径**：未设 hover 的按钮 hover 时区域像素 = normal（采样）；显式设 hover 的按钮不受影响；读回与像素结论一致；
4. **默认 normal 场景**：不设任何色 → 三态 == 该组静态缺省 normal（保留用例，但**不单独作为验收**）；
5. **布局顺序**：JSON 中 `colors.background` 与 `background.hover` 任意先后 → 解析值一致；
6. **主题回归**：设计器全量布局（显式设色）视觉与回归不变；auto=2/3 全绿；
7. **对象路径 round-trip（第二轮补②）**：设 normal=C1 → `WinFrame::setBackgroundStateColor`（两态对象路径）→ 再 `SetColor("background", C3)` → 断言 hover 保留显式值、pressed/disabled == C3（防重打包把 flags 全部置位）；
8. **像素例外注明**：像素断言使用**无 `m_disabledActor`** 的控件（Button 设有 disabled actor 时以 actor 为准，Button.cpp:78-92）；含 actor 控件按读回断言；
9. 全量回归仅 4 项预存；Debug/Release 零警告。

## 5. 影响面与文档（第二轮补③）

- **源码**：`include/ControlBase.h`（StateColor flags/解析 getter/clear 辅助）；`src/Button.cpp` / `src/CheckBox.cpp` / `src/Label.cpp`（删除构造历史缺省写入）；`src/WinFrame.cpp`（重打包修复）；
- **文档**：控件开发规范（`design/guidelines/coding.md`：状态色缺省约定"未设态=normal"）；用户手册 colors 章节（`docs/controls/*.html` colors 说明补充回退语义）；
- **ABI**：`StateColor` 为 C++ 内部类型；C ABI `UIStateColor` 结构不变 ✓（无接口变更）；
- **设计器**：引擎同步后回收导出归一化；WinFrame 背景/子控件配色回归纳入联测。

## 6. 待确认（Q，含复核答复）

| # | 问题 | 结论 |
|---|---|---|
| Q-1 | 是否保留"无主题时的内置缺省态色" | **不保留**（未设态=normal；历史观感见 §3/Q-B） |
| Q-2 | disabled 一并纳入 | **纳入**（本版核心修订） |
| Q-A（复核） | 新增"某态是否显式"查询 API | **不新增**（设计器自记账；引擎只保证读回解析值）✓ |
| Q-B（复核） | 默认主题承接历史观感 | 引擎无默认主题机制 → **接受示例观感变化**；历史色不入构造函数 ✓ |
| Q-C（复核） | 布局加载状态键应用顺序 | 解析 getter 下**顺序无关** ✓ |
