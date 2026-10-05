# P065_HoverOnlyBg_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-04
- 对象：`design/P065_HoverOnlyBg_Design.md`（P0-65①②，状态：待评审）
- 结论：**① 通过；② 有缺口需修订（合并场景常态背景丢失）——修订后放行**

## 1. 评审要点

### P0-65① ListView `hasBg` ✓（方案正确）

- 根因确认准确（值初始化条目 → `SColor()` α=1.0 → `hasBase=alpha()>0` 恒真）✓；
- 抽验补充：`setCellTextColor/setCellHoverBackgroundColor/setCellFontName` 均为**直接引用条目字段**（不重置 bgColor），仅 `setCellStyle` 写全量 CellStyle——`hasBg` 仅由 setCellStyle 置位**不会误伤文字色/字体/hover-only 条目** ✓；
- 门槛 `hasBg && alpha()>0`：hover-only（hasBg=false）→ 常态不填 ✓；显式透明（hasBg=true, α=0）→ 不填 ✓。

### P0-65② TreeView 门槛——**缺口：合并场景（单色 API + hover API）常态背景丢失**

- 现 `setNodeHoverBackgroundColor`：`bgColor.setHover(color); bgMask |= 2; hasBgStyle = true;`（不设 normal 位）；
- 拟用门槛 `hasBgStyle && (bgMask == 0 || (bgMask & bit(bgSt)))`：
  - hover-only（mask=2）→ 常态不填 ✓（修复目标达成）；
  - **单色 API + hover API（mask=0→2）→ 常态 `2&1=0` → 不填** → **节点常态背景丢失** ✗——这是设计器 `#bg@hb#hover` 的**主用组合**（`setNodeBackgroundColor` 置 mask=0，随后 hover API 置 mask|=2，单色语义被破坏）。

**建议修订（数据模型解耦）**：
- `TreeNode` 增 `bool hasHoverBg = false`（专用 hover API 置位；**不再** `bgMask |= 2`、**不再**置 `hasBgStyle`）；
- base fill 门槛保持 `hasBgStyle && (bgMask == 0 || (bgMask & bit))`（`hasBgStyle` 仅由单色 API/对象路径置位；单色 API mask=0 → 全态填充）；
- 绘制顺序（hover 显式色独立于 base）：
  ```
  bgHit = hasBgStyle && (mask == 0 || (mask & bit(bgSt)))
  if (bgHit)                    填 resolveStateColor(bgColor, bgSt)
  else if (selected)            填 m_selectedColor
  else if (hovered && !hasHoverBg)  填 m_hoverColor（既有链）
  if (rowSt == Hover && hasHoverBg) 填 hoverBgColor（显式优先，覆盖 base/既有链）
  叠加 tint：仅 bgHit && !disabled && !(rowSt==Hover && hasHoverBg) && !(rowSt==Pressed && (mask&4))
  ```
- 各场景核对：
  - 单色：base 填 + hover tint ✓（P0-64 行为不变）
  - 单色+hover：常态单色、hover 显式色 ✓（修复合并缺口）
  - hover-only（API）：常态不填、hover 显式 ✓
  - 对象仅 normal（mask=1）：常态填、hover 落既有实色链 ✓（与设计意图一致）
  - 对象 hover 位（mask=2）：常态不填、hover 显式（mask&2 豁免 tint）✓
  - 对象全态（mask=0xF）：全态显式填充 ✓

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | ① `hasBg` 仅 setCellStyle 置位；显式 α=0 不填？ | **确认**（抽验无误伤路径） |
| 2 | ② `bgMask==0` 单色全态、位掩码仅命中态填充？对象仅 normal 落既有实色链？ | **确认语义方向**；但需按上节修订（`hasHoverBg` 解耦）——否则合并场景破坏 |
| 3 | 设计器过渡（透明垫底两处）同步后删除？ | **确认**（修复后删） |

## 3. 放行

**结论：① 放行；② 按"hasHoverBg 解耦 + 绘制顺序"修订后放行**。修订不改变 API 形态（ABI 不变，仅内部字段/绘制逻辑），验收表建议补一项：**合并场景（单色+hover API）常态单色、hover 显式色**（像素探针）。实施同步后设计器删过渡 + 联测（`#bg@hb#hover` 组合为主用例）。
