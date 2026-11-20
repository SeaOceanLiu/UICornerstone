# CR20261117001 — Shape 图元绘制裁剪到控件 rect（引擎侧分析 + 设计）

> 状态：**closed（2026-11-19 设计器联调通过；设计器/引擎两项均 closed）**
> 来源：`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_2026-11.md` → CR20261117001
> 关联：CR20261112001（box-shadow 柔光）、`Shape` 单图元/多图元绘制、`RenderDevice::pushClipRect/popClipRect`
> 边界：只处理 Shape 自身图元绘制裁剪；不改变 box-shadow、背景/边框/焦点环、命中与布局。

## 1. 现状与根因

`Shape::draw()` 的图元绘制没有裁剪：

- 单图元：`src/Shape.cpp:233-286`，`r = getDrawRect()` 后直接 `fillRect/drawRoundRect/drawEllipse/drawPolyline/drawPolygon`；
- 多图元：`drawPrimitiveAt()`（`src/Shape.cpp:171-217`）按 `getDrawRect()` 左上角 + 局部坐标直接绘制；
- 图元 `rect`/`points` 超出控件范围时，内容会画到 Shape 之外，覆盖相邻控件，破坏布局隔离。

`RenderDevice` 已提供 `pushClipRect/popClipRect`，且三后端均为**与栈顶相交**语义（SDL3/SFML/raylib），父容器 `clip-children` 已使用同一机制。Shape 只需在自身图元绘制期间压入 `getDrawRect()`。

## 2. 目标与非目标

### 2.1 目标

1. 单图元与 `primitives[]` 的 fill/stroke/points 统一裁剪到 Shape 的 `getDrawRect()`；
2. 新增 Shape 布尔属性 `clip`（默认 **true**），默认修复溢出；`clip=false` 保留旧行为；
3. `box-shadow` 仍绘制在控件外侧，**不受**裁剪影响（CR20261112001 语义）；
4. 背景/边框/焦点环、子控件绘制不受影响；
5. 与父容器 `clip-children` 自动求交集（嵌套 clip）；
6. 命中/布局/选中仍以控件 rect 为准；
7. 不新增 C ABI/Binding 函数，属性走现有 `SetBool/GetBool`。

### 2.2 非目标

- 不做图元级独立裁剪开关（只按整个 Shape rect）；
- 不做抗锯齿羽化/1px 余量（后端 scissor 为硬裁）；
- 不改变 Shape 图元坐标缩放/`points` 映射规则；
- 不处理 `box-shadow` 的 offset 间隙（另属 CR20261112001 实现行为）。

## 3. 设计

### 3.1 裁剪范围

`Shape::draw()` 结构调整：

```
if (!m_visible) return;
ControlImpl::beforeDraw();          // 背景 + box-shadow：不裁剪
RenderDevice* dev = getRenderDevice();
if (!dev) return;

const SRect clipRect = getDrawRect();
const bool doClip = m_clipShape && clipRect.width > 0 && clipRect.height > 0;

if (doClip) dev->pushClipRect(clipRect);
    // 仅图元绘制（单图元 switch / primitives 循环）
if (doClip) dev->popClipRect();

ControlImpl::draw();                // 子控件：不受 Shape 图元裁剪影响
ControlImpl::afterDraw();           // 边框 / 焦点环：不裁剪
```

- 多图元路径：一次 push/pop 包住全部 `drawPrimitiveAt()`，不逐图元压栈；
- 单图元路径：push/pop 包住 switch；
- 图元为空或 rect 无效：跳过图元绘制，保持子控件/边框逻辑；
- `beforeDraw` 在 push 之前，因此背景和 box-shadow 不受影响；
- `afterDraw` 在 pop 之后，边框/焦点环不受影响。

### 3.2 裁剪矩形

- 使用 `getDrawRect()`（已含位置、xScale/yScale、父链变换），与 Panel `clip-children` 一致；
- `pushClipRect` 与栈顶相交，父容器已有裁剪时自动求交集；
- 硬裁剪：描边正好压在边界时可能被切掉半像素，属预期（CR 允许）；
- 不在 `drawPrimitiveAt` 内部重复 push，避免每图元开销。

