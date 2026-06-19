# GraphTool 绘图工具库设计文档

## 1. 概述

GraphTool 是一个后端无关的 2D 绘图工具库，提供完整的颜色系统、画笔/画刷抽象、基本图形绘制、渐变填充、高级绘图效果等功能。该库通过 `RenderDevice` 抽象接口封装底层渲染 API，提供面向对象、类型安全的绘图接口。

### 1.1 设计目标

- **抽象层次**：在 `RenderDevice` 抽象接口之上提供高级绘图抽象，屏蔽底层细节
- **类型安全**：使用强类型枚举和类，避免原始整数和魔法值
- **易用性**：提供直观的 API，支持链式调用和工厂方法
- **可扩展性**：支持自定义画笔样式、虚线模式、渐变等
- **性能**：对高频绘制操作使用 inline，对复杂算法使用 cpp 分离编译

### 1.2 架构概览

```
┌─────────────────────────────────────────────────┐
│                 AdvancedDrawing                  │  ← 高级绘图效果
├─────────────────────────────────────────────────┤
│                 DrawingContext                   │  ← 核心绘图上下文
├────────┬────────┬────────┬────────┬─────────────┤
│ SColor │  SPen  │ SBrush │SGradient│   Utils     │  ← 基础组件
├────────┴────────┴────────┴────────┴─────────────┤
│              RenderDevice Interface              │  ← 抽象渲染接口
└─────────────────────────────────────────────────┘
```

### 1.3 后端无关性

所有绘图操作通过 `RenderDevice` 抽象接口分发，后端实现（SDL3、SFML、Raylib）各自提供 `RenderDevice` 的具体实现。GraphTool 本身不依赖任何后端头文件。

## 2. 文件结构

| 文件 | 说明 |
|------|------|
| `include/GraphTool.h` | 头文件，包含类声明、枚举、inline 工具函数 |
| `src/GraphTool.cpp` | 实现文件，包含 DrawingContext 和 AdvancedDrawing 的方法实现 |
| `test/test_graphtool.cpp` | 测试文件，覆盖所有功能模块 |

## 3. 颜色系统

### 3.1 SColor 类

SColor 是独立于 GraphTool 命名空间的颜色类（位于 `include/SColor.h`），GraphTool 通过 `using SColor = ::SColor;` 引入。使用浮点数（0.0-1.0）存储 RGBA 分量，避免整数精度损失。

#### 构造方式

```cpp
SColor c1;                                    // 默认黑色 (0,0,0,1)
SColor c2(1.0f, 0.0f, 0.0f);                 // 浮点 RGBA
SColor c3(255, 0, 0);                         // 整数 RGB (0-255)
SColor c4(0xFF0000FF);                        // uint32_t RGBA8888 格式
```

**RGBA8888 格式说明**（little-endian x86）：

| Byte offset | 0 (LSB) | 1 | 2 | 3 (MSB) |
|------------|---------|---|---|---------|
| Channel | A | B | G | R |

`uint32_t` 值构造方式：`(R<<24) | (G<<16) | (B<<8) | A`
例：`0xFF0000FF` = R=255,G=0,B=0,A=255 = 不透明红色
`0x000000FF` = R=0,G=0,B=0,A=255 = 不透明黑色

#### 静态工厂方法

```cpp
SColor::Black()          // 黑色
SColor::White()          // 白色
SColor::Red()            // 红色
SColor::Green()          // 绿色
SColor::Blue()           // 蓝色
SColor::Yellow()         // 黄色
SColor::Cyan()           // 青色
SColor::Magenta()        // 品红
SColor::Gray(0.5f)       // 灰色（可指定亮度）
SColor::Transparent()    // 透明
```

所有工厂方法均支持可选的 `alpha` 参数，如 `SColor::Red(0.5f)` 表示半透明红色。

#### 颜色操作

| 方法 | 说明 |
|------|------|
| `withAlpha(float)` | 返回修改透明度后的新颜色 |
| `brighter(float)` | 返回变亮后的新颜色，factor 控制变亮程度 |
| `darker(float)` | 返回变暗后的新颜色，factor 控制变暗程度 |
| `blend(other, ratio)` | 线性插值混合两个颜色 |

#### SDL 桥接（仅内部后端使用）

SColor 保留了 `toSDLColor()` / `toSDLFColor()` 桥接方法，供 SDL3 后端内部使用。GraphTool 的用户代码无需调用这些方法：

```cpp
SDL_Color sdlColor = color.toSDLColor();     // 转为 SDL_Color（SDK3 内部使用）
SDL_FColor fColor = color.toSDLFColor();     // 转为 SDL_FColor（SDK3 内部使用）
```

