# BindingCanvasGrid_Design — 画布网格渲染驱动能力设计（评审修正版 v2）

> 提出方：CornerstoneDesigner（画布/设计器）
> 需求来源：`requirements/UICornerstone_binding需求_画布网格图元纹理.md`（2026-09-04）
> 评审意见：`requirements/UICornerstone_BindingCanvasGrid_Design_评审意见.md`（2026-09-13，结论：基本通过，修正后实施）
> 复核意见：`requirements/UICornerstone_BindingCanvasGrid_Design_复核意见_v2.md`（2026-09-13，结论：**通过——放行实施**）
> 状态：**v3 已放行，实施中**

## 0. 评审修正摘要（v1 → v2）

| 编号 | 评审要求 | 本版处理 |
|---|---|---|
| §2.1 | SDL3 `drawTexture` 忽略 srcRect（阻塞前置缺陷） | 新增实施项 I0：恢复 srcRect 分支 + 三后端像素级回归（含 CENTER_CROP）；现状表与风险节修正 |
| §2.2 | TILE 边缘裁剪不能依赖渲染器 clip | §3.2 改为 Actor 显式 `dst∩drawRect + srcRect 部分裁剪`，验收补边缘像素断言 |
| §2.3 | C ABI `ActorSetSourceRect` 必补（Binding 为纯 C ABI 动态封装） | §4.3 定为必补（含清除语义），DynamicApi 同步 |
| §4-Q2 | tile×缩放取整、match-parent-rect 组合语义 | §3.2 增补 |
| §4-Q3 | C ABI 结论 | 必补 |
| §5 | 性能验收（大画布+小瓦片实测、多单元瓦片推荐） | §5 增补 |

v3 复核（`requirements/UICornerstone_BindingCanvasGrid_Design_复核意见_v2.md`）：**通过——放行实施**。
非阻塞建议 3 条吸收情况：
- 建议 1（source-rect×scaleType 界定）→ §3.3 补界定句；
- 建议 2（末块 srcRect clamp + 非整数缩放用例）→ §3.2 clamp、§5 用例；
- 建议 3（文档自包含）→ 两份评审意见已复制进 `requirements/`。

## 1. 问题与目标

设计器中央画布需要三类动态内容，当前实现均被迫退化为「按 resize 重建大纹理 / 临时 Image 控件」：

| 内容 | 特性 | 现状痛点 |
|---|---|---|
| 网格 | 规则重复图案，resize/splitter 只变区域 | 每次 resize 重生成整张 PNG（无压缩数 MB）→ 引擎解码 ≈ 每帧数百 ms |
| 覆盖层（选中框/8 向控点/对齐线/橡皮筋） | 动态、少量、位置实时变化 | C ABI 有 Shape 多图元但 Binding 未暴露，只能退化临时 Image |
| 程序化纹理 | 一次性上传、之后只改矩形 | 无 raw RGBA 入口，必须 PNG 编解码往返 |

目标：resize/splitter 时**纹理零重建、密度恒定、性能恒常数**；覆盖层矢量图元化。

## 2. 现状核实（源码事实，含评审复核）

| 需求 | 现状 |
|---|---|
| 1. Shape 多图元 | C ABI 完整：`ShapeAddPrimitive / ShapeSetPrimitiveColor / ShapeSetPrimitiveFloat / ShapeSetPrimitivePoints / ShapeClearPrimitives`（UICornerstoneAPI.cpp:1330-1374）；**Binding 未包装**（DynamicApi.h 仅 fnShapeSetPoints/fnShapeMapToDrawPoint） |
| 2. 纹理平铺 | `kScaleTypeStretch/FitCenter/CenterCrop/None` 四枚举；Actor 绘制 4 分支；**无 tile**；渲染器**无控件级裁剪**（clip 为视口/裁剪栈级） |
| 3. 源矩形 | ⚠️ **前置缺陷（评审 §2.1）**：SDL3 后端 `drawTexture` 忽略 srcRect（`SDL_RenderTexture(..., nullptr, &sdlDst)`，sdl3/RenderDevice.cpp:247-253）；SFML（532）与 raylib（585）正确使用 srcRect。**Actor 的 CENTER_CROP 在 SDL3 下当前即坏**（整图拉伸）。修复前需求 3 与 tile 边缘裁剪均不可行 |
| 4. raw 纹理 | MemoryResourceProvider `registerMemory/adoptMemory` 存**字节流**，引擎按 PNG/JPEG 解码；后端 SDL3 有 `SDL_CreateTexture(...RGBA8888...)` 路径（capture 在用）；**无「RGBA 直接纹理」注册语义** |

