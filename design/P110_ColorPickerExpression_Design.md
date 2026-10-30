# P110_ColorPickerExpression_Design — ColorPicker 表达式模式（`{{...}}` 输入）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-29 追加：P0-110 高）
> 状态：**已放行，已实施（R2）**（2026-10-29 实施；2026-10-30 按交付复核 §9 明确提交/取消语义：编辑过程不写状态、取消丢弃未提交输入、回车=确定并关闭；探针 6/6 + UI 探针 6/6 + 全量回归基线保持）
> 复核意见：`requirements/引擎P108_P110设计评审.md`

---

## 1. 现状核实

| # | 事实 | 依据 |
|---|---|---|
| ① 文本域仅接受 hex | `onHexInputChanged` 过滤非 hex 字符，非法时不改色且 `syncUIFromColor()` **还原显示** ✗ | `src/ColorPicker.cpp:604-618` |
| ② `color` 只接收 hex | `setStringProperty("color")` → `setColor(hex)`，`fromHex` 失败静默忽略 | `src/ColorPicker.cpp:707-710, 521-528` |
| ③ 无表达式属性 | 属性分发无 `allow-expression` / `color-expression` ✗ | `src/ColorPicker.cpp:668-760` |
| ④ 事件 | `color-changed` 仅在 `onOK` 提交（用户取色）时触发 | `src/ColorPicker.cpp:635-643` |
| ⑤ **提交无事件不可用（阻塞）** | 表达式提交若不发事件，设计器无法获知提交时机 → 无法写回模板模型 | 评审 §3.2-1 |

## 2. 方案

### 2.1 新属性（运行时键 + JSON + schema）
| 键 | 类型 | 缺省 | 语义 |
|---|---|---|---|
| `allow-expression` | Bool | false | 开启后 hex 文本域**接受任意字符串**（设计器模板颜色槽置 1） |
| `color-expression` | String | "" | 当前表达式（如 `{{bgColor}}`）；空=无表达式；可读写 |

- `PropertyNames`：`kAllowExpression` / `kColorExpression` / **`kEventExpressionChanged="expression-changed"`** / 声明式键 **`kEventKeyExpressionChanged="onExpressionChanged"`**；
- `LayoutParser::parseColorPicker` 增两键解析；schema `$defs/color-picker` 增两键（`additionalProperties:false` 同步）+ events 允许 `expression-changed`（若 def 含事件列表）。

### 2.2 行为
1. `allow-expression=1` 时，`onHexInputChanged`：
   - 合法 hex → **清空表达式** + 正常应用颜色（用户取色路径）；
   - 非 hex（含 `{{...}}`）→ **保留原文**（不过滤/不还原）→ 存 `m_colorExpression`；**不改 `m_color`**；
2. `allow-expression=0`：保持现状（非 hex 过滤并还原）；**由 1→0 时立即清空表达式、恢复 hex 显示，但不发事件**（Q-3 + 复核 §8.3 选项 1：避免监听方在行池复用期间误清其他控件绑定；构造/程序化配置关闭一律静默）；
3. `syncUIFromColor`：表达式非空时**跳过**覆盖 hex 文本域与关闭态标签；关闭态标签显示表达式原文，swatch 显示回退色（Q-2）；
4. **提交路径**（确定 / 回车确认；输入框不实现失焦提交）：**编辑过程中的文本不写任何状态**（不更新 current/committed、不发事件）；仅提交时读取文本并更新状态。回车等价「确定」（提交 + 关闭弹窗；R2 明确）：
   - 表达式非空 → 更新 `m_committedExpression`，`m_color` 不变，**触发 `expression-changed`**（载荷=表达式原文，String）；**不触发 `color-changed`**；
   - 文本为空 → 按"无表达式"处理：清空表达式；**若原表达式非空，触发 `expression-changed("")`**（清宿主绑定，复核 §6.3-②）；走常规 hex/颜色提交（合法 hex 应用）+ `color-changed`（§3.3-5）；
5. **用户取色路径**（预设/滑块/合法 hex 输入/onOK 且 hex）：**一律清空表达式**（current + committed），并**触发 `expression-changed`（载荷=""）**告知宿主已清除；颜色路径照常触发 `color-changed`；
6. **程序化 `color` 设置（`SetString("color")`/`setColor`）不得清空表达式**（评审 §3.2-2）：仅更新回退色；表达式与 committed 表达式保留；关闭态仍显示表达式（设计器刷新回退色/重开不丢绑定）；
7. `onCancel` / ESC / 点击外部：**丢弃未提交输入**（编辑过程未写状态）→ 恢复 `m_color` 与 **`m_committedExpression`**，刷新文本域/标签显示（评审 §3.2-3；R2：不再存在"提交后仍可取消"的歧义，回车/确定即关闭弹窗）；
8. 外部 `SetString("color-expression")`：更新 current **与 committed**（非编辑态重开/取消后回显该值；评审 §3.2-4）；空串 → 恢复 hex 显示；
9. 事件语义汇总：
   - `color-changed`：**仅用户取色提交**（保持既有语义）；
   - `expression-changed`（新）：表达式**提交**（非空）与**被取色清空**（空串）时触发；载荷 `strVal`=表达式原文。

