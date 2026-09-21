# Wheel_Support_Design — Panel 容器级滚轮事件 + ScrollBar 滚轮处理

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`CornerstoneDesigner/Temp/UICornerstone_配合修改清单_第二批.md` §P1 追加（2026-09-19，设计器已临时绕行：Win32 WH_MOUSE_LL 钩子——Windows 专有 + DPI 域不可换算，待本设计落地后回收）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/Wheel_Support_Design_复核意见.md`，2026-09-19 通过——5 项确认 + 2 项建议；复核 3.2 已纳入：kEventMouseWheel 注释注明载荷不含坐标、坐标域由引擎事件分发保证）

---

## 1. 问题与目标

滚轮事件（`EventType::MouseWheel`，带坐标）在引擎内被**静默丢弃**：`ControlImpl::handleEvent` 仅用 wheel 坐标做子控件遮挡路由，子控件不消费即 `return false`，本层无处理、无回调。设计器被迫用 Win32 低级钩子绕行（不可移植 + DPI 物理域与逻辑域不可换算 + 面板列硬编码）。

目标：
1. **Panel 容器级滚轮回调**——wheel 落在面板内且子控件未消费时，经事件体系 `SetCallback("mouse-wheel")` 通知应用（架构归位，可移植）。
2. **ScrollBar 滚轮**——滚轮落在滚动条上直接驱动 `setValue(±stepSize)`（标准滚轮交互）。

设计器回收：删除 Win32 钩子与面板列硬编码，改事件订阅。

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| wheel 路由 | `ControlImpl::handleEvent`（ControlBase.cpp:317 wheel 坐标 hasPos 提取；:331-341 子控件逆序分发 + isContainsPoint 遮挡） | 子控件不消费 → `return false` **丢弃**；本层无处理、无回调 |
| wheel 载荷 | `EventMouseWheel { float x, y; float scrollX, scrollY; }`（EventTypes.h:161） | 已存在；坐标为窗口屏幕域（`wheel.mouse_x/y`） |
| wheel 符号 | sdl3 后端（InputBackend.cpp:360）`scrollY = sdlEvent.wheel.y` | **+1 向上滚 / -1 向下滚**（SDL3 惯例） |
| C 层转换 | UICornerstoneAPI.cpp:202-209（`UI_EVENT_WHEEL_DELTA` → scrollY） | 已存在；注入测试可用（X=data+4/Y=data+8/DELTA=data+0 不重叠） |
| ScrollBar | `handleEvent`（ScrollBar.cpp:179）仅 MouseDown/拖 thumb 等，**无 MouseWheel 分支** | 缺滚轮处理 |
| ScrollBar 数值 | `setValue`（:263）**已 clamp** 到 [minValue, maxValue]；`m_stepSize` 缺省 1.0f（:16）；`setStepSize`（:290） | 现成 |
| Panel::handleEvent | Panel.cpp:43 纯转发 `ControlImpl::handleEvent` | 缺 wheel 分支 |
| 事件常量 | kEventClick="click"、kEventCheckChanged="check-changed"、kEventColorChanged="color-changed"（PropertyNames.h:366-377，kebab 命名） | **kEventMouseWheel 不存在**，需新增 |
| 回调体系 | `fireCCallback(eventName, CCallbackData::Float, &value)`（ControlBase.h:549） | 现成，Float 载荷 |
| 订阅入口 | `UICornerstone_SetCallback(inst, ctl, "mouse-wheel", cb, user)`（既有通用机制） | **零新增 C ABI** |

## 3. 架构选择与关键设计决策

### 3.1 Panel 容器级 wheel（决策：Panel::handleEvent 覆写，子控件优先、命中即回调）

```cpp
// Panel.cpp
bool Panel::handleEvent(shared_ptr<Event> event) {
    if (ControlImpl::handleEvent(event)) return true;   // 子控件/既有逻辑优先（含遮挡路由）
    if (event->m_type == EventType::MouseWheel &&
        isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)) {
        float dy = event->mouseWheel.scrollY;
        fireCCallback(PropertyNames::kEventMouseWheel, CCallbackData::Float, &dy);
        return true;                                     // 容器已处理，停止传播
    }
    return false;
}
```

决策点：
- **在 Panel 而非 ControlImpl**：需求点名容器语义；放 ControlImpl 会让 Button 等叶控件也发 wheel 回调，面过宽。
- **顺序天然去重**：`ControlImpl::handleEvent` 先跑——子控件命中并消费（如 ScrollBar 滚轮、EditBox 等）时直接 `return true`，**不会走到 Panel 的 fireCCallback**，无重复回调。
- **isContainsPoint 判定**：wheel 坐标在本面板 drawRect 内才回调（面板外穿透）。
- **载荷 = Float（scrollY 单值）**：需求点名 wheelY；坐标/scrollX 不传（最简原则，未来需要时再加新事件名或扩展载荷）。
- **符号语义写入契约**：scrollY = **+1 向上 / -1 向下**（SDL3 原生值透传，不取反）。
- **返回 true（消费）**：命中面板即停止传播，避免兄弟容器重复回调。

### 3.2 ScrollBar 滚轮（决策：命中滚动条 → setValue(value ± stepSize)，向上滚 = value 减）

```cpp
// ScrollBar::handleEvent 新增分支（置于 MouseDown 分支之前或之后均可，互斥事件类型）
if (event->m_type == EventType::MouseWheel) {
    SRect r = getDrawRect();
    float wx = event->mouseWheel.x, wy = event->mouseWheel.y;
    if (wx >= r.left && wx <= r.right() && wy >= r.top && wy <= r.bottom()) {
        float dir = (event->mouseWheel.scrollY > 0.f) ? -1.f : 1.f;  // 向上滚 → 内容上移 → value 减
        setValue(m_value + dir * m_stepSize);                        // setValue 已 clamp
        return true;
    }
    return false;   // 滚轮不在滚动条上：不消费（Panel 容器回调可接手）
}
```

决策点：
- **方向**：向上滚（scrollY=+1）→ value **减**（滚动条上移、内容回卷顶部）——Windows/主流桌面惯例。**需评审确认**（若设计器期望反向可一行翻转）。
- **setValue 已 clamp**（:265），无需重复边界处理；`notifyPositionChanged` 照常触发（滚动条联动内容既有机制）。
- **step 来源**：`m_stepSize`（缺省 1.0，`setStepSize` 可配）——与按钮点击步进一致。
- **消费语义**：滚轮在滚动条范围内才消费；范围外返回 false（交给 Panel 容器回调/继续传播）——设计器面板列滚动可经容器回调与滚动条形成互补而非竞争。

### 3.3 事件常量与订阅（决策：新增 kEventMouseWheel = "mouse-wheel"，零新增 C ABI）

```cpp
// PropertyNames.h（kEvent 区，kebab 命名惯例）
PROP_CONSTEXPR_PROP const char* kEventMouseWheel = "mouse-wheel";
```

- 订阅走既有 `UICornerstone_SetCallback(inst, ctl, "mouse-wheel", cb, user)`——C ABI 零新增。
- Binding 侧：`Control::SetCallback` 既有即可用；`Event::GetWheelY()` 等 Binding Event 扩展列**可选 backlog**（设计器当前走 C 回调）。
- 载荷契约：`UIEventData.data.floatVal = scrollY`（+1 向上 / -1 向下）。

## 4. API 设计

### 4.1 核心库
```cpp
// PropertyNames.h
PROP_CONSTEXPR_PROP const char* kEventMouseWheel = "mouse-wheel";