### 3.3 `clip` 属性

- `PropertyNames`：新增 `kClipShape = "clip"`（Shape 专用；与 `clip-children` 不冲突）；
- `Shape`：新增 `bool m_clipShape = true;`；`setBoolProperty/getBoolProperty` 处理 `clip`，其余转发基类；
- JSON：`shape` 定义新增 `"clip": { "type": "boolean", "x-default": "true", "description": ... }`；
- `LayoutParser::parseShape` 显式解析 `clip`（`parseCommonProperties` 不覆盖该键）；
- C ABI/Binding：无新增函数；`UICornerstone_SetBool(inst, shape, "clip", 0/1)`、Binding `SetBool/GetBool` 直接可用；
- 默认 true：历史布局中溢出图元本来就会干扰其他控件，默认裁剪属于缺陷修复；需要旧行为的布局显式 `"clip": false`。

### 3.4 与现有绘制/事件的关系

| 项 | 行为 |
|---|---|
| `box-shadow` | `beforeDraw` 中绘制，在 clip 外，保持控件外侧柔光 |
| 背景色 | `beforeDraw` 中绘制，不裁剪（本就在 rect 内） |
| 边框/焦点环 | `afterDraw` 中绘制，不裁剪 |
| 子控件 | `ControlImpl::draw()` 在 pop 后，不受 Shape 图元裁剪影响 |
| 命中/布局 | 不变，仍以 Shape rect 为准 |
| 父容器 `clip-children` | push/pop 自动交集；Shape 裁剪不会扩大父裁剪 |
| 缩放 | `getDrawRect()` 已含缩放，图元坐标按现有 `sx/sy` 映射 |

### 3.5 兼容性

- 未写 `clip` 的布局：默认 true，溢出图元开始被裁剪（缺陷修复，可能改变旧布局视觉）；
- 需要保留溢出的布局：显式 `"clip": false`；
- `clip` 读写不影响已有 `shape`/`fill`/`stroke`/`line-width`/`radius`/`ring-width` 属性；
- `box-shadow`、菜单/面板、Label 等其它控件零影响。

## 4. 影响面与 ABI

| 层 | 影响 |
|---|---|
| C++ | `Shape` 增加成员与 `setBoolProperty/getBoolProperty` override → **类布局/vtable 变化，DLL/头文件/C++ 使用方需同批重编译**；无其它类改动 |
| C ABI | 无新增函数；现有 `SetBool/GetBool` 接受 `clip` |
| Binding | 无新增封装；透传 `clip` |
| JSON/Schema | `shape.clip` 选填，默认 true；旧布局仍合法 |
| 设计器 | 属性面板按 schema 自动生成 `clip` 行；无需专用 UI |
| 渲染 | 仅 Shape 图元绘制增加一次 push/pop；三后端复用现有 scissor/clip 栈 |

## 5. 测试计划

新增 `test/test_cr_shape_clip.cpp` + `layouts/cr_shape_clip.json`，注册 `test/CMakeLists.txt` 并纳入 `layouts/` 复制条件。

| # | 用例 | 断言 |
|---|---|---|
| C1 | 单图元 rect 越界（负 x/y、超宽高） | 默认 clip：仅 rect 内像素绘制；外部像素保持背景 |
| C2 | circle/ellipse/round-rect 描边越界 | 外部不绘制；内部图元正常 |
| C3 | polyline/polygon points 越界 | 越界段被裁剪；全部越界时不绘制 |
| C4 | `primitives[]` 部分图元越界 | 越界图元裁剪，其他图元不受影响 |
| C5 | `clip=false` | 保持旧行为，图元溢出到 Shape 外 |
| C6 | box-shadow + 越界图元 | 阴影仍在控件外侧可见，不受裁剪影响 |
| C7 | 父 Panel `clip-children` + Shape clip | 两者交集；父裁剪更小时以父为准 |
| C8 | 缩放 + 父偏移 | 父 Panel 偏移 + Shape 1.5×/2× 缩放：裁剪边界随 `getDrawRect()` 全局坐标，不偏移 |
| C9 | C ABI/属性 | `SetBool/GetBool("clip")` 读写；JSON 默认 true / 显式 false |
| C10 | 回归 | `test_shape`、`test_soft_shadow`、多图元示例（house）不变；新增有意越界的旧布局样本，`clip=false` 像素与旧版一致 |
| C11 | C ABI/Binding | `SetBool/GetBool("clip")` 透传；Binding 同名封装可用 |
| C12 | 绘制顺序 | `box-shadow` 在 clip 外、`primitives` 图元在 clip 内，叠加时顺序正确 |
| C13 | 设计器 schema | schema 自动生成 `clip` 行（设计器侧验证，非阻塞） |

