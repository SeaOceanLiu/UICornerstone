# P109_PlaceholderTyped_Design — 组件模板占位符支持数字/布尔类型（整串替换为 JSON 值）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-29 追加：P0-109 高）
> 状态：**已放行，已实施**（2026-10-29；第 3 版复核通过；单遍构造不递归已落实；探针 11/11 + 全量回归基线保持）
> 复核意见：`requirements/引擎P108_P110设计评审.md`

---

## 1. 现状核实

| # | 事实 | 依据 |
|---|---|---|
| ① 仅处理字符串节点 | `replacePlaceholders` 首行 `if (node.is_string())`——数字/布尔节点不支持占位（本需求针对字符串写法 `"font-size": "{{size}}"`） | `src/LayoutParser.cpp:3847` |
| ② 数字/布尔值回写成**字符串** | instance/default 为 number → `to_string(double)`；bool → `"true"/"false"`，`node = val`（string）✗ | `src/LayoutParser.cpp:3853-3885` |
| ③ 类型化属性被拒 | `font-size`（int）/`visible`（bool）等属性收到字符串 → 解析器拒绝 → **数字/布尔属性无法模板化** ✗ | 同上（设计器实测"硬伤"） |
| ④ 模板机制 | 组件模板深拷贝后 `replacePlaceholders(expanded, props, instanceJ)`；instance 值优先、缺省回退 `props[name].default` | `src/LayoutParser.cpp:3980-3987` |
| ⑤ **仅替换首个占位符** | 现有实现只替换字符串中第一个 `{{...}}`；多占位串（如 `"{{label}} ({{id}})"`）其余保持原文 ✗（设计器幽灵已全量替换，预览/导出将不一致） | `src/LayoutParser.cpp:3849-3885` |

## 2. 方案

### 2.1 整串占位类型化（核心）
在字符串分支中，先做**整串判定**（trim 后）：
```
name = trim(val 去掉首尾空白)；若 name 形如 "{{prop}}" 且恰为整串（单占位、无其它字符）：
    取值 = instanceJ[prop]（存在且非 null）否则 props[prop].default
    - 值为 number/bool → node = 该 JSON 值（原样类型，不再 to_string）
    - 值为 string    → node = 字符串值
    - 无值/默认       → 保留字面占位 + warn（现状）
否则：进入多占位/混合串替换（§2.2）
```
- **trim**：整串判定与 prop 名查询均 trim（`"{{ size }}"` 与 `"{{size}}"` 等价；评审 2.2-1）；
- **0/false 原样类型化**：用 `is_number()/is_boolean()` 判定（**不得**用真值/`as<bool>` 转换）——`0`/`false` 正常替换（评审 2.2-2）；
- **null 语义**：instance 值为 `null` → 视为"未提供"，回退 default；default 为 `null` → 视为无值（保留字面占位 + warn）（评审 2.2-2）；
- **不递归**：替换后的值不再二次扫描（若值恰为 `"{{other}}"` 保持字面量）（评审 2.2-4）。

### 2.2 多占位/混合串：全量替换（**单遍扫描原串构造**，不递归——复核 §6.2）
- 实现口径：**单遍扫描原始字符串**，逐段 append 到输出；**替换结果不再扫描**：
  - 找到每个 `{{...}}`：instance 值（非 null）优先 → default 回退；number → `to_string`；bool → `"true"/"false"`；string → 原样 append；
  - 缺失值 / default 为 null：append **原字面量** + warn（每个占位一次）；
  - **禁止**对替换后的字符串循环 `find`（否则 prop 值恰为 `"{{other}}"` 会被二次展开，违反"不递归"）；
- 整串单占位在扫描**前**判定（trim 后恰为 `{{name}}` 且值/默认 number/bool → 直接赋类型，§2.1）；
- 含多个占位或混合字符的串仍为**字符串**（评审 Q-1 采纳）。

### 2.3 嵌套组件
外层替换把占位值写入内层实例的 props 后，内层再执行各自的整串/混合替换与类型化（逐层独立；补测试）。

### 2.4 数字格式与整型
- 整串类型化：JSON number 直传（保留原始精度）；
- 混合串：仍 `to_string(double)`（可能出现 `18.000000`）——文档注明该差异（设计器幽灵显示 `18`，预览混合串可能为 `18.000000`）；
- 数字属性（int）：`18.9` 传给 int 属性按引擎现有解析行为（JSON `get<int>` 截断）——文档注明。

## 3. 验收与测试矩阵

1. **数字**：模板 `"font-size": "{{size}}"` + 实例 `{"size": 18}` → `GetInt("font-size") == 18`（非字符串）；
2. **布尔**：模板 `"visible": "{{vis}}"` + 实例 `{"vis": false}` → `GetBool("visible") == 0`（非字符串）；**`false`/`0` 不得被真值判断丢弃**；
3. **null**：instance `null` → 回退 default；default `null` → 保留字面占位 + warn；
4. **空白**：`"{{ size }}"`（含空格）整串类型化生效；
5. **default 类型**：实例未提供、`props.size.default == 20` → `GetInt==20`；default 为字符串时保持字符串；
6. **字符串不变**：`"caption": "{{title}}"` + `"标题"` → 字符串标题；
7. **多占位全量替换**：`"caption": "{{label}} ({{id}})"` + `{"label":"A","id":7}` → `"A (7)"`（本批修复后）；
8. **混合串数值格式**：保持字符串（`"x-{{n}}"` + 3 → `"x-3"`；浮点呈现 `to_string` 特性，文档注明）；
9. **不递归**：`{"x": "{{y}}", "y": "Z"}` → `x` 为字面 `"{{y}}"`；
10. **嵌套组件**：外层 prop 注入内层实例 → 内层类型化正确；
11. **颜色/路径不变**；嵌套对象/数组同规则；全量回归仅 4 项预存。

## 4. 影响面与文档

- 源码：`src/LayoutParser.cpp`（`replacePlaceholders`，局部）；
- 无 ABI/schema 变更（组件模板为布局 JSON 语法）；
- 文档：组件/模板章节 + 用户手册组件语法处注明"整串占位继承值类型；trim；多占位全量替换；混合串数值格式差异"；
- 设计器：占位符字符串保留策略不变（与幽灵侧行为一致）。

## 5. 待确认（Q，含复核答复）

| # | 问题 | 结论 |
|---|---|---|
| Q-1 | 仅"恰好整串 `{{name}}`"类型化，混合串保持字符串 | **采纳**（`0/false/null/空白` 边界按 §2.1） |
| Q-2 | 整串 + number 的原样类型（不转 to_string）是否含浮点精度（JSON double 直传） | **采纳**（JSON number 直传） |
| Q-3 | 多占位/混合串的数值格式（to_string）本批不改 | **修订**：多占位**本批改为全量替换**（字符串）；混合串数值格式仍 to_string（文档注明） |
