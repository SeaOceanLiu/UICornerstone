# AGENTS.md - UIControls

## Language

始终使用中文进行交互。

## Design Rules

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 生成字体时，使用缩放后的字体大小（即和字体大小相关的数据都是缩放后的）
   - 绘制时才对缩放进行处理，而字体因为是已缩放的，所以可以直接绘制

## Build Commands

- **Build library**: Run `build_scripts\build.bat Debug` (requires Visual Studio 2022 environment)
- **Build all tests**: `build_scripts\build_test.bat`
- **Build single test**: `build_scripts\build_test.bat test_label`
- **Available tests**: test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced, test_winframe, test_graphtool, test_button

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

### 2026-06-01: Phase 2 — RenderDevice Abstraction (Infrastructure + GraphTool + Control Migration)

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

**Full Control Migration (54 SDL calls → RenderDevice)**:
- `ControlBase.cpp`: `drawBackground()`/`drawBorder()` — 4 SDL calls → `getRenderDevice()->setDrawColor()`/`fillRect()`/`drawRect()`
- `Bench.cpp`: Loading progress bar — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`/`drawRect()`
- `Label.cpp`: Debug drawing — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`drawRect()`
- `ScrollBar.cpp`: Track/thumb drawing — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`
- `ProgressBar.cpp`: Background/progress fill — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`
- `EditBox.cpp`: Clip rect, selection fill, cursor — 6 SDL calls → `setClipRect()`/`clearClipRect()`/`setDrawColor()`/`fillRect()`
- `TextArea.cpp`: Clip rect, selection fill (with blend), cursor — 6 SDL calls → `setClipRect()`/`clearClipRect()`/`setBlendMode(BlendMode::Blend)`/`setDrawColor()`/`fillRect()`
- `Menu.cpp`: MenuItem hover, checkmark, separator, MenuPanel items, MenuBar bg/items/border — 12 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`/`drawLine()`
- `MenuPanel::drawShadow()`: Removed unused `getRenderer()` null-check
- `CheckBox.cpp`: Removed 4 unused `getRenderer()` null-checks
- `Actor.cpp`: 4 `SDL_RenderTexture` → `getRenderDevice()->drawTexture(m_texture, ...)`
- `Material.cpp`: 1 `SDL_RenderTexture` → `getRenderDevice()->drawTexture(m_texture, ...)`

**Interface Addition**: Added `BlendMode` enum + `setBlendMode()` to `RenderDevice` interface and `SDL3RenderDevice` impl (for TextArea selection alpha blending)

**Status**: All 10 tests build successfully. Phase 2 core migration complete.

### 2026-06-01: Phase 3 — Texture/Surface Abstraction (Complete)

**Core Types**:
- `include/Texture.h`: New abstract `Texture` class — `width()`, `height()`, `setBlendMode()`, `setAlphaMod()`, `getBlendMode()`, `getAlphaMod()`
- `include/Surface.h`: New abstract `Surface` class — pixel ops (`getPixel`/`setPixel`), `blit()` (scaled + unscaled), `createTexture(RenderDevice*)`, `rotate()`, factory statics (`create`/`loadFromFile`/`loadFromMemory`)
- `include/RenderDevice.h`: Replaced `void*` texture API with `Texture*`/`SharedTexture`; added `createTextureFromSurface(Surface*)`, `SharedTexture`/`SharedSurface` type aliases
- `src/RenderDevice.cpp`: Added `SDL3Texture` (wraps `SDL_Texture*`), `SDL3Surface` (wraps `SDL_Surface*`, implements pixel/format/blit/rotate ops), all `Surface` factory statics; `SDL3RenderDevice` updated to use `Texture*`/`SharedTexture`

