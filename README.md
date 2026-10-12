# UICornerstone

<p align="center">
  <img src="docs/assets/UICornerstone_Logo.svg" alt="UICornerstone Logo" width="200"/>
</p>

**UICornerstone** 是一个从零构建的跨后端 GUI 库，采用「核心库 + 后端插件」架构：核心实现全部与图形后端无关（渲染经 `RenderDevice / Texture / TextRenderer / InputBackend` 四层抽象），同一套代码可在 **SDL3**、**SFML**、**raylib** 三种后端上运行。

## 功能特性

- **四层后端抽象**：RenderDevice（绘制原语）、Texture（纹理）、TextRenderer（文本）、InputBackend（输入）——换后端零业务代码改动。
- **声明式 UI**：JSON 布局文件描述控件树、布局引擎（流式/锚点/网格）、事件绑定与主题样式，支持热加载。
- **20+ 控件**：Button、CheckBox（三态）、ComboBox、ContextMenu、EditBox、ListView（含单列 ListBox 模式）、NumericUpDown、Panel、ProgressBar、ScrollBar、Shape（七种图元+多图元组合）、Slider、StatusBar（VSCode 风格）、TabControl（四方向页签）、TextArea、TreeView、WinFrame、Dialog/Popup、HandleControl 等。
- **LuotiAni 动画引擎**：基于 JSON 关键帧描述（translate/scale/rotate/opacity/visible + 缓动与轨迹），支持多实例共享烘焙帧数据。
- **Actor 图片系统**：控件可绑定多状态图片（normal/hover/pressed/disabled），支持缩放模式/锚点/透明度/匹配父矩形；LuotiAni 每帧动画即为一个帧 Actor。
- **三种集成方式**：C ABI 动态库（纯 C 调用）、C++ Binding（现代 C++ 封装）、核心 + 后端源码混合编译。
- **多实例 / 多视口**：一个进程多个独立 UI 实例、多窗口与子视口渲染（按后端能力位条件化）。
- **焦点系统**：Tab 环、焦点边界（WinFrame 内循环）、Ctrl+Tab 焦点作用域切换、焦点环绘制。
- **高 DPI**：内置 `dpiScale` 机制，布局与渲染自动适配。

## 架构总览

| 层 | 职责 | 目录 |
|---|---|---|
| 核心库 | 控件树、事件系统、焦点管理、布局引擎、JSON 解析、动画引擎 | `include/` + `src/` |
| 后端插件 | SDL3 / SFML / raylib 各自的窗口、输入、渲染适配 | `src/backend/` |
| C ABI | 纯 C 接口：实例管理、布局加载、控件工厂、属性系统、回调 | `include/UICornerstoneAPI.h` |
| C++ Binding | 类型安全封装：`UICornerstone`、`Control`、`Event` | `binding/` |
| 样例与测试 | 4 种集成模式样例、全量自动化测试 | `samples/`、`test/` |

![UICornerstone 分层架构图](docs/assets/architecture.svg)

支持 **SDL3 / SFML / Raylib** 三种渲染后端，提供 C ABI 接口可被纯 C / C++ / 动态加载等多种集成方式调用；另附 **C++ Binding**（`binding/`，纯动态加载封装）与 4 个可直接修改的样例程序。

## 环境要求

- Windows 10+
- Visual Studio 2022（需包含"使用 C++ 的桌面开发"工作负载）
- CMake 3.16+

## 多实例 / 多视口

UICornerstone 支持单进程内创建多个独立窗口实例：

