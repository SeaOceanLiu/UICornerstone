# P082_MenuBarHoverState_Design — 菜单栏项状态色残留（展开后移向相邻菜单，来源项仍保持底色）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-09 追加：P0-82）
> 前置：P0-80/P0-81 批已实施并同步（设计器全部引擎绕过清零）
> 状态：**已放行，已实施**（2026-10-09；Q-1~Q-4 确认；§2.1 `return true` 条件化修正已采纳）
> 复核：`requirements/P082_MenuBarHoverState_Design_复核意见.md`（通过）
> 实施备注：① `MenuBar::handleEvent` Move 双模式均更新 `m_hoveredIndex`（菜单模式保持消费、非菜单模式落入原 drawRect 判定）；② `MenuBar::onMouseLeave` 覆写清 per-item 悬停（不退出菜单模式）；③ `MenuPanel` 指针移出面板清 item 悬停（子菜单优先消费链保持——评审依赖注明）；④ `MenuBar::getHoveredIndex()` + `getIntProperty("hovered-index")` **只读**读回；⑤ 验证：**P0-82 探针 OK**——a) 悬停→移出 hovered=-1；b) 展开 File→移 Help `hovered=1`（本复现）；c) **像素差分**：File 项恢复常态基线；d) 移出栏 hovered=-1 且展开保持；e) 外部点击关闭清空；f) 面板移入=0/移出=-1；g) **子菜单依赖**：移入子菜单可见保持且父项高亮保持；⑥ 全量构建 0 错误、回归仅 4 项预存

---

## 1. 问题确认（源码核实，与设计器定位一致）

**复现**：点击"文件"展开 → 鼠标移到"帮助" → "文件"仍保持 `background.pressed` 底色（应转移激活/悬停，来源项恢复常态）。

| 环节 | 事实 | 位置 |
|---|---|---|
| ① Move 分支双模式差异 | `m_menuMode` 下**只 `switchMenu(index)`、不更新 `m_hoveredIndex`**；仅非菜单模式执行 `m_hoveredIndex = index` | src/Menu.cpp:1609-1619 |
| ② 场景 A（switch 成功） | `m_activeIndex=1`（帮助激活 ✓）但 `m_hoveredIndex` 仍为 0（点击前悬停文件置位）→ 绘制 `else if (i==m_hoveredIndex) fill(m_hoverBgColor)` → **文件残留悬停底色**；文本色/阴影同源（:1542-1546） | :1526-1535、:1542 |
| ③ 场景 B（switch 未达） | 活动面板先行消费 Move（:1459-1461 `panel->handleEvent` 命中即 return）→ bar 的 Move 分支不执行 → 文件保持 `m_activeIndex` → **残留激活（pressed）底色** | :1459-1461 |
| ④ 无 enter/leave 清理 | `MenuBar` 未覆写 `onMouseEnter/onMouseLeave`（基类仅切 `ControlState`，不清 per-item `m_hoveredIndex`，ControlBase.cpp:684-698） | include/Menu.h |
| ⑤ MenuPanel 同类风险 | 面板 Move 仅**命中 rect 时** `setHoveredIndex(index)`（:975）；指针移出面板（回栏/外部）不清，仅 `hide()` 清（:461）→ 面板展开时 item 悬停底色可残留 | :975、:454-461 |
| ⑥ 读回缺口 | `m_hoveredIndex` 为 MenuBar 私有、**无 getter/属性读回**（`hovered-index` 仅 MenuPanel 有读写 :606/:621）→ 探针/设计器无法读状态 | include/Menu.h、src/Menu.cpp |

## 2. 修改方案

### 2.1 MenuBar Move 分支：双模式均更新悬停索引
```cpp
if (event->m_type == EventType::MouseMove) {
    m_hoveredIndex = index;              // P0-82：两种模式均更新（-1 自然清除，消除残留）
    if (m_menuMode) {
        if (index >= 0 && index != m_activeIndex) switchMenu(index);
        return true;                     // 仅菜单模式消费（原语义）
    }
    // 非菜单模式：落入原 drawRect.contains 判定（保持放行/消费原语义——评审修正）
}
```
> 复核修正已采纳：`return true` 仅限菜单模式；非菜单模式保持原"栏外放行"语义（兄弟控件悬停依赖该链路）。
- 场景 A：hovered 随 active 转移 → 文件恢复常态 ✓；
- 场景 B（面板消费未达 bar）：bar 未收 Move，仍由 2.2 的 leave 清理兜底（指针离开栏时）✓。

### 2.2 MenuBar `onMouseLeave` 覆写（防御）
```cpp
void MenuBar::onMouseLeave(float x, float y) override {
    ControlImpl::onMouseLeave(x, y);     // 原有 setState 切回
    m_hoveredIndex = -1;                 // P0-82：清 per-item 悬停（不退出菜单模式——展开保持）
}
```
- 覆盖窗口级/跨控件离开路径（帧级 enter/leave），与 2.1 双保险。

### 2.3 MenuPanel：指针移出面板清 item 悬停
- `MenuPanel::handleEvent` Move 分支：未命中 `drawRect` 时 `setHoveredIndex(-1)`（保持面板展开与子菜单链逻辑不变；`setHoveredIndex` 内部已处理子菜单打开/关闭状态 :583-600）。

### 2.4 读回补齐（探针/设计器状态显示）
- `MenuBar` 增 `int getHoveredIndex() const { return m_hoveredIndex; }` + `getIntProperty("hovered-index")` **只读**分支（写回不暴露，维持引擎状态机单一驱动；Q-2）。

### 2.5 不改动
- `enterMenuMode/exitMenuMode/switchMenu/closeAllMenus`（:1233 已清 hovered）；
- 绘制色源（active→`m_activeBgColor`、hovered→`m_hoverBgColor`）；
- 面板消费链（:1459-1461）与菜单展开语义（移出栏不自动关闭，点击外部/Esc 关闭——标准行为）。

## 3. 测试计划
- **引擎探针（Temp，驱动 handleEvent + 读回）**：
  a) 非展开：悬停项 → 移出栏 → `hovered=-1`；
  b) **本复现**：点击展开项 0 → Move 到项 1 → `active=1 && hovered=1`（无残留 0）；
  c) 展开 → 移出栏外：`hovered=-1`，`active` 保持（展开态）；
  d) 展开 → 移入面板：bar `hovered=-1`；面板 item hovered 生效；面板内 → 面板外：面板 `hovered=-1`；
  e) 外部点击 / Esc：`exitMenuMode` → `active=-1 && hovered=-1`；
  f) 快速移动序列（0→1→2→-1→1 连续 Move）；
  g) **像素断言**：残留 pressed/hover 底色消失（修复前后差分）；
- **回归**：test_menu / test_property_cabi（P0-69 菜单移除段）/ test_overlay_viewport_cabi + 全量（仅 4 项预存）；
- 设计器联测：本复现步骤 + 菜单模式移出/移入面板。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 修法：Move 双模式均更新 `m_hoveredIndex` + `onMouseLeave` 覆写（双保险） | 确认 |
| Q-2 | MenuBar `hovered-index` 读回：只读 getter + `getIntProperty`（不暴露写回） | 建议补（探针/状态显示需要） |
| Q-3 | MenuPanel 移出面板清 item 悬停同批 | 确认（同类残留，一并修） |
| Q-4 | 展开后移出栏不自动关闭（点击外部/Esc 才关）保持标准行为 | 确认 |
