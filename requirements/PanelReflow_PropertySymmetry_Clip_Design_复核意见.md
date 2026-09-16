# PanelReflow_PropertySymmetry_Clip_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-16
- 对象：`design/PanelReflow_PropertySymmetry_Clip_Design.md`（状态：待审核）
- 结论：**通过，放行实施**（6 个待审核问题全部答复见 §2），附 3 项小补充（均不阻塞）

## 1. 分项评审

### 需求一（Panel::addControl reflow）✓

- 方案与需求一致；**removeControl 对称补齐是正确的超出需求的补充**——验收 3（布局无残留）的必要实现，单补 addControl 不够。
- §1.2.1 Bench 复用边界分析准确：`Bench::addControl` 显式调用 `Panel::addControl` 基类实现（虚分派不递归），新分支对显式设了 layoutEngine 的 Bench 自动生效且语义正确；Bench 一般无 engine 不触发；②③ 与新分支互补不双重 reflow。结论认可。
- 小提示（不阻塞）：逐控件 AddChild = 逐次全量 reflow（设计器动态区 ~16 控件 16 次，量级无碍）。若未来有批量挂入场景（如表格初始化数百行），可再议 batch 入口，本期不做。

### 需求二 A（getter 补齐）✓

- `Button::getCaptionSize(float) const` 笔误定性准确（带参未用/uint32_t 截断）；签名修正的调用点 grep 评估思路认可。
- P0（Button caption-size + check-state 等高频）/P1（脚本扫描差集分批）分批合理。

### 需求二 B（命名统一）✓ —— 本文档最有价值的部分

- 方向已按评审讨论落地：统一到运行时属性名（kebab），"改数据 < 改代码"的论证成立。
- 数据核实扎实：schema 176 可编辑键/92 camelCase/4 简写特例；`kStyleCross` 与颜色键 `kCross` 不同分发域不冲突（改名安全性关键疑点已排除）。
- P0（特例改名 + schema/layouts 迁移 + version bump）/P1（LayoutParser kJson 体系合并 890 处）分批正确——P1 不阻塞设计器收益。
- **getter 补齐仍必须做**的定位准确（键名与读写对称是两个独立缺陷）。

### 需求三（clip-children）✓

- Panel 层实现、默认 false 兼容、pushClipRect 包 `ControlImpl::draw()`（子项递归在 clipStack 内）、内容 rect 与 EditBox 同口径（含缩放）——方案完整。
- 渲染器 clipStack 三后端已有（EditBox 在用）——风险评估属实。

## 2. 对 6 个待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | P0（1+2+3+6）先行、P1（kJson 合并）重构 | **确认**。P1 期间 kJson* 与新常量并存可接受 |
| 2 | getter P0 先行、P1 全量扫描分批 | **确认** |
| 3 | removeControl 对称补 reflow | **确认**（必要，非可选） |
| 4 | clip-children 归属 Panel 层 | **确认**。ControlImpl 全控件开放裁剪无应用场景且影响面大，留作后续 |
| 5 | text→caption 等破坏性键名变更 | **接受**。无正式应用使用的窗口期，一步到位；设计器 main_layout.json 由我方同批迁移（见 §3） |
| 6 | 动画 jsonc 范围 | 由引擎自查：动画键名若独立于属性系统则不同批迁移，我方不阻塞 |

## 3. 设计器侧配合承诺（同步实施，非引擎阻塞项）

1. `layouts/main_layout.json` 键名同批迁移：92 类 camelCase 键（borderVisible/imageResource/fontSize/matchParentRect 等）→ kebab；version bump；
2. 命名统一就绪后：删除 `jsonKeyToProp`（规则+特例表）与 `textIsCaption` 特判，动态面板直接用 schema 键名调 `Set*/Get*`；
3. clip-children 就绪后：`propDynamic` 开启裁剪，删除"超界行隐藏"逻辑，滚动改纯偏移；
4. Panel reflow 就绪后：动态行可回归声明式布局（当前自管 y 因滚动偏移仍需保留行 baseY 记录，二者兼容）。

## 4. 补充建议（不阻塞）

1. **§2.3 实施清单 2 补充**：schema 键 `text` 语义分叉（Label/Button/CheckBox→caption；EditBox/TextArea/ComboBox/ProgressBar→text）落地时建议在 schema 的 description 注明"此键映射运行时属性 X"，避免后续维护者再混淆；
2. **clip-children 与滚动条**：设计器滚动容器内含 ScrollBar 子控件（rect 在容器内），裁剪开启不影响——无需特殊处理，仅提示验证时留意滚动条滑块绘制不被容器边框裁掉（内容 rect 若含 border 口径需对齐）；
3. **文档同步清单**（§5 已提）补充一项：`API_Mapping_Table.md` 若含属性键名速查，同步 4 项特例改名。

## 5. 放行

**结论：放行实施**（按 §4 顺序：二 A → 一 → 三 → 二 B P0；二 B P1 随后重构）。实施完成后同步 subModules，我方按 §3 清单同批迁移并做回归：

- 动态行：初始位置正确（reflow）+ 值回填真实值（getter）+ 滚动裁剪平滑（clip）+ 零桥接直连（命名统一）四项联合验收。
