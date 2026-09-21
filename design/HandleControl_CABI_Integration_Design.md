# HandleControl_CABI_Integration_Design — 手柄控件集成 C ABI 扩展

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`CornerstoneDesigner/Temp/UICornerstone_配合修改清单_第二批.md`（2026-09-18，P0×4）
> 关联需求：`requirements/` 无直接对应独立文件（本批为 HandleControl 集成缺口，汇总于第二批清单）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/HandleControl_CABI_Integration_Design_复核意见.md`，2026-09-18 通过——5 项确认 + 4 项补充；4 项补充已纳入：坐标域注释/UIHandleRectFilter 统一/右缘联动提示/可见性约定）
> 前置：HandleControl 已落地并迁移（设计器已整体迁移至引擎 HandleControl，自绘方案已删除）；P0-1~4 为集成中发现的功能缺口

---

## 1. 问题与目标

设计器已将选中框/手柄/光标整体迁移到引擎 `HandleControl`。本方绘制已集成，但发现 4 个 C ABI 缺口阻塞**精确集成**：

| # | 缺口 | 现状影响 |
|---|---|---|
| P0-1 | **切换 target / detach 无 C ABI** | 每次改选控件被迫 Destroy + 重建 HandleControl（选中/取消/切换均触发销毁创建），选中切换有创建开销 |
| P0-2 | **手柄命中查询无 C ABI** | 设计器按下沿需区分"点击在手柄/本体/空白"，目前用"选中 rect 外扩 10px"近似，与引擎 8×8 精确判定不一致 |
| P0-3 | **拖拽几何无外部回调（rect filter）** | 设计器网格吸附靠"轮询差异→snap→写回"，resize 拖拽中不能写回（漂移），只能松手收敛——无实时吸附反馈 |
| P0-4 | **配置项无 C ABI**（可选） | Move 手柄开关/尺寸/颜色/最小尺寸定制缺入口（缺省可用，非阻塞） |

目标：
- SetHandleTarget 一行切换目标 / detach 语义（恢复光标+移出容器）由引擎保证，设计器删除销毁重建。
- HandleHitTest 与引擎内部 `hitTestHandle` 像素级一致，删除 10px 近似。
- HandleRectFilter 使吸附在**拖拽过程中实时生效**（当前松手才收敛），并为未来对齐线/智能参考线保留扩展点。
- P0-4 按引擎判断：若 P0-3 落地，Move 手柄保留并经 filter 吸附，此项可降级为纯配置补齐。

---

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| 附加/分离 | `HandleControl::setTarget(Control*)`（HandleControl.cpp:44）+ `detach()`（:63）：detach 已含 **恢复光标**（`Cursor::setCurrent(m_cursorDefault)`）+ **移出父容器**（`parent->removeControl(shared_from_this())`） | **已存在**；`detach()` C ABI 语义直接可用 |
| C ABI 路径目标绑定 | `setTarget(Control*)` 用 no-op deleter shared_ptr 标记存活（:50） | 已存在；**构造时绑定不可切换**（CreateHandleControl 每次建新） |
| 手柄命中 | `static` 私有 `hitTestHandle(float, float)`（:111）返回 `HandleType`（private enum），依赖 `m_handleAreas`（draw() 中 updateHandleAreas 重建） | 已存在；**未暴露**、且依赖 draw 一次性更新区域缓存 |
| 拖拽几何 | `updateResize(:179)`/`updateDrag(:271)` 各自算新 rect 后 `m_target->setRect(...)`（:250 / :284） | 已存在；**无外部回调**，应用无法参与几何决策 |
| 配置 | `setHandleSize/setMinSize/setHandleColor/setActiveColor/setSelectionColor/setMoveHandleVisible/...`（HandleControl.h:21-41） | 已存在 C++ API；无 C ABI |
| C ABI 现状 | `UICornerstone_CreateHandleControl(instance, target, x, y, w, h, xScale, yScale)`（UICornerstoneAPI.cpp:2324）——target 构造绑定、不可更换 | 已有；**缺 Set/查询/回调/配置** |
| Binding 现状 | `UICornerstone::CreateHandleControl`（UICornerstone.cpp:451）+ DynamicApi `RESOLVE(CreateHandleControl)` | 已有；缺对应包装 |
| 控件句柄还原 | `static Control* validateControl(instance, handle)`（UICornerstoneAPI.cpp:141）返回 `Control*`，可 `dynamic_cast<HandleControl*>` | 已有机制 |

---

## 3. 架构选择与关键设计决策

### 3.1 P0-1：目标切换 / detach（决策：复用既有 setTarget(Control*) + detach()）

核心层**零新增逻辑**——现有 API 已满足，C ABI 仅在"句柄校验 + NULL 语义"处薄封装：

```c
int SetHandleTarget(UIInstance, UIControlHandle handle, UIControlHandle target);
// target == NULL  → detach()（移出容器 + 恢复光标，既有 detach 语义）
// target != NULL  → validate 后 setTarget(targetV)
```

要点：
- **NULL 语义映射到 detach() 而非 setTarget(nullptr)**：`setTarget(Control*)` 无 NULL 防护（`target->getParent()` 会空解引用），因此 C ABI 显式分支。
- **切换开销 O(1)**：`setTarget(Control*)` 内部已处理"旧 target detach → 新 target attach"（:46 `if (m_target) detach();`），选中切换零创建、零销毁。
- 重复附加同一 target：`setTarget` 会先 detach 再 attach（幂等无害；如需避免可加 `if (m_target == target) return;` 快速路径——**建议加**，因设计器高频率重复选中）。
- 生命周期：沿用 no-op deleter shared_ptr 标记（C ABI 路径），目标存活由调用方（设计器/BENCH）保证——与 CreateHandleControl 现有语义一致，不引入新所有权模型。

### 3.2 P0-2：手柄命中查询（决策：暴露包装 + 强制刷新区域缓存）

`hitTestHandle` 依赖 `m_handleAreas`，而区域缓存仅在 `draw()` 中重建。C ABI 纯查询路径可能尚未 draw（按下沿发生在首帧 draw 前），**必须先 `updateHandleAreas` 再 hitTest**。故：

```c
int HandleHitTest(UIInstance, UIControlHandle handle, float x, float y, int* outHandleType);
```

实现（C ABI 内联编排）：
1. `auto* hc = dynamic_cast<HandleControl*>(validateControl(...))`；非 HandleControl → 0。
2. `hc->updateHandleAreas()`（**先刷新**，基于当前 target 屏幕 rect）。
3. `hc->hitTestHandle(x, y)` 返回 `HandleType`（改 public 包装或加 public 查询接口）。
4. `*outHandleType = (int)type`（HandleType 枚举公开映射；`None=0` 与语义对齐）。返回 `type != None`。

设计决策：
- **`HandleType` 从 private 转 public 枚举**（或提供 `handleTypeToInt` 映射）——C ABI 需输出可区分的类型（Move/NW/N/NE/E/SE/S/SW/W），设计器可能按类型细化行为。采用"public enum + 公开 `hitTestHandlePublic` 包装方法"最小暴露。
- **坐标域**：`x, y` 为窗口屏幕坐标（与既有 handleEvent 的 mousePos 同域），内部 hitTest 亦用屏幕坐标（m_handleAreas 屏幕坐标）——一致，无换算。
- **性能**：C ABI 查询每次 updateHandleAreas（O(9)）——按下沿单次查询，开销可忽略。

### 3.3 P0-3：拖拽几何回调（rect filter）（决策：局部坐标，setRect 前插槽）

需求 C 签名：

```c
typedef int (*UIHandleRectFilter)(UIControlHandle target, float* ioX, float* ioY,
                                  float* ioW, float* ioH, void* userData);
