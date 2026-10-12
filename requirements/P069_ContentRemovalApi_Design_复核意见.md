# P069_ContentRemovalApi_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-05
- 对象：`design/P069_ContentRemovalApi_Design.md`（P0-69①②③，状态：待评审）
- 结论：**通过，放行实施**（四问全部确认）

## 1. 评审要点

- §1 现状确认 ✓（内部已有 removeMenu/removeTab，CABI 未暴露）；
- §2 生命周期分析质量高 ✓：MenuPanel 保活池往返（menuPoolKeep/Take）、**Tab 页 bench+tab 双挂载**（`setParent` 不摘旧父 children）——后者对设计器"页 Panel"流程（CreatePanel 先挂 bench → TabSetPage）**至关重要**：不修则设计器自管的页 Panel 会残留 bench 鬼影；
- §3 方案（摘除原语 + 全摘除保活 + TabSetPage 对齐）✓；§4 测试计划覆盖句柄有效性/再挂载/鬼影 ✓。

## 2. §6 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | 仅 `MenuBarRemoveMenu`（caption 遍历足够）？ | **确认**（设计器 menus 同步 = 按旧 caption 移除 → 重建；无需 ClearMenus） |
| Q-2 | `TabSetPage` 替换旧页清理（全摘除+保活）纳入本批？ | **确认纳入**（与移除同根因；设计器页 Panel 创建路径直接受益） |
| Q-3 | ③ MenuPanel 清空暂缓？ | **确认暂缓**（设计器每菜单新建 panel，不阻塞） |
| Q-4 | Binding 返回 `int`？ | **确认**（判定成败） |

## 3. 设计器随批（同步后）

1. **MenuBar `menus`**：行式编辑（`>` 子菜单层级 / `@快捷键` / `--` 分隔符）→ 同步 = 按旧模型菜单 caption 逐个 `MenuBarRemoveMenu` → 新建 MenuPanel 逐项组装（`MenuPanelAddItem`/`AddMenuSeparator`/`MenuItemSetSubMenu`）→ `MenuBarAddMenu`；
2. **TabControl `tabs`**：行式编辑（`标题@图标`）→ 同步 = 从尾 `TabRemovePage` 旧页 → 逐页 `TabAddPage` + 设计器自管页 Panel（`CreatePanel` + `TabSetPage`）+ `TabSetTitle`；
3. TabControl 容器语义（页 Panel 为放置目标）+ 序列化 `tabs[i].page`（随批实施，方案 A 已获用户确认）。

## 4. 放行

**结论：放行实施**。§3.4 双挂载修复为设计器页 Panel 流程的前置保障。实施同步后设计器随批 menus/tabs 编辑器 + 联测（菜单增删改、页增删、页内放置控件、保存/导出往返）。
