# 后端抽象层设计文档

## 1. 概述

UIControls 当前硬编码绑定 SDL3，所有控件直接在头文件中引用 `SDL_Color`、`SDL_Renderer*`、`SDL_Texture*` 等 SDL3 类型。本文档规划如何抽离出**平台无关的抽象接口层**，使库可以无缝切换后端（SDL3 / SFML / raylib 等）。

### 1.1 设计目标

- **控制层零 SDL 依赖**：所有控件的 `.h` / `.cpp` 不再 `#include <SDL3/...>`
- **后端可插拔**：编译时通过 CMake 选项 `-DBACKEND=SDL3|SFML|RAYLIB` 选择
- **迁移可增量**：不必一次性全部替换，可定义接口 + 保留旧实现并行
- **零性能损失**：抽象层为纯虚基类 + 内联转换，不引入虚函数调用开销在渲染热路径上

### 1.2 总体架构

```
┌─────────────────────────────────────────────────┐
│  UIControls Layer (Controls, Layout, Events...)  │
│  只使用抽象类型: SColor, RenderDevice, Texture,  │
│  Font, Window, Event                             │
├─────────────────────────────────────────────────┤
│  Backend Interface Layer (纯虚基类)              │
│  IRenderDevice / ITexture / IFont / IWindow      │
├──────────────────┬──────────────────┬────────────┤
│  SDL3Impl        │  SFMLImpl        │ raylibImpl │
│  (现有 SDL3 迁移) │  (新实现)         │ (新实现)    │
└──────────────────┴──────────────────┴────────────┘
```

### 1.3 已有基础设施

库中已有部分跨平台抽象雏形：

| 已有类型 | 位置 | 说明 |
|---------|------|------|
| `GraphTool::SColor` | `include/GraphTool.h` | 完整的颜色类，支持 float/byte/uint32 构造、颜色运算、`toSDLColor()` |
| `GraphTool::DrawingContext` | `include/GraphTool.h` | 部分封装了绘图操作 |
| `SPoint` / `SRect` | `include/Utility.h` | 几何类型，已有 `toSDLFPoint()` / `toSDLFRect()` 等转换 |
| `EventQueue` / `Event` | `include/EventQueue.h` | 内部事件系统（未完全解耦 SDL） |

## 1.4 当前进度

| Phase | 状态 | 完成内容 |
|-------|------|---------|
| 1 — SColor 统一 | ✅ **已完成** | SColor 独立头文件；所有控件 public API 迁移；ConstDef 常量迁移；StateColor → SColor；GraphTool SColor 别名 |
| 2 — RenderDevice 抽象 | ✅ **已完成** | RenderDevice 接口 + SDL3RenderDevice 实现；MainWindow/ControlBase 集成 RenderDevice*；GraphTool.cpp 完整迁移（零 SDL 调用）；全 54 处控件层 SDL 调用迁移（Menu/ControlBase/EditBox/TextArea/Bench 等 11 文件）|
| 3 — Texture / Surface 抽象 | ✅ **已完成** | Texture/Surface 抽象类；SDL3Texture/SDL3Surface 实现；`m_texture`: SDL_Texture* → SharedTexture；`m_surface`: SDL_Surface* → SharedSurface；GraphTool `void*` → `Texture*`；**LuotiAni.h ~180 SDL 调用完成迁移**；桥接方法全部移除 |
| 4 — SDL 头文件解耦 | ✅ **已完成** | 从 8 个非后端 header 移除 umbrella `#include <SDL3/SDL.h>`，替换为具体子头文件或完全消除；4 header 零 SDL 依赖；仅 MainWindow.h 保留 umbrella（后端必经依赖） |
| 5 — Font / TextRenderer | ⏳ **未开始** | |
| 6 — Window / Application | ⏳ **未开始** | |
| 7 — SFML 后端 | ⏳ **未开始** | |
| 8 — raylib 后端 | ⏳ **未开始** | |

## 2. Phase 1——SColor 统一

### 2.1 现状

已有 `GraphTool::SColor`，提供完整功能。**但所有控件的核心头文件直接使用 `SDL_Color`**：

| 位置 | SDL_Color 用途 |
|------|---------------|
| `ControlBase.h::StateColor` 4 个成员 | 颜色存储 |
| `ControlBase.h` 所有颜色 setter/getter | 接口签名 |
| `ConstDef.h` 所有默认颜色常量 | 常量定义 |
| `Panel.h` / `Button.h` / `CheckBox.h` 等 | 颜色参数 |
| `LayoutParser.cpp` | 颜色构造 |
| `WinFrame.h` / `WinFrame.cpp` | 颜色参数 |

### 2.2 迁移方案

SColor 已具备完整能力，**不需要重写，只需要提升和普及**。

#### 2.2.1 步骤 1：将 SColor 独立为顶层类型

