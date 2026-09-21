# Wheel_Fix2_Design — 滚轮语义修正（消费通则 + Panel 空转消费修复 + 坐标域勘误）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-19，P0×3 + P1×3）
> 前置调查：`test/test_wheel_nested_cabi.cpp` 四场景对照（N1 单层 ✓ / N2 深嵌套×5 ✓ / N3 经 TabControl ✓ / N4 遮挡机制 ✓）——**引擎分发链全深度正常；P0-3 根因已定位为本设计 §3.1 的 Panel 空转消费**
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/Wheel_Fix2_Design_复核意见.md`，2026-09-19 通过——5 项确认 + 放行条件 N6 对照测试）
> N6 实测数据（放行条件）：**N6a**（内层订阅/鼠标内层内）修复前 `inner=1` PASS、修复后 `inner=1` PASS（内层订阅正常）；**N6b**（中层订阅/内层无订阅空转）修复前 `mid=0` FAIL → 修复后 `mid=1` PASS（空转消费缺陷复现并修复 ✓）
> 实施补充：NumericUpDown **无 wheel 消费代码**（源码级核实 + W7 透传验证）——P0-2 描述现象未在引擎复现；同类"未聚焦消费"仅存于 ComboBox hover-cycle（ComboBox.cpp:220），是否加聚焦门控待设计器联测确认（列 backlog）

---

## 1. 问题与目标

| # | 问题 | 根因 |
|---|---|---|
| P0-3 | 深层嵌套场景 Panel mouse-wheel **全程零触发**（含行间空隙） | **Panel wheel 分支无条件消费**：`fireCCallback` 后无条件 `return true`——无订阅者时也消费（空转），外层有订阅者的 Panel 永远收不到（§3.1 详述） |
| P0-1 | TextArea 内容不足一屏仍消费 wheel（无视觉效果、容器不滚） | `TextArea::handleEvent` wheel 分支仅 isContainsPoint 即消费，不检查可滚性 |
| P0-2 | NumericUpDown 未聚焦也消费 wheel 并改值 | wheel 消费无焦点前置条件 |
| P1-4 | HandleHitTest 注释"SDL 窗口坐标 - 视口偏移"有误（实测应传原样） | 注释与实现域不一致（见 §4.2） |
| P1-6 | GetRect（父局部域）vs getDrawRect（窗口全局域）无文档 | 集成方两度踩坑 |

目标：建立**滚轮消费通则**（可滚才消费/聚焦才消费/无订阅透传），修复 Panel 空转消费，随批落地坐标域勘误。

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| Panel wheel 分支 | Panel::handleEvent（Wheel_Support 批实施）：`ControlImpl::handleEvent` 先行 → isContainsPoint → `fireCCallback` → **无条件 return true** | **空转消费缺陷（P0-3 根因）** |
| wheel 分发遮挡 | ControlImpl::handleEvent（ControlBase.cpp:331-341）：更高层级兄弟 isContainsPoint 命中 → 子控件整树**跳过** | 机制存在（N4 证明）；设计器 MouseDown 正常说明属性列未被遮挡——非本因 |
| TabControl | handleEvent 最终转发 ControlImpl；页挂载 addControl ✓；非当前页 setVisible(false) ✓（TabControl.cpp:114） | 无嫌疑（N3 证明） |
| 事件循环 | MainWindow pump → inputControl → 队列 → bench->handleEvent（Bench 仅拦 Tab） | 无嫌疑（对照测试全通） |
| TextArea wheel | TextArea.cpp:775-780：isContainsPoint 即消费 | 缺可滚性检查（P0-1） |
| NumericUpDown wheel | handleEvent 消费无焦点检查 | 缺焦点前置（P0-2） |
| ScrollBar wheel | Wheel_Support 批：命中即步进（独立机制，不涉本批） | 已验证 |
| 回调订阅查询 | `m_cCallbacks`（ControlBase.cpp:994-998 直接 map 存取） | **无 hasCallback 查询**——需新增 |
| HandleHitTest 域 | 注释"子视口场景 = SDL 窗口坐标 - 视口偏移"（UICornerstoneAPI.h:462）；实现基于 `targetToScreen() = getDrawRect()`（父链递归累加至窗口全局域） | **注释错误（P1-4）** |

## 3. 架构选择与关键设计决策

### 3.1 P0-3：Panel wheel 仅在有订阅者时消费（决策：hasCallback 门控 + 无订阅透传）

**根因机制**（对照测试 + 代码定位）：

```
wheel 在 propDynamic 行间空隙
└── tabProperty(Panel, 无订阅者) 的 ControlImpl 分发行控件 → 全部 false
    └── tabProperty 的 wheel 分支：isContainsPoint 命中
        → fireCCallback（无订阅者，空转）
        → return true  ← 【缺陷】事件被无订阅面板吞掉
