# CR20261110004 — LuotiAni 等纯显示控件绑定 ContextMenu 后右键不弹出（分析 + 设计）

> 状态：**已关闭（2026-11-10；设计器联调通过）**
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261110004）
> 关联：CR20261110003（右键 context-menu 统一派发）、LuotiAni / Actor / HandleControl

## 1. 问题与根因

设计器已正确绑定：

```cpp
auto t = ui->FindControl("ani_1");
auto m = ui->FindControl("ctx_6");
t.SetPtr("context-menu", m.Handle());
```

但右键动画控件不弹菜单。

根因：

- `LuotiAni::isContainsPoint()` 恒返回 `false`（纯显示控件不参与普通事件命中）；
- `Actor::isContainsPoint()` 恒返回 `false`；
- `HandleControl::isContainsPoint()` 恒返回 `false`；
- CR20261110003 的 `ControlImpl::tryOpenContextMenu()` 复用 `isContainsPoint(mx,my)` 作为右键命中判定，因此这些控件永远不进入菜单分支；
- 这是“普通事件不参与命中”的正常语义与“右键菜单目标”需求冲突，不能在普通命中路径上简单改成 true。

## 2. 目标与范围

- V1 仅纳入 **LuotiAni、Actor**（真正无普通事件的显示控件）绑定 ContextMenu 后，右键落在 `getDrawRect()` 内即可弹出；
- HandleControl 本 CR **不纳入**（有手柄/拖拽语义，另立 CR 验证）；
- 不改变其普通鼠标/键盘事件“不参与命中”的既有语义；
- 不改变 Button/Label/Panel/Bench 等已有右键绑定行为；
- 保持 CR20261110003 的子控件菜单优先、父菜单兜底、`isPopupVisible` 防重复；
- 不新增 C ABI/Binding/JSON/schema API。

## 3. 方案

### 方案 A（采用）：ContextMenu 专用几何命中

在 `ControlImpl` 增加独立的 context-menu 命中入口（**protected virtual，不扩大公共 ABI**；不改变 `isContainsPoint`）：

```cpp
protected:
    virtual bool isContextMenuHit(float x, float y) {
        SRect dr = getDrawRect();
        if (dr.width <= 0.0f || dr.height <= 0.0f) return false;  // 零矩形不命中
        return dr.contains(x, y);
    }
```

`tryOpenContextMenu()` 顺序保持：

```cpp
if (!getVisible() || !getEnable()) return false;   // 先可见/启用
if (!isContextMenuHit(mx, my)) return false;       // 再几何命中
```

- 默认矩形命中覆盖动画透明区域，这是 V1 有意的语义；
- 未来特殊形状（圆形/透明像素）可 override `isContextMenuHit`，不在本 CR 实现；
- **HandleControl 必须显式 `isContextMenuHit() override { return false; }`**（或明确拒绝 context-menu 绑定），否则会继承默认 drawRect 命中，违背 V1 不纳入范围；该 override 是本 CR 的阻塞实现项。

- 纯显示控件不需要 override `isContainsPoint`，右键菜单可用；
- 普通事件路径仍调用 `isContainsPoint`，LuotiAni/Actor/HandleControl 仍不拦截普通鼠标事件；
- Button/Label 等矩形控件行为不变；
- 若未来某控件需要特殊右键命中形状（如圆形动画），可在自己的类中 override `isContextMenuHit`，默认矩形几何。

### 方案 B（排除）：把 LuotiAni/Actor/HandleControl 的 `isContainsPoint` 改为 drawRect

- 会让纯显示控件开始参与普通 hover/点击/拖拽/子控件派发，改变大量既有语义；
- 可能导致动画覆盖层吞掉下层控件事件。

### 方案 C（排除）：只在 CR10003 的派发前扫描直接判断 drawRect

- 能修当前三个控件，但把 context-menu 命中逻辑散在派发层；
- 未来新增纯显示控件仍容易遗漏；helper 内统一更清晰。

**定稿：方案 A。**

## 4. 与 CR20261110003 的交互

- `tryDispatchContextMenuToChild` 仍按逆序/遮挡/可见性扫描；纯显示控件的“遮挡”判断仍使用普通 `isContainsPoint`：
  - 纯显示控件绑定菜单时，扫描到它后 `tryOpenContextMenu` 用几何命中可打开；
  - 未绑定菜单的纯显示控件不会阻止下层控件菜单（保持“显示层不参与普通命中”的直觉）；
  - 若业务希望纯显示层完全屏蔽下层右键，应另立需求扩展 context-menu 遮挡语义，本 CR 不做。
- 父子菜单优先级、父兜底、已打开不重复 show 不变。
- `getVisible()` / `getEnable()` 守卫保持不变。
- `getDrawRect()` 已包含缩放/层级变换；多视口/缩放右键坐标沿用 CR10003 测试。

## 5. 测试/验收

### 5.1 自动化