从 `GraphTool::` 命名空间下**移动到独立头文件** `include/SColor.h`。

```cpp
// include/SColor.h（从 GraphTool.h 原样移出，不修改逻辑）
#pragma once
#include <cstdint>
#include <algorithm>

class SColor {
    // ... 现有实现完整保留 ...
    // float RGBA + byte 访问器
    // 工厂方法: Black(), White(), Red(), Green(), Blue(), ...
    // 运算: withAlpha(), brighter(), darker(), blend()
    // 比较: operator==, operator!=
};
```

保留兼容别名：

```cpp
// include/GraphTool.h 中添加
using SColor = ::SColor;  // 或 #include <SColor.h>
```

#### 2.2.2 步骤 2：跨文件批量替换 SDL_Color → SColor

每个头文件/ch 的改动方向（共约 25 个头文件 + 10 个 cpp）：

| 文件 | 替换内容 | 替换数 |
|------|---------|--------|
| `include/ControlBase.h` | `StateColor` 四成员 `SDL_Color` → `SColor`；所有颜色 setter/getter 签名 | ~20 |
| `include/ConstDef.h` | 所有 `DEFAULT_*_COLOR` 常量 `SDL_Color` → `SColor` | ~24 |
| `include/Panel.h` | `setBGColor(SDL_Color)` 等 | 2 |
| `include/Button.h` | `setBackgroundStateColor(StateColor)` 等参数（StateColor 内部改为 SColor，外部不变） | 3 |
| `include/Label.h` | `setTextNormalStateColor(SDL_Color)` 等 | 4 |
| `include/CheckBox.h` | CheckBoxBuilder 颜色方法 | 4 |
| `include/ProgressBar.h` | ProgressBarBuilder 颜色方法 | 4 |
| `include/ScrollBar.h` | ScrollBarBuilder 颜色方法 | 2 |
| `include/EditBox.h` | EditBoxBuilder 颜色方法 | 2 |
| `include/TextArea.h` | TextAreaBuilder 颜色方法 | 1 |
| `include/Menu.h` | MenuBarBuilder/ContextMenu 颜色方法 | 4 |
| `include/WinFrame.h` | `SColor` 已在用（`using GraphTool::SColor`），改为 `#include <SColor.h>` | 0 逻辑改 |
| `include/Theme.h` | 默认颜色类型 | 2 |
| `include/LayoutParser.h` | `parseColor` / `parseStateColor` 返回类型 | 2 |
| `include/GraphOperaAdapt2d.h` | `SColor` 已是正在使用 | 0 |
| `src/ControlBase.cpp` | 所有颜色实现 | ~20 |
| `src/Panel.cpp` | setBGColor 实现 | 2 |
| `src/Button.cpp` | ButtonBuilder 颜色方法实现 | 6 |
| `src/Label.cpp` | setTextXxxStateColor/TTF_SetTextColor(SDL_Color → SColor::toSDLColor) | 6 |
| `src/CheckBox.cpp` | CheckBoxBuilder 实现 | 4 |
| `src/LayoutParser.cpp` | 颜色解析（`#303030FF` → SColor） | ~100 |
| **小计** | | **~220 处** |

涉及 `#include` 变动：每个控件的 cpp 需要新增 `#include <SColor.h>`（或通过 `ControlBase.h` 间接引用）。

#### 2.2.3 步骤 3：StateColor 内部迁移

```cpp
// 改前
class StateColor {
    SDL_Color normal, hover, pressed, disabled;
public:
    StateColor(SDL_Color n, SDL_Color h, SDL_Color a, SDL_Color d);
    StateColor& setNormal(SDL_Color color);
    SDL_Color getNormal();
    // ...
};

// 改后
class StateColor {
    SColor normal, hover, pressed, disabled;
public:
    StateColor(SColor n, SColor h, SColor a, SColor d);
    StateColor& setNormal(SColor color);
    SColor getNormal();
    // ...
};
```

此改动不破坏任何外部调用——当前所有传 `SDL_Color` 的地方改为传 `SColor`，且

```cpp
// 构造 SColor 的方式与 SDL_Color 略有不同：
// SDL_Color 聚合初始化：{0x64, 0x64, 0x64, 0xFF}
// SColor 构造函数：SColor(0x64, 0x64, 0x64, 0xFF)
// 或 SColor(uint32_t) 构造函数：SColor(0x646464FF)
```

#### 2.2.4 步骤 4：绘制层的 SColor → SDL_Color 桥接

在所有需要调用原生绘制 API 的地方，通过 `.toSDLColor()` 或 `.toSDLFColor()` 转换：

```cpp
// ControlBase.cpp 绘制背景
SDL_Color c = m_bgColor.getNormal().toSDLColor();
SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
```

这部分代码**在 Phase 2（RenderDevice 抽象）中会被进一步消除**——RenderDevice 的 `setDrawColor(SColor)` 内部自动处理转换。

