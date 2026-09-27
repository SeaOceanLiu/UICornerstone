# PopupFactories_HoverChain_ImageButton_Design — Popup 工厂 + 编辑态常显 + hover 链路 + ImageButton 移除

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-23 联测收尾）——**P0-27d**、**P0-28**（含扩围）、**P0-30**、**P0-31**
> 前置结论：P0-27b 已结案（设计器接受引擎不复现，占位映射所致）；P0-27c 已实施（`text` 键直达 + 自动切 custom，见 properties.html）
> 状态：**已放行，已实施**（复核：`requirements/PopupFactories_HoverChain_ImageButton_Design_复核意见.md`，2026-09-23 通过——六问全确认 + P0-30 补充"pressed 基类对称切态"维度）
> **实施记录（2026-09-23）**：
> 1. **P0-27d**：新增 C ABI `CreatePopup`/`CreateConfirmPopup`（保持隐藏，不自动 open；`visible` 属性控制显隐；popupPool 生命期）+ Binding 2 工厂 + DynamicApi 2 项。**勘误**：`CreateConfirmPopup` **无 cancelText 参数**（ConfirmPopup 仅确认按钮，签名按实际 API 收敛）。
> 2. **P0-28**：确认能力已存在（`close-on-click-outside`/`close-on-esc`/`visible` 在 Popup/ConfirmPopup/Dialog/ContextMenu 均可设，ContextMenu 继承 Popup 未 override）——文档固化编辑态用法（properties.html 弹窗小节 + contextmenu.html），test_p0_getter §12 覆盖工厂 + 常显键读写。
> 3. **P0-30**：实现采用**StateColor 缺省构造调整**替代"显式设置标志"（更简洁且读写一致）：`StateColor(Type)` 的 hover/pressed 缺省 = normal（disabled 保留缺省灰）；Button/CheckBox/Label 构造时显式写入历史缺省 hover/pressed（视觉不变）；基类 `onMouseEnter/Leave` 默认切 Hover/Normal + `applyPressState`（基类 handleEvent 自身命中 + EditBox 点击路径显式调用）；EditBox/ScrollBar 链基类。**追加根因**：`EditBox::update()` 未调用 `ControlImpl::update()` → EditBox 族 hover 检测从未运行（已修复，NUD hover 实测转绿）。探针实测：panel/actor/nud hover ✓、button 历史 hover 蓝保持 ✓、panel pressed ✓。
> 4. **P0-31**：C ABI `CreateImageButton` + Binding + DynamicApi 删除；LayoutParser `image-button` 降级 button + Warn（常量保留至下批）；测试改造（fromsource/resourceprovider → 四态图 Button 等价用例；sample_viewport_scale 同步）；文档 6 页同步（类型 27→26、ImageButton 小节移除、工厂行移除）。
> 5. **回归**：全量测试扫描剩余失败 = menu_cabi/treeview_cabi（长期预存）+ multiviewport（F1；经**发布版 DLL 对照确认为预存**，与本批无关）。

---

## 1. 问题与目标

| # | 优先级 | 需求 | 目标 |
|---|---|---|---|
| P0-27d | 低 | binding 补 `CreatePopup`/`CreateConfirmPopup` 工厂 | 设计器 tPopup/tConfirm 工具项接入真实控件（现为 Panel 占位） |
| P0-28 | 中 | Dialog 编辑态常显（dismiss 抑制） | 编辑态不因点击画布其它控件而关闭；范围 Dialog + ContextMenu（+未来 Popup/ConfirmPopup） |
| P0-30 | 中 | 各控件 hover/pressed 状态机链路核查 | `colors` 四态对所有声明控件**真实可达**（消除"声明 4 态但 hover 恒不可达"） |
| P0-31 | 中 | ImageButton 全面移除 | 四态图资源已覆盖图片按钮语义；移除冗余类型全链（AnimatedButton 保留） |

## 2. 现状核实（源码事实）

