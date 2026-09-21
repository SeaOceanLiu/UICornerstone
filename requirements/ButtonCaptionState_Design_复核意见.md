# ButtonCaptionState_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-21
- 对象：`design/ButtonCaptionState_Design.md`（状态：待评审）
- 结论：**通过，放行实施**（附**范围扩展请求**——用户明确要求"一次性把相关控件的全部状态联动都支持"，见 §3）

## 1. 分项评审

### §2 现状核实与同型排查 ✓（定位准确）

- `ControlImpl::setState` 仅改自身、`Control::setState` 为纯虚（基类不传播）——源码级核实 ✓。
- `setEnable → 虚 setState`（disabled 路径可被 override 覆盖）——关键判断正确 ✓。
- `Label::draw` 按 caption 自身 state 取色——与设计器观察（字段写入成功、态不可达）吻合 ✓。

### §3.1 方案 ✓

- **override `setState` 而非 setEnable**：正确（虚分派使 disabled 自动覆盖，改动最小）✓。
- **不泛化基类传播**：正确判断（Actor/嵌套控件面过宽）✓。
- **caption (重)建时按当前态同步**：覆盖"Hover 中替换 caption"边缘——严谨 ✓。
- **caption 不 setEnable(false)**：保持现状（事件由父消费，state 显式同步）✓。

### §5 验收 ✓（含 disabled 路径、重建同步、回归）

## 2. 对 §7 待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 联动范围 Button+CheckBox 本批 | **确认 + 扩展请求（见 §3）** |
| 2 | caption 重建按当前态同步 | **接受** |
| 3 | WinFrame 标题态色留 backlog | **建议纳入本批（见 §3.2）** |
| 4 | 验证载体 test_colorfixes | **确认** |

## 3. 范围扩展请求（用户要求：一次性全部支持）

### 3.1 背景

设计器属性面板对**所有控件类型**生成 `colors.*` 15 色槽（schema common 通用）——**任一含内部文本的控件，其 text/textShadow 各态色槽都应可用**；否则面板出现"能设置但不生效"的槽位（当前 Button 已修、CheckBox 本批、**其余同型缺口需一并清零**）。

### 3.2 WinFrame 纳入本批（建议）

- **现状**：`WinFrame::m_titleLabel`（WinFrame.cpp:68 创建）——**无状态联动**（无 setState override）→ title 的 `text.hover`/`.pressed`/`.disabled` 同型缺口。
- **建议**：与 Button/CheckBox 同模式——`WinFrame::setState` override → `m_titleLabel->setState(state)`；标题 (重)设时同步当前态。设计器侧 win-frame 类型色槽即可全态可用。
- 若引擎评估成本不可接受（title 无态视觉语义等），**请明确回复理由**，设计器将 win-frame 的 text.* 非 normal 槽列入"不支持"清单（面板需注明）——不宜静默不生效。

### 3.3 ImageButton 确认

- 仓库无独立 ImageButton 类文件——**应为工厂创建的 Button**（GetControlType 亦注"image-button 本质为 button"）→ **Button override 自动覆盖** ✓。请引擎实施时确认（若存在独立类则同批纳入）。

### 3.4 Dialog / ConfirmPopup 标题自查

- `Dialog : ConfirmPopup`——标题文本的承载 Label（若有）同型排查；无则豁免。**请引擎自查确认**（次要）。

### 3.5 ColorPicker 关闭态（尊重现状）

- "closed-text-color 单色、无态语义"——**接受不纳入**（色槽 hover 时 hex 变色非当前需求）；如未来需要列 backlog。

## 4. 验收补充

- **P0-7-7 WinFrame（若纳入）**：`wframe->setState(Hover/Pressed/Disabled/Normal)` → title Label state 逐一相符；标题重设后同步当前态。
- **P0-7-8 ImageButton**：工厂创建的图片按钮状态联动（按钮本体断言即可，确认无独立类）。
- 设计器联测（实施后随批）：win-frame / check-box / button / image-button 四类的 text/textShadow 各态色槽改色 → 对应态（hover/pressed/disabled）视觉跟随。

## 5. 放行

**结论：放行实施**（按 §3 扩展后的范围）。实施完成后同步 subModules，设计器随批：
1. 全类型色槽各态联测（hover 悬停 / pressed 按住 / disabled 取消启用）；
2. 清理颜色组相关全部诊断日志（已清）；
3. 颜色组补全工作闭环，更新 05/history 文档。
