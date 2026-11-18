# CR20261113001 — ContextMenu 设计态坐标被 Popup 默认 Centered 重置（分析 + 设计）

> 状态：**closed（v3 设计通过复核；2026-11-16 引擎实施并验证，设计器视觉验证通过；设计器/引擎两项均 closed）**
> 来源：`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_2026-11.md` → CR20261113001
> 关联：CR20261112002（ContextMenu 外框尺寸同步）、`ContextMenu_Analysis.md`、`Dialog.h` / `Dialog.cpp`、`ContextMenu.cpp`
> 边界：分析 + 设计 + 实施记录；设计已通过复核，实现与验证见 §10。

## 1. 现状与根因

### 1.1 现象

设计器创建/重建 `ContextMenu` 后，设置 `visible=1`（设计态显示）会把已设置的绝对坐标 x/y 重置为视口居中位置，例如 `509,407.5` 回到视口中心。items 重建、选中/取消选中、切页后同样会回退。CR20261112002 的尺寸同步只改 w/h，不负责位置，二者不是同一问题。

### 1.2 源码证据

- `Popup` 默认锚定模式为 **Centered**：
  `include/Dialog.h:38` → `AnchorMode m_anchorMode = AnchorMode::Centered;`
- `Popup::setRect()` 会把模式改为 Absolute：
  `src/Dialog.cpp:194-197` → `m_anchorMode = Absolute; Panel::setRect(rect);`
- `ContextMenu::setRect()` 只调用 `Panel::setRect()`，**没有**设置 Absolute：
  `src/ContextMenu.cpp` 当前实现。
- `Popup::setBoolProperty("visible", 1)` 调用 `open()`：
  `src/Dialog.cpp:442-444`。
- `Popup::open()` 先 `computeTargetRect()` 再 `setRect(target)`：
  `src/Dialog.cpp:132-151`；`computeTargetRect()` 的 Centered 分支按视口中心重算 x/y：
  `src/Dialog.cpp:76-85`。
- `ContextMenu` 继承 `Popup`，未覆盖锚定模式；设计器 `SetRect`/JSON `rect` 设置的位置在 `visible=1` 时被 Centered 覆盖。

### 1.3 结论

根因不是 `ContextMenu` 尺寸同步，而是 **显式位置没有建立“绝对定位”语义**：

- `ContextMenu::setRect()` 绕过 `Popup::setRect()`，没有把 `m_anchorMode` 置为 `Absolute`；
- 默认构造仍是 `Centered`，`Popup::open()` 因此按视口居中重写显式位置。
- 无显式位置时 `Centered` 是旧版行为，本设计保留；但 `Popup::open()` 内部的 `setRect(target)` 不能把模式偷偷改成 Absolute，否则第二次 open 起行为变化（V1-1）。

## 2. 目标与范围

### 2.1 目标

1. ContextMenu 的定位语义分两段明确：
   - 无显式位置（未调用 setRect/JSON rect/C ABI SetRect）时保留 `Popup` 默认 **Centered**，重复 open/resize 仍按旧版重新居中；
   - 一旦显式 `setRect` / JSON `rect` / C ABI `SetRect` / Binding `SetRect`，建立 **Absolute** 并保持；
2. 设计态 `visible=1`（`Popup::open()`）不得按视口居中重写 x/y；
3. 运行时 `show(x,y)` 保持现有视口钳制与弹出逻辑，钳制只影响 `show` 计算，不回写设计态位置；
4. 不改变 `Dialog` / `Popup` / `ConfirmPopup` 的 Centered/Anchored 语义；
5. 与 CR20261112002 尺寸同步兼容：`setRect` 仍传播到 MenuPanel 并触发内容重算；
6. 旧布局/旧调用行为兼容：无显式位置时保持 Popup 旧 Centered 行为（包括重复 open/resize）；不改变 `show()` 的钳制结果。

### 2.2 非目标

- 不新增属性键、C ABI、Binding 或 schema 字段；
- 不改变 `show()` 的视口钳制算法；
- 不改变 MenuPanel 尺寸同步（CR20261112002）；
- 不处理设计器 `CanvasPane` 的宿主模型回写/临时 `SetRect` workaround；该 workaround 由设计器在引擎交付后移除。

## 3. 设计

### 3.1 定位模式与兼容性（C1/C2）

