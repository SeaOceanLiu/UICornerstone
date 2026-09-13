# UICornerstone C++ Binding 能力需求（画布网格渲染驱动）

> 提出方：CornerstoneDesigner（画布/设计器）
> 日期：2026-09-04
> 背景：设计器中央画布需要高性能网格 + 覆盖层（选中框 / 8 向控点 / 对齐参考线）。
> 当前 C++ Binding 缺少下述接口，导致画布网格在 resize/splitter 时被迫"重建整张内容区
> 大纹理"（无压缩 PNG 生成 + 大内存拷贝 + 引擎解码 ≈ 每帧数百 ms），且覆盖层只能退化为
> 临时 Image 控件。特提出以下需求。

## 1. Shape 多图元（组合图形）完整封装 —— 覆盖层绘制

**现状**：C ABI 已实现，但 Binding 未暴露：
- `ShapeAddPrimitive(inst, sh, type, x, y, w, h)` → 图元索引
- `ShapeSetPrimitiveColor(inst, sh, idx, prop, color)`（prop = "fill"/"stroke"）
- `ShapeSetPrimitiveFloat(inst, sh, idx, prop, value)`（prop = "line-width"/"radius"/"ring-width"）
- `ShapeSetPrimitivePoints(inst, sh, idx, count, xs, ys)`（polyline/polygon 点集，本地像素）
- `ShapeClearPrimitives(inst, sh)`

**需求**：C++ Binding 增加一一对应方法：
```cpp
int  ShapeAddPrimitive(Control& sh, const std::string& type, float x, float y, float w, float h);
void ShapeClearPrimitives(Control& sh);
void ShapeSetPrimitiveColor(Control& sh, int idx, const char* prop, UIColor value);
void ShapeSetPrimitiveFloat(Control& sh, int idx, const char* prop, float value);
void ShapeSetPrimitivePoints(Control& sh, int idx, const std::vector<std::pair<float,float>>& pts);
```

**用途**：选中框蓝色描边（`kShapeRect` 空心 + stroke #4A90D9）、8 方向控点
（`kShapeFilledRect`）、对齐参考线（polyline #FF8800，80% 透明）、框选橡皮筋。
这些是**动态、少量、位置实时变化**的矢量线，用 Shape 图元每帧改点集即可，
无需每帧建/毁控件。

## 2. 纹理平铺（Tile / Repeat）—— 网格铺满零重建

**现状**：`Image`/`Actor` 仅支持把整张纹理铺到目标 rect（`scaleType`: stretch /
fit-center / center-crop / none），无平铺语义。

**需求**：`Image` 增加平铺能力（任选一种形态）：
- `scaleType: "tile"`：把纹理按原像素尺寸重复平铺填满目标 rect；或
- 独立键 `tile: true` + `sourceRect`（见需求 3），或 `image-repeat`, `wrap` 风格。

**用途**：网格是**规则重复图案**——每格 `gridCell×z` 物理像素一幅单元瓦片。
若能平铺，画布铺满一张"单元网格瓦片"即可，**resize/splitter 只改目标 rect，
纹理零重建、零密度变化**，性能恒为常数。

## 3. 图片源矩形 / 显示子区域（Source Rect / UV）

**需求**：`Image`/`Actor` 支持从纹理取子区域贴到目标 rect：
- 属性：`source-rect`（`{x,y,w,h}` 或 `srcX/srcY/srcW/srcH`），或
- C++ Binding：`ImageSetSourceRect(Control&, float x, float y, float w, float h)`。

**用途**：配合一张"足够大的网格纹理"，resize 时只移动显示窗口（源矩形），
纹理永不重建；也用于整张贴图的区域裁剪。

## 4. 原始像素纹理（Raw RGBA 内存纹理）—— 免 PNG 编解码

**现状**：`RegisterResource` / `AdoptResource` 注册的是"图片文件字节流"，
引擎按 PNG/JPEG 解码；`CreateImage` 只能传路径，无法直接给 RGBA 缓冲。
画布网格因此必须先逐像素生成大 PNG（stored 无压缩 ≈ 数 MB）再解码。

**需求**：增加原始像素纹理入口（任选）：
- `RegisterTexture(name, width, height, const uint8_t* rgba8888, pitch=width*4)`，
  创建 `provider:name` 引用的**直接 RGBA 纹理**（1:1 像素，无重采样）；或
- `AdoptTexture(name, w, h, rgba, pitch, freeFn)` 零拷贝版。

**用途**：程序化纹理（网格、噪点、检测图、覆盖层底色）一次性上传为纹理，
之后只改源矩形/目标矩形，**完全避免每帧 PNG 编解码 + 大内存拷贝**。

## 5. 选型组合建议

对"画布网格铺满 + resize/splitter 密度恒定 + 性能恒常数"：

| 方案 | 依赖需求 | 说明 |
|------|---------|------|
| A（推荐，最省） | 2 或 3 | 单元网格瓦片平铺 / 大网格纹理 + 源矩形移动；resize 零重建 |
| B | 4 | 大网格纹理一次性构造为 raw 纹理，源矩形移动 resize 窗口 |
| C | 1 | 覆盖层（选中框/控点/对齐线）全部矢量图元化，实时改点集 |

三者叠加后画布区不再有任何"按 resize 重建的大纹理"，性能问题根治。