## 4. 画笔系统

### 4.1 枚举定义

#### PenStyle 画笔样式

```cpp
enum class PenStyle {
    Solid,       // 实线 ────────
    Dash,        // 虚线 ── ── ──
    Dot,         // 点线 · · · · · ·
    DashDot,     // 点划线 ── · ── ·
    DashDotDot,  // 双点划线 ── · · ── · ·
    Custom       // 自定义虚线样式
};
```

#### LineCap 线帽样式

```cpp
enum class LineCap {
    Flat,    // 平头 - 线条在端点处平截
    Round,   // 圆头 - 线条在端点处添加半圆
    Square   // 方头 - 线条在端点处添加半方形
};
```

#### LineJoin 线连接样式

```cpp
enum class LineJoin {
    Miter,   // 尖角 - 外边缘延伸相交
    Round,   // 圆角 - 在连接处添加圆弧
    Bevel    // 斜角 - 截断外边缘
};
```

### 4.2 SPen 类

SPen 封装了画笔的所有属性：颜色、宽度、样式、线帽、线连接、虚线偏移和自定义虚线模式。

#### 构造方式

```cpp
SPen pen1;                                    // 默认：黑色实线，宽度1
SPen pen2(SColor::Red());                     // 红色实线
SPen pen3(SColor::Blue(), 3.0f);              // 蓝色3像素宽
SPen pen4(SColor::Green(), 2.0f, PenStyle::Dash);  // 绿色虚线
SPen pen5(SColor::Red(), 2.0f, PenStyle::Solid, LineCap::Round, LineJoin::Round);
```

#### 工厂方法

```cpp
SPen::NoPen()              // 透明画笔（宽度0）
SPen::BlackPen(2.0f)       // 黑色画笔
SPen::DashPen(SColor::Red())  // 红色虚线画笔
SPen::DotPen(SColor::Blue())  // 蓝色点线画笔
```

#### 虚线模式

标准虚线模式根据线宽自动缩放：

| 样式 | 模式（×线宽） |
|------|---------------|
| Dash | `[4, 2]` |
| Dot | `[1, 2]` |
| DashDot | `[4, 2, 1, 2]` |
| DashDotDot | `[4, 2, 1, 2, 1, 2]` |
| Custom | 用户自定义 |

```cpp
SPen customPen(SColor::Cyan(), 2.0f, PenStyle::Custom);
customPen.setDashPattern({10.0f, 5.0f, 2.0f, 5.0f});
customPen.setDashOffset(3.0f);  // 虚线起始偏移
```

## 5. 画刷系统

### 5.1 BrushStyle 枚举

```cpp
enum class BrushStyle {
    None,            // 无填充（透明）
    Solid,           // 实心填充
    LinearGradient,  // 线性渐变
    RadialGradient,  // 径向渐变
    ConicGradient    // 锥形渐变
};
```

### 5.2 SBrush 类

#### 构造方式

```cpp
SBrush brush1;                                    // 默认黑色实心
SBrush brush2(SColor::Red());                     // 红色实心
SBrush noBrush = SBrush::NoBrush();               // 透明画刷

// 线性渐变画刷
SLinearGradient grad(0, 0, 200, 0);
grad.addStop(0.0f, SColor::Red());
grad.addStop(1.0f, SColor::Blue());
SBrush linearBrush(grad);

// 径向渐变画刷
SRadialGradient rGrad(100, 100, 80);
rGrad.addStop(0.0f, SColor::White());
rGrad.addStop(1.0f, SColor::Blue());
SBrush radialBrush(rGrad);
```

#### 工厂方法

```cpp
SBrush::NoBrush()      // 透明画刷
SBrush::BlackBrush()   // 黑色画刷
SBrush::WhiteBrush()   // 白色画刷
```

#### 位置相关颜色

```cpp
SColor color = brush.getColorAt(x, y);  // 根据位置获取渐变颜色
```

## 6. 渐变系统

### 6.1 SGradientStop 停止点

```cpp
struct SGradientStop {
    float position;   // 0.0 - 1.0 之间的位置
    SColor color;     // 该位置的颜色
};
```

### 6.2 SGradient 基类

```cpp
SGradient grad;
grad.addStop(0.0f, SColor::Red());
grad.addStop(0.5f, SColor::Yellow());
grad.addStop(1.0f, SColor::Green());

SColor mid = grad.getColorAt(0.5f);  // 颜色插值
size_t count = grad.stopCount();      // 停止点数量
```

### 6.3 SLinearGradient 线性渐变

