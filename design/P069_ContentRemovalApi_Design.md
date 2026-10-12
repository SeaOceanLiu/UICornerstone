# P069_ContentRemovalApi_Design — 内容类移除 API 暴露（MenuBar 菜单移除 / TabControl 页移除）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-05 追加：P0-69）
> 前置：P0-68 批已实施并同步
> 状态：**已放行，已实施**（2026-10-06；四问全部确认）
> 复核：`requirements/P069_ContentRemovalApi_Design_复核意见.md`（通过）
> 实施备注：① 引擎 `MenuBar::detachMenu(caption)`（removeMenu 复用）；② CABI `UICornerstone_MenuBarRemoveMenu`（摘除后 menuPoolKeep 保活，原句柄可再挂载）+ `UICornerstone_TabRemovePage`（全摘除 tab/bench + hide + detachedControls 保活）；③ `UICornerstone_TabSetPage` 对齐（detachPoolTake + 旧父摘除 + 替换旧页全摘除保活——修复 bench+tab 双挂载鬼影）；④ Binding `MenuBarRemoveMenu`/`TabRemovePage`（int）；⑤ ③ MenuPanel 清空按 Q-3 暂缓；⑥ 测试：test_property_cabi 178→**191/0**（移除/再挂载/句柄有效/越界）；⑦ 回归仅 4 项预存；⑧ 文档 menu/tabcontrol/capi/binding 同步

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-69① | MenuBar 菜单移除未暴露 | **确认** | `MenuBar::removeMenu(caption)` 内部已有（Menu.h:273；src/Menu.cpp:1062-1071：按 caption 查首匹配 → erase → layoutEntries），CABI 仅 `CreateMenuBar` / `MenuBarAddMenu`（UICornerstoneAPI.h:382-390），Binding 仅 `CreateMenuBar` / `MenuBarAddMenu`；结构化 `menus` 重编辑无法移除旧菜单 |
| P0-69② | TabControl 页移除未暴露 | **确认** | `TabControl::removeTab(index)` 内部已有（TabControl.h:34；src/TabControl.cpp:89-99：removeControl(page) → erase → index 钳制 → relayout → applyCurrentPage），CABI 仅 `TabAddPage`（src/UICornerstoneAPI.cpp:1732）+ `TabSetPage`/`TabSetTitle`，无移除 |
| P0-69③ | （可选）MenuPanel 清空/移除项 | **确认（内部有，未暴露）** | `MenuPanel::removeItem(shared_ptr<MenuItem>)`（Menu.h:134）内部存在、无索引/清空形态；设计器可每菜单重建 panel 规避（见 3.6） |

## 2. 生命周期/挂载风险分析（先于 API 设计）

直接转发内部移除会引入句柄悬垂/鬼影，必须在 CABI 包装层处理：

| 对象 | 归属链 | 直接移除的后果 |
|---|---|---|
| **MenuPanel** | `CreateMenuPanel` → `menuPoolKeep`（src/UICornerstoneAPI.cpp:1300-1306）→ `MenuBarAddMenu` `menuPoolTake`（:1326）→ 由 `MenuBar::m_entries` 持有 | `removeMenu` 直接 erase entry → **panel 失去全部持有者 → 析构 → 设计器持有的 `UIControlHandle` 悬垂**；且原句柄无法再 `MenuBarAddMenu`（已不在 menuPool，`menuPoolTake` 返回空） |
| **Tab 页（CABI 创建）** | `CreatePanel` 先挂 `bench`（src/UICornerstoneAPI.cpp:1256-1257）→ `TabSetPage` → `setTabPage`（setParent + addControl，src/TabControl.cpp:127-136）→ **page 同时在 bench 与 tab 两个孩子列表（双挂载潜在问题：`setParent` 不摘旧父 children，见 src/ControlBase.cpp:429-441）** | `removeTab` 仅 `removeControl`（tab 列表）→ page 因 bench 持有而存活（句柄不悬垂），但**仍留在 bench children 且可能可见 → 移除后鬼影绘制** |
| **Tab 页（JSON 创建）** | `parseControl(page, nullptr)` → `addTab`（仅 tab 持有） | `removeTab` 后 shared_ptr 归零 → 销毁；无 CABI 句柄场景安全 |

**结论**：移除类 API 需定义为"**全摘除 + 保活**"：
1. 全摘除：从当前父（tab/bar）与残留容器（bench）移除 children 引用、`setParent(nullptr)`、`setVisible(false)`；
2. 保活：放入实例保活池（`detachedControls` 或 `menuPool`），句柄持续有效、可重复挂载；
3. 顺带对齐 `TabSetPage` 挂载路径（消除既有的 bench+tab 双挂载；与 P0-50 `AddChildControl` 的旧父自动摘除同款守卫）。

## 3. 修改方案

### 3.1 引擎：MenuBar 摘除原语
- 新增 `shared_ptr<MenuPanel> MenuBar::detachMenu(const string& caption)`：按 caption 查首匹配 → 取出 panel → erase → layoutEntries → 返回（未找到 nullptr）；
- `removeMenu(caption)` 改为调用 `detachMenu` 并忽略返回值（行为不变，内部复用）。

