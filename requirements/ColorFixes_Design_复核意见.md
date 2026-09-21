# ColorFixes_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-20
- 对象：`design/ColorFixes_Design.md`（状态：待评审）
- 结论：**通过，放行实施**（附 1 项**根因链补充**与 2 项确认，见 §3——涉及验收场景覆盖）

## 1. 分项评审

### P0-5（caption-label 暴露 + 键一致性合并）✓

- **通用 GetPtr 链**（用户已定）：GetPtr/FromHandle 两端既有、零新增接口 ✓。
- **单一常量 `kCaptionLabel`**（删 `kJsonCaptionLabel`）：属性键 == JSON 布局键，延续 92 键一致性原则 ✓；LayoutParser 3 处引用同批替换 ✓。
- **附带发现的 camelCase 死分支**（LayoutParser.cpp:849 `contains("captionLabel")` 永不命中 → `caption-label` JSON 配置从未生效）：**同一致性 violation，随批修复正确** ✓。
- `getPtrProperty("caption-label")` 分发返回 `m_caption.get()`——与 C ABI 句柄同构、validateControl 经 Button 子树 ✓。
- **P0-5-5（8 个单色 setter 保持现状）**：确认——应用从 GetStateColor 读改写改直控 caption（引擎落地后设计器简化）。

### P0-6（createClosedStateControls 幂等化）✓

- **根因链源码级成立**（pre-create 属性应用路径 `recreateClosedState()` 建 pair-A → `create()` 无条件再建 pair-B 未清理 → 指针指 B、A 残留渲染恒显 JSON 初始色）——**与设计器观察完全吻合**（蓝 #4A90D9FF = JSON 初始色恒显 + 浅色 = 当前色 = pair-B）。
- **幂等方案**（createClosedStateControls 先清理后创建）：覆盖所有调用路径 ✓；`recreateClosedState()` 的 remove 变冗余（无害）✓；备选 m_isCreated 守卫只覆盖单路径——**确认幂等方案**。
- **附带发现的 camelCase 死分支**：见 P0-5 段。

### 同型排查表 ✓

Button（本批暴露）/ CheckBox（按需）/ ComboBox·NUD（无子 Label 无需）/ WinFrame（已有专有同步）——分类正确。

## 2. 对 §7 待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 通用 GetPtr 链（用户已定） | **确认**（复述一致） |
| 2 | kJsonCaptionLabel 合并 + camelCase 修复 | **确认** |
| 3 | CheckBox 同型暴露：本批不做（按需）或随批 | **本批不做（按需）**——设计器动态区 bool 行用 CheckBox 自带 caption，无独立颜色控制需求 |
| 4 | P0-6 幂等方案（先清理后创建） | **确认**（覆盖全路径，优于守卫备选） |
| 5 | appearance JSON 是否含 closed-* 键 | **已核（见 §3 补充）**——设计器 prop_color **无 closed-* 键、但有 `font` 键** |
| 6 | 验证载体（内部测试） | **确认** + 设计器联测（颜色组场景）随批 |
| 7 | caption-label JSON 恢复生效的布局影响 | **设计器 main_layout.json 无 caption-label 键**（grep=0）——无影响 ✓ |

## 3. 根因链补充：设计器场景的触发器是 `font` 键（非 closed-*）

**核对结果**：设计器 `prop_color` JSON = `{type, id, color:"#4A90D9", rect, font:{name, size:16}}`——**无任何 closed-* 键**。

**推论**：§2 根因链的"parse 应用 closed-* 属性 → recreateClosedState 建 pair-A"**在设计器场景不成立**（无触发器）——但设计器实测双 hex 重叠确凿。

**最可能的补充触发器**：**`font` 键解析**——`setFontSize(16)` 若经 `setClosedFontSize` 路径触发 `recreateClosedState()`（引擎设计 §3.2 自述"recreateClosedState（size/字号变更时）"），则 **`font` 键即 pair-A 触发器**——触发链闭合，且解释了"含 font 的 color-picker JSON 普遍复现"。

**请引擎确认**：`LayoutParser` 解析 color-picker 的 `font` 键时是否调用 `setFontSize`→`setClosedFontSize`→`recreateClosedState` 链（或 ColorPicker 是否 override `setFontSize`）。

**验收场景补充**（放行条件）：P0-6-1 的测试 JSON 请**增加设计器同构用例**——`{color, font:{name,size}}`（无 closed-*）→ 修复后 `getChildren()` 中关闭态控件对数 == 1。若该场景修复前即复现双（经 font 触发）→ 根因链完整闭合。

## 4. 其余确认

- P0-5-6（键一致性验收：全库无 kJsonCaptionLabel 残留 + LayoutParser 解析生效）✓。
- §6 行为变化（caption-label JSON 恢复生效）：设计器布局无该键，无影响 ✓。
- §8 配套（手册/API_Mapping_Table/测试/归档）✓。

## 5. 放行

**结论：放行实施**（附 §3 根因链补充确认与设计器同构验收用例）。实施完成后同步 subModules，设计器随批：
1. **回收占位三件套**（P0-4 终态）——行内直建 ColorPicker；
2. **text/textShadow 色槽改直控 caption**（`GetPtr("caption-label")` → FromHandle → SetColor/SetStateColor），删除 GetStateColor 读改写绕行；
3. 联测：颜色组 15 槽秒建、双 hex 重叠消失（设计器同构场景）、caption 直控（颜色/字体）。