```cpp
SLinearGradient(float x1, float y1, float x2, float y2);
SLinearGradient(const SPoint& start, const SPoint& end);

// 计算某点在渐变方向上的投影比例
float ratio = gradient.getProjectionRatio(x, y);
```

### 6.4 SRadialGradient 径向渐变

```cpp
SRadialGradient(float cx, float cy, float radius);
SRadialGradient(float cx, float cy, float radius, float fx, float fy);  // 带焦点

// 计算某点在径向渐变中的比例
float ratio = gradient.getRatio(x, y);
```

## 7. 辅助结构

### 7.1 SRoundedCorners 圆角半径

```cpp
SRoundedCorners(float allCorners);              // 四角相同
SRoundedCorners(float tl, float tr, float br, float bl);  // 分别指定
```

### 7.2 SLineRectPoints 线段矩形点

用于粗线绘制，存储线段扩展为矩形后的四个顶点：

```cpp
struct SLineRectPoints {
    SPoint startLeft;    // 起点左侧点
    SPoint startRight;   // 起点右侧点
    SPoint endLeft;      // 终点左侧点
    SPoint endRight;     // 终点右侧点

    bool isValid() const;
    vector<SPoint> toVector() const;
};
```

### 7.3 TextAlignment 文本对齐

```cpp
enum class TextAlignment {
    Left,    // 左对齐
    Center,  // 居中
    Right    // 右对齐
};
```

### 7.4 CornerStyle 拐角样式

```cpp
enum class CornerStyle {
    Hard,    // 硬角（直接连接）
    Round    // 圆弧角（使用圆弧连接）
};
```

## 8. Utils 工具函数

Utils 命名空间提供几何计算和图形生成的工具函数，全部为 inline 实现。

### 8.1 颜色工具

| 函数 | 说明 |
|------|------|
| `interpolateColor(c1, c2, t)` | 两个颜色线性插值 |
| `multiplyColor(color, factor)` | 颜色分量乘以因子（保持 alpha） |

### 8.2 几何工具

| 函数 | 说明 |
|------|------|
| `distance(p1, p2)` | 两点间距离 |
| `interpolate(p1, p2, t)` | 两点间线性插值 |
| `pointInPolygon(point, polygon)` | 点是否在多边形内（射线法） |
| `boundingBox(points)` | 计算点集的包围盒 |

### 8.3 图形生成

| 函数 | 说明 |
|------|------|
| `generateCirclePoints(center, radius, segments)` | 生成圆的顶点 |
| `generateEllipsePoints(center, rx, ry, segments)` | 生成椭圆的顶点 |
| `generateArcPoints(center, radius, start, end, segments)` | 生成弧线的顶点 |
| `generateLineRectPoints(start, end, lineWidth)` | 根据线段和线宽生成矩形四顶点 |
| `generateRoundedRectPoints(rect, corners, segments)` | 生成圆角矩形的顶点 |

## 9. DrawingContext 绘图上下文

DrawingContext 是核心绘图类，封装了 `RenderDevice*`，提供完整的 2D 绘图功能。

### 9.1 构造与基本设置

```cpp
DrawingContext dc(renderDevice);   // 传入 RenderDevice*，非 SDL_Renderer*

// 画笔/画刷设置
dc.setPen(SPen(SColor::Red(), 2.0f));
dc.setBrush(SBrush(SColor::Blue()));
dc.setFont("Arial", 14.0f);

// 向后兼容接口
dc.setPenColor(SColor::Black());
dc.setPenWidth(2.0f);
dc.setFillColor(SColor::White());
```

### 9.2 基本图形绘制

| 方法 | 说明 |
|------|------|
| `drawPoint(x, y)` | 绘制点 |
| `drawPoint(x, y, color)` | 绘制指定颜色的点 |
| `drawLine(x1, y1, x2, y2)` | 绘制线段（支持虚线） |
| `drawLine(points)` | 绘制粗线（使用 SLineRectPoints） |
| `drawRect(rect, filled)` | 绘制矩形 |
| `drawRoundedRect(rect, radius, filled)` | 绘制圆角矩形 |
| `drawRoundedRect(rect, corners, filled)` | 绘制不同圆角的矩形 |
| `drawRoundedRectWithBorder(...)` | 绘制带边框的圆角矩形 |

### 9.3 圆形绘制

| 方法 | 说明 |
|------|------|
| `drawCircle(center, radius, filled)` | 绘制圆 |
| `drawCircle(center, radius, fill, stroke, width)` | 绘制带描边的填充圆 |
| `drawFilledCircleWithStroke(cx, cy, R, thickness, fill, stroke)` | 抗锯齿填充圆+描边 |
| `drawFilledCircleWithStroke(cx, cy, R)` | 使用当前画笔/画刷 |
| `drawFilledCircleAA(cx, cy, radius, color)` | 抗锯齿填充圆（仅填充） |
| `drawCircleWithThickness(cx, cy, radius, thickness, color)` | 带线宽的圆环 |

