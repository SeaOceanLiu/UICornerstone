# CR20261112001 — 通用 box-shadow rect 柔光阴影（分析 + 设计）

> 状态：**closed（2026-11-12 实施；2026-11-13 设计器验收；2026-11-17 最终视觉验证通过）**
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261112001）
> 评审：`Temp/CR20261112001_SoftShadow_Design_评审意见.md`（有条件通过，已逐项闭合）
> 关联：CR20261102001（通用 box-shadow/主题）、Menu/MenuPanel、Label text-shadow、Shape V1 bbox

## 1. 现状核实

- `ControlImpl` 已保存 `m_boxShadowColor/OffsetX/OffsetY/Radius/Spread`，JSON/主题 `box-shadow` 与拆散键均已接通。
- `ControlImpl::drawBackground()` 当前只画**一层**扩展圆角矩形；`blur` 仅改变圆角扩展，**没有真实 alpha 柔化**，边缘是硬边。
- `drawBackground()` 的阴影仅在该控件 `!getTransparent()` 内执行；Label、Shape、默认 ListView 等透明控件当前不会出现 box-shadow。
- `MenuPanel::drawShadow()` 是独立固定黑色近似，只有 `shadow-radius` 可配；`shadow-radius=0` 是现有“关闭投影”语义。
- Label 的 `text-shadow`/`shadow-offset` 是**字形按偏移重绘一次**的硬阴影，没有柔化。
- Shape 没有轮廓阴影入口；通用 box-shadow 目前只能按包围盒绘制。
- `MenuBar`、`MenuPanel`、`MenuItem`、`HandleControl` 为自定义 draw，不自动经过 `beforeDraw()`。

## 2. 目标与范围

- 通用控件、MenuPanel/ContextMenu、MenuBar：rect 外围柔光阴影；
- 阴影在背景外侧、不覆盖内容、不影响布局/命中；
- 预烘焙纹理 + 共享缓存 + 9-slice，禁止每帧高斯模糊；
- Label：字形轮廓柔光，与 rect box-shadow 分开；
- Shape：V1 只支持包围盒 rect；轮廓发光 V2 独立 opt-in；
- 不新增 CABI 函数/绑定入口；属性/主题/schema 增量。

## 3. 关键决策（Q1~Q5）

### Q1：文字柔化字段

采用独立 `text-shadow-blur`（Float，默认 0）：
- `blur=0` 保持现有硬偏移字形阴影；
- `blur>0` 使用字形 alpha mask 柔化；
- 不增加 `text-shadow-spread`（V1 不做）；
- 仅 Label（含 Label 派生的文本展示）接入；其他控件 caption 后续另立需求。

### Q2：模糊核与范围（C1）

- `blur`：0～64 逻辑像素；`0` 必须走**硬边路径**，不进入任何 blur；`>64` 钳制并诊断一次；
- `spread`：仅支持 `>=0`，负值钳制 0 并诊断；schema `minimum: 0`；
- **主路径采用可分离 Gaussian**：`sigma = blur / 2`，support = `ceil(3 * sigma)`；预烘焙一次；
- 若实现改用三次 box blur 优化，必须使用正确半径：
  `r = (blur == 0) ? 0 : max(1, round((sqrt(1 + 4*sigma*sigma) - 1) / 2))`；
  support = `3*r`；不得使用旧稿的 `sqrt(3*sigma^2+0.25)`（会过模糊）；
- 文档给出量化表：`blur=0/1/8/16/64` → `sigma=0/0.5/4/8/32`、support=`0/2/12/24/96`；
- `offset-x/y` 在绘制阶段应用，不烘入纹理，避免平移触发重新烘焙；
- 阴影纹理边缘预留 support + 1px gutter，尾迹不得被裁断。

### Q3：缓存、纹理、Alpha 与 9-slice

**Alpha/混合契约（B1）：采用 straight alpha。**

