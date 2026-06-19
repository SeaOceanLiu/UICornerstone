# 后端抽象层设计文档

## 1. 概述

UICornerstone 当前硬编码绑定 SDL3，所有控件直接在头文件中引用 `SDL_Color`、`SDL_Renderer*`、`SDL_Texture*` 等 SDL3 类型。本文档规划如何抽离出**平台无关的抽象接口层**，使库可以无缝切换后端（SDL3 / SFML / raylib 等）。

### 1.1 设计目标

- **控制层零 SDL 依赖**：所有控件的 `.h` / `.cpp` 不再 `#include <SDL3/...>`
- **后端可插拔**：编译时通过 CMake 选项 `-DBACKEND=SDL3|SFML|RAYLIB` 选择
- **迁移可增量**：不必一次性全部替换，可定义接口 + 保留旧实现并行
- **零性能损失**：抽象层为纯虚基类 + 内联转换，不引入虚函数调用开销在渲染热路径上

### 1.2 总体架构

```
┌─────────────────────────────────────────────────┐
│  UICornerstone Layer (Controls, Layout, Events...)  │
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
| `ResourceProvider` | `include/ResourceProvider.h` | 文件 I/O 抽象，支持 filesystem + 资源包 |
| `Cursor` | `include/Cursor.h` | 系统光标抽象，支持 20 种光标类型 |

> **注**：实际执行中 phase 编号与本文档初始规划有偏差。规划中 Phase 6="Window/Application/Event+BackendPlugin"、Phase 7="SFML 后端"、Phase 8="raylib 后端"。额外完成的 Phase 7-12 为本文档编写时未预见的工作项（实际去耦步骤），见 §10-§14a。Phase 13（SFML 后端）已完成（§15），Phase 13a（SFML 性能优化 + 后端一致性修复）已完成（§15.4），Phase 14（raylib 后端）已完成（§16），Phase 15（CheckBox/Label 性能优化）已完成（§16.7）。

## 1.4 当前进度

| Phase | 状态 | 完成内容 |
|-------|------|---------|
| 1 — SColor 统一 | ✅ **已完成** | SColor 独立头文件；所有控件 public API 迁移；ConstDef 常量迁移；StateColor → SColor；GraphTool SColor 别名 |
| 2 — RenderDevice 抽象 | ✅ **已完成** | RenderDevice 接口 + SDL3RenderDevice 实现；MainWindow/ControlBase 集成 RenderDevice*；GraphTool.cpp 完整迁移（零 SDL 调用）；全 54 处控件层 SDL 调用迁移（Menu/ControlBase/EditBox/TextArea/Bench 等 11 文件）|
| 3 — Texture / Surface 抽象 | ✅ **已完成** | Texture/Surface 抽象类；SDL3Texture/SDL3Surface 实现；`m_texture`: SDL_Texture* → SharedTexture；`m_surface`: SDL_Surface* → SharedSurface；GraphTool `void*` → `Texture*`；**LuotiAni.h ~180 SDL 调用完成迁移**；桥接方法全部移除 |
| 4 — SDL 头文件解耦 | ✅ **已完成** | 从 8 个非后端 header 移除 umbrella `#include <SDL3/SDL.h>`，替换为具体子头文件或完全消除；4 header 零 SDL 依赖；仅 MainWindow.h 保留 umbrella（后端必经依赖） |
| 5 — Font / TextRenderer | ✅ **已完成** | Font/TextRenderer 抽象接口；SDL3Font/SDL3TextRenderer 实现；Label/EditBox/TextArea 全部迁移至 TextRenderer；LayoutParser TTF 类型引用消除；测试文件 TTF_Init/TTF_Quit 移除 |
| 6 — Window / Application + AppCallbacks | ✅ **已完成** | InputBackend 独立文件；MainWindow.h 脱 SDL（Window 抽象）；AppCallbacks 接口；tick-based + run() 双模式；所有 10 测试迁移至 AppCallbacks |
| 7 — TTF_Text* 缓存 | ✅ **已完成** | Label 层缓存 `TTF_Text*` 对象（`m_cachedTexts`）；`releaseTexts()` + `recreate()` 增量更新；`~Label()/setCaption()/setFont()/resized()` 触发失效 |
| 8 — ResourceProvider 抽象 | ✅ **已完成** | `ResourceProvider` 接口（`readFile/openFileStream/exists`）；`FilesystemResourceProvider` 实现；集成 ControlBase/MainWindow；Label/EditBox/Actor/LuotiAni 使用；`FontData` 生命周期修复 |
| 9 — 移除 ResourceLoader | ✅ **已完成** | `ResourceLoader.h/.cpp` 删除；`FontName`/`fontFiles` 迁至 `ConstDef`；Bench 简化；所有测试移除 `detachLoadingThread()` |
| 10 — SDL Cursor 抽象 | ✅ **已完成** | `Cursor` 抽象类 + `SystemCursorType`；`SDLCursor` 实现；Label/WinFrame 全部迁移；Label.h/WinFrame.h 零 SDL 光标类型引用 |
| 11 — 移除 ControlBase SDL_Renderer | ✅ **已完成** | ControlBase.h 移除 `SDL_Renderer*` 接口/成员/`#include <SDL3/SDL_render.h>`；ConstDef.h SDL 宏→字面量；Bench 死代码清理 |
| 12 — 事件系统迁移（Controls → Union API） | ✅ **已完成** | 所有 8 控件 handleEvent 从 `EventName+std::any` 迁移至 `EventType` union；`Event(EventName,any)` 构造自动映射至新 API；旧数据 struct 统一至 EventTypes.h |
| 13 — SFML 后端 | ✅ **已完成** | 全部 6 个抽象接口的 SFML 实现（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）；nanosvg SVG 栅格化支持；SFML v3 API 适配；10 测试编译通过 |
| 13a — SFML 性能优化 + 后端一致性修复 | ✅ **已完成** | `sf::VertexArray` 批量顶点提交（`m_fillBatch` + `m_lineBatch`）；`RenderDevice::flush()` 接口；TextRenderer 刷新前脏标记检查；TextArea 控制字符过滤；InputBackend 事件修饰符修复；中文编码修复；**SDL3 RenderDevice 默认 blend mode 修复（BLENDMODE_NONE→BLEND），消除 NoBrush + alpha=0 填充的黑块差异** |
| 14 — raylib 后端 | ✅ **已完成** | A-F 全部 12 子阶段，包含骨架/窗口/Cursor/InputBackend/基础图元/三角形绕序修复/Surface+SVG/字体+文本/wrapWidth/Resize Freeze 修复/Font DPI 缩放  |
| 15 — CheckBox/Label 性能优化 | ✅ **已完成** | SDL3 TextRenderer 字体缓存；CheckBox/Label 脏矩形/脏父检查；`updateChildScale()` 替代 `setParent(this)` 传播缩放；基类 ControlImpl 脏矩形防护注释；`m_layoutDone` 已删除（被脏矩形覆盖）|
| 15a — WinFrame 修复 + 绘制架构 + 帧率同步 | ✅ **已完成** | WinFrame 置顶/Z-order 修复；WinFrame 重叠缩放光标修复；标题栏 `setClickable(false)` 防拦截拖动；`beforeDraw/afterDraw` 两阶段绘制架构 + `m_frameDrawRect` 缓存；`WINDOW_FLAG` 修复（误用 `SDL_WINDOW_UTILITY` 导致不在任务栏显示 → `SDL_WINDOW_HIGH_PIXEL_DENSITY`）；Raylib 移除 `WaitTime` 60 FPS 锁；SDL3 显式 `SDL_SetRenderVSync(renderer, 0)` |
| 15b — SFML z-order + DebugTrace | ✅ **已完成** | SFML 批处理 z-order 修复；DebugTrace.h 从项目中移除 |
| 16 — RGBA8888 像素格式排查 + DLL 桥接验证 | ✅ **已完成** | 发现 RGBA8888 在 little-endian x86 上字节顺序为 A(LSB),B,G,R(MSB)；`uint32_t` 像素值必须用 `(R<<24)|(G<<16)|(B<<8)|A` 格式；验证 DLL 桥接（`GetUIBackendCallbacks`）纹理绘制/程序化表面/PNG 加载全部正常 |
| 16b — Raylib DrawTexturePro DLL 桥接修复 | ✅ **已完成** | `DrawTexturePro` 和 `rlBegin/rlEnd` 在 fromsource/bridge 模式下不可见（根因未知）；改用 `rlPushMatrix + rlScalef + DrawTextureEx` 实现非均匀拉伸纹理绘制；`DrawTextureEx` 在 scale=1 时正常工作；透明热身（alpha=0）无效 |
| 16c — fromsource 纹理桥接 + 字体原地重载 | ✅ **已完成** | Actor::loadFromFile 回退 createTextureFromFile（Surface 工厂在桥接模式下不可用）；RaylibFont::reload() 原地重载字体（不更换 shared_ptr 身份，桥接句柄保持有效）| 
| 16d — WinFrame 关闭按钮 X 可见性修复 | ✅ **已完成** | Raylib drawTexture 应用 BlendMode（BeginBlendMode）；WinFrame::draw() 向量 X 叠加层作为跨后端回退方案（drawLine 画 X 对角线）|
| 16e — fromsource 三后端 Separate TU 编译 | ✅ **已完成** | test_fromsource_{sdl3,sfml,raylib}.cpp 三文件独立编译；FROMSOURCE_BACKEND_SOURCES/LIBS CMake 变量；winmm.lib（raylib）、opengl32.lib（SFML）依赖 |
| 16f — fromsource 4 bug 修复 | ✅ **已完成** | SFML/Raylib 表面工厂注册（RegisterSFMLSurfaceFactories）；SFML vsync 关闭；Raylib newFrame 回调桥接解决"未响应"；Raylib 中文码点懒加载 |
| 16g — Raylib 纹理不可见根因排查 + 彻底修复 | ✅ **已完成** | 双根因：(1)bridge_drawTexture nullptr src→零SRect；(2)rlPushMatrix+rScalef+DrawTextureEx DLL 边界不可见。修复：bridge 传 nullptr 而非 &zeroRect；改用 DrawTexturePro |

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

- ✅ SColor.h 已从 `#include <SDL3/SDL.h>` 缩小为 `<SDL3/SDL_pixels.h>`（Phase 4 完成）

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

> ✅ **已完成**（Phase 2 最终状态：所有 54 处 SDL 调用已在 ControlBase/Menu/EditBox/TextArea/Label/ScrollBar/ProgressBar/Bench/LuotiAni/Actor/Material 等 11 文件中完成迁移至 RenderDevice）

仍使用 `getRenderer()` 并直接调用 SDL API 的文件（迁移前状态）：

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

## 5. Font / TextRenderer 抽象（原始设计）

> **说明**：本节的原始设计已被 §6（Phase 5）的实际实现取代，保留作为设计演进的历史记录。

### 5.1 设计目标（Phase 4 实际已由 Phase 4——Header Cleanup 取代，内容详见 §6. Phase 5 完成报告）

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

## 6. Phase 5——Font / TextRenderer 抽象

### 6.1 现状

Phase 4 解耦 SDL 头文件后，Label / EditBox / TextArea 仍然在头文件中直接引用 `TTF_Font*`、`TTF_Text*`、`TTF_TextEngine*` 等 SDL_ttf 类型，并在 .cpp 中使用大量 SDL_ttf API（`TTF_OpenFont`、`TTF_CreateText`、`TTF_DrawRendererText`、`TTF_GetTextSize` 等）。

### 6.2 设计方案

定义两层抽象：

#### 6.2.1 Font 抽象

```cpp
// include/Font.h
class Font {
public:
    virtual ~Font() = default;
    virtual int getSize() const = 0;    // 缩放后的像素大小
};
using SharedFont = shared_ptr<Font>;
```

#### 6.2.2 TextRenderer 抽象

```cpp
// include/TextRenderer.h
class TextRenderer {
public:
    virtual ~TextRenderer() = default;
    virtual SharedFont loadFont(const string& path, int ptSize) = 0;
    virtual SharedFont loadFontFromMemory(const uint8_t* data, size_t len, int ptSize) = 0;
    virtual SSize measureText(const Font* font, const string& text) const = 0;
    virtual int getFontHeight(const Font* font) const = 0;
    virtual void drawText(Font* font, const string& text, float x, float y, SColor color) = 0;
    virtual void drawText(Font* font, const string& text, float x, float y, SColor color, float wrapWidth) = 0;
};

TextRenderer* CreateSDL3TextRenderer(RenderDevice* device);
```

### 6.3 迁移模式

```
// Before (控制层直接调用 SDL_ttf):
TTF_Text* ttfText = TTF_CreateText(m_textEngine, m_font, text.c_str(), len);
TTF_GetTextSize(ttfText, &w, &h);
TTF_DrawRendererText(ttfText, x, y);
TTF_DestroyText(ttfText);

// After (通过 TextRenderer):
SSize size = getTextRenderer()->measureText(m_font.get(), text);
getTextRenderer()->drawText(m_font.get(), text, x, y, color);
```

### 6.4 迁移记录