└── （回调挂在 propertyPanel/更上层时）外层永远收不到 → 零触发
```

行控件消费（P0-1/2 现状）同样在内层短路——即便修复 TextArea/NUD 透传，若内层 Panel 无订阅仍空转吞事件。**两修必须同批**。

**修复**：

```cpp
// ControlBase.h 新增（protected，供子类查询）
bool hasCallback(const char* eventName) const { return m_cCallbacks.count(eventName) > 0; }

// Panel.cpp wheel 分支修正
if (event->m_type == EventType::MouseWheel &&
    isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)) {
    if (hasCallback(PropertyNames::kEventMouseWheel)) {
        float dy = event->mouseWheel.scrollY;
        fireCCallback(PropertyNames::kEventMouseWheel, CCallbackData::Float, &dy);
        return true;                       // 有订阅者：消费
    }
    return false;                          // 无订阅者：透传给外层 Panel/兄弟
}
```

语义：**wheel 由"最近的有订阅者 Panel"消费**（内层优先——ControlImpl 先行的顺序保证内层先判断）；无任何订阅者 → 全树透传丢弃（与 Wheel_Support 之前的行为兼容）。ScrollBar 步进不受影响（独立分支，命中即消费——滚动条本身是"可见交互件"，消费语义合理且已验证）。

**对照验证（实施必测）**：嵌套 Panel（内层无订阅、外层有订阅）+ 行控件——wheel 在空隙 → 外层触发；修复前零触发。

### 3.2 P0-1：TextArea 可滚性门控（决策：无滚动余量透传）

```cpp
// TextArea::handleEvent MouseWheel 分支（775-780）前置：
if (getMaxScrollY() <= 0.f) return false;   // 内容未超视口：透传给容器
```

- `getMaxScrollY()` 为 TextArea 既有内部量（滚动余量 = 内容高 - 视口高；若接口名不同以实际为准，语义为"无可滚空间"）。
- 有滚动余量时消费如旧（滚文本）；消费后仍不满足时**不回滚透传**（保持简单，滚动到底后 wheel 停在 TextArea——与主流编辑器一致，评审可改）。
- 同型控件排查：**ListView/TreeView** 若有 wheel 消费同加门控（实施时 grep `EventType::MouseWheel` 全量核对，符合 §3.4 通则）。

### 3.3 P0-2：NumericUpDown 焦点门控（决策：聚焦才消费）

```cpp
// NumericUpDown wheel 消费前置：
if (!getFocused()) return false;            // 未聚焦：透传（用户意图是滚容器）
```

- 聚焦态滚轮改值保留（标准交互）；点击控件获得焦点后滚轮生效——与主流 NUD/SpinBox 一致。
- CheckBox 参照（无 wheel 消费）已是通则形态，无需改。

### 3.4 消费语义通则（写入控件开发规范 + 设计文档）

| 控件类 | wheel 消费条件 | 其余行为 |
|---|---|---|
| 可滚容器（TextArea/ListView/TreeView） | 有滚动余量 | 透传 |
| 数值输入（NumericUpDown/Slider） | 聚焦 | 透传 |
| ScrollBar | 命中滚动条 | 消费（可见交互件） |
| Panel | **有 mouse-wheel 订阅者且坐标在面板内** | 透传 |
| 其余控件 | 不消费 | 透传 |

### 3.5 P1-4/6：坐标域勘误（决策：注释修正 + 全局约定入手册）

- **HandleHitTest 注释修正**：x/y 为**窗口全局坐标原样**（与 `getDrawRect`/`mousePos`/`mouseWheel.x,y` 同域；递归累加至窗口全局；子视口不减偏移——视口根 rect 已含窗口位置）。
- **GetRect vs GetDrawRect**：`UICornerstone_GetRect` 返回**直接父局部坐标**（m_rect 原值）；`getDrawRect` 为递归累加的窗口全局域——C ABI 头注释 + 用户手册坐标域小节明确。
- 手册新增"坐标参照系约定"小节（events-focus 或 property-system 页）：控件 rect（父局部）/ getDrawRect（窗口全局）/ 事件坐标（窗口全局）三域对照。

## 4. API 设计

### 4.1 核心库
```cpp
// ControlBase.h（protected）
bool hasCallback(const char* eventName) const;