**`m_texture` Migration (SDL_Texture* → SharedTexture)**:
- `include/ControlBase.h`: `SDL_Texture* m_texture` → `SharedTexture m_texture`
- `include/Actor.h`: `getTexture()` returns `Texture*`, `setTexture()` takes `SharedTexture`
- `src/Material.cpp`: Removed `SDL_DestroyTexture`; `SDL_SetTextureBlendMode`/`SDL_SetTextureAlphaMod` → `m_texture->setBlendMode()`/`m_texture->setAlphaMod()`
- `src/Actor.cpp`: Uses `surface->createTexture(device)` instead of bridge; texture size via `m_texture->width()`/`height()`
- `include/GraphTool.h`: `drawImage()` takes `Texture*` instead of `void*`

**`m_surface` Migration (SDL_Surface* → SharedSurface)**:
- `include/ControlBase.h`: `SDL_Surface* m_surface` → `SharedSurface m_surface`
- `src/Actor.cpp`: `loadFromFile()`/`loadFromResource()` use `Surface::loadFromFile()`/`loadFromMemory()`; `setParent()` uses `m_surface->createTexture()`
- `ControlBase.cpp`: Copy/assign semantics unchanged (shared_ptr handles refcounting)
- Added `Surface::rotate()` for GPU-accelerated rotation (render-to-texture + readback, inside `SDL3Surface`)

**`LuotiAni.h` Full Migration (~180 SDL calls)**:
- Total rewrite: 1341-line header-only animation engine
- `SDL_Surface*` → `SharedSurface` in `OpData`, `m_frameSurfaces`, all helpers
- `SDL_BlendMode` → `BlendMode` enum; `SDL_GetTicks()` → `std::chrono::steady_clock`
- `SDL_Log`/`SDL_GetError()` → `printf`; `Uint64`/`Uint8` → standard types
- `SDL_CreateSurface`/`SDL_BlitSurfaceScaled`/`SDL_SetSurfaceAlphaMod`/`SDL_SetSurfaceBlendMode` → `Surface::create()`/`blit()`/`setAlphaMod()`/`setBlendMode()`
- `IMG_Load_IO` → `Surface::loadFromMemory()`
- Removed dead code: `normalRotateSurface`, `matrixRotateSurface`, `gpuRotateSurface`
- GPU rotation → `Surface::rotate(angle, getRenderDevice())`
- `loadFromStream(SDL_IOStream*)` replaced with `parseJsonDesc()` using `std::ifstream`
- Pixel helpers simplified for RGBA8888 format

**Bridge Removal**:
- Removed `createTextureFromSDLSurface` and `getNativeRenderer` from `RenderDevice` (no longer used)
- Removed `loadTextureFromSurface(SDL_Surface*)` from `Actor` (no longer used)
- Removed `struct SDL_Renderer`/`struct SDL_Surface` forward declarations from `RenderDevice.h`

**Animation Fixes**:
- Frame actors in `LuotiAni::prepare()` now get non-zero rect via `frame->setRect()`
- `LuotiAni::setRect()` propagates unconditionally to frame actors (removed `m_isPrepared` guard)
- `Button::setLuotiAni()` syncs button rect to LuotiAni
- `Button::setRect()` propagates to `m_luotiAni`

**Testing**:
- Added 2x scaled LuotiAni Button test (`g_button6` in `test_button.cpp`)
- All 10 tests build successfully

### 2026-06-02: Phase 4 — Remove Umbrella `#include <SDL3/SDL.h>` from Non-Backend Headers

**Umbrella include completely removed from 8 headers**:
- `Menu.h`: No SDL types used — removed entirely
- `StateMachine.h`: `SDL_Log()` → `printf()` — removed entirely
- `Material.h`: `Uint8`/`SDL_ALPHA_OPAQUE` → `uint8_t`/`255` — replaced with `<cstdint>`
- `Actor.h`: `Uint8`/`SDL_ALPHA_OPAQUE` → `uint8_t`/`255` — removed entirely
- `Utility.h`: `SDL_FRect`/`SDL_Rect`/`SDL_FPoint` bridge methods → `#include <SDL3/SDL_rect.h>`
- `SColor.h`: `SDL_Color`/`SDL_FColor` bridge methods → `#include <SDL3/SDL_pixels.h>`
- `ControlBase.h`: `SDL_Renderer*` member/API → `#include <SDL3/SDL_render.h>`
- `EditBox.h`: Removed umbrella, kept `SDL3/SDL_keyboard.h` and `SDL3/SDL_clipboard.h`

