# UICornerstone DLL + C ABI 设计

> 对应 Phase 16 | 编制 2026-06-12 | 最后更新 2026-06-19 | 状态: **已完成** — R1~R10b + fromsource + 16c~16i 全部三后端验证通过

---

## 目录

1. [动机与目标](#1-动机与目标)
2. [架构总览](#2-架构总览)
3. [要点 1 — Bench 作为可渲染区域（Viewport）](#3-要点-1--bench-作为可渲染区域viewport)
4. [要点 2 — JSON 布局系统](#4-要点-2--json-布局系统)
5. [要点 3 — 重命名 UIControls → UICornerstone](#5-要点-3--重命名-uicontrols--uicornerstone)
6. [C ABI 公开 API 定义](#6-c-abi-公开-api-定义)
7. [内部架构](#7-内部架构)
8. [CMake 构建模式](#8-cmake-构建模式)
9. [实施步骤](#9-实施步骤)
10. [附录：游戏集成示例](#10-附录游戏集成示例)

---

## 1. 动机与目标

### 现状

- `UIControls` 是一个 C++ 静态库，编译时通过 `-DUICONTROLS_BACKEND=xxx` 选择 SDL3/SFML/raylib 后端
- 后端源码直接编译进库中，更换后端需重新 cmake
- 对外暴露 C++ 类（`Button`、`Label` 等），游戏开发者需使用相同 MSVC+CRT 才能集成
- 依赖具体窗口管理（MainWindow 管理 OS 窗口）

### 目标

1. **闭源分发**：核心控件逻辑封装为 DLL，对外只暴露 C ABI（C 函数 + 回调表）
2. **游戏集成**：游戏开发者提供自己的渲染/输入/字体系统，通过回调表注入；UICornerstone 只管理一个矩形区域（viewport）
3. **声明式 UI**：通过 JSON 布局文件定义 UI 结构，无需硬编码 C++ 控件创建
4. **开源后端插件**（后续阶段）：`UIBackend_*.dll` 提供现成的 SDL3/SFML/raylib 实现，社区可贡献更多

### 非目标

- 本阶段不改动控件 C++ 类名（`Button`、`Label` 等保持不变）
- 本阶段不改动文件名或目录结构（仅修改内容中的项目名）
- 本阶段不要求跨编译器兼容（C ABI 保证函数调用级兼容，但控件内部仍使用 MSVC 构建）

---

## 2. 架构总览

```
┌─────────────────────────────────────────────────┐
│              游戏 / 应用程序                       │
│                                                   │
│  ┌─ 游戏渲染管线 ─┐  ┌─ 游戏输入系统 ─┐            │
│  │ OpenGL/D3D/Vulkan │  │ 键盘/鼠标/手柄│            │
│  └────────┬───────┘  └──────┬────────┘            │
│           │                 │                      │
│  ┌────────▼─────────────────▼────────┐             │
│  │     UIBackendCallbacks 回调表      │             │
│  │  (用户提供: fillRect, pollEvent,   │             │
│  │   drawText, ...)                  │             │
│  └────────────────┬──────────────────┘             │
└───────────────────┬─────────────────────────────────┘
                    │  UICornerstone_Init(&callbacks)
                    ▼
┌──────────────────────────────────────────────────────┐
│              UICornerstone.dll  (闭源)                │
│                                                      │
│  ┌────────────────────────────────────────┐          │
│  │         C ABI 公开层                    │          │
│  │  UICornerstone_Init / _SetViewport /   │          │
│  │  _LoadLayout / _CreateButton / _Render │          │
│  └────────────────┬───────────────────────┘          │
│                   │                                   │
│  ┌────────────────▼───────────────────────┐          │
│  │       CallbackAdapter 层                │          │
│  │  CallbackWindow        → implements Window        │
│  │  CallbackRenderDevice  → implements RenderDevice  │
│  │  CallbackInputBackend  → implements InputBackend  │
│  │  CallbackTextRenderer  → implements TextRenderer  │
│  │  CallbackRP            → implements ResourceProv. │
│  └────────────────┬───────────────────────┘          │
│                   │                                   │
│  ┌────────────────▼───────────────────────┐          │
│  │     现有控件系统 (完全不变)              │          │
│  │  Bench / Button / Label / EditBox /    │          │
│  │  Menu / CheckBox / ProgressBar / ...   │          │
│  │  LayoutParser / LayoutEngine           │          │
│  └────────────────────────────────────────┘          │
└──────────────────────────────────────────────────────┘
                    ▲
                    │ LoadLibrary + GetProcAddress
                    │ (可选)
  ┌─────────────────┴──────────────────────┐
  │      UIBackend_sdl3.dll  (开源插件)     │
  │  导出 GetUIBackendCallbacks → 预填充    │
  │  的回调表 (使用 SDL3 实现)              │
  └────────────────────────────────────────┘
```

---

## 3. 要点 1 — Bench 作为可渲染区域（Viewport）

### 3.1 概念

UICornerstone **不管理 OS 窗口**。它只管理一个由用户指定的矩形渲染区域（viewport），所有 UI 内容渲染在该区域内，由用户自己的渲染管线管理。

```
游戏画面 (1920×1080)
┌────────────────────────────────────────────┐
│  3D 场景渲染                               │
│                                            │
│   ┌───────────────────┐                    │
│   │ UICornerstone     │                    │
│   │ Viewport          │  ← 用户指定位置和大小 │
│   │ (x=100, y=200,    │                    │
│   │  w=800, h=600)    │                    │
│   │                   │                    │
│   └───────────────────┘                    │
│                                            │
│  游戏 HUD / 其他 UI                        │
└────────────────────────────────────────────┘
```

### 3.2 Bench 的角色

现有 `Bench` 类已经是根容器。在 viewport 模式下：

- `Bench` 的矩形等于用户设置的 viewport
- `UICornerstone_SetViewport(x, y, w, h)` 设置 Bench 的位置和大小
- 所有控件作为 Bench 的子节点，坐标相对于 Bench
- 渲染时，`setClipRect` 确保内容不溢出 viewport

### 3.3 窗口模式兼容

`createWindow` 回调**是可选的**（可为 NULL）：

| 模式 | createWindow | 使用场景 |
|------|-------------|---------|
| Viewport 模式 | NULL | 游戏集成，窗口由游戏管理 |
| 窗口模式 | 提供实现 | 独立运行（测试程序、工具） |

窗口模式下，BackendManager 内部调用 `createWindow` 创建 OS 窗口，并将 viewport 设置为全窗口大小。

### 3.4 C ABI 接口

```c
// 设置渲染区域（必须在使用前调用）
void UICornerstone_SetViewport(float x, float y, float w, float h);

// 查询当前视口
void UICornerstone_GetViewport(float* x, float* y, float* w, float* h);
```

---

## 4. 要点 2 — JSON 布局系统

### 4.1 概念

用户通过 JSON 文件声明 UI 结构，无需写 C++ 代码创建控件。现有 `LayoutParser` + `LayoutEngine` 已支持此功能，本阶段将其通过 C ABI 暴露。

### 4.2 布局 JSON 格式

```json
{
    "bench": {
        "size": {"w": 800, "h": 600},
        "background": "#2a2a2a"
    },
    "controls": [
        {
            "type": "Button",
            "id": "start_btn",
            "text": "开始游戏",
            "font": "fonts/msyh.ttf",
            "size": 24,
            "rect": {"x": 300, "y": 200, "w": 200, "h": 50},
            "onClick": "onStartGame",
            "visible": true,
            "enabled": true
        },
        {
            "type": "Label",
            "id": "game_title",
            "text": "我的游戏",
            "font": "fonts/msyh.ttf",
            "size": 36,
            "rect": {"x": 300, "y": 100, "w": 200, "h": 60},
            "align": "Center"
        },
        {
            "type": "CheckBox",
            "id": "options_cb",
            "text": "启用特效",
            "rect": {"x": 300, "y": 300, "w": 200, "h": 30}
        }
    ]
}
```

支持的控件类型：`Button`、`Label`、`EditBox`、`CheckBox`、`ProgressBar`、`Menu`、`Panel`、`WinFrame`。

### 4.3 C ABI 接口

```c
// 从 JSON 字符串加载布局
// 返回 1 成功，0 失败
int UICornerstone_LoadLayout(const char* jsonContent);

// 从文件加载布局（相对于资源路径）
int UICornerstone_LoadLayoutFromFile(const char* filePath);

// 按 id 查找控件（布局中定义的 id 属性）
UIControlHandle UICornerstone_FindControl(const char* id);

// 注册动作回调（对应布局中的 onClick 等字符串）
typedef void (*UIActionCallback)(UIControlHandle ctl, void* userData);
void UICornerstone_RegisterAction(const char* name, UIActionCallback cb, void* userData);
```

### 4.4 使用流程

```c
// 1. 注册事件处理函数
void onStartGame(UIControlHandle btn, void* user) {
    // 开始游戏...
}
UICornerstone_RegisterAction("onStartGame", onStartGame, NULL);

// 2. 加载布局
UICornerstone_LoadLayoutFromFile("ui/main_menu.json");

// 3. 之后可获取控件引用
UIControlHandle btn = UICornerstone_FindControl("start_btn");
UICornerstone_SetText(btn, "连接中...");
```

### 4.5 与现有系统的关系

```
LayoutParser 解析 JSON → 创建 LayoutConfig 对象
  → LayoutEngine 计算布局 → 调用 C++ 控件工厂（new Button(...)）
  → 控件挂到 Bench 下
```

C ABI 的 `UICornerstone_LoadLayout` 包装这个流程，对外隐藏所有 C++ 细节。

---

## 5. 要点 3 — 重命名 UIControls → UICornerstone

### 5.1 范围

| 类别 | 改前 | 改后 |
|------|------|------|
| CMake 项目名 | `UIControls` | `UICornerstone` |
| 静态库目标 | `UIControls` | `UICornerstone` |
| DLL 目标 | `UIControls.dll` | `UICornerstone.dll` |
| CMake 变量前缀 | `UICONTROLS_*` | `UICORNERSTONE_*` |
| 编译定义前缀 | `UICONTROLS_*` | `UICORNERSTONE_*` |
| 包含守卫中的项目标识 | `UIControls` | `UICornerstone` |
| App 名称字符串 | `"UIControls"` | `"UICornerstone"` |
| GitHub 仓库名 | `SeaOceanLiu/UIControls` | `SeaOceanLiu/UICornerstone` |
| BackendManager 后端名 | `"sdl3"` | `"sdl3"`（不变——后端名独立于项目名） |

### 5.2 不变的部分

- 类名（`Button`、`Label`、`Bench` 等）
- 文件名（`Button.h`、`Label.cpp` 等）
- 文件级包含守卫（`#ifndef BUTTON_H` 等）
- 后端子目录名（`src/backend/sdl3/` 等）
- 后端子目录名（`src/backend/sdl3/` 等）

### 5.3 具体变更清单

| 文件 | 变更 |
|------|------|
| `CMakeLists.txt` | `project(UICornerstone)`、目标名、编译定义 |
| `test/CMakeLists.txt` | 目标依赖名 `UICornerstone`、定义名 |
| `include/BackendPlugin.h` | 包含守卫、注释 |
| `include/MainWindow.h` | `#define APP_NAME` |
| `src/BackendManager.cpp` | 注释、日志 |
| `src/ConstDef.cpp` | 日志 |
| `doc/*.md`（多个设计文档） | 项目名引用 |
| `AGENTS.md` | 项目名引用 |
| `README.md` | 项目名引用 |
| `build_scripts/*.bat` | CMake 变量名（如果引用） |

### 5.4 GitHub 仓库改名

安装 `gh` CLI 后执行：

```bash
gh repo rename UICornerstone
```

或手动在 `https://github.com/SeaOceanLiu/UIControls/settings` 操作。

---

## 6. C ABI 公开 API 定义

### 6.1 回调表

```c
// include/UICornerstoneAPI.h

#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 句柄类型 ============ */
typedef void* UIWindowHandle;
typedef void* UIRenderDeviceHandle;
typedef void* UIInputBackendHandle;
typedef void* UITextRendererHandle;
typedef void* UIResourceProviderHandle;
typedef void* UIControlHandle;

/* ============ 基础类型 ============ */
typedef struct { float x, y, w, h; }    UIRect;
typedef struct { uint8_t r, g, b, a; }  UIColorF;
typedef struct { float r, g, b, a; }    UIColorF;

/* ============ 后端回调表 ============ */
typedef struct {
    int version;  // 设为 1

    // --- Window (可选，可为 NULL) ---
    UIWindowHandle  (*createWindow)(const char* title, int w, int h, uint32_t flags);
    void            (*destroyWindow)(UIWindowHandle);
    void            (*getWindowSize)(UIWindowHandle, float* w, float* h);

    // --- RenderDevice (必须) ---
    UIRenderDeviceHandle (*createRenderDevice)(void* nativeContext);
    void                 (*destroyRenderDevice)(UIRenderDeviceHandle);
    void                 (*setDrawColor)(UIRenderDeviceHandle, UIColor);
    void                 (*fillRect)(UIRenderDeviceHandle, UIRect);
    void                 (*drawRect)(UIRenderDeviceHandle, UIRect);
    void                 (*drawLine)(UIRenderDeviceHandle, float x1,float y1, float x2,float y2);
    void                 (*clear)(UIRenderDeviceHandle, UIColor);
    void                 (*present)(UIRenderDeviceHandle);

    // --- InputBackend (必须) ---
    UIInputBackendHandle (*createInputBackend)(void* nativeWindowHandle);
    void                 (*destroyInputBackend)(UIInputBackendHandle);
    int                  (*pollEvent)(UIInputBackendHandle, void* eventBuf);
    int                  (*getModState)(UIInputBackendHandle);

    // --- TextRenderer (必须) ---
    UITextRendererHandle (*createTextRenderer)(UIRenderDeviceHandle);
    void                 (*destroyTextRenderer)(UITextRendererHandle);
    void*                (*loadFont)(UITextRendererHandle, const char* path, float size);
    void*                (*loadFontFromMemory)(UITextRendererHandle, const void* data, int len, float size);
    void                 (*destroyFont)(void* font);
    float                (*measureTextWidth)(UITextRendererHandle, void* font, const char* text);
    float                (*getFontHeight)(UITextRendererHandle, void* font);
    void                 (*drawText)(UITextRendererHandle, void* font, const char* text, float x, float y, UIColor);

    // --- ResourceProvider (可选, 可为 NULL) ---
    UIResourceProviderHandle (*createResourceProvider)(const char* basePath);
    void                     (*destroyResourceProvider)(UIResourceProviderHandle);
    int                      (*readFile)(UIResourceProviderHandle, const char* path, void* buf, int maxLen);

    // --- Cursor factories (可选, 可为 NULL；NULL 时禁用光标反馈) ---
    void* (*createSystemCursor)(int type);
    void* (*getDefaultCursor)();
    void  (*setCurrentCursor)(void* cursor);
} UIBackendCallbacks;

/* ============ 初始化 ============ */
int  UICornerstone_Init(const UIBackendCallbacks* callbacks);
void UICornerstone_Shutdown(void);
int  UICornerstone_InitFromPlugin(const char* pluginName);

/* ============ 视口控制 ============ */
void UICornerstone_SetViewport(float x, float y, float w, float h);
void UICornerstone_GetViewport(float* x, float* y, float* w, float* h);

/* ============ 帧循环 ============ */
void UICornerstone_ProcessEvents(void);
void UICornerstone_Update(double deltaTime);
void UICornerstone_Render(void);
int  UICornerstone_IsQuitRequested(void);

/* ============ 布局系统 ============ */
int               UICornerstone_LoadLayout(const char* jsonContent);
int               UICornerstone_LoadLayoutFromFile(const char* filePath);
UIControlHandle   UICornerstone_FindControl(const char* id);

typedef void (*UIActionCallback)(UIControlHandle ctl, void* userData);
void UICornerstone_RegisterAction(const char* name, UIActionCallback cb, void* userData);

/* ============ 编程式控件创建 ============ */
UIControlHandle UICornerstone_CreateButton(const char* text,
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateLabel(const char* text, float fontSize,
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateCheckBox(const char* text,
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateEditBox(
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateProgressBar(
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateSlider(
    float x, float y, float w, float h, float min, float max, float value);
UIControlHandle UICornerstone_CreatePanel(
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateTextArea(
    float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateWinFrame(
    const char* title, float x, float y, float w, float h);
UIControlHandle UICornerstone_CreateMenu(void);
UIControlHandle UICornerstone_CreateColorPicker(
    float x, float y, float w, float h, const char* color);
UIControlHandle UICornerstone_CreateImageButton(
    const char* normalImage, const char* hoverImage, const char* pressedImage,
    float x, float y, float w, float h);

/* ============ 控件通用操作 ============ */
void UICornerstone_SetRect(UIControlHandle ctl, float x, float y, float w, float h);
void UICornerstone_GetRect(UIControlHandle ctl, float* x, float* y, float* w, float* h);
void UICornerstone_SetVisible(UIControlHandle ctl, int visible);
void UICornerstone_SetEnabled(UIControlHandle ctl, int enabled);
void UICornerstone_SetText(UIControlHandle ctl, const char* text);
void UICornerstone_AddChild(UIControlHandle parent, UIControlHandle child);
void UICornerstone_SetOnClick(UIControlHandle ctl, UIActionCallback cb, void* userData);
void UICornerstone_SetBGColor(UIControlHandle ctl, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void UICornerstone_SetProgress(UIControlHandle ctl, float value);
void UICornerstone_SetChecked(UIControlHandle ctl, int checked);
void UICornerstone_DestroyControl(UIControlHandle ctl);
const char* UICornerstone_GetText(UIControlHandle ctl);
int UICornerstone_GetChecked(UIControlHandle ctl);
float UICornerstone_GetProgress(UIControlHandle ctl);
float UICornerstone_GetSliderValue(UIControlHandle ctl);
void UICornerstone_SetSliderValue(UIControlHandle ctl, float value);
void UICornerstone_SetOnSliderChanged(UIControlHandle ctl, UIActionCallback cb, void* userData);
void UICornerstone_WinFrameSetClientText(UIControlHandle wf, const char* text);
void UICornerstone_SetButtonAnimation(UIControlHandle btn, const char* jsoncPath);
void UICornerstone_GetColorPickerColor(UIControlHandle ctl, char* hexOut, int maxLen);
void UICornerstone_SetOnColorChanged(UIControlHandle ctl, UIActionCallback cb, void* userData);
void UICornerstone_SetClosedSwatchSize(UIControlHandle ctl, float size);
void UICornerstone_SetClosedFontSize(UIControlHandle ctl, int size);
void UICornerstone_SetClosedTextColor(UIControlHandle ctl, const char* hex);
void UICornerstone_SetPopupBGColor(UIControlHandle ctl, const char* hex);

/* ============ ComboBox ============ */
UIControlHandle UICornerstone_CreateComboBox(
    float x, float y, float w, float h);
int UICornerstone_ComboBoxSetItems(UIControlHandle ctl, const char* itemsJson);
int UICornerstone_ComboBoxSetSelectedIndex(UIControlHandle ctl, int index);
int UICornerstone_ComboBoxGetSelectedIndex(UIControlHandle ctl);
const char* UICornerstone_ComboBoxGetSelectedLabel(UIControlHandle ctl);
void UICornerstone_ComboBoxSetOnSelectionChanged(
    UIControlHandle ctl, UIActionCallback cb, void* userData);

/* ============ Dialog / Popup ============ */
UIControlHandle UICornerstone_CreateDialog(
    const char* confirmText, const char* cancelText,
    float x, float y, float w, float h);
void UICornerstone_Show(UIControlHandle ctl);
void UICornerstone_Close(UIControlHandle ctl);
void UICornerstone_SetDialogCentered(UIControlHandle ctl, int centered);
void UICornerstone_SetDialogPosition(UIControlHandle ctl, float x, float y, float w, float h);
void UICornerstone_SetContent(UIControlHandle dlg, UIControlHandle content);
void UICornerstone_SetOnConfirm(UIControlHandle ctl, UIActionCallback cb, void* userData);
void UICornerstone_SetOnCancel(UIControlHandle ctl, UIActionCallback cb, void* userData);
void UICornerstone_SetOnClose(UIControlHandle ctl, UIActionCallback cb, void* userData);
void UICornerstone_SetConfirmButtonText(UIControlHandle ctl, const char* text);
void UICornerstone_SetCancelButtonText(UIControlHandle ctl, const char* text);

/* ============ 控件查询 ============ */
const char* UICornerstone_GetControlId(UIControlHandle ctl);

#ifdef __cplusplus
}
#endif
```

### 6.2 事件结构

```c
// 事件数据缓冲区大小需满足所有事件类型
#define UI_EVENT_BUF_SIZE 128

typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_MOUSE_MOVE,
    UI_EVENT_MOUSE_DOWN,
    UI_EVENT_MOUSE_UP,
    UI_EVENT_MOUSE_WHEEL,
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_TEXT_INPUT,
    UI_EVENT_WINDOW_RESIZE,
    UI_EVENT_WINDOW_CLOSE,
    UI_EVENT_FOCUS_GAINED,
    UI_EVENT_FOCUS_LOST,
} UIEventType;

typedef struct {
    UIEventType type;
    uint8_t data[UI_EVENT_BUF_SIZE];  // 具体事件数据
} UIEvent;
```

`pollEvent` 回调填充 `UIEvent` 结构，UICornerstone 内部转换为现有事件系统。

---

## 7. 内部架构

### 7.1 CallbackAdapter 层

每个 adapter 实现对应的抽象接口，每个虚方法委托给回调表：

```cpp
// src/CallbackRenderDevice.h
class CallbackRenderDevice : public RenderDevice {
    UIRenderDeviceHandle m_handle;
    const UIBackendCallbacks* m_cbs;
public:
    CallbackRenderDevice(const UIBackendCallbacks* cbs, void* nativeContext);
    ~CallbackRenderDevice() override;

    void setDrawColor(SColor color) override;
    void fillRect(const SRect& rect) override;
    void drawRect(const SRect& rect) override;
    // ... 其他虚方法
};
```

```cpp
// src/CallbackAdapters.cpp
void CallbackRenderDevice::fillRect(const SRect& rect) {
    m_cbs->fillRect(m_handle, rect.left, rect.top, rect.width, rect.height);
}
```

> **实施说明**：SRect 成员名为 `.left/.top/.width/.height`（非 `.x/.y/.w/.h`），传递时需逐字段传入回调。drawTexture 等涉及 UIRect 结构体的场景需使用 `UIRect{rect.left, rect.top, rect.width, rect.height}` 聚合初始化。

Adapter 列表：

| 抽象接口 | Adapter 类 | 文件 |
|---------|-----------|------|
| `Window` | `CallbackWindow` | `src/CallbackAdapters.h/cpp` |
| `RenderDevice` | `CallbackRenderDevice` | 同上 |
| `InputBackend` | `CallbackInputBackend` | 同上 |
| `TextRenderer` | `CallbackTextRenderer` | 同上 |
| `ResourceProvider` | `CallbackResourceProvider` | 同上 |

> **实施记录**：所有 5 个 Adapter 合并到 `CallbackAdapters.h` + `CallbackAdapters.cpp` 两个文件中，避免过多源文件膨胀。

### 7.2 C ABI 控件工厂

```cpp
// src/UICornerstoneAPI.cpp
extern "C" UIControlHandle UICornerstone_CreateButton(
    const char* text, float x, float y, float w, float h)
{
    auto* ctl = new Button(BENCH, SRect(x, y, w, h));
    if (text) ctl->setCaption(text);
    return reinterpret_cast<UIControlHandle>(ctl);
}
```

> **实施说明**：Button/Label 以 Bench 为父节点，父指针不可为 nullptr。CheckBox 通过 `getCaption()->setCaption(text)` 设置文本，因为 CheckBox 的 caption 是独立的 Label 子控件。EditBox 无文本参数（文本由用户输入）。控件工厂不暴露 in-place 构造函数，先创建再设属性。

控件通过 `UIControlHandle`（即 `void*`）传递，内部 `reinterpret_cast` 为 `Control*`。保持 C ABI 的同时，复用所有现有 C++ 控件代码。

### 7.3 初始化流程

```
UICornerstone_Init(callbacks)
  ├─ 保存回调表指针到全局
  ├─ 如果 callbacks->createWindow 非 NULL:
  │    创建 OS 窗口 → setViewport(0,0,winW,winH)
  ├─ 否则:
  │    viewport 未设置，等待用户调用 UICornerstone_SetViewport
  ├─ 创建 CallbackRenderDevice
  ├─ 创建 CallbackTextRenderer
  ├─ 创建 CallbackInputBackend
  ├─ 创建 CallbackResourceProvider (如果有)
  ├─ 创建 Bench 作为根容器
  └─ 注册到 BackendManager
```

### 7.4 帧循环

```
UICornerstone_ProcessEvents()
  ├─ 从 inputBackend pollEvent 获取 UIEvent
  ├─ 转换为旧事件系统格式
  └─ 派发给 Bench → 递归到子控件

UICornerstone_Update(deltaTime)
  └─ 调用 Bench 的 update 逻辑

UICornerstone_Render()
  ├─ 设置 clip rect = viewport
  └─ 调用 Bench 递归绘制
     （不做 clear / present, 由调用者管理全帧生命周期）
```

> **重要**：`UICornerstone_Render` 只渲染视口区域，不调用 `clear()` 和 `present()`。调用者必须在外层自行管理帧缓冲区清除和交换。典型帧循环：
>
> ```c
> while (!UICornerstone_IsQuitRequested()) {
>     callbacks->clear(device, bgColor);       // 应用层清除
>     游戏引擎渲染 3D 场景...
>     UICornerstone_Render();                   // 仅绘制 UI 视口
>     callbacks->present(device);               // 应用层交换
> }
> ```

### 7.5 插件 DLL 加载

`UICornerstone_InitFromPlugin("sdl3")` 内部（R10b 起纯 `LoadLibrary`；1.12 起恢复静态回退）：

```c
int UICornerstone_InitFromPlugin(const char* pluginName) {
    if (!pluginName || !pluginName[0]) return 0;

    char dllName[128];
    snprintf(dllName, sizeof(dllName), "UIBackend_%s.dll", pluginName);
    HMODULE dll = LoadLibraryA(dllName);
    if (!dll) {
#if !UICORNERSTONE_BUILD_SHARED
        // 静态链接回退：UIBackend_*.dll 不存在时直连 GetUIBackendCallbacks
        extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);
        UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
        if (callbacks) return UICornerstone_Init(callbacks);
#endif
        return 0;
    }

    auto getter = (UIBackendCallbacks*(*)())GetProcAddress(dll, "GetUIBackendCallbacks");
    if (!getter) return 0;

    UIBackendCallbacks* callbacks = getter();
    if (!callbacks) return 0;

    return UICornerstone_Init(callbacks);
}
```

> **说明**：R10b 之后 `UICornerstone.dll` 不再包含任何后端源码，移除了静态回退。1.12 起通过 `#if !UICORNERSTONE_BUILD_SHARED` 守卫，在静态模式下恢复静态回退路径：`LoadLibrary` 失败时直接调用 `GetUIBackendCallbacks()`（该函数在静态模式下编译入 `UICornerstone.lib`）。DLL 模式下该代码被预处理器排除，`InitFromPlugin` 仍纯 `LoadLibrary` 路径。

---

## 8. CMake 构建模式

### 8.1 静态模式（默认，开发用，完全向后兼容）

```cmake
cmake -B build\sdl3 -DUICORNERSTONE_BACKEND=SDL3
# 编译 UICornerstone.lib（含 backend 源码）+ test 可执行文件
```

### 8.2 DLL 模式（通过选项开启）

```cmake
cmake -B build\sdl3_dll -DUICORNERSTONE_BACKEND=SDL3 -DUICORNERSTONE_BUILD_DLL=ON
# 编译:
#   UICornerstone.lib       — 静态库（test 链接此目标，无变化）
#   UICornerstone.dll       — 核心共享库（控件 + C ABI，无后端代码）
#   UIBackend_sdl3.dll      — 后端插件 DLL（渲染/输入/文本引擎）
#   test_xxx.exe            — 链接静态 UICornerstone.lib
#   test_api.exe            — 链接 UICornerstone.dll 导入库 + 复制 UIBackend_sdl3.dll
```

### 8.3 静态/DLL/插件关系

```
UICORNERSTONE_BUILD_DLL=OFF (默认):
  └─ UICornerstone (STATIC) — CORE_SOURCES + BACKEND_SOURCES 编译入库

UICORNERSTONE_BUILD_DLL=ON:
  ├─ UICornerstone (STATIC)       — 不变，供 test 链接
  ├─ UICornerstone_dll (SHARED)   — 仅 CORE_SOURCES，OUTPUT_NAME=UICornerstone
  │   导出 UICORNERSTONE_API 函数 + CORE_API 符号（registerFactories 等）
  └─ UIBackend_sdl3 (SHARED)     — 仅 BACKEND_SOURCES
      导出 GetUIBackendCallbacks，链接 UICornerstone_dll 导入库
```

> **设计决策**：核心 DLL 与后端插件 DLL 分离，后端插件通过 `CORE_API` 导出/导入机制调用核心的函数（`Surface::registerFactories`、`Cursor::registerFactories`、`ResourceProvider::createFilesystem`）。静态模式下 `CORE_API` 为空宏，不影响链接。

### 8.4 纯 DLL 集成（第三方游戏用）

游戏开发者只需：

```
game.exe + UICornerstone.dll + UICornerstone.lib + UICornerstoneAPI.h
```

游戏提供自己的 `UIBackendCallbacks` 实现，无需 `UIBackend_*.dll`。

如需插件模式（使用 SDL3/SFML/raylib 后端）：

```
game.exe + UICornerstone.dll + UIBackend_sdl3.dll + UICornerstone.lib + UICornerstoneAPI.h
```

调用 `UICornerstone_InitFromPlugin("sdl3")` 自动动态加载 `UIBackend_sdl3.dll`。

---

## 9. 实施步骤

### 9.1 实施顺序

| 步骤 | 内容 | 涉及文件 | 状态 |
|------|------|---------|------|
| **R1** | 重命名：CMake、编译定义、文档、AGENTS.md | ~20 文件 | ✅ 完成 |
| **R2** | 创建 `include/UICornerstoneAPI.h` | 1 新建 | ✅ 完成 |
| **R3** | 实现 5 个 CallbackAdapter 类（合并为 CallbackAdapters.h/cpp） | 2 新建 | ✅ 完成 |
| **R4** | 实现 `src/UICornerstoneAPI.cpp`（初始化+控件工厂+viewport） | 1 新建 | ✅ 完成 |
| **R5** | JSON 布局 C ABI 包装 | `src/UICornerstoneAPI.cpp` | ✅ 完成 |
| **R6** | BackendManager 改造：支持回调表初始化 | `BackendPlugin.h`, `BackendManager.cpp` | ✅ 完成 |
| **R7** | 3 个 BackendPlugin.cpp 改造：导出 `GetUIBackendCallbacks` | `BackendBridge.h`, 3×`BackendPlugin.cpp` | ✅ 完成 |
| **R8** | CMake DLL 模式分支 | `CMakeLists.txt`, `test/CMakeLists.txt`, `UICornerstoneAPI.h` | ✅ 完成 |
| **R9** | Bugfix + test_api.c C 测试 | `test/test_api.c`, 多个核心 C++ 文件 | ✅ 完成 |
| **R10** | 构建验证：3 后端 × 2 模式 | — | ✅ 完成 |
| **R10b** | **DLL 拆分：UICornerstone.dll + UIBackend_xxx.dll 插件** | 参见下方详细记录 | ✅ 完成 |

### 9.2 依赖关系

```
R1 (重命名) → 所有后续步骤
R2 (API 头文件) → R3, R4
R3 (Adapter) → R4 (C ABI 实现)
R4 → R5, R6, R7
R6 + R7 → R8 (CMake)
R8 → R9 (测试)
R9 → R10 (验证)
```

### 9.3 实施记录（R1–R4）

#### R1 重命名（已完成）

- 全部 20+ 文件完成 CMake 目标名、编译定义、包含守卫、项目名替换
- GitHub 6 个子模块仓同步改名 `UICornerstone-*`，`.gitmodules` URL 更新
- 构建验证：SDL3/SFML/raylib × 10 测试 = 30/30 全部通过

#### R2–R4 C ABI 实现（已完成）

| 发现 | 对应调整 |
|------|---------|
| SRect 成员名 `.x/.y/.w/.h` 不存在 | 全部改用 `.left/.top/.width/.height` |
| Button/Label 构造函数为 `(parent, SRect)`，不支持文本参数 | 先创建再 `setCaption(text)` |
| CheckBox 无 `setText()` 方法 | 通过 `getCaption()->setCaption(text)` 设置 |
| Button::setOnClick 接收 `std::function<void(shared_ptr<Button>)>` | Lambda 包装参数为 `shared_ptr<Button>` |
| LayoutParser 的接口名是 `parseLayout()` 而非 `parse()` | 使用正确方法名 |
| Event 类型在 CallbackAdapters.cpp 中不完整 | 添加 `#include "StateMachine.h"` |
| MSVC C4819 警告因文件非 UTF-8 BOM | 全部新文件写入后通过 PowerShell 添加 BOM |
| 5 个 Adapter 若各建独立文件则文件膨胀 | 合并为 `CallbackAdapters.h` + `CallbackAdapters.cpp` |

**验证结果**：`UICornerstone.lib` 编译 0 错误，全部 10 个 SDL3 测试无回归。

#### 初始化流程差异

设计文档假设 `BackendManager` 改造后才支持回调表路径，实际 R4 中 `UICornerstone_Init` 直接管理全局 Adapter 实例，不经过 BackendManager。BackendManager 改造（R6）将作为后续独立步骤。

#### R5 JSON 布局 C ABI 包装（已完成）

`UICornerstone_LoadLayout` 完成以下操作：

1. 遍历 `g_actions` 映射，通过 `parser.registerHandler(name, lambda)` 将 C ABI 的 `UIActionCallback` 适配为 LayoutParser 期望的 `function<void(shared_ptr<Control>)>`
2. 调用 `parser.parseLayout(jsonContent)` 解析 JSON
3. 根控件通过 `BENCH->addControl(root)` 注册到 Bench
4. MenuBar 通过 `BENCH->addControl(mb)` 逐个添加
5. 遍历 `parser.getAllControlIds()`，每个带 ID 的控件注册到 `g_controlsById`，供 `UICornerstone_FindControl` 查找

**验证**：编译通过，全部 10 个 SDL3 测试无回归。

#### R6 BackendManager 回调表初始化（已完成）

`BackendManager::initialize(const UIBackendCallbacks* callbacks)` 新增：
- 接收 C ABI 回调查表，创建 5 个 CallbackAdapter 实例（Window/RenderDevice/TextRenderer/InputBackend/ResourceProvider）
- 适配器实例通过标准访问器 `window()` / `renderDevice()` / `textRenderer()` / `inputBackend()` 对外暴露
- 与现有的 `initialize(backendName, ...)`（内置后端）共存

**`UICornerstone_Init` 改造**：
- 不再直接管理适配器创建，改为委托 `BackendManager::instance()->initialize(callbacks)`
- 全局状态（`g_renderDevice` 等）改为抽象指针类型（`RenderDevice*` 而非 `CallbackRenderDevice*`），仅缓存 BackendManager 返回的指针
- 注销流程改为 `BackendManager::instance()->shutdown()` + `delete g_resourceProvider`
- 修复 `BackendManager::shutdown()` 中 `m_renderDevice` 未 delete 的泄漏（回调查表路径下删除，内置路径保持原有行为）

**验证**：编译通过，全部 10 个 SDL3 测试无回归。

#### R7 BackendPlugin 导出 GetUIBackendCallbacks（已完成）

`src/backend/BackendBridge.h` 桥接函数将 C ABI 回调查表委托回 C++ 抽象接口：

| 模块 | 桥接函数 |
|------|---------|
| Window | `bridge_destroyWindow`, `bridge_getWindowSize`, `bridge_getWindowPosition`, `bridge_getDisplayWidth/Height`, `bridge_getDpiScale`, `bridge_setWindowTitle`, `bridge_getMousePosition` |
| RenderDevice | `bridge_destroyRenderDevice`, `bridge_setDrawColor`, `bridge_setBlendMode`, `bridge_setClipRect/clearClipRect`, `bridge_fillRect/drawRect/drawLine/drawPoint`, `bridge_clear/present/flush`, `bridge_getNativeHandle`, `bridge_createTextureFromFile/destroyTexture/drawTexture/getTextureSize` |
| InputBackend | `bridge_destroyInputBackend`, `bridge_pollEvent`（事件转换 12 种类型）, `bridge_startTextInput/stopTextInput`, `bridge_getModState` |
| TextRenderer | `bridge_destroyTextRenderer`, `bridge_loadFont/loadFontFromMemory/destroyFont`, `bridge_measureTextWidth/getFontHeight/drawText/drawTextWrapped` |
| ResourceProvider | `bridge_destroyResourceProvider`, `bridge_readFile`, `bridge_fileExists` |

每个后端通过 `plugin_createWindow/RenderDevice/TextRenderer/InputBackend` 静态工厂创建对象，填入 `UIBackendCallbacks` 表。

桥接函数使用 `reinterpret_cast` 将 `void*` 句柄转为 C++ 抽象接口指针，调用虚方法。`Texture` 和 `Font` 通过堆分配的 `shared_ptr` 管理生命周期。

**实现文件**：
- `src/backend/BackendBridge.h` — 250 行 inline 桥接函数（共享）
- `src/backend/sdl3/BackendPlugin.cpp` — 已添加 `GetUIBackendCallbacks()`
- `src/backend/sfml/BackendPlugin.cpp` — 已添加
- `src/backend/raylib/BackendPlugin.cpp` — 已添加

**验证**：编译通过，全部 10 个 SDL3/SFML/raylib 测试无回归。

#### R8 CMake DLL 模式分支（已完成）

新增 `UICORNERSTONE_BUILD_DLL` CMake 选项（BOOL，默认 OFF）：

| 模式 | 目标 | 类型 | 用途 |
|------|------|------|------|
| OFF | `UICornerstone` | STATIC | 测试/开发（现有行为不变） |
| ON | `UICornerstone` | STATIC | 同上，保持向后兼容 |
| ON | `UICornerstone_dll` | SHARED | 导出 C ABI 函数的 DLL |

**UICornerstoneAPI.h 导出宏**：

```c
#if defined(UICORNERSTONE_BUILD_SHARED)
#  if defined(UICORNERSTONE_API_EXPORT)
#    define UICORNERSTONE_API __declspec(dllexport)
#  else
#    define UICORNERSTONE_API __declspec(dllimport)
#  endif
#else
#  define UICORNERSTONE_API
#endif
```

- 静态模式：`UICORNERSTONE_API` 为空
- DLL 模式：所有 `UICornerstone_*` 函数标记 `__declspec(dllexport)`

**CMake 关键变更**：

```cmake
add_library(UICornerstone STATIC ${CORE_SOURCES} ${BACKEND_SOURCES})

if(UICORNERSTONE_BUILD_DLL)
    add_library(UICornerstone_dll SHARED ${CORE_SOURCES} ${BACKEND_SOURCES})
    target_compile_definitions(UICornerstone_dll
        PRIVATE UICORNERSTONE_API_EXPORT=1
        PUBLIC UICORNERSTONE_BUILD_SHARED=1)
endif()
```

> **设计决策**：两个目标编译两次源码而非 OBJECT 库。优点是 test 完全不感知 DLL 模式、CMake 结构清晰、导出控制精确。缺点约增加 60~100% 的编译时间（~3 分钟/后端）。

**验证**：静态模式 10/10 测试通过，DLL 模式 3 后端均生成 `UICornerstone_dll.dll`（SDL3: 3.3MB）。

#### R10b DLL 拆分：UICornerstone.dll + UIBackend_xxx.dll 插件（已完成）

**问题**：R8 的 DLL 模式将 `UICornerstone_dll.dll` 包含全部 CORE_SOURCES + BACKEND_SOURCES，后端默认与核心 DLL 耦合，无法动态切换后端。

**方案**：

| 目标 | 类型 | 内容 |
|------|------|------|
| `UICornerstone_dll` (→ `UICornerstone.dll`) | SHARED | CORE_SOURCES 仅（控件、布局、C ABI） |
| `UIBackend_sdl3` (→ `UIBackend_sdl3.dll`) | SHARED | BACKEND_SOURCES 仅（渲染/输入/文本/Cursor） |

**关键技术障碍和解决方案**：

1. **静态工厂方法跨 DLL 符号解析**：`Surface::create()` / `Cursor::createSystem()` 等静态方法实现在后端源码中，但被核心控件直接调用。
   - **解决**：移到核心源文件 `src/Surface.cpp` / `src/Cursor.cpp`，通过 `registerFactories` 委托机制注册。
   - 后端 `BackendPlugin.cpp` 在 `GetUIBackendCallbacks()` 中调用 `RegisterSDL3SurfaceFactories()` / `RegisterSDL3CursorFactories()`。
   - 静态模式下 `BackendManager::initialize(string)` 也调用相同注册函数。

2. **导出 `registerFactories` 供后端插件调用**：`UIBackend_sdl3.dll` 需调用 `Surface::registerFactories(...)` 等方法将工厂函数指针注册到 `UICornerstone.dll`。
   - **解决**：新增 `CORE_API` 宏（`__declspec(dllexport)` / `__declspec(dllimport)`），由 `UICORNERSTONE_CORE_API_EXPORT` 控制。
   - `Surface.h`、`Cursor.h`、`ResourceProvider.h` 使用 `CORE_API`。
   - `UIBackend_xxx` 目标链接 `UICornerstone_dll` 导入库，定义 `UICORNERSTONE_BUILD_SHARED=1`。

3. **`InitFromPlugin` 静态回退**：R7 设计的静态回退路径在 DLL 拆分后无法链接。
   - **R10b 解决**：完全移除静态回退，`InitFromPlugin` 纯 `LoadLibrary` 路径。
   - **1.12 恢复**：SFML/Raylib 静态构建模式下 `UIBackend_*.dll` 不存在，通过 `#if !UICORNERSTONE_BUILD_SHARED` 守卫恢复静态回退。`InitFromPlugin` 在 `LoadLibrary` 失败后直接调用 `GetUIBackendCallbacks()`（静态模式下该函数编译入 `UICornerstone.lib`）。

4. **`BackendManager::initialize(string)` 外部符号**：`g_sdl3Backend` 等符号在 DLL 模式下不存在。
   - **解决**：用 `#if !defined(UICORNERSTONE_BUILD_SHARED)` 守卫，DLL 模式下该路径编译为空。

**验证**：

| 模式 | SDL3 | SFML | Raylib |
|------|------|------|--------|
| 静态 (UICornerstone.lib) | 10/10 测试 | 10/10 测试 | 10/10 测试 |
| DLL (UICornerstone.dll) | test_fromsource_cabi ALL PASS | test_fromsource_cabi ALL PASS | test_fromsource_cabi ALL PASS |
| API 测试 | test_api 6/6 全过 | test_api 6/6 全过 | test_api 6/6 全过 |

test_api 输出：
```
UICornerstone: loaded UIBackend_sdl3.dll
BackendManager: initialized from callback table
UICornerstone initialized
Loading layout...
UICornerstone: LoadLayout OK (18 control ids, 0 menu bars)
FindControl: 18/18 found
```

### 9.4 Fromsource 集成模式（2026-06-13 ~ 06-19 新增）

#### 9.4.1 动机

除了 DLL + 回调路径，部分场景希望后端从源码编译进 EXE（而非插件 DLL）：

- **单文件编译调试**：后端 `.cpp` 通过 `#include` 纳入测试 exe，无需 DLL 构建
- **窗口复用**：SDL3 回调模式下复用已有 SDL 窗口和渲染器，免去二次窗口创建
- **避免跨 DLL 符号解析**：Surface/Cursor 工厂函数通过静态链接直接调用，无需 `CORE_API` 导出

#### 9.4.2 架构

```
test_fromsource_cabi.exe
  ├── 动态加载: LoadLibrary("UICornerstone.dll")
  │     → GetProcAddress 解析所有 C ABI 函数指针
  │     → UICornerstone_Init(callbacks) 传入回调查表
  ├── 源码编译（独立 TU）:
  │     → BackendPlugin.cpp, RenderDevice.cpp, Window.cpp,
  │       InputBackend.cpp, TextRenderer.cpp, Cursor.cpp
  │     → GetUIBackendCallbacks() 填入回调表
  ├── 控件工厂: UICornerstone.dll 的 C ABI 函数
  └── 帧循环: ProcessEvents → Update → Clear → Render → Present
```

fromsource 测试已重构为单源文件模式，通过 `BACKEND_SHORT_NAME`/`BACKEND_DISPLAY_NAME` 编译定义区分后端，后端源码作为独立 TU 编译：

| 文件 | 说明 |
|------|------|
| `test/test_fromsource_cabi.cpp` | 三后端 C ABI 集成测试，`LoadLibrary` + `main()` 帧循环 |
| `test/test_dialog_cabi.cpp` | 三后端 C ABI Dialog 测试，`LoadLibrary` + JSON dialogs |
| `test/test_combobox_cabi.cpp` | 三后端 C ABI ComboBox 测试，`LoadLibrary` + JSON ComboBox |

CMake 通过 `add_fromsource_target` 宏统一创建目标，自动注入后端编译定义和链接库。

#### 9.4.3 后端工厂注册

`UICornerstone.dll` 中的 `Surface::loadFromFile()` / `Cursor::createSystem()` 等静态工厂函数依赖后端回调。fromsource 模式下通过 `GetUIBackendCallbacks()` 中的 `Register*Factories()` 完成注册：

| 工厂 | 注册函数 | 调用时机 |
|------|---------|---------|
| `Surface::create` / `loadFromFile` / `loadFromMemory` | `RegisterSDL3SurfaceFactories()` / `RegisterSFMLSurfaceFactories()` / `RegisterRaylibSurfaceFactories()` | `BackendPlugin.cpp` 的 `GetUIBackendCallbacks` |
| `Cursor::createSystem` / `getDefault` / `setCurrent` | `RegisterSDL3CursorFactories()` / `RegisterSFMLCursorFactories()` / `RegisterRaylibCursorFactories()` | 同上 |

静态模式下（`UICORNERSTONE_BUILD_SHARED=0`），`BackendManager::initialize(string)` 也在 `BackendPlugin.cpp` 路径下调用相同注册函数。

#### 9.4.4 关键修复项（2026-06-15 ~ 06-19）

| 修复 | 根因 | 文件 |
|------|------|------|
| SFML/WinFrame 关闭按钮 X 不显示 | Surface 工厂未注册 → `loadFromFile` 返回 nullptr | `RenderDevice.cpp` 新增 `RegisterSFMLSurfaceFactories` |
| SFML 事件响应慢 | VSync 开启阻塞 `display()` | `Window.cpp` 添加 `setVerticalSyncEnabled(false)` |
| Raylib 窗体"未响应" | `PollInputEvents()` 从未被调用 | `UICornerstoneAPI.h` 新增 `newFrame` 回调 + `CallbackInputBackend::newFrame` |
| Raylib 中文显示"?" | 字体码点懒加载时新 `shared_ptr` 替换旧对象，Brige 句柄悬空 | `RaylibFont::reload()` 原地重载 |
| SFML 事件响应慢（二次） | Label::recreate 每帧 ~5-10MB 字体文件磁盘 I/O | `Label::loadFromResource()` 缓存命中提前返回 |
| RaYlib `DrawTexturePro` DLL 桥接不可见 | `DrawTexturePro` 在跨 DLL 调用链下失效 | 改用 `rlPushMatrix + rlScalef + DrawTextureEx` |
| raylib texture src=(0,0,0,0) | `bridge_drawTexture` 将 `nullptr` src 转为零 SRect | `BackendBridge.h` 不再转换 `nullptr` |
| SFML `Actor::setParent` 覆盖纹理 | `setParent` 无条件调用 `createTexture` 覆盖已有纹理 | `src/Actor.cpp` 添加 `&& !m_texture` 保护 |
| WinFrame 关闭按钮 X 不显示（跨后端） | PNG 纹理在部分后端渲染不可见 | `WinFrame::draw()` 新增向量 X 叠加层（6 线粗） |
| Actor PNG 加载回退 | `Surface::loadFromFile()` 桥接路径返回 nullptr | `Actor::loadFromFile()` 回退 `createTextureFromFile` |

#### 9.4.5 回调查表扩展

fromsource 测试暴露了回调查表的不完整，以下字段在 R9 之后追加：

| 字段 | 类型 | 说明 |
|------|------|------|
| `newFrame` | `void (*)(UIInputBackendHandle)` | 帧开始事件轮询前调用（Raylib 需要 `PollInputEvents()`） |
| `fillTriangle` | `void (*)(UIRenderDeviceHandle, float x1,float y1, float x2,float y2, float x3,float y3, UIColor)` | 实心三角形（Bridge 路径下 `drawTriangle` 退化） |
| `fillQuad` | `void (*)(UIRenderDeviceHandle, float x1,float y1, float x2,float y2, float x3,float y3, float x4,float y4, UIColor)` | 实心四边形 |
| `setClipboardText` | `void (*)(UIInputBackendHandle, const char*)` | 剪贴板写入 |
| `getClipboardText` | `const char* (*)(UIInputBackendHandle)` | 剪贴板读取 |

### 9.5 回退计划

任何步骤出现问题：

- **R1 重命名错误**：通过 git diff 确认所有替换正确，可回退单个文件
- **R2–R5 新代码错误**：不修改现有代码，仅添加新文件，不影响现有构建
- **R6–R8 后端/CMake 改造**：静态模式（`UICORNERSTONE_BUILD_DLL=OFF`）完全不影响，仅影响 DLL 模式。DLL 目标为独立 `add_library(... SHARED)`，不存在任何 `#ifdef` 条件，完全隔离。

---

## 10. 附录：集成场景示例

### 场景 1：自研引擎（目标场景）

这是 UICornerstone 的核心目标场景——使用自研 C++ 渲染引擎的团队，需要快速添加一套声明式 UI。

```c
// MyEngine_UI.c — 自研 C++ 引擎集成

// ---- 引擎侧渲染后端 ----
// 自研引擎有自己的一套 RenderDevice 封装（OpenGL / DirectX / Metal / Vulkan）

static UIRenderDeviceHandle MyEngine_CreateRenderDevice(void* nativeContext) {
    // nativeContext = 引擎的 GraphicsContext 或 Device
    return (UIRenderDeviceHandle)MyEngine::GetDevice();
}

static void MyEngine_FillRect(UIRenderDeviceHandle device, UIRect r) {
    MyEngine::DrawQuad(r.x, r.y, r.w, r.h, MyEngine::GetCurrentColor());
}

static void MyEngine_DrawText(UITextRendererHandle tr, void* font,
    const char* text, float x, float y, UIColor color)
{
    MyEngine::DrawString((MyFont*)font, text, x, y, color);
}

static void MyEngine_Present(UIRenderDeviceHandle) {
    // UICornerstone 的命令已通过 fillRect/drawText 等即时执行
    // 无需额外 present——引擎自有 SwapChain
}

// ---- 引擎侧输入转发 ----
// 引擎每帧收集输入事件并推入队列

typedef struct { UIEvent events[64]; int count; } EventRingBuffer;

static UIInputBackendHandle MyEngine_CreateInputBackend(void* nativeWindow) {
    return calloc(1, sizeof(EventRingBuffer));
}

static int MyEngine_PollEvent(UIInputBackendHandle ib, void* eventBuf) {
    EventRingBuffer* buf = (EventRingBuffer*)ib;
    if (buf->count > 0) {
        memcpy(eventBuf, &buf->events[--buf->count], sizeof(UIEvent));
        return 1;
    }
    return 0;
}

void MyEngine_FlushInputToUICornerstone() {
    EventRingBuffer* buf = (EventRingBuffer*)UICornerstone_GetInputBackend();
    // 从引擎获取鼠标/键盘事件
    while (buf->count < 64 && MyEngine::PollRawEvent(&buf->events[buf->count])) {
        buf->count++;
    }
}

// ---- 初始化与帧循环 ----

UIBackendCallbacks g_callbacks = {
    1,
    NULL,                           // createWindow = NULL — 引擎管窗口
    MyEngine_CreateRenderDevice,
    MyEngine_DestroyRenderDevice,
    MyEngine_SetDrawColor,
    MyEngine_FillRect,
    MyEngine_DrawRect,
    MyEngine_DrawText,
    MyEngine_Present,
    // ...其余回调
};

int main() {
    MyEngine::Init("My Game", 1920, 1080);

    UICornerstone_Init(&g_callbacks);
    UICornerstone_SetViewport(100, 100, 800, 600);
    UICornerstone_LoadLayoutFromFile("ui/main_menu.json");

    while (MyEngine::IsRunning()) {
        MyEngine::BeginFrame();

        MyEngine_FlushInputToUICornerstone();
        UICornerstone_ProcessEvents();
        UICornerstone_Update(MyEngine::GetDeltaTime());
        UICornerstone_Render();             // 仅绘制 UI viewport 区域

        MyEngine::EndFrame();               // 引擎自行 Present
    }

    UICornerstone_Shutdown();
    return 0;
}
```

### 场景 2：轻量工具（像素缓冲区——即时集成）

适用于：调试覆盖层（overlay）、编辑器工具、分析仪表盘等。不依赖任何图形 API，直接输出 RGBA 像素。

```c
// Tool_Overlay.c

static uint8_t s_pixelBuf[1024 * 768 * 4];

static UIRenderDeviceHandle Tool_CreateRenderDevice(void* nativeDevice) {
    memset(s_pixelBuf, 0, sizeof(s_pixelBuf));
    return (UIRenderDeviceHandle)s_pixelBuf;
}

static void Tool_FillRect(UIRenderDeviceHandle buf, UIRect r) {
    uint8_t* pixels = (uint8_t*)buf;
    int vpW = 1024; // viewport width
    for (int y = (int)r.y; y < (int)(r.y + r.h); y++)
        for (int x = (int)r.x; x < (int)(r.x + r.w); x++) {
            int idx = (y * vpW + x) * 4;
            pixels[idx]   = g_curColor.r;
            pixels[idx+1] = g_curColor.g;
            pixels[idx+2] = g_curColor.b;
            pixels[idx+3] = g_curColor.a;
        }
}

static void Tool_Present(UIRenderDeviceHandle buf) {
    // 像素缓冲区已就绪，外部可读（写入文件、或通过共享内存传给宿主进程）
    g_pixelsReady = true;
}

int main() {
    UIBackendCallbacks cb = { 1,
        NULL,                   // 无窗口
        Tool_CreateRenderDevice,
        Tool_FillRect,
        Tool_DrawText,
        Tool_Present,
        // ...
        NULL                    // 无输入回掉—静态面板
    };

    UICornerstone_Init(&cb);
    UICornerstone_SetViewport(0, 0, 1024, 768);
    UICornerstone_LoadLayoutFromFile("ui/overlay.json");
    UICornerstone_Render();      // 渲染到 s_pixelBuf

    // 保存为 PNG
    stbi_write_png("overlay_output.png", 1024, 768, 4, s_pixelBuf, 1024*4);
    return 0;
}
```

### 场景 3：使用插件 DLL（测试程序）

适用于无需自研引擎、直接用 SDL3/SFML/raylib 做原型的开发者。

```c
// test_menu.cpp

```c
// test/test_menu.cpp
int main() {
    UICornerstone_InitFromPlugin("sdl3");
    UICornerstone_SetViewport(0, 0, 1024, 768);
    UICornerstone_LoadLayoutFromFile("layouts/test_menu.json");

    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0/60.0);
        UICornerstone_Render();
    }
    UICornerstone_Shutdown();
    return 0;
}
```

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2026-06-12 | 初版 — 草案待确认 |
| 1.1 | 2026-06-12 | R2~R4 编码完成：更新 Adapter 文件结构（合并为 CallbackAdapters.h/cpp）、修正 SRect 成员名示例、修正控件工厂示例、标记进度状态 |
| 1.2 | 2026-06-12 | R5 编码完成 + Render 修正：LoadLayout 实现（Action 绑定 + ID 注册）、Render 移除 clear/present |
| 1.3 | 2026-06-12 | R6 编码完成：BackendManager 回调表初始化路径、UICornerstone_Init 委托重构、RenderDevice 泄漏修复 |
| 1.4 | 2026-06-13 | R7 编码完成：BackendBridge.h 桥接函数、3 后端 GetUIBackendCallbacks 导出、InitFromPlugin 实现。R8 编码完成：UICORNERSTONE_BUILD_DLL 选项、UICORNERSTONE_API 导出宏、独立 UICornerstone_dll 目标 |
| 1.5 | 2026-06-13 | R10b DLL 拆分：UICornerstone.dll 仅含 CORE_SOURCES，UIBackend_sdl3.dll 独立后端插件；Surface/Cursor 静态工厂委托注册；CORE_API 跨 DLL 导出机制 |
| 1.6 | 2026-06-14 | test_fromsource — 单文件编译 + 窗口复用 + 控件可见性修复 + 事件注入机制 |
| 1.7 | 2026-06-15 | fromsource 4 bug 修复：Surface 工厂注册、newFrame 桥接、SFML vsync、Raylib 窗体事件；RGBA8888 像素格式确认 |
| 1.8 | 2026-06-15 | 三后端 fromsource 架构切换（Separate TU 编译），避免 SFML `<windows.h>` 宏污染 |
| 1.9 | 2026-06-16 | WinFrame 向量 X 叠加层（`draw()` override）；Actor `loadFromFile` 回退 `createTextureFromFile`；Raylib 字体 `reload()` 原地重载；回调查表新增 `fillTriangle/fillQuad/setClipboardText/getClipboardText` |
| 1.10 | 2026-06-18 | Raylib `DrawTexturePro` DLL 桥接不可见修复：改用 `rlPushMatrix + rlScalef + DrawTextureEx` |
| 1.11 | 2026-06-19 | SFML fromsource 纹理不可见修复（`Actor::setParent` 保护 + `sf::Sprite`）；SFML 事件响应慢修复（Label recreate 字体缓存优化） |
| 1.12 | 2026-06-20 | SFML/Raylib 静态+DLL 双构建目录（`build/{sfml,raylib}` + `build/{sfml,raylib}_dll`）；`test_fromsource.cpp` → `test_fromsource_sdl3.cpp`；`InitFromPlugin` 恢复静态回退（`#if !UICORNERSTONE_BUILD_SHARED`）；`test_api.c` 改用 `UICORNERSTONE_BACKEND_NAME` 编译定义替代硬编码 `"sdl3"` |
| 1.13 | 2026-07-12 | Dialog C ABI API（`CreateDialog/Show/Close/SetOnConfirm/SetOnCancel/SetOnClose` 等 11 个函数）；`UIBackendCallbacks` 新增 `createSystemCursor/getDefaultCursor/setCurrentCursor` 光标工厂回调；`test_dialog_cabi` 三后端单源文件测试（共享头文件→合并为单一 `.cpp`，后端独立 TU 编译）；`windows.h` 冲突工作区（`#ifndef _WINDOWS_` 条件式手动 Win32 API 声明，后因独立 TU 编译移除） |
| 1.14 | 2026-07-15 | ComboBox C ABI API（`CreateComboBox`/`ComboBoxSetItems`/`ComboBoxSetSelectedIndex`/`ComboBoxGetSelectedIndex`/`ComboBoxGetSelectedLabel`/`ComboBoxSetOnSelectionChanged`）；鼠标滚轮事件桥接新增 x/y 坐标；ComboBox JSON 布局解析；`test_combobox_cabi` 三后端单源文件测试 |
| 1.15 | 2026-07-16 | 重构：`test_fromsource_cabi`、`test_dialog_cabi`、`test_combobox_cabi` 统一为单源文件 + 编译定义模式，删除共享头文件和后端变体文件；CMake 改用 `add_fromsource_target` 宏统一管理 |
