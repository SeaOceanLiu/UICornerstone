# P065_HoverOnlyBg_Design — hover-only 背景常态落默认色修复（P0-65①②）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-04 追加：P0-65①②，P0-64 联测缺陷）
> 前置：P0-64 批已实施并同步（显式 hover 背景 + bgMask + 叠加机制）
> 状态：**已放行，已实施**（2026-10-04；② 按复核意见修订后实施）
> 复核：`requirements/P065_HoverOnlyBg_Design_复核意见.md`（① 直接放行；② 按"hasHoverBg 解耦 + 绘制顺序"修订后放行）
> 实施备注：① `CellStyle.hasBg`（仅 setCellStyle 置位）+ 门槛 `hasBg && alpha>0`；② **修订采纳**：`TreeNode.hasHoverBg` 与 bgMask 解耦（专用 hover API 不置 bgMask/hasBgStyle），base fill 按 `bgMask==0（单色全态）‖ 位命中 bgSt`，绘制链 = base → selected 实色 → hover 实色（未显式）→ 显式 hover 覆盖（selected 优先）→ tint 叠加（豁免含显式 hover）；合并场景（单色+Hover API）常态保留 ✓；测试：test_listview 51/0、test_treeview P0-65b、P0-65 静态像素探针 8/8（hover-only/合并 × ListView/TreeView 常态与悬停精确色）、P0-64 探针复跑 11/11；回归仅 4 项预存失败

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-65① | ListView 单元格 hover-only 常态填黑 | **确认** | `setCellHoverBackgroundColor`（ListView.cpp:426-431）经 `m_rows[row].cellStyles[col]` **值初始化**创建条目 → `CellStyle.bgColor = SColor()`（缺省 **alpha=1.0**，SColor.h:16）；draw 常态门槛 `hasBase = csC.bgColor.alpha() > 0`（ListView.cpp:700）→ 为真 → **常态填黑**（P0-64③"无常态 bg 也可用"语义未成立） |
| P0-65② | TreeView 节点 hover-only 常态落默认深色 | **确认** | `setNodeHoverBackgroundColor`（TreeView.cpp:931-938）置 `hasBgStyle=true + bgMask|=2`；draw base fill（TreeView.cpp:240-244）仅判 `hasBgStyle` → 常态 `resolveStateColor(bgColor, Normal)` = StateColor 缺省（引擎默认深色）→ **常态深色** |
| 现状确认 | StatusBar 段无此问题（`hasBackground` 独立位）✓；单色四态同色（bgMask=0 / setNodeBackgroundColor）与对象全态行为不受影响 | — | — |

**设计器过渡（现以透明色垫底规避，引擎修复后删除）**：rows `SetCellStyle(透明,0)` 再 hover 色；tree `SetNodeBackgroundColor(透明)` 再 hover 色。

## 2. 修改方案

### 2.1 P0-65① ListView：`CellStyle.hasBg` 显式位
- `CellStyle` 增 `bool hasBg = false;`（仅**显式设置常态背景**时置位）；
- `ListView::setCellStyle`（engine 唯一常态背景入口；ABI `ListViewSetCellStyle` 同路径）置 `hasBg = true`（写入 map 条目时）；
- draw 常态门槛改判：`const bool hasBase = csC.hasBg && csC.bgColor.alpha() > 0;`
  - hover-only 条目（hasBg=false）→ 常态**不填**（回退行/控件级背景）；hover 时 `hasHoverBg` → 显式色（P0-64③）✓；
  - 显式设透明（hasBg=true 且 alpha=0）→ 仍不填（保留"显式透明不绘制"语义）；
  - selected 行：既有叠加链（P0-63①）不受影响。

### 2.2 P0-65② TreeView：base fill 按 `bgMask` 状态位门槛
- base fill 判定改为：
  ```cpp
  const ControlState bgSt = (i == m_selectedRow) ? ControlState::Normal : rowSt;
  const bool hasBg = rowNode->hasBgStyle
      && (rowNode->bgMask == 0 || (rowNode->bgMask & nodeStateBit(bgSt)));   // bgMask==0=单色全态
  if (hasBg) { fill resolveStateColor(bgColor, bgSt); …叠加（P0-64① 不变）… }
  else if (既有 selected/hover 实色链) …
  ```
  - `bgMask==0`（单色 API / JSON 字符串）→ 全态填充（行为不变）；
  - 对象/专用路径（位掩码）→ **仅位命中的态填充**；未命中不填 → 走既有链（hover-only 节点：常态透明；hover 命中华 hover 位 → 显式色 + 叠加豁免）；
  - selected（bgSt=Normal）：仅 normal 位/全态才填背景，selected 叠加（P0-64 规则）不变；hover-only 节点选中 → 既有 selected 实色链；
  - pressed/disabled 位同理独立判定。
- 设计器过渡（透明垫底）同步后删除。

## 6. 复核修订（2026-10-04）

- **② 缺口**：原方案 `bgMask` 门槛在"单色 API（mask=0）+ hover API（mask|=2）"组合下 → 常态 `2&1=0` → **常态背景丢失**（设计器 `#bg@hb#hover` 主用组合）。
- **修订**：`TreeNode.hasHoverBg` 解耦（专用 hover API 置位、不置 mask/hasBgStyle）；`setNodeBackgroundColor` 单色仍 mask=0 → 全态；绘制链：
  ```
  bgHit = hasBgStyle && (bgMask == 0 || (bgMask & bit(bgSt)))
  bgHit → 填 base；else selected → 填实色；else hovered(未显式) → 填实色
  hover 显式（hasHoverBg 且非 selected）→ 覆盖填 hover 色
  tint 叠加：仅未豁免（显式 hover / mask hover 位 / mask pressed 位 / disabled）
  ```
- 场景核对（像素探针）：单色 ✓ / 单色+hover ✓ / hover-only ✓ / 对象仅 normal ✓ / 对象 hover 位 ✓ / 对象全态 ✓。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | ListView hover-only 单元格：常态**不填黑**（透出背景）、hover 显式色精确 | 静态像素探针 |
| 2 | ListView 显式背景（setCellStyle）+ hasBg 置位；显式透明不填 | 引擎断言 + 像素 |
| 3 | TreeView hover-only 节点：常态透明、hover 显式色；对象仅 normal 节点 hover 落既有实色链 | 静态像素探针 |
| 4 | 单色/对象全态既有行为不变（回归：P0-64 探针 11/11 复跑） | 探针复跑 |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **① 置位入口**：`hasBg` 仅由 `setCellStyle`（含 ABI）置位；显式 alpha=0 仍不填——确认？
2. **② 门槛语义**：`bgMask==0` 视为"单色全态填充"，位掩码仅命中态填充——确认？对象仅 normal 的节点 hover 落既有实色链（m_hoverColor 实色）——确认？
3. **设计器过渡**（透明垫底两处）同步后删除——确认？

## 5. 配套改动

- 代码：`ListView.h/.cpp`（hasBg + 门槛）、`TreeView.cpp`（bgMask 门槛）；
- 测试：静态像素探针（hover-only 两控件，含常态透明/hover 显式）+ 引擎断言 + P0-64 探针复跑 + 回归；
- 文档：listview/treeview 页 hover API 语义补注（hover-only 常态透明）；设计文档标注 + 复核归档 + make_release + 同步。