| 文件 | 替换内容 |
|------|---------|
| `Label.h/cpp` | `TTF_Font*` → `SharedFont`；移除 `TTF_TextEngine*` / `TTF_Text*` 向量；`TTF_FontStyleFlags` → `int`；所有 SDL_ttf 调用 → TextRenderer 方法 |
| `EditBox.h/cpp` | `TTF_Font*` → `SharedFont`；移除 `TTF_TextEngine*` / `TTF_Text*` 对象；移除 `createTextEngine/createTextObjects/destroyTextObjects/recreateTextObjects`；`getTextEngine()` 移除；所有 SDL_ttf 调用 → TextRenderer 方法 |
| `TextArea.h/cpp` | 移除独立的 `TTF_TextEngine* m_textEngine`；所有 `TTF_CreateText/TTF_GetTextSize/TTF_DestroyText` → `measureText()`；`TTF_DrawRendererText` → `drawText()` |
| `LayoutParser.h/cpp` | `TTF_FontStyleFlags` → `int`；`TTF_STYLE_*` 常量 → 字面整数值 |
| `TextRenderer.cpp`（新增） | `SDL3Font` 包装 `TTF_Font*`；`SDL3TextRenderer` 包装 `TTF_TextEngine*`；factory 管理 `TTF_Init/TTF_Quit` 生命周期 |
| 所有 test 文件 | 移除 `TTF_Init()` / `TTF_Quit()` 调用 |

### 6.5 关键决策

- **`drawText()` 每次创建/销毁 TTF_Text**：Label 和 EditBox 每帧画 1-10 行，性能可接受。避免引入抽象的 TextObject 句柄层，大幅降低接口复杂度。
- **`getNativeHandle()` 桥接**：`RenderDevice` 暴露 `void* getNativeHandle()` 返回 `SDL_Renderer*`，供 SDL3TextRenderer 创建 `TTF_TextEngine`。仅后端实现需要。
- **TTF_Init/TTF_Quit 生命周期管理**：SDL3TextRenderer 的 factory `CreateSDL3TextRenderer` 调用 `TTF_Init()`，析构时调用 `TTF_Quit()`。测试文件不再需要手动管理。
- **Const-correctness 妥协**：`getTextWidth()`、`getCharWidth()`、`getLinePixelWidth()` 等方法改为非 const，因为 `getTextRenderer()` 实现需要上行遍历父控件。Label 的 `SSize getTextSize(const string&)` 配合 `measureText()` 天然 const，但 EditBox/TextArea 的测量方法关联到控件状态。

## 7. Phase 6——Window / Application / Event 抽象 + Backend Plugin 架构

### 7.1 背景：DLL/SO 分级分发模型

UICornerstone 采用**分级分发模型**——不同层级的模块有不同的开源/闭源策略：

```
┌───────────────────────────────────────────────────────────┐
│  用户应用程序 (开源或闭源)                                 │
│  MyApp.exe / libApp.so                                    │
│  #include <UICornerstone.h>  (仅抽象接口头文件 + API 头)      │
│  UICornerstone::initialize("sdl3");                          │
├───────────────────────────────────────────────────────────┤
│  UICornerstone.dll / libUICornerstone.so  ★★★ 闭源 ★★★         │
│  包含: ControlBase, Label, Button, EditBox, Menu,         │
│  LayoutEngine, EventQueue, Theme, Bench, WinFrame...      │
│  不包含任何后端图形库代码                                  │
│  不包含任何图形库头文件引用（如 SDL3/SDL.h）                │
│  通过抽象接口与后端交互                                     │
├───────────────────────────────────────────────────────────┤
│  Abstract Interface Headers  ★★★ 开源 ★★★                 │
│  include/RenderDevice.h   — RenderDevice, Texture, Surface│
│  include/TextRenderer.h   — TextRenderer                  │
│  include/Font.h           — Font                          │
│  include/Window.h         — Window (Phase 6 新增)          │
│  include/InputBackend.h   — InputBackend (Phase 6 新增)   │
│  include/BackendPlugin.h  — BackendAPI 结构体              │
│  include/EventTypes.h     — Event 类型定义                 │
│  ├── 这些头文件是 UICornerstone 公共 API 的一部分              │
│  ├── 不依赖任何后端图形库 (纯虚基类 + 值类型)               │
│  └── 第三方可用这些头文件开发新后端，无需 UICornerstone 源码    │
├──────────────────────┬──────────────────────┬─────────────┤
│  Backend_SDL3.dll/.so│  Backend_SFML.dll/.so│  ...        │
│  ★★★ 开源 ★★★         │  (第三方/社区)        │             │
│  实现所有抽象接口      │                      │             │
│  依赖 SDL3 头文件     │                      │             │
│  导出 extern "C" 工厂  │                      │             │
└──────────────────────┴──────────────────────┴─────────────┘
```

**分级原则：**

| 层级 | 内容 | 开源？ | 说明 |
|------|------|--------|------|
| **主框架 (UICornerstone.dll/.so)** | ControlBase, Label, Button, EditBox, Menu, LayoutEngine, EventQueue, Theme, Bench, WinFrame, LuotiAni, ... | ❌ 闭源 | 核心业务逻辑 — 控件树构建、布局计算、事件路由、动画引擎 |
| **抽象接口 (Public API Headers)** | RenderDevice.h, TextRenderer.h, Font.h, Window.h, InputBackend.h, BackendPlugin.h, EventTypes.h, SColor.h, Utility.h, ConstDef.h, Texture.h, Surface.h | ✅ 开源 | 所有纯虚基类 + 值类型 + 工厂函数签名 — 这是 UICornerstone 的 "Windows API" |
| **后端适配器** | SDL3 后端: SDL3Window, SDL3RenderDevice, SDL3TextRenderer, SDL3InputBackend | ✅ 开源 | 纯胶水代码，将抽象接口映射到具体图形库。可由社区维护 |

**这意味着**：
- 第三方可以为 UICornerstone 开发新后端（SFML, raylib, GLFW+Vulkan...），**只需要公开头文件，不需要 UICornerstone 源代码**
- 抽象接口是稳定的公共契约，一旦发布不能 breaking change（只能在 major 版本变更）
- 后端 DLL 与主 DLL 通过 C ABI 通信，不绑定编译器版本

| 约束 | 影响 |
|------|------|
| **跨 DLL 的 vtable 稳定性** | 所有抽象接口不可添加/移除虚方法到已发布的类；析构函数必须是 virtual 且 inline |
| **无 STL 跨 DLL 边界** | `std::string`、`vector`、`shared_ptr` 可作为接口参数（前提：同一 CRT），但 `std::any` 不行 |
| **工厂函数必须是 `extern "C"`** | 后端 DLL 用 C 链接导出创建函数，避免 C++ name mangling 跨编译器问题 |
| **单例 / 全局状态** | `MainWindow::getInstance()` 必须替换为抽象接口，因为主 DLL 不能引用 SDL 类型 |
| **事件系统类型安全** | 当前 `std::any m_eventParam` 无法跨 DLL 工作，必须替换为类型安全联合体 |
| **控制层零后端依赖** | 已有抽象接口 (RenderDevice, TextRenderer) 满足此要求；Window 和 Input 需新增 |

### 7.3 当前现状 (截至 Phase 13a)

**已就绪的抽象接口：**

| 接口 | 头文件 | 实现文件 (SDL3) | 实现文件 (SFML) | 跨 DLL 安全 |
|------|--------|----------------|----------------|------------|
| `Window` | `include/Window.h` | `src/backend/sdl3/Window.cpp` | `src/backend/sfml/Window.cpp` | ✅ 纯虚基类, 仅使用 SColor/SRect 等值类型 |
| `RenderDevice` | `include/RenderDevice.h` | `src/backend/sdl3/RenderDevice.cpp` | `src/backend/sfml/RenderDevice.cpp` | ✅ 同上 |
| `Texture` | `include/Texture.h` | `src/backend/sdl3/RenderDevice.cpp` | `src/backend/sfml/RenderDevice.cpp` | ✅ 同上 |
| `Surface` | `include/Surface.h` | `src/backend/sdl3/RenderDevice.cpp` | `src/backend/sfml/RenderDevice.cpp` | ✅ 同上 |
| `Font` | `include/Font.h` | `src/backend/sdl3/TextRenderer.cpp` | `src/backend/sfml/TextRenderer.cpp` | ✅ 同上 |
| `TextRenderer` | `include/TextRenderer.h` | `src/backend/sdl3/TextRenderer.cpp` | `src/backend/sfml/TextRenderer.cpp` | ✅ 同上 |
| `InputBackend` | `include/InputBackend.h` | `src/backend/sdl3/InputBackend.cpp` | `src/backend/sfml/InputBackend.cpp` | ✅ 同上 |
| `Cursor` | `include/Cursor.h` | `src/backend/sdl3/Cursor.cpp` | `src/backend/sfml/Cursor.cpp` | ✅ 同上 |
| `ResourceProvider` | `include/ResourceProvider.h` | `src/ResourceProvider.cpp` | `src/ResourceProvider.cpp` | ✅ 同上 |

**非后端 header 零 SDL 引用：** ✅ 已完成（ControlBase.h 已移除全部 SDL include 和 SDL 类型）

**已删除的旧模块：**
| 模块 | 替代 |
|------|------|
| `ResourceLoader.h/.cpp` | `ResourceProvider` + `ConstDef` |
| `src/RenderDevice.cpp` → `src/backend/sdl3/` | 后端源文件统一迁至 `src/backend/{backend}/` |
| `src/Cursor.cpp` → `src/backend/sdl3/` | 同上 |
| `src/InputBackend.cpp` → `src/backend/sdl3/` | 同上 |
| `src/TextRenderer.cpp` → `src/backend/sdl3/` | 同上 |
| `src/Window.cpp` → `src/backend/sdl3/` | 同上 |
| `src/BackendPlugin.cpp` → `src/BackendManager.cpp` | 重构为 BackendManager 单例 |

**仍需处理的遗留问题：**

| 问题 | 位置 | 说明 |
|------|------|------|
| `MainWindow` 仍持有具体后端对象的裸指针 | `src/MainWindow.cpp` | 当前为静态链接，尚未实现 DLL 动态加载 |
| ~~`Event::std::any m_eventParam`~~ | ~~`include/EventQueue.h`~~ | ✅ 已完全移除（旧字段已清理） |

### 7.4 接口设计

#### 7.4.1 Window 抽象

```cpp
// include/Window.h
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
    virtual void setTitle(const string& title) = 0;
};
```

#### 7.4.2 Application 抽象

```cpp
// include/Application.h
// 回调模式（类似 SDL3 callback main，适合 game loop）
class AppCallbacks {
public:
    virtual ~AppCallbacks() = default;
    virtual bool onInit() = 0;
    virtual void onEvent(const Event& event) = 0;
    virtual void onUpdate() = 0;
    virtual void onRender() = 0;
    virtual void onQuit() = 0;
};
```

#### 7.4.3 Event 系统重构（跨 DLL 安全版本）

```cpp
// include/EventTypes.h
struct MousePos     { float x, y; };
struct MouseWheel   { float x, y; float scrollX, scrollY; };
struct KeyEvent     { int keycode; int mod; bool repeat; };
struct TextInput    { char text[32]; };
struct ResizeEvent  { int width, height; };
struct FocusEvent   { bool focused; };

enum class EventType : uint8_t {
    None,
    MouseMove, MouseDown, MouseUp, MouseWheel,
    KeyDown, KeyUp, TextInput,
    WindowResize, WindowClose,
    FocusGained, FocusLost,
};

class Event {
public:
    EventType type;
    Control* target;  // 仅内部使用，跨 DLL 用 ID 或 handle
    union {
        MousePos   mousePos;
        MouseWheel mouseWheel;
        KeyEvent   keyEvent;
        TextInput  textInput;
        ResizeEvent resizeEvent;
        FocusEvent focusEvent;
    };
};
```

#### 7.4.4 InputBackend 抽象

```cpp
// include/InputBackend.h
class InputBackend {
public:
    virtual ~InputBackend() = default;
    virtual void startTextInput() = 0;     // 替代 SDL_StartTextInput
    virtual void stopTextInput() = 0;
    virtual bool isTextInputActive() const = 0;
    virtual void setClipboardText(const string& text) = 0;
    virtual string getClipboardText() const = 0;
    virtual bool hasScreenKeyboard() const = 0;
};
```

### 7.5 Backend Plugin 系统

#### 7.5.1 后端注册与加载

```cpp
// include/BackendPlugin.h

// 后端 DLL 必须导出的 C 接口
extern "C" {
    using CreateWindowFunc       = Window* (*)(const char* title, int w, int h);
    using CreateRenderDeviceFunc = RenderDevice* (*)(Window* window);
    using CreateTextRendererFunc = TextRenderer* (*)(RenderDevice* device);
    using CreateInputBackendFunc = InputBackend* (*)(Window* window);
    using DestroyBackendFunc     = void (*)();

    struct BackendAPI {
        unsigned version;
        CreateWindowFunc       createWindow;
        CreateRenderDeviceFunc createRenderDevice;
        CreateTextRendererFunc createTextRenderer;
        CreateInputBackendFunc createInputBackend;
        DestroyBackendFunc     destroy;
    };
}

// UICornerstone.dll 内部
class BackendManager {
public:
    static BackendManager* instance();

    bool load(const string& backendName);  // 加载 Backend_SDL3.dll 等
    void unload();

    Window* createWindow(const string& title, int w, int h);
    RenderDevice* createRenderDevice(Window* window);
    TextRenderer* createTextRenderer(RenderDevice* device);
    InputBackend* createInputBackend(Window* window);

private:
    void* m_moduleHandle = nullptr;   // LoadLibrary / dlopen
    BackendAPI m_api = {};
};
```

#### 7.5.2 后端实现的开放性