- **多实例**：`UICornerstone_CreateInstance` 每调用一次即创建一个独立实例（独立的窗口、事件队列、控件 ID 表、Action 表、资源根目录、焦点管理器）；各实例互不干扰，句柄不可跨实例混用
- **多视口**：`UICornerstone_CreateViewport` 在既有实例内创建共享同一后端的子视口；焦点系统支持 Tab 环内导航与 **Ctrl+Tab 跨视口跳转**（焦点智能路由到最近实例）
- **后端能力位**：`UICornerstone_GetBackendCapabilities` 返回 `UICORN_BACKEND_CAP_*` 位组合（`MULTI_WINDOW`/`RENDER_TARGET`/`CLIP_RECT`/`READBACK`），调用方据此决定行为。**raylib 为单窗口架构**（内部全局只跟踪最近创建的窗口，预编译 DLL 无源码不可修补），声明**不含 `MULTI_WINDOW`**——多实例下仅首个实例有真实窗口，其余为 headless（`Window::isHeadless()`），对其渲染会串扰到主实例窗口（内容闪动）；多窗口渲染前须查能力位条件化（sdl3/sfml 四能力全有，不受限）。详见 [后端抽象设计文档 §20](design/BackendAbstraction_Design.md)
- 生命周期：实例销毁自动清理后端窗口与渲染设备；日志以 `[Instance_N]` 前缀区分实例
- 详见 [C ABI 多实例支持改造设计](design/CABI_MultiInstance_Design.md)，实测用例 `test_multi_instance_cabi`、`test_multiviewport_cabi`（K1-K8 三后端全过）、`test_multiinstance_visual_cabi` / `test_multiviewport_visual_cabi`（视觉状态断言）

## LuotiAni 动画引擎（"洛蒂"）

LuotiAni 是内置的关键帧动画引擎，采用"JSON 描述 → 一次性烘焙 → 按帧播放"三段式流水线：

![LuotiAni 三段式流水线](docs/assets/luotiani_arch.svg)

- **运行时零插值**：所有帧在加载时全部烘焙为贴图，播放只做跳帧，CPU 开销极低
- **动画能力**：多图层（每层一张贴图 + 关键帧）、平移/缩放/旋转/透明度/可见性操作、`loop` 循环、`frameRate` 帧率
- **绑定方式**：Button 的 `animation` 属性、JSON 布局 `luotiAni` 节点、C ABI `UICornerstone_CreateAnimation`
- **资源引用**：动画描述（`path`/`animation`）与帧图 `src` 均支持 `provider:` 资源 ID（内存 provider 加载；未注册键经文件系统回退并记录日志——P0-78/P0-79）
- **多实例共享**：`LuotiInstance` 共享同一份帧数据多路播放，内存只存一份
- 视觉校验工具：`test_aniviewer <动画.jsonc> [loop=0|1] [auto=<秒>] [vsync=0|1]`（任意顺序），窗口覆盖层实时显示设定/实际 fps
- 详见 [LuotiAni 动画开发手册](design/LuotiAni_DevGuide.md)

## Actor 图片系统

Actor 是控件可绑定的图片素材（`Actor → Material`）：

- **多状态外观**：Button 等控件每种状态（normal/hover/pressed/disabled）可挂独立 Actor
- **属性**：`image`（文件或资源）、`match-parent-rect`（匹配父矩形）、`alpha`（透明度）、`scale-type`/`anchor`（缩放模式与锚点）
- **绘制次序**：Actor 图片位于控件背景之上、标题文字之下
- **帧 Actor**：LuotiAni 每帧画布即一个帧 Actor（`make_shared<Actor>(this, true)`），随帧号切换贴图

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

### C++ Binding 与样例

```cmd
cmake --build build\binding --config Debug
```