#### 2.2.5 ConstDef 默认常量兼容

```cpp
// 改前（SDL_Color 聚合初始化）
static constexpr SDL_Color DEFAULT_NORMAL_COLOR = {0x64, 0x64, 0x64, 0xFF};

// 改后（SColor 构造函数）
static constexpr SColor DEFAULT_NORMAL_COLOR(0x64, 0x64, 0x64, 0xFF);
```

需要确保 SColor 构造函数为 `constexpr`（当前 SColor 的 float 构造函数不是 constexpr，uint8_t 也不是）。

**建议**：在 SColor 中添加 constexpr uint8_t 构造函数，或提供 `constexpr` 工厂：

```cpp
static constexpr SColor fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return SColor(r, g, b, a); // 若构造函数非 constexpr，需调整
}
```

或另一种方案：**将 ConstDef 的常量改为 `inline static const`（非 constexpr）**，运行期初始化，对性能无影响。

### 2.3 向后兼容策略

迁移期间需要同时兼容新旧两套颜色：

```cpp
// 方式 A：先改 StateColor 内部为 SColor，保留 SDL_Color 重载
class StateColor {
    SColor normal;
public:
    // 新接口
    StateColor& setNormal(SColor c) { normal = c; return *this; }
    SColor getNormal() const { return normal; }

    // 临时兼容（可在一段时间后删除）
    StateColor& setNormal(SDL_Color c) { normal = SColor(c.r, c.g, c.b, c.a); return *this; }
    SDL_Color getNormalSDL() const { return normal.toSDLColor(); }
};
```

**建议**：不做兼容层，一次性替换所有文件。因为：
- SColor 的构造函数接受 `uint8_t` 格式，与 `SDL_Color` 字段一一对应
- 替换是机械的，可以写脚本辅助
- 兼容层会留下技术债

### 2.4 执行情况

| 条目 | 实际情况 |
|------|---------|
| 涉及头文件 | 17（含 SColor.h 新建） |
| 涉及 cpp 文件 | 14 |
| 替换/修改处 | ~220 处 |
| 实际工时 | <1 天（AI 辅助） |
| 测试覆盖 | 9 个原始测试全部构建通过 |
| 关键改动 | `include/SColor.h` 新建；`include/GraphTool.h` 移除内联 SColor + 添加引用别名；`include/ControlBase.h` StateColor 四成员 `SDL_Color` → `SColor`；`include/ConstDef.h` 所有默认颜色常量改为 `SColor`；`GraphOperaAdapt2d.h` 替换重复 SColor |

### 2.5 后续清理

- SColor.h 的 `#include <SDL3/SDL.h>` 暂保留（从 GraphTool.h 继承），待 Phase 2 完成后移除

## 2.6 工作量评估（原始计划）

| 条目 | 值 |
|------|-----|
| 涉及头文件 | ~20 |
| 涉及 cpp 文件 | ~12 |
| 单处替换数 | ~220 处 |
| 预估工时 | 2-3 天（含全面构建+测试） |
| 风险 | ★（类型替换，不改逻辑） |

## 3. Phase 2——RenderDevice 抽象

### 3.1 现状

`SDL_Renderer*` 穿过整个代码栈：

- `MainWindow` 持有 `SDL_Renderer*`，通过 `getInstance()->getRenderer()` 全局访问
- `ControlImpl` 有 `setRenderer()` / `getRenderer()` 虚方法
- `GET_RENDERER` 宏在 `ConstDef.h` 定义为 `MainWindow::getInstance()->getRenderer()`
- 所有控件的 `draw()` 实现直接调用 `SDL_RenderFillRect` 等
- `GraphTool::DrawingContext` 包装了部分绘图操作但仍暴露 `SDL_Renderer*`

所有绘制调用分布：

| SDL API | 调用数 | 主要使用者 |
|---------|--------|-----------|
| `SDL_SetRenderDrawColor` | ~30 | ControlBase, Bench, GraphTool, Menu, CheckBox, ScrollBar, ProgressBar, EditBox, TextArea 等 |
| `SDL_RenderFillRect` | ~15 | ControlBase, Bench, CheckBox, ScrollBar, ProgressBar, EditBox, TextArea, Menu, GraphTool |
| `SDL_RenderRect` | ~5 | ControlBase, Bench, GraphTool |
| `SDL_RenderLine` | ~5 | Menu, GraphTool |
| `SDL_RenderGeometry` + `SDL_Vertex` | ~22 | GraphTool（多边形/圆角矩形） |
| `SDL_RenderTexture` | ~4 | Actor |
| `SDL_RenderTextureRotated` | ~1 | LuotiAni |
| `SDL_SetRenderClipRect` | ~7 | TextArea, EditBox, GraphTool |
| `SDL_RenderReadPixels` | ~1 | LuotiAni |
| `SDL_SetRenderTarget` | ~2 | LuotiAni |