后端适配器（如 `Backend_SDL3`）**本身就是开源计划的一部分**——它们不包含 UICornerstone 核心逻辑，只是将抽象接口映射到具体图形库的"胶水代码"。社区可以：

1. 基于公开头文件开发新后端（SFML, raylib, GLFW+OpenGL, DirectX...）
2. 为现有后端贡献修复和优化
3. 独立于 UICornerstone 主版本发布后端更新

后端与主框架的契约就是 `BackendAPI` 结构体——任何满足该契约的 DLL 都可以被加载。

#### 7.5.3 后端 DLL 示例 (Backend_SDL3.dll)

```cpp
// Backend_SDL3.cpp —— 开源，编译为 Backend_SDL3.dll
#include <UICornerstone/Window.h>
#include <UICornerstone/RenderDevice.h>
// ...

class SDL3Window : public Window { /* ... */ };
class SDL3RenderDevice : public RenderDevice { /* ... */ };
class SDL3TextRenderer : public TextRenderer { /* ... */ };
class SDL3InputBackend : public InputBackend { /* ... */ };

extern "C" __declspec(dllexport)
BackendAPI* GetBackendAPI() {
    static BackendAPI api = {
        1,
        [](const char* title, int w, int h) -> Window* { return new SDL3Window(title, w, h); },
        [](Window* w) -> RenderDevice* { return new SDL3RenderDevice(static_cast<SDL3Window*>(w)->getRenderer()); },
        [](RenderDevice* d) -> TextRenderer* { return new SDL3TextRenderer(d); },
        [](Window* w) -> InputBackend* { return new SDL3InputBackend(static_cast<SDL3Window*>(w)->getWindow()); },
        []() { TTF_Quit(); SDL_Quit(); }
    };
    return &api;
}
```

### 7.6 迁移策略（从 MainWindow 单例到 BackendManager）

**第一步：提取抽象接口**

- 从 `MainWindow` 提取 `Window` 抽象到 `include/Window.h`
- 从 `MainWindow` 事件循环提取 `InputBackend` 抽象到 `include/InputBackend.h`
- 实现 `BackendManager` 类

**第二步：重建 MainWindow**

```cpp
// 新的 MainWindow 不再持有 SDL 类型，而是通过 BackendManager 获取：
class MainWindow {
    static MainWindow* instance();

    Window* getWindow() const;
    RenderDevice* getRenderDevice() const;
    TextRenderer* getTextRenderer() const;
    InputBackend* getInputBackend() const;

    bool init(const string& backend, const string& title, int w, int h);
    void shutdown();

    // 启动主循环
    int run(AppCallbacks* app);

private:
    unique_ptr<BackendManager> m_backend;
    Window* m_window = nullptr;
    RenderDevice* m_renderDevice = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    InputBackend* m_inputBackend = nullptr;
};
```

**第三步：迁移 MainWindow.cpp**

`MainWindow.cpp` 从直接调用 `SDL_CreateWindowAndRenderer` 改为：

```cpp
bool MainWindow::init(const string& backend, const string& title, int w, int h) {
    m_backend = make_unique<BackendManager>();
    if (!m_backend->load(backend)) return false;

    m_window = m_backend->createWindow(title, w, h);
    if (!m_window) return false;

    m_renderDevice = m_backend->createRenderDevice(m_window);
    m_textRenderer = m_backend->createTextRenderer(m_renderDevice);
    m_inputBackend = m_backend->createInputBackend(m_window);
    return true;
}
```

**第四步：主循环（运行时选择后端）**

```cpp
int MainWindow::run(AppCallbacks* app) {
    if (!app->onInit()) return 1;

    bool running = true;
    while (running) {
        Event event;
        while (m_inputBackend->pollEvent(event)) {
            if (event.type == EventType::WindowClose) running = false;
            EventQueue::getInstance()->pushEvent(event);
            app->onEvent(event);
        }
        app->onUpdate();
        app->onRender();
        m_renderDevice->present();
    }
    app->onQuit();
    return 0;
}
```

### 7.7 事件系统迁移

当前的 `Event` 类使用 `std::any m_eventParam`，需要迁移到新的 union 类型。

迁移要点：

| 旧 Event | 新 Event | 说明 |
|----------|---------|------|
| `EventName::MOUSE_LBUTTON_DOWN` | `EventType::MouseDown` | 枚举重命名，参数从 `shared_ptr<SPoint>` 变为 `mousePos` |
| `EventName::TEXT_INPUT` | `EventType::TextInput` | 参数从 `TextInputEventData` 变为 `textInput` |
| `EventName::KEY_DOWN` | `EventType::KeyDown` | 参数从 `KeyEventData` 变为 `keyEvent` |
| `m_eventParam = make_shared<SPoint>(x, y)` | `event.mousePos = {x, y}` | 值语义，无堆分配 |

**警告**：这是个破坏性变更，需要同时更新：
- `EventQueue.h/cpp` —— Event 类定义
- `EventQueue::pushEvent()` —— 存储方式（当前用 `shared_ptr<Event>`）
- 所有控件的 `handleEvent()` —— 访问方式
- 后端 DLL 中的输入转换逻辑

**建议方案**：保持向下兼容——在新 `Event` 中添加 `EventType m_type` 但不删除 `m_eventName` / `m_eventParam`，在一个开发周期内双轨运行，然后一次性清理。

### 7.8 工作量评估

| 子任务 | 文件数 | 预估工时 | 风险 |
|--------|--------|---------|------|
| Window 抽象接口 | ~3 (Window.h, SDL3Window.cpp, CMakeLists) | 0.5天 | ★ |
| InputBackend 抽象 | ~3 (InputBackend.h, SDL3InputBackend.cpp) | 0.5天 | ★★ |
| BackendManager + Plugin 加载 | ~3 (BackendPlugin.h, .cpp, CMakeLists) | 1天 | ★★★ |
| 重构 MainWindow | ~2 (MainWindow.h, .cpp) | 1天 | ★★★★ |
| Event 系统（union 重构 + 所有 handleEvent 迁移） | ~15 (EventQueue + 所有控件) | 2天 | ★★★★★ |
| 后端 DLL 构建系统 | CMakeLists | 1天 | ★★★ |
| **合计** | **~27** | **6天** | |

### 7.9 对已有代码的影响

- `ControlBase.h`：移除 `SDL_Renderer* m_renderer` 和 `setRenderer()`（不再需要，用 `RenderDevice*` 即可）
- `EditBox.cpp`：`EnsureTextInputStarted()` 改为 `GET_INPUTBACKEND->startTextInput()`
- 所有 `GET_RENDERDEVICE` → 不变（已是抽象接口）
- 所有 `GET_TEXTRENDERER` → 不变（已是抽象接口）
- `MainWindow.h`：移除 `SDL_Window*` / `SDL_Renderer*` 成员，改为 `Window*` / `RenderDevice*` / `BackendManager`
- 所有 test 文件：`MainWindow::getInstance()->init(...)` 改为 `MainWindow::getInstance()->init("sdl3", ...)`

## 10. Phase 7——TTF_Text\* 缓存（Label 层）

### 10.1 问题

Phase 5 引入 TextRenderer 后，`measureText()` 和 `drawText()` 每次调用都创建/销毁 `TTF_Text*` 对象。Label 每帧的 `computeLineOffsets()` + `draw()` 会产生 N×M 次 `TTF_CreateText/DestroyText`，在文本行数多时性能浪费严重，且 `TTF_Text*` 创建失败后后续 TTF 操作会挂死。

### 10.2 方案

在 Label 层缓存 `TTF_Text*`：

```cpp
// include/Label.h
std::vector<void*> m_cachedTexts;   // 每行一个
void releaseTexts();                // 销毁所有缓存对象
```

**缓存生命周期**：创建于 `Label::create()`，更新于 `computeLineOffsets()`（修改的行 recreate），销毁于 `~Label()`/`setCaption()`/`setFont()`/`resized()`/`recreate()`。

### 10.3 迁移记录

| 文件 | 变更 |
|------|------|
| `include/TextRenderer.h` | 新增 `createText()`/`destroyText()`/`measureText(void*)`/`drawText(void*)` 方法 |
| `src/TextRenderer.cpp` | 实现上述方法——包装 `TTF_CreateText/DestroyText/GetTextSize/DrawRendererText` |
| `include/Label.h` | 新增 `m_cachedTexts`、`releaseTexts()` |
| `src/Label.cpp` | 使用缓存文本对象而非每次创建 |

### 10.4 关键决策

- **TextRenderer 接口同步扩展**：新增 `void*` 句柄方法（`createText/destroyText`），而非在 Label 中直接依赖 TTF。TextRenderer 原有 `measureText(Font*, string)` / `drawText(Font*, ...)` 保留，供 EditBox/TextArea 等一次性使用。

## 11. Phase 8——ResourceProvider 抽象

### 11.1 动机

所有文件 I/O 操作通过 `ResourceLoader` 单例进行，该模块同时处理资源包加载和文件系统访问。添加纯文件系统资源（如动画 JSON）需要 `ResourceLoader` 提供桥接方法，架构耦合。此外，`Label`/`EditBox` 的 `loadFromResource()` 将字体数据暂存于局部 `vector<char>`，而 `TTF_OpenFontIO` 可能延迟引用数据——数据离开作用域后 font 引用野指针，导致崩溃。

### 11.2 设计方案

```cpp
// include/ResourceProvider.h
class ResourceProvider {
public:
    virtual ~ResourceProvider() = default;
    virtual shared_ptr<vector<char>> readFile(const string& path) = 0;
    virtual SDL_IOStream* openFileStream(const string& path) = 0;  // backend-specific
    virtual bool exists(const string& path) = 0;
};

ResourceProvider* CreateFilesystemResourceProvider(const string& basePath);
```

**集成方式**：像 RenderDevice/TextRenderer 一样通过 ControlBase/MainWindow 传播（`getResourceProvider()`/`setResourceProvider()` + `GET_RESOURCEPROVIDER` 宏）。

### 11.3 关键变更

| 变更 | 说明 |
|------|------|
| `ResourceProvider` 接口 + `FilesystemResourceProvider` | `include/ResourceProvider.h` / `src/ResourceProvider.cpp` |
| ControlBase 集成 | `m_resourceProvider` 成员 + getter/setter + 父子传播 |
| MainWindow 集成 | 创建 + `getResourceProvider()` + `GET_RESOURCEPROVIDER` 宏 |
| `Label::m_fontData` 生命周期修复 | `loadFromResource()` → 存储字体数据在 `m_fontData` 成员中 |
| `EditBox::m_fontData` 生命周期修复 | `loadFontInternal()` → 存储字体数据在 `m_fontData` 成员中 |
| Actor/LuotiAni 迁移 | `loadFromFile()`/`loadFromResource()` 使用 ResourceProvider |

## 12. Phase 9——移除 ResourceLoader

### 12.1 变更内容

ResourceLoader（584 行）作为旧资源包系统，在 ResourceProvider 上线后被彻底删除：

| 位置 | 变更 |
|------|------|
| `include/ResourceLoader.h` / `src/ResourceLoader.cpp` | **删除**（已从 CMakeLists 移除） |
| `ConstDef.h/cpp` | `FontName` 枚举和 `fontFiles` 映射从 ResourceLoader 迁入 |
| `src/WinFrame.cpp` | 内联 RID 路径常量（cross_up/cross_over/cross_down PNG）|
| `src/Bench.cpp` | 简化——加载瞬间完成，移除进度条绘制代码 |
| 所有 10 个测试文件 | 移除 `detachLoadingThread()` 调用 |

### 12.2 清理统计

- 删除 2 文件（ResourceLoader.h/.cpp），约 656 行
- 无需异步加载线程 → 控件/测试逻辑简化
- Bench 的进度条绘制不再需要（加载瞬间完成）

## 13. Phase 10——SDL Cursor 抽象

### 13.1 问题

`Label.h` 和 `WinFrame.h` 直接引用 `SDL_Cursor*` 并调用 `SDL_CreateSystemCursor`/`SDL_SetCursor`/`SDL_DestroyCursor`，引入 SDL 视频 API 依赖。

### 13.2 方案

```cpp
// include/Cursor.h
class Cursor {
public:
    enum class SystemCursorType {
        Arrow, IBeam, Wait, Crosshair, WaitArrow,
        SizeNWSE, SizeNESW, SizeWE, SizeNS, SizeAll, No, Hand, // ... 
    };
    static Cursor* createSystem(SystemCursorType type);
    static Cursor* getDefault();   // 静态单例
    static void setCurrent(Cursor* cursor);
};
```

**实现**：`SDLCursor` 包装 `SDL_Cursor*`（owned）或 `SDL_GetDefaultCursor()`（unowned）。`getDefault()` 返回静态单例，无需释放。

### 13.3 迁移记录

| 文件 | SDL 调用 | 替换 |
|------|---------|------|
| `src/Label.cpp` | `SDL_CreateSystemCursor`/`SDL_GetCursor`/`SDL_SetCursor`/`SDL_DestroyCursor` | `Cursor::createSystem`/`getDefault`/`setCurrent`/`delete` |
| `src/WinFrame.cpp` | 5 处光标创建/设置/销毁 | `Cursor` API |

## 14. Phase 11——移除 ControlBase SDL_Renderer

### 14.1 问题

Phase 2 引入 RenderDevice 后，`getRenderer()`/`setRenderer(SDL_Renderer*)` 在 ControlBase 中已是死代码——没有绘制代码再使用它。但它：

