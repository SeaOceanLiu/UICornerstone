# ColorFixes_Design — Button 内部 caption Label 暴露 + ColorPicker 关闭态双构建修复

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md` §P0 追加（2026-09-20）——**P0-5**（方案改定：暴露内部 caption Label 直控接口）与 **P0-6**（截图实锤：LoadLayout 的 ColorPicker 关闭态双 hex 重叠）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/ColorFixes_Design_复核意见.md`，2026-09-20 通过）
> **实施结果（2026-09-20）**：
> 1. **P0-6 根因实证（放行条件闭合）**：设计器的 `font` 触发假设经代码链核验**否定**（`ApplyFontToControl` 无 ColorPicker 分支；`parseCommonProperties` 的 font 路径不触达关闭态）；**真实触发器** = `ControlImpl::setContext` 级联对每个子控件 **recreate 两次**（`child->setContext()` 内部自我 recreate + 父循环再 `impl->recreate()`），而 `recreate()` 先重置 `m_isCreated=false` 再调 `create()`，绕过派生类守卫 → 非幂等的 `createClosedStateControls()` 双建。实证：设计器同构用例（font、无 closed-*）`children=4`、closed-* 变体 `children=6`；幂等修复后均为 **2**。
> 2. **P0-5 实施约定（重要）**：`ControlImpl` 虚继承 `Control` —— 句柄必须存 **`static_cast<Control*>` 基地址**（与 ComboBox::getPtrProperty 既有约定一致）；直接存 `Label*` 最派生地址会在句柄还原后虚调用崩溃（本批实施中已踩坑并修正）。
> 3. 测试：新增 `test_colorfixes`（内部：双用例 children==2 + caption 句柄断言）；`test_p0_getter` 补 C ABI 端到端（GetPtr→SetColor→GetColor 往返 + 非 Button 返回 0）；关键回归全绿。
> 方案变更记录：
> 1. P0-5 原方案"补 8 个单色 setter 同步 caption"经设计器复核**改定为"应用直控内部 Label"**（引擎零行为变化、应用获 caption 全族属性控制）；
> 2. **用户已定**：使用通用 GetPtr 链（不做专用 C ABI）；**属性 Key 必须与 JSON 布局 Key 相同**（延续 92 键一致性修改）——据此 `kJsonCaptionLabel` 与新增运行时键**合并为单一常量 `kCaptionLabel`**，并随批修复 LayoutParser 的一处 camelCase 残留（§3.1）。

---

## 1. 问题与目标

| # | 现象 | 目标 |
|---|---|---|
| P0-5 | `SetColor("text-shadow", c)` / `SetColor("text", c)` 写 Button 视觉无变化（文本由内部 caption Label 绘制） | 暴露 caption Label 句柄（`getPtrProperty("caption-label")` + 既有 GetPtr/FromHandle 链），应用经句柄直控 Label 标准属性；**键名与 JSON 布局键一致（单一常量）**；引擎零行为变化 |
| P0-6 | LoadLayout 创建的 ColorPicker 关闭态双 hex 文本重叠（恒定蓝 = JSON 初始色 + 浅色 = 当前色） | 关闭态控件唯一（幂等构建），任何创建路径均只一对 |

## 2. 现状核实（源码事实）

### P0-5（新方案可行性 + 键一致性）

