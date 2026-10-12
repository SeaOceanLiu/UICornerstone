# P085_ComboBoxTextInput_Design — 非编辑 ComboBox 无条件吞 TextInput（P0-84 根因定案）

> 提出方：CornerstoneDesigner（UI 设计器）——P0-84 复测更新（2026-10-10：a) 校验抓到设计器手柄越界并已修，但画布内联输入仍失效，请求引擎插桩/最小拓扑复现）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-10 追加更新）
> 前置：P0-83/P0-84 批已实施并同步
> 状态：**已放行，已实施**（2026-10-10；Q-1~Q-4 全部确认）
> 复核：`requirements/P085_ComboBoxTextInput_Design_复核意见.md`（通过；设计器侧逐条证据对齐——画布行组序 id/标题(EditBox) → 缩放模式(ComboBox) 在后=吞；控件行字体 ComboBox 在 id 行前=正常；对话框无 ComboBox=正常）
> 实施备注：① `ComboBox::handleEvent` 非编辑 TextInput 消费改 **`return getFocused();`**（未聚焦放行链上其它控件；自身聚焦仍拦截防只读显示文本被写）；② 审计确认仅 ComboBox 需修（NumericUpDown readOnly/EditBox/TextArea/MenuBar 均已有门控）；③ 验证：**P0-85 探针 5/5**——1) EditBox 先加+Combo 后加（原复现）修复后 `'X'` ✓；2) 顺序反转 ✓；3) Combo 聚焦仍吞 ✓；4) [combo, editbox, combo] 全排列 ✓；5) Combo→EditBox 焦点迁移 ✓；④ 全量构建 0 错误、回归仅 4 项预存；⑤ **P0-84 全链结案**：本项修复 + a/b/c 硬化保留（`Debug_ValidateHandle` 长期自检）；设计器恢复画布行内联直编并复测

---

## 1. 根因定案（引擎复现，与设计器全部证据吻合）

**根因**：`ComboBox::handleEvent` 对**非编辑模式**的 TextInput **无条件消费**（不检查是否聚焦/事件归属）：
```cpp
// src/ComboBox.cpp:157-160
if (!m_editable && event->m_type == EventType::TextInput) {
    return true;   // ← 无条件吞；m_editable 缺省 false（include/ComboBox.h:64）
}
```
`ControlImpl::handleEvent` 逆序遍历子控件（后添加先见事件，src/ControlBase.cpp:323-338）；TextInput 无坐标、无遮挡判定 → **同容器内任何位于聚焦 EditBox 之后的默认（非编辑）ComboBox 都会先吞掉 TextInput** → 聚焦 EditBox 收不到（光标/焦点样式正常，仅无字符）✗。

**探针证据（Temp/temp_p085_probe，本机当前构建）**：
| 用例 | 结果 |
|---|---|
| 1) EditBox 先加、非编辑 ComboBox 后加（逆序：Combo 先见） | 输入被吞 `''` **FAIL（复现）** |
| 2) ComboBox 先加、EditBox 后加（逆序：EditBox 先见） | `'X'` OK |
| 3) ComboBox 自身聚焦 | 仍吞 `''` OK（原意图：防只读显示文本被写） |

**与设计器证据全链吻合**：
- "唯一变量=选中态（控件行 ✓ / 画布行 ✗）"→ **行集子控件顺序不同**：画布行集含默认 ComboBox 且排在 id 行 EditBox 之后（控件行不含或排在之前）；
- "同容器/同 API/同回调"→ 完全一致（差异只在**顺序**）；
- "池复用传染"→ 行重建改变 child 顺序，ComboBox 落到 EditBox 之后即吞输入；
- p084c 合法拓扑探针（TabControl 页/树/池，**无 ComboBox**）全过 → 与 TabControl/树无关 ✓；
- a) 校验抓到的手柄越界为真实另一问题（已由设计器修复），非本项根因 ✓。

## 2. 修改方案
- `ComboBox::handleEvent` 非编辑 TextInput 分支改为**聚焦门控**：
  ```cpp
  if (!m_editable && event->m_type == EventType::TextInput) {
      return getFocused();   // 仅自身聚焦时消费（防只读显示文本被写）；未聚焦放行链上其它控件
  }
  ```
- 审计同类"未聚焦却消费输入"分支：`NumericUpDown` readOnly 对 TextInput 返回 false ✓、`EditBox/TextArea` 均查 `m_focused` ✓、`MenuBar` watcher 仅 KeyDown/MouseDown ✓——**仅 ComboBox 需要修**（Q-2 复核确认）。
- 不改：编辑模式 ComboBox（转发 EditBox，自身聚焦语义不变）、点击/下拉/收起行为。

## 3. 测试计划
- 探针（p085 回归）：用例 1 修复后 `'X'`；用例 2/3 保持；
- 扩展矩阵：两个默认 ComboBox 与 EditBox 多顺序组合；嵌套面板（ComboBox 在外层面板）；ComboBox 聚焦后点 EditBox 再输入（焦点迁移）；
- 回归：test_combobox_cabi / test_property_cabi / 全量（仅 4 项预存）；
- 设计器联测：画布行内联 id/标题恢复直编（设计器恢复"直编"过渡），控件行不受影响；`DebugValidateHandle` 自检保留。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 修法：非编辑 TextInput 消费改 `return getFocused()` | 确认 |
| Q-2 | 同类审计范围：仅 ComboBox（其余均已有聚焦/只读门控） | 确认 |
| Q-3 | 测试矩阵（顺序组合/嵌套/焦点迁移）是否足够 | 确认；设计器侧补"画布行直编恢复"复测 |
| Q-4 | P0-84 收尾：本项修复后 a/b/c 硬化保留（`Debug_ValidateHandle` 作为长期自检），P0-84 结案 | 确认 |
