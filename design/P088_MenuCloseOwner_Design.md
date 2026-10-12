# P088_MenuCloseOwner_Design — 菜单项点击后 MenuBar 状态未复位（closeMenuChain 父链断裂）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-10 追加：P0-88，高）
> 前置：P0-86/P0-87 批已实施并同步
> 状态：**已放行，已实施**（2026-10-10；Q-1~Q-4 确认；补充 1 祖先循环已简化、补充 2 测试两例已覆盖、补充 3 `active-index` 已补）
> 复核：`requirements/P088_复核意见.md`（通过；设计器独立核实挂载单点/传播点/递归风险）
> 实施备注：① `MenuPanel` 增 **weak `m_ownerBar`** + `setOwnerBar/ownerBar`（`addMenu` 单点设置；`setOpenSubMenu` 传播子菜单 + `addItem` 对已带子菜单 item 兜底——覆盖"先建后挂"）；② `closeMenuChain`：父链优先（离线/单测路径）+ **祖先循环取最近 MenuPanel 的 ownerBar 兜底**（复核 1 简化）；③ `MenuBar::getActiveIndex()` + `getIntProperty("active-index")` **只读**（新增 `kActiveIndex`，无命名冲突；menu.html/properties.html 注明）；④ 验证：**P0-88 探针 6/6**——1) 点击项→面板收起+bar 复位（active/hovered/menuMode 全 -1）+父级还原 ✓；2) 打开态面板 parent==bench（P0-71① 定位语义保持）✓；3) **两级子菜单**叶子点击→整链关闭+复位 ✓；4) 悬停切换后点击复位 ✓；5) 外部点击不回归 ✓；6) **ContextMenu 回归**（ownerBar=null → 兜底 no-op，零影响）✓；⑤ 全量构建 0 错误、回归仅 4 项预存；⑥ **附带观察（未改）**：`MenuItem::handleEvent` 的 MouseDown 关闭链位于 `m_onClick` 门控内——无 onClick 的普通项点击不关闭菜单（设计器菜单项均有动作，不受影响）；如需"无动作项也关闭"可另立项

---

## 1. 问题确认（源码核实，与设计器定位一致）

**复现**：点击菜单项（如 编辑→撤销）→ 下拉收起 ✓，但**主菜单标题保持点击态底色**、悬停切换模式仍激活——须再点一次屏幕才能恢复。

| 环节 | 事实 | 位置 |
|---|---|---|
| ① 关闭链查找 | `MenuItem::closeMenuChain()` 沿**父链**上溯找 MenuPanel/MenuBar | src/Menu.cpp:250-268 |
| ② 打开态父链断裂 | `MenuBar::attachMenuPanel`（P0-71①）为取得根复合把面板 **setParent(bench)**；打开态父链 = item→panel→**bench**→null → **永远到不了 MenuBar** | :1246-1258 |
| ③ 结果 | `closeWithChildren()` 仅 hide 面板/子菜单；**`bar->closeAllMenus()/exitMenuMode()` 不执行** → `m_activeIndex`/`m_menuMode`/`m_hoveredIndex` 残留、面板滞留 bench（未 detach、父链未还原） | :256-266、:1222-1240 |
| ④ 次生现象 | 点击别处由 before-handling watcher → `exitMenuMode` 补做（"再点一次才能操作"） | — |
| ⑤ 子菜单同链 | 子面板 parent=MenuItem（:113-121）→ item parent=面板 → 打开态同样经 bench 断链 ✗ | :113-121 |

## 2. 修改方案（采纳复核建议方案 A）