三后端各跑一遍；SDL3 全量回归仅允许 4 项历史失败；`validate_layout --strict` 通过。

## 6. 用户手册与文档更新计划

| 文档 | 更新 |
|---|---|
| `docs/controls/shape.html` | 属性表新增 `clip`；补充“图元溢出默认裁剪到 rect、`clip=false` 恢复旧行为”；补多图元越界示例与截图（越界裁剪前后） |
| `docs/appendix/declarative-syntax.html` | `shape.clip` 键说明 |
| `docs/appendix/properties.html` | `clip` 行（Bool，默认 true） |
| `docs/schema/declarative-ui.schema.json` | `shape.clip` 字段 |
| 设计器 `docs/12-controls-media.html` | “当前引擎不裁剪”提示改为“超出部分会被裁剪”；`clip=false` 恢复旧行为；auto=2 增加 `clip` 行/开关/保存打开断言 |
| `README.md` | 特性/测试列表补 Shape 裁剪 |
| `design/API_Mapping_Table.md` | 增 CR20261117001 行（无新增 C ABI/Binding） |

截图：`docs/assets/controls/shape-clip.png`（越界图元默认裁剪 vs `clip:false` 溢出），由 `manual_shots` 布局生成；示例布局放入 `docs/assets/controls/layouts/shape-clip.json`。

## 7. 风险与回退

| 风险 | 处理 |
|---|---|
| 旧布局依赖溢出绘制 | 默认裁剪是缺陷修复；文档标注 `clip:false` 兼容开关 |
| 描边边界被切半像素 | 硬裁剪预期；截图/像素测试允许 1px 误差 |
| 父裁剪交集错误 | 复用 `pushClipRect` 既有相交语义；C7 覆盖 |
| box-shadow 被误裁 | push 在 `beforeDraw` 之后；C6 |
| 子控件被误裁 | pop 后再 `ControlImpl::draw()`；回归 |
| 三后端裁剪差异 | 三后端同测；边界像素容差 1px |
| C++ ABI | Shape 布局/vtable 变化，同批重编译声明 |

## 8. 验收对照

| CR 验收项 | 设计落点 |
|---|---|
| 单图元越界裁剪 | §3.1 / C1~C2 |
| 点集越界裁剪 | §3.1 / C3 |
| 多图元越界裁剪 | §3.1 / C4 |
| box-shadow 不受裁剪 | §3.4 / C6 |
| 背景/边框/焦点环不受影响 | §3.4 |
| 命中/布局不变 | §3.4 |
| 父 clip-children 交集 | §3.2 / C7 |
| 缩放边界正确 | §3.2 / C8 |
| 属性/schema/C ABI | §3.3 / C9 |
| 三后端一致与回归 | §5 / C10 |

## 9. 实施顺序

1. `PropertyNames` + `Shape` 成员/属性 override + `draw` 裁剪；
2. `LayoutParser` / schema；
3. 新测试与布局样本 + CMake；
4. 文档/截图/README/API 映射；
5. 三后端验证 → 设计器联调 → 关闭引擎项。

## 10. 评审意见处理（2026-11-18，设计器）

设计器结论：**通过（设计阶段）**，N1~N5 为非阻塞完善项。处理如下：

