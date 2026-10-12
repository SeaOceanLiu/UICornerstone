# P071_MenuBarSubViewportPopup_Design — 子视口 MenuBar 弹层定位 + LuotiAni 资源引用读回

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-05 追加：P0-71）
> 前置：P0-69/P0-70 批已实施并同步
> 状态：**已放行，已实施**（2026-10-06；Q-1 同批修 ContextMenu；Q-2 维持不读回）
> 复核：`requirements/P071_MenuBarSubViewportPopup_Design_复核意见.md`（通过）
> 实施备注：① **MenuBar**（src/Menu.cpp `attachMenuPanel`）：定位改为 bench 局部域——`bench->mapViewportToCanvas(barDraw左上)` + `hitRect×复合/根复合`（与 `Popup::computeTargetRect` 同公式）；主实例 anchor=0/scale=1 逐位等价；② **ContextMenu**（src/ContextMenu.cpp `show`，Q-1）：视口钳制后落 `setRect` 前做同款全局→bench 局部换算（补 `#include "Bench.h"`）；③ P0-71② 维持不读回（§3.1 导出/加载链证据）；④ 探针 `Temp/temp_p071_probe`：主实例 MenuBar 不变（0,0→0,32）/子视口 MenuBar 不叠视口原点（120,80→120,112，局部 0,32）/ContextMenu 子视口 (200,150)→(80,70) 换算正确 —— **P0-71 OK**；⑤ 全量回归仅 4 项预存

---

## 1. P0-71① 缺陷确认（源码核实，根因链完整）

**报告**：画布（子视口）内 MenuBar 打开菜单，弹层不在菜单项下方，偏移量 = 画布视口原点。

**根因链（逐环核实）**：

| 环节 | 事实 | 位置 |
|---|---|---|
| ① 弹层挂树 | `attachMenuPanel`：panel `setParent(bench)` + `bench->addControl`（父级 = **实例 bench**） | src/Menu.cpp:1147-1157 |
| ② 定位域 | 同一函数用 `getDrawRect()`（**递归全局/窗口域**）+ `hitRect×复合` 直接 `setRect` | src/Menu.cpp:1161-1170 |
| ③ bench 坐标模型 | Off 模式：bench `rect=(0,0,canvasW,canvasH)`，**视口偏移由 anchor 携带**（`m_anchorX/Y = vp.left/top`） | Bench::recomputeViewportTransform，src/Bench.cpp:141-186 |
| ④ bench 全局域 | `Bench::getDrawRect()` = `rect + anchor`（含视口原点）；**bench 子控件 rect 是画布局部域**，其全局 = `rect×benchScale + benchDraw` | src/Bench.cpp:238-241；ControlBase.cpp:640-651 |
| ⑤ 偏差 | panel.rect 被赋 **全局坐标** → 最终全局位置 = 期望 + benchDraw（Off 下 benchScale=1）= 期望 + **视口原点**；主实例 `viewport=(0,0)` → 无可见差 | — |

**结论**：设计器根因判断成立。主实例菜单栏不受影响（锚点为 0），子视口（设计器画布）弹层整体偏移 = 视口原点 ✓ 与症状一致。

**同类缺陷核查**：
- `Popup::computeTargetRect`（Dialog.cpp:61-100）**已正确**做全局→bench 局部换算（`(global - benchDraw) / benchScale`）→ Dialog/ConfirmPopup 无此问题；
- `ContextMenu::show`（src/ContextMenu.cpp:37-67）在 `Popup::open()` 之后用**视口/全局坐标** `px,py` 直接 `setRect(px,py,w,h)`（Popup::open 内的正确 target 被覆盖）→ **与 MenuBar 同根因**（子视口右键菜单偏移）。设计器未单独列出，见 Q-1。

## 2. 修改方案

### 2.1 P0-71① MenuBar（必改）
`attachMenuPanel` 定位改为 **bench 局部（画布）域**（换算与 `computeTargetRect` 同公式）：

```cpp
SRect barDraw = getDrawRect();
float sx = getScaleXX(), sy = getScaleYY();
SRect logicSize = entry.panel->getRect();
// 全局 → bench 局部（anchor 与根复合反查）
SPoint anchor = bench->mapViewportToCanvas(SPoint(barDraw.left, barDraw.top));
float bsx = bench->getScaleXX(), bsy = bench->getScaleYY();
float hx = (bsx != 0.0f) ? entry.hitRect.left * sx / bsx : 0.0f;
float hy = (bsy != 0.0f) ? (entry.hitRect.top + entry.hitRect.height) * sy / bsy : 0.0f;
entry.panel->setRect(SRect(anchor.x + hx, anchor.y + hy, logicSize.width, logicSize.height));
```

