# CR20261110003 — Button 右键 context-menu 被吞 / 全控件右键派发（分析 + 设计）

> 状态：**已关闭（2026-11-10；设计器联调通过）**——子控件菜单优先/父兜底；派发边界统一预检查
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261110003）
> 关联：ControlImpl 右键 context-menu、CR20261109003（ContextMenu items）、全控件事件派发

## 1. 问题与根因

`ControlImpl::handleEvent` 已实现“右键命中 + `m_contextMenu` + 未显示 → show”，但该逻辑位于基类。部分控件 override `handleEvent` 后，右键路径在进入基类前就消费/返回，导致绑定失效。

Button 示例：

```cpp
if (gotPos) {
    if (drawRect.contains(mx, my)) {
        // left/right/move 分支...
        return true;           // 右键也在这里返回，基类 context-menu 永远不执行
    }
}
...
if (ControlImpl::handleEvent(event)) return true;  // 已经太晚
```

Label/Panel 等先调用基类，所以同样绑定可弹出；这是“控件的 override 差异”，不是 ContextMenu 本身的问题。

## 2. 全控件审计（引擎源码 `::handleEvent`）

| 控件/组 | 右键路径 | 原状态 |
|---|---|---|
| Button | 命中 drawRect 后末尾 `return true`，未进入基类 | ❌ 吞掉 |
| EditBox | 只处理左键/键盘，末尾 `return false`，未调用基类 | ❌ 不触发自身菜单 |
| ComboBox / NumericUpDown / TextArea / InlineEditBox | 继承/转调 EditBox，同上 | ❌ |
| ListView | 只处理左键分支，末尾 `return false`，未调用基类 | ❌ |
| Slider | 左键/键盘分支，末尾未进入基类 | ❌ |
| ScrollBar | 左键/滚轮分支，末尾 `return false` | ❌ |
| Splitter | 左键拖拽分支，末尾 `return false` | ❌ |
| HandleControl | 手柄左键/移动分支，末尾 `return false` | ❌ |
| MenuBar / MenuPanel / MenuItem | 菜单自身消费右键/未转调基类（非典型绑定目标） | ⚠️ 不一致 |
| Panel / Bench | 先 `ControlImpl::handleEvent` | ✅ |
| Label | 先基类 | ✅ |
| CheckBox | 非左键状态分支，末尾基类 | ✅ |
| ColorPicker | `Panel::handleEvent` → 基类 | ✅ |
| TreeView | 末尾基类 | ✅ |
| TabControl | 末尾基类 | ✅ |
| StatusBar | 末尾基类 | ✅ |
| WinFrame | Step 3 `ControlImpl::handleEvent` | ✅ |
| ProgressBar | 直接基类 | ✅ |
| Shape / Actor / Material / Image / Animation | 不 override 或基类 | ✅ |
| Popup / ConfirmPopup / Dialog | `Panel::handleEvent` / `Popup::handleEvent` | ✅（普通事件） |
| ContextMenu | 浮层自身；不是绑定目标 | — |

结论：**不能只改 Button**；需要把右键 context-menu 尝试放到“事件派发边界”，对所有 `Control` 统一生效，避免未来新增控件重蹈覆辙。

## 3. 方案选择

### 方案 A（采用）：基类提取 helper + 子控件菜单优先/父兜底

1. 从 `ControlImpl::handleEvent` 抽出：
   ```cpp
   bool ControlImpl::tryOpenContextMenu(const shared_ptr<Event>& event) {
       if (!m_contextMenu || !event) return false;
       if (event->m_type != EventType::MouseDown ||
           event->mouseButton.button != MouseButton::Right) return false;
       if (!getVisible() || !getEnable()) return false;
       float mx = event->mouseButton.x, my = event->mouseButton.y;
       if (!isContainsPoint(mx, my)) return false;
       if (m_contextMenu->isPopupVisible()) return true;   // 已打开视为已处理
       m_contextMenu->show(mx, my);
       return true;
   }
   ```