| # | 意见 | 处理 |
|---|---|---|
| N1 | 补“父容器偏移 + 缩放”组合用例 | §5 C8 扩展：父 Panel 偏移 + Shape 1.5×/2× 缩放，校验 `getDrawRect()` 全局坐标口径 |
| N2 | 设计器联调项写入设计 | §6 增加设计器侧：`docs/12` 溢出提示改为“超出部分会被裁剪”、属性面板自动 `clip` 行、auto=2 开关/保存打开回归 |
| N3 | `clip` 键命名边界 | `PropertyNames` 注释与 schema 描述注明“Shape 专用，与 `clip-children` 不同；未来通用 `clip` 需兼容别名” |
| N4 | 边界描边文案 | schema/用户手册注明“压在 rect 边界上的描边可能被裁切半像素” |
| N5 | 旧布局回归样本 | C10 增加有意越界的旧布局样本，`clip=false` 与旧版像素一致 |
| 测试补充 | C11~C13 | C11 C ABI/Binding `SetBool/GetBool("clip")`；C12 `clip` + box-shadow + primitives 绘制顺序；C13 设计器 schema 行（设计器侧验证） |

## 11. 实施记录（2026-11-18）

### 11.1 落地文件

- `include/PropertyNames.h`：`kClipShape = "clip"`（Shape 专用；注释说明与 `clip-children` 区别）。
- `include/Shape.h` / `src/Shape.cpp`：
  - `m_clipShape = true`；`setBoolProperty/getBoolProperty` 处理 `clip`；
  - `draw()` 仅在图元绘制期间 `pushClipRect(getDrawRect())`/`popClipRect()`；`primitives[]` 共一次；`beforeDraw`（背景 + box-shadow）与 `afterDraw`（边框/焦点环）、子控件均在裁剪外。
- `src/LayoutParser.cpp`：`parseShape` 解析 `clip`（默认 true）。
- `docs/schema/declarative-ui.schema.json`：`shape.clip`（Bool，默认 true，注明边界描边可能被裁切）。
- `test/test_cr_shape_clip.cpp` + `layouts/cr_shape_clip.json` + `test/CMakeLists.txt`；`docs/assets/controls/layouts/shape-clip.json` + `shape-clip.png`。
- 文档：`docs/controls/shape.html`（属性表 + 4.19.9 图元裁剪 + 截图）、`docs/appendix/{declarative-syntax,properties}.html`、`README.md`、`design/API_Mapping_Table.md`。

### 11.2 验证结果

- `test_cr_shape_clip`：SDL3 / SFML / raylib 均 **19/19 通过**（单/多图元、polygon points、clip=false、box-shadow、父 clip-children 交集、缩放、JSON 默认/显式 false、C ABI Set/Get）。
- 回归：`test_shape` 17/17；`test_soft_shadow` 13/13；SDL3 全量回归仅 4 项历史失败，无新增。
- `validate_layout layouts/*.json --strict` 与 `docs/assets/controls/layouts/shape-clip.json --strict` 均 PASS。
- 影响面：`Shape` 类布局/vtable 变化，DLL/头文件/C++ 使用方需同批重编译；C ABI/Binding 无新增函数。
- Release DLL SHA256：`7fcbce6ebe8b7056da98c42e0e72bbd3a0116727730878f5de782df6b15aa568`；已 `robocopy /MIR` 同步设计器 `subModules/UICornerstone`。
- 2026-11-19 设计器联调：schema 自动 `clip` 行、`clip=false` 保存/打开、溢出提示文案、auto=2/3、strict 均通过；CR closed。

## 修订记录

- 2026-11-18 v1：引擎侧首版设计；采用 `clip` 属性默认 true，仅裁剪图元绘制，box-shadow/背景/边框/子控件不受影响；裁剪复用 `pushClipRect` 交集语义；待评审。
- 2026-11-18 v2：按设计器评审补 N1~N5 与 C11~C13；设计阶段通过，进入实施。
- 2026-11-18 v3（实施）：`clip` 属性 + 图元绘制裁剪落地；三后端 19/19；补 §11 实施记录；引擎项 closed。
