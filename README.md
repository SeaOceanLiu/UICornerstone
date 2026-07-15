# UICornerstone

基于 C++17 的多后端跨平台 UI 控件库，适用于游戏和图形应用。

支持 **SDL3 / SFML / Raylib** 三种渲染后端，提供 C ABI 接口可被纯 C / C++ / 动态加载等多种集成方式调用。

## 功能特性

- **14+ UI 控件**：Label、Button（支持 Actor/LuotiAni）、CheckBox（三态）、EditBox、TextArea（多行+滚动）、ProgressBar、ScrollBar、Slider、Menu、WinFrame、ColorPicker、Panel 等
- **声明式 UI（JSON 布局）**：通过 JSON 描述控件树和事件绑定，`LayoutParser` 自动解析，无需手写创建代码
- **三后端切换**：SDL3（主开发后端）、SFML、Raylib，只需改 CMake 变量即可切换
- **LuotiAni 粒子动画引擎**：为 Button 控件提供粒子动画支持
- **Actor 图片系统**：控件可绑定多状态图片（normal/hover/pressed/disabled）
- **四层抽象架构**：`RenderDevice` → `Texture/Surface` → `TextRenderer` → `InputBackend`，完全不直接依赖后端 API
- **C ABI 公开接口**：纯 C 兼容的 `UICornerstone_*` 函数，支持静态链接、DLL 隐式加载、显式 `LoadLibrary`
- **缩放感知**：内置 `dpiScale` 机制，布局和渲染自动适配高 DPI 显示

## 环境要求

- Windows 10+
- Visual Studio 2022（需包含"使用 C++ 的桌面开发"工作负载）
- CMake 3.16+

## 快速开始

### 克隆仓库（包含所有依赖）

```bash
git clone --recursive https://github.com/SeaOceanLiu/UICornerstone.git
cd UICornerstone
```

### 编译全部（SDL3 后端，默认）

```cmd
build_scripts\build.bat sdl3
```

### 编译并运行某个测试

```cmd
build_scripts\build_test.bat test_label
cd build\sdl3\test\Debug
test_label.exe
```

### SFML / Raylib 后端

```cmd
build_scripts\build.bat sfml
build_scripts\build.bat raylib
```