不在 `ContextMenu` 构造函数中强制 `Absolute`，以保留 **C++ 直接构造 / JSON 未写 rect** 的旧默认 Centered 行为；C ABI 创建路径总是显式 SetRect，不涉及该分支：

- **无显式 rect**：保持 `Popup` 默认 `Centered`；`visible=1` 按旧版居中，且**重复 open/窗口 resize 后仍重新居中**（C2）。该分支只来自 C++ 直接构造/JSON 未写 rect；`UICornerstone_CreateContextMenu` 创建时必调 `setRect(x,y,w,h)`，C ABI 路径本来就是显式 rect → Absolute，不存在“C ABI 默认居中”场景。
- **显式 rect**：外部 `setRect` / JSON `rect` / C ABI `UICornerstone_SetRect` / Binding `SetRect` 调用 `Popup::setRect()`，并置 `m_hasExplicitRect=true`，建立 **Absolute** 语义；之后 `visible=1` 不再居中。
- **显式 rect 后**：`ContextMenu` 保持 Absolute；`show()` 也以 Absolute + 视口钳制写入 live rect。
- 该设计不是“ContextMenu 永远 Absolute”，而是“**一旦设置显式位置，就保持绝对定位；无位置时保留 Popup 旧默认**”。这避免 C++ 直接构造/JSON 未写 rect 的旧行为破坏；C ABI 创建路径总是显式 SetRect，不涉及此兼容分支。

### 3.2 `setCentered()` / `setAnchored()` 语义（C1）

`ContextMenu` 仍公开继承 `Popup::setCentered()` / `setAnchored()`。为避免外部调用破坏“显式 rect 后保持 Absolute”的不变量，采用**明确不支持 + 不改变模式**的方案：

- `Popup::setCentered()` / `Popup::setAnchored()` 改为 `virtual`（C++ 内部 API，无 C ABI 变化）；
- `ContextMenu` 覆盖两者：
  ```cpp
  void ContextMenu::setCentered() override {
      // ContextMenu 定位由 setRect/show 控制；不支持运行时改定位模式。
      static bool logged = false;
      if (!logged) { Platform::Log("ContextMenu::setCentered ignored; use setRect/show"); logged = true; }
  }
  void ContextMenu::setAnchored(Control*, const SRect&) override {
      static bool logged = false;
      if (!logged) { Platform::Log("ContextMenu::setAnchored ignored; use setRect/show"); logged = true; }
  }
  ```
- 不改变 `m_anchorMode`，因此：
  - 无显式 rect 时仍保持默认 Centered（兼容旧行为）；
  - 显式 rect 后仍保持 Absolute；
- `Popup::setAbsolute()` 保留可用（显式设置绝对 rect，仍走 `ContextMenu::setRect`）；
- 测试固定：`setRect → setCentered/setAnchored → visible=1` 位置不变；无 rect 时 `setAnchored` 不影响默认居中。
- 文档注明：ContextMenu 的定位模式不再通过 `setCentered`/`setAnchored` 切换；C ABI `SetEnum("centered", ...)` 对 ContextMenu 不生效。
- 实施细节（V2-5）：`ContextMenu.cpp` 显式 `#include "PlatformUtils.h"`；诊断用文件内匿名 namespace 的 `static void logPositionModeIgnored(const char* api)`，内部 `static bool logged` 去重，不新增全局日志工具。

### 3.3 `ContextMenu::setRect` 复用 `Popup::setRect`

将当前：

```cpp
void ContextMenu::setRect(SRect rect) {
    ensureSizeCallback();
    Panel::setRect(rect);
    ...
}
```

改为：

```cpp
void ContextMenu::setRect(SRect rect) {
    ensureSizeCallback();
    m_hasExplicitRect = true;       // 外部显式位置（open 内部不得走此入口）
    Popup::setRect(rect);           // 先置 Absolute，再 Panel::setRect（百分比子项链）
    if (m_menuPanel) {
        m_menuPanel->setRect(SRect(0, 0, rect.width, rect.height));
        m_menuPanel->recalculateSize();   // CR20261112002：内容尺寸回调同步外框
    }
}
```

要点：

- `Popup::setRect()` 内部执行 `m_anchorMode = Absolute` + `Panel::setRect(rect)`；保持现有 Panel 布局链；
- 保留 CR20261112002 的 `MenuPanel` 尺寸传播/重算；
- 不直接写 `m_anchorMode` 后调 `Panel::setRect`，避免遗漏 Popup 语义；
- 该方法同时覆盖 C ABI `SetRect`、JSON `rect`、Builder/设计器路径。

