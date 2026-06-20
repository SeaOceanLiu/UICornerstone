# Sample 示例代码设计

## 1. 设计原则

- **逐步扩展**：从声明式 UI（JSON 布局）→ 命令式 UI（C ABI 工厂函数） → 混合集成（核心 DLL + 后端源码），逐步增加复杂度
- **集成路径**：完全静态链接 `UICornerstone.lib`（含后端源码），零 DLL 依赖
- **代码量最小**：~50 行，单文件，纯 C
- **UI 定义**：声明式（JSON 布局）或编程式（C ABI 工厂函数）
- **交互演示**：Button 点击 → Label 更新

## 2. 后端选择

UICornerstone 支持三个后端，编译时通过 `DUICORNERSTONE_BACKEND` 选择：


| 后端   | CMake 变量 | 依赖                                | 特点                         |
| ------ | ---------- | ----------------------------------- | ---------------------------- |
| SDL3   | `SDL3`     | SDL3 + SDL3_ttf + SDL3_image        | 主开发后端，功能最全，跨平台 |
| SFML   | `SFML`     | SFML 3.x (Graphics, Window, System) | 替代选择                     |
| Raylib | `RAYLIB`   | raylib 6.0                          | 最小依赖，单库               |

本 sample 默认使用 SDL3。换后端只需改 CMake 变量：

```batch
cmake -B build/sdl3 -DUICORNERSTONE_BACKEND=SDL3
cmake -B build/sfml -DUICORNERSTONE_BACKEND=SFML
cmake -B build/raylib -DUICORNERSTONE_BACKEND=RAYLIB
```

代码无需修改。

## 3. 集成架构

### hello_uicornerstone（声明式 UI——JSON 布局）

```
hello_uicornerstone.exe
  └── 静态链接:
        ├── UICornerstone.lib (CORE_SOURCES + BACKEND_SOURCES)
        │     ├── 控件 (Button, Label, Panel...)
        │     ├── 布局系统 (LayoutParser)   ← 解析 JSON 生成控件树
        │     ├── C ABI (UICornerstone_* 函数)
        │     ├── BackendPlugin.cpp (GetUIBackendCallbacks)
        │     └── 后端 (Window, RenderDevice, TextRenderer, InputBackend, Cursor)
        ├── SDL3.lib (第三方后端库)
        └── SDL3_ttf.lib
```

数据流：`main()` → `UICornerstone_LoadLayout(JSON)` → `LayoutParser::parseLayout()` → 控件树 → `BENCH->addControl()`

输出路径：`build/sample/hello_uicornerstone/<backend>/Debug/`

### sample_programmatic（命令式 UI——C ABI 工厂函数）

```
sample_programmatic.exe
  └── 静态链接:
        ├── UICornerstone.lib (CORE_SOURCES + BACKEND_SOURCES)
        │     ├── 控件 (Button, Label, Panel...)
        │     ├── C ABI (UICornerstone_* 函数)   ← 工厂函数直接创建控件
        │     ├── BackendPlugin.cpp (GetUIBackendCallbacks)
        │     └── 后端 (Window, RenderDevice, TextRenderer, InputBackend, Cursor)
        ├── SDL3.lib (第三方后端库)
        └── SDL3_ttf.lib
```

数据流：`main()` → `UICornerstone_CreateButton/CreateLabel/...` → 控件树 → `UICornerstone_AddChild()` 挂接父子关系

输出路径：`build/sample/sample_programmatic/<backend>/Debug/`

### sample_fromsource（混合集成——核心 DLL + 后端源码）

```
sample_fromsource.exe
  ├── 静态链接的后端源码:
  │     ├── Window.cpp, RenderDevice.cpp, TextRenderer.cpp
  │     ├── InputBackend.cpp, Cursor.cpp, BackendPlugin.cpp
  │     └── GetUIBackendCallbacks() → 返回回调查表
  ├── 运行时加载:
  │     └── UICornerstone.dll (ILT 隐式加载)   ← 核心控件 + C ABI
  ├── SDL3.lib (第三方后端库)
  └── SDL3_ttf.lib
```

数据流：`main()` → `GetUIBackendCallbacks()`(exe) → `UICornerstone_Init(callbacks)`(DLL) → 工厂函数(通过ILT调用DLL) → 控件树

输出路径：`build/sample/sample_fromsource/<backend>/Debug/`

**特色**：后端编译进 exe，核心 DLL 通过导入表（ILT）链接。exe 272KB，DLL 3.2MB。需 `UICORNERSTONE_BUILD_DLL=ON`。

**零外部 DLL**：前两个示例是单个 exe，双击即运行。第三个需要 `UICornerstone.dll` 同目录。

### sample_loadlibrary（显式 LoadLibrary + #include 后端源码）

