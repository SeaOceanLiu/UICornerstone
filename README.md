# UICornerstone

基于 SDL3 的 C++17 UI 控件库，使用 Direct3D 11 渲染，适用于游戏和图形应用。

## 功能特性

- **14+ UI 控件**：Label、Button（支持 Actor/LuotiAni）、CheckBox（三态）、EditBox、TextArea（多行+滚动）、ProgressBar、ScrollBar、Menu、Dialog、Bench（底部面板）等
- **JSON 布局系统**：通过 `LayoutParser` 将 JSON 文件解析为控件树，支持嵌套布局和事件绑定
- **Direct3D 11 渲染**：所有控件通过 D3D11 绘制，支持纹理、字体、粒子动画
- **LuotiAni 粒子动画引擎**：为 Button 控件提供粒子动画支持
- **Actor 图片系统**：控件可绑定多状态图片（normal/hover/pressed/disabled）
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

### 编译全部

```cmd
build_scripts\build.bat Debug
```

### 编译并运行某个测试

```cmd
build_scripts\build_test.bat test_label
cd build\test\Debug
test_label.exe
```

### 可用测试

| 测试名 | 说明 |
|--------|------|
| test_menu | 菜单控件测试 |
| test_label | 标签/按钮动画演示 |
| test_editbox | 输入框测试 |
| test_checkbox | 复选框测试 |
| test_progressbar | 进度条测试 |
| test_layout | JSON 布局解析演示（推荐） |

### 手动编译

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Debug
```

编译后在 `build\test\Debug\` 下生成测试可执行文件，DLL 和资源文件由 CMake POST_BUILD 自动复制。

## 项目结构

```
UICornerstone/
├── src/              # 源代码（LayoutParser、各控件实现、Bench、ResourceLoader）
├── include/          # 头文件
├── test/             # 测试用例（test_*.cpp）
├── layouts/          # JSON 布局文件
├── doc/              # 设计文档（*.md, *.drawio, *.svg）
├── build_scripts/    # 编译脚本（build.bat, build_test.bat）
├── subModules/       # 子模块依赖
│   ├── libs/         # 预编译 SDL3/SDL3_ttf/SDL3_image/DebugInfo 库
│   ├── SDL3/         # SDL3 头文件
│   ├── SDL3_ttf/     # SDL3_ttf 头文件
│   ├── SDL3_image/   # SDL3_image 头文件

│   ├── json/         # nlohmann/json
│   └── assets/       # 字体、图片等资源文件
└── CMakeLists.txt
```

## 依赖项

| 依赖 | 许可证 | 说明 |
|------|--------|------|
| SDL3 | zlib | 底层窗口/输入/渲染 |
| SDL3_ttf | zlib | 字体渲染 |
| SDL3_image | zlib | 图片加载 |
| json (nlohmann) | MIT | JSON 解析 |
| 字体资源 | SIL OFL | HarmonyOS Sans / MapleMono 等 |

## 文档

详细设计文档位于 `doc/` 目录：

| 文档 | 说明 |
|------|------|
| LayoutSystem_Design.md | JSON 布局系统设计（推荐首先阅读） |
| ControlBase_Design.md | 控件基类架构与绘制机制 |
| Button_Design.md / Label_Design.md / ... | 各控件设计 |
| Build_Guide.md | 编译指南 |
| GraphTool_Design.md | 内部图形工具设计 |

## 许可证

本项目基于 **GNU General Public License v3.0** 发布，详见 [LICENSE](LICENSE) 文件。

第三方组件许可证：
- SDL3 / SDL3_ttf / SDL3_image：zlib License
- json (nlohmann)：MIT License
- 字体资源：SIL Open Font License v1.1

## 作者

SeaOceanLiu