- Surface 为 RGBA8888，按项目既有 little-endian 内存序 `A,B,G,R`；像素写入 `(R<<24)|(G<<16)|(B<<8)|A`；
- 纹理 RGB = 阴影颜色 RGB（颜色烘入纹理），A = 阴影蒙版 coverage × color alpha；
- 使用普通 `BlendMode::Blend` 绘制；**不预乘 RGB**；不引入 premultiplied backend blend mode；
- 颜色/alpha 进入缓存键；不做绘制期 tint；
- 跨三后端 + callback 后端补“深色/浅色背景无暗边、白边”像素验收。

**缓存/纹理：**

- **ShadowCache 生命周期**：挂在 `UIContext`（RenderDevice 上下文），不在全局 static；实例销毁/RenderTarget 重建时清空。
- **纹理**：RGBA8888 straight alpha；CPU Surface 烘焙优先（跨 callback backend 可用）；native RenderTarget 仅作可选加速，不作为唯一路径。
- **缓存键**：
  `cornerRadius + blur + spread + colorRGBA + DPI/effectiveScale + renderer identity + texture format + shadowVersion`；
  offset 不进入键；精确尺寸 fallback 时额外带 width/height。
- **单位（R2）**：所有 `blur/spread/cornerRadius/offset` 为逻辑像素；烘焙时乘 effectiveScale（DPI/画布缩放）转设备像素；cache key 含 scale；9-slice inset 以设备像素存储、绘制时按设备像素切块。
- **9-slice 与单位顺序（B3/C3）**：
  - 先全部在**逻辑域**计算：`sigma=blur/2`、`support=ceil(3*sigma)`、`cornerRadius`、`spread`；
  - 再统一乘 `effectiveScale` 转设备域：
    `blurPx=blur*scale`、`spreadPx=spread*scale`、`cornerPx=cornerRadius*scale`、`supportPx=ceil(support*scale)`；**不得对已是设备像素的 support 再乘 scale**；
  - `gutter=1 device px`；`insetPx = ceil(cornerPx + spreadPx + supportPx + gutter)`；
  - patch 尺寸 `2*insetPx + 1`；四角固定、边/中心拉伸；纹理外缘保留 1px 透明 gutter/edge extrusion；
  - 9 个目标矩形**不重叠**；源矩形按整数纹素边界取样；双线性过滤下不得出现接缝；源边界不重复 alpha；
  - 控件设备宽/高 `< 2*cornerPx`、`< 2*insetPx+1`，或 patch 超过后端纹理上限时，回退**精确尺寸缓存**（key 加设备宽高）；
  - 胶囊形/极小控件同样走精确回退；最大 blur/DPI 非整数缩放纳入测试；
  - 角部亮度连续性/边缘 alpha 尾迹作为像素验收，不只测“能画出”。

- **共享**：同 key 返回同一 SharedTexture（引用计数 + LRU）；跨实例按 RenderDevice 隔离。
- **上限（R6）**：默认 32 MB/实例，rect 阴影与 Label 文字阴影**共享同一预算**；LRU 淘汰只移除 cache 持有引用，不销毁仍被控件持有的纹理。
- **失效（R5）**：尺寸（精确 fallback）、corner-radius、blur、spread、颜色/alpha、主题/class/变体、DPI、renderer 重建、Texture 丢失、菜单参数变化；`ApplyTheme`/变体/class 变化递增全局 shadowVersion，不能只依赖控件局部 diff。
- **回退**：纹理创建失败/超限时使用最多 4 层程序化柔光并诊断一次，不得崩溃；不回到每帧高斯模糊。

### Q4：MenuPanel/MenuBar 复用、圆角与优先级（B2/C2）