```
sample_loadlibrary.exe
  ├── #include 后端 6 个 .cpp（同一 TU）:
  │     ├── Window.cpp, RenderDevice.cpp, TextRenderer.cpp
  │     ├── InputBackend.cpp, Cursor.cpp, BackendPlugin.cpp
  │     └── GetUIBackendCallbacks() → 回调查表
  ├── 内联 3 个 Core 符号（替代导入库）:
  │     ├── Surface::registerFactories (no-op)
  │     ├── Cursor::registerFactories (no-op)
  │     └── ResourceProvider::createFilesystem (完整实现)
  ├── LoadLibrary("UICornerstone.dll")       ← C ABI 函数指针
  ├── SDL3.lib (第三方后端库)
  └── 不链接 UICornerstone_dll.lib
```

数据流：`main()` → `LoadLibrary` → `GetProcAddress`(每个函数) → `GetUIBackendCallbacks()`(exe内) → `uiInit(callbacks)`(函数指针→DLL)

输出路径：`build/sample/sample_loadlibrary/<backend>/Debug/`

**特色**：零导入库依赖。C ABI 全部走运行时函数指针。内联 3 个 Core 符号满足链接器。需 `UICORNERSTONE_BUILD_DLL=ON`。

## 4. hello_uicornerstone.c（声明式 UI——JSON 布局）

### 代码结构（~50 行）

```c
#include "UICornerstoneAPI.h"
#include <stdio.h>

/* ======== 回调函数 ======== */
static int g_clickCount = 0;

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;
    g_clickCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_clickCount);
    UIControlHandle status = UICornerstone_FindControl("status");
    if (status) UICornerstone_SetText(status, buf);
}

/* ======== JSON 布局 ======== */
static const char* LAYOUT =
"{"
"  \"controls\": [{"
"    \"type\": \"Panel\", \"id\": \"root\","
"    \"rect\": {\"x\":0,\"y\":0,\"w\":800,\"h\":480},"
"    \"children\": ["
"      {\"type\":\"Label\",\"id\":\"title\","
"       \"rect\":{\"x\":20,\"y\":10,\"w\":760,\"h\":30},"
"       \"font\":{\"size\":18},\"caption\":\"UICornerstone Sample\"},"
"      {\"type\":\"Button\",\"id\":\"btn\",\"caption\":\"Click Me\","
"       \"rect\":{\"x\":20,\"y\":60,\"w\":200,\"h\":80},"
"       \"colors\":{\"background\":{\"normal\":\"#4A90D9\",\"hover\":\"#5BA0E9\",\"pressed\":\"#3A80C9\"}},"
"       \"events\":{\"onClick\":\"onBtnClick\"}},"
"      {\"type\":\"Label\",\"id\":\"status\","
"       \"rect\":{\"x\":20,\"y\":160,\"w\":400,\"h\":24},"
"       \"caption\":\"Click the button above\"}"
"    ]"
"  }]}";

/* ======== main ======== */
int main(void) {
    if (!UICornerstone_InitFromPlugin(UICORNERSTONE_BACKEND_NAME)) return 1;
    UICornerstone_SetViewport(0, 0, 800, 480);
    UICornerstone_RegisterAction("onBtnClick", onBtnClick, NULL);

    if (!UICornerstone_LoadLayout(LAYOUT)) {
        UICornerstone_Shutdown(); return 1;
    }

    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0 / 60.0);
        UICornerstone_Clear();
        UICornerstone_Render();
        UICornerstone_Present();
    }

    UICornerstone_Shutdown();
    return 0;
}
```

### 不包含

- `LoadLibrary` / `GetProcAddress` / 函数指针 typedef
- SDL 回调模式
- 事件注入
- 窗口复用
- 多控件演示（仅 Button + 2 个 Label）

## 5. sample_programmatic.c（命令式 UI（C ABI 工厂函数））

### 与声明式 UI（JSON 布局）的区别


| 维度     | hello_uicornerstone（声明式 UI——JSON 布局） | sample_programmatic（命令式 UI（C ABI 工厂函数）） |
| -------- | --------------------------------------------- | -------------------------------------------------- |
| UI 定义  | 内联 JSON 字符串                              | C ABI 工厂函数                                     |
| 布局解析 | `LayoutParser::parseLayout()`                 | 无                                                 |
| 控件查找 | `UICornerstone_FindControl` 按 ID             | 全局变量保存句柄                                   |
| 按钮颜色 | JSON`colors.background`                       | `UICornerstone_SetBGColor`                         |
| 代码量   | ~50 行                                        | ~45 行                                             |

### 代码结构（~45 行）