## 3. 架构选择与关键设计决策

### 3.0 实施项 I0（新增，前置）：SDL3 drawTexture 恢复 srcRect

- 恢复 sdl3 `drawTexture` 的 srcRect 分支：`SDL_RenderTexture(m_renderer, sdlTex->native(), &sdlSrc, &sdlDst)`（srcRect 有效时；否则 nullptr）。
- **三后端像素级回归**（评审要求，不能只测 API 返回值）：同一张双色纹理（左红右蓝），分别以 CENTER_CROP / source-rect 渲染，断言裁剪区域像素颜色（SDL3/SFML/raylib 各一）。
- 本项同时修复既有 CENTER_CROP 在 SDL3 的潜伏 bug，是需求 2/3 的共同前置。

### 3.1 需求 1：Shape 多图元 Binding（决策：照搬 C ABI，无引擎改动）

Binding 包装模式已确立（`ShapeSetPoints` 同款：UICornerstone.h 声明 + UICornerstone.cpp 经 `Dyn::API().fnXxx` + DynamicApi.h 函数指针 + DynamicApi.cpp `RESOLVE`）。五方法一一对应，**零引擎改动**。

排除方案：走通用 `Set*` 属性键——图元是**对象/数组数据**非属性系统键（与 `points` 同款），专用方法语义清晰，维持既有惯例。

### 3.2 需求 2：纹理平铺（决策：新增 `kScaleTypeTile`，Actor 显式边缘裁剪平铺）

- 新增 `ScaleType::TILE` 分支：按瓦片逻辑尺寸 × 控件复合缩放换算分块尺寸，整数次循环 `drawTexture` 铺满；**末行/末列瓦片显式 `dst∩drawRect` 求交 + 对应 srcRect 部分裁剪**（依赖 I0 的 srcRect 修复；不依赖渲染器 clip——渲染器无控件级裁剪）。
- **取整规则（评审 Q2 细化）**：分块尺寸 `tileW = texW × getScaleXX()` 取整（四舍五入），步进累加用**同一取整值**（`pos[i] = origin + i×tileW`），末块 dst.width = `drawRect.right - pos[last]`——避免逐块单独取整产生 1px 缝隙/重叠。
- **末块 srcRect clamp（v3 复核建议 2）**：末块对应 `srcTail.w = lastDst.w / (tileW / srcW)` 取值后 `min(…, srcW)` 钳制，避免浮点误差越界（SDL3 `SDL_RenderTexture` 对越界 src 行为不稳定）。
- **source-rect 与 tile 组合语义**（评审 Q2 确认）：先按 source-rect 从纹理取子区域，再以该子区域为瓦片平铺；source-rect 空 = 整图。
- **与 match-parent-rect 组合**：match-parent-rect 仅影响目标 rect 尺寸来源，不改变平铺语义（分块尺寸仍由瓦片逻辑尺寸 × 复合缩放决定）。
- **与其它 scaleType 互斥**：`setScaleType` 覆盖（含 setEnumProperty 分发）。
- 常量 `kScaleTypeTile = "tile"`、Actor::setEnumProperty 分发、schema `scale-type` 枚举各加一处。
- 排除方案：后端新增 drawTextureTiled——三后端 × 能力位 × 测试面大；sourceRect 语义模拟平铺——泄漏实现细节给应用层。

### 3.3 需求 3：源矩形（决策：Actor 增加 `source-rect`，C ABI 必补）