输出目录：`build\{sdl3|sfml|raylib}\test\Debug\`

## 首个应用：5 分钟快速上手

UICornerstone 提供 4 种集成模式的完整示例，详见 [用户开发教程](doc/Tutorial.md)：

| 模式 | 示例 | 一句话说明 |
|------|------|-----------|
| **声明式 UI（JSON 布局）** | `hello_uicornerstone` | 写 JSON 字符串描述 UI，`LoadLayout` 自动解析 |
| **命令式 UI（C ABI 工厂函数）** | `sample_programmatic` | `CreateButton/CreateLabel` 代码创建控件 |
| **混合集成（核心 DLL + 后端源码）** | `sample_fromsource` | 核心控件在 DLL，后端源码编译进 exe |
| **显式 LoadLibrary** | `sample_loadlibrary` | `LoadLibrary + GetProcAddress` 完全运行时加载 |

构建示例：

```cmd
cmake --build build\sdl3 --config Debug --target hello_uicornerstone
build\sdl3_dll --config Debug --target sample_fromsource
```

所有示例输出到 `build/sample/<name>/<backend>/Debug/`。

## 可用测试

### 核心功能测试（所有后端均可编译）

| 测试名（文件名排序） | 说明 |
|----------------------|------|
| test_button | 按钮动画（LuotiAni 粒子动画）测试 |
| test_checkbox | 复选框（三态）测试 |
| test_colorpicker | 颜色选择器测试 |
| test_editbox | 输入框测试（含中文输入） |
| test_graphtool | 图形工具绘制测试（几何图元、线型、填充） |
| test_label | 标签及标题栏按钮动画演示 |
| test_layout | JSON 布局解析基础演示 |
| test_layout_advanced | 高级布局：百分比、嵌套、对齐 |
| test_menu | 菜单控件测试（MenuItem / MenuPanel / MenuBar） |
| test_progressbar | 进度条动画测试 |
| test_slider | 滑块控件测试（含刻度线/值标签） |
| test_winframe | 窗口框架测试（拖动、缩放、关闭按钮） |

### C ABI 测试

| 测试名 | 说明 |
|--------|------|
| test_api | 纯 C 编写的 C ABI 全功能验证（6 种控件 + JSON 布局 + 事件绑定） |

### From-source / DLL 桥接测试（仅 `UICORNERSTONE_BUILD_DLL=ON` 模式）

fromsource 测试使用单源文件 + 编译定义区分后端，后端源码作为独立 TU 编译：

| 测试名 | 说明 |
|--------|------|
| test_fromsource_cabi | C ABI 编程式创建控件（Button/Label/CheckBox/EditBox/ProgressBar/Panel/Slider/ColorPicker...） |
| test_dialog_cabi | JSON Dialog 颜色选择器（预设色 + RGB 滑块 + Hex 输入 + Dialog 确定/取消） |
| test_combobox_cabi | JSON ComboBox（10 个城市选项，选中回调验证） |

## 项目结构

```
UICornerstone/
├── src/                     # 核心源码
│   ├── *.cpp                #   控件实现 + 基础设施
│   └── backend/             #   后端实现（sdl3/ sfml/ raylib/）
├── include/                 # 头文件（含公有 C ABI 头 UICornerstoneAPI.h）
├── test/                    # 测试用例（test_*.cpp）
├── samples/                 # 4 种集成模式示例
│   ├── hello_uicornerstone/ #   声明式 UI（JSON 布局）
│   ├── sample_programmatic/ #   命令式 UI（C ABI 工厂函数）
│   ├── sample_fromsource/   #   混合集成（核心 DLL + 后端源码）
│   └── sample_loadlibrary/  #   显式 LoadLibrary
├── layouts/                 # JSON 布局文件
├── doc/                     # 设计文档 + 用户教程
├── build_scripts/           # 编译脚本（build.bat, build_test.bat）
├── subModules/              # 子模块依赖
│   ├── libs/                #   预编译 SDK 库
│   ├── SDL3/ SDL3_ttf/ SDL3_image/
│   ├── SFML/
│   ├── raylib/
│   ├── json/                #   nlohmann/json
│   └── assets/              #   字体、图片等资源
└── CMakeLists.txt
```

## 依赖项

| 依赖 | 许可证 | 说明 |
|------|--------|------|
| SDL3（可选） | zlib | SDL3 后端的窗口/输入/渲染 |
| SDL3_ttf（可选） | zlib | SDL3 后端的字体渲染 |
| SDL3_image（可选） | zlib | SDL3 后端的图片加载 |
| SFML（可选） | zlib | SFML 后端的图形/窗口/系统 |
| raylib（可选） | zlib | Raylib 后端的渲染/窗口/输入 |
| json (nlohmann) | MIT | JSON 解析 |
| 字体资源 | SIL OFL | HarmonyOS Sans / MapleMono 等 |

## 文档

| 文档 | 说明 |
|------|------|
| [Tutorial.md](doc/Tutorial.md) | **用户开发教程（推荐首先阅读）** — 从零开始构建 UICornerstone 应用 |
| [Build_Guide.md](doc/Build_Guide.md) | 编译指南 |
| [Sample_Design.md](doc/Sample_Design.md) | 4 种集成模式的架构设计 |
| [UICornerstone_DLL_Design.md](doc/UICornerstone_DLL_Design.md) | C ABI 与 DLL 架构 |
| [BackendAbstraction_Design.md](doc/BackendAbstraction_Design.md) | 多后端抽象架构设计 |
| [LayoutSystem_Design.md](doc/LayoutSystem_Design.md) | JSON 布局系统设计 |
| [ControlBase_Design.md](doc/ControlBase_Design.md) | 控件基类架构与绘制机制 |
| [GraphTool_Design.md](doc/GraphTool_Design.md) | 内部图形工具设计 |
| [EventSystem_Design.md](doc/EventSystem_Design.md) | 事件系统设计（EventType → InputBackend → EventQueue → 控件分派 → FocusManager） |
| [FocusSystem_Design.md](doc/FocusSystem_Design.md) | 焦点系统设计（Tab 环、FocusBoundary、焦点环绘制） |
| [Dialog_Design.md](doc/Dialog_Design.md) | Dialog/Popup 弹窗设计 |
| [ComboBox_Design.md](doc/ComboBox_Design.md) | ComboBox 下拉框设计 |
| Button_Design.md / Label_Design.md / ... | 各控件详细设计（CheckBox / EditBox / TextArea / ScrollBar / ProgressBar / WinFrame / Menu / Slider / ColorPicker / HandleControl） |

## 许可证

本项目基于 **GNU General Public License v3.0** 发布，详见 [LICENSE](LICENSE) 文件。

第三方组件许可证：
- SDL3 / SDL3_ttf / SDL3_image：zlib License
- SFML：zlib License
- raylib：zlib License
- json (nlohmann)：MIT License
- 字体资源：SIL Open Font License v1.1

## 作者

Architecture by SeaOceanLiu, program by SeaOceanLiu and AIs (DeepSeek V4 Flash, GLM 5.1, MiniMax-M2.5)