| 项 | 事实 | 结论 |
|---|---|---|
| 弹窗工厂 | C ABI 仅 `UICornerstone_CreateDialog`（自动 `open()` + `popupPool` 生命期，UICornerstoneAPI.cpp:2058-2074）；binding 仅 `CreateDialog` | **缺** Popup/ConfirmPopup 工厂（P0-27d 确认） |
| Popup 打开通道 | `Popup::setBoolProperty(kVisible)` → `open()/close()`（Dialog.cpp:434-441）；`kPopupVisible` 只读查询；schema popup/confirm-popup/dialog def 均含 `visible` | 无需新 API 即可控制显隐 |
| dismiss 抑制 | `Popup::setBoolProperty(kCloseOnClickOutside)`（Dialog.cpp:437）+ 外部点击关闭受 `m_closeOnClickOutside` 门控（:238）；ContextMenu 继承 Popup 且**未 override** `setBoolProperty`/`onOutsideClicked` | **能力已存在**（P0-28 主要是文档/联测 + 设计器用法） |
| hover 链路 | `ControlImpl::onMouseEnter/Leave` 默认空实现（ControlBase.cpp:663-670）；Hover 态仅 Button/CheckBox/Label 自实现（Button.cpp:209/CheckBox.cpp:315/Label.cpp:417）；EditBox::onMouseEnter 仅记 `m_mouseInside`（:556） | **基类不切态** → Actor/LuotiAni/ColorPicker/NUD/ListView/TreeView/ProgressBar/StatusBar/TabControl/Slider/Shape/Panel/Splitter 等 hover 不可达（P0-30 确认） |
| hover 检测路径 | `ControlImpl::update()` 用 `drawRect.contains(mouse)` 判定（ControlBase.cpp:155-176），**不依赖** `isContainsPoint` | Actor/LuotiAni 虽 `isContainsPoint=false`（不参与点击/遮挡），但**可达 hover 检测** → 基类切态即可生效，无需改命中语义 |
| 缺省态色 | `StateColor` 各态独立缺省：bg hover 蓝(100,149,237)/pressed/disabled 灰；text hover 灰(128) 等（ConstDef.cpp:13-30） | 若"全控件切 hover"而不改语义 → 未设色控件 hover 变蓝（不可接受） |
| 既有 hover 视觉 | Button hover = 缺省蓝（无显式设色，`BUTTON_HOVER_COLOR` 常量未被引用）；Label hover 文本 = 缺省灰 | 状态语义调整需为这些控件**显式保留**历史缺省 |
| ImageButton | **无独立类**；= Button + 四态 Actor 工厂：C ABI `UICornerstone_CreateImageButton`（:1257）、binding `CreateImageButton`（+DynamicApi）、LayoutParser `image-button` 分支（:269，与 button 同语法）；常量 `kControlTypeImageButton`（PropertyNames.h:599）；测试 test_fromsource_cabi / test_resourceprovider_cabi；schema **无** image-button def ✓ | 移除清单明确（P0-31） |
| AnimatedButton | C ABI `UICornerstone_CreateAnimatedButton` + binding `CreateAnimatedButton` 独立存在 | **保留**（语义不同） |

## 3. 设计决策

### 3.1 P0-27d：Popup/ConfirmPopup 工厂（C ABI + Binding）

```cpp
// 新增 C ABI（语义与 CreateDialog 对齐，唯一差异：不自动 open）
UICORNERSTONE_API UIControlHandle UICornerstone_CreatePopup(
    UIInstance instance, float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateConfirmPopup(
    UIInstance instance, const char* confirmText, const char* cancelText,
    float x, float y, float w, float h, float xScale, float yScale);
```
- 均：`make_shared<Popup/ConfirmPopup>(bench, ...)` → `addControl` → `create()`（保持隐藏，`visible` 属性控制显隐）；`popupPool` 生命期（同 Dialog）。
- **不自动 open**（决策点 §7-1）：Popup 是通用浮层，运行态由调用方按需打开；CreateDialog 保持既有自动 open（向后兼容）。
- Binding：`CreatePopup(...)` / `CreateConfirmPopup(confirmText, cancelText, ...)` 包装 + DynamicApi `fnCreatePopup/fnCreateConfirmPopup` + RESOLVE。
- 文档：capi.html 工厂行 + binding.html 工厂表 + popup/confirm-popup 控制页（如缺）。