### 3.4 `Popup::open()` 路径

`Popup::open()` 改为：

```
target = computeTargetRect();
applyOpenRect(target);      // protected virtual；默认实现调用 setRect(target)
layoutContent();
setVisible(true);
...
```

`ContextMenu` 覆盖：

```cpp
void ContextMenu::applyOpenRect(const SRect& target) override {
    ensureSizeCallback();                 // V2-1：无 rect 直接 visible 也必须能同步外框
    if (m_menuPanel && !m_menuPanel->isCreated() && GET_CONTEXT)
        m_menuPanel->create();            // V2-3：open 已挂树，补 create 后字体/行高才可测
    if (m_hasExplicitRect) {
        setRect(target);                  // 显式位置：Absolute，正常传播
        return;
    }
    // 无显式位置：应用居中/锚定结果，但不把定位模式改成 Absolute
    Panel::setRect(target);               // V2-2：保留 Panel 百分比/布局链语义，不改锚定模式
    if (m_menuPanel) {
        m_menuPanel->setRect(SRect(0, 0, target.width, target.height));
        m_menuPanel->recalculateSize();   // callback 已确保注册，外框 w/h 随内容
    }
}
```

效果：

- 显式 rect 后 `m_anchorMode == Absolute`，`computeTargetRect()` 返回当前 `m_rect`；
- 无显式 rect 时 `m_anchorMode` 保持 Centered/Anchored，**每次 open 都重新居中/锚定**（修复 V1-1）；
- `layoutContent()`/watcher/焦点/可见性逻辑不变。

### 3.5 `show(x,y)` 保持原逻辑

`ContextMenu::show()` 继续：

- `Popup::open()` 挂树、创建、布局、显示；
- 第二遍 `recalculateSize()` 后计算视口钳制；
- 换算 bench 局部坐标并 `setRect(SRect(lx, ly, w, h))`。

由于 `setRect` 仍走绝对语义，`show()` 的钳制结果就是最终位置；不会因为 `Popup::open()` 的 Absolute 计算而跳过钳制。`close()` 后再 `show()` 同样重新钳制。

### 3.6 状态矩阵

| # | 场景 | 操作 | x/y 结果 |
|---|---|---|---|
| P1 | 新构造无显式 rect | `visible=1 → 0 → 1`、resize/改内容后再 `visible=1` | 每次都按 Popup 旧默认重新居中（V1-1/C2） |
| P2 | 显式 setRect | `setRect(509,407.5,140,80)` → `visible=1` | x/y = `509,407.5` |
| P3 | JSON rect | `rect:{x:640,y:260,...}` → `visible=1` | x/y = `640,260` |
| P4 | 移动后 items 增删/重建 | `setRect(620,430,...)` → add/remove/separator | x/y = `620,430`（w/h 按 CR12002 内容） |
| P5 | 选中/取消/切页 | `visible` 切换/重建 | x/y 保持最后一次绝对位置 |
| P6 | 运行时 show | `show(x,y)` | 按现有视口钳制后的 x/y |
| P7 | close 后再次 show | `close()` → `show(x2,y2)` | 重新钳制，不回退旧居中 |
| P8 | 旧布局无 rect | 重复 visible / resize | 无 SetRect：每次重新居中；有 SetRect：Absolute 保持设置值 |
| P9 | Dialog/Popup/ConfirmPopup | 原 Centered/Anchored | 行为不变（仅 ContextMenu 改） |

### 3.7 与 CR20261112002 的交互

- `ContextMenu::setRect` 先 `Popup::setRect`（Absolute + Panel::setRect），再 `m_menuPanel->setRect(0,0,w,h)` + `recalculateSize()`；
- 内容重算回调 `applySyncedSize` 仍只改外框 w/h，不改 x/y；
- `Popup::open()` 的 `applyOpenRect(target)` 显式 rect 分支会触发一次幂等重算；无 rect 分支保持 Centered 并重新居中；
- `syncSize()` 不触碰位置。

## 4. 影响面与兼容性