2. `ControlImpl::handleEvent` 对右键 MouseDown 采用**子控件优先**：
   ```text
   if Right MouseDown:
       if tryDispatchContextMenuToChild(event): return true   // 只尝试子控件菜单
       if tryOpenContextMenu(event):            return true   // 子无菜单时父兜底
   // 其后按原有子控件派发/自身逻辑处理非菜单右键
   ```
3. 新增 `tryDispatchContextMenuToChild(event)`：按 `dispatchToChild` 的逆序/遮挡规则扫描子控件，但**只调用** `ControlImpl::tryOpenContextMenu`，不调用子控件完整 `handleEvent`：
   - 子控件有菜单 → 打开并消费，父不再尝试；
   - 子控件无菜单 → 继续；全部子控件无菜单才轮到父自身 `tryOpenContextMenu`；
   - 父/子同时绑定 → **子控件优先**，父菜单不打开；
   - 已打开（`isPopupVisible`）→ 返回 true，不重复 `show`、不关闭/重开；
   - 不会先调用子 `handleEvent` 再调用菜单 helper，避免重复/双开。
4. Bench 的根级控件也走 `Bench/Panel` 的派发路径，因此 Button/EditBox/ListView 等根控件同样覆盖。
5. **不逐个改** Button/EditBox/ListView/… 的 handleEvent；避免遗漏和未来新增控件回归。Button 的 `handleEvent` 可保持原样，生产链路由派发边界先消费右键。

### 方案 B（排除）：逐控件在 handleEvent 入口补 `ControlImpl::handleEvent`

- 需审计 26 类以上及未来新增控件；
- 控件内部可能希望在右键时做别的事，顺序难统一；
- 容易遗漏（本次已发现 Button 之外的多个控件）。

### 方案 C（排除）：Bench 全局命中扫描

- 需要重做坐标、遮挡、子视口、焦点/捕获逻辑；
- 与现有 dispatchToChild 重复，风险高。

**定稿：方案 A。**

## 4. 语义与边界

- 仅当控件绑定了 `m_contextMenu` 且右键落在其命中区、菜单未显示时才打开；
- 右键不会再触发 Button 左键 click/pressed 状态；事件在派发边界被消费；
- 父/子同时绑定：**子控件优先**；子控件无菜单时父菜单兜底；已打开菜单不重复 show；
- 未绑定 context-menu 的子控件/控件仍走原 `handleEvent`；没有菜单可弹，不改变其右键行为；
- ContextMenu `show` 的坐标仍为屏幕坐标，缩放/多视口沿用现有逻辑；
- `isPopupVisible` 时返回 true，避免二次打开；同一 Right MouseDown 只 `show` 一次；与 Popup 关闭/外部 watcher 的语义不变；
- 未绑定 `m_contextMenu`：完全走原 `handleEvent`；Button 右键不触发左键 click/pressed、不改变状态；
- 不新增 C ABI/Binding/JSON/schema API；`SetPtr("context-menu", ...)` 不变。

## 5. 测试/验收

### 5.1 全控件矩阵

对以下控件在可交互位置绑定同一 ContextMenu，右键应打开（覆盖评审 B4 清单）：

- 容器/基础：Panel、Label、CheckBox、Button；
- 输入族：EditBox、ComboBox、NumericUpDown、TextArea、InlineEditBox（内部）；
- 列表/树：ListView（含行/列头）、TreeView、TabControl；
- 取值/拖动：Slider、ScrollBar、Splitter、HandleControl；
- 状态/框架：StatusBar、WinFrame、ProgressBar、ColorPicker；
- 图形：Shape、Actor/Image、Animation；
- 弹窗：Popup、ConfirmPopup、Dialog 内控件（弹窗目标边界按现有行为）；
- MenuBar/MenuItem：仅在明确绑定 context-menu 时验证，不改变菜单自身点击优先。

每个控件至少检查：