- MenuPanel 不再保留独立每帧阴影循环；使用统一 `ShadowSpec` + ShadowCache。
- **圆角**：`ShadowSpec.cornerRadius` 必须使用面板实际有效圆角（当前 `MenuColors::PANEL_RADIUS = 5`；若未来可配置则使用配置值），不得默认 0，否则 9-slice 角部错误。
- `shadow-radius <= 0` **始终显式关闭**投影（即使颜色 alpha>0，也即使同时存在通用 `box-shadow`），保持 P0-77。
- 归一化优先级：
  1. 默认：color=`PANEL_SHADOW`（`#00000066`）、offset=(0,0)、blur=8、spread=0、corner=5；
  2. legacy `shadow-radius`/`shadow-offset`/`shadow-color`/`shadow-spread` 显式设置时覆盖默认；
  3. 通用 `box-shadow` 任意字段**显式设置**时，与 legacy **互斥**：完全走通用字段（缺失字段取默认），不再叠加/回退 legacy；
  4. 仅 legacy 路径下，`shadow-radius` 显式为 0 才关闭（B2）；通用 `box-shadow` 存在时 legacy 0 不再额外禁用。
  MenuBar 的默认 radius=8 不视为“显式”，需用 `legacyShadowExplicit` 标志区分，避免默认覆盖用户 `box-shadow`。
- 新增 `shadow-color`、`shadow-spread` 必须同步 schema/属性表/主题文档；不能只做内部映射。
- **默认视觉基线（C2）**：不再承诺“blur=8 与旧菜单像素一致”——旧实现是实心扩展矩形，新实现是真柔光。验收改为：
  - 默认菜单仍清晰可辨、无阴影裁断、无发黑/发白边；
  - 0 档关闭、8 档为可接受柔光、32 档扩展不裁断；
  - 旧/新默认菜单截图纳入验收记录，发布说明标注视觉升级；
  - 若未来要求像素级旧观感，另加兼容开关/旧路径，不在 V1。
- MenuBar `applyStateToPanel` 传播 radius、颜色、扩散，并保留显式标志。
- ContextMenu 复用 MenuPanel，自动获得同样缓存与柔光。
### Q5：Shape V2

- V1：Shape 的 `box-shadow` 严格按 `getDrawRect()` 包围盒绘制；不尝试点集/轮廓；
- V2：独立键建议 `shape-glow`（enabled/color/width/blur/spread），默认关闭；轮廓 mask、点集变化、缓存和性能另立 CR；
- V1 遇到 V2 字段忽略/诊断，不改变外观。

## 4. 架构设计

### 4.1 ShadowSpec 与属性

内部结构（不放公共 API）：

```cpp
struct ShadowSpec {
    bool enabled = false;       // 显式启用（Menu 由 radius>0 映射）
    SColor color{0,0,0,0};
    float offsetX = 0, offsetY = 0;
    float blur = 0, spread = 0;
    float cornerRadius = 0;
};
```

- 从现有字段归一化；`box-shadow` 对象优先级高于拆散键（沿用 CR20261102001）。
- `box-shadow-color` alpha=0 且非 Menu 时关闭；`blur=0/spread=0` 仍可画硬边（显式 enabled）。
- `text-shadow-blur` 独立字段加到 Label/PropertyNames/schema；不混入 box-shadow。
- R1：删除设计初稿中冗余条件，`enabled` 由显式声明/菜单 radius 决定。

### 4.2 绘制顺序与透明控件

`ControlImpl::drawBackground()`：

1. `drawSoftRectShadow(drawRect, spec)`：**在透明判断之外**执行；透明控件也能有 rect 外阴影；
2. 非透明时绘制背景；
3. 边框/内容由原路径继续；
4. 阴影不改变 `getDrawRect()`、布局或命中；父容器 clip 仍有效。