| 机制 | 位置 | 状态 |
|---|---|---|
| C++ 获取入口 | `Button::getCaptionLabel()`（Button.h:55，返回 `shared_ptr<Label>`） | ✅ 已存在 |
| 属性系统读句柄 | `ControlImpl::getPtrProperty`（ControlBase.cpp:990-992）= **基类未处理任何键**；Button 无 `getPtrProperty` override | **缺口（本需求）** |
| C ABI 读取 | `UICornerstone_GetPtr(instance, ctl, prop, void** out)` → `getPtrProperty` | ✅ 已存在（无需新增——用户已定） |
| Binding 读取 | `Control::GetPtr(const char* prop)`（binding/Control.h:47 + Control.cpp:94 → fnGetPtr）；`UICornerstone::FromHandle(h)` | ✅ 已存在（无需新增） |
| 句柄归属校验 | `validateControl`（UICornerstoneAPI.cpp:113+）：Debug 沿 parent 链上溯 + `treeContains`——caption Label 是 Button 子控件，上溯经 Button 命中树 | ✅ 通过（Release 直通） |
| Label 标准属性直控 | Label 自绘（自身状态色/字体字段），无转发层 | ✅ 直控即生效 |
| **键一致性** | `kJsonCaptionLabel = "caption-label"`（PropertyNames.h:482，JSON 解析键，**无运行时对应**）；schema button def 已公开 `caption-label`（object，"自定义标题 Label 描述"） | **合并为 `kCaptionLabel` 单一常量（用户已定原则）** |
| **LayoutParser 残留 bug** | LayoutParser.cpp:849：`j.contains("captionLabel")`（**camelCase 字面量**）&& `j[kJsonCaptionLabel]`（kebab）——92 键迁移后该分支**完全不可达**（camelCase 不命中；kebab 键访问时空对象 is_object 假）→ `caption-label` JSON 配置从未生效 | **随批修复**（同键同常量；分支恢复生效——行为变化见 §6） |
| Button 单色 setter | `ControlImpl::setColorProperty` 8 单色键 → 基类字段（不同步 caption） | **保持现状**（设计器接受零行为变化；应用改走 caption 直控） |

**同型排查**（含内部文本子控件的控件）：

| 控件 | 内部文本 | 暴露现状 | 结论 |
|---|---|---|---|
| Button | caption Label（子控件） | C++ `getCaptionLabel()`；无属性/ABI 读句柄 | **本批暴露**（P0-5） |
| CheckBox | caption Label（子控件） | C++ `getCaption()`；无属性/ABI | **按需**（设计器定；同型补 `getPtrProperty("caption-label")` 即对齐） |
| ComboBox / NUD | 无子 Label（继承 EditBox，自身绘制文本） | 标准文本属性直接生效 | ✅ 无需暴露 |
| WinFrame | title Label | 专有 `setTitleTextColor` 同步已存在 | ✅ 无需改 |

### P0-6

| 机制 | 位置 | 状态 |
|---|---|---|
| 关闭态构建 | `ColorPicker::createClosedStateControls()`（ColorPicker.cpp:157-183）：直接 `addControl(...)`——**非幂等**（不清理旧实例） | **缺陷点** |
| 重建路径 | `recreateClosedState()`（:185-193）：先 remove 旧对再调 `createClosedStateControls()` | 自身幂等 ✓ |
| 三 setter 触发重建 | `ColorPicker.h:118-120`：`setClosedSwatchSize/setClosedFontSize/setClosedTextColor` 内联调 `recreateClosedState()` | 属性应用即重建 |
| LayoutParser 顺序 | `parseColorPicker`（LayoutParser.cpp:1432+）：`setColor`（JSON 初始色）→ closed-* setters（若有）→ … → **`cp->create()`** | **pre-create 属性应用 → 先建一对；create() 再建一对 → 双** |

**根因链**：
1. parse 应用 closed-* 属性（均在 create 之前）→ `recreateClosedState()` 建 **pair-A**（caption = 当时 m_color = JSON 初始色）；
2. 随后 `cp->create()`（ColorPicker.cpp:91）**无条件再调** `createClosedStateControls()` → 建 **pair-B**（未清理 A），指针指向 B；
3. `syncUIFromColor()` 只更新 B；**pair-A 残留渲染**（恒显 JSON 初始色）→ 双 hex 重叠。

**时间相关性说明**：根因链与惰性化批**无因果**（parse 顺序与非幂等构建均先于该批）；此前未观察到的可能原因：外观区 JSON 近日才引入 closed-* 键（颜色组补全同期）或 LoadLayout 路径近期启用。**请设计器核对 appearance JSON**。

## 3. 架构选择与关键设计决策

### 3.1 P0-5：暴露 caption Label 句柄 + 键一致性合并（决策：通用 GetPtr 链 + 单一常量 `kCaptionLabel`）