- **前置**：实施项 I0（SDL3 srcRect 修复）落地后本需求才可用。
- Actor 增加成员 `SRect m_sourceRect`（空=整图）＋ `setSourceRect/getSourceRect`；绘制分支 `drawTexture(tex, 有效?&m_sourceRect:nullptr, &drawRect)`（复用现有 drawTexture 签名，除 I0 外三后端零新增）。
- **source-rect × scaleType 界定（v3 复核建议 1）**：本期 source-rect 对 **STRETCH 与 TILE** 生效（方案 B 移动窗口 / 子区域瓦片平铺）；FIT_CENTER / CENTER_CROP / NONE 忽略 source-rect（后续按需扩展），测试范围与此保持一致。
- **C ABI 必补（评审 §2.3）**：`UICornerstone_ActorSetSourceRect(inst, handle, x, y, w, h)`——Binding 是纯 C ABI 动态封装（经 Dyn::API 解析），无 C ABI 导出则 Binding 无实现路径；清除语义：`x < 0 或 w ≤ 0 或 h ≤ 0` → 清除为整图。DynamicApi.h/.cpp 同步函数指针与 RESOLVE。
- 属性系统不设键（复合数据，与 points 同款走专用方法）；JSON 层 `sourceRect:{x,y,w,h}` 本期不做（设计器走 Binding）。

### 3.4 需求 4：raw RGBA 纹理（决策：本期**不实施**，保留扩展点）

- 落地路径：MemoryResourceProvider 条目增加「格式标记（raw-rgba8888 / 图片文件）」＋ Texture 加载链路分支（图片解码 vs SDL_CreateTexture 直灌）。
- 不实施理由：a) 资源加载链全局改动风险高；b) 需求 2/3 组合（单元瓦片平铺）已达成「resize 零重建、性能恒常数」目标；c) 若 §5 性能验收在「大画布 + 小瓦片」不达标，方案 B 与 raw RGBA 一并复议（评审 Q1 有条件同意）。

### 3.5 选型组合（回应需求 5）

| 方案 | 依赖 | 结论 |
|---|---|---|
| A（网格铺满） | I0 + 需求 2（tile） | **实施** |
| C（覆盖层） | 需求 1（Shape 图元 Binding） | **实施** |
| B（大纹理+源矩形） | I0 + 需求 3 | **实施需求 3**（含 C ABI）；需求 4 不实施，性能不达标时复议 |

## 4. API 设计

### 4.1 需求 1：Binding 新增（C ABI 已有，零引擎改动）

```cpp
// UICornerstone.h（与 ShapeSetPoints 同区）
int  ShapeAddPrimitive(Control& sh, const std::string& type, float x, float y, float w, float h);
void ShapeClearPrimitives(Control& sh);
void ShapeSetPrimitiveColor(Control& sh, int idx, const char* prop, UIColor value);
void ShapeSetPrimitiveFloat(Control& sh, int idx, const char* prop, float value);
void ShapeSetPrimitivePoints(Control& sh, int idx, const std::vector<std::pair<float,float>>& pts);
```

实施面：UICornerstone.h 声明 / UICornerstone.cpp 实现 / DynamicApi.h 五指针 / DynamicApi.cpp 五 RESOLVE。

### 4.2 实施项 I0 + 需求 2：SDL3 srcRect 修复 + tile 平铺

- `src/backend/sdl3/RenderDevice.cpp`：恢复 srcRect 分支。
- `PropertyNames.h`：`kScaleTypeTile = "tile"`。
- `Actor`：`enum ScaleType` 加 `TILE`；`setEnumProperty(kScaleType)` 加分支；`draw()` 加 TILE 分支（显式边缘裁剪，见 §3.2）。
- schema：`scale-type` 枚举加 `"tile"`。
- 文档：actor 4.1.2 属性表、7.2 速查、7.3 枚举速查（生成器自动）。

### 4.3 需求 3：source-rect

- `Actor`：`m_sourceRect`（空=整图）＋ `setSourceRect(SRect)/getSourceRect()`＋ 绘制分支 `drawTexture(tex, 有效?&m_sourceRect:nullptr, &drawRect)`。
- C ABI（必补）：`UICornerstone_ActorSetSourceRect(inst, handle, x, y, w, h)`；`x<0 || w≤0 || h≤0` → 清除为整图；返回 1 成功 / 0 无效句柄。
- Binding：`ImageSetSourceRect(Control& img, float x, float y, float w, float h)`（经 Dyn::API().fnActorSetSourceRect）。
- 属性系统不设键；JSON 层本期不做。