### 2.3 Binding
- `Control::SetCallback(PropertyNames::kEventExpressionChanged, ...)` 通用通路可用（引擎 fireCCallback **`CCallbackData::String`**，事件字段 `strVal`——复核 §6.3-①）；
- 可选便捷访问器（Q-6）：`Event::IsExpressionChanged()/GetExpression()`（读 `strVal`；与既有 IsTextChanged 同款）；
- 声明式：`events: { "onExpressionChanged": "handler" }` 经 `LayoutParser::parseEvents` 接线到 `ColorPicker::setOnExpressionChanged(handler)`（handler 收到空 `shared_ptr<Control>`，与 onColorChanged 同款；P0-110 实施补充）。

## 3. 验收与测试矩阵

1. `allow-expression=1`：输入 `{{bgColor}}` → 失焦/回车后保留显示；`GetString("color-expression")=="{{bgColor}}"`；`GetString("color")` 仍为回退色；**触发 `expression-changed`（载荷 `{{bgColor}}`）且无 `color-changed`**；
2. 重开弹窗 → 文本域回显 `{{bgColor}}`；关闭态标签显示表达式；
3. 选预设色/拖滑块/输入合法 hex → 表达式清空（**触发 `expression-changed` 载荷=""**）、`color` 更新、`color-changed` 触发；
4. `allow-expression=0`：输入非 hex → 过滤/还原（现状回归）；从 1→0 即时清空表达式、恢复 hex，**不发事件**（复核 §8.3 选项 1）；
5. 程序化 `SetString("color")`（刷新回退色）→ 表达式与 committed 保留、显示不变；随后 onCancel/重开仍回显表达式；
6. `onCancel`（编辑中输入未提交表达式）→ 恢复 committed 表达式与颜色显示；
7. 空文本 onOK（原表达式非空）→ `expression-changed("")` + 无表达式、常规颜色提交 + `color-changed`；
8. 外部 `SetString("color-expression")` → 当前+committed 更新、重开回显；
9. JSON：`"allow-expression": true, "color-expression": "{{bgColor}}"` 解析 + 导出 round-trip；schema strict PASS；
10. 全量回归仅 4 项预存。

## 4. 影响面与文档

- 源码：`include/ColorPicker.h`、`src/ColorPicker.cpp`、`include/PropertyNames.h`、`src/LayoutParser.cpp`、`docs/schema/declarative-ui.schema.json`；binding 便捷访问器（可选）；
- 文档：用户手册 colorpicker 页（表达式模式 + `expression-changed`）、properties 速查（两键 + 事件）；
- 设计器：模板颜色槽改回取色器输入；订阅 `expression-changed` 写模型；`fx` 保留快捷入口；普通颜色槽 `allow-expression=0` 反向回归。

## 5. 待确认（Q，含复核答复）

| # | 问题 | 结论 |
|---|---|---|
| Q-1 | `allow-expression` 缺省 false（仅模板槽开启） | **采纳** |
| Q-2 | 表达式非空时关闭态标签显示表达式字符串（swatch 显示回退色） | **采纳** |
| Q-3 | 关闭 allow-expression 时清空既有表达式 | **采纳**（立即生效） |
| Q-4 | 表达式提交不触发事件 | **不采纳（评审阻塞）** → **新增 `expression-changed`**（提交/清空时触发；`color-changed` 保持仅取色） |
| Q-5（复核） | 程序化 `color` 设置保留表达式 | **采纳**（仅用户取色路径清空） |
| Q-6（复核） | Binding 事件便捷访问器（可选） | 采纳（`IsExpressionChanged/GetExpression`） |
| Q-7（复核） | 空表达式提交语义 | 采纳（§2.2-4：按无表达式；原表达式非空时发 `expression-changed("")` + 常规颜色提交 + `color-changed`） |
| Q-8（复核） | allow-expression 1→0 是否发事件 | **采纳复核 §8.3 选项 1：程序化 1→0 静默清空，不发事件**（用户取色/清空文本仍发 `expression-changed("")`） |
| Q-9（复核） | 事件载荷类型 | **`CCallbackData::String`**（字段 `strVal`）；Binding 访问器按 `IsTextChanged/GetTextChanged` 同款 |


---

## 6. 交付复核修复（R2，2026-10-30）

- **提交点显式化**：`onHexInputChanged` 更名 `commitHexInput`（仅 `onOK`/回车确认调用）；编辑过程中的文本不写 `m_colorExpression`/`m_committedExpression`、不发事件。消除"逐键提交"歧义（复核 §9）。
- **取消语义**：`onCancel`/ESC/点击外部仅丢弃未提交输入并恢复 `m_color` 与 `m_committedExpression`；回车与「确定」等价（提交+关闭弹窗），不存在"提交后仍可取消"的不一致。
- **验证**：新增 UI 级探针 `Temp/temp_p110_ui_probe.cpp` 6 场景（回车提交 / ESC 丢弃 / 取消丢弃 / 确定提交 / 合法 hex 清空+color-changed / 未提交 hex+ESC 保留表达式）全部通过。
- **附带修复**：补齐 `popup-visible` Bool getter（API 映射表既有声明，此前未实现）。
- **1→0 语义**：维持程序化静默清空（复核 §8.3 选项 1），设计 §2.2-2/Q-8 已按实现表述。