int SetHandleRectFilter(UIInstance, UIControlHandle handle, filter, userData);
```

**坐标域决策**：filter 收到 **target 局部坐标**（即 `setRect` 直接消费的 rect），而非屏幕坐标。理由：
- `updateResize/updateDrag` 中局部 rect 已算好（:250/:284 的 `localLeft/localWidth` 等），插槽无需再转换。
- 设计器吸附逻辑最终写回的是**逻辑坐标**（模型存逻辑 rect），局部坐标即逻辑坐标——语义闭环。
- 权衡：filter 拿不到屏幕坐标（外部算法如需像素级对齐需自行转换）——文档明确，当前吸附需求用局部坐标足够。

**调用时机**：updateResize / updateDrag 各算完局部新 rect 后、`m_target->setRect(...)` 前调用：

```cpp
SRect newLocal(localLeft, localTop, localWidth, localHeight);
if (m_rectFilter) {
    float io[4] = {newLocal.left, newLocal.top, newLocal.width, newLocal.height};
    int use = m_rectFilter(targetHandle, &io[0], &io[1], &io[2], &io[3], m_rectFilterUser);
    if (use == 1) newLocal = SRect(io[0], io[1], io[2], io[3]);   // 用修正值
    /* use==0 → 保持 newLocal（原值） */
}
m_target->setRect(newLocal);
```

**决策点**：
- **filter 返回 0 / 1 语义严格按需求**：1 → 用修正值；0 → 用原值（整帧计算结果）。不引入其他返回值。
- **filter 在"最小尺寸约束"之后还是之前**？——**在约束之后**（先保证不小于 minSize，再给应用修正权）。若 filter 又想violate minSize，允许（应用自己负责边界）——文档标注"filter 修正优先于 minSize 钳位"。
- **anchor 方向联动**：NW/W/SW（改 left 同时 width 反方向变化）场景下，updateResize 已算出**最终**局部 rect（含 left 修正），filter 修改的是最终 rect 四要素——应用修改 ioX 后 width 不自动联动，**四值独立、以 filter 输出为准**。语义简单、可预期。
- **move 与 resize 均经过 filter**（需求：drag 路径也插槽）。

### 3.4 P0-4：配置 C ABI（决策：独立 setter 组，非属性系统）

HandleControl 非 JSON 控件（`kControlTypeHandleControl = "handle-control"`，仅 C ABI 返回，无属性系统）。配置走**专用 C ABI setter**（非 SetFloat/SetBool 属性链），避免污染属性键体系：

```c
int HandleSetMoveVisible(UIInstance, handle, int show);                 // 控制 Move 手柄
int HandleSetSize(UIInstance, handle, float size);                      // 手柄方块尺寸
int HandleSetMinSize(UIInstance, handle, float w, float h);             // 最小约束
int HandleSetColors(UIInstance, handle, UIColor fill, UIColor border,   // 手柄填充/边框/高亮/选择框
                    UIColor active, UIColor selection);
