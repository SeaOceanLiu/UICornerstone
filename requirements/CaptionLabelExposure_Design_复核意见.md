# CaptionLabelExposure_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-21
- 对象：`design/CaptionLabelExposure_Design.md`（状态：待评审）
- 结论：**通过，放行实施**（**附 1 项方案修正**：WinFrame 采用既有 `title-label`、**不新增 `caption-label` 别名**——见 §2）

## 1. 分项评审

### CheckBox 部分（§2/§3.1/§5）✓ 全部认可

- `m_caption` 已挂树（CheckBox.cpp:163）→ validateControl 归属校验通过 ✓。
- `getPtrProperty(kCaptionLabel)` → `static_cast<Control*>`（虚继承偏移修正约定，与 Button/ComboBox 一致）✓。
- 空 caption 守卫（返回 0 不崩）✓；验收 P0-8-1/3/4/5 覆盖完整 ✓。

### 句柄约定/生命周期（§2 前提、§6）✓

借用语义、随控件生命周期、不跨销毁缓存——与 P0-5 一致 ✓。

## 2. 方案修正：WinFrame 键策略（对 §7-1 的答复）

**结论：采纳"WinFrame 直接用 `title-label`、不新增 `caption-label` 别名"**（用户质疑，我方复核同意）。设计文档 §3.1 的 WinFrame 别名分支与 §6 别名说明**删除**。

理由：

| 维度 | caption-label 别名 | title-label（采纳） |
|---|---|---|
| **92 键一致性原则** | WinFrame 的布局/属性语义键是 `title`（标题）——`caption-label` 是**为其发明的新键**（无 JSON 对应） | 既有键（WinFrame.cpp:485-491 已暴露，映射表:344 已文档化）✓ |
| **语义** | caption（字幕/说明）≠ title（标题） | 准确 |
| **应用成本** | 统一键（但应用本就知类型） | 设计器回调按类型分派（一行三元：win-frame → `title-label`，其余 → `caption-label`）——**零成本** |
| **双键同指** | 需文档注明别名关系（§6 自述"避免困惑"） | 无别名歧义 |

**设计器侧对应**：回调键按控件类型选择（`m->props.type == "win-frame" ? "title-label" : "caption-label"`），无需引擎加键。

## 3. 对 §7 其余待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | WinFrame 键策略 | **修正采纳**：用 `title-label`，不加别名（§2） |
| 2 | CheckBox 仅 `caption-label` | **确认** |
| 3 | 验收载体（test_colorfixes + test_p0_getter） | **确认** |
| 4 | 文档配套 | **确认**（按修正后口径：winframe.html 说明经 `title-label` 直控；**无需**别名注明；映射表 WinFrame 行不加别名） |

## 4. 验收补充

- WinFrame 相关断言简化为**既有键回归**（`GetPtr("title-label")` 可用性保持，P0-8-2 保留）+ CheckBox 新键（P0-8-1/3）为准；P0-8-4 全链（状态+句柄）对 **CheckBox** 与 **WinFrame（经 title-label）** 各验一遍。
- 设计器联测随批：CheckBox / WinFrame 的 `text.*`/`textShadow.*` 各态色槽改色 → hover 悬停 / pressed 按住 / disabled 取消启用 → 视觉跟随（Button 已验）。

## 5. 放行

**结论：放行实施**（CheckBox 新增分发 + WinFrame 按修正后不加别名）。实施完成后同步 subModules，设计器随批：
1. 回调键按类型分派（win-frame 用 title-label）；
2. 全类型色槽各态联测（Button/CheckBox/WinFrame）；
3. 颜色组补全闭环（05/history 文档更新）。