1. LuotiAni 绑定 ContextMenu：右键 drawRect 内 → 菜单弹出；菜单项 action 正常；
2. Actor 绑定 ContextMenu：同上；Image 仅回归既有右键行为，不作为本 CR 纯显示扩展；
3. HandleControl **不纳入** V1；显式 override 返回 false，绑定菜单也不弹出（当前行为不变）；
4. 嵌套 Panel 内的 Animation/Actor 右键 → 菜单弹出；
5. 三者未绑定菜单时右键/左键/普通事件不回归（动画仍不参与普通命中）；
6. Button/Label/Panel/StatusBar 既有右键菜单不回归；
7. 父子绑定：子 Actor 菜单优先，无菜单时父菜单兜底；
8. 缩放/多视口/捕获期间右键不崩溃；
9. `test_cr_contextmenu_dispatch` 扩容覆盖 LuotiAni/Actor；HandleControl 不在本 CR；
10. 全量回归 + `auto=2`/`auto=3`/strict。

### 5.2 手工/导出

- 导出工程 `ani_1` + `ctx_6` 右键实测；
- 动画控件右键菜单弹出、动画继续播放/不冻结；
- 菜单项 action 在 C/C++ 导出宿主执行；
- 空菜单/无绑定动画右键行为与现状一致。

## 5.3 遮挡/普通事件/导出专项（B3~B5）

### 遮挡关系

- 上层 LuotiAni 未绑菜单时，下层 Button 菜单**确实打开**（自动化断言，不只测“能打开”）；
- 上层 LuotiAni 已绑菜单时，菜单来源是 LuotiAni 而不是下层 Button；
- 父 Panel 绑菜单、子动画未绑定 → 父菜单兜底确实打开；
- 两个纯显示控件叠放：只有实际绑菜单的那个响应；
- 以上场景均同时断言动画 `isContainsPoint()==false`。

### 普通事件不回归

- 断言 `LuotiAni::isContainsPoint()/Actor::isContainsPoint()` 仍为 false；HandleControl 的 `isContextMenuHit()==false`；
- 左键点击动画/图片不触发选中/拖拽/普通 click；
- 未绑定菜单右键不改变动画状态；
- ContextMenu 打开时动画继续播放、不冻结。

### 导出宿主

- 设计器侧负责绑定并导出 `main.cpp`；
- 引擎侧负责 `isContextMenuHit` 实现与引擎测试；
- 导出 `ani_1 + ctx_6` 后右键菜单项 action 必须实际执行（不仅是菜单可见）；
- 缩放/多视口右键坐标使用 `getDrawRect()` 目标区域。

## 6. 文档/Schema

- `docs/controls/contextmenu.html` 补“LuotiAni/Actor 等纯显示控件可绑定右键菜单；普通事件命中语义不变；默认矩形命中含透明区域；HandleControl 后续 CR”；
- 用户手册 Animation/Actor 章节可补一句右键菜单支持；
- 不新增 JSON 键/schema；`context-menu` 控件级键已存在；
- Binding 头无新增。

## 7. 风险

| 风险 | 处理 |
|---|---|
| 透明动画/图片区域成为右键目标 | 这是需求；仅绑定菜单时响应，未绑定保持原行为 |
| 纯显示控件遮挡下层右键目标 | V1 覆盖判断仍用 `isContainsPoint`，不改变遮挡；只让自己可打开；显式测试 |
| 特殊形状命中 | protected virtual 默认 drawRect；未来可 override 精细化 |
| HandleControl 误纳入 | V1 明确不纳入，另立 CR；避免手柄右键语义回归 |
| 动画播放生命周期 | 菜单 show 不改变动画状态；回归验证播放/暂停 |
| 多视口坐标 | 复用 getDrawRect/缩放现有路径，补测试 |

## 8. 实施顺序

1. `ControlImpl::isContextMenuHit` + `tryOpenContextMenu` 改用；
2. `HandleControl::isContextMenuHit` 显式 false；`test_cr_contextmenu_dispatch` 增加 LuotiAni/Actor（HandleControl 绑定菜单仍不弹出）；
3. 导出工程/动画回归；
4. 全量回归、Release 同步。

## 9. 待设计器确认

1. 是否接受“纯显示控件未绑定菜单时不遮挡下层菜单”的 V1 语义（已补遮挡测试）；
2. 是否需要特殊形状右键命中；V1 默认矩形含透明区域，已接受；
3. HandleControl 已确认 V1 不纳入，另立 CR。

## 修订记录

- 2026-11-10 初稿：定位 isContainsPoint=false；采用独立的 isContextMenuHit 几何命中，不改变普通事件语义；待评审。
- 2026-11-10 v1（按评审 B1~B6）：V1 限定 LuotiAni/Actor；isContextMenuHit 定为 protected virtual、零矩形不命中、守卫顺序；补遮挡/普通事件/导出宿主/透明区域测试。
- 2026-11-10 v2（按复核 R1~R3）：HandleControl 显式 override false；Image 与纯显示范围拆分；补未绑定不阻挡/菜单来源断言。

## 10. 实施记录（2026-11-10）

- `ControlImpl::isContextMenuHit` 默认 drawRect（零矩形 false）；`tryOpenContextMenu` 使用；`HandleControl` 显式 override false。
- `test_cr_contextmenu_dispatch` 已覆盖 LuotiAni/Actor 右键、HandleControl 不弹出、父子优先级/父兜底及既有控件。
- 全量回归 **60 OK + 4 项预存失败**；Release 已同步，DLL SHA256 `a3fcef001c36975f14c6c75c513fa3e65a3f1e50a71f11e3b27a028f815f7c70`。
- 收尾（2026-11-10）：`isContextMenuHit` 为 `protected virtual`；设计器移除 SetRect 兜底、781/a3f 实测通过；Release `a3fcef00...` 已同步。
