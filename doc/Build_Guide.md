# UICornerstone 编译指南

## 概述

本文档说明如何编译 UICornerstone 库的测试程序。

## 目录结构

```
UICornerstone/
├── build/              # 编译输出目录
├── build_scripts/      # 编译脚本
├── doc/                # 设计文档
├── include/            # 头文件
├── src/                # 源代码
└── test/              # 测试代码
```

## 编译要求

- Visual Studio 2022 Build Tools 或更高版本
- CMake 3.20+
- SDL3 库
- SDL3_ttf 库

## 快速开始

### 编译所有测试

```batch
cd UICornerstone\build_scripts
build_test.bat
```

### 编译特定测试

```batch
build_test.bat -Target test_label
build_test.bat -Target test_checkbox
build_test.bat -Target test_editbox
build_test.bat -Target test_menu
build_test.bat -Target test_progressbar
```

### 清理并重新编译

```batch
build_test.bat -Clean
build_test.bat
```

## 输出目录

编译完成后，可执行文件位于：

```
UICornerstone/build/test/Debug/
├── test_label.exe
├── test_checkbox.exe
├── test_editbox.exe
├── test_menu.exe
├── test_progressbar.exe
├── SDL3.dll
├── SDL3_ttf.dll
└── DebugInfoX64.dll
```

## 运行测试

直接在 `Debug` 目录中运行对应的 exe 文件即可。

```batch
cd UICornerstone\build\test\Debug
test_label.exe
```

## 编译脚本说明

`build_test.bat` 脚本支持以下参数：

| 参数 | 说明 |
|------|------|
| `-Target <name>` | 指定编译目标 |
| `-Clean` | 清理构建目录 |
| `-ListTargets` | 列出可用的编译目标 |

## 故障排除

### CMake 配置失败

确保已安装 Visual Studio 2022 Build Tools，并正确设置了环境变量。

### 链接错误 LNK1104

确保所有依赖项（SDL3 / SFML / raylib 等）已正确配置。