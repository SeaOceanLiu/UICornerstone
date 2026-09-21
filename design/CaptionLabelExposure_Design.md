# CaptionLabelExposure_Design — CheckBox/WinFrame 统一 `caption-label` 句柄暴露

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md` §P0 追加（2026-09-21，P0-8：各态联测发现）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/CaptionLabelExposure_Design_复核意见.md`，2026-09-21 通过 + **方案修正**）
> **方案修正（采纳）**：WinFrame **不新增 `caption-label` 别名**——直接使用既有 `title-label` 键（WinFrame.cpp:485-491 已暴露、映射表:344 已文档化）。理由：92 键一致性（WinFrame 布局/属性语义键为 `title`，`caption-label` 为其发明的新键、无 JSON 对应）；语义准确（caption≠title）；应用零成本（回调按键型分派：win-frame → `title-label`，其余 → `caption-label`）。§3.1 的 WinFrame 别名分支与 §6 别名说明已删除。
> **实施结果**：CheckBox `getPtrProperty(kCaptionLabel)` 已实施（Control* 约定）；WinFrame 零改动（`title-label` 既有键回归验证）；测试与文档按修正后口径落地。
> 前置：`ColorFixes_Design`（P0-5：Button `getPtrProperty("caption-label")`，句柄约定 Control*）+ `ButtonCaptionState_Design`（P0-7：三控件状态联动已实施）——**仅差 CheckBox/WinFrame 句柄暴露即可全链生效**

---

## 1. 问题与目标

| # | 现象 | 目标 |
|---|---|---|
| P0-8 | 设计器统一经 `GetPtr("caption-label")` 直控内部文本各态色：**Button 生效**；**CheckBox / WinFrame 失败**（GetPtr 返回 0）→ 应用兜底写自身字段（视觉无效果） | CheckBox / WinFrame 暴露 `caption-label` 句柄（统一键，应用无感知类型差异）；WinFrame 保留既有专有键（向后兼容） |

## 2. 现状核实（源码事实）

| 控件 | 内部文本 | `getPtrProperty` 现状 | 缺口 |
|---|---|---|---|
| Button | `m_caption`（shared_ptr\<Label\>） | ✅ `kCaptionLabel` → m_caption（P0-5 实施，Control* 约定） | — |
| **CheckBox** | `m_caption`（CheckBox.h:59；**已挂树**：CheckBox.cpp:163 `addControl(m_caption)`） | ❌ 无 override（基类 `ControlImpl::getPtrProperty` 恒返回 0，ControlBase.cpp:990-992） | **`kCaptionLabel` 分发** |
| **WinFrame** | `m_titleLabel`（WinFrame.h:27；已挂树：m_titleBar 子控件） | ⚠️ 已有 override（WinFrame.cpp:485-491）：`title-bar` / **`title-label`** / `close-button` / `client-panel`（Control* 约定 ✓） | **缺 `caption-label` 统一别名**（设计器统一键；`title-label` 保留兼容） |
| 其它含内部文本子控件 | ComboBox（list-panel 已暴露，无 caption 概念）/ ColorPicker 关闭态（专有口径，批复不纳入）/ NUD·ComboBox 自绘 | — | 无（P0-5 排查表结论沿用） |

关键前提（已核）：
- `kCaptionLabel = "caption-label"` 常量已存在（PropertyNames.h，P0-5 建）；键一致性（属性键 == JSON 键）满足。
- **句柄约定 `static_cast<Control*>`**（ControlImpl 虚继承 Control；与 ComboBox/WinFrame 既有 getPtrProperty 一致）——P0-5 实施踩坑已验证。
- `validateControl`：CheckBox caption（CheckBox 子树）/ WinFrame titleLabel（titleBar 子树）**均在控件树中** → Debug 归属校验通过。

## 3. 架构选择与关键设计决策

### 3.1 修复（决策：CheckBox 新增分发 + WinFrame 统一别名）

