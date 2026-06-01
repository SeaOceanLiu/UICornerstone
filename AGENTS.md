# AGENTS.md - UIControls

## Design Rules

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 生成字体时，使用缩放后的字体大小（即和字体大小相关的数据都是缩放后的）
   - 绘制时才对缩放进行处理，而字体因为是已缩放的，所以可以直接绘制

## Build Commands

- **Build library**: Run `build_scripts\build.bat Debug` (requires Visual Studio 2022 environment)
- **Build all tests**: `build_scripts\build_test.bat`
- **Build single test**: `build_scripts\build_test.bat test_label`
- **Available tests**: test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced

## Running Tests

Executables are in `build\test\Debug\`. Assets and DLLs are auto-copied by CMake POST_BUILD:

```batch
cd build\test\Debug
test_label.exe
```

## Important Notes

- Clone with `--recursive` to get all submodules
- Build scripts automatically call VsDevCmd.bat - do not run from vanilla cmd.exe
- Output directory is `build\test\Debug\`, not `build\Debug`
- DLLs (SDL3.dll, SDL3_ttf.dll, SDL3_image.dll, DebugInfoX64.dll) and assets folder are auto-copied to output

## Dependencies (Submodules)

| Submodule | Path | Source |
|----------|------|--------|
| SDL3 | subModules/SDL3 | SeaOceanLiu/UIControls-sdl3 |
| SDL3_ttf | subModules/SDL3_ttf | SeaOceanLiu/UIControls-sdl3_ttf |
| SDL3_image | subModules/SDL3_image | libsdl-org/SDL_image (tag: release-3.2.4) |
| DebugTrace | subModules/DebugTrace | SeaOceanLiu/DebugTrace |
| json | subModules/json | nlohmann/json (tag: v3.12.0) |
| assets | subModules/assets | SeaOceanLiu/UIControls-assets |
| libs | subModules/libs | SeaOceanLiu/UIControls-libs |

## Session History

### 2026-06-01: Phase 1 — SColor Unification

**Changes**:
- `include/SColor.h`: Created standalone SColor class with `constexpr` constructors (float/int/rgba), color operations (brighter/darker/blend), `toSDLColor()`/`toSDLFColor()` bridge methods
- `include/GraphTool.h`: Removed inline SColor class, added `#include "SColor.h"` and `using SColor = ::SColor;` in GraphTool namespace
- `include/ControlBase.h`: StateColor members → SColor, all virtual setters/getters → SColor
- `include/ConstDef.h`: All `static const SDL_Color` → `static const SColor`
- `include/Menu.h`: SDL_Color member vars → SColor
- `include/CheckBox.h`, `include/ProgressBar.h`, `include/Panel.h`, `include/Theme.h`, `include/LayoutParser.h`: All SDL_Color in public API → SColor
- `include/GraphOperaAdapt2d.h`: Replaced duplicate SColor class with `#include "SColor.h"`
- All `src/*.cpp`: Drawing code updated to use `redByte()/greenByte()/blueByte()/alphaByte()` accessors on SColor; brace init → function-style constructors; removed unnecessary `.toSDLColor()` calls where receiving interfaces now take SColor
- `test/test_graphtool.cpp`: Fixed ambiguous SColor constructor call

**Status**: All 9 tests build successfully

### 2026-06-01: Phase 2 — RenderDevice Abstraction (Infrastructure + GraphTool)

**Core Infrastructure**:
- `include/RenderDevice.h`: Abstract interface with ~25 pure virtual methods covering state, primitives, textures, render targets, and frame ops. Includes convenience `drawTriangle()`/`drawQuad()` methods for common patterns. Forward-declares `SDL_Renderer` only in factory function.
- `src/RenderDevice.cpp`: `SDL3RenderDevice` concrete implementation. Converts all abstract types (`SColor`, `SRect`, `Vertex`) to SDL3 equivalents internally. Factory `CreateSDL3RenderDevice()` returns heap-allocated instance.
- `include/MainWindow.h`: Added `RenderDevice* m_renderDevice` member, `getRenderDevice()` accessor, `GET_RENDERDEVICE` macro. `MainWindow` constructor calls `CreateSDL3RenderDevice(m_renderer)`. Added destructor for cleanup.
- `include/ControlBase.h`: Added `getRenderDevice()`/`setRenderDevice()` to `Control` interface and `ControlImpl` implementation.
- `src/ControlBase.cpp`: Implemented `getRenderDevice()` with parent/game fallback chain; `setRenderDevice()` propagates to children; `inheritRenderer()` also inherits render device.
- `CMakeLists.txt`: Added `src/RenderDevice.cpp` to library sources.

**GraphTool Migration**:
- `include/GraphTool.h`: Constructor changed from `SDL_Renderer*` to `RenderDevice*`. `SDL_Texture*` → `void*` in `drawImage()`. Removed `#include "SDL3/SDL.h"`, now includes `RenderDevice.h`. Removed `SDL_Renderer*` from transform stack.
- `src/GraphTool.cpp`: Complete rewrite of all 1452 lines. All 17 `SDL_SetRenderDrawColor` → `setDrawColor(SColor)`. All `SDL_RenderFillRect`/`SDL_RenderRect`/`SDL_RenderLine`/`SDL_RenderPoint` → `fillRect`/`drawRect`/`drawLine`/`drawPoint`. All 16 `SDL_RenderGeometry`+`SDL_Vertex` arrays → `drawTriangle()`/`drawQuad()` calls. All `SDL_RenderTexture` → `drawTexture()`. All clip rect → `setClipRect()`/`clearClipRect()`. No SDL3 headers included.

**Test Migration**:
- All 10 test files updated: `SDL_SetRenderDrawColor`/`SDL_RenderClear`/`SDL_RenderPresent` → `RenderDevice` equivalents. `DrawingContext(renderer)` → `DrawingContext(getRenderDevice())`.

**Remaining SDL calls in controls (not yet migrated from `getRenderer()` to `RenderDevice*`)**:
- `Menu.cpp`: 12 direct SDL calls (SetRenderDrawColor, RenderFillRect, RenderLine) in `MenuPanel::draw()` and `MenuBar::draw()`
- `ControlBase.cpp`: 4 SDL calls in `drawBackground()`/`drawBorder()`
- `Label.cpp`: 4 SDL calls in debug drawing
- `EditBox.cpp`: 8 SDL calls (clip, fill, cursor drawing)
- `TextArea.cpp`: 8 SDL calls (clip, fill, cursor drawing)
- `ScrollBar.cpp`: 4 SDL calls
- `ProgressBar.cpp`: 4 SDL calls
- `Bench.cpp`: 8 SDL calls (grid drawing)
- `LuotiAni.h`: Texture/render-target operations
- `Actor.cpp`: Texture creation and rendering
- `Material.cpp`: Texture rendering

**Status**: test_graphtool, test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced, test_winframe, test_button — all 10 build successfully