- 保留 `#include <SDL3/SDL_render.h>` → ControlBase.h 硬依赖 SDL 子头文件
- `setRenderer()` 内仍有 SDL3 引用（`SDL_Renderer*` 传参）
- 占用 `m_renderer` 成员，每个控件 8 字节

### 14.2 删除清单

| 位置 | 移除内容 |
|------|---------|
| `include/ControlBase.h` | `#include <SDL3/SDL_render.h>`；`SDL_Renderer* getRenderer/setRenderer` 接口；`SDL_Renderer* m_renderer` 成员 |
| `src/ControlBase.cpp` | `getRenderer()`/`setRenderer()` 实现（8+25 行）；`m_renderer` 从 `addControl()`/`inheritRenderer()` 传播中移除；从构造/拷贝/赋值中移除 |
| `include/MainWindow.h` | `#define GET_RENDERER` 宏 |
| `include/ConstDef.h` | `SDL_WINDOWPOS_CENTERED`/`SDL_WINDOW_RESIZABLE`/`SDL_WINDOW_HIGH_PIXEL_DENSITY` → hex 字面量 |
| `include/EditBox.h` | `#include <SDL3/SDL_keyboard.h>`（未使用） |
| `include/Bench.h` | `drawCenteredRectangle(SDL_Renderer*)` 方法 |
| `test/test_label.cpp` | `BENCH->getRenderer()` → `MAINWIN->getRenderer()` |

### 14.3 结果

`ControlBase.h` 不再有任何直接 SDL include 或 SDL 类型在其 public API 中——达成"控制层零 SDL 依赖"目标的里程碑。

## 14a. Phase 12——事件系统迁移（Controls → Union API）

### 14a.1 背景

Phase 6 在 `Event` 类中引入了新的 union 字段（`EventType m_type` + `mousePos/mouseButton/keyEvent/textInput` 等），同时保留了旧 API 字段（`EventName m_eventName` + `std::any m_eventParam`）。Phase 6 仅完成了基础设施（`InputBackend` 双轨推送、`EventTypes.h` 定义），所有控件的 `handleEvent()` 仍使用旧 API。

### 14a.2 迁移范围

| 控件 | 旧 API 模式 | 新 API 模式 |
|------|-----------|-----------|
| EditBox | `any_cast<shared_ptr<SPoint>>` × 3 + `any_cast<KeyEventData>` × 2 + `any_cast<TextInputEventData>` | union 字段直接读取，零 any_cast |
| TextArea | 同上，~12 处 any_cast（含委托 ScrollBar） | union 字段 + m_type switch |
| ScrollBar | `any_cast<shared_ptr<SPoint>>` × 2 | union 字段 |
| Button | `isPositionEvent` + any_cast + EventName switch | EventType switch + FINGER fallback |
| Label | 同上 | 同上 |
| CheckBox | `isPositionEvent` + any_cast | EventType switch |
| WinFrame | `hasPos` guard + ~12 处 EventName 比较 | EventType + mousePos/mouseButton |
| Menu (3 classes) | `isPositionEvent` + any_cast + EventName switch | EventType switch |

### 14a.3 关键设计

1. **双轨并存**：旧 `Event(EventName, std::any)` 构造自动设置 `m_type` 和 union 字段（通过 `EventQueue.h` 中的构造体），无需修改 46 处事件创建点
2. **FINGER 事件保留旧 API**：`FINGER_DOWN/UP/MOTION` 无 `EventType` 等价物，Button/Label 保留 `isPositionEvent` fallback
3. **旧数据结构的统一**：`KeyEventData/TextInputEventData/FocusEventData/MouseWheelEventData` 从 `EditBox.h/TextArea.h` 移到 `EventTypes.h`
4. **EventQueue `noexcept`**：watcher 路由仍使用 `m_eventName`，`handleEvent()` 分发无过滤——各控件自己检查 `m_type`

### 14a.4 迁移记录

| 文件 | 修改内容 |
|------|---------|
| `include/StateMachine.h` | `Event(EventName,any)` 构造声明移至类外 → `EventQueue.h` 实现 |
| `include/EventQueue.h` | 构造体：EventName→EventType 映射 + union 字段填充 |
| `include/EventTypes.h` | 移入 `KeyEventData/TextInputEventData/FocusEventData/MouseWheelEventData`；添加 `<string>` include |
| `include/EditBox.h` | 删除重复数据结构定义（已移至 EventTypes.h） |
| `include/TextArea.h` | 删除 `MouseWheelEventData`（已移至 EventTypes.h） |
| `src/EditBox.cpp` | 7 处 any_cast 移除 → union 字段 |
| `src/TextArea.cpp` | ~12 处 any_cast 移除 → union 字段 |
| `src/ScrollBar.cpp` | 3 处旧 API → union 字段 |
| `src/CheckBox.cpp` | isPositionEvent → EventType switch |
| `src/Button.cpp` | isPositionEvent + EventName switch → EventType switch + FINGER fallback |
| `src/Label.cpp` | 同上 |
| `src/Menu.cpp` | MenuItem/MenuPanel/MenuBar 全部转换 |
| `src/WinFrame.cpp` | hasPos 逻辑 + 全部 EventName 比较 → EventType |

### 14a.5 结果

所有 10 测试编译通过、运行时事件响应正常。`std::any_cast` 和 `try/catch` 完全从控件事件处理中移除。✅ EventQueue watcher 路由已迁移至 `EventType`（不再使用 `m_eventName`）。

## 15. Phase 13——SFML 后端实现

### 15.1 实现内容

| 抽象接口 | SFML 实现 | 代码量 |
|---------|-----------|--------|
| `RenderDevice` | 包装 `sf::RenderWindow` / `sf::Image` | ~514 行 |
| `Texture` | 包装 `sf::Texture`（含 POT 兼容） | 同上 |
| `Surface` | 使用 `sf::Image` + nanosvg SVG 栅格化 | 同上 |
| `TextRenderer` | 使用 `sf::Text` + 全局缓存（keyed by 文本+字号） | ~250 行 |
| `Window` | 包装 `sf::RenderWindow` + sf::View 视口适配 | ~96 行 |
| `InputBackend` | SFML 事件轮询 + 剪贴板/文本输入 | ~310 行 |
| `Cursor` | SFML 系统光标包装 | ~67 行 |
| `BackendPlugin` | 后端工厂函数表 | ~46 行 |
| **总计** | **9 文件** | **~1283 行** |

### 15.2 关键差异与解决方案

| 差异 | 解决方案 |
|------|---------|
| SFML 无 `SDL_RenderFillRect` 等效 API | 使用 `sf::VertexArray(sf::Triangles)` 批量提交矩形填充（6 顶点/矩形），`present()` 时一次性绘制 |
| SFML 无批量几何绘制 | 使用 `sf::VertexArray(sf::Triangles)` 收集所有三角形/四边形顶点，`present()` 时合并为单次 `glDrawArrays` |
| SFML 不支持 SVG 加载 | 嵌入 nanosvg（来自 SDL3_image 源码）检测 SVG 魔数自动栅格化 |
| SFML v3 API 变更 | `sf::FloatRect(0,0,w,h)` → `sf::FloatRect({0,0},{w,h})`（两个 Vector2 参数）；`sf::VideoMode` 使用 `Vector2u` |
| SFML 无内建裁剪区域 | 用 `sf::Viewport` / `sf::Glsl::Vec4` scissor 模拟 |
| PDB 锁冲突（MSVC 并行编译） | CMake 全局 `/FS` 编译选项 + `/m:1` 单进程编译 |

### 15.3 已知问题

无已知不一致问题。

### 15.4 性能优化（批量顶点提交）

**问题**：原始 SFML 后端使用 `sf::RectangleShape` / `sf::ConvexShape` / `sf::Quads` 等逐形状 API，每个形状独立 OpenGL draw call，开销极高。

**方案**：将所有简单图元合并到两个 `sf::VertexArray` 批处理缓冲区中：

| 缓冲区 | 图元类型 | 收集的绘制操作 |
|--------|---------|--------------|
| `m_fillBatch` | `sf::PrimitiveType::Triangles` | `fillRect`（2 三角形/矩形）、`drawTriangle`、`drawQuad`、`drawTriangles`、`drawTriangleStrip`、`drawTriangleFan` |
| `m_lineBatch` | `sf::PrimitiveType::Lines` | `drawRect` 轮廓（4 线段/矩形）、`drawLine` |

**刷新策略**：

- `present()` / `clear()` / `drawTexture()` / `setClipRect()` / `setRenderTarget()` / `readPixels()` — 强制刷新批次
- `drawText()` — 通过 `RenderDevice::flush()` 检查 `m_batchDirty` 标记，仅在有挂起顶点时刷新
- `flush()` 使用布尔脏标记避免空批次时的虚拟函数开销

**优化效果**：

| 操作 | 优化前 | 优化后 |
|------|--------|--------|
| 矩形填充 | N × `sf::RectangleShape` → N 个 draw call | 1 次 `glDrawArrays`（三角形） |
| 矩形轮廓 | 4N × `sf::Vertex{2}` → 4N 个 draw call | 1 次 `glDrawArrays`（线段） |
| 三角形/四边形 | 逐个 `sf::ConvexShape` | 合并到三角形批次 |

**帧渲染顺序保证**：`drawText()` 前调用 `flush()`，确保背景 `fillRect` 在文字绘制前已提交，避免 Z 顺序颠倒。

### 15.5 已修复问题

| 问题 | 根因 | 修复 |
|------|------|------|
| **所有文字不显示**（8 测试） | 批处理延迟渲染导致背景 `fillRect` 在 `present()` 时覆盖文字 | `drawText` 前调用 `RenderDevice::flush()` |
| **Ctrl-C 变成 Ctrl-X** | SFML 为 Ctrl+字母生成 `TextEntered` 事件（如 `\x03`），TextArea 未过滤控制字符；键盘修饰符使用轮询而非事件字段 | 过滤控制字符 + 使用 `keyPressed->control` 事件字段 |
| **中文编码损坏**（`drawText(void*, wrapWidth)`） | `sf::String::toAnsiString()` 丢失非 ASCII 字符 | 改为 `toUtf8()` |
| **SDL3 graphtool 第3组黑色黑块**（SFML 无） | SDL3 后端未设置默认 blend mode，`SDL_BLENDMODE_NONE` 下 `alpha=0` 绘制为实心黑色 | `SDL3RenderDevice` 构造函数设置 `SDL_BLENDMODE_BLEND`，使透明填充行为与 SFML 一致 |

## 16. Phase 14——raylib 后端实现

### 16.1 预备工作

#### 16.1.1 已有基础设施

- `subModules/raylib/` 子模块已存在，含 **raylib v6.0**（`include/raylib.h`、`lib/raylib.lib`、`lib/raylib.dll`）
- `src/backend/raylib/` 目录已创建（**空**）
- `CMakeLists.txt` 已有 `_BACKEND_UPPER STREQUAL "RAYLIB"` 分支，链接 `subModules/raylib/lib/raylib.lib`
- `BackendManager.cpp` 已有 `#ifdef UICORNERSTONE_BACKEND_RAYLIB` 分支（在 Phase 6/13 已预留）

#### 16.1.2 所需新建文件

| 文件 | 职责 | 预计代码量 | 参照模板 |
|------|------|-----------|---------|
| `src/backend/raylib/RenderDevice.cpp` | 包装 raylib 绘图、纹理、Surface、RenderTexture | ~400 行 | `sfml/RenderDevice.cpp` (~520 行) |
| `src/backend/raylib/TextRenderer.cpp` | 包装 raylib 字体加载/文本测量/文本绘制 | ~200 行 | `sfml/TextRenderer.cpp` (~250 行) |
| `src/backend/raylib/Window.cpp` | 包装 InitWindow/CloseWindow/输入轮询 | ~130 行 | `sfml/Window.cpp` (~96 行) |
| `src/backend/raylib/InputBackend.cpp` | 包装 raylib 输入查询 API 为事件轮询模型 | ~350 行 | `sfml/InputBackend.cpp` (~310 行) |
| `src/backend/raylib/Cursor.cpp` | SystemCursorType 到 raylib MouseCursor 映射 | ~50 行 | `sfml/Cursor.cpp` (~67 行) |
| `src/backend/raylib/BackendPlugin.cpp` | 工厂函数表注册 | ~30 行 | `sfml/BackendPlugin.cpp` (~46 行) |
| `src/backend/raylib/Surface.cpp` | SVG 栅格化（nanosvg）适配 | ~60 行 | 复用已有 nanosvg 方案 |
| **总计** | | **~1220 行** | |

### 16.2 核心关注点：raylib 与 SDL3/SFML 的关键差异

#### 16.2.1 渲染模型

| 特性 | SDL3 | SFML | raylib |
|------|------|------|--------|
| 绘图 API 风格 | 保留模式（`SDL_RenderFillRect` 等） | OOP（`sf::RectangleShape`） | **立即模式**（`DrawRectangleRec` 等） |
| 帧模型 | 手动 `BeginFrame` / `Present` | `window.clear()` / `window.display()` | `BeginDrawing()` / `EndDrawing()` |
| 输入轮询 | `SDL_PollEvent()` → 事件驱动 | `window.pollEvent()` → 事件驱动 | **帧末轮询**：`PollInputEvents()` 在 `EndDrawing()` 内部调用 |
| 内建批处理 | SDL3 内部自动批处理 | 无（需手动 `sf::VertexArray` 批处理） | raylib 内部通过 `rlgl` 自动批处理 |
| 裁剪 | `SDL_RenderClipRect` | 需手动 scissor 或 viewport | `BeginScissorMode/EndScissorMode` |