输出：`build\binding\Debug\`（`UICornerstoneBinding.lib` + 4 个样例 exe + 运行所需核心/后端 DLL）。

## 首个应用：5 分钟快速上手

UICornerstone 提供 4 种集成模式的完整示例，详见 [用户开发教程](design/Tutorial.md)：

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

### C++ Binding 样例（binding/samples/）

| 样例 | 说明 |
|------|------|
| `sample_cpp_hosted` | Hosted 模式：UI 托管循环，游戏逻辑嵌入回调 |
| `sample_cpp_embed` | Embedded 模式：UI 嵌入用户游戏循环 |
| `sample_cpp_multiview` | 单窗口两个子视口（Bench A/B），弹窗展示本视口输入 |
| `sample_cpp_multiinstance` | 双窗口独立实例双向通信（A 按钮 → B 标签），多窗口事件泵 + 能力位条件化渲染 |
| `sample_scale` | 缩放对照组：1x/2x 按钮/图片按钮/动画按钮 rect 断言 |

构建/运行（`build\binding\Debug\` 下）：

```cmd
cmake --build build\binding --config Debug
sample_cpp_multiinstance.exe backend=sdl3      # 命令行选后端（缺省 sdl3）
sample_cpp_multiinstance.exe auto=4            # 无人值守冒烟（auto=<秒> 满时长自动退出，与标准测试同方案）
```

详见 [CppBinding_UserManual.md](design/CppBinding_UserManual.md)（用户手册）与 [CppBinding_Design.md](design/CppBinding_Design.md)（设计）。

## 可用测试

### 核心功能测试（所有后端均可编译）

| 测试名（文件名排序） | 说明 |
|----------------------|------|
| test_animation | 动画测试 |
| test_aniviewer | LuotiAni 视觉校验工具（加载 jsonc 播放，窗口覆盖层显示设定/实际 fps，支持 vsync/loop/auto 参数） |
| test_api | 纯 C 编写的 C ABI 全功能验证（7 种控件 + JSON 布局 + 事件绑定） |
| test_button | 按钮动画（LuotiAni 关键帧动画）测试 |
| test_checkbox | 复选框（三态）测试 |
| test_colorpicker | 颜色选择器测试 |
| test_dialog | Dialog/Popup 弹窗测试 |
| test_editbox | 输入框测试（placeholder / 密码模式 / 禁用 / 2x 缩放 / TextArea 多行滚动与清除） |
| test_graphtool | 图形工具绘制测试（几何图元、线型、填充） |
| test_handlecontrol | HandleControl 句柄调整控件测试 |
| test_image | Image 图片控件测试（T1-T8） |
| test_label | 标签及标题栏按钮动画演示 |
| test_layout | JSON 布局解析基础演示 |
| test_layout_advanced | 高级布局：百分比、嵌套、对齐 |
| test_luotiani | LuotiAni 动画引擎测试 |
| test_menu | 菜单控件测试（MenuItem / MenuPanel / MenuBar） |
| test_multi_instance | 多实例隔离（静态链接，事件/Action/生命周期隔离） |
| test_multiviewport | 多视口 + 键盘跨视口导航（K1-K8，静态链接） |
| test_progressbar | 进度条动画测试 |
| test_slider | 滑块控件测试（含刻度线/值标签） |
| test_splitter | 分割条控件测试 |
| test_treeview | 树控件测试 |
| test_winframe | 窗口框架测试（拖动、缩放、关闭按钮） |

### From-source / DLL 桥接测试（仅 `UICORNERSTONE_BUILD_DLL=ON` 模式）

fromsource 测试使用单源文件 + 编译定义区分后端，后端源码作为独立 TU 编译：

| 测试名 | 说明 |
|--------|------|
| test_fromsource_cabi | C ABI 编程式创建控件（Button/Label/CheckBox/EditBox/ProgressBar/Panel/Slider/ColorPicker...） |
| test_dialog_cabi | JSON Dialog 颜色选择器（预设色 + RGB 滑块 + Hex 输入 + Dialog 确定/取消） |
| test_combobox_cabi | JSON ComboBox（10 个城市选项，选中回调验证） |
| test_numericupdown_cabi | JSON NumericUpDown（+/- 步进与回调验证） |
| test_splitter_cabi | JSON Splitter（分割条拖动与布局验证） |
| test_treeview_cabi | JSON TreeView（树节点展开/选中验证） |
| test_property_cabi | C ABI 通用属性系统验证（Set/Get 对称 + 边界条件） |
| test_multi_instance_cabi | 多实例隔离（CreateInstanceFromPlugin 动态加载，事件/Action/生命周期隔离；渲染按 `MULTI_WINDOW` 能力位条件化） |
| test_multiviewport_cabi | 多视口 + 键盘跨视口导航（K1-K8，动态加载） |
| test_multiinstance_visual_cabi | 多实例视觉状态（hover/焦点环跨窗口隔离、跨窗口内容传递、逆序销毁；raylib 渲染冒烟按能力位 SKIP） |
| test_multiviewport_visual_cabi | 多视口视觉状态（hover 隔离、弹窗视口内居中、右下视口 Popup 回归） |

所有测试均支持 `auto=<秒>` 无人值守参数（任意顺序）；三后端全量回归 exit=0。

## 项目结构

```
UICornerstone/
├── src/                     # 核心源码
│   ├── *.cpp                #   控件实现 + 基础设施
│   └── backend/             #   后端实现（sdl3/ sfml/ raylib/）
├── include/                 # 头文件（含公有 C ABI 头 UICornerstoneAPI.h）
├── binding/                 # C++ Binding（纯动态加载封装 + 4 个样例）
│   ├── src/                 #   DynamicApi 层 + 主类/Control/Event 封装
│   ├── include/             #   UICornerstone.h / PropertyNames.h 等
│   └── samples/             #   sample_cpp_{hosted,embed,multiview,multiinstance}
├── test/                    # 测试用例（test_*.cpp）
├── samples/                 # 4 种集成模式示例
│   ├── hello_uicornerstone/ #   声明式 UI（JSON 布局）
│   ├── sample_programmatic/ #   命令式 UI（C ABI 工厂函数）
│   ├── sample_fromsource/   #   混合集成（核心 DLL + 后端源码）
│   └── sample_loadlibrary/  #   显式 LoadLibrary
├── layouts/                 # JSON 布局文件
├── docs/                      # 用户手册（网站形式，入口 index.html）
├── design/                    # 设计文档 + 用户教程
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
| [docs/index.html](docs/index.html) | **用户手册（网站形式）** — 介绍 / 快速起步 / 基础入门 / 深入控件（每控件含创建、方法、属性、回调、JSON 语法、C++ Binding 样例）/ 进阶主题 / 附录 / FAQ |
| [Tutorial.md](design/Tutorial.md) | **用户开发教程（推荐首先阅读）** — 从零开始构建 UICornerstone 应用 |
| [Build_Guide.md](design/Build_Guide.md) | 编译指南 |
| [Sample_Design.md](design/Sample_Design.md) | 4 种集成模式的架构设计 |
| [UICornerstone_DLL_Design.md](design/UICornerstone_DLL_Design.md) | C ABI 与 DLL 架构 |
| [BackendAbstraction_Design.md](design/BackendAbstraction_Design.md) | 多后端抽象架构设计（含 §20 后端能力位机制） |
| [CABI_MultiInstance_Design.md](design/CABI_MultiInstance_Design.md) | C ABI 多实例/多视口支持设计（UIContext 隔离、焦点路由、生命周期） |
| [CABI_Property_Design.md](design/CABI_Property_Design.md) | C ABI 属性系统设计 |
| [CppBinding_Design.md](design/CppBinding_Design.md) | C++ Binding 设计（DynamicApi 纯动态加载、双模式循环、P1-P17） |
| [CppBinding_UserManual.md](design/CppBinding_UserManual.md) | C++ Binding 用户手册（上手/Config/事件/多实例与子视口/FAQ/API 速查） |
| [LayoutSystem_Design.md](design/LayoutSystem_Design.md) | JSON 布局系统设计 |
| [ControlBase_Design.md](design/ControlBase_Design.md) | 控件基类架构与绘制机制 |
| [GraphTool_Design.md](design/GraphTool_Design.md) | 内部图形工具设计 |
| [EventSystem_Design.md](design/EventSystem_Design.md) | 事件系统设计（EventType → InputBackend → EventQueue → 控件分派 → FocusManager） |
| [FocusSystem_Design.md](design/FocusSystem_Design.md) | 焦点系统设计（Tab 环、FocusBoundary、焦点环绘制） |
| [Image_Design.md](design/Image_Design.md) | Image 图片控件设计 |
| [LuotiAni_Design.md](design/LuotiAni_Design.md) | LuotiAni 关键帧动画引擎设计 |
| [LuotiAni_DevGuide.md](design/LuotiAni_DevGuide.md) | LuotiAni 开发手册（原理/上手/JSON 参考/语义陷阱） |
| [Dialog_Design.md](design/Dialog_Design.md) | Dialog/Popup 弹窗设计 |
| [ComboBox_Design.md](design/ComboBox_Design.md) | ComboBox 下拉框设计 |
| Button_Design.md / Label_Design.md / ... | 各控件详细设计（CheckBox / EditBox / TextArea / ScrollBar / ProgressBar / WinFrame / Menu / Slider / ColorPicker / HandleControl / Splitter / TreeView / NumericUpDown） |

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