### 2.1 `MenuPanel` 增 owner 背指针（weak 防悬垂）
```cpp
// MenuPanel
std::weak_ptr<MenuBar> m_ownerBar;
void setOwnerBar(const std::shared_ptr<MenuBar>& bar) { m_ownerBar = bar; }
std::shared_ptr<MenuBar> ownerBar() const { return m_ownerBar.lock(); }
```
- `MenuBar::addMenu`：`panel->setOwnerBar(std::dynamic_pointer_cast<MenuBar>(getThis()))`；
- **子菜单传播**（覆盖设计器"先建子面板 + setSubMenu 再挂入"顺序）：`MenuPanel::setOpenSubMenu(sub)` 中 `sub->setOwnerBar(ownerBar())`（子面板 show 时传播；多层子菜单递归天然覆盖）；另在 `MenuPanel::addItem` 对已带子菜单的 item 顺带传播一次（兜底）。

### 2.2 `closeMenuChain` 父链优先 + owner 兜底
```cpp
void MenuItem::closeMenuChain() {
    Control* p = getParent();
    while (p) {
        if (auto* panel = dynamic_cast<MenuPanel*>(p)) { panel->closeWithChildren(); p = panel->getParent(); continue; }
        if (auto* bar = dynamic_cast<MenuBar*>(p)) { bar->closeAllMenus(); bar->exitMenuMode(); return; }
        p = p->getParent();
    }
    // P0-88：打开态父链经 bench 断裂 → owner 背指针兜底
    if (auto* selfPanel = dynamic_cast<MenuPanel*>(getParent())) {
        if (auto bar = selfPanel->ownerBar()) { bar->closeAllMenus(); bar->exitMenuMode(); return; }
    }
    // 子菜单项：item parent 为 MenuPanel（同上）；若 item 自身 parent 非面板（离线单测），逐层取最近面板
    for (Control* q = getParent(); q; q = q->getParent()) {
        if (auto* panel = dynamic_cast<MenuPanel*>(q)) {
            if (auto bar = panel->ownerBar()) { bar->closeAllMenus(); bar->exitMenuMode(); return; }
        }
    }
}
```
（实现取简：父链找到 bar 直接走；否则逐层找最近 MenuPanel 的 ownerBar 兜底——保留离线单测"面板父级为 bar"路径。）

### 2.3 只读断言钩子（复核建议 ③）
- `MenuBar` 增 `int getActiveIndex() const { return m_activeIndex; }` + `getIntProperty("active-index")` **只读**（新增常量 `kActiveIndex="active-index"`；与 `hovered-index` 只读同款）；menu.html/properties.html 注明。

### 2.4 不改动
- P0-71① bench 挂载定位语义（面板仍在 bench 下参与根复合）；
- `detachMenuPanel` 父级还原（`exitMenuMode` 触发后恢复 panel parent=bar）；
- P0-82 悬停/切换语义、P0-72 回放。

## 3. 测试计划（引擎探针 + 回归）
- **主路径**：MouseDown 打开"文件" → MouseDown 点击项 0 → 断言：面板不可见、`bar->isInMenuMode()==false`、`hovered-index==-1`、`active-index==-1`、面板 `getParent()==bar`（父级已还原）；
- **子菜单**：悬停打开子菜单 → 点击子项 → 整链关闭 + bar 复位；
- **悬停切换回归**（P0-72/P0-82）：打开→移相邻（hovered 转移）→点击项 → 复位；
- **外部点击/Esc**：watcher → exitMenuMode 复位（原路径不回归）；
- **定位回归**（P0-71①）：面板 rect 仍按 bench 局部换算（像素/坐标断言）；
- 全量回归（仅 4 项预存）+ 设计器手动复测（点击项即恢复，无需补点）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 方案 A（owner 背指针 weak + 父链优先兜底） | 确认（B 的 hide 通知递归风险大，不采用） |
| Q-2 | 子菜单 owner 传播点：`setOpenSubMenu`（+`addItem` 兜底） | 确认（覆盖"先建后挂"顺序） |
| Q-3 | 补 `active-index` 只读读回（探针/设计器断言） | 建议补（与 hovered-index 同款） |
| Q-4 | 父链路径保留（离线/单测兼容） | 确认 |