| 层 | 影响 |
|---|---|
| C++ | `Popup::setCentered/setAnchored` 改 virtual + 新增 protected virtual `applyOpenRect`；**C++ vtable/ABI 变化，DLL/头文件/C++ 使用方必须同批重编译；旧 C++ 二进制加载新 DLL 不支持** |
| C ABI | 无新增函数；`SetRect` 后保持绝对；`SetEnum("centered")` 对 ContextMenu 不生效 |
| Binding | 无新增；`Control::SetRect` 同样受益 |
| JSON/Schema | 无字段变化；CR20261112002 的 `context-menu.rect` 继续用于位置持久化 |
| Dialog/Popup | 行为不变（仍默认 Centered；显式 setRect 仍 Absolute）；仅方法签名 virtual |
| 设计器 | 可移除 `createViewportControl` / `syncContextMenuItems` 的临时 SetRect 恢复位置 workaround |

## 5. 测试计划

新增 `test/test_cr_contextmenu_position.cpp`，并加入 `test/CMakeLists.txt`；复用/新增 `layouts/` 样本时，确认 POST_BUILD 的 `layouts/` 复制规则覆盖该测试。

1. **P2 基础绝对位置**：`ContextMenu` 构造 → `setRect(509,407.5,140,80)` → `visible=1`，x/y 不变；`isPopupVisible()` 为真；CR12002 外框 w/h = 内容。
2. **P1 兼容/V1-1/V2-1/V2-3**：直接 `make_shared<ContextMenu>(nullptr)` + `addItem` + `visible=1`（不调用 create/show），断言 x/y 居中、**外框 w/h == MenuPanel 内容尺寸**；重复 `visible=0/1` 尺寸稳定；再测改窗口尺寸/菜单内容后 `visible=1` 仍重新居中。
3. **P3 JSON**：含 `rect` 的 `type:"context-menu"` 布局加载 → `visible=1`，x/y 保持 JSON 值；CR12002 外框 w/h = 内容。
4. **C1 定位模式**：`setRect` 后调用 `setCentered()` / `setAnchored(anchor,offset)` → 再 `visible=1`，x/y 仍为 setRect 值；`Popup::setAbsolute` 仍可显式设置。
5. **P4/P5**：移动后 add/remove/separator/改字体/缩放 → x/y 保持；选中/取消/切页（visible 切换）不重置。
6. **P6/P7**：`show(0,0)`/`show(viewportRight,viewportBottom)` 仍钳制；`close()` 后再次 `show` 正常；设计态 `setRect` 不被 show 回写。
7. **C3 C ABI**：`UICornerstone_SetRect` + `UICornerstone_SetBool(inst, ctl, "visible", 1)`（不存在 `UICornerstone_SetVisible`）后 x/y 保持；`GetRect` 读回稳定；空/非空菜单、无效句柄不崩溃。
8. **C3 缩放/parent**：1.5x/2x ContextMenu 在 `visible=1` 与 `show()` 下的位置/尺寸；带非空 parent 的绝对位置语义。
9. **C4 设计态后 show**：`setRect + visible=1` 后再 `show(x2,y2)`，验证钳制/移动、watcher/焦点/bench 归属；之后 `close()`/再次 `show()` 正常。
9.1 **V2-3 create 时序**：直接 `visible=1` 时 `applyOpenRect` 应补 `m_menuPanel->create()`；断言字体/行高可测、外框尺寸非 10×10；若 context 未就绪则 create 延迟到挂树后。
10. **C6 重复 open/close**：非空/空菜单反复 `visible=1/0`、`show/close`，断言无回调递归、尺寸抖动、位置漂移；`show()` 内两次 `recalculateSize()` 顺序不互相覆盖。
11. **P8**：旧布局无 rect；无 SetRect 反复 visible/resize 保持旧默认 Centered；有 SetRect 保持绝对设置值。
12. **P9**：Dialog/Popup/ConfirmPopup 的 Centered/Anchored 现有测试不回归。
13. **CR12002 回归**：`test_cr_contextmenu_sync` S1~S9 不回归；C ABI/Binding 无新增符号。
14. **设计器联调**：移除临时 SetRect 后，创建/重建/保存/打开/预览位置一致；`auto=2/3`、`strict` 通过。
15. **三后端**：SDL3/SFML/raylib 位置/尺寸断言一致。

## 6. 用户手册更新计划