// Panel.cpp —— wheel 分支 hasCallback 门控（§3.1）
// TextArea.cpp —— getMaxScrollY() <= 0 透传（§3.2）
// NumericUpDown.cpp —— !getFocused() 透传（§3.3）
```

### 4.2 C ABI / Binding
零新增。P1-4：UICornerstoneAPI.h HandleHitTest 注释修正；P1-6：GetRect 注释补域说明。

## 5. 实现要点与验收

| 验收 | 测试 |
|---|---|
| 1. 内层无订阅、外层有订阅：空隙 wheel → **外层**触发（修复前零触发） | test_wheel_nested_cabi 新用例 N5：外层挂回调、内层不挂 → 触发且仅 1 次 |
| 2. 内层有订阅：内层消费，外层不重复 | 既有 W2 场景扩展 |
| 3. TextArea 无可滚 → 透传（容器滚动/回调触发）；有可滚 → 滚文本 | 新用例：小 TextArea（内容少）挂容器回调验证透传 |
| 4. NUD 未聚焦 → 透传；聚焦 → 改值 | 新用例：Debug_SetMousePosition+点击聚焦后 wheel 改值；未聚焦 wheel 触发容器回调 |
| 5. ScrollBar 步进行为不变 | test_wheel_cabi 既有全绿 |
| 6. 全量回归 | 既有测试全绿 |

## 6. 影响面与风险

- **Panel 消费语义收紧**（唯一行为变化点）：Wheel_Support 批后唯一已知使用方为设计器（挂外层被内层吞）——本修复即为其所需语义；无订阅者场景与 Wheel_Support 之前行为一致。
- TextArea/NUD 消费收紧：未聚焦/无可滚时事件透传——设计器行控件场景即所需；独立使用 TextArea（有可滚内容）行为不变。
- hasCallback 为 O(1) map.count——热路径无感。
- 三后端无涉（纯事件分发层）。

## 7. 待审核问题

1. **Panel 无订阅透传**（§3.1 核心修复）：wheel 由"最近的有订阅者 Panel"消费——确认？
2. **TextArea 滚动到底后不回滚透传**（§3.2，保持简单）：确认？或需要"到底后透传给容器继续滚"（链式滚动，复杂度高建议后续批次）？
3. **NUD 聚焦门控**：是否需要 hover+聚焦 之外再加"hover 即消费"变体（部分工具库行为）？引擎按"仅聚焦"实施——确认？
4. **ListView/TreeView 同型排查**：实施时 grep MouseWheel 全量核对消费条件并按通则补齐——确认随批？
5. **P1-5（mouse-wheel 文档）**：Wheel_Support 批已落地（events-focus/panel/scrollbar 手册 + API_Mapping）——确认闭环，本批仅补坐标域小节？

## 8. 待提交配套改动（实施时随批）

- 控件开发规范（design/guidelines/coding.md 或独立节）：滚轮消费通则表（§3.4）。
- 手册坐标域小节 + HandleHitTest/GetRect 注释勘误。
- test_wheel_nested_cabi 扩展 N5-N7（§5）。
- 复核意见归档 requirements/；make_release 同步。