```cpp
// PropertyNames.h —— 键一致性（用户已定原则）：合并单一常量
//   删除：kJsonCaptionLabel = "caption-label"（JSON 专用，无运行时对应——不再保留）
//   新增（属性键区，kCaption 附近）：JSON 解析与运行时属性共用
PROP_CONSTEXPR const char* kCaptionLabel = "caption-label";

// Button.h（Property system overrides 区）
int getPtrProperty(const char* prop, void*& out) override;

// Button.cpp
int Button::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kCaptionLabel) == 0) {
        out = static_cast<void*>(m_caption.get());   // Label* → 句柄（与 C ABI 句柄同构）
        return m_caption ? 1 : 0;
    }
    return ControlImpl::getPtrProperty(prop, out);
}

// LayoutParser.cpp:849-852 —— 同键同常量 + 修复 camelCase 残留
if (j.contains(PropertyNames::kCaptionLabel) && j[PropertyNames::kCaptionLabel].is_object()) {
    pushJsonPath(PropertyNames::kCaptionLabel);
    const json& cl = j[PropertyNames::kCaptionLabel];
    ...
```

应用侧使用链（零新增 C ABI / 零新增 Binding）：
```c
void* cap = NULL;
UICornerstone_GetPtr(inst, btn, "caption-label", &cap);   // 既有 API
UICornerstone_SetColor(inst, cap, "text", color);          // Label 标准属性直控
```
```cpp
Control cap = ui->FromHandle(btn.GetPtr(PropertyNames::kCaptionLabel));
cap.SetColor(PropertyNames::kText, color);
```

决策点：
- **通用 GetPtr 链**（用户已定）：GetPtr + FromHandle 两端既有，零新增接口；不做专用 `UICornerstone_GetCaptionLabel`。
- **单一常量 `kCaptionLabel`**（用户已定原则）：属性键 == JSON 布局键 == 同一常量；`kJsonCaptionLabel` 删除（LayoutParser 3 处引用改为 `kCaptionLabel`）。嵌套配置键（`kJsonStyle`/`kJsonAlignment` 等）是 caption-label 对象**内部**的配置语义键（与运行时属性键不同字符串），保持不动。
- **LayoutParser camelCase 残留随批修复**（:849 `contains("captionLabel")` → `contains(kCaptionLabel)`）：属同一致性 violation；修复后 `"caption-label": {...}` JSON 配置**恢复生效**（此前分支死代码，见 §6 行为变化）。
- **引擎零行为变化**：8 个单色 setter 维持现状；应用从"GetStateColor 读改写绕行"改为直控 caption。
- **validateControl 兼容**：caption Label 在 Button 子树中（§2 已核）。
- **CheckBox 按需**（设计器定；同型补法已列）。

### 3.2 P0-6：`createClosedStateControls()` 幂等化（决策：先清理后创建）

```cpp
void ColorPicker::createClosedStateControls() {
    // 幂等：清理可能存在的旧实例（pre-create 属性应用路径已建过一对时，避免双构建）
    if (m_closedSwatch) { removeControl(m_closedSwatch); m_closedSwatch.reset(); }
    if (m_closedLabel)  { removeControl(m_closedLabel);  m_closedLabel.reset(); }
    // …原创建逻辑不变
}
```

- 幂等覆盖所有调用路径；`recreateClosedState()` 既有 remove 变冗余（无害，不动）；`create()` 无需改。
- 备选（不做）：`m_isCreated` 守卫——只覆盖单路径且改变既有语义。
- 与弹窗惰性化路径独立，无交互。

## 4. API 设计