**Backend headers with umbrella preserved** (inherently SDL-dependent):
- `MainWindow.h`: Header-only window/renderer manager — keeps `#include <SDL3/SDL.h>`
- `ResourceLoader.h`: Narrowed to `SDL3/SDL_thread.h` + `SDL3/SDL_iostream.h`; `.cpp` gets umbrella

**Other improvements**:
- `RenderDevice.h`: `struct SDL_Renderer;` forward declaration instead of any SDL include
- `ResourceLoader.cpp`: Added explicit `#include <SDL3/SDL.h>` (was getting it transitively)

**Status**: All 10 tests build successfully. Only `MainWindow.h` retains the umbrella `#include <SDL3/SDL.h>` (as a intentional backend dependency).

### 2026-06-02: Phase 5 — Font/TextRenderer Abstraction (Complete)

**Core Infrastructure**:
- `include/Font.h` — abstract `Font` class with `getSize()`, `SharedFont` alias
- `include/TextRenderer.h` — abstract `TextRenderer` interface with `loadFont`, `loadFontFromMemory`, `measureText`, `getFontHeight`, `drawText` (plain + wrapWidth), factory `CreateSDL3TextRenderer(RenderDevice*)`
- `src/TextRenderer.cpp` — `SDL3Font` wraps `TTF_Font*`, `SDL3TextRenderer` wraps `TTF_TextEngine*`; factory calls `TTF_Init()` and `TTF_Quit()`
- `RenderDevice.h/cpp` — added `getNativeHandle() -> void*` for backend bridge
- `MainWindow.h` — added `TextRenderer* m_textRenderer`, creation/destruction, `getTextRenderer()`, `GET_TEXTRENDERER` macro
- `ControlBase.h/cpp` — added `TextRenderer* m_textRenderer`, `getTextRenderer()`, `setTextRenderer()` with parent propagation, inherit in `inheritRenderer()`
- `CMakeLists.txt` — added `src/TextRenderer.cpp`

**Full Control Migration (Label + EditBox + TextArea)**:
- `Label.h/cpp`: Removed `#include <SDL3_ttf/SDL_ttf.h>`, replaced `TTF_Font*`/`TTF_TextEngine*`/`vector<TTF_Text*>` with `SharedFont` + `TextRenderer`; `TTF_FontStyleFlags` → `int`; added `computeLineOffsets()`; `SDL_Cursor` forward-declared
- `EditBox.h/cpp`: Removed `#include <SDL3_ttf/SDL_ttf.h>`, replaced `TTF_Font*`/`TTF_TextEngine*`/`TTF_Text*` with `SharedFont` + `TextRenderer`; removed `createTextEngine()`/`createTextObjects()`/`destroyTextObjects()`/`recreateTextObjects()`; added `loadFontInternal()`
- `TextArea.h/cpp`: Removed `TTF_TextEngine* m_textEngine` member; replaced all `TTF_CreateText`/`TTF_GetTextSize`/`TTF_DrawRendererText`/`TTF_DestroyText` with `getTextRenderer()->measureText()`/`drawText()`
- `LayoutParser.h/cpp`: `parseFontStyle()` return type `TTF_FontStyleFlags` → `int`; removed `TTF_STYLE_*` literal dependencies (use plain int values)

**Test Cleanup**:
- Removed `TTF_Init()`/`TTF_Quit()` calls from all 10 test files (TextRenderer internally manages TTF lifecycle)

**Status**: All 10 tests build successfully. No non-backend header includes `SDL3_ttf/SDL_ttf.h` or umbrella `SDL3/SDL.h`.