- 默认透明控件**不自动产生阴影**；只有显式 `box-shadow` 才绘制（避免 Label/ListView/Shape 误染）。
- Label：rect shadow（若有）→ 字形 text-shadow → 正文；两者可同时存在。
- MenuPanel：先统一 shadow，再背景/边框/items；子菜单复用 cache；阴影超出面板区域不得被菜单自身 clip。
- **MenuBar（B5）纳入 V1**：`MenuBar::draw()` 自定义路径开头显式调用统一 shadow；测试 bar 与下拉面板叠放。
- **MenuItem 不纳入 V1**（内部菜单项，逐项 shadow 不在本 CR）；文档明确。
- **HandleControl 不纳入 V1**（自绘手柄层，避免手柄/拖拽语义回归）；文档明确。
- ComboBox/NumericUpDown/TreeView 通过既有 draw/beforeDraw 或 EditBox 路径纳入；回归确认。

### 4.3 预烘焙算法

1. 计算逻辑 `inset`，按 scale 转设备像素，生成 alpha 蒙版（圆角矩形 + spread）；
2. 主路径做可分离 Gaussian（sigma=blur/2，support=ceil(3*sigma)）；若用三次 box blur，按 C1 正确半径；blur=0 跳过 blur；纹理外缘留 1px gutter；
3. 以 **straight alpha** 写入 shadow RGB + coverage*alpha（不预乘）；
4. `Surface::createTexture(renderDevice)`；失败则程序化 fallback；
5. 9-slice cache 存纹理 + 设备像素 inset；绘制时 `offset` 只移动目标位置；
6. 精确 fallback 以完整控件尺寸 + 留白烘焙，key 加宽高。

所有步骤只在 cache miss/参数变化时执行；每帧只做纹理绘制。

### 4.4 Label 文字柔光（B4）

- **粒度**：V1 按**逐行 glyph mask** 烘焙，不整段复制矩形；每行纹理 bbox = 该行可见文字 bounds + `abs(offset) + blurSupport + 1px gutter`。
- **裁切/对齐**：key 包含 text/font/size/style/alignment/lineWidth/ellipsis/clip；只烘焙可见字形；最终绘制裁剪到 Label 内容 clip（V1 不越界；未来 opt-in 另议）。
- **颜色/alpha**：与 rect shadow 相同 straight alpha；颜色烘入，key 含颜色/状态。
- **离屏路径**：native RenderTarget 可用时绘制字形 mask → readback → CPU blur → 纹理；callback 后端不支持 target 时回退**最多 8 层字形偏移绘制**，诊断一次并文档标注质量低于纹理路径。
- `blur=0` 直接走旧硬阴影路径，不建纹理；空文本不生成。
- 文本/字体/对齐/换行/裁切/主题/DPI 任一变化使缓存失效；多行逐行缓存，共享同 key。
- Label 文字阴影计入统一 32MB 预算，不另设无界缓存。

### 4.5 Shape

- V1 直接使用 `getDrawRect()` 的 rect shadow cache；不与点集/轮廓关联；文档/schema 标注“V1 bbox only”。
- V2 `shape-glow` 独立字段/缓存，后续设计。

## 5. 自定义 draw 支持矩阵（B5）

| 控件 | 路径 | V1 box-shadow |
|---|---|---|
| Button/Label/EditBox/ComboBox/NumericUpDown/TextArea/CheckBox/ProgressBar/Slider/ScrollBar/Panel/Bench/TreeView/ListView/TabControl/StatusBar/WinFrame/ColorPicker/Dialog/Popup/ColorPicker | `beforeDraw`/EditBox 链 | ✅ |
| Shape/Actor/Animation | `beforeDraw`（透明/显示控件） | ✅ bbox |
| MenuPanel/ContextMenu | 自定义 draw，接入统一 helper | ✅ |
| MenuBar | 自定义 draw，显式接入 helper | ✅（本 CR） |
| MenuItem | 自定义 item 绘制 | ❌（文档明确） |
| HandleControl | 自定义手柄层 | ❌（文档明确） |

## 6. API / Schema / 兼容