- `PropertyNames.h`：`kCaptionLabel = "caption-label"` 新增；`kJsonCaptionLabel` 删除（-1/+1）。
- `Button.h/.cpp`：`getPtrProperty` override（各 1 处）。
- `LayoutParser.cpp`：3 处常量替换 + 1 处 camelCase 修复。
- 核心 / C ABI / Binding：**零新增接口**（GetPtr 链既有）。
- `ColorPicker.cpp`：`createClosedStateControls()` 幂等化（+3 行）。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| P0-5-1 C++ 直取 | `Button::getPtrProperty("caption-label")` 返回非空且 == `getCaptionLabel().get()` |
| P0-5-2 C ABI 端到端 | `CreateButton` → `GetPtr("caption-label")` 得句柄 → `SetColor(cap, "text", red)` → caption 状态色变红；`SetStateColor`/字体/对齐抽验 |
| P0-5-3 Binding 链 | `FromHandle(btn.GetPtr("caption-label"))` → `SetColor` 生效（冒烟） |
| P0-5-4 空/非 Button | 无 caption 返回 0；非 Button `GetPtr("caption-label")` 返回 0（不崩） |
| P0-5-5 引擎零行为 | `SetColor("text")` 直写 Button 仍无视觉变化（既有语义保持）；既有 Button/CheckBox 色测试全绿 |
| **P0-5-6 键一致性** | 编译期：全库无 `kJsonCaptionLabel` 残留；行为：LayoutParser 解析 `"caption-label": {...}`（含 caption/font/alignment）→ **配置生效**（自定义 caption Label 属性被应用）；同一常量同时服务运行时 `GetPtr` 与 JSON 解析 |
| P0-6-1 单对关闭态 | 内部测试：LayoutParser 解析含 closed-* 键的 color-picker JSON → `picker->getChildren().size()==2`（修复前为 4） |
| P0-6-2 无残留 | 解析后 `setColor(不同色)` → 子树内 Label 数量==1 且随色更新 |
| P0-6-3 回归 | test_colorpicker / test_layout / test_button 全绿；C ABI 直建路径不受影响 |

## 6. 影响面与风险

- P0-5：纯新增查询能力 + 一致性修复；**行为变化点**——`"caption-label": {...}` JSON 配置由"死分支（从未生效）"恢复为"生效"（属修复既有意图，schema 早已公开该键）；如有布局文件曾依赖"该键无效"，需核对（设计器确认）。
- P0-6：pre-create 第一对被 create() 替换（生命周期正确）；无副作用。
- 无 ABI 契约破坏；三后端无涉。

## 7. 待审核问题（原 Q1/Q2 已由用户裁定，余下交设计器）

| # | 问题 | 状态 |
|---|---|---|
| 1 | 通用 GetPtr 链 vs 专用 C ABI | **已定（用户）**：通用链，不做专用 C ABI |
| 2 | `kJsonCaptionLabel` 合并为 `kCaptionLabel` 单一常量（同键同常量）+ LayoutParser camelCase 残留随批修复 | **已定（用户原则）**：属性 Key == JSON 布局 Key，延续 92 键一致性；实施按 §3.1 |
| 3 | **CheckBox 同型暴露**：本批不做（按需）或随批（`getPtrProperty("caption-label")` 对齐）？ | 待设计器定 |
| 4 | **P0-6 幂等方案**（vs `m_isCreated` 守卫备选） | 待设计器定 |
| 5 | **P0-6 触发条件核对**：appearance JSON 是否含 `swatch-size`/`closed-font-size`/`closed-text-color`？ | 待设计器核 |
| 6 | **验证载体**：内部测试（test_button/test_colorpicker C++ 断言 + Binding 冒烟） | 待设计器定 |
| 7 | **caption-label JSON 恢复生效**（§6 行为变化）：确认现有布局对该键的使用预期 | 待设计器核 |

## 8. 待提交配套改动（实施时随批）

- `design/API_Mapping_Table.md`：`setCaptionLabel | caption-label` 行补"读"方向（C ABI GetPtr / Binding GetPtr 可用）。
- 用户手册 Button 章节：补"经 `caption-label` 句柄直控内部 Label"用法（C ABI/Binding 示例）。
- 测试补充（§5）+ 全量回归；设计文档状态标注；复核意见归档。
- make_release 同步（设计器随后回收 text/textShadow 的 GetStateColor/SetStateColor 读改写绕行，改直控 caption）。