| 文档 | 更新 |
|---|---|
| `docs/controls/contextmenu.html` | 补充：显式 `rect`/SetRect 后 ContextMenu 为绝对定位，`visible=1` 不居中；无显式位置时保留 Popup 旧默认居中；`show(x,y)` 才做视口钳制且只改 live rect，不回写设计器模型；`setCentered/setAnchored` 对 ContextMenu 不生效 |
| C ABI / Binding 速查表 | 无新增 API；补充 `SetRect`/`GetRect` 绝对位置语义、`SetEnum("centered")` 对 ContextMenu 不生效；无显式位置时旧居中行为保留 |
| `README.md` | 特性行补充“ContextMenu 显式位置后保持绝对定位、显示不被居中重置” |
| schema | 无字段变化；沿用 CR20261112002 `context-menu.rect` 描述 |

## 7. 风险与回退

| 风险 | 处理 |
|---|---|
| `Popup::open()` 先应用 target 覆盖用户位置 | 改走 `applyOpenRect`：显式 rect 走 Absolute；无 rect 不改变定位模式 |
| 设计态 `visible=1` 与运行时 `show` 混淆 | `show` 显式钳制后 `setRect`；测试覆盖 P6/P7/C4 |
| 旧布局无 rect | 保留旧默认 Centered（C2 兼容）；显式 SetRect 才 Absolute |
| CR12002 尺寸同步重入 | `applySyncedSize` 只改 w/h；`setRect` 顺序固定；测试回归 |
| Dialog/Popup 语义被误改 | Popup 仅新增 virtual/applyOpenRect 钩子，默认行为不变；P9 回归 |
| 设计器仍临时 SetRect | 引擎交付后设计器移除；引擎不依赖该 workaround |
| setCentered/setAnchored 外部调用 | ContextMenu override 为 no-op + 诊断；C1 测试 |
| 反复 open/close 递归/抖动 | setRect 幂等；show 顺序固定；C6 回归 |

## 8. 实施顺序

1. `Popup::setCentered/setAnchored` 改 virtual；新增 protected virtual `applyOpenRect`，`open()` 改调它（默认 `setRect`）；
2. `ContextMenu` 覆盖 `setCentered/setAnchored` 为 no-op + `Platform::Log`（static bool 去重）；增加 `m_hasExplicitRect` 与 `applyOpenRect` 覆盖；
3. `ContextMenu::setRect` 置 `m_hasExplicitRect=true`，改用 `Popup::setRect`，保留 MenuPanel 传播/重算；
4. 新增位置测试 + 布局样本/既有 CR12002 测试回归（含 C3~C6）；
5. 文档/schema 说明（无字段变化）；
6. 三后端验证、设计器联调后关闭 CR。

## 9. 验收对照

| CR 验收项 | 设计落点 |
|---|---|
| 创建并 `visible=1` 保持 setRect x/y | §3.1/§3.2/§5 P2/C1 |
| 移动后 items 增删/重建位置保持 | §3.2/§3.6/§5 P4/P5 |
| `show(x,y)` 视口钳制仍正确 | §3.5/§5 P6/P7/C4 |
| close/再次 show 正常 | §3.5/§5 P7/C4/C6 |
| C ABI/Binding/schema/文档同步 | §4/§6（无新 API；C3 测试） |
| 设计器保存/打开/strict 回归 | §5 P3/P8 + §4 设计器 |
| CR12002 尺寸同步不回归 | §3.7/§5 C6 |

## 10. 实施记录（2026-11-16）

### 10.1 落地文件

- `include/Dialog.h` / `src/Dialog.cpp`：
  - `Popup::setCentered()` / `setAnchored()` 改 `virtual`；
  - 新增 `protected virtual void applyOpenRect(const SRect&)`，默认实现 `setRect(target)`；
  - `Popup::open()` 由 `setRect(target)` 改为 `applyOpenRect(target)`。
- `include/ContextMenu.h` / `src/ContextMenu.cpp`：
  - 新增 `m_hasExplicitRect = false`（构造初始为 false）；
  - `ContextMenu::setRect()` 置 `m_hasExplicitRect = true` 后调用 `Popup::setRect()`（建立 Absolute），再同步 MenuPanel 并重算；
  - `ContextMenu::setCentered()` / `setAnchored()` 覆盖为 no-op，文件内 `logPositionModeIgnored()`（`Platform::Log` + `static bool` 去重）输出一次性诊断；
  - 覆盖 `applyOpenRect()`：开头 `ensureSizeCallback()`；MenuPanel 未 create 且 context 就绪时补 `create()`（幂等）；显式 rect 走 `setRect(target)`；无显式 rect 用 `Panel::setRect()`（不改锚定模式），空菜单保持当前外框尺寸，非空菜单先按内容测最终尺寸、再重新取 centered/anchored 目标并应用；
  - 显式包含 `PlatformUtils.h`，不依赖间接 include。