- 主实例：`benchDraw=(0,0)`、`benchScale=1` → 与现行为**逐位等价**（回归保证）；
- 子视口 Off：rect 落到画布局部域，绘制时再叠加 anchor → 全局位置正确；
- fit/stretch：`mapViewportToCanvas` 与 `/benchScale` 与 `computeTargetRect` 一致，缩放自适应；
- 面板尺寸 `logicSize` 维持（panel 复合 = 根复合，绘制尺寸已正确）；无实例离线分支（MenuBar 子级 setPosition）不动。

### 2.2 （Q-1 决定）ContextMenu 同步修
`ContextMenu::show` 钳制后 `setRect` 前做同款换算（`(px - bx)/bsx`），或改为复用 `computeTargetRect` 的 Anchored 语义；两三行改动，同一根因。建议同批修复以一并覆盖设计器画布右键菜单。

### 2.3 不改动
- `Popup`/`Dialog`/`ConfirmPopup`（已正确）；
- `detachMenuPanel`/`closeAllMenus`/`layoutEntries` 逻辑；
- `MenuPanel::setPosition` 离线分支。

## 3. P0-71② LuotiAni 资源引用读回（确认项，建议维持现状）

**核实**：`LuotiAni::loadFromResource` 清空 `m_filePathStr`（src/LuotiAni.cpp:162-164），`getStringProperty`（:884-891）对空值返回 0——**P0-21 既定语义**；`Actor` 同款（src/Actor.cpp:134 资源引用清空 → image 键读回 0），属引擎一致的"资源引用不冒充文件路径/不读回"。

**结论**：设计器模型 extended 兜底（`isPathLikeProp` 补 `"path"`）已足够；**建议维持语义（不改）**——若改为读回 `provider:xxx` 原值串，将破坏 P0-21 已放行口径且与 Actor 不对称（Q-2）。

### 3.1 导出/加载链核实（2026-10-06 补充：维持不读回是否影响导出 JSON 后的应用取资源）

| 链路 | 核实 | 结论 |
|---|---|---|
| 设计器导出 | `CanvasPane::serializeLayoutJson()`（CornerstoneDesigner/src/CanvasPane.cpp:4593-4631）完全基于**自身模型**（`modelPropsJson` + `m_resourceProviders`），**不调用引擎 `GetString`** | 导出 JSON 中资源引用值完整（含 `provider:` 前缀/目录挂载），读回语义不影响导出 |
| 应用加载 | `LoadLayout` → 解析 → `loadFromFile`/`loadFromResource`（`provider:` 分流），读的是 JSON 原值 | 与属性读回无关；导出的布局可正常取到资源 |
| 受读回影响的场景 | ① 设计器属性面板显示当前值（模型兜底已足）；② 运行期反查 / **引擎侧序列化**（把运行时控件树写回 JSON）——当前引擎无任何序列化/导出 API；P0-21 决策备注"序列化需求后续再议"即指此类 | 现阶段无人受损 |

**后续建议**：若未来"引擎侧运行时序列化"需求成立，单独立项——为资源引用**记忆原值串**并支持读回（读写对称）；代价=变更 P0-21 口径 + 同步 `test/test_image.cpp:215`（现断言返回 0）等回归项，不宜混入本缺陷批。

## 4. 测试计划
- **静态探针（Temp/temp_p071_probe.cpp）**：
  1. owner + `CreateViewport({120,80,400,300})`；子视口内 CABI 建 MenuBar + 面板/项 + AddMenu；
  2. 全局坐标模拟点击菜单项（`handleEvent` MouseDown/Up，坐标 = 视口原点 + 栏内局部）触发 openMenu；
  3. 断言 `panel->getDrawRect().left/top == barDraw.left / barDraw.bottom()`（全局域），**不得含视口原点偏移**；
  4. 主实例对照组同断言（现行为不变）；面板局部 `getRect` = bar 局部 + (0,barHeight)；
  5. （若含 2.2）子视口 ContextMenu.show(全局点) 后 `getDrawRect` 断言。
- **回归**：test_menu、test_multiviewport、test_multiviewport_cabi、test_overlay_viewport_cabi + 全量（仅 4 项预存）；设计器联测：画布内菜单弹层对齐 + 主实例菜单栏位置不变。
- （可选 Q-3）在 test_multiviewport_cabi 补点击注入的 CABI 级断言。

## 5. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | ContextMenu 同根因缺陷是否同批修复？ | **建议同批**（同一根因、两三行；一并覆盖设计器画布右键菜单） |
| Q-2 | P0-71② 维持"资源引用不读回"（设计器兜底已足）？ | **维持**（P0-21 口径，Actor/LuotiAni 一致性；导出/加载链不受影响——见 §3.1 核实） |
| Q-3 | 是否在 test_multiviewport_cabi 增加 CABI 级弹层定位断言？ | 可选（静态探针已覆盖；CABI 断言更贴近设计器路径） |