- 新属性：`text-shadow-blur`（Float，Label；0～64）；
- MenuPanel/MenuBar 可选 `shadow-color`、`shadow-spread`（与 `shadow-radius` 并存）；必须同步 schema/属性面板/主题文档；
- `box-shadow.blur/spread` schema `minimum: 0` 且 blur max 64；`text-shadow-blur` 0～64；
- 现有 `box-shadow` 字段、拆散键、主题优先级不变；
- C ABI/Binding 通过通用属性系统读写，不新增函数；
- 旧布局/旧主题未声明新字段时视觉不变；
- **R3/N5**：旧 `box-shadow` 的 `blur>0` 从“硬边扩展”变成真实柔化，属于本次有意的视觉升级；保留旧主题回归基线（`#00000066, offset 0/6, blur 16`）的旧/新截图，在发布说明标注变化；
- **R4**：负 spread 旧值钳制 0 并诊断；schema 限制 `minimum:0`；若历史布局存在负值，发布说明提示；
- **N2**：MenuItem/HandleControl 不支持 box-shadow，schema/属性面板/用户手册明确显示“不支持”，不得静默无效；
- **N3**：`text-shadow-blur` 仅 Label，其他控件 caption 后续另立需求；支持矩阵不得写成“所有文本控件”；
- Shape V2 字段未知时忽略并诊断，不改变 V1 外观。

## 7. 性能、缓存与内存

- 命中缓存时：每个阴影 1 次纹理绘制（9-slice 最多 9 次小纹理）；无 blur；
- 烘焙成本：一次 CPU 蒙版 + Gaussian（或正确三次 box blur，blur=0 跳过）；按 32MB/LRU 控制；
- 9-slice 避免大尺寸控件各自整图；同 key 跨控件共享；
- 动态 resize：9-slice 不重建；精确 fallback 重建，尺寸稳定后合并；
- **R7**：9 次 drawTexture 的状态/批处理成本纳入性能测试；V1 至少不劣于当前单层圆角矩形；
- Label 逐行缓存与 rect shadow 共享预算；fallback 8 层有上限；
- 不引入每帧高斯模糊/每帧 RenderTarget 创建。

## 8. 测试/验收

1. 通用：`blur=0` 硬边；`blur>0` alpha 尾部完整、无暗边/白边（straight alpha 像素验收）；offset/spread/圆角/半透明/重叠/父级 clip；
2. Alpha 契约：SDL3/SFML/raylib/callback 在深/浅背景的暗边/白边像素断言；
3. 9-slice：大尺寸无接缝、角部亮度连续、1px gutter 生效、极小/胶囊/最大 blur/非整数 DPI 回退正确；
4. 缓存：同 key 共享；尺寸/圆角/颜色/blur/spread/主题/DPI 变化失效；LRU 上限；纹理失败 fallback 不崩溃；`shadowVersion` 主题切换；
5. 核/单位：`blur=0` 硬边；`1/8/16/64` 实际尾迹符合 support 表；逻辑/设备换算不重复缩放；
6. Menu：MenuPanel/ContextMenu `shadow-radius=0` 关闭、8/32 视觉基线、MenuPanel 圆角 5；legacy 与通用 box-shadow 同时设置的字段优先级；MenuBar box-shadow 生效且不被裁断；MenuItem/HandleControl 明确不支持；
7. 透明控件：Label/ListView/Shape 默认无阴影；显式 box-shadow 才显示；父级 clip 正确；
8. Label：逐行字形柔光无矩形光晕；多行/对齐/裁切/省略/动态文本/空文本；`blur=0` 旧硬阴影；8 层 fallback 质量边界；DPI/状态；
9. Shape：V1 bbox only；V2 字段不改变外观；
10. 旧布局/主题、C ABI/Binding/schema/docs；`validate_layout --strict`；设计器 auto=2/auto=3/strict；旧/新 shadow 截图基线；
11. 缓存生命周期：LRU 淘汰仍被控件持有的 shared_ptr 不得提前销毁；
12. 性能：100 个带阴影控件滚动/动画，无每帧模糊；9-slice 绘制成本可接受。