- 右键未打开 → 打开一次；已打开 → 不重复 show/不关闭重开；
- 父/子同时绑定 → 子菜单优先；子无菜单 → 父菜单兜底；
- 未绑定菜单的控件右键仍走原行为（Button 不触发左键 click/pressed）；
- 缩放/多实例/捕获期间右键不崩溃；
- 左键 click/拖动/选择/聚焦不回归。

测试要求：

1. 右键打开、菜单可见、仅一次；
2. 再次右键/已打开时不重复；
3. 左键行为不回归（Button click、Slider 拖动、ListView 选择、EditBox 聚焦等）；
4. 未绑定 context-menu 的控件右键不改变状态/不触发 click；
5. 父/子同时绑定：子控件菜单优先、父兜底；记录实际结果；
6. 缩放/视口、多实例、捕获期间右键不崩溃；
7. C ABI `SetPtr("context-menu")` 与 C++ Binding `SetPtr` 均验证。

### 5.2 回归

- `test_contextmenu`、`test_contextmenu_cabi`、`test_button`、`test_editbox`、`test_combobox`、`test_numericupdown`、`test_textarea`、`test_listview`、`test_treeview`、`test_slider`、`test_scrollbar`、`test_splitter`、`test_window`、`test_menu`；
- `auto=2`、`auto=3`、`strict`；
- 导出 `testBtn` + `ctx_6` 实测弹出。

## 6. 文档/交付

- 更新 ContextMenu 用户手册/API 附录：任何控件通过 `context-menu` 键或 `SetPtr` 绑定后，右键优先打开；不需要控件自身实现右键逻辑；
- 不新增 schema 属性；控件级 `context-menu` 已有文档；
- 内部注释说明“右键在 dispatchToChild 边界预处理”。

## 7. 风险

| 风险 | 处理 |
|---|---|
| 改变控件自定义右键语义 | 只有绑定 context-menu 时才拦截；未绑定保持原路径 |
| 父/子菜单优先级变化 | V1 明确子控件优先、父兜底；与初稿的“父先”描述不同，评审已确认；测试覆盖 |
| 右键事件被重复消费 | `isPopupVisible` 防重复；预检查返回 true 后不再调用子 handleEvent |
| 移动/捕获状态 | 只处理 MouseDown Right；清除捕获后按现有 down 路径 |
| 多视口/缩放坐标 | 复用 `isContainsPoint`/`show` 现有坐标语义 |

## 8. 实施顺序

1. `tryOpenContextMenu` helper（基类行为不变）；
2. `ControlImpl::tryDispatchContextMenuToChild` + `tryOpenContextMenu` 子先/父兜底；
3. 全控件矩阵自动/手工测试；
4. 回归、Release 同步、导出 `testBtn` 联调。

## 修订记录

- 2026-11-10 初稿：确认 Button 及多个 override 控件会吞右键；采用派发边界统一预检查，覆盖所有 ControlImpl；不逐个改控件，不新增 API；待评审。
- 2026-11-10 v1（按评审 B1~B4）：明确子控件优先/父兜底；新增 `tryDispatchContextMenuToChild` 避免与子 handleEvent 重复；补未绑定右键语义与全控件测试矩阵。

## 9. 实施记录（2026-11-10）

- `ControlImpl::tryOpenContextMenu`、`tryDispatchContextMenuToChild` 已实现；递归子控件扫描（孙控件优先），父菜单兜底。
- `ControlImpl::handleEvent` 与 `dispatchToChild` 均在右键路径预检查；未绑定菜单控件保持原路径；`isPopupVisible` 消费但不重复 show。
- 新增 `test_cr_contextmenu_dispatch`：Button、EditBox、ListView、Slider、ScrollBar、Splitter、ComboBox、NumericUpDown、TreeView、TabControl、StatusBar 及父子菜单优先级/父兜底均通过。
- 全量回归 **59 OK + 4 项预存失败**；Release SHA256 `a227d8110a63978dca885809f127b3b66c40d22016b5573be71cdeeb0df5561f` 已同步。
- 联调（2026-11-10）：导出 First Project.csd StatusBar/ContextMenu action 通过；testBtn 右键菜单通过；CR 设计器/引擎均 closed。