**结论**：raylib 的绘图 API 最接近我们的抽象层需求——几乎所有 `RenderDevice` 方法都有直接的 raylib 1:1 映射。**不需要 SFML 风格的顶点批处理**，raylib 内部的 `rlgl` 批处理已经足够高效。

#### 16.2.2 接口映射

##### RenderDevice

| RenderDevice 方法 | raylib 对应 API | 备注 |
|------------------|----------------|------|
| `setDrawColor(SColor)` | `color = rlColorFromSColor(s)` | raylib 不使用全局 draw color；每个绘图函数直接接受 `Color` 参数 |
| `setBlendMode(BlendMode)` | `BeginBlendMode(BLEND_ALPHA/ADDITIVE/MULTIPLIED)` | raylib 有 `BLEND_ALPHA`、`BLEND_ADDITIVE`、`BLEND_MULTIPLIED`、`BLEND_ADD_COLORS`、`BLEND_SUBTRACT_COLORS`、`BLEND_CUSTOM` |
| `setClipRect(SRect)` | `BeginScissorMode(x, y, w, h)` | 注意 raylib 的 scissor 坐标是屏幕坐标系（左下角为原点），需翻转 Y |
| `clearClipRect()` | `EndScissorMode()` | |
| `fillRect(SRect)` | `DrawRectangleRec(Rectangle, Color)` | 直接映射 |
| `drawRect(SRect)` | `DrawRectangleLinesEx(Rectangle, 1.0f, Color)` | raylib 的 `DrawRectangleLines` 总是 1px；`LinesEx` 可设线宽 |
| `drawLine(x1,y1,x2,y2)` | `DrawLine(x1,y1,x2,y2,Color)` | |
| `drawPoint(x,y)` | `DrawPixel(x,y,Color)` | |
| `drawTriangles(vertices, count)` | 需使用 `rlgl`：`rlBegin(RL_TRIANGLES)` ... `rlEnd()` | raylib 无直接三角形顶点数组 API，需要通过 `rlgl` 提交 |
| `drawTriangleStrip/Fan` | 同理 `rlBegin(RL_TRIANGLE_STRIP/FAN)` | |
| `drawTriangle(x0,y0,...,color)` | `DrawTriangle(Vector2 v1, v2, v3, Color)` | 直接映射 |
| `drawQuad(...)` | 4 顶点无内置 quad 函数，可用 `rlBegin(RL_QUADS)` 或两个 `DrawTriangle` | |
| `clear()` | `ClearBackground(Color)` | |
| `present()` | `EndDrawing()` | **关键**：`EndDrawing()` 内部执行 `SwapBuffers()` + `PollInputEvents()` |
| `createTextureFromFile(path)` | `LoadTexture(path)` → `Texture2D` | |
| `createTextureFromSurface(surface)` | `LoadTextureFromImage(Image)` | raylib 的 `Image` ↔ `Texture2D` 转换非常直接 |
| `createRenderTexture(w, h)` | `LoadRenderTexture(w, h)` → `RenderTexture2D` | |
| `destroyTexture(texture)` | `UnloadTexture(Texture2D)` | |
| `drawTexture(texture, src, dst)` | `DrawTexturePro(tex, src, dst, origin, rotation, tint)` | raylib 的 `DrawTexturePro` 功能完整，连旋转都内建 |
| `drawTextureRotated(...)` | 同样是 `DrawTexturePro`（传入 rotation 参数） | |
| `setRenderTarget(texture)` | `BeginTextureMode(RenderTexture2D)` | raylib 只支持 `RenderTexture2D` 作为渲染目标，普通 `Texture2D` 不行 |
| `resetRenderTarget()` | `EndTextureMode()` | |
| `readPixels(buffer, rect)` | `ReadTexturePixels(Texture2D)` 或 `LoadImageFromScreen()` | 需注意格式转换（RGBA bytes） |
| `getNativeHandle()` | 返回 `&m_window`（窗口句柄）或 nullptr | |

**关键问题**：`drawTriangles`/`drawTriangleStrip`/`drawTriangleFan` 需要 `rlgl` 底层 API。raylib 公开了 `rlgl.h`（已包含在子模块中），可以使用 `rlBegin(RL_TRIANGLES)` / `rlColor4f()` / `rlVertex2f()` / `rlEnd()`。这与 SDL3 的 `SDL_RenderGeometry` 在结构上非常接近。

##### Surface（Image 适配）

| Surface 方法 | raylib 对应 API | 备注 |
|-------------|----------------|------|
| `static create(w, h)` | `GenImageColor(w, h, BLANK)` | |
| `static loadFromFile(path)` | `LoadImage(path)` | |
| `static loadFromMemory(data, len)` | `LoadImageFromMemory(fileType, data, size)` | SVG 需要 nanosvg（同 SFML） |
| `width()` / `height()` | `image.width` / `image.height` | 直接字段访问 |
| `pixels()` | `image.data` | |
| `getPixel(x, y)` | `GetImageColor(image, x, y)` → 转 uint32 | |
| `setPixel(x, y, pixel)` | `ImageDrawPixel(&image, x, y, color)` | |
| `setBlendMode(mode)` | 无 CPU-side blend，可忽略（Surface 是 CPU 像素缓冲区） | |
| `setAlphaMod(alpha)` | 无等效，可通过 `ImageColorTint` 模拟 | |
| `blit(src, ...)` | `ImageDraw(&dst, src, srcRec, dstRec, tint)` | raylib 5.0+ 的 `ImageDraw` 支持带缩放的 blit |
| `createTexture(device)` | `LoadTextureFromImage(image)` | |
| `rotate(angle, device)` | `ImageRotateCW/ImageRotateCCW`（仅支持 90° 步进）或 GPU 旋转 | 任意角度需 GPU render-to-texture（同 SFML 方案） |

**SVG 支持**：raylib 不原生支持 SVG。方案与 SFML 相同——在 `Surface::loadFromMemory` 中检测 SVG 魔数，用 nanosvg 栅格化为 RGBA `Image`。

##### Font / TextRenderer

| TextRenderer 方法 | raylib 对应 API | 备注 |
|------------------|----------------|------|
| `loadFont(path, size)` | `LoadFontEx(path, size, NULL, 0)` | raylib 在加载 TTF 时自动生成字型图集（stb_truetype） |
| `loadFontFromMemory(data, len, size)` | `LoadFontFromMemory(".ttf", data, size, fontSize, NULL, 0)` | |
| `getFontHeight(font)` | `font.baseSize` 或通过度量计算 | raylib 的 `Font.baseSize` 是加载时指定的字号 |
| `createText(font, text)` | 无等效缓存对象；直接使用 `(Font, text)` 对 | **回调测文本**或缓存为 `(Font, string, fontSize)` 对 |
| `destroyText(text)` | 无操作（raylib 无 TTF_Text* 等效） | |
| `measureText(text)` | `MeasureTextEx(font, str, fontSize, spacing)` | raylib 返回 `Vector2` |
| `drawText(text, x, y, color)` | `DrawTextEx(font, str, pos, fontSize, spacing, tint)` | |
| `drawText(text, x, y, wrapWidth, color)` | 需手动换行：`MeasureTextEx` 按单词测量，超宽换行，逐行 `DrawTextEx` | **核心复杂度**：Label.cpp 的自动换行逻辑需适配 |

**关键差异**：
1. raylib 的 `Font` 结构体包含 `Texture2D texture`（字型图集）+ `GlyphInfo* glyphs` 数组 + `baseSize`。与 SDL3_ttf 的 `TTF_Font*` 完全不同。
2. 没有 `TTF_Text*` 这样的缓存对象——raylib 的 `DrawTextEx` 每次调用都会重新遍历字型。这相当于 SDL3 的旧方案（每次绘制时创建临时 `TTF_Text*`）。
3. 如果要做 `createText`/`destroyText` 缓存，需要自己维护 `(Font*, string, fontSize, spacing) → (glyph positions, texture coords)` 缓存。这对于性能优化很重要，但不是实现正确性的必要条件——可以先实现基本版本（每次绘制时测量+绘制），后续再优化。

**最简实现路径**：
- `createText(font, text)` → 分配一个 `CachedText` 结构体，存储 `Font*` + `string` + `fontSize`。
- `measureText(text)` → 调用 `MeasureTextEx(font, str.c_str(), fontSize, 1.0f)`。
- `drawText(text, x, y, color)` → 调用 `DrawTextEx(font, str.c_str(), {x, y}, fontSize, 1.0f, color)`。
- `drawText(text, x, y, wrapWidth, color)` → 按单词拆分，计算换行，逐行 `DrawTextEx`。

##### Window

| Window 方法 | raylib 对应 API | 备注 |
|------------|----------------|------|
| `getSize()` | `(GetScreenWidth(), GetScreenHeight())` | |
| `getPosition()` | `GetWindowPosition()` → `Vector2` | |
| `getDisplayWidth/Height()` | `GetMonitorWidth(GetCurrentMonitor())` / `GetMonitorHeight(...)` | |
| `getDpiScale()` | `GetWindowScaleDPI()` → `Vector2`，取 X 分量 | |
| `setTitle(title)` | `SetWindowTitle(title)` | |
| `nativeHandle()` | 返回 `nullptr`（raylib 内部封装 GLFW，不暴露原生句柄） | |
| `renderDevice()` | 返回 `m_renderDevice`（我们的 RaylibRenderDevice） | |
| `getMousePosition(x, y)` | `GetMousePosition()` → `Vector2` | |
| `onResized(w, h)` | `IsWindowResized()` 帧末检测 → 自动调用 | raylib 的 resize 检测只有一帧有效 |
| 窗口初始化 | `SetConfigFlags()` → `InitWindow(w, h, title)` → `SetTargetFPS(60)` | |
| 帧循环 | `WindowShouldClose()` → 在 run() 循环中使用 | |

**关键差异**：raylib 的窗口生命周期是**全局函数式**而非对象式。`InitWindow` 只能调用一次，`CloseWindow` 只能调用一次。我们需要在 `RaylibWindow` 的构造函数/析构函数中调用。这与 SDL3（窗口对象）和 SFML（`sf::RenderWindow` 对象）都不同。

##### InputBackend

InputBackend 的 `pollEvent(Event&)` 将 raylib 的**状态查询 API**转换为事件驱动的模型：

| EventType | raylib 检测方法 | 备注 |
|-----------|----------------|------|
| MouseMove | `GetMousePosition()` 与上一帧比较 | 每次 poll 检查 |
| MouseDown | `IsMouseButtonPressed(btn)` | raylib 的 `Pressed` 语义是"首次按下" |
| MouseUp | `IsMouseButtonReleased(btn)` | |
| MouseWheel | `GetMouseWheelMoveV()` | `Vector2` 返回（x: 水平, y: 垂直） |
| KeyDown | `IsKeyPressed(key)` | raylib 的 `Pressed` 语义是"首次按下" |
| KeyUp | `IsKeyReleased(key)` | |
| TextInput | `GetCharPressed()` → unicode codepoint | 循环调用直到返回 0 |
| WindowResize | `IsWindowResized()` | 只有一帧为 true |
| WindowClose | `WindowShouldClose()` | 在外部循环检测 |
| FocusGained/Lost | `IsWindowFocused()` | |

**关键问题**：raylib 的输入是在 `EndDrawing()` 内部的 `PollInputEvents()` 中统一采集的。这意味着：
1. `PollInputEvents()` 在 `EndDrawing()` 中被自动调用
2. 输入状态在帧之间保持（`IsKeyPressed` 在当前帧返回 true，下一帧返回 false）
3. **我们的 `pollEvent()` 必须在 `EndDrawing()` 之后调用**才能获取到该帧的输入状态

这导致一个根本性的架构差异：

```
SDL3/SFML 模式:
  while (running) {
    while (pollEvent(event)) { handleEvent(event); }  // 逐事件处理
    update();
    render();
    present();
  }

raylib 模式:
  while (!WindowShouldClose()) {
    update();
    render();
    // EndDrawing() 内部: PollInputEvents() + SwapBuffers()
    // 然后我们轮询输入状态 → 生成事件队列 → 下一帧处理
  }
```

简单但不完全准确的简化方案：
```
run() 循环中:
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground();
    draw();  // 使用上一帧的事件
    EndDrawing();   // 内部 PollInputEvents
    pollAndQueueEvents(); // 生成下一帧要用的事件
    processEvents(); // 分发给控件
    update();
  }
```

但更干净的做法是**双缓冲事件**：`EndDrawing()` 后读取所有输入状态，填入事件队列，下一帧 `update()` 前消费。或者更直接——在 `Window::run()` 的循环中手动调用 `PollInputEvents()`，在 `EndDrawing()` 之前/之后插入事件处理。

实际上最简单的方案（来自 SDL3/SFML 经验）是：
```
while (!WindowShouldClose()) {
    // 步骤 1: 轮询输入 → 生成事件（在 EndDrawing 之后，因为这时的输入状态是完整的）
    pollInputEvents();    // 手动调用 raylib 的 PollInputEvents
    queueEvents();        // 将 IsKeyPressed/IsMouseButtonPressed 等转为 Event 队列
    
    // 步骤 2: 处理事件（分发给控件）
    processEvents();
    
    // 步骤 3: 更新
    update();
    
    // 步骤 4: 渲染
    BeginDrawing();
    ClearBackground();
    render();
    EndDrawing();   // 这会自动调用 PollInputEvents 但我们已经手动调用了
}
```

