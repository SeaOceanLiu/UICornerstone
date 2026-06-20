# UICornerstone 编译指南

## 概述

UICornerstone 支持三个后端：SDL3、SFML、Raylib。每个后端可编译为**静态库**或**DLL**模式。

## 目录结构

```
UICornerstone/
├── build/                  # 编译输出目录
│   ├── sdl3/               # SDL3 静态模式
│   ├── sdl3_dll/           # SDL3 DLL 模式
│   ├── sfml/               # SFML 静态模式
│   ├── sfml_dll/           # SFML DLL 模式
│   ├── raylib/             # Raylib 静态模式
│   └── raylib_dll/         # Raylib DLL 模式
├── build_scripts/          # 编译脚本
├── doc/                    # 设计文档
├── include/                # 头文件
├── src/                    # 源代码
└── test/                   # 测试代码
```

## 编译要求

- Visual Studio 2022 Build Tools 或更高版本
- CMake 3.20+
- 子模块依赖（`git submodule update --init --recursive`）

## 快速开始

### 编译所有测试（指定后端）

```batch
cd UICornerstone
build_scripts\build.bat sdl3     # SDL3 后端
build_scripts\build.bat sfml     # SFML 后端
build_scripts\build.bat raylib   # Raylib 后端
```

### 编译单个测试

```batch
build_scripts\build_sdl3.bat test_label
build_scripts\build_sfml.bat test_label
build_scripts\build_raylib.bat test_label
```

### 编译单测快捷方式

```batch
build_scripts\build_test.bat test_label             # SDL3（默认）
build_scripts\build_test.bat test_label sfml        # SFML
build_scripts\build_test.bat test_label raylib      # Raylib
```

## 可用测试 & 示例

| 目标 | 类型 | 说明 |
|------|------|------|
| `test_label` | 测试 | Label 控件 |
| `test_button` | 测试 | Button 控件 |
| `test_checkbox` | 测试 | CheckBox 控件 |
| `test_editbox` | 测试 | EditBox 控件 |
| `test_progressbar` | 测试 | ProgressBar 控件 |
| `test_menu` | 测试 | Menu 控件 |
| `test_winframe` | 测试 | WinFrame 控件 |
| `test_graphtool` | 测试 | GraphTool 绘制 |
| `test_layout` | 测试 | Layout 布局 |
| `test_layout_advanced` | 测试 | 高级布局 |
| `test_api` | 测试 | C ABI API 测试 |
| `test_fromsource_sdl3` | 测试 | SDL3 fromsource 测试 |
| `test_fromsource_sfml` | 测试 | SFML fromsource 测试 |
| `test_fromsource_raylib` | 测试 | Raylib fromsource 测试 |
| `hello_uicornerstone` | 示例 | 纯 C 示例（JSON 布局），Button + Label 交互 |
| `sample_programmatic` | 示例 | 纯 C 示例（编程式创建），Button + Label 交互 |
| `sample_fromsource` | 示例 | 纯 C 示例（混合集成），Button + Label 交互，需 DLL 模式 |
| `sample_loadlibrary` | 示例 | 纯 C++ 示例（LoadLibrary + #include），Button + Label 交互，需 DLL 模式 |

## 输出目录

编译完成后，可执行文件位于对应后端和模式的 test/Debug 目录：

```
build\sdl3\test\Debug\
├── test_label.exe
├── test_button.exe
├── ...
├── SDL3.dll              # 第三方依赖
├── SDL3_ttf.dll
└── SDL3_image.dll
```

示例独立输出到 `build/sample/` 目录，按示例名+后端命名：

```
build\sample\hello_uicornerstone\sdl3\Debug\
├── hello_uicornerstone.exe
├── assets/               # 字体等资源
├── SDL3.dll
├── SDL3_ttf.dll
└── SDL3_image.dll
```

```
build\sample\sample_fromsource\sdl3\Debug\
├── sample_fromsource.exe
├── UICornerstone.dll     # 核心 DLL（由 ILT 隐式加载）
├── assets/
├── SDL3.dll
├── SDL3_ttf.dll
└── SDL3_image.dll
```

DLL 模式额外包含 `UICornerstone.dll`（and `UIBackend_*.dll` for fromsource tests, but NOT for sample_fromsource which compiles backend into exe）。

## 运行测试

直接在对应输出目录运行：

```batch
cd build\sdl3\test\Debug
test_label.exe
```

## DLL 模式构建

```batch
# 预配置的 DLL 目录已有 CMakeCache
cmake --build build\sdl3_dll --config Debug
cmake --build build\sfml_dll --config Debug
cmake --build build\raylib_dll --config Debug
```

## Sample 构建

```batch
# 静态示例（任意模式）
cmake --build build\sdl3 --config Debug --target hello_uicornerstone
cmake --build build\sdl3 --config Debug --target sample_programmatic

# 混合集成示例（仅 DLL 模式）
cmake --build build\sdl3_dll --config Debug --target sample_fromsource
cmake --build build\sdl3_dll --config Debug --target sample_loadlibrary
```

运行：

```batch
build\sample\hello_uicornerstone\sdl3\Debug\hello_uicornerstone.exe
build\sample\sample_programmatic\sdl3\Debug\sample_programmatic.exe
build\sample\sample_fromsource\sdl3\Debug\sample_fromsource.exe
build\sample\sample_loadlibrary\sdl3\Debug\sample_loadlibrary.exe
```

静态示例零自家 DLL 依赖；fromsource 示例需 `UICornerstone.dll` 同目录。

## fromsource 测试与示例

fromsource 将后端源码（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）作为独立翻译单元编译进 exe，通过 ILT 或 `LoadLibrary` 加载 `UICornerstone.dll`。仅 DLL 模式可用。

| 目标 | 说明 |
|------|------|
| `test_fromsource_sdl3` | 纯测试，`LoadLibrary` + SDL App 模式 |
| `sample_fromsource` | 纯 C 示例，`ILT` 隐式加载 + `main()` 帧循环 |
| `sample_loadlibrary` | 纯 C++ 示例，`LoadLibrary` 显式加载 + `#include` 后端源码 |

## 故障排除

### CMake 配置失败

确保已安装 Visual Studio 2022 Build Tools，并正确设置了环境变量。

### 链接错误 LNK1104

确保所有子模块依赖已正确通过 `git submodule update --init --recursive` 拉取。
