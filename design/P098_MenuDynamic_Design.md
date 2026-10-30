# P098_MenuDynamic_Design — 菜单点击回调门控 + 动态化配套（动作回调实例上下文 / RemoveItem / caption 重算）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-17 追加：P0-98 高）
> 前置：P0-87（菜单项 id 注册）/ P0-88（菜单关闭链 ownerBar）已实施并同步
> 状态：**已放行，已实施**（2026-10-17；复核：`requirements/P098_099_100_复核意见.md`——通过）
> 复核勘误：`MenuPanel::getItems()` **已存在**（include/Menu.h:170）——§2.3"需补"作废。
> 实施备注（2026-10-17）：① ①b 按复核补丁逐字落地（Normal 无条件关链 + `m_onClick`/SetCallback 并存均触发）；② ActionThunk 落地（Impl::actions 载体调整；动作内 `ctl.GetId()`/`GetString()` 已验证）；③ `MenuPanelRemoveItem` 落地（id 注销 + 子菜单链 hide + 断父 + 重算；移除后句柄失效——Debug 下对失效句柄调用 API 会断言，属 P0-84c 既有语义）；④ setCaption 经 `getParent()` 定位面板重算（项在 addItem 时已 setParent，无需新成员）；⑤ 验证：引擎探针 `temp_p098_099_100_probe` 1~3 全过；**binding 探针** `temp_p098_binding_probe`（RegisterAction→GetId='btnA'/GetString='B1'）OK；全量回归仅 4 项预存。⑥ **后续语义变更（P0-103）**：click 回调改为**延迟一帧**执行（关链仍同步）——详见 `P103_MenuClickDeferred_Design.md`。

---

## 1. 复现与现状核实

| # | 断言 | 核实结果 |
|---|---|---|
| ①b | 动态菜单项 `SetCallback("click")` 永不触发 | ✓ `MenuItem::handleEvent`（src/Menu.cpp:236-243）：`closeMenuChain()` 与 `fireCCallback(kEventClick,...)` **均写在 `if (m_type==Normal && m_onClick)` 分支内** → CABI `CreateMenuItem` + `SetCallback` 创建项（无 `m_onClick`）点击后：回调不触发 **且菜单链不关闭** ✗ |
| ①b 附 | 与 API 头文档不符 | ✓ include/UICornerstoneAPI.h:383 明确"MenuItem 的 caption/checked/shortcut/click 走统一属性系统（SetString/SetBool/SetCallback，事件名 click）"；`m_onClick` 仅由 `setOnClick`（Menu.h:54，parser JSON 路径）设置 |
| ① | 动作回调 Control 无实例上下文 | ✓ binding/src/UICornerstone.cpp:1095 `state->instance = nullptr` → `UICornerstone_GetControlId`（API:2653 `if(!instance||!ctl) return ""`）→ 动作处理器内 `ctl.GetId()/GetString()` 恒失效 ✗ |
| ② | 无 `MenuPanelRemoveItem` CABI | ✓ 内部 `MenuPanel::removeItem` 存在（Menu.cpp:445：erase m_items + recalculateSize——**菜单项不在 m_children，removeItem 即完整移除**）；CABI 仅 AddItem/AddSeparator（API 头:394-396）✗ |
| ③ | `MenuItem::setCaption` 不触发面板重算 | ✓ Menu.cpp:101-103 仅赋值；项不在 m_children、无父链通知 → 运行时改标题宽度不更新 ✗（面板宽=最长标题实测） |

## 2. 方案

### 2.1 ①b click 门控（核心修复）
```cpp
if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
    if (m_type == MenuItemType::Normal) {
        closeMenuChain();
        if (m_onClick) m_onClick(dynamic_pointer_cast<MenuItem>(getThis()));
        fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
    }
    return true;
}
```
- Normal **无条件**关链 + 发 `click`（`m_onClick` 与 `SetCallback` 可并存、均触发）；Separator/SubMenu 行为不变；禁用项由前置 `getEnable()` 守卫拦截（P0-87 既有）。

### 2.2 ① 动作回调实例上下文（绑定层）
- `ActionCallback` 载体改包裹结构 `struct ActionThunk { ActionCallback cb; UIInstance inst; }`（`Impl::actions` 存 `shared_ptr<ActionThunk>`）；
- thunk 内 `state->instance = t->inst;`（注册时捕获 `m_impl->instance`；thunk 由 impl 持有，实例存活期内有效）。

### 2.3 ② MenuPanelRemoveItem（CABI + Binding）
```c
int UICornerstone_MenuPanelRemoveItem(UIInstance instance, UIControlHandle panel, UIControlHandle item);
```
- 校验 `panel→MenuPanel*`、`item→MenuItem*`；在面板项列表按指针查找 shared_ptr → `removeItem`；项有 id 已注册 → 同步注销（`SetControlId(inst,item,"")` 语义）；项持有打开的子菜单 → 先 hide 子菜单链；返回 1/0；
- 需补 `MenuPanel::getItems()` 访问器（若无）；
- **句柄语义：移除后该 item 句柄失效**（shared_ptr 释放）——宿主重建流程 = 逐个 Remove → 重新 AddItem；"保活往返"（同 TabRemovePage 模式）如后续需要另批（Q-2）。

### 2.4 ③ setCaption 重算
- `MenuItem` 增 `MenuPanel* m_ownerPanel`（`addItem` 设置、`removeItem` 清空）；
- `setCaption`：值变化且有 owner → `m_ownerPanel->recalculateSize()`（可见时联动重排/重钳制——与 P0-99 配合）；
- parser 构造期 `setCaption` 在 `addItem` 前 → owner 空 → 无副作用 ✓。

## 3. 验收与测试矩阵
1. **动态项 click**：CABI 创建项 + `SetCallback("click")` → 合成点击 → 回调 1 次 **且菜单链关闭**（`active-index`/menuMode 复位）；
2. **parser 项不回归**：`m_onClick` 路径原语义保持（并存时双触发——文档注明）；
3. **RegisterAction 上下文**：动作内 `ctl.GetId()` 返回真实 id、`GetString("caption")` 可读；
4. **RemoveItem**：移除后项数 -1、面板尺寸重算、`FindControl(项 id)` 为空、点击不崩溃；随后 AddItem 正常；
5. **setCaption**：运行时改长标题 → 可见面板宽度立即变化；
6. **回归**：P0-88 菜单探针 6 用例 + 全量回归（仅 4 项预存）。

## 4. 影响面
- 源码：`src/Menu.cpp/.h`、`src/UICornerstoneAPI.cpp/.h`、`binding/src/UICornerstone.cpp` + `Impl.h` + DynamicApi；
- 文档：capi/binding 速查增 RemoveItem 行；Menu 文档段 click 语义澄清（`m_onClick` 与 SetCallback 关系）；菜单相关控件页（menubar 章节如有）；
- 设计器：实施同步后可回收 GetProcAddress 直调过渡 → 改回全动态重建。

## 5. 待确认（Q）
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | Normal 项同时有 `m_onClick` 与 click 回调 → 均触发 | 采纳（语义统一，文档注明） |
| Q-2 | RemoveItem 句柄语义：移除即失效 | 采纳（保活往返后续按需另批） |
| Q-3 | setCaption 可见时联动重钳制（P0-99） | 采纳 |