```

优先级判定：**按引擎判断**。若 P0-3 落地，设计器倾向保留 Move 手柄并经 filter 吸附 → **P0-4 仅需 `HandleSetMoveVisible`**（策略开关），其余（size/colors/minSize）缺省可用、设计器暂不覆盖。**建议实施范围收敛为 `HandleSetMoveVisible` 单函数**，其余列为可选 backlog——避免过度设计（见设计文档规范"能简单就别复杂"）。

---

## 4. API 设计

### 4.1 核心库（HandleControl.cpp/.h）

```cpp
// HandleControl.h 新增
public:
    enum class HandleType : uint8_t {          // 由 private 转 public（P0-2 输出）
        None, Move, NW, N, NE, E, SE, S, SW, W
    };
    void setTarget(Control* target);           // 既有，补充 NULL 防护（target==nullptr → detach()）
    HandleType hitTestHandle(float mx, float my);       // 由 private 转 public（P0-2 查询）
    void updateHandleAreas();                  // 由 private 转 public（P0-2 先刷新，当前为 draw 内私有）

    using RectFilter = int (*)(UIControlHandle target, float* ioX, float* ioY,
                               float* ioW, float* ioH, void* userData);
    void setRectFilter(RectFilter filter, void* userData);

private:
    RectFilter m_rectFilter       = nullptr;
    void*      m_rectFilterUser   = nullptr;
```

### 4.2 C ABI（UICornerstoneAPI.h + .cpp）

```c
/* ============ HandleControl 集成扩展 ============ */
// P0-1：切换附加目标。target==NULL → detach（移出容器+恢复光标）。
UICORNERSTONE_API int UICornerstone_SetHandleTarget(UIInstance instance,
    UIControlHandle handle, UIControlHandle target);
// P0-2：手柄命中查询。命中返回 1 并写 outHandleType（映射 HandleType 枚举，None=0）；未命中/非手柄控件返回 0。
UICORNERSTONE_API int UICornerstone_HandleHitTest(UIInstance instance,
    UIControlHandle handle, float x, float y, int* outHandleType);
// P0-3：拖拽几何过滤回调。filter==NULL 清除回调。
UICORNERSTONE_API int UICornerstone_SetHandleRectFilter(UIInstance instance, UIControlHandle handle,
    UIHandleRectFilter filter, void* userData);