```c
#include "UICornerstoneAPI.h"
#include <stdio.h>

static int g_clickCount = 0;
static UIControlHandle g_statusLabel = NULL;

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;
    g_clickCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_clickCount);
    if (g_statusLabel)
        UICornerstone_SetText(g_statusLabel, buf);
}

int main(void) {
    if (!UICornerstone_InitFromPlugin(UICORNERSTONE_BACKEND_NAME))
        return 1;
    UICornerstone_SetViewport(0, 0, 800, 480);

    UIControlHandle root = UICornerstone_CreatePanel(0,0,800,480);

    UIControlHandle title = UICornerstone_CreateLabel(
        "UICornerstone Sample (Programmatic)", 18,
        20, 10, 760, 30);
    UICornerstone_AddChild(root, title);

    UIControlHandle btn = UICornerstone_CreateButton(
        "Click Me", 20, 60, 200, 80);
    UICornerstone_SetBGColor(btn, 74, 144, 217, 255);
    UICornerstone_SetOnClick(btn, onBtnClick, NULL);
    UICornerstone_AddChild(root, btn);

    g_statusLabel = UICornerstone_CreateLabel(
        "Click the button above", 14,
        20, 160, 400, 24);
    UICornerstone_AddChild(root, g_statusLabel);

    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0 / 60.0);
        UICornerstone_Clear();
        UICornerstone_Render();
        UICornerstone_Present();
    }
    UICornerstone_Shutdown();
    return 0;
}
```

### 关键差异说明

1. **无 JSON**：控件通过 `UICornerstone_CreatePanel/CreateButton/CreateLabel` 直接创建
2. **父子关系**：`UICornerstone_AddChild(root, child)` 将子控件挂到根 Panel
3. **句柄存储**：`g_statusLabel` 静态变量替代 `FindControl` 查找
4. **按钮颜色**：`UICornerstone_SetBGColor` 自动生成 hover（变亮 30%）和 pressed（变暗 30%）状态

## 6. sample_fromsource.c（混合集成——核心 DLL + 后端源码）

### 与前两个示例的结构对比

| 维度 | hello_uicornerstone / sample_programmatic | sample_fromsource |
|------|-------------------------------------------|-------------------|
| 链接方式 | 静态链接 `UICornerstone.lib` | 链接 `UICornerstone_dll`（导入库） |
| 后端位置 | 编译进 `UICornerstone.lib` | 编译进 **exe** |
| 核心库 | 无额外 DLL | `UICornerstone.dll`（ILT 隐式加载） |
| exe 大小 | ~3.3 MB | ~272 KB |
| 构建模式 | 全部模式 | 仅 `UICORNERSTONE_BUILD_DLL=ON` |

### 代码结构（~50 行）

```c
#include "UICornerstoneAPI.h"
#include <stdio.h>

extern UIBackendCallbacks* GetUIBackendCallbacks(void);

static int g_clickCount = 0;
static UIControlHandle g_statusLabel = NULL;

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;
    g_clickCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_clickCount);
    if (g_statusLabel)
        UICornerstone_SetText(g_statusLabel, buf);
}

int main(void) {
    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) return 1;

    if (!UICornerstone_Init(callbacks)) return 1;
    UICornerstone_SetViewport(0, 0, 800, 480);

    UIControlHandle root = UICornerstone_CreatePanel(0,0,800,480);

    UIControlHandle title = UICornerstone_CreateLabel(
        "UICornerstone Sample (Hybrid)", 18,
        20, 10, 760, 30);
    UICornerstone_AddChild(root, title);

    UIControlHandle btn = UICornerstone_CreateButton(
        "Click Me", 20, 60, 200, 80);
    UICornerstone_SetBGColor(btn, 74, 144, 217, 255);
    UICornerstone_SetOnClick(btn, onBtnClick, NULL);
    UICornerstone_AddChild(root, btn);

    g_statusLabel = UICornerstone_CreateLabel(
        "Click the button above", 14,
        20, 160, 400, 24);
    UICornerstone_AddChild(root, g_statusLabel);

    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0 / 60.0);
        UICornerstone_Clear();
        UICornerstone_Render();
        UICornerstone_Present();
    }
    UICornerstone_Shutdown();
    return 0;
}
```

### 关键差异说明

1. **`GetUIBackendCallbacks()`**：来自编译进 exe 的 `BackendPlugin.cpp`，返回后端回调表
2. **`UICornerstone_Init(callbacks)`**：通过 ILT（Import Library Thunk）调用 DLL 中的初始化函数
3. **ILT 隐式加载**：链接 `UICornerstone_dll.lib` 时编译器自动生成导入表，程序启动时自动加载 `UICornerstone.dll`
4. **控件工厂**：`UICornerstone_CreateButton` 等函数同样通过 ILT 调用 DLL 中的实现
5. **构建限制**：仅 `UICORNERSTONE_BUILD_DLL=ON` 模式可用，需 `build/sdl3_dll/` 目录