但注意 `BeginDrawing`/`EndDrawing` 内部也会调用 `PollInputEvents`，会导致重复处理。解决：

1. 设置 `DisableEventWaiting()` 来禁止 `EndDrawing()` 中的 `PollInputEvents()`
2. 或者使用 `EnableEventWaiting(false)` 等效
3. 手动在帧开始处调用 `PollInputEvents()`，然后处理事件，接着绘制

查阅 raylib API：
- `EnableEventWaiting()` / `DisableEventWaiting()` — 控制 `EndDrawing()` 是否等待事件

DisableEventWaiting 会让 `EndDrawing()` 不进行事件轮询。那我就可以在绘制前手动轮询：

```
while (!WindowShouldClose()) {
    // 手动轮询输入事件
    PollInputEvents();  // raylib 内部函数？不公开...
    
    // 不行，PollInputEvents 是内部函数
}
```

实际上，看 raylib 的代码，`PollInputEvents()` 由 `EndDrawing()` 自动调用，但我们可以把事件处理放在 `BeginDrawing()` 之前，在上一帧的 `EndDrawing()` 已经采集了输入状态：

```
while (!WindowShouldClose()) {
    // 此时输入状态已经由上一帧的 EndDrawing 更新
    processEvents();   // 处理 IsKeyPressed 等状态
    
    update();
    
    BeginDrawing();
    ClearBackground();
    render();
    EndDrawing();     // 内部 PollInputEvents → 更新输入状态
}
```

但有一个问题：第一帧时还没有调用过 EndDrawing，所以输入状态是初始值。

更好的方案：在循环开始前先调用一次假绘制：

```
BeginDrawing();
EndDrawing();  // 首次采集输入
while (!WindowShouldClose()) {
    processEvents();
    update();
    BeginDrawing();
    ClearBackground();
    render();
    EndDrawing();
}
```

或者更干净——在 `Window::init()` 中调用 `InitWindow` 后，立即 `BeginDrawing(); EndDrawing();` 一次。

**架构决策**：采用"帧初处理事件"模式：
- 第一次进入 `run()` 时，先 `BeginDrawing(); EndDrawing();` 空帧以初始化输入状态
- 主循环中：`ProcessEvents() → Update() → BeginDrawing() → Render() → EndDrawing()`
- `ProcessEvents()` 内部调用 `IsKeyPressed`、`GetMousePosition` 等查询当前帧状态，生成 Event 对象

##### Cursor

| SystemCursorType | raylib MouseCursor | 备注 |
|-----------------|-------------------|------|
| Default | `MOUSE_CURSOR_DEFAULT` | |
| Text | `MOUSE_CURSOR_IBEAM` | |
| Pointer | `MOUSE_CURSOR_POINTING_HAND` | |
| Crosshair | `MOUSE_CURSOR_CROSSHAIR` | |
| EW_Resize | `MOUSE_CURSOR_RESIZE_EW` | |
| NS_Resize | `MOUSE_CURSOR_RESIZE_NS` | |
| NWSE_Resize | `MOUSE_CURSOR_RESIZE_NWSE` | |
| NESW_Resize | `MOUSE_CURSOR_RESIZE_NESW` | |
| Move | `MOUSE_CURSOR_RESIZE_ALL` | |
| NotAllowed | `MOUSE_CURSOR_NOT_ALLOWED` | |
| Wait | 无直接等效 → `MOUSE_CURSOR_DEFAULT` | |
| Progress | 无直接等效 → `MOUSE_CURSOR_DEFAULT` | |
| NW/N/NE/E/SE/S/SW/W_Resize | 无直接等效 → 对应 Resize_ALL | raylib 只有 4/8 方向，没有单个方向 |
| Arrow | `MOUSE_CURSOR_ARROW` | |
| **10/20 类型有直接映射** | | 其余 10 种 fallback 到 DEFAULT |

`RaylibCursor::createSystem(type)` 调用 `SetMouseCursor(raylibType)` 立即生效（raylib 是全局的）。返回一个简单的包装对象。

##### BackendPlugin

按照已有模式（SDL3/SFML 的 `BackendPlugin.cpp`），注册：
- `CreateRaylibWindow(title, w, h, flags)`
- `CreateRaylibInputBackend(window)`
- raylib 不需要显式的 `Renderer*` 或 `TextRenderer*` 创建（因为它们是全局的或从 Window 获取）

### 16.3 系统就绪状态检查表

| 检查项 | 状态 | 备注 |
|--------|------|------|
| raylib 子模块 | ✅ `subModules/raylib/` 含 `v6.0`，`include/raylib.h` + `.lib` + `.dll` |
| CMake 配置 | ✅ 已有 `RAYLIB` 分支，链接 `raylib.lib` |
| 后端目录 | ✅ `src/backend/raylib/` 已创建（空） |
| BackendManager | ✅ 已有 `#ifdef UICORNERSTONE_BACKEND_RAYLIB` |
| 工厂函数声明 | ✅ 已在各自 `.cpp` 文件中实现（同 SFML 模式，通过 BackendPlugin 本地声明 + 调用） |
| nanosvg | ✅ 已在 `src/backend/raylib/RenderDevice.cpp` 中集成（复用 SFML 后端的 nanosvg 方案） |
| 测试适配 | ✅ 已创建 `build_raylib.bat`（同 `build_sdl3.bat` / `build_sfml.bat`） |

### 16.4 实施步骤（12 步，按依赖排序）

每个步骤完成后都应能编译通过（即使运行时功能不完整），确保早期发现集成问题。

#### 子阶段 A：骨架（可编译，不可运行）

| 步骤 | 内容 | 文件 | 估计行数 | 验证方式 |
|------|------|------|---------|---------|
| **A1** | 添加 raylib 后端工厂函数声明到公共头文件 | `include/Window.h`（+`CreateRaylibWindow`）、`include/InputBackend.h`（+`CreateRaylibInputBackend`）、`include/TextRenderer.h`（+`CreateRaylibTextRenderer`）、`include/Cursor.h`（即可，raylib 使用静态方法） | ~10 | 编译通过 |
| **A2** | 编写 BackendPlugin.cpp：工厂函数表 + `RaylibBackend` 注册 | `src/backend/raylib/BackendPlugin.cpp` | ~30 | `build_raylib.bat test_label` 链接成功 |
| **A3** | 编写 Window.cpp 框架：`InitWindow/CloseWindow/SetTargetFPS`、`getSize/getPosition`、`renderDevice()` 返回 nullptr | `src/backend/raylib/Window.cpp` | ~80 | 编译通过；运行后窗口出现并立即关闭（RenderDevice/InputBackend 未实现） |

#### 子阶段 B：输入 + 光标（可交互）

| 步骤 | 内容 | 文件 | 估计行数 | 验证方式 |
|------|------|------|---------|---------|
| **B1** | 编写 Cursor.cpp：`SystemCursorType` → `MouseCursor` 映射表（10 种直接映射 + 10 种 fallback）；`createSystem/getDefault/setCurrent` | `src/backend/raylib/Cursor.cpp` | ~50 | 编译通过；内部测试验证映射正确性 |
| **B2** | 编写 InputBackend.cpp：`pollEvent(Event&)` 实现"帧初处理事件"模式；支持 MouseMove/Down/Up/Wheel、KeyDown/Up、TextInput（`GetCharPressed`）、WindowResize/Close、Focus | `src/backend/raylib/InputBackend.cpp` | ~350 | 编译通过；窗口出现，鼠标键盘事件能传递到控件 |

**B2 关键实现细节**：帧初处理模式——`pollEvent` 在 `BeginDrawing()` 之前调用，读取 raylib 当前帧的输入状态。`EndDrawing()` 内部由 raylib 自动调用 `PollInputEvents()` 更新状态。因此每次 `present()` 后，下一帧 `pollEvent()` 立即读到新状态。窗口关闭检测使用 `WindowShouldClose()`。

#### 子阶段 C：渲染核心（图形可见）

| 步骤 | 内容 | 文件 | 估计行数 | 验证方式 |
|------|------|------|---------|---------|
| **C1** | 编写 RenderDevice.cpp 基础图元：`setDrawColor`（内部状态）、`fillRect`、`drawRect`、`drawLine`、`drawPoint`、`clear`、`present`、`setClipRect/clearClipRect`、`setBlendMode`、`flush`（空操作）、`getNativeHandle`（返回 nullptr） | `src/backend/raylib/RenderDevice.cpp` | ~120 | 编译通过；run `test_graphtool` 能看到背景色 + 矩形填充 + 线段（但复杂图元不可见） |
| **C2** | 添加复杂图元：`drawTriangle`、`drawQuad`、`drawTriangles`、`drawTriangleStrip`、`drawTriangleFan`（通过 `rlgl`：`rlBegin`/`rlColor4ub`/`rlVertex2f`/`rlEnd`） | 同上 | ~80 | `test_graphtool` 第 1-3 组全部正确渲染 |

**C2 关键实现细节**：`rlgl` 调用需要在 `BeginDrawing()`/`EndDrawing()` 之间进行。`rlBegin(RL_TRIANGLES)` 接受顶点颜色和位置。注意 raylib 的坐标系原点在左上角（与 SDL3 一致，与 OpenGL 左下角不同——raylib 在 `BeginDrawing()` 内自动翻转 Y 轴）。

#### 子阶段 D：纹理（图片可见）

| 步骤 | 内容 | 文件 | 估计行数 | 验证方式 |
|------|------|------|---------|---------|
| **D1** | 实现纹理工厂和绘制：`createTextureFromFile`、`createTextureFromSurface`、`destroyTexture`、`drawTexture`（`DrawTexturePro`）、`drawTextureRotated`（同 `DrawTexturePro` 加 rotation） | 同上 | ~80 | `test_button` 中纹理按钮可见（文字仍不可见） |
| **D2** | 实现渲染目标：`createRenderTexture`（包装 `RenderTexture2D`）、`setRenderTarget`/`resetRenderTarget`（`BeginTextureMode`/`EndTextureMode`）、`readPixels`（`ReadTexturePixels`） | 同上 | ~60 | `render-to-texture` 功能正常（可在 test_graphtool 中验证） |

#### 子阶段 E：文本（文字可见）

| 步骤 | 内容 | 文件 | 估计行数 | 验证方式 |
|------|------|------|---------|---------|
| **E1** | 编写 TextRenderer.cpp 基础：`loadFont`（`LoadFontEx`）、`loadFontFromMemory`（`LoadFontFromMemory`）、`getFontHeight`、`createText`（分配 `CachedText{font, string, fontSize}`）、`destroyText`（释放 `CachedText`）、`measureText(void*)`（`MeasureTextEx`）、`drawText(void*, x, y, color)`（`DrawTextEx`）、`measureText(Font*, string)`（`MeasureTextEx`）、`drawText(Font*, string, x, y, color)`（`DrawTextEx`） | `src/backend/raylib/TextRenderer.cpp` | ~140 | `test_label` 显示文字（单行） |
| **E2** | 实现 wrapWidth：`drawText(void*, x, y, wrapWidth, color)` + `drawText(Font*, string, x, y, wrapWidth, color)`；按空格/字符拆分+测量+换行逻辑 | 同上 | ~60 | `test_label` 多行文字换行正常 |

**E1 关键实现细节**：raylib 的 `Font` 类型与 SDL3_ttf 差异大。`LoadFontEx` 加载 TTF 后内部用 `stb_truetype` 生成字型图集（`Font.texture`）。每次 `DrawTextEx` 遍历 `Font.glyphs` 数组查找字型，从图集中取出对应区域绘制。因此 `createText` 不需要预计算——可以直接存储 `(Font*, string, fontSize, 1.0f)` 四元组，实际渲染时即时查字型。这与 SFML 的 `sf::Text`（缓存顶点）不同，但功能等价。

**E2 关键实现细节**：raylib 没有内建的 wrapWidth 文本测量。实现方式：用 `MeasureTextEx` 测量整行，如果 ≤ wrapWidth 则直接一行；否则按空格拆分单词，逐个添加，每次测量累积宽度，超宽则换行。与 SFML 方案完全相同。

#### 子阶段 F：Surface + 完整验证

| 步骤 | 内容 | 文件 | 估计行数 | 验证方式 |
|------|------|------|---------|---------|
| **F1** | 在 RenderDevice.cpp 中实现 Surface 相关：`Surface::create`（`GenImageColor`）、`loadFromFile`（`LoadImage`）、`loadFromMemory`（`LoadImageFromMemory` + SVG fallback）、`getPixel`（`GetImageColor`）、`setPixel`（`ImageDrawPixel`）、`blit`（`ImageDraw`）、`createTexture`（`LoadTextureFromImage`）、`rotate`（GPU render-to-texture + readback） | 同上 | ~100 | `test_button` 所有动画按钮正确渲染（含 SVG 图） |
| **F2** | 构建脚本 + 全测试验证：添加 `build_scripts\build_raylib.bat`、`build_raylib_test.bat`；逐个运行 10 个测试，修复发现的问题 | `build_scripts/*.bat` | ~20 | 全部 10 测试编译+运行通过，行为与 SDL3 一致 |

**F1 关键实现细节**：SVG 检测复用 SFML 方案——检查内存数据前 4 字节魔数（`<?xm`、`<svg`、`<!DO`），通过 nanosvg 栅格化为 RGBA 字节数组，然后用 `GenImageColor` + `ImageDraw` 或直接 `LoadImageFromMemory` 回写格式正确的 Image。nanosvg 源码可直接引用 SFML 后端已有的 `src/backend/sfml/nanosvg.h` 和 `nanosvgrast.h`（或复制到 `src/backend/raylib/`）。

