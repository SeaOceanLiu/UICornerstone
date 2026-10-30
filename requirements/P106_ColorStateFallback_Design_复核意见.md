# P106_ColorStateFallback_Design — 设计复核意见

> 评审方：CornerstoneDesigner（设计器侧）
> 评审对象：`D:\GitSpace\UIControls\design\P106_ColorStateFallback_Design.md`（2026-10-21 待评审版）
> 关联需求：`Temp/UICornerstone_配合修改清单_第三批.md` → P0-106（中）
> 结论：首版 **退回修订** → 修订版 **基本通过，附 3 条实施补充**（见 §7 第二轮复核）

---

## 1. 结论摘要

设计文档的**工作方向正确**（P0-30 已有"未设=normal"的意图；真正缺口包含三控件构造写入），但**实施项不完整**，按现方案落地后仍不满足 P0-106：

1. `disabled` 不会回退 normal——`StateColor` 构造显式初始化为内置禁用色（灰）✗；
2. `hover/pressed` 只回退"内置缺省 normal"，**不跟随宿主设置的实际 normal** ✗——设计器/宿主对每个控件都会设 normal，落地方案会出现"自定义底色控件的 hover 变回内置灰/蓝"；
3. 文档验收矩阵（"新建控件不设色 → 各态相等"）在**完全不修**的情况下也能通过 ✗，无法证明修复。

---

## 2. 事实核对

| # | 设计文档表述 | 核实结果 | 证据 |
|---|---|---|---|
| ① | StateColor 构造已回退 normal（P0-30） | **部分成立（静态缺省 ✗）**：hover/pressed 初始化为内置 `DEFAULT_*_NORMAL_COLOR`，不是跟随 `normal` 成员；`setNormal` 不联动未设态 | `include/ControlBase.h:47-58`；`src/ControlBase.cpp:729-731` |
| ② | 三控件构造显式写入历史缺省态色 | **成立 ✓**：Button 8 行 / CheckBox 8 行 / Label 2 行（hover/pressed） | `src/Button.cpp:20-27`；`src/CheckBox.cpp:30-37`；`src/Label.cpp:36-37` |
| ③ | 读回路径 → StateColor.hover（构造回退后 = normal） | **仅在 normal 从未修改时成立 ✗**；读回返回的是存储值，无解析逻辑 | `src/ControlBase.cpp:1084-1109` |
| ④ | disabled 无构造写入 → 已为 normal | **错误 ✗**：构造显式初始化为 `DEFAULT_DISABLED_COLOR` / `DEFAULT_BORDER_DISABLED_COLOR` / `DEFAULT_TEXT_DISABLED_COLOR` / `DEFAULT_TEXT_SHADOW_DISABLED_COLOR`（如 128,128,128,255） | `include/ControlBase.h:59-63`；`src/ConstDef.cpp:13` |

**反例（现方案落地后的实际行为）**：

```cpp
btn.setColorProperty("background", SColor(0xDC,0x26,0x26,0xFF));   // normal = 红
btn.getColorProperty("background.hover", c);    // 仍为 DEFAULT_NORMAL_COLOR（内置灰），≠ 红 ✗
btn.getColorProperty("background.disabled", c); // 仍为 (128,128,128,255) ✗
```

---

## 3. 修订要求（必须）

1. **增加"显式设置"标记**（推荐）：每个色组（background/border/text/text-shadow）为 hover/pressed/disabled 各维护 1 位（或状态值 + dirty 位）；`setColorProperty` 对应单态键时置位；
2. **解析型 getter**：未显式设置的态 → 返回该组 `normal`；`getColorProperty` 与绘制/命中解析统一走解析值（`resolveStateColor` 同口径）；
3. **disabled 一并纳入**：构造缺省与 P106 语义冲突，必须回退 normal（含 `ConstDef::DEFAULT_*_DISABLED_COLOR` 在 `StateColor` 构造中的使用）；
4. **语义边界**：明确 `setNormal` 后未设态跟随、布局加载先 normal 后状态键的顺序、重置/主题应用时的行为；
5. **测试矩阵修正**（关键）：
   - 新建控件**先 `setColorProperty("background", 自定义色)`**，再断言 hover/pressed/disabled == 该色（四组 × 三态）；
   - 显式设置某态为独立色 → 不被 normal 覆盖；
   - 读回与像素两条路径同断言；
   - 现有"不设任何色 → 相等"用例保留，但**不能单独作为验收**；
6. **范围声明**：WinFrame 关闭按钮/标题栏等**内部子部件的显式内置色**不属于 P106 范围（如 `src/WinFrame.cpp:48-56`），文档需写明，避免误删；
7. **兼容性**：移除三控件构造写入后，引擎自带示例/未设色场景的 hover/pressed 视觉会变为 normal（需求目标 ✓）；如仍需保留旧观感，建议把历史色移入**默认主题**（显式设色路径），而不是保留在构造函数中。

---

## 4. Q-1 补充建议

设计文档 Q-1 建议"不保留内置缺省态色"——同意语义目标 ✓。但请区分两条路径：