## 7. sample_loadlibrary.cpp（显式 LoadLibrary + #include 后端源码）

### 与 sample_fromsource 的对比

| 维度 | sample_fromsource | sample_loadlibrary |
|------|-------------------|-------------------|
| DLL 加载 | ILT 隐式（Windows 加载器） | `LoadLibrary` 显式 |
| C ABI 调用 | 直接符号链接 | `GetProcAddress` 函数指针 |
| 后端编译 | CMake 独立 TU | `#include .cpp` 同一 TU |

### 代码结构（~80 行）

```c
#include <windows.h>
#include "UICornerstoneAPI.h"

// 1) #include 后端 6 个 .cpp（编译入同一 TU）
#include "src/backend/sdl3/Window.cpp"
#include "src/backend/sdl3/TextRenderer.cpp"
#include "src/backend/sdl3/Cursor.cpp"
// ... (RenderDevice, InputBackend, BackendPlugin)

// 2) LoadLibrary 获取 C ABI 函数指针
HMODULE dll = LoadLibraryA("UICornerstone.dll");
uiInit = GetProcAddress(dll, "UICornerstone_Init");
uiCreateButton = GetProcAddress(dll, "UICornerstone_CreateButton");
// ...

// 3) 从 TU 内获 BackendCallbacks，通过函数指针调用 Init
UIBackendCallbacks* cbs = GetUIBackendCallbacks();
uiInit(cbs);

// 4) 帧循环（全部通过函数指针）
while (!uiIsQuitRequested()) {
    uiProcessEvents(); uiUpdate(1/60.);
    uiClear(); uiRender(); uiPresent();
}
FreeLibrary(dll);
```

### 关键差异说明

1. **零导入库依赖**：不链接 `UICornerstone_dll.lib`。C ABI 全部通过 `GetProcAddress` 按名字符串查找，Windows 加载器不介入
2. **`#include` 同一 TU**：6 个后端 .cpp 文件全部编译入 sample_loadlibrary.cpp 一个翻译单元，无需 CMake 单独文件
3. **3 个 Core 符号内联**：`Surface::registerFactories`（空实现）、`Cursor::registerFactories`（空实现，光标反馈缺省但功能无影响）、`ResourceProvider::createFilesystem`（完整实现，保证字体加载正常）。不定义 `UICORNERSTONE_BUILD_SHARED`，`CORE_API` 为空宏，故不产生 `dllimport`

## 8. CMake 构建

前两个示例 CMake 模式相同；sample_fromsource 需独立配置：

```cmake
add_executable(hello_uicornerstone hello_uicornerstone.c)
add_executable(sample_programmatic sample_programmatic.c)
# 各目标链接 UICornerstone + 编译宏 + 输出路径 + POST_BUILD 复制 DLL/assets
```

构建命令：

```batch
cmake --build build\sdl3 --config Debug --target hello_uicornerstone
cmake --build build\sdl3 --config Debug --target sample_programmatic
cmake --build build\sdl3_dll --config Debug --target sample_fromsource
cmake --build build\sdl3_dll --config Debug --target sample_loadlibrary
```


| 示例                | 输出路径                                            |
| ------------------- | --------------------------------------------------- |
| hello_uicornerstone | `build/sample/hello_uicornerstone/<backend>/Debug/` |
| sample_programmatic | `build/sample/sample_programmatic/<backend>/Debug/` |
| sample_fromsource | `build/sample/sample_fromsource/<backend>/Debug/` |
| sample_loadlibrary | `build/sample/sample_loadlibrary/<backend>/Debug/` |

## 9. 文件清单（含 loadlibrary）

```
samples/
├── hello_uicornerstone/
│   ├── CMakeLists.txt           # ~30 行
│   └── hello_uicornerstone.c    # ~55 行
├── sample_programmatic/
│   ├── CMakeLists.txt           # ~30 行
│   └── sample_programmatic.c    # ~45 行
├── sample_fromsource/
│   ├── CMakeLists.txt           # ~35 行
│   └── sample_fromsource.c      # ~55 行
└── sample_loadlibrary/
    ├── CMakeLists.txt           # ~30 行
    └── sample_loadlibrary.cpp   # ~80 行
```

## 10. 后续扩展


| 阶段 | Sample              | 新增内容                           | 状态      |
| ---- | ------------------- | ---------------------------------- | --------- |
| 1    | hello_uicornerstone | 完全静态 + JSON 布局 + Button 响应 | ✅ 已实现 |
| 2    | sample_programmatic | 编程式控件创建代替 JSON            | ✅ 已实现 |
| 3 | sample_fromsource | 混合集成（核心 DLL + 后端源码） | ✅ 已实现 |
| 4 | sample_loadlibrary | 显式 LoadLibrary + #include 后端源码 | ✅ 已实现 |