### 3.2 接口设计

```cpp
// include/render/RenderDevice.h
class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    // === 生命周期 ===
    virtual bool init(SDL_Window* window) = 0;  // 或 void* nativeHandle
    virtual void beginFrame(SColor clearColor) = 0;
    virtual void endFrame() = 0;

    // === 渲染状态 ===
    virtual void setDrawColor(SColor color) = 0;
    virtual void setClipRect(const SRect& rect) = 0;
    virtual void clearClipRect() = 0;
    virtual void setBlendMode(BlendMode mode) = 0;

    // === 基础图元 ===
    virtual void fillRect(const SRect& rect) = 0;
    virtual void drawRect(const SRect& rect) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2) = 0;

    // === 复杂图元（GraphTool 核心需求） ===
    virtual void fillTriangle(const SPoint& v0, const SPoint& v1, const SPoint& v2) = 0;
    virtual void drawPolygon(const SPoint* vertices, int count) = 0;
    virtual void fillPolygon(const SPoint* vertices, int count) = 0;

    // === 纹理操作 ===
    virtual SharedTexture createTexture(const void* data, int width, int height) = 0;
    virtual SharedTexture createTextureFromFile(const string& path) = 0;
    virtual void drawTexture(Texture* tex, const SRect& src, const SRect& dst) = 0;
    virtual void drawTextureRotated(Texture* tex, const SRect& src, const SRect& dst, float angle) = 0;

    // === 渲染到纹理 ===
    virtual void setRenderTarget(Texture* tex) = 0;
    virtual void resetRenderTarget() = 0;
    virtual void readPixels(void* buffer, const SRect& rect) = 0;
};
```

> **注**：`BlendMode` 为枚举类，提供 `None / Blend / Add / Mod / Mul` 等值，各后端内部映射到具体 API 值。

### 3.3 SDL3 实现示例

```cpp
// src/backend/sdl3/SDL3RenderDevice.cpp
class SDL3RenderDevice : public RenderDevice {
    SDL_Renderer* m_renderer;
public:
    void setDrawColor(SColor color) override {
        SDL_SetRenderDrawColor(m_renderer, color.redByte(), color.greenByte(),
                               color.blueByte(), color.alphaByte());
    }
    void fillRect(const SRect& rect) override {
        SDL_FRect r = rect.toSDLFRect();
        SDL_RenderFillRect(m_renderer, &r);
    }
    // ... 其余实现
};
```

### 3.4 实际接口（已实现）

`include/RenderDevice.h` 包含以下纯虚方法（约 25 个）：

| 类别 | 方法 |
|------|------|
| 渲染状态 | `setDrawColor(SColor)` / `setClipRect(SRect)` / `clearClipRect()` |
| 基础图元 | `fillRect(SRect)` / `drawRect(SRect)` / `drawLine(x1,y1,x2,y2)` / `drawPoint(x,y)` |
| 复杂图元 | `fillTriangle(v0,v1,v2)` / `drawTriangle(v0,v1,v2,c0,c1,c2)` (逐顶点颜色) / `drawQuad()` (两种重载) |
| 纹理操作 | `drawTexture(void*, SRect, SRect)` / `drawTexture(void*, SRect, SRect, double angle)` |
| 帧操作 | `clear()` / `present()` |

> **设计决策**：未实现 `beginFrame`/`endFrame`，改用 `clear()` + `present()` 更贴近 SDL3 语义。未实现 `fillPolygon`/`drawPolygon`，改用 `drawTriangle`/`drawQuad` 覆盖 GraphTool 实际需求。

### 3.5 已完成工作

| 文件 | 改动 |
|------|------|
| `include/RenderDevice.h` | 新建抽象接口 |
| `src/RenderDevice.cpp` | `SDL3RenderDevice` 具体实现 + 工厂函数 `CreateSDL3RenderDevice(SDL_Renderer*)` |
| `include/MainWindow.h` | `RenderDevice* m_renderDevice` + `getRenderDevice()` + `GET_RENDERDEVICE` 宏 + 析构清理 |
| `include/ControlBase.h` / `src/ControlBase.cpp` | `getRenderDevice()` / `setRenderDevice()` 带 parent/game 回退链；`inheritRenderer()` 改为继承 render device |
| `include/GraphTool.h` | `DrawingContext` 构造函数从 `SDL_Renderer*` 改为 `RenderDevice*`；移除 `#include <SDL3/SDL.h>` |
| `src/GraphTool.cpp` | **完整重写（1452 行）**：17 处 `SDL_SetRenderDrawColor` → `setDrawColor`；所有 `SDL_RenderFillRect`/`Rect`/`Line`/`Point` → device 调用；16 处 `SDL_RenderGeometry` + `SDL_Vertex` → `drawTriangle()`/`drawQuad()`；纹理/裁剪 → device 方法。**零 SDL 调用残留** |
| `CMakeLists.txt` | 添加 `src/RenderDevice.cpp` 到库源 |
| 所有 10 个 test 文件 | `SDL_SetRenderDrawColor` → `device->setDrawColor()`；`SDL_RenderClear` → `device->clear()`；`SDL_RenderPresent` → `device->present()`；`DrawingContext(getRenderer())` → `DrawingContext(getRenderDevice())` |
| `layouts/test_layout_advanced.json` | rootPanel 改为 `"w": "100%", "h": "100%"` 以响应窗口缩放；修复 caption 文案 |
| `src/Bench.cpp` | `Bench::addControl()` 添加 `resolveChildPercentages()` 调用以支持子控件百分比尺寸 |