## 9. 风险与回退

| 风险 | 处理 |
|---|---|
| straight alpha 暗边 | 明确不预乘；跨后端像素测试 |
| 9-slice 尾部/接缝/角半径 | support+gutter、精确尺寸回退、角部连续性测试 |
| Menu `radius=0` 语义 | 显式 enabled=false；0/8/32 回归 |
| Label 纹理/裁剪 | 逐行 mask、内容 clip、8 层 fallback 质量记录、共享预算 |
| MenuBar/自定义 draw | V1 纳入 MenuBar；MenuItem/HandleControl 明确排除 |
| 旧 blur 视觉变化 | 文档/发布说明标注，旧示例基线回归 |
| 负 spread/DPI | clamp+诊断；scale 进入 key/设备像素 |
| LRU 管理 | shared_ptr 引用计数；32MB 共享预算，不销毁仍持有纹理 |

## 10. 实施顺序

1. ShadowSpec/ShadowCache/straight-alpha 烘焙 helper；
2. `ControlImpl::drawBackground` rect 阴影（透明控件也绘制）；
3. MenuPanel/MenuBar 接入统一缓存；
4. Label `text-shadow-blur` + 逐行 glyph mask/回退；
5. Shape V1 bbox、schema/文档；
6. 像素/性能/回归/Release/设计器联调。

## 修订记录

- 2026-11-12 初稿：确认硬边/无缓存/菜单独立路径；预烘焙+9-slice+共享 LRU；Label 字形柔光；Shape V1 bbox；回答 Q1~Q5；待评审。
- 2026-11-12 v1（按评审 B1~B5/R1~R7）：明确 straight alpha；Menu radius=0 关闭；9-slice support/gutter/精确回退；Label 逐行纹理边界/裁剪/fallback；MenuBar 纳入、MenuItem/HandleControl 排除；补单位、主题版本、共享预算和像素验收。
- 2026-11-12 v2（按复核 C1~C3/N1~N5）：修正 blur 核/hard path；Menu 圆角/优先级/默认视觉基线；逻辑→设备单位顺序/gutter；补不支持声明和 LRU 生命周期测试。

## 11. 实施记录（2026-11-12）

### 11.1 落地文件

- `include/ShadowRenderer.h` / `src/ShadowRenderer.cpp`：`ShadowSpec`、实例级 `ShadowCache`、straight-alpha 预烘焙、Gaussian 柔化、9-slice、32MB LRU、程序化回退。
- `include/UIContext.h`：`shadowCache` 实例字段；`UIContext::destroy()` 先释放纹理再关后端。
- `include/RenderDevice.h`：新增 `supportsTextShadowTexture/readTexturePixels/suspendClipRect/resumeClipRect` 内部虚函数。
- `include/Surface.h`：新增跨后端 `setPixelRGBA`（修正 SFML/raylib Surface 字节序差异）。
- `ControlImpl::drawBackground`：阴影移到透明判断外；透明控件显式 `box-shadow` 生效。
- `MenuPanel/MenuBar`：统一 ShadowCache；`shadow-radius<=0` 保持关闭；legacy 与通用 `box-shadow` 字段级优先级；MenuBar 自定义 draw 显式接入。
- `Label`：`text-shadow-blur`（0~64）+ 逐行字形柔光；`blur=0` 保持旧硬阴影；无纹理能力时 8 层回退。
- `MenuItem/HandleControl`：不支持 `box-shadow`，设置时输出一次诊断。
- Schema/用户手册/主题文档/API 映射同步；新增 `test_soft_shadow` 像素/缓存/菜单/LRU 测试。

### 11.2 实施中的关键修正