### 9.4 椭圆与弧线

| 方法 | 说明 |
|------|------|
| `drawEllipse(center, rx, ry, filled)` | 绘制椭圆 |
| `drawArc(center, radius, startAngle, endAngle, filled)` | 绘制弧线/扇形 |

### 9.5 多边形与折线

| 方法 | 说明 |
|------|------|
| `drawPolygon(points, filled, debugCorner, debugColor)` | 绘制多边形 |
| `drawPolyline(points, debugCorner, debugColor)` | 绘制折线 |

### 9.6 文本与图像

| 方法 | 说明 |
|------|------|
| `drawText(position, text)` | 在指定位置绘制文本 |
| `drawText(bounds, text, alignment)` | 在矩形区域内绘制对齐文本 |
| `drawImage(position, texture)` | 绘制图像 |
| `drawImage(destRect, texture)` | 缩放绘制图像 |
| `drawImage(destRect, texture, srcRect)` | 裁剪+缩放绘制图像 |

### 9.7 裁剪与变换

| 方法 | 说明 |
|------|------|
| `pushClipRect(rect)` | 压入裁剪矩形 |
| `popClipRect()` | 弹出裁剪矩形 |
| `pushTransform()` | 压入变换状态 |
| `popTransform()` | 弹出变换状态 |
| `scale(sx, sy)` | 缩放 |

### 9.8 粗线绘制原理

当线宽 > 1.0f 时，线段绘制采用以下策略：

1. 使用 `Utils::generateLineRectPoints()` 将线段扩展为矩形（4个顶点）
2. 使用 `getRenderDevice()->drawTriangle()` 渲染为两个三角形组成的矩形

对于粗边框矩形和圆角矩形，采用内外两个轮廓之间的区域填充策略：

1. 计算外轮廓（向外扩展半线宽）和内轮廓（向内收缩半线宽）
2. 如果内轮廓尺寸为负，则整体填充
3. 否则在内外轮廓之间逐段填充

### 9.9 抗锯齿圆形绘制原理

`drawFilledCircleWithStroke` 使用逐像素覆盖度计算实现抗锯齿：

1. 遍历圆形包围盒内的每个像素
2. 计算像素中心到圆心的距离平方 D²
3. 使用 `coverageD2()` 函数计算覆盖度（0.0-1.0）
4. 分别计算填充区域和描边区域的覆盖度
5. 使用 Over 合成运算合并填充和描边颜色
6. 调用 `drawPoint()` 绘制合成后的颜色

```
coverageD2(D², R):
  inner = R - 0.5, outer = R + 0.5
  if D² ≤ inner² → coverage = 1.0
  if D² ≥ outer² → coverage = 0.0
  else → coverage = (outer² - D²) / (outer² - inner²)
```

## 10. AdvancedDrawing 高级绘图

AdvancedDrawing 提供静态方法实现高级绘图效果。

### 10.1 渐变绘制

| 方法 | 说明 |
|------|------|
| `drawGradientRectWithBrush(ctx, rect, gradient, stroke)` | 使用 SLinearGradient 画刷绘制渐变矩形 |
| `drawGradientRect(ctx, rect, startColor, endColor, horizontal)` | 简化渐变矩形 |
| `drawRadialGradientRect(ctx, rect, gradient)` | 径向渐变矩形 |

### 10.2 效果绘制

| 方法 | 说明 |
|------|------|
| `drawShadow(ctx, rect, shadowSize, shadowColor, blurRadius)` | 绘制阴影 |
| `drawGlow(ctx, drawFunc, glowRadius, glowColor, layers)` | 绘制发光效果 |

### 10.3 样式绘制

| 方法 | 说明 |
|------|------|
| `drawStyledRect(ctx, rect, pen, brush)` | 使用 SPen/SBrush 绘制矩形 |
| `drawStyledRoundedRect(ctx, rect, radius, pen, brush)` | 使用 SPen/SBrush 绘制圆角矩形 |
| `drawStyledCircle(ctx, center, radius, pen, brush)` | 使用 SPen/SBrush 绘制圆 |
| `drawStyledEllipse(ctx, center, rx, ry, pen, brush)` | 使用 SPen/SBrush 绘制椭圆 |

样式绘制方法会保存/恢复当前的画笔和画刷状态，确保不影响后续绘制。

## 11. 测试覆盖

test_graphtool.cpp 包含 12 个测试场景，覆盖所有功能模块：