### 3.6 剩余工作（~60 处 SDL 调用待迁移）

仍使用 `getRenderer()` 并直接调用 SDL API 的文件：

| 文件 | 残留 SDL 调用 | 说明 |
|------|--------------|------|
| `src/Menu.cpp` | ~12 | `SetRenderDrawColor` + `RenderFillRect` + `RenderLine` 在 MenuPanel/MenuBar 绘制 |
| `src/ControlBase.cpp` | ~4 | `drawBackground()` / `drawBorder()` |
| `src/EditBox.cpp` | ~8 | 选区填充、光标绘制、裁剪 |
| `src/TextArea.cpp` | ~8 | 选区填充、光标绘制、裁剪 |
| `src/Label.cpp` | ~4 | debug 边框绘制 |
| `src/ScrollBar.cpp` | ~4 | 轨道/滑块绘制 |
| `src/ProgressBar.cpp` | ~4 | 进度条绘制 |
| `src/Bench.cpp` | ~8 | 加载进度条网格绘制（`SDL_SetRenderDrawColor` + `RenderFillRect` + `RenderRect`） |
| `subModules/LuotiAni/LuotiAni.h` | ~5 | 渲染到纹理、像素回读 |
| `src/Actor.cpp` | ~2 | 纹理创建、渲染 |
| `src/Material.cpp` | ~2 | 纹理渲染 |

### 3.7 工作量评估（原始计划）

- **预估工时**：5-7 天
- **实际进度**：基础设施 + GraphTool 迁移 ≈ 2 天（AI 辅助）
- **风险**：★★★
- **关键难点**：LuotiAni 的渲染到纹理 + 像素回读；GraphTool 的几何渲染（已解决）
- **预计剩余工时**：1-2 天

## 4. Phase 3——Texture 抽象

### 4.1 现状

`SDL_Texture*` 和 `SDL_Surface*` 散布在：

- `Actor.h`：`SDL_Texture*` + `SDL_Surface*`
- `LuotiAni.h`：大量 Surface 合成操作（blit、旋转、缩放、alpha 调整）
- `Material.h`：纹理管理
- `GraphTool.h`：纹理尺寸查询

### 4.2 接口设计

```cpp
// include/render/Texture.h
class Texture {
public:
    virtual ~Texture() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
};
using SharedTexture = shared_ptr<Texture>;
```

### 4.3 迁移方案

```cpp
// Actor.h 改前：
SDL_Texture* m_texture;
SDL_Surface* m_surface;

// Actor.h 改后：
SharedTexture m_texture;
```

Surface 在 LuotiAni 中用于 CPU 端像素合成。有两种处理策略：

| 策略 | 做法 | 优劣 |
|------|------|------|
| **轻量包装** | 定义一个 `class Surface` 包装 CPU pixel buffer，提供 `blit()` / `blitScaled()` / `setAlphaMod()` 等操作 | 与具体 API 无关；只需在最终上传纹理时调用 `RenderDevice::createTexture(data, w, h)` |
| **直接保留 SDL_Surface** | LuotiAni 内部保留 SDL_Surface 操作，仅在上传纹理时抽象 | 迁移量小但仍有 SDL 残留 |

> **建议**：定义 `Surface` 类包装 CPU pixel buffer，因为 SFML 和 raylib 都有各自的像素图操作需求。

```cpp
// include/render/Surface.h
class Surface {
public:
    virtual ~Surface() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void* pixels() = 0;

    virtual void blit(Surface* src, const SRect& srcRect, const SRect& dstRect) = 0;
    virtual void blitScaled(Surface* src, const SRect& srcRect, const SRect& dstRect) = 0;
    virtual void setAlphaMod(uint8_t alpha) = 0;
    virtual void setBlendMode(BlendMode mode) = 0;

    // 工厂
    static SharedSurface create(int width, int height);
    static SharedSurface loadFromFile(const string& path);
    static SharedSurface loadFromMemory(const void* data, size_t len);
};
```

### 4.4 工作量评估

- **预估工时**：4-5 天
- **风险**：★★★★（LuotiAni 动画系统耦合深）