1. **9-slice 中央采样区**：初版 patch 中央仅 1px，Gaussian 会把 1×1 形状能量摊到整张纹理，导致柔光几乎不可见。实施改为中央 `centerSpan = 2*support + 2`，边条取中央直边列/行，避免角部污染。
2. **文字柔光不在帧中切 RenderTarget**：帧中切换目标会使已绘制的窗口内容失效（SDL/SFML/raylib 均有风险）。实现改为 `drawTextShadow` 未命中时排队，下一帧 `Update` 阶段集中烘焙；当帧 Label 走有限层回退。
3. **Callback 后端**：`UIBackendCallbacks` 末尾新增可选离屏渲染回调（version>=2）；SDL3/SFML/raylib 插件桥接，旧插件缺失时自动回退。
4. **Surface 写入**：统一改用 `Surface::setPixelRGBA`，SFML/raylib 覆盖为原生 `sf::Color` / `Color`，避免 Surface 内存序差异导致阴影 alpha 通道错误。
5. **SFML RenderTexture 生命周期**：修复 `SFMLTexture` 对 RenderTexture 内部 texture 的重复 delete。
6. **9-slice 角区过渡**：角区向控件内部延伸 `spread + blur support`，边条从过渡区之后的直边列/行取样，消除四角亮度台阶。
7. **方形控件阴影角**：`shapeRadius = corner > 0 ? corner + spread : 0`；避免 `spread` 给直角控件凭空制造圆角（硬阴影角必须与矩形框一致）。
8. **等强度柔光**：阴影 alpha 采用欧氏距离场，并在角部做“边中同距离”近场补偿；四角不再明显弱于边中，同时保留圆润外缘（避免 L∞ 的尖硬角）。
9. **菜单 legacy/通用互斥**：声明任意通用 `box-shadow` 字段后完全使用通用字段（缺失取默认），不再叠加/回退 legacy `shadow-radius`；只有纯 legacy 路径下 `shadow-radius<=0` 才关闭。legacy 默认偏移改为 `(0,0)`，避免菜单下方出现独立矩形阴影。
10. **ContextMenu 外层抑制**：`Panel::draw` 内部是限定调用 `ControlImpl::beforeDraw()`，不会走虚 override；ContextMenu 重写 `draw()`，跳过 Popup 背景/矩形 box-shadow，只绘制内部 MenuPanel，避免“底层矩形”叠加。
11. **菜单边框绘制顺序**：MenuPanel 的边框改到 item hover/显式背景之后绘制，避免 hover 背景覆盖左侧/右侧 1px 边框。

### 11.3 验证

- `test_soft_shadow`：SDL3/SFML/raylib 均 13/13 通过（硬边、柔光尾部、内容不覆盖、菜单 0/8、缓存共享、LRU 回退、跨帧命中）。
- SDL3 全量回归：61 个测试通过，保留 4 项历史失败（`test_menu_cabi` bar-height、`test_multiviewport*`、`test_treeview_cabi` item-font），无新增失败。
- `validate_layout --schema-only` 与 `layouts/*.json --strict` 通过。
- 用户手册截图：`docs/assets/controls/label-shadow-blur.png`（可复现布局 `layouts/label-shadow-blur.json`），并刷新 menu/contextmenu 截图。
- Release DLL SHA256：`267fbe8fe10e793a22af8620bb71096c84b53e99d9fc03cf38065cf57a066a89`；已同步设计器 `subModules/UICornerstone`。

## 12. 最终视觉验证（2026-11-17）

- 设计器重新验收：通用 `box-shadow` 柔光（预烘焙纹理/9-slice 缓存）、Label `text-shadow-blur` 字形柔光、Shape V1 包围盒阴影均符合预期；截图与文档复核通过。
- 引擎回归：`test_soft_shadow` 三后端 13/13；SDL3 全量回归无新增失败。
- 结论：设计器/引擎两项均 closed，满足归档门禁；无代码变更。
