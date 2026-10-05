# P062_CellNodeColor_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-03
- 对象：`design/P062_CellNodeColor_Design.md`（P0-62①/②，状态：待评审）
- 结论：**通过，放行实施**（附 1 项实施注意：TreeView 节点样式未设态回退——防"仅设 normal → hover 变暗/变色"复发）

## 1. 评审要点

### P0-62① ListView 单元格文字色 ✓（含附带缺陷抓取到位）

- 字段/绘制链现状与"ABI setter 缺失"抽验一致 ✓。
- **附带缺陷确认**：`CellStyle.textColor` 缺省 `SColor`（alpha=1）且绘制条件为 `alpha() > 0` → 现有 `SetCellStyle`（仅设背景）会把文字覆盖为黑——`hasTextColor` 稀疏化修复正确 ✓（同"仅设 normal"家族）。

### P0-62② TreeView 现状更正确认 ✓（抽验属实）

- 运行时泛型链确实存在：`item-id` 定位 + `item-background/item-border/item-text/item-text-shadow`（四态对象与单态键，TreeView.cpp:872-916）、`item-shadow`/`item-shadow-offset-x/y`（:984-991）、读回对称（:888-892/:1071-1076）；设置即 `hasStyle=true` ✓。
- 真实缺口=① 专用 id 参数 API；② JSON items 样式键解析——判断准确 ✓。

### 方案（§2）✓

- 2.1：`hasTextColor` + 绘制稀疏 + 独立 API（保 ABI 兼容）✓。
- 2.2：3 个专用 API（直写节点字段 + hasStyle，不依赖共享 `item-id` 状态）+ JSON 键与 P0-56 同族命名 ✓；schema description 更新足够（元素宽松）✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | P0-62② 现状更正确认 + 专用 API 仍需要？ | **确认两者**：泛型链存在属实；专用 API 需要（共享 `item-id` 状态易误写——顺序错/并发即写错节点；设计器随批直接调专用 API） |
| 2 | P0-62① 附带缺陷一并修复（hasTextColor）？ | **确认一并修复** |
| 3 | JSON `text-color` 四态对象 + `background-color` 单色？ | **确认**（对齐 P0-55 statusbar 段色形态） |
| 4 | 节点阴影 API 形态（色+偏移，置开关）？ | **确认** |
| 5 | ListView 单元格 JSON 样式键本批一并？ | **确认后置**（设计器行式格式走 API 绑定；JSON 单元格样式后续按需） |

## 3. 实施注意（放行附项）

1. **节点样式未设态回退（重要）**：`hasStyle` 节点绘制走 `ControlImpl::resolveStateColor(node->textColor/bgColor, rowSt)`（TreeView.cpp:234/:304），而 `TreeNode` 的 StateColor 缺省为引擎默认（bg 深色/text 灰）——**仅设 normal 后 hover/pressed 会落到缺省色**（浅色行 hover 变暗、文字变色，同"仅设 normal 致 hover 变黑"家族）。建议：
   - `TreeViewSetNodeBackgroundColor` / JSON `background-color`（单色语义）→ **四态同色**（无回退需求，最简）；
   - `TreeViewSetNodeTextColor`（`state=NULL`）/ JSON `text-color` 字符串 → 未设态**回退该节点 normal**（掩码或 draw 侧回退，同 P0-55 段色方案）；对象四态 → 显式设态；
   - `text-shadow` 单色无态 ✓ 无此问题。
2. **设计器随批格式（供参考，无需引擎动作）**：tree items 行格式尾部扩展样式字段（`>…标签|#文字色|#背景色|#阴影色|偏移x|偏移y`；标签内 `|` 经 `\|` 转义，复用既有 tree 转义函数）；ListView rows 单元格样式在实现时定 `@` 后缀语法。

## 4. 放行

**结论：放行实施**。现状核实（含附带缺陷）与方案（稀疏化修复 + 专用 API + JSON 键）质量高；附 1 项实施注意（节点样式未设态回退）。实施完成后同步 subModules，设计器随批：tree items 样式字段扩展 + rows 单元格样式 + 专用 API 接线，端到端联测（单元格/节点着色 + 未设继承控件级）。