> **实际完成 (2026-06-02)**：LuotiAni.h ~180 处 SDL 调用全部迁移至 `Surface::create()` / `blit()` / `setAlphaMod()` / `setBlendMode()` / `rotate()` 等抽象 API。`loadFromStream(SDL_IOStream*)` 替换为 `parseJsonDesc()` 使用 `std::ifstream`。桥接方法 `createTextureFromSDLSurface`、`getNativeRenderer`、`loadTextureFromSurface(SDL_Surface*)` 全部移除。`RenderDevice.h` 无任何 SDL include（仅 `struct SDL_Renderer;` 前向声明）。

## 5. Phase 4——Font / TextRenderer 抽象

### 5.1 现状

SDL3_ttf 的 API 绑定紧密：

```
Label:       TTF_Font* + TTF_TextEngine* + TTF_Text*
EditBox:     TTF_Font* + TTF_TextEngine* + TTF_Text* (光标/选中文本有独立 TTF_Text)
TextArea:    TTF_TextEngine* + (行缓存: vector<TTF_Text*>)
```

三个控件各有独立文本渲染管线，但核心流程一致：
1. 创建 `TTF_CreateRendererTextEngine(renderer)`（绑定到 renderer）
2. 创建 `TTF_CreateText(engine, font, text)` 
3. 测量 `TTF_GetTextSize(text, &w, &h)`
4. 绘制 `TTF_DrawText(text, x, y)`（内部用 renderer 绘制）

### 5.2 接口设计

```cpp
// include/text/Font.h
class Font {
public:
    virtual ~Font() = default;
    virtual float measureWidth(const string& text, float fontSize) = 0;
    virtual float measureHeight(const string& text, float fontSize) = 0;
    virtual SSize measureText(const string& text, float fontSize) = 0;
};
using SharedFont = shared_ptr<Font>;

// include/text/TextRenderer.h
class TextRenderer {
public:
    virtual ~TextRenderer() = default;
    virtual SharedFont loadFont(const string& path, float size) = 0;
    virtual SharedFont loadFontFromMemory(const void* data, size_t len, float size) = 0;

    // 绘制文本（通过 SColor 解耦）
    virtual void drawText(Font* font, const string& text, float x, float y, SColor color) = 0;
    virtual void drawTextInRect(Font* font, const string& text, const SRect& rect,
                                SColor color, AlignmentMode align) = 0;
};
```

### 5.3 SDL3 实现要点

SDL3_ttf 的 `TTF_CreateRendererTextEngine` 要求传入 `SDL_Renderer*`，但在 RenderDevice 抽象后应改为：

```cpp
// SDL3TextRenderer 在初始化时从 SDL3RenderDevice 获取底层 renderer
// 方式 A：RenderDevice 增加 getNativeHandle() → void*，SDL3 实现返回 SDL_Renderer*
// 方式 B：SDL3TextRenderer 持有一个 SDL_Renderer* 引用
```

### 5.4 Label 迁移示例

```diff
// Label.h 改前：
- TTF_Font* m_font;
- TTF_TextEngine* m_textEngin;

// Label.h 改后：
+ SharedFont m_font;
+ TextRenderer* m_textRenderer;  // 从 ControlBase 或全局获取
```

```diff
// Label.cpp 绘制改前：
- SDL_SetRenderDrawColor(m_renderer, ...);
- TTF_SetTextColor(m_text, ...);
- TTF_DrawText(m_text, x - m_translatedPos.x, y - m_translatedPos.y);

// Label.cpp 绘制改后：
+ m_textRenderer->drawText(m_font.get(), m_caption, drawX, drawY, m_textColor.getNormal());
```

### 5.5 工作量评估

- **预估工时**：5-7 天
- **风险**：★★★★★（三个控件各自有文本缓存、选中、滚动等复杂逻辑）

## 6. Phase 5——Window / Application 抽象

### 6.1 现状

所有 test 文件使用 SDL3 callback main：

```cpp
#define SDL_MAIN_USE_CALLBACKS 1
SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) { ... }
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) { ... }
SDL_AppResult SDL_AppIterate(void* appstate) { ... }
void SDL_AppQuit(void* appstate, SDL_AppResult result) { ... }
```

`MainWindow` 单例持有 `SDL_Window*` + `SDL_Renderer*`，通过 `getInstance()` 全局访问。

### 6.2 接口设计

```cpp
// include/core/Window.h
class Window {
public:
    virtual ~Window() = default;
    virtual bool create(const string& title, int width, int height, uint32_t flags) = 0;
    virtual void destroy() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual float dpiScale() const = 0;
    virtual RenderDevice* renderDevice() = 0;
    virtual void* nativeHandle() = 0;  // HWND / NSWindow* / x11 Window
};

// include/core/Application.h —— 两种入口模式
// 模式 A：回调模式（保留 SDL3 风格，适合 game loop）
class AppCallbacks {
public:
    virtual bool onInit() = 0;
    virtual void onEvent(const Event& event) = 0;
    virtual void onUpdate() = 0;
    virtual void onRender() = 0;
    virtual void onQuit() = 0;
};

// 模式 B：轮询模式（传统 main loop）
class Application {
public:
    virtual bool init(int argc, char** argv) = 0;
    virtual bool pollEvent(Event& event) = 0;
    virtual void swapBuffers() = 0;
    virtual void quit() = 0;
    virtual Window* mainWindow() = 0;
};
```