### 3.2 CABI ①：`UICornerstone_MenuBarRemoveMenu`
```c
int UICornerstone_MenuBarRemoveMenu(UIInstance instance, UIControlHandle bar, const char* caption);
```
- 语义：validate → `detachMenu` → 命中则 `menuPoolKeep(instance, panel)` 保活 → 返回 1；未命中 0；参数非法 0；
- 保活后可经 `MenuBarAddMenu` **原句柄再挂载**（menuPoolTake 恰好恢复），设计器重建/复用两便；
- Binding：`int MenuBarRemoveMenu(Control& bar, const std::string& caption)`（对齐 TabAddPage 的 int 风格；Q-4）。

### 3.3 CABI ②：`UICornerstone_TabRemovePage`
```c
int UICornerstone_TabRemovePage(UIInstance instance, UIControlHandle tab, int index);
```
- 语义：validate（tabControlOf + index ∈ [0,count)）→ 取 `getTabs()[index].page`（public getter）→ `removeTab(index)` → 若 page 非空：`tab->removeControl(page)`（幂等）、`instance->bench->removeControl(page)`（清残留双挂载）、`setParent(nullptr)`、`setVisible(false)`、`detachPoolKeep(instance, page)` → 返回 1；越界 0；
- 保活页可经 `TabSetPage` 原句柄再挂载；JSON 页（无外部句柄）保活至实例销毁，避免移除中的回调悬垂；
- Binding：`int TabRemovePage(Control& tab, int index)`。

### 3.4 对齐 `UICornerstone_TabSetPage`（消除双挂载）
- 挂载前：`detachPoolTake(instance, page)`（池内页移出）+ **旧父摘除**（`oldParent != nullptr → oldParent->removeControl`；否则 `bench->removeControl`），与 `AddChildControl`（src/UICornerstoneAPI.cpp:2096-2107）同款；
- 替换已有页时（`setTabPage` 内部会 removeControl 旧页）：旧页同样全摘除 + hide + `detachPoolKeep`（避免旧页残留 bench 鬼影；Q-2）。

### 3.5 Binding 新增
| C++ Binding | C ABI |
|---|---|
| `int MenuBarRemoveMenu(Control& bar, const std::string& caption)` | `UICornerstone_MenuBarRemoveMenu` |
| `int TabRemovePage(Control& tab, int index)` | `UICornerstone_TabRemovePage` |

- 实现点：binding/include/UICornerstone.h、binding/src/DynamicApi.h（函数指针）、binding/src/DynamicApi.cpp（RESOLVE）、binding/src/UICornerstone.cpp。

### 3.6 ③ MenuPanel 清空（可选）
- **建议暂缓**：实现 ① 后设计器可"移除菜单 → 旧 panel 保活 → 新建 panel 重建"闭环；`ClearItems`/`RemoveItem(index)` 不阻塞（Q-3）。

## 4. 测试计划
- **菜单（C ABI 探针/测试）**：AddMenu(captionA, panelA)、AddMenu(captionB, panelB) → `RemoveMenu(captionA)`=1 → 重复=0 → 断言 panelA 句柄仍有效（`SetString`/`SetRect` 成功）、`getMenuPanel(0)` 后移 → 原句柄 `MenuBarAddMenu` 再挂载成功（menuPool 往返）→ 移除未知名=0；
- **Tab（C ABI 探针/测试）**：AddPage×3 + SetPage(句柄A/B) → `RemovePage(1)`=1 → count-1；断言页 B 句柄仍有效、`getParent()==nullptr`、`visible==false`、bench 无鬼影（重绘无残留）→ 同句柄 `TabSetPage` 再挂载 → count 恢复、当前页钳制正确；越界/负索引返回 0；JSON 页移除后布局重载正常；
- **回归**：test_menu、test_tabcontrol、test_property_cabi、test_overlay_viewport_cabi + 全量（仅 4 项预存可接受）。

## 5. 文档
- docs/controls/menu.html（移除 API + 保活/再挂载语义）、docs/controls/tabcontrol.html（页移除 + 双挂载修复说明）；
- docs/appendix/capi.html、docs/appendix/binding.html 速查表追加两行；
- README 如涉及 API 清单则同步。

## 6. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 是否同时提供 `MenuBarClearMenus`？ | 仅 `MenuBarRemoveMenu`（caption 遍历足够；如需清空另议） |
| Q-2 | `TabSetPage` 替换旧页清理（全摘除+保活）是否纳入本批？ | **纳入**（与移除同根因：残留 bench 鬼影） |
| Q-3 | ③ MenuPanel 清空是否暂缓？ | 暂缓（不阻塞；① 后重建路径已通） |
| Q-4 | Binding 返回类型：`int`（判定成败） | 确认（Menu 侧 AddMenu 为 void，Remove 建议 int） |
