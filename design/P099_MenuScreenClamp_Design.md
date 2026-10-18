# P099_MenuScreenClamp_Design — 菜单/子菜单屏幕钳制（子菜单翻转 + 收边钳制）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-17 追加：P0-99 中）
> 前置：P0-71①（弹层 bench 挂载定位）/ P0-88（关闭链）已实施并同步
> 状态：**已放行，已实施**（2026-10-17；复核：`requirements/P098_099_100_复核意见.md`——通过）
> 实施备注（2026-10-17）：① 子菜单钳制（翻转/收边/垂直上移+顶部兜底——复核附注 2 确认）；② 顶层收边（bench 逻辑域）；③ 验证：探针用例 4（右缘翻转 rect=[226..684] 不出屏）/5（常规右贴不回归）/6（顶层收边 [0..698]）全过；④ 设计器保留标题压缩（复核附注 1，非阻塞）。

---

## 1. 复现与现状核实

| # | 断言 | 核实结果 |
|---|---|---|
| ① | 子菜单无边界检查 | ✓ `MenuPanel::setHoveredIndex`（src/Menu.cpp:612-616）：`subMenu->setPosition(item->getRect().width, 0)` 固定右贴父项、无任何钳制 ✗ |
| ② | 顶层面板无钳制 | ✓ `MenuBar::attachMenuPanel`（Menu.cpp:1267-1301）bench 局部定位公式无边界检查 ✗ |
| ③ | 长标题必出屏 | ✓ 面板宽 = 最长标题实测（`recalculateSize`）→ 最近文件长路径场景必然出屏（设计器已"…\尾段\文件名"压缩过渡） |
| ④ | 可用量 | 实例视口 `UIContext::viewport`（SRect，窗口像素；UIContext.h:59）；显示尺寸=逻辑×复合缩放（Menu.cpp:1291 既有注释口径）；`item->getDrawRect()`=全局像素矩形 |

## 2. 方案

### 2.1 子菜单（相对 MenuItem 定位，Menu.cpp setHoveredIndex）
- 目标绝对矩形 = `item->getDrawRect()`（全局像素）+ 子菜单显示尺寸（`subMenu->getRect()` 逻辑 × `subMenu->getScaleXX/YY`）；
- **水平**：右缘 `> viewport.right()` → **翻转到父项左侧**（`setPosition(-subMenu逻辑宽, 0)`）；翻转后仍越界 → 收边钳到视口内；
- **垂直**：下缘 `> viewport.bottom()` → 上移钳制（y 逻辑 = `(viewport.bottom - itemDraw.top - 显示高) / scaleY`，负值向上）。

### 2.2 顶层菜单面板（bench 局部域钳制，MenuBar::attachMenuPanel）
- 面板 canvas 逻辑矩形：`x = hitRect.left*sx/bsx`、`y = (hitRect.bottom)*sy/bsy`、`w/h = 逻辑尺寸`；
- 钳制在 **bench 逻辑矩形**内（`bench->getRect()`）：右缘超 → `x = benchW - w`（<0 取 0）；下缘超 → `y = benchH - h`；
- 主实例（bsx=1、anchor=0）与像素钳制等价；子视口经换算同口径。

### 2.3 可选（Q-2）
- 超宽标题绘制省略号：**本批不做**（设计器已标题压缩；引擎钳制落地后设计器放宽预算，如需再另批）。

## 3. 验收与测试矩阵
1. 长子菜单贴近屏幕右缘 → 翻转到父项左侧且完整可见；翻转仍不够 → 收边钳制；
2. 深子菜单（多级）接近屏幕下缘 → 上移钳制；
3. 顶层长菜单在栏右端（"帮助"）→ 收边不出屏；
4. 常规短菜单位置**不回归**（P0-71① 定位公式仅在越界时修正）；
5. 子视口实例：钳制按实例 viewport（Q-1）；
6. 全量回归（仅 4 项预存）。

## 4. 影响面
- 源码：`src/Menu.cpp` 局部（setHoveredIndex 子菜单定位 + attachMenuPanel 顶层）；
- 无 ABI/JSON/schema 变更；文档：用户手册菜单相关章节/5.7 边界注（P0-99 钳制行为）；
- 设计器：标题压缩预算可放宽（后续，非阻塞）；
- 与 P098③ 联动：可见面板 `setCaption` 重算后重钳制（P098 Q-3）。

## 5. 待确认（Q）
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 钳制边界取实例视口（`UIContext::viewport`） | 采纳（多窗口/子视口同口径） |
| Q-2 | 超宽标题省略号本批不做 | 采纳 |
| Q-3 | 翻转仅子菜单；顶层无父项只收边 | 采纳 |