```cpp
// CheckBox.h
int getPtrProperty(const char* prop, void*& out) override;
// CheckBox.cpp
int CheckBox::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kCaptionLabel) == 0) {
        // 句柄约定：Control* 基地址（虚继承偏移修正，与 Button/ComboBox 一致）
        out = m_caption ? static_cast<Control*>(m_caption.get()) : nullptr;
        return m_caption ? 1 : 0;
    }
    return ControlImpl::getPtrProperty(prop, out);
}

// WinFrame：**不改**（方案修正）——既有 getPtrProperty（WinFrame.cpp:485-491）
// 已暴露 title-bar / title-label / close-button / client-panel（Control* 约定）；
// 设计器回调按键型分派：win-frame → "title-label"，其余 → "caption-label"。
```

决策点：
- **CheckBox 用 `caption-label`；WinFrame 用既有 `title-label`**（方案修正）：不为 WinFrame 发明新键（92 键一致性）；设计器按键型分派，零引擎新增。
- **CheckBox 仅加 `caption-label`**（P0-8 请求范围）；不新增其它句柄（保持最小暴露）。
- **空 caption 守卫**：返回 0 不崩（CheckBox 空文本时 caption 仍存在（createCaption 恒建）；WinFrame titleLabel 恒建）。
- 状态联动（P0-7）已覆盖两控件 → 句柄暴露后**全链生效**（各态色设置 → 状态可达 → 视觉跟随）。
- **不改 button**（已就绪）；**不改 ColorPicker**（批复不纳入）。

## 4. API 设计

- 核心库：CheckBox.h/.cpp（+1 override）、WinFrame.cpp（+1 别名分支）；**无新常量**（kCaptionLabel 已存在）。
- C ABI / Binding：**零新增**（既有 `UICornerstone_GetPtr` / `Control::GetPtr`）——与 P0-5 同链。
- 设计器侧：无代码改动（统一 `GetPtr("caption-label")` 即可）。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| P0-8-1 CheckBox 句柄 | 内部：`cb->getPtrProperty(kCaptionLabel, out)` == `static_cast<Control*>(cb->getCaption().get())`；句柄直控 `setTextNormalStateColor` 往返 |
| P0-8-2 WinFrame 既有键回归 | 内部：`wf->getPtrProperty(kTitleLabel, out)` == titleLabel（Control*）——既有键可用性保持（**不新增别名**，方案修正） |
| P0-8-3 C ABI 端到端 | `test_p0_getter` 扩展：`CreateCheckBox` / `CreateWinFrame` → `UICornerstone_GetPtr(inst, ctl, "caption-label", &cap)`（非空）→ `SetColor(cap,"text",red)` → `GetColor` 往返 |
| P0-8-4 全链（状态+句柄） | CheckBox：句柄设 `text.hover` → `setState(Hover)` 字段命中；**WinFrame（经 `title-label`）**同验一遍（状态联动 P0-7 已实施） |
| P0-8-5 非目标/空 | Label 等无该键返回 0（既有断言保留）；空 caption 场景返回 0 不崩 |
| P0-8-6 回归 | test_colorfixes / test_p0_getter / test_checkbox / test_winframe / test_button 全绿 |

## 6. 影响面与风险

- 纯新增查询能力（CheckBox 新键 + WinFrame 别名），无行为变化；两控件既有属性/状态/视觉不变。
- 句柄生命周期：借用语义（与 P0-5 一致）——caption/titleLabel 随控件生命周期；应用不应缓存跨销毁使用（文档既有说明）。

## 7. 待审核问题

1. **WinFrame 键策略**：~~统一别名~~ → **已裁定（复核修正）**：用既有 `title-label`，不新增别名。
2. **CheckBox 暴露范围**：仅 `caption-label`（P0-8 请求）——确认无需其它句柄（如 checkbox 框体无关）？
3. **验收载体**：test_colorfixes（内部）+ test_p0_getter（C ABI）扩展——确认？
4. **文档配套**：checkbox.html / winframe.html（caption-label 直控说明 + 别名注明）/ properties.html（CheckBox·WinFrame 段补行）/ API_Mapping_Table（WinFrame 行加别名、CheckBox 行新增）——确认范围？

## 8. 待提交配套改动（实施时随批）

- 测试扩展（§5）+ 全量回归；设计文档状态标注；复核意见归档。
- 手册四件（§7-4）；make_release 同步（设计器随批完成颜色组全类型直控联测）。