### 3.2 P0-28：编辑态常显（能力确认 + 用法固化，不新增 API）

- **结论**：`close-on-click-outside`（Popup/ConfirmPopup/Dialog/ContextMenu 均可设）+ `visible` 已构成完整能力；编辑态用法：
  ```cpp
  // 编辑态创建
  auto dlg = CreateDialog(...);          // 或 CreateConfirmPopup/CreatePopup
  SetBool(dlg, "close-on-click-outside", 0);
  SetBool(dlg, "close-on-esc", 0);       // 可选
  // 显隐：SetBool(dlg, "visible", 1/0)
  ```
- 补齐：文档（dialog.html/popup 页/速查）+ 回归测试（设 false 后外部点击不关闭；设 true 关闭）。
- 不新增实例级标志（避免双轨语义）；若设计器后续仍要"画布全局抑制"，另立需求（§7-2）。

### 3.3 P0-30：hover/pressed 链路（决策：状态语义增强 + 基类默认切态）

**根因**：基类 `onMouseEnter/Leave` 空实现 → 仅 4 个控件自实现切态；`update()` 的 hover 检测本身对全部控件可用。

**方案 A（推荐）**：
1. **StateColor 增"显式设置"标志**（normal/hover/pressed/disabled 各一）；`ControlImpl::resolveStateColor` 未显式设置态 → **回退 normal**（语义：未设态=normal）。
2. **历史缺省显式化**（保持既有视觉）：Button（bg/border hover=100,149,237、pressed/disabled 对应缺省）、Label（text/text-shadow 各态缺省）、CheckBox（同 Button）在构造时显式写入原缺省态色（值取自 ConstDef 现有常量）。
3. **基类默认切态**：`ControlImpl::onMouseEnter` → 若 enabled 且非 Pressed/Disabled 则 `setState(Hover)`；`onMouseLeave` → 若 Hover 则 `setState(Normal)`；`setEnable(true)` 恢复时按鼠标位置（沿用现有 update 逻辑）。
4. **EditBox 链**：`EditBox::onMouseEnter/Leave` 保留 `m_mouseInside` 并调用基类默认（EditBox/NUD/ComboBox/TextArea 获得 hover；未设 hover 色 → 回退 normal，视觉不变）。
5. **Actor/LuotiAni**：不改 `isContainsPoint`（保持不参与点击/遮挡）；基类切态后 hover 可达 ✓。
6. 已自实现控件（Button/CheckBox/Label）保留现有 override（与基类行为一致，无冲突）。

**方案 B（备选）**：保持缺省态色语义，仅对目标控件逐个开启 hover 并在 ctor 初始化 hover/pressed/disabled=normal。代码量更大（~16 控件），零语义变化。

**影响（方案 A）**：全控件 hover 可达；未显式设态色的控件视觉不变（回退 normal + 历史缺省显式化）；`state=hover` 属性路径同样回退 normal（未设时）。**风险**：任何依赖"缺省 hover 色"的第三方（仅引擎内部 Button/Label 等，已显式化）——可控。

### 3.4 P0-31：ImageButton 移除（决策：解析降级 Warn 一个版本，其余直接删）

| 移除项 | 处置 |
|---|---|
| C ABI `UICornerstone_CreateImageButton`（声明 + 实现） | 删除 |
| binding `CreateImageButton`（header/impl/DynamicApi fn+RESOLVE） | 删除 |
| LayoutParser `image-button` 类型分支（:269） | **降级**：按 `button` 解析（同语法）+ `logWarn("image-button 已移除，按 button 处理；请迁移为 button + 四态图")` |
| `kControlTypeImageButton`（PropertyNames.h:599） | 删除（解析降级用字面量或保留常量至下批？→ 建议本批保留常量仅用于降级识别，随下批删除） |
| 测试 test_fromsource_cabi / test_resourceprovider_cabi | 改为四态图 Button 等价用例（或移除 ImageButton 段） |
| 文档（declarative-syntax 27 种 → 26 种、properties ImageButton 小节、button.html、README 类型表） | 同步移除/改为"button + 四态图" |
| schema | 无 def ✓ 无需动 |
| **AnimatedButton** | **保留**（C ABI/binding/解析 animated-button 分支不变） |

