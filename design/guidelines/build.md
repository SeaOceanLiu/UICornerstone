# 构建与测试

## Build library + all tests

```batch
build_scripts\build.bat sdl3    # SDL3 backend (default)
build_scripts\build.bat sfml    # SFML backend
build_scripts\build.bat raylib  # Raylib backend
```

## Build single test

```batch
build_scripts\build_test.bat test_label             # SDL3 (default) → build/sdl3/test/Debug/
build_scripts\build_test.bat test_label sfml        # SFML → build/sfml/test/Debug/
build_scripts\build_test.bat test_label raylib      # Raylib → build/raylib/test/Debug/
build_scripts\build_sdl3.bat test_label             # shortcut for SDL3
build_scripts\build_sfml.bat test_label             # shortcut for SFML
build_scripts\build_raylib.bat test_label           # shortcut for Raylib

# DLL 模式 C ABI 测试需从 *_dll 构建树编译：
cmake --build build/sdl3_dll --target test_treeview_cabi    # → build/sdl3_dll/test/Debug/
cmake --build build/sfml_dll --target test_treeview_cabi    # → build/sfml_dll/test/Debug/
cmake --build build/raylib_dll --target test_treeview_cabi  # → build/raylib_dll/test/Debug/
```

**Available tests (标准)**: test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced, test_winframe, test_graphtool, test_button, test_slider, test_colorpicker, test_combobox, test_dialog, test_handlecontrol, test_numericupdown, test_splitter, test_treeview, test_soft_shadow

**Available tests (DLL 模式 C ABI)**: test_fromsource_cabi, test_api, test_dialog_cabi, test_combobox_cabi, test_numericupdown_cabi, test_splitter_cabi, test_treeview_cabi, test_property_cabi, test_multi_instance_cabi, test_multiviewport_cabi, test_multiinstance_visual_cabi, test_multiviewport_visual_cabi

> **视觉状态测试（test_multiinstance_visual_cabi / test_multiviewport_visual_cabi）**：遵循测试规范用 JSON 布局创建控件（LoadLayout + FindControl + events.onClick 绑定 RegisterAction），参照 `sample_cpp_multiinstance`/`sample_cpp_multiview` 场景，用 Debug 辅助 API 断言视觉状态——hover 跨窗口/跨视口隔离（`Debug_SetMousePosition` 注入窗口内坐标驱动 hover 链路，无头环境真实鼠标不可控）、点击聚焦 + 焦点环并存 → `FocusLost` 清除、按钮点击 → 多窗口测试跨实例内容传递（本窗口 EditBox 内容显示到对方窗口 Label）、多视口测试 Dialog 弹窗显示本视口 EditBox 内容并视口内居中（右下视口 Popup 不显示 bug 回归）。Debug 构建运行。**运行模式**：无参数 = 人工模式（窗口驻留，真实鼠标输入 EditBox → 点击按钮 → 观察对方窗口/弹窗显示内容，关闭窗口退出，无断言）；`auto=<秒>` = 自动断言模式（注入坐标驱动，N 秒后超时退出，无人值守回归）。

## Running Tests

Executables are backend-specific. Assets and DLLs are auto-copied by CMake POST_BUILD:

```batch
cd build\sdl3\test\Debug
test_label.exe

cd build\sfml\test\Debug
test_label.exe

cd build\sdl3_dll\test\Debug
test_treeview_cabi.exe
```

## Important Notes

- Clone with `--recursive` to get all submodules
- Build scripts automatically call VsDevCmd.bat - do not run from vanilla cmd.exe
- Output directory is `build\{sdl3|sfml}\test\Debug\`, not `build\Debug`
- 标准测试（DLL 模式下）自动拷贝 `UICornerstone.dll` 与 `UIBackend_${backend}.dll` 到输出目录（POST_BUILD，且已通过 `add_dependencies` 保证先构建库再拷贝），库代码改动直接生效；若运行时行为与源码不符，先检查 `test\Debug\` 下 DLL 时间戳是否最新
- SDL3 backend DLLs (SDL3.dll, SDL3_ttf.dll, SDL3_image.dll, DebugInfoX64.dll) and assets/layouts folders are auto-copied to output
- SFML backend DLLs and assets are auto-copied to `build\sfml\test\Debug\`
- `NOMINMAX` must appear before any `#include` in files using `std::min`/`std::max` because SDL3 headers pull in `windows.h` transitively

### Backend Compilation Order

- **先编译 SDL3 后端的测试用例**，确保功能正确后再编译其他后端。
- SDL3 编译通过且**经评估审核后**，再启动 SFML 和 Raylib 后端的编译和测试。
- 三后端的差异通常在 RenderDevice、InputBackend 等抽象层，而非控件逻辑本身，因此快速迭代期间只编译 SDL3 即可。

### 构建目录说明

- **标准测试**（非 DLL 模式）：使用 `build/{backend}/` 构建目录

  | 后端   | 构建目录          | 测试输出目录                 |
  | ------ | ----------------- | ---------------------------- |
  | SDL3   | `build/sdl3/`     | `build/sdl3/test/Debug/`     |
  | SFML   | `build/sfml/`     | `build/sfml/test/Debug/`     |
  | Raylib | `build/raylib/`   | `build/raylib/test/Debug/`   |
  | 构建   | `cmake --build build/sdl3 --target test_treeview` | |

- **DLL 模式集成测试**（C ABI 测试，如 `test_*_cabi`）：使用 `build/{backend}_dll/` 构建目录；多实例/多视口测试（test_multi_instance_cabi、test_multiviewport_cabi）经 `CreateInstanceFromPlugin` 动态加载后端 DLL

  | 后端   | 构建目录              | 测试输出目录                         |
  | ------ | --------------------- | ------------------------------------ |
  | SDL3   | `build/sdl3_dll/`     | `build/sdl3_dll/test/Debug/`         |
  | SFML   | `build/sfml_dll/`     | `build/sfml_dll/test/Debug/`         |
  | Raylib | `build/raylib_dll/`   | `build/raylib_dll/test/Debug/`       |
  | 构建   | `cmake --build build/sdl3_dll --target test_treeview_cabi` | |

- `build/{backend}_dll/` 目录与 `build/{backend}/` 是**两个独立的 CMake 构建树**，不可混用。
- 构建命令示例：

  ```batch
  build_scripts\build.bat sdl3     # → build/sdl3/       （标准测试，正确）
  build_scripts\build.bat sfml     # → build/sfml/       （标准测试，正确）
  build_scripts\build.bat raylib   # → build/raylib/     （标准测试，正确）
  ```
- 如果不确定当前构建输出目录，检查 `build/` 下各子目录中的 `test/Debug/`。

### Raylib Backend Notes

- `EndDrawing()` is never called — `present()` manually flushes + swaps
- `PollInputEvents()` is called only in `InputBackend::newFrame()`, once per frame
- `SetTargetFPS` is not used — frame rate limited by `WaitTime` in `RenderDevice::present()`
- Fonts loaded at `size * 96/72` and cached by `(data, size, cpHash)`
- **单窗口架构**：raylib 为单窗口后端（CORE 全局只跟踪最近创建的窗口，预编译 DLL 无源码）。多实例下仅**首个实例** `InitWindow`，后续实例为 headless（`Window::isHeadless()`），窗口/输入 API 全部守卫；能力位声明无 `MULTI_WINDOW`——多窗口测试/样例对第二实例渲染前须查 `GetBackendCapabilities`（详见 BackendAbstraction_Design.md §20）