// P0-4（收敛）：Move 手柄可见性（设计器策略开关）。
UICORNERSTONE_API int UICornerstone_SetHandleMoveVisible(UIInstance instance, UIControlHandle handle, int show);
```

### 4.3 Binding（UICornerstone.h/.cpp + DynamicApi.h/.cpp）

```cpp
// UICornerstone 实例方法
bool SetHandleTarget(Control handle, Control target);     // target 无效(空 Control) = detach
bool HandleHitTest(Control handle, float x, float y, int& outHandleType);
bool SetHandleRectFilter(Control handle, UIRectFilterFn filter, void* userData);
bool SetHandleMoveVisible(Control handle, bool show);
```

DynamicApi：新增 `fnSetHandleTarget/fnHandleHitTest/fnSetHandleRectFilter/fnSetHandleMoveVisible` 指针 + RESOLVE。
Binding 层回调类型复用核心库 `UIHandleRectFilter` 签名（C 函数指针，跨 DLL 安全）。

---

## 5. 实现要点与验收（对照需求 §验收）

| 需求 | 实现 | 验收 |
|---|---|---|
| P0-1 SetHandleTarget | C ABI 校验 + `hc->setTarget(v) or detach()`；`setTarget` 加 `if (m_target == target) return;` 幂等 | 连续切换 3 次 target → 手柄跟随新 target（drawRect）；NULL → HandleControl 移出容器（getParent()==nullptr）+ 光标恢复默认 |
| P0-2 HandleHitTest | 公开 HandleType + 公开 hitTestHandle/updateHandleAreas；C ABI 先刷新区域再命中 | 与引擎内部一致：8 个手柄中心 8×8 内命中返回对应类型；手柄外（含 target 本体非手柄区）返回 0 |
| P0-3 RectFilter | updateResize/updateDrag setRect 前插槽调 filter；返回 1 用修正值 | 拖拽过程中 filter 内 snap 修改 → setRect 立即生效（视觉实时吸附，非松手收敛）；filter 返回 0 → 行为与现状一致；move 与 resize 均经过 filter |
| P0-4（收敛） | HandleSetMoveVisible 透传 setMoveHandleVisible | 传 0 → Move 手柄不绘制/不可拖 |
| Binding 包装 | DynamicApi + 4 方法 | binding 冒烟：SetHandleTarget 切换/detach、HandleHitTest 命中、RectFilter 回调触发 |

## 6. 影响面与风险

- **HandleType/updateHandleAreas/hitTestHandle 转 public**：仅放宽可见性，不改逻辑；`handleTypeToInt` 映射需同步文档（C ABI 输出值契约）。
- **setTarget(NULL) 防护**：新增分支（target==nullptr → detach），可防 C ABI 误用崩溃——纯收益。
- **setTarget 幂等快路径**：高频重复选中 O(1) 返回，无行为差异。
- **filter 插槽**：仅拖拽路径（updateResize/updateDrag）新增一次函数指针判空调用——非拖拽帧零开销；指针默认为 nullptr 返回，无性能影响。
- **三后端无涉**：纯逻辑层 + C 函数指针（跨 DLL），不触碰渲染/后端。
- **设计器迁移**：选中切换改 SetHandleTarget（删除销毁重建）；按下沿先 HandleHitTest；吸附逻辑移入 filter（删除轮询快写回）；move 路径可统一交 HandleControl 或经 SetHandleMoveVisible 关闭后仍走本体拖动。

## 7. 待审核问题

1. **P0-3 filter 坐标系**：采用 **target 局部坐标**（= 逻辑 rect，setRect 直接消费）——吸附写回零转换。如设计器统一用屏幕坐标的吸附算法，需引擎侧预留屏幕→局部换算或改为屏幕坐标回调，请裁决。
2. **P0-3 filter 与 minSize 顺序**：先 minSize 钳位后 filter（filter 修正可越过 minSize）。确认接受"filter 优先级高于最小尺寸"。
3. **P0-4 收敛**：仅实施 `HandleSetMoveVisible`，setSize/colors/minSize 列入 backlog（按引擎判断，非阻塞）——接受？
4. **HandleType C ABI 值映射**：None=0、Move=1、NW=2、N=3、NE=4、E=5、SE=6、S=7、SW=8、W=9（按 enum 顺序）——稳定契约，确认？
5. **P1 项（第二批 §5/6/7）**：SetControlId binding 包装为**已完成项**（上批已实施）；schema 裁剪布局语义键（flow-weight/anchor 等从可编辑键清单排除）与字体预热为**存续 backlog**，本次不实施——确认？

---

## 8. 待提交配套改动（实施时随批）

- 用户手册 `docs/appendix/capi.html`：8.x 节补 4 个 C ABI；binding.html 补 4 方法。
- `design/API_Mapping_Table.md`：补 HandleControl 集成行（C ABI + Binding 双列）。
- `test/test_handlecontrol.cpp`：新增 C ABI 用例（target 切换/detach/命中/filter）。
- 复核意见文档：写入 `requirements/HandleControl_CABI_Integration_Design_复核意见.md`（确认后实施）。