**建议**：采用模式 A（`AppCallbacks`），因为：
- 与现有 SDL3 callback main 结构一致，迁移最小
- test 文件只需将 `SDL_App*` 函数体搬到 `AppCallbacks` 虚方法中
- 后端可以在内部选择使用 callback main 或传统 main

```cpp
// 使用示例（test 文件）
class MyApp : public AppCallbacks {
    bool onInit() override { /* 原 SDL_AppInit 内容 */ }
    void onEvent(const Event& e) override { /* 原 SDL_AppEvent 内容 */ }
    void onUpdate() override { /* 原 SDL_AppIterate 中非渲染部分 */ }
    void onRender() override { /* 原 SDL_AppIterate 中渲染部分 */ }
    void onQuit() override { /* 原 SDL_AppQuit 内容 */ }
};

BACKEND_MAIN(MyApp)  // 宏，各后端展开为正确的入口
```

### 6.3 事件系统重构

当前内部 `Event` 类的问题：

```cpp
// include/EventQueue.h
class Event {
    EventName m_eventName;                          // 枚举
    std::any m_eventParam;                          // ← 类型不安全
    shared_ptr<Control> m_who;                      // 目标控件
};
```

需要改为类型安全的 union/variant：

```cpp
// 方式一：union + enum（推荐，性能和类型安全平衡）
struct MousePos { float x, y; };
struct MouseWheelData { float x, y; float scrollX, scrollY; };
// ...

class Event {
public:
    EventName name;
    Control* target;
    union {
        MousePos mousePos;
        MouseWheelData mouseWheel;
        // ... 其他事件类型
    };
};
```

这将一并解决历史问题：
- `isPositionEvent()` + `bad_any_cast` 问题
- MOUSE_WHEEL 参数类型不一致问题

### 6.4 迁移策略

将当前 SDL3 的 `SDL_AppEvent` 中的事件转换逻辑抽取为 `SDL3InputBackend`：

```cpp
// SDL3 后端
class SDL3App : public AppCallbacksWrapper {
    SDL_Window* m_window;
    SDL3RenderDevice* m_renderDevice;
    SDL3InputBackend m_input;
public:
    bool onInit() override {
        SDL_Init(SDL_INIT_VIDEO);
        m_window = SDL_CreateWindowAndRenderer(...);
        m_renderDevice = new SDL3RenderDevice(m_window);
        m_input = SDL3InputBackend(m_window);
    }
};
```

### 6.5 工作量评估

- **预估工时**：3-4 天
- **风险**：★★

## 7. Phase 6——SFML 后端实现

### 7.1 实现内容

| 抽象接口 | SFML 实现 | 预计代码量 |
|---------|-----------|-----------|
| `RenderDevice` | 包装 `sf::RenderWindow` / `sf::RenderTexture` | ~500 行 |
| `Texture` | 包装 `sf::Texture` | ~100 行 |
| `Surface` | 使用 `sf::Image` 作为 pixel buffer | ~200 行 |
| `Font` | 包装 `sf::Font` | ~100 行 |
| `TextRenderer` | 使用 `sf::Text` + `window.draw()` | ~300 行 |
| `Window` | 包装 `sf::RenderWindow` | ~200 行 |
| `AppCallbacks` | 自定义 main loop：`while(window.isOpen())` | ~100 行 |
| **总计** | | **~1500 行** |

### 7.2 关键差异点

- SFML 没有直接对应 `SDL_SetRenderDrawColor` + `SDL_RenderFillRect` 的简单 API——每次绘制需要构造 `sf::RectangleShape` + 设置 `fillColor`。可在 `fillRect` 实现中缓存 shape 对象
- SFML 的 `sf::Text` 设置颜色是直接 `setFillColor()`，与 SDL3_ttf 的 `TTF_SetTextColor` 不同
- SFML 没有内建的圆角矩形，需要自己组合（可用 `sf::ConvexShape` 或 vertex array）

## 8. Phase 7——raylib 后端实现

### 8.1 实现内容

| 抽象接口 | raylib 实现 | 预计代码量 |
|---------|-----------|-----------|
| `RenderDevice` | 传统的 `BeginDrawing()` / `EndDrawing()` | ~300 行 |
| `Texture` | 包装 `Texture2D` | ~100 行 |
| `Surface` | 使用 `Image` 类型 | ~200 行 |
| `Font` | 包装 `Font` | ~100 行 |
| `TextRenderer` | `DrawTextEx()` 等 | ~200 行 |
| `Window` | `InitWindow()` / `CloseWindow()` | ~150 行 |
| `AppCallbacks` | 自定义 main loop | ~100 行 |
| **总计** | | **~1150 行** |

