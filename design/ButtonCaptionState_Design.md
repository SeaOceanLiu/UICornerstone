# ButtonCaptionState_Design — Button/CheckBox 状态联动内部 caption Label

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md` §P0 追加（2026-09-21，P0-7：caption 直控联测发现）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/ButtonCaptionState_Design_复核意见.md`，2026-09-21 通过 + 范围扩展）
> **实施结果（2026-09-21）**：
> 1. **Button/CheckBox/WinFrame 三控件 setState 联动已实施**：`setState` override → 内部 caption/title Label 跟随；`setEnable` 经虚 `setState` 自动覆盖（disabled 可达）；caption/title (重)建/替换按当前态同步。
> 2. **ImageButton 确认**：仓库无独立类（工厂 `CreateImageButton` 创建 `Button`，GetControlType 注"image-button 本质为 button"）→ Button 联动自动覆盖 ✓。
> 3. **Dialog/ConfirmPopup 自查**：无标题 Label（grep 零命中）→ 豁免 ✓。
> 4. **WinFrame hover/pressed 态可达性（明确回复，按复核 §3.2 要求）**：WinFrame **自身交互不进入 Hover/Pressed**（`onMouseEnter/onMouseLeave` 未覆写；无状态切换逻辑）——且其边框/背景**已配置 hover 差异色**（normal 0x60 → hover 0x80，WinFrame.cpp:36-43），若直接补 hover 追踪将**立即改变所有 WinFrame 的悬停视觉**（行为变更，超出本批批准范围）。本批按批准范围仅做**状态联动**：disabled 可达 + 程序化 `setState`/`SetEnum("state")` 全态可达。建议设计器将 win-frame 的 `text.hover/.pressed` 槽标注"依赖控件交互态"；若确需 hover/pressed 视觉，另开设计评估 WinFrame 交互态语义（含对既有视觉的影响）。
> 5. 测试：`test_colorfixes` 扩展 P0-7 断言（Button 全态/重建同步、CheckBox 同型、WinFrame title Normal/Disabled）——20 项全 PASS；关键回归（button/checkbox/winframe/colorpicker/layout/handlecontrol 等）全绿。
> 前置：P0-5（caption-label 句柄暴露）已实施——应用可直控 caption 的 `text.hover`/`.pressed`/`.disabled`/`text-shadow.*` 各态字段，但**态不可达**（本需求）

---

## 1. 问题与目标

| # | 现象 | 目标 |
|---|---|---|
| P0-7 | 经 `caption-label` 直控改 caption 的 `text.hover`/`text.pressed`/`text.disabled`/`text-shadow.*` 各态色——**字段写入成功（回读正确）但视觉永不显示** | Button/CheckBox 自身状态（Normal/Hover/Pressed/Disabled）变更时**联动内部 caption Label 的 state**；同型排查含内部文本子控件的控件 |

设计器侧无绕行（字段可写、态不可达）；normal 态已可用（P0-5 后）。

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| 状态设置 | `ControlImpl::setState`（ControlBase.cpp:676-678）：仅 `m_state = state`；`Control` 中为**纯虚**（ControlBase.h:246，注释"需要控件自行处理状态变化"） | 可 override；**基类不传播** |
| 使能设置 | `ControlImpl::setEnable`（:506-509）：置 `m_enable` + `setState(Normal/Disabled)`（虚调用） | 经虚分派可被 override 影响 |
| Button 状态切换 | Button.cpp:159/167/179/211 等多处 `setState(Pressed/Hover/Normal)`（事件处理内） | **只改 Button 自身** |
| CheckBox 状态切换 | CheckBox.cpp:247/253（hover）、297/301（leave）`setState(...)` | **只改自身**（同型） |
| caption 创建 | Button::create（Button.cpp:305-315）：LabelBuilder 建 caption + `setTransparent(true)` + `addControl`；**未 setEnable(false)**、无状态同步 | caption 恒 Normal（无事件到达，Button 消费） |
| CheckBox caption | CheckBox::createCaption（CheckBox.cpp:49-61）：LabelBuilder + `setTextStateColor(m_textColor)` 建一次 | 同上（恒 Normal） |
| 文本取色 | `Label::draw`（Label.cpp:355/371-382）：`switch(getState())` → `m_textColor.getDisabled()/getHover()/getPressed()/getNormal()` | **按 caption 自身 state 取色**——state 不同步则各态字段永不参与绘制 ✓（与设计器观察吻合） |

**同型排查**（含内部文本子控件/可态视觉的控件）：

