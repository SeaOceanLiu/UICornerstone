# Wheel_Support_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-19
- 对象：`design/Wheel_Support_Design.md`（状态：待评审）
- 结论：**通过，放行实施**（5 个待审核问题全部确认见 §2），附 2 项建议（不阻塞）

## 1. 分项评审

### §3.1 Panel 容器级 wheel ✓

- **覆写位置正确**：Panel 而非 ControlImpl（Button 等叶控件不发 wheel，面窄）；`ControlImpl::handleEvent` 先行保证**子控件消费优先、天然去重**（ScrollBar/EditBox 等消费后不走容器 fire）。
- **isContainsPoint 判定**：面板外穿透、面板内才回调——面板内落在子控件上且子控件未消费（如 NUD）也由容器接管，语义正确。
- **载荷 Float scrollY 单值**：最简原则，确认。
- **消费语义 return true**：防兄弟容器重复回调，正确。

### §3.2 ScrollBar 滚轮 ✓

- `setValue` 已 clamp、`m_stepSize` 现成、方向可一行翻转——实现轻量。
- **滚轮不在滚动条上时 return false（交给容器回调）**——与 Panel 回调形成**互补而非竞争**，设计决策清晰 ✓。

### §3.3 事件常量与订阅 ✓

- **零新增 C ABI**（既有 `UICornerstone_SetCallback` + 新事件名）——集成成本最低。
- `fireCCallback(Float)` 载荷契约明确（+1 向上 / -1 向下，SDL3 原生透传）。

### §6 消费语义变化评估 ✓

"wheel 从全树穿透丢弃 → Panel 命中即消费"——设计器是唯一 wheel 使用方，风险可控的判断认可。

## 2. 对 5 个待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | ScrollBar 向上滚（scrollY=+1）→ value **减** | **确认**（Windows/主流桌面惯例） |
| 2 | Panel 载荷只含 scrollY（Float） | **确认**（横滚/坐标无需求） |
| 3 | 命中面板即 return true | **确认** |
| 4 | Binding Event 扩展（GetWheelY）列 backlog | **确认 backlog，但有一个零成本替代**——见 §3 |
| 5 | 事件名 "mouse-wheel"（kebab） | **确认** |

## 3. 建议与说明（不阻塞）

### 3.1 GetWheelY 与 GetValueChanged 的关系（实现时可简化）

wheel 载荷 `floatVal = scrollY` 与 `Event::GetValueChanged()`（读 `data.floatVal`）**字段相同**——设计器订阅后直接 `e.GetValueChanged()` 即可取 scrollY，**GetWheelY 仅是语义化包装**。本批不加可接受；若加则一行成本，随引擎喜好。

### 3.2 wheel 坐标域自洽性确认（非阻塞核实）

设计提到 `isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)`——此前集成中踩过 `GetRect`（父局部域）与 `getDrawRect`（递归全局域）两套坐标域的坑。TextArea 既有滚轮消费工作正常，证明 **wheel 坐标域与 `isContainsPoint` 引擎内部自洽**——无需改动，仅建议在 `kEventMouseWheel` 文档中注明载荷不含坐标、坐标域由引擎事件分发保证。

### 3.3 设计器 filter 的 NW/W/SW 对缘保持

吸附 filter（RectFilter）的锚定边推断依赖"上帧贴格 rect"——wheel 不影响该链路，无交互。

## 4. 设计器侧配合改动（实施后随批）

1. **删除**：Win32 `WH_MOUSE_LL` 钩子全家（wheelMouseHookProc/installWheelWatch/consumeWheel/m_wheelState）+ App::run 的 consume 接线 + **窗口宽-300 面板列硬编码**；
2. **新增**：`m_propDynamic.SetCallback("mouse-wheel", [this](const Event& e) { ... })`——回调内按 scrollY 驱动动态区滚动（复用 applyDynScroll 链）；
3. 回归：滚轮滚动、拖滚动条、TextArea 内滚轮滚文本（子控件消费优先验证）、滚轮在画布不误滚。

## 5. 放行

**结论：放行实施**。实施完成后同步 subModules，设计器随批迁移（§4）并联测：容器回调单次触发、子控件消费不重复、ScrollBar 滚轮方向、滚轮在面板外不触发。