- `test/test_cr_contextmenu_position.cpp`（新增）+ `test/CMakeLists.txt`：注册测试并纳入 `layouts/` POST_BUILD 复制条件。
- 用户手册/速查表：`docs/controls/contextmenu.html`、`docs/appendix/capi.html`、`docs/appendix/binding.html`、`README.md`、`design/API_Mapping_Table.md`（无新增 API/字段）。

### 10.2 验证结果

- `test_cr_contextmenu_position`：SDL3 / SFML / raylib 均 **44/44 通过**。覆盖：
  - 直接构造 + addItem + `visible=1`（不 create/show）：x/y 居中、外框 w/h == MenuPanel 内容尺寸、MenuPanel 已 create；
  - 重复 `visible=0/1`、内容变化后再次 open：尺寸稳定且按新尺寸重新居中（V1-1/V2-1）；
  - 显式 `setRect` 后 `setCentered/setAnchored` 不改变位置；移动后 addItem 保持 x/y；
  - 1.5× 缩放、设计态 visible 后 `show`、`close` 后再次 `show`、重复 open/close 稳定性；
  - C ABI `SetRect` + `SetBool("visible",1)` + `GetRect` x/y 保持、外框与面板一致、空菜单/无效句柄安全；
  - JSON 有 rect / 无 rect 两条加载路径；base `Popup` 仍居中（P9）。
- 回归：`test_cr_contextmenu_sync` 48/48；`test_contextmenu` 19/19；`test_cr_contextmenu_dispatch` 通过。
- SDL3 全量回归：仅 4 项历史失败（`test_menu_cabi` bar-height、`test_multiviewport*`、`test_treeview_cabi` item-font），无新增。
- `validate_layout layouts/*.json --strict`：0 error / 0 warn。
- C++ ABI：`Popup` vtable 变化，DLL、头文件与 C++ 使用方同批重编译；C ABI/Binding 函数签名与行为不变（新增能力仅位置语义），`SetEnum("centered")` 对 ContextMenu 不生效。
- Release DLL SHA256：`2207367d1b51df5a5d14aab98fd890289dcc9979bec0a750e49927f96bc426d3`；已 `robocopy /MIR` 同步设计器 `subModules/UICornerstone`（待设计器父仓提交 gitlink）。

## 修订记录

- 2026-11-13 初稿：确认根因为 ContextMenu 未建立绝对定位语义；设计构造默认 Absolute + `setRect` 复用 `Popup::setRect`，保持 `show` 钳制与 CR12002 尺寸同步；待评审。
- 2026-11-16 v1（按评审 C1~C6）：改为“显式 rect 后 Absolute，无 rect 保留旧 Centered”以兼容 C2；`setCentered/setAnchored` 在 ContextMenu 中 no-op + 诊断；补 C ABI/缩放/parent、设计态 visible 后 show、CMake/layout 复制、重复 open/close 测试；注明 show 只改 live rect。
- 2026-11-16 v2（按复核 V1-1~V1-6）：`Popup::open()` 改走 `applyOpenRect`，无显式 rect 时保持 Centered 且重复 open 重新居中；`m_hasExplicitRect` 区分外部/内部；修正 C ABI 无默认居中场景；明确 C++ vtable/ABI 同批重编译；`UICornerstone_SetVisible` 改为 `SetBool("visible")`；补重复 open/resize 与 warnOnce 落地。
- 2026-11-16 v3（按复核 V2-1~V2-5）：`applyOpenRect` 开头 `ensureSizeCallback` 并按需 `MenuPanel::create`；无 rect 分支改用 `Panel::setRect`；补直接 addItem+visible 的外框尺寸/重复显隐测试；修正 C2 措辞为 C++/JSON；明确 `PlatformUtils.h` 与文件内静态诊断 helper。
- 2026-11-16 v4（实施）：引擎实现完成并三后端验证（44/44）；补 §10 实施记录；用户手册/速查表/README/API 映射同步；设计器视觉验证通过，CR 设计器/引擎两项均 closed。