- **未设色控件**：未设态 = normal ✓（本需求）；
- **引擎内置示例的既有观感**：请评估是否由"默认主题"显式设色承接（避免示例视觉突变）；若引擎无默认主题机制，请明确接受示例观感变化。

---

## 5. 设计器侧配合（DLL 同步后）

1. 删除导出归一化（`colorStatesWithFallback`）——行显示回退已由引擎读回解析值自动生效；
2. 已保存工程中此前被归一化填充的显式态保持不变（无损，无需迁移）；
3. 回归：控件属性动态行 hover/pressed/disabled 显示、导出 JSON 不含冗余态、放置/撤销流程不变。

---

## 6. 待引擎确认

| # | 问题 | 设计器建议 |
|---|---|---|
| Q-A | 是否新增"查询某态是否显式设置"的 API（如 `IsColorStateExplicit`） | 暂不新增；设计器按自身模型记账，引擎只保证读回解析值 |
| Q-B | 默认主题是否承接历史 hover/pressed 观感 | 由引擎决定；语义上不应放回控件构造 |
| Q-C | 布局加载状态键应用顺序（normal 与各态） | 解析 getter 方案下顺序无关，请按此实现 |

---

## 7. 第二轮复核（2026-10-21 修订版）

> 评审对象：`P106_ColorStateFallback_Design.md` 修订版（状态：修订版待复核）
> 结论：**基本通过**——首版 7 项问题均已覆盖；以下 3 条为实施前必须补齐的细节。

### 7.1 已确认通过 ✓

| 首版问题 | 修订版落实 | 核实 |
|---|---|---|
| disabled 未回退 | §2.1：disabled 纳入 flags；1 参构造不再使用 `DEFAULT_*_DISABLED_COLOR` | 与 `ControlBase.h:59-63` 现状对应 ✓ |
| hover/pressed 静态回退 | §2.1 解析型 getter：未置位返回该组当前 normal（`setNormal` 后动态跟随） | 解决 `ControlBase.cpp:729-731` 不联动问题 ✓ |
| 无显式位 | `stateFlags`（bit0/1/2，每组独立）；copy/move/赋值同步 | 覆盖构造/拷贝/赋值路径 ✓ |
| 测试矩阵无效 | §4.1 先设自定义 normal → 断言三态；§4.2 显式态不被覆盖；§4.3 读回+像素 | ✓ |
| 范围声明 | §2.4 内部子部件/菜单自管色不在本批 | ✓ |
| 加载顺序 | §2.3：逐键 setColorProperty，顺序无关 | 已核实 `LayoutParser::applyColorGroupPerKey`（`LayoutParser.cpp:3658-3684`）逐键写、未给键不写 ✓；字符串=normal ✓ |
| 4 参构造语义 | `stateFlags=7`（主题/单色显式） | 调用点为显式风格/快照，未发现"改 normal 需跟随"场景 ✓ |

### 7.2 必须补充（阻塞项）

1. **手工重打包 StateColor 的位丢失** ✗
   `WinFrame::setBackgroundStateColor`（`src/WinFrame.cpp:466-475`）用 `cp.setNormal/setHover/setPressed/setDisabled` **重建**目标色组：
   - 未设态的 `getHover()` 返回解析后的 normal，但 `cp.setHover(...)` 会**置显式位**；
   - `cp.setPressed(normal)/setDisabled(normal)` 同样全部置位；
   → 之后 `setNormal` 不再跟随，"未设=normal"的语义被破坏 ✗。
   设计器中 WinFrame 背景走对象路径时会触发。
   **要求**：直接赋值源 `StateColor`（赋值已带 flags），仅对确有意图的态用 `set*`；或提供"按源 flags 复制并覆盖指定态"的辅助方法。并请逐一审计 `setStateColorProperty` 的 override 重打包点：`WinFrame` / `ListView` / `MenuPanel` / `TabControl` / `TreeView`（`ControlImpl::setBackgroundStateColor` 为直接赋值 ✓ `ControlBase.cpp:702-709`）。

2. **测试矩阵补一格**：对象路径 round-trip ✗
   设 normal=C → `SetStateColor`（或 WinFrame 背景对象路径）→ 再 `SetColor("background", C2)` → 断言 hover/pressed/disabled == C2（防止重打包把 flags 全部置位）。
   像素断言请使用**无 `m_disabledActor`** 的控件（Button 有 disabled actor 时以 actor 为准，`src/Button.cpp:78-92`），或文档注明该例外。

3. **补回"影响面/文档"章节** ✗
   首版 §5 影响面在修订版中缺失，请补：
   - 源码清单（Button/CheckBox/Label + `StateColor`/`ControlBase` + 上述重打包点）；
   - 文档：控件开发规范（状态色缺省约定）+ 用户手册 colors 章节；
   - ABI 说明：`StateColor` 为 C++ 内部类型，C ABI 的 `UIStateColor` 结构不变 ✓。

### 7.3 设计器侧确认

- DLL 同步后删除导出归一化（`colorStatesWithFallback`）✓；行显示回退保留无害（读回自动为普通值）✓；
- WinFrame 背景/子控件配色回归（本次 §7.2-1 修复后）纳入联测 ✓。