| 控件 | 内部文本 | 状态联动现状 | 结论 |
|---|---|---|---|
| Button | caption Label | ❌ 不同步（本需求） | **本批修复** |
| CheckBox | caption Label | ❌ 不同步（同型） | **本批修复**（同模式） |
| ColorPicker | 关闭态 hex Label | 专有 `closed-text-color` 单色（无态语义） | 无需（关闭态不随 CP 的 hover/pressed 变色） |
| WinFrame | title Label | 专有 `setTitleTextColor` 单色路径（:407-410） | 无需（标题无态色语义；如未来需要列 backlog） |
| ComboBox / NUD | 无子 Label（自身绘制） | 自身状态色直接生效 | 无需 |
| ListView/TreeView/Menu | 行/项自绘 | 自绘按自身状态取色 | 无需 |

## 3. 架构选择与关键设计决策

### 3.1 修复方案（决策：Button/CheckBox override `setState`，caption 跟随）

```cpp
// Button.h / CheckBox.h
void setState(ControlState state) override;

// Button.cpp / CheckBox.cpp
void Button::setState(ControlState state) {
    ControlImpl::setState(state);
    if (m_caption) m_caption->setState(state);   // 内部文本随 Button 状态取色（Label::draw 按 state）
}
```

决策点：
- **override `setState` 而非 setEnable**：`ControlImpl::setEnable` 内部走虚 `setState`（:508）——override setState 后 disabled 路径自动覆盖；无需改 setEnable。
- **不在基类做通用传播**（备选，否决）：`ControlImpl::setState` 泛化会波及**所有子控件**（Actor/动画/嵌套控件等无态语义者，以及本应独立持态的控件），面过宽、风险高；按控件定向联动（与设计器建议一致）。
- **caption (重)建时同步当前状态**：Button::create（caption 创建后）与 `setCaptionLabel`（替换后）补一次 `m_caption->setState(getState())`——避免"Button 已 Hover/Pressed/Disabled 时新 caption 停留 Normal"；CheckBox::createCaption 同。
- **caption 不 setEnable(false)**：保持现状（Label 非交互，事件被父消费；state 由父显式同步）。禁用 Button 时 caption 收到 Disabled state → Label::draw 用 disabled 色 ✓。
- 不影响既有语义：Button/CheckBox 自身各态视觉（actor/背景/边框/勾选框）不变；仅新增 caption 的状态跟随。

## 4. API 设计

- 核心库：Button.h/.cpp、CheckBox.h/.cpp 各 +1 override（+ caption 创建/替换点状态同步）。
- C ABI / Binding / PropertyNames：**零变更**（纯行为修复；各态色字段写入路径 P0-5 已通）。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| P0-7-1 Button caption 随态 | 内部测试：`btn->setState(Hover/Pressed/Disabled/Normal)` → `btn->getCaptionLabel()->getState()` 逐一相符 |
| P0-7-2 disabled 经 setEnable | `btn->setEnable(false)` → caption state == Disabled（经虚 setState 路径）；`setEnable(true)` → Normal |
| P0-7-3 caption 重建同步 | Button 处于 Hover 时 `setCaptionLabel(新 label)` → 新 caption state == Hover；`create` 同（状态已设时） |
| P0-7-4 CheckBox 同型 | 同 P0-7-1/2 断言（CheckBox caption） |
| P0-7-5 视觉链路 | caption 句柄设 `text.hover` 色 → `btn->setState(Hover)` → caption 取色路径命中 hover（`getTextStateColor().getHover()` 可断言字段；视觉由设计器联测） |
| P0-7-6 回归 | test_button/test_checkbox/test_colorfixes 全绿；Button/CheckBox 自身各态视觉无回归 |

测试载体：`test_colorfixes`（内部断言扩展，控件可由 LayoutParser 或 Builder 构造）。

## 6. 影响面与风险

- 仅 Button/CheckBox 新增状态联动：自身各态绘制不变；caption 从"恒 Normal"变为"随父态"——**使用 caption 各态色的应用获得正确行为**；只用 normal 色的应用零感知。
- `setState` 在事件热路径（hover/pressed 切换）多一次 caption 调用（仅指针判空 + 赋值）——开销可忽略。
- 无 ABI/键/文档契约变化；三后端无涉。

## 7. 待审核问题

1. **联动范围**：Button + CheckBox 本批（同型排查其余控件确认无需——§2 表）？
2. **caption (重)建时按当前态同步**（§3.1 决策点 3）——接受（覆盖 Hover 中替换 caption 的边缘）？
3. **WinFrame 标题态色**留 backlog（当前标题无态色语义需求）——确认？
4. 验证载体：`test_colorfixes` 内部断言扩展——确认？

## 8. 待提交配套改动（实施时随批）

- 测试扩展（§5）+ 全量回归；设计文档状态标注；复核意见归档。
- 用户手册 Button/CheckBox 章节：补注"caption 各态色经 caption-label 句柄设置，随控件状态自动生效"。
- make_release 同步（设计器可完成颜色组各态色槽全链路联测）。