### 8.2 关键差异点

- raylib 是立即模式 API，与 SDL3/SFML 的保留模式不同。例如文本渲染已是最简：`DrawTextEx(font, text, pos, fontSize, spacing, color)`
- raylib 的 `BeginMode2D` / `EndMode2D` 对应 GraphTool 的坐标变换
- raylib 的 `GuiDrawRectangle()` 等内置 GUI 函数与 UIControls 的控件可能冲突，需确保只使用底层 API
- raylib 的 `rlgl` 模块可访问 OpenGL 1.1 风格的立即模式顶点提交，可对标 GraphTool 的 `SDL_RenderGeometry`

## 9. 总体工时汇总

| Phase | 内容 | 文件数 | 新增行数 | 预估天数 | 实际进度 | 风险 |
|-------|------|--------|---------|---------|---------|------|
| 1 | SColor 统一 | ~35 | ~100 | 2-3 | ✅ 已完成 | ★ |
| 2 | RenderDevice 接口 + SDL3 实现 | ~23 | ~650 (净增) | 5-7 | ✅ 已完成 | ★★★ |
| 3 | Texture + Surface 抽象 | ~10 | ~1000 | 4-5 | ✅ 已完成（含 LuotiAni 完整迁移） | ★★★★ |
| 4 | SDL 头文件解耦 | ~10 | ~35 | 1 | ✅ 已完成 | ★ |
| 5 | Font / TextRenderer 抽象 | ~12 | ~800 | 5-7 | ⏳ **当前阶段** | ★★★★★ |
| 6 | Window / App / Event 重构 | ~15 | ~600 | 3-4 | ⏳ | ★★ |
| 7 | SFML 后端 | ~5 | ~1500 | 5-7 | ⏳ | ★★★★ |
| 8 | raylib 后端 | ~5 | ~1150 | 5-7 | ⏳ | ★★★★ |
| **合计** | | **~115** | **~5700** | **30-42** | | |

## 10. 执行建议

### 10.1 实际执行顺序与计划

```
Phase 1 (SColor)          ← ✅ 已完成
    ↓
Phase 2 (RenderDevice)    ← ✅ 已完成 — 所有 SDL 绘制调用已迁移至 RenderDevice
    ↓
Phase 3 (Texture/Surface) ← ✅ 已完成 — 含 LuotiAni.h 完整迁移 (~180 SDL 调用)
    ↓
Phase 4 (Header Cleanup)  ← ✅ 已完成 — 8 非后端 header 移除 umbrella SDL include
    ↓
Phase 5 (Font/TextRenderer) ← 🔄 当前阶段
    ↓
Phase 6 (Window/App/Event)  ← ⏳ 待开始
    ↓
Phase 7 (SFML backend)      ← ⏳ 待开始
    ↓
Phase 8 (raylib backend)    ← ⏳ 待开始
```

> **注**：Phase 4（SDL 头文件解耦）在 Phase 3 后自然衍生——Texture/Surface 抽象完成后，大量头文件不再需要 SDL 类型，可直接移除或替换为具体子头文件。

### 10.2 编译时选择后端

CMake 选项：

```cmake
option(UICONTROLS_BACKEND "Backend library" "SDL3")  # 可选: SDL3, SFML, RAYLIB
target_compile_definitions(UIControls PUBLIC UICONTROLS_BACKEND=${UICONTROLS_BACKEND})

if(UICONTROLS_BACKEND STREQUAL "SDL3")
    target_link_libraries(UIControls PUBLIC SDL3 SDL3_ttf SDL3_image)
elseif(UICONTROLS_BACKEND STREQUAL "SFML")
    target_link_libraries(UIControls PUBLIC sfml-graphics sfml-window sfml-system)
elseif(UICONTROLS_BACKEND STREQUAL "RAYLIB")
    target_link_libraries(UIControls PUBLIC raylib)
endif()
```

### 10.3 关于"自绘鼠标"

Phase 2 完成后，RenderDevice 接管了所有绘制。Phase 5 的 Window 抽象暴露了鼠标位置。此时在 UIControls 层实现软光标只需：

```cpp
// 在 BENCH->draw() 最后
if (m_useCustomCursor) {
    GET_RENDER_DEVICE->drawTexture(m_cursorTexture, ..., mousePos);
}
```

**不需要等到后端抽象完成**——SColor 统一后（Phase 1），即可在现有 SDL3 代码中用 `SDL_ShowCursor(SDL_DISABLE)` + 手动绘制光标纹理实现。推荐在 Phase 1 后评估是否值得独立做。
