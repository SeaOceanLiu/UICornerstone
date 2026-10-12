# P066_ListViewJson_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-04
- 对象：`design/P066_ListViewJson_Design.md`（P0-66①②③，状态：待评审）
- 结论：**通过，放行实施**（Q1 建议收窄键面：`header-font-size` 单键，不加 `header-size` 别名）

## 1. 评审要点

### P0-66①② 确认 ✓

- ① columns 缺 `header-font-size`/`header-font-name` 解析属实（字段/API 齐备，仅缺 JSON 入口）✓；
- ② rows cells 只读字符串、对象 cell 读空串、CellStyle 无 JSON 入口属实 ✓。

### P0-66③ 附带回归 ✓（已独立核实——重要）

- 抽验属实：`UICornerstone_ListViewSetCellShadow`（UICornerstoneAPI.cpp:1903-1915）→ `getCellStyle`+`setCellStyle`；P0-65① 后 `setCellStyle` 强制 `hasBg=true`（ListView.cpp:416-420）→ **仅阴影/文字色+阴影单元格常态填黑**（默认 bgColor 黑 α=1）。**设计器 `内容@s#000000`（仅阴影）当前实际受影响**——引擎主动发现回归，价值高。
- §2.3 粒度方法方案正确（精确保留字段 + 不置 hasBg；parser 组合式写入）。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | ① 键名：`header-font-size`（推荐）+ `header-size` 别名？或仅 `header-size`？ | **建议仅 `header-font-size`**（不加别名）：`header-size` 语义含糊（字号 vs 宽高）、从未生效（无兼容包袱）；设计器导出映射 `header-size`→`header-font-size` 一行即可。`header-font-name` ✓ 单键 |
| 2 | ② 背景键形态：对象四态（主）+ `hover-background-color` 别名都支持？text-color 字符串单色？ | **确认两者都支持**（别名=对象 hover 等价，设计器导出 key-for-key 免映射）；`text-color` 字符串单色 ✓（CellStyle 无态）；pressed/disabled 忽略 ✓ |
| 3 | ③ 粒度引擎方法（ABI 不动、SetCellShadow 内部改调用）？ | **确认**（并修复设计器仅阴影单元格实际缺陷） |
| 4 | schema（columns 两键 + rows 宽松 description）？ | **确认** |

## 3. 实施注意

1. **粒度方法**：`setCellBackgroundColor`（bgColor+hasBg）/`setCellFontSize`/`setCellShadow`（色+偏移+shadowEnabled）——均**精确保留其余字段**（等价 getCellStyle 修改回写，但不触碰 hasBg）；parser cells 组合式写入。
2. **别名常量**：`hover-background-color` 若采纳，PropertyNames 补常量（与 kItemBackgroundColor 同族）；`header-font-size` 亦补（如无）。
3. **验收补充**：③ 补设计器侧用例——`内容@s#000000`（仅阴影）常态不填黑、hover/常态绘制正确；② 对象 cell 全字段断言建议含 `hasBg/hasHoverBg/hasTextColor/hasFontName` 稀疏位。
4. **设计器随批（无需引擎动作）**：导出映射 `header-color`→`header-text`、`header-size`→`header-font-size`；`.csd` 整包存模型（含 extended）。

## 4. 放行

**结论：放行实施**。①② 缺口确认准确；③ 附带回归发现质量高（已核实并影响设计器实际用例）。实施同步后设计器随批：导出映射接线 + `内容@s#000000` 回归复测，随后按序实施 保存→打开→导出→导入。