// Panel.cpp —— handleEvent 覆写（见 §3.1）
// ScrollBar.cpp —— handleEvent 新增 MouseWheel 分支（见 §3.2）
```

### 4.2 C ABI / Binding
零新增（`UICornerstone_SetCallback` + `"mouse-wheel"` 直接可用）。

## 5. 实现要点与验收

| 验收 | 实现 | 测试 |
|---|---|---|
| 1. Panel wheel 回调 | Panel::handleEvent 分支 | test 新用例：布局 Panel（含 Label 子控件）+ `SetCallback("mouse-wheel")`；注入 wheel（坐标在面板内）→ 回调触发且 floatVal == 注入 scrollY；坐标在面板外 → 不触发 |
| 2. 子控件消费不重复回调 | 顺序：ControlImpl 先行 | Panel 内嵌 ScrollBar，wheel 落在滚动条上 → 滚动条 value 变化，Panel 回调**不**触发（单次消费） |
| 3. ScrollBar 滚轮 | MouseWheel 分支 | 注入 wheel 于滚动条：scrollY=+1 → value = value - step；scrollY=-1 → value + step；超出 range 被 clamp |
| 4. 无关控件零影响 | 仅 Panel/ScrollBar 两类 | 回归：既有测试全绿（wheel 无订阅者时行为同现状——无回调、无消费路径变化） |

测试载体：新建 `test/test_wheel_cabi.cpp`（DLL 动态加载 + PushUIEvent 注入，UI_EVENT_WHEEL_DELTA=data+0 / X=data+4 / Y=data+8）。

## 6. 影响面与风险

- **仅 Panel/ScrollBar 两类**新增分支；未订阅 "mouse-wheel" 时 `fireCCallback` 查表空转（与 click 等既有事件同开销），无行为差异。
- **消费语义变化**：此前 wheel 事件永远 `return false`（全树穿透丢弃）；现在 Panel 命中会消费——理论上若有应用依赖"wheel 穿透到兄弟容器"将被改变。当前已知唯一 wheel 使用方是设计器（绕行中），风险可控。
- **多面板嵌套**：内层面板先命中（子控件顺序）消费，外层不重复——语义正确。
- **三后端无涉**：纯事件分发层改动，不触碰渲染/后端（wheel 上报既有）。

## 7. 待审核问题

1. **ScrollBar 方向语义**：向上滚（scrollY=+1）→ value **减**（桌面惯例）——确认？或反向？
2. **Panel 载荷只含 scrollY（Float）**：坐标与 scrollX 不传——确认（未来按需扩展）？
3. **Panel 消费语义**：命中面板即 `return true`（停止向兄弟/上层传播）——确认？
4. **Binding Event 扩展（GetWheelY）列 backlog**：设计器当前走 C 回调，确认无需本批实施？
5. **事件名**："mouse-wheel"（kebab，与既有 "check-changed"/"color-changed" 一致）——确认？

## 8. 待提交配套改动（实施时随批）

- 用户手册：事件速查表/控件手册补 "mouse-wheel" 事件（Panel 手册 4.x + ScrollBar 手册滚轮说明）。
- `design/API_Mapping_Table.md`：事件区补 mouse-wheel 行。
- `PropertyNames.h` 常量 + validate 工具键集自动纳入（kEvent 前缀）。
- 设计器回收项：删除 Win32 WH_MOUSE_LL 钩子与面板列硬编码（设计器侧）。