#### 依赖拓扑

```
A1 ─→ A2 ─→ A3 ─→ B1 ─→ B2 ─→ C1 ─→ C2 ─→ D1 ─→ D2 ─→ E1 ─→ E2 ─→ F1 ─→ F2
      ↗         ↘                    ↕
  (工厂声明)   (窗口框架)         (共享 RenderDevice.cpp)
```

- A1/A2 无依赖，可同时开始
- B1 依赖 A3（需要窗口存在），C1 依赖 B2（需要事件能传递）
- C2/C3/D1/D2 共享同一个 RenderDevice.cpp，建议按顺序在一个会话中完成，减少文件开关开销
- E1/E2 依赖 D1（需要纹理机制加载字体图集），但 TextRenderer.cpp 独立文件
- F1 依赖 D1（纹理创建），可与其他步骤并行（独立文件）
- F2 最终集成验证

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| `InputBackend::pollEvent()` 与 raylib 的帧末轮询模型不匹配 | ★★★★ | 采用"帧初处理事件"模式：在 `BeginDrawing()` 前处理 `IsKeyPressed` 等状态查询 |
| `drawTriangles` 需要 rlgl 底层 API，raylib 公开层无对应 | ★★★ | 使用 `rlBegin(RL_TRIANGLES)` / `rlColor4ub()` / `rlVertex2f()` / `rlEnd()`，与 SDL3 的 `SDL_RenderGeometry` 结构类似 |
| raylib 无 `TTF_Text*` 缓存对象，`createText/destroyText` 需自定义缓存 | ★★ | 先实现无缓存版本（每次测量/绘制时传参数），后续优化再添加 `(Font*, string) → positions` 缓存 |
| 文本换行（wrapWidth）需手动实现 | ★★ | 参照 SFML 方案：`MeasureTextEx` 按空格拆分单词，计算宽度是否超过 wrapWidth |
| raylib `Color` 是 RGBA 非预乘，与我们的 SColor 一致 | ★ | 直接映射，无额外转换 |
| SVG 支持（nanosvg） | ★ | 完全复用 SFML 的 nanosvg 方案 |
| 无原生句柄（`nativeHandle()` 返回 nullptr） | ★ | TextRenderer 不需要原生句柄（raylib 的 `Font` 不依赖 OpenGL context） |
| raylib 只支持 `RenderTexture2D` 作为渲染目标，不支持任意 Texture | ★ | `createRenderTexture` 返回包装 `RenderTexture2D` 的 Texture；`setRenderTarget` 检查类型 |

#### 无需重复 SFML 批处理架构

raylib 内部通过 `rlgl` 实现了高效的顶点批处理。所有 `Draw*` 函数都会自动合并到 `rlgl` 的渲染批中，在 `EndDrawing()` 时统一提交。因此 **不需要** SFML 后端中的 `sf::VertexArray` 手动批处理 + `RenderDevice::flush()` 机制。`flush()` 可以实现为空操作。

### 16.5 与 SFML 后端的工作量对比

| 方面 | SFML | raylib | 分析 |
|------|------|--------|------|
| 绘制 API 1:1 映射度 | 中（需手动 batching） | **高**（大多数据有直接函数） | raylib 更容易 |
| 事件模型适配 | 易（SFML 也是对象级事件轮询） | **难**（raylib 是帧末全局轮询） | raylib 更困难 |
| 文本渲染 | 中（sf::Text + sf::Font） | **中-high**（Font 含图集，需自定义缓存） | 相近 |
| 图像/Surface 操作 | 中（sf::Image 功能有限） | **高**（Image API 非常完整） | raylib 更容易 |
| SVG 支持 | 需 nanosvg | 需 nanosvg | 相同 |
| 裁剪 | 手动 | **内置 scissor** | raylib 更容易 |
| 混合模式 | 需 OpenGL 状态设置 | **内置 blend modes** | raylib 更容易 |
| 文件数 | 9 | 7 | 相近 |
| 预计代码量 | ~1283 行 | ~1220 行 | 相近 |
| **总体难度** | **★★★☆** | **★★★☆**（但难度分布不同） | |

### 16.6 实施记录

| 日期 | 子阶段 | 内容 | 关键发现/修复 |
|------|--------|------|--------------|
| 2026-06-07 | C2 | `drawTriangle` / `drawQuad` 绕序修复 | raylib 的 `DrawTriangle(v1, v2, v3, color)` 要求 CCW（逆时针）绕序（注释明确标注 "(vertex in counter-clockwise order!)"），**且内部不自动翻转 CW 三角形**。SDL3/SFML 后端不关心绕序（SDL `SDL_RenderGeometry` 和 SFML `sf::ConvexShape` 都会自动 cull 不可见面或双面渲染），但 raylib 的 OpenGL 通道启用了背面剔除，CW 三角形被剔除。初始实现的 `drawTriangle` 和 `drawQuad` 在计算有符号面积（y-down: `area > 0` → CW, `area < 0` → CCW）后，错误地将 CW 三角形原样传递给 DrawTriangle，CCW 三角形反而被翻转成 CW，**导致所有三角形不可见**。修复：`area < 0`（CCW）时保持顶点顺序不变，`area >= 0`（CW）时交换后两个顶点以产生 CCW 三角形。影响范围：所有通过 `drawTriangle`/`drawQuad` 渲染的图元（粗线段、填充圆角矩形、填充椭圆/弧、填充多边形）。 |
| 2026-06-08 | D1 | Resize Freeze 修复 + Font DPI 缩放 | **问题**：拖拽窗口边框调整大小后释放鼠标导致 raylib 后端冻结。**根因链**：① `EndDrawing()` → `PollInputEvents()` 与 `InputBackend::newFrame()` 中的 `PollInputEvents()` 冲突。② 移除 `EndDrawing()` 后，`IsWindowResized()` 单个 resize 返回 500+ 次，事件循环洪水。③ resize drag 产生的 `MOUSE_UP` 到达时没有对应的 `MOUSE_DOWN`（按下发生在窗口边框），导致控件处理异常。**修复**：`present()` 不再调用 `EndDrawing()`（手动 flush + `SwapScreenBuffer()` + `WaitTime` 帧限速）；`InputBackend::newFrame()` 每帧仅一次 `PollInputEvents()`，添加 `m_resizeDetected` 标志抑制 resize 后误报的 `MOUSE_UP`；`MainWindow::processEvents()` 对 `WindowResize` 按 `(w, h)` 去重 + 500 事件安全阀。Font DPI 缩放：字体加载按 `size * 96/72` 缩放以匹配 SDL3_ttf 点→像素换算。 |
| 2026-06-09 | 15 | CheckBox/Label 性能优化 | 见 §16.7。 |

### 16.7 Phase 15——CheckBox/Label 性能优化

**问题**：`test_checkbox` 初始化 16 个复选框耗时 ~48s。根因为 `Bench::addControl()` → `resolveChildPercentages()` 对每个已有子控件调用 `setRect`，而 `CheckBox::setRect` 无条件 `recreate()`，导致 O(n²) 级联。每个 recreate 包含 Label 重建（含字体加载），而 SDL3 TextRenderer 之前没有任何字体缓存。

**改动（5 文件）**：

| 文件 | 改动 |
|------|------|
| `src/backend/sdl3/TextRenderer.cpp` | 新增字体缓存 `m_fontCache`（keyed by `(contentHash, size)`）和 `m_pathCache`（keyed by `(path, size)`）。消除冗余 `TTF_OpenFontIO` 调用。 |
| `src/CheckBox.cpp` + `include/CheckBox.h` | `setRect` 脏矩形检查（`if (m_rect == rect) return`）。原 `m_layoutDone` 标志已删除——脏矩形已足够防止级联。 |
| `src/Label.cpp` | `setRect` 脏矩形检查 + `setParent` 脏父检查。脏父检查仍调用 `ControlImpl::setParent(parent)` 以确保缩放更新（`setScaleX`/`setScaleY` 依赖此路径）。 |
| `src/ControlBase.cpp` | `setScaleX`/`setScaleY` 从 `child->setParent(this)` 改为 `updateChildScale(child.get())`——后者直接更新子控件复合缩放值 `m_xxScale/m_yyScale`，避免触发 `setParent` 的不必要开销（`inheritRenderer` 等）及其与脏父检查的冲突。 |
| `include/ControlBase.h` | 新增 `updateChildScale(Control*)` 私有内联方法，使用 `dynamic_cast` 安全转换至 `ControlImpl*` 后直接设置复合缩放。注释指导 override 的脏检查模式。 |

**`setScaleX`/`setScaleY` 重构**：
- **之前**：`setScaleX` 遍历 children 调用 `child->setParent(this)`，通过 setParent 内部的缩放公式更新子控件的 `m_xxScale`。这导致 `inheritRenderer()` 被反复调用，且与 Label 的脏父检查冲突。
- **之后**：`setScaleX` 遍历 children 调用 `updateChildScale(child.get())`，直接计算 `impl->m_xxScale = impl->m_xScale * m_xxScale`。等语义、零副作用。

**性能结果**：

| 后端 | 优化前 | 优化后 |
|------|--------|--------|
| SDL3 | ~48s | ~5-8s |
| SFML | — | ~5-7s |
| Raylib | — | ~3-5s |

~6.5× 加速。剩余瓶颈为 ~80 次 `TTF_CreateText` 调用（~25ms/次 ≈ 2s），进一步优化需跨 recreate 缓存 `TTF_Text*` 对象。

**调试清理**：移除了 Label/CheckBox/TextRenderer 中所有调试 printf（`g_recreateDepth`、`[LABEL_RECREATE]`、`[CHECKBOX_RECREATE]`、`[FONT_LOAD]`、`[FONT_RELOAD]`、`[FONT_HIT]`）及不再使用的 `Platform::GetTicks()` 变量。

### 16.8 Phase 15a——WinFrame 修复 + 绘制架构 + 帧率同步

**问题**：多个独立问题集中在 2026-06-11 修复：

1. **WinFrame 置顶/Z-order**：重叠 WinFrame 时点击下层窗口带参「置顶」会覆盖上层窗口的 `bringToFront()`，导致焦点混乱。
2. **WinFrame 重叠缩放光标**：鼠标在重叠窗口边缘时，下层窗口的 resize cursor 会覆盖上层窗口的正确光标。
3. **WinFrame 标题栏拖动**：标题 Label 拦截了 `MouseDown` 事件，导致无法通过标题栏拖动窗口。
4. **任务栏不显示**：`ConstDef.h` 中 `WINDOW_FLAG` 误用 `0x00020000`（`SDL_WINDOW_UTILITY`）替代 `0x00002000`（`SDL_WINDOW_HIGH_PIXEL_DENSITY`），导致所有窗口不在任务栏显示。
5. **绘制顺序**：边框在子控件下方绘制，覆盖子控件边缘内容；`getDrawRect()` 在每帧每控件多次遍历父链。
6. **帧率**：Raylib 的 `WaitTime` 60 FPS 锁未放开；SDL3 后端 `SDL_CreateWindowAndRenderer` 可能因驱动默认开启垂直同步。

**改动（16 文件）**：

| 文件 | 改动 |
|------|------|
| `src/WinFrame.cpp` | ① Step 0 置顶逻辑：新增 `consumedByFocus` 标志，判定 `bringToFront()` 后返回该标志而非直接 `return true`。② `MouseMove`：仅当 `m_resizable && m_lastEdgeFlags != 0` 时消费事件，否则返回 `false` 让兄弟 WinFrame 处理光标。③ 标题 Label 设置 `enableExpand(false) + AM_MID_LEFT + setClickable(false)`，使 `MouseDown` 穿透至标题栏拖动逻辑。 |
| `include/Label.h` | 新增 `m_clickable` 成员、`setClickable(bool)` 方法及 `LabelBuilder::setClickable(bool)` 声明。 |
| `src/Label.cpp` | `setClickable(bool)` 实现：当 `m_clickable == false` 时 `handleEvent` 立即返回 `false`（跳过 hotRect 检查和手型光标）。`setRect` 脏矩形早期返回。`setParent` 脏父检查。 |
| `include/ControlBase.h` | 新增 `m_frameDrawRect`（`SRect`）、`m_frameDrawRectValid`（`bool`）用于 Frame-drawRect 缓存。`beforeDraw()` / `afterDraw()` 声明。 |
| `src/ControlBase.cpp` | `beforeDraw()`：计算并缓存 `m_frameDrawRect`，然后绘制背景。`afterDraw()`：使用 `m_frameDrawRect` 绘制边框，清除有效标志。`mapToDrawRect()` / `mapToDrawPoint()`：绘制阶段内直接使用 `m_frameDrawRect`，避免父链遍历。 |
| `src/Panel.cpp`、`src/Button.cpp`、`src/CheckBox.cpp`、`src/ProgressBar.cpp`、`src/Label.cpp`、`src/EditBox.cpp`、`src/ScrollBar.cpp`、`src/TextArea.cpp` | 各自 `draw()` 末尾调用 `afterDraw()` 以确保边框在子控件/内容之上渲染。 |
| `include/ConstDef.h` | `WINDOW_FLAG` 修正：`0x00020000`（`SDL_WINDOW_UTILITY`）→ `0x00002000`（`SDL_WINDOW_HIGH_PIXEL_DENSITY`），窗口恢复在任务栏显示。 |
| `src/backend/sdl3/Window.cpp` | `SDL_CreateWindowAndRenderer` 之后添加 `SDL_SetRenderVSync(renderer, 0)`，显式关闭可能由驱动默认开启的垂直同步。 |
| `src/backend/raylib/RenderDevice.cpp` | `present()` 中移除 `WaitTime` 60 FPS 帧率限制代码（`m_lastPresentTime` 及相关变量一并移除），完全放开帧率。 |

**结果**：
- WinFrame 重叠交互（置顶、光标、拖动）行为正确。
- 所有 SDL3 测试窗口恢复在任务栏显示。
- 帧率表现（test_button / FPS 打印）：SDL3 ~420 FPS，SFML ~1000 FPS，Raylib ~1000 FPS（均无垂直同步）。

### 16.9 Phase 15b——SFML 批处理 z-order 修复 + DebugTrace 清理

**问题**：SFML `test_layout_advanced` 中 `anchorCenter`（紫色 CENTER 按钮）的边框线从 FILL bar（青色 FILL 按钮）下方透出。root cause 是 `SFMLRenderDevice` 使用 `m_fillBatch`（Triangle）和 `m_lineBatch`（Line）两个独立顶点缓冲，`flushBatches()` 先画所有 fills 再画所有 lines，导致**前一个控件的边框线画在后一个控件的填充色之上**，z-order 被破坏：

```
绘制顺序（SFML 批处理版）：
1. anchorCenter fillRect      → m_fillBatch ✓
2. anchorCenter drawRect      → m_lineBatch  ✓（边框线）
3. anchorFill fillRect        → m_fillBatch  ✓（应覆盖边框线）
4. anchorFill drawRect        → m_lineBatch  ✓
flushBatches(): 先 m_fillBatch（1+3），后 m_lineBatch（2+4）
→ 结果：边框（2）画在 fill（3）之上 ✗
```

**改动（1 文件）**：`src/backend/sfml/RenderDevice.cpp`

| 操作 | flush 前提 |
|------|-----------|
| `fillRect` / `drawTriangles` / `drawTriangleStrip` / `drawTriangleFan` / `drawTriangle` / `drawQuad` | flush `m_lineBatch` 先（保证前控件的线在后续填充前已提交） |
| `drawRect` / `drawLine` | flush `m_fillBatch` 先（保证前控件的填充在后续描线前已提交） |

每个填充/描线操作前 flush 对方批次的顶点，确保每个控件的 fill+line **在下一个控件的 fill 之前完整提交**。这样批处理退化为"每对 fill+line 一个 draw call"，但保证正确的 z-order。

**DebugTrace 清理**：
- `subModules/DebugTrace` 子模块整体移除
- `.gitmodules`：删除 DebugTrace 条目

### 16.10 Raylib DrawTexturePro DLL 桥接修复

**问题**：raylib fromsource 测试（`test_fromsource_raylib`）中所有纹理不可见。`DrawTexturePro` 从 `UICornerstone.dll` 经回调桥接调用到 exe 中的 `RaylibRenderDevice::drawTexture`，日志显示正确的纹理 ID 和 dst rect，但画面上无任何纹理。

**根因**：未知。预编译 `raylib v6.0` 库中 `DrawTexturePro` 函数在跨 DLL/exe 桥接模式下始终不渲染。替代尝试：
- `DrawTexturePro` 单独调用（含 `rlDrawRenderBatchActive` + `rlSetTexture` 等各种变体）→ 皆不可见
- `rlBegin(RL_TRIANGLES)` + `rlTexCoord2f` + `rlVertex2f` 低级 API → 同样不可见
- 透明热身（`DrawTextureV` + alpha=0）→ 不可见（"假"画不足以初始化 OpenGL 管线）
- **唯一可行**：先调用 `DrawTextureV`（或 `DrawTextureEx` 原生大小），再调用任何函数 → 两者都可见。

`DrawTextureV` 在 `raylib.h` 中定义为 `static inline`，其内部调 `DrawTextureEx` → `DrawTexturePro`；而直接从 exe 调用 `DrawTexturePro` 失败——说明问题出在**函数跨库调用路径**上，而非渲染逻辑本身。

**修复**：完全绕过 `DrawTexturePro` 和 `rlBegin/rlEnd`，使用 `rlPushMatrix + rlScalef + DrawTextureEx` 组合实现非均匀缩放：

```cpp
// 之前（无效）：DrawTexturePro(tex, src, dst, origin, 0, WHITE)
// 之后（有效）：
rlPushMatrix();
rlTranslatef(dst.x, dst.y, 0);                            // 定位
rlScalef(sx, sy, 1.0f);                                   // 非均匀拉伸
rlTranslatef(-src.x, -src.y, 0);                           // 子区域偏移
DrawTextureEx(nativeTex, {0,0}, 0, 1.0f, WHITE);           // 原生大小绘制
rlPopMatrix();
```

矩阵变换将纹理的源子区域映射到目标矩形——等价于 `DrawTexturePro` 的行为，但不直接调用该函数。

**关键发现**：
- `DrawTextureV`（`static inline`）编译在 exe 翻译单元，`DrawTexturePro` 来自预编译 `raylib.lib`——函数调用路径可能是关键差异
- 透明热身（alpha=0）无效——必须真正绘制（alpha > 0）才能初始化渲染管线的 OpenGL 纹理绑定状态
- 同时修复了 `drawTextureRotated` 中缺失的 `EndBlendMode()` 调用

**文件变更**：
| 文件 | 变更 |
|------|------|
| `src/backend/raylib/RenderDevice.cpp` | `drawTexture()` 和 `drawTextureRotated()` 改用 `rlPushMatrix + rlScalef + DrawTextureEx`；`drawTextureRotated` 补 `EndBlendMode()` guard |
- `CMakeLists.txt`：删除 `DEBUG_TRACE_DIR` 相关变量、`DebugTrace.cpp` 编译、`DebugInfoX64.dll` 复制
- `test/CMakeLists.txt`：删除 `DEBUG_TRACE_DIR` 引用
- `README.md` / `doc/Build_Guide.md` / `doc/LayoutSystem_Design.md` / `AGENTS.md`：同步清理所有 DebugTrace 引用

**结果**：
- SFML 下 anchorCenter 边框不再透出 FILL bar
- SDL3/raylib 后端不受影响
- DebugTrace 完全从项目中移除（含子模块反注册、CMake 配置、文档）

## 17. 总体工时汇总

| Phase | 内容 | 文件数 | 新增行数 | 预估天数 | 实际进度 | 风险 |
|-------|------|--------|---------|---------|---------|------|
| 1 | SColor 统一 | ~35 | ~100 | 2-3 | ✅ 已完成 | ★ |
| 2 | RenderDevice 接口 + SDL3 实现 | ~23 | ~650 (净增) | 5-7 | ✅ 已完成 | ★★★ |
| 3 | Texture + Surface 抽象 | ~10 | ~1000 | 4-5 | ✅ 已完成（含 LuotiAni 完整迁移） | ★★★★ |
| 4 | SDL 头文件解耦 | ~10 | ~35 | 1 | ✅ 已完成 | ★ |
| 5 | Font / TextRenderer 抽象 | ~12 | ~800 | 5-7 | ✅ 已完成 | ★★★★★ |
| 6 | Window / App / Event 重构 | ~15 | ~600 | 3-4 | ✅ 已完成 | ★★ |
| 7 | TTF_Text\* 缓存 | ~4 | ~80 | 1 | ✅ 已完成 | ★★ |
| 8 | ResourceProvider 抽象 | ~6 | ~200 | 1 | ✅ 已完成 | ★★ |
| 9 | 移除 ResourceLoader | - | -656 | 0.5 | ✅ 已完成 | ★ |
| 10 | SDL Cursor 抽象 | ~4 | ~150 | 0.5 | ✅ 已完成 | ★ |
| 11 | 移除 ControlBase SDL_Renderer | ~6 | -100 | 0.5 | ✅ 已完成 | ★ |
| 12 | 事件系统迁移（Controls → Union API）| ~12 | ~400 | 2 | ✅ 已完成 | ★★ |
| 13 | SFML 后端 | 9 | ~1283 | 5-7 | ✅ 已完成 | ★★★★ |
| 13a | SFML 性能优化 + 后端一致性修复 | 6 | ~110 | 1 | ✅ 已完成 | ★★ |
| 14 | raylib 后端 | 7 | ~1220 | 5-7 | ✅ 已完成 | ★★★★ |
| 15 | CheckBox/Label 性能优化 | 5 | ~120 | 1 | ✅ 已完成 | ★★ |
| 15a | WinFrame 修复 + 绘制架构 + 帧率同步 | 16 | ~60 | 1 | ✅ 已完成 | ★★ |
| 15b | SFML 批处理 z-order 修复（DebugTrace 清理） | 11 | ~80 | 0.5 | ✅ 已完成 | ★ |
| **合计** | | **~170** | **~7030** | **43-55** | | |

## 18. 执行建议

### 18.1 实际执行顺序与计划

```
Phase 1  (SColor)             ← ✅ 已完成
    ↓
Phase 2  (RenderDevice)       ← ✅ 已完成 — 所有 SDL 绘制调用已迁移至 RenderDevice
    ↓
Phase 3  (Texture/Surface)    ← ✅ 已完成 — 含 LuotiAni.h 完整迁移 (~180 SDL 调用)
    ↓
Phase 4  (Header Cleanup)     ← ✅ 已完成 — 8 非后端 header 移除 umbrella SDL include
    ↓
Phase 5  (Font/TextRenderer)  ← ✅ 已完成 — Label/EditBox/TextArea 全部迁移
    ↓
Phase 6  (Window/App/Event)   ← ✅ 已完成 — InputBackend 独立 + MainWindow.h 脱 SDL + AppCallbacks
    ↓
Phase 7  (TTF_Text* 缓存)     ← ✅ 已完成 — Label 层缓存 TTF_Text* 对象
    ↓
Phase 8  (ResourceProvider)   ← ✅ 已完成 — ResourceProvider 接口 + Filesystem 实现
    ↓
Phase 9  (移除 ResourceLoader) ← ✅ 已完成 — ResourceLoader.h/.cpp 删除
    ↓
Phase 10 (SDL Cursor)         ← ✅ 已完成 — Cursor 抽象类 + SDLCursor 实现
    ↓
Phase 11 (移除 SDL_Renderer)  ← ✅ 已完成 — ControlBase.h 零 SDL 依赖
    ↓
Phase 12 (事件系统迁移)        ← ✅ 已完成 — 所有控件 handleEvent → EventType union API
    ↓
Phase 13 (SFML backend)       ← ✅ 已完成 — 所有抽象接口的 SFML 实现 + nanosvg SVG 栅格化
    ↓
Phase 13a (SFML 性能优化 + 后端一致性) ← ✅ 已完成 — 批量顶点提交 + flush() 接口 + 文字/键盘 Bug 修复 + SDL3 默认混合模式修复
    ↓
Phase 14 (raylib backend)     ← ✅ 已完成（§16.6）
    ↓
Phase 15 (CheckBox/Label 性能优化) ← ✅ 已完成（§16.7）
    ↓
Phase 15a (WinFrame 修复 + 绘制架构 + 帧率同步) ← ✅ 已完成（§16.8）
    ↓
Phase 15b (SFML 批处理 z-order 修复 + DebugTrace 清理) ← ✅ 已完成
    ↓
Phase 16 (RGBA8888 像素格式排查 + DLL 桥接验证) ← ✅ 已完成
    ↓
Phase 16b (Raylib DrawTexturePro DLL 桥接修复) ← ✅ 已完成
```

> **注**：Phase 4（SDL 头文件解耦）在 Phase 3 后自然衍生——Texture/Surface 抽象完成后，大量头文件不再需要 SDL 类型，可直接移除或替换为具体子头文件。Phases 7-12 为执行过程中识别出的额外去耦机会，均在 Phase 5 后顺次完成。

### 18.2 编译时选择后端

```cmake
option(UICORNERSTONE_BACKEND "Backend library" "SDL3")  # 可选: SDL3, SFML, RAYLIB
target_compile_definitions(UICornerstone PUBLIC UICORNERSTONE_BACKEND=${UICORNERSTONE_BACKEND})

if(UICORNERSTONE_BACKEND STREQUAL "SDL3")
    target_link_libraries(UICornerstone PUBLIC SDL3 SDL3_ttf SDL3_image)
elseif(UICORNERSTONE_BACKEND STREQUAL "SFML")
    target_link_libraries(UICornerstone PUBLIC sfml-graphics sfml-window sfml-system)
elseif(UICORNERSTONE_BACKEND STREQUAL "RAYLIB")
    target_link_libraries(UICornerstone PUBLIC raylib)
endif()
```

### 18.3 关于"自绘鼠标"

Phase 2 完成后，RenderDevice 接管了所有绘制。Phase 6 的 Window 抽象暴露了鼠标位置。此时在 UICornerstone 层实现软光标只需：

```cpp
// 在 BENCH->draw() 最后
if (m_useCustomCursor) {
    GET_RENDER_DEVICE->drawTexture(m_cursorTexture, ..., mousePos);
}
```

**不需要等到后端抽象完成**——SColor 统一后（Phase 1），即可在现有 SDL3 代码中用 `SDL_ShowCursor(SDL_DISABLE)` + 手动绘制光标纹理实现。推荐在 Phase 1 后评估是否值得独立做。