| 编号 | 测试场景 | 覆盖内容 |
|------|----------|----------|
| 1 | SColor 颜色系统 | 构造、工厂方法、混合、变亮/变暗 |
| 2 | SPen 画笔系统 | 构造、样式、虚线模式、工厂方法 |
| 3 | SBrush 画刷系统 | 实心、渐变、透明画刷 |
| 4 | 渐变系统 | 线性渐变、径向渐变、颜色插值 |
| 5 | 基本图形绘制 | 点、线、矩形（填充/描边/粗边框） |
| 6 | 圆角矩形 | 填充、描边、粗边框、不同圆角 |
| 7 | 圆形绘制 | 填充、描边、AA填充、带线宽圆环 |
| 8 | 椭圆与弧线 | 椭圆、弧线、扇形 |
| 9 | 多边形与折线 | 三角形、五边形、折线 |
| 10 | 虚线绘制 | 所有 PenStyle 样式 |
| 11 | AdvancedDrawing | 渐变矩形、阴影、发光、styled 绘制 |
| 12 | Utils 工具函数 | generateLineRectPoints、generateRoundedRectPoints |

## 12. 使用示例

### 12.1 基本绘图

```cpp
DrawingContext dc(renderDevice);

// 绘制红色填充矩形
dc.setFillColor(SColor::Red());
dc.drawRect(SRect(10, 10, 100, 50), true);

// 绘制蓝色粗边框
dc.setPen(SPen(SColor::Blue(), 3.0f));
dc.drawRect(SRect(10, 70, 100, 50), false);

// 绘制圆角矩形
dc.setFillColor(SColor(0.9f, 0.9f, 1.0f));
dc.drawRoundedRect(SRect(10, 130, 100, 50), 10.0f, true);
```

### 12.2 渐变绘制

```cpp
// 水平渐变
AdvancedDrawing::drawGradientRect(dc, SRect(10, 10, 200, 50),
    SColor::Red(), SColor::Blue(), true);

// 使用 SLinearGradient
SLinearGradient grad(0, 0, 200, 0);
grad.addStop(0.0f, SColor::Red());
grad.addStop(0.5f, SColor::Yellow());
grad.addStop(1.0f, SColor::Green());
AdvancedDrawing::drawGradientRectWithBrush(dc, SRect(10, 70, 200, 50), grad);
```

### 12.3 样式绘制

```cpp
SPen pen(SColor::Blue(), 2.0f);
SBrush brush(SColor(0.8f, 0.8f, 1.0f));

AdvancedDrawing::drawStyledRoundedRect(dc, SRect(10, 10, 100, 50), 10.0f, pen, brush);
AdvancedDrawing::drawStyledCircle(dc, SPoint(60, 100), 30.0f, pen, brush);
```

### 12.4 虚线绘制

```cpp
dc.setPen(SPen(SColor::Red(), 2.0f, PenStyle::Dash));
dc.drawLine(10.0f, 10.0f, 200.0f, 10.0f);

// 自定义虚线
SPen customPen(SColor::Blue(), 2.0f, PenStyle::Custom);
customPen.setDashPattern({10.0f, 5.0f, 2.0f, 5.0f});
dc.setPen(customPen);
dc.drawLine(10.0f, 30.0f, 200.0f, 30.0f);
```

### 12.5 阴影与发光

```cpp
// 绘制阴影
AdvancedDrawing::drawShadow(dc, SRect(50, 50, 100, 60),
    5.0f, SColor(0, 0, 0, 0.3f), 3.0f);

// 绘制发光效果
AdvancedDrawing::drawGlow(dc,
    [&dc]() {
        dc.setFillColor(SColor::Red());
        dc.drawCircle(SPoint(200, 80), 20.0f, true);
    },
    10.0f, SColor(1.0f, 0.5f, 0.0f, 0.5f), 5);
```

## 13. 注意事项

1. **坐标系统**：所有坐标使用浮点数，原点在左上角
2. **颜色范围**：SColor 内部使用 0.0-1.0 浮点数，自动 clamp
3. **线宽为0**：画笔宽度为0时不绘制任何内容
4. **虚线偏移**：`setDashOffset()` 控制虚线起始偏移，可用于动画
5. **状态保存**：AdvancedDrawing 的 styled 方法会自动保存/恢复画笔画刷状态
6. **裁剪栈**：`pushClipRect`/`popClipRect` 支持嵌套裁剪
7. **后端无关**：所有渲染通过 `RenderDevice` 接口分发，不依赖任何特定后端
8. **编码警告**：源文件包含中文注释，MSVC 可能报 C4819 警告，不影响功能
