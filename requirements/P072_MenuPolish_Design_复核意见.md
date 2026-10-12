# P072_MenuPolish_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-07
- 对象：`design/P072_MenuPolish_Design.md`（P0-72①~④，状态：待评审）
- 结论：**通过，放行实施**（② 依赖 P074④ 逐键，建议同批/先行；附设计器随批 4 项）

## 1. 评审要点

### ① shadow-radius（MenuPanel）——✓

- 根因核实 ✓（`m_shadowRadius` 死字段、`drawShadow` 用常量）；方案 ✓；
- Q-1 **同意 menus[i]（每菜单）**：设计器按菜单存面板级状态（与 font/item-height-ratio 同层一致 ✓）；JSON `menus[i].shadow-radius` → panel ✓；
- 设计器随批：menus 行式格式扩展"菜单行属性"（panel 级：颜色/字体/阴影半径——现菜单行仅 caption；复用 parseStructAttrs 共享解析 + 半径命名 attr），模型存 `extended.menus[i]`，导出 1:1 ✓。

### ② colors-menu.background → state-color-4——✓（依赖 P074④）

- 现状核实 ✓（运行期 kStatePressed→m_activeBgColor 读写齐备；schema colors-menu 仅 3 态）；升级四态 + 依赖逐键解析 ✓；
- 语义确认：pressed=**主菜单激活（展开中）态**（bar 自身）✓；面板/item 收到 pressed 仅存储不绘制——无害 ✓；
- 设计器侧零代码：menu-bar 属性面板 background 组自动增 pressed 行（schema 驱动）✓；既有布局（N/H/D）strict 仍合法（pressed 非 required）✓。

### ③ addMenu 状态回放——✓

- 缺陷链核实 ✓（仅传播既有 m_entries；重建后新面板回默认）；
- 方案（applyStateToPanel + addMenu 末尾调用 + setter 统一走同一实现）✓；
- Q-4 回放清单**确认**：11 色（background/hover/pressed/disabled/border/border-hover/border-disabled/text/text-hover/text-disabled/text-shadow）+ fontSize + fontName + itemHeightRatio + text-shadow(enabled/offset) + shadow-radius（①）；
- 设计器随批：**删除 syncMenus 手动回放过渡**（CanvasPane.cpp:2432-2441，引擎修复同步后）✓。

### ④ item 级颜色——✓

- 方案 ✓；Q-2 **同意分散键**（`item-text`/`item-background`/`item-hover-background`，与既有 `item-id` + `item-font-size` 定向模式一致）；Q-3 同意"四态 text + 两态 bg 先行、item 阴影后续"；
- JSON 键（`text-color` 字符串/四态对象、`background-color` 字符串/{normal,hover}、`hover-background-color` 别名）与 statusbar/tree/ListView cell 家族一致 ✓；
- 绘制回退链（item 显式 → 面板级 ← bar 经 ③ 回放）闭环 ✓；
- 设计器随批：
  1. menus items 行式 attrs 扩展（`b#/t#/hb#` 等，与 statusbar/tree 家族同款——现菜单项仅 `caption@shortcut`）；
  2. **菜单项需稳定 id**：引擎 `populateMenuPanel` 读 `"id"` → `setItemId`（src/LayoutParser.cpp:1806-1809）✓，设计器 menus 模型项现无 id → 定向 CABI（item-id 模式）无法定位 → 设计器将生成稳定 id（如 `mi_N`）并入模型/导出（引擎侧无需改动）。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | shadow-radius 层级 | **同意 menus[i]**（设计器 per-menu 存储/编辑；与同层键一致） |
| Q-2 | item 级 CABI 键名 | **同意分散键**（与 item-id+item-font-size 定位模式一致；`item-colors` 对象无必要） |
| Q-3 | ④ 范围 | 同意四态 text + 两态 bg 先行；item 阴影后续 |
| Q-4 | ③ 回放清单 | **确认**（含 ① 的 shadow-radius） |

## 3. 设计器随批（同步后）

1. 重建设计器 + auto=1/2/3 回归；
2. 删除 syncMenus 回放过渡；联测：结构化重建菜单后颜色/字体保持（回归）；
3. menus 行式格式扩展（菜单行 panel attrs + 项级颜色 attrs + 项 id 生成）随批实施；
4. 探针：shadow-radius 读写回 + 菜单项独立变色（引擎侧测试覆盖后可省设计器探针）。

## 4. 放行

**结论：放行实施**（② 与 P074④ 同批/其后即可；③ 回放实现后设计器删过渡）。