## 4. API 设计

### 4.1 C ABI（新增 2 个）
`UICornerstone_CreatePopup` / `UICornerstone_CreateConfirmPopup`（§3.1）。

### 4.2 Binding（新增 2 个）
`CreatePopup` / `CreateConfirmPopup` + DynamicApi 2 项。

### 4.3 移除（P0-31）
`UICornerstone_CreateImageButton` + binding `CreateImageButton`（属破坏性删除，无正式外部依赖——设计器已同步清理；引擎侧测试同步改）。

## 5. 实现要点与验收

| # | 验收 | 测试 |
|---|---|---|
| 1 | `CreatePopup/CreateConfirmPopup` 创建成功（隐藏）；`SetBool("visible",1)` 显示、`0` 关闭；`close-on-click-outside=0` 时外部点击不关闭 | 新 C ABI 测试（test_popup_cabi 或并入 test_dialog_cabi） |
| 2 | 编辑态常显：Dialog/ContextMenu 设 `close-on-click-outside=0` 后外部点击不消失；=1 恢复 | 同上 + test_contextmenu 扩展 |
| 3 | hover 可达：Actor/LuotiAni/ColorPicker/NUD/ComboBox/TextArea/ListView/TreeView/ProgressBar/StatusBar/TabControl/Slider/Shape/Panel 设 `colors.background.hover` → 鼠标移入变色（探针像素） | 新探针 + test_capture_cabi 扩展 |
| 4 | 未设态色控件 hover 视觉不变（回归） | 全量像素回归（capture/colorfixes 等） |
| 5 | `image-button` 布局降级为 button + Warn；`CreateImageButton` 符号消失 | test_layout 扩展（含 Warn 断言）+ fromsource/resourceprovider 测试改造 |
| 6 | 三后端编译 0 错误；全量回归（预存失败除外） | ALL_BUILD + 全量测试 |

## 6. 影响面与风险

- **P0-27d**：纯增量；Popup 不自动 open 的语义与 Dialog 不同（文档注明）。
- **P0-28**：无代码风险（能力已存在）；文档/联测为主。
- **P0-30**：状态语义调整（未设态回退 normal）影响面广——已用"历史缺省显式化"对冲；需像素回归确认 Button/Label/CheckBox/EditBox 视觉不变；`state` 属性读写语义同步（未设态读回 normal）。
- **P0-31**：C ABI 删除属破坏性（设计器已清理，无其他使用方）；解析降级保留一个版本兼容既有布局。

## 7. 待审核问题

1. **CreatePopup 是否自动 open**：建议**不自动**（运行态按需；编辑态由设计器 `visible=1` 常显）；CreateDialog 保持自动 open——确认？
2. **P0-28 是否需要实例级抑制**：建议先按控件属性（`close-on-click-outside`）落地，不引入实例级双轨——确认？
3. **P0-30 状态语义**：接受"未显式设置的状态回退 normal"（方案 A，含历史缺省显式化）？或保守方案 B（逐控件开启）？
4. **P0-30 范围**：`pressed` 同样处理（基类按下态切 Pressed？现 Button 有按下逻辑；建议基类仅切 Hover，Pressed 保持各控件自实现）——确认？
5. **P0-31 降级周期**：`image-button` 解析 Warn 降级保留一个版本（推荐）还是本批直接删？
6. **P0-31 测试处置**：fromsource/resourceprovider 的 ImageButton 段改为"四态图 Button"等价用例（推荐）还是删除该段？

## 8. 待提交配套改动（实施时随批）

- C ABI/binding/DynamicApi（新增 2 + 移除 1）；LayoutParser 降级；PropertyNames 清理。
- 文档：capi.html/binding.html 工厂表；declarative-syntax 类型清单（27→26）与 button 段；properties.html ImageButton 小节移除；popup/dialog/contextmenu 编辑态用法说明。
- 测试：popup 工厂/常显/ hover 探针与像素回归；fromsource/resourceprovider 改造。
- requirements/ 归档（本批清单）；复核意见归档后 make_release 同步 CornerstoneDesigner。
