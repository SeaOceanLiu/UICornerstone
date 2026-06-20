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

## 可用测试

| 测试 | 说明 |
|------|------|
| `test_label` | Label 控件 |
| `test_button` | Button 控件 |
| `test_checkbox` | CheckBox 控件 |
| `test_editbox` | EditBox 控件 |
| `test_progressbar` | ProgressBar 控件 |
| `test_menu` | Menu 控件 |
| `test_winframe` | WinFrame 控件 |
| `test_graphtool` | GraphTool 绘制 |
| `test_layout` | Layout 布局 |
| `test_layout_advanced` | 高级布局 |
| `test_api` | C ABI API 测试 |
| `test_fromsource_sdl3` | SDL3 fromsource 测试 |
| `test_fromsource_sfml` | SFML fromsource 测试 |
| `test_fromsource_raylib` | Raylib fromsource 测试 |

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

DLL 模式额外包含 `UICornerstone.dll` 和 `UIBackend_*.dll`。

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

## fromsource 测试

fromsource 测试将后端源码（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）作为独立翻译单元编译进 exe，通过 `LoadLibrary("UICornerstone.dll")` 和 C ABI 调用控件。仅 DLL 模式可用。

## 故障排除

### CMake 配置失败

确保已安装 Visual Studio 2022 Build Tools，并正确设置了环境变量。

### 链接错误 LNK1104

确保所有子模块依赖已正确通过 `git submodule update --init --recursive` 拉取。