## 5. 实现要点与验收

| 项 | 实施范围 | 验收标准 |
|---|---|---|
| I0 SDL3 srcRect | sdl3/RenderDevice.cpp | 三后端像素级回归：双色纹理 CENTER_CROP 与 source-rect 各断言裁剪区颜色（SDL3 与 SFML/raylib 结果一致）；既有 CENTER_CROP 用例在三后端全绿 |
| Shape 图元 Binding | binding/ 四文件 | 样例：Shape 加 3 图元（空心 rect stroke 蓝 / 实心 rect 控点 / polyline 参考线），逐帧改点集实时生效；ClearPrimitives 清空重加正常 |
| tile 平铺 | PropertyNames/Actor/schema/文档 | `scale-type=tile`：16×16 瓦片铺满 400×300 rect，**边缘像素断言**末行/末列正确裁剪（不溢出控件矩形）；**非整数复合缩放用例（v3 复核建议 2）**：xScale=1.3（texW 16 → tileW 21），锁定取整/步进/末块行为；resize 仅改 rect、纹理零重建；与 stretch/fit 互斥切换正常 |
| source-rect | Actor/C ABI/Binding/文档 | 大纹理 `ActorSetSourceRect(x,y,w,h)` 显示子区域；清除语义（x<0）回整图；无效句柄返回 0；与 tile 组合=子区域瓦片平铺 |
| 性能验收（评审 §5） | 实测用例 | 大画布 1600×1000 + 16×16 瓦片帧耗时实测，预算 < 2ms；不达标则文档推荐**多单元瓦片**（如 4×4 单元合成一张瓦片，调用数降 1/16），仍不达标时方案 B + raw RGBA 复议 |
| 测试 | test 扩充 | 三后端回归；C ABI 无效句柄拒绝；像素断言不得只测返回值 |

## 6. 影响面与风险

- **I0（SDL3 srcRect 恢复）**：修复既有 CENTER_CROP 潜伏 bug，行为向 SFML/raylib 对齐；风险低但**影响所有用 srcRect 的调用点**（当前仅 Actor CENTER_CROP），像素级回归兜底。
- **需求 1**：仅 binding 四文件，无核心库/ABI 变动，风险最低。
- **需求 2**：Actor 绘制分支 +1，常量 +1，schema +1；不触碰既有 4 分支语义；边缘裁剪为显式实现。
- **需求 3**：Actor 成员/分支 +1，C ABI +1，Binding +1；依赖 I0。
- **需求 4**：不实施，无风险。
- 组合语义：tile/source-rect 与 xScale/yScale（取整规则 §3.2）、match-parent-rect（仅尺寸来源）已明确。
- 三后端一致性：平铺/源矩形统一走 `drawTexture(tex, src, dst)`；I0 修复后三后端语义对齐。

## 7. 评审三问答复（已定案）

1. **Q1 需求 4 不实施**：同意，前提 I0 修复后 tile 完整可用；§5 性能不达标时方案 B + raw RGBA 一并复议。
2. **Q2 source-rect×tile 组合**：确认——先取子区域，再以子区域为瓦片平铺；取整/缩放叠加语义见 §3.2。
3. **Q3 C ABI `ActorSetSourceRect`**：**必补**（Binding 纯 C ABI 动态封装），见 §4.3。

## 8. 放行条件（评审 §6 逐条对应）

- [x] §2.1 → 实施项 I0 纳入（§3.0/§4.2），三后端像素级回归含 CENTER_CROP
- [x] §2.2 → TILE 边缘裁剪显式实现方案（§3.2），验收补边缘像素断言（§5）
- [x] §2.3 → `ActorSetSourceRect` C ABI 必补（§4.3）
- [x] §4 → 三问答复定案（§7），Q2 取整/缩放叠加、match-parent-rect 语义写入（§3.2）
- [x] §5 → 性能验收场景补充（§5）
