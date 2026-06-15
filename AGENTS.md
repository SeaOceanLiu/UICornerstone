# AGENTS.md - UICornerstone

## Language

始终使用中文进行交互。

## File Encoding

所有源代码文件（.h/.cpp）必须保存为 **UTF-8 with BOM** 编码格式。不要使用 UTF-8 without BOM 或其他编码。

## Design Rules

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 生成字体时，使用缩放后的字体大小（即和字体大小相关的数据都是缩放后的）
   - 绘制时才对缩放进行处理，而字体因为是已缩放的，所以可以直接绘制

## Build Commands

**Build library + all tests:**
```batch
build_scripts\build.bat sdl3    # SDL3 backend (default)
build_scripts\build.bat sfml    # SFML backend
build_scripts\build.bat raylib  # Raylib backend
```

**Build single test:**
```batch
build_scripts\build_test.bat test_label             # SDL3 (default)
build_scripts\build_test.bat test_label sfml        # SFML
build_scripts\build_test.bat test_label raylib      # Raylib
build_scripts\build_sdl3.bat test_label             # shortcut for SDL3
build_scripts\build_sfml.bat test_label             # shortcut for SFML
build_scripts\build_raylib.bat test_label           # shortcut for Raylib
```

**Available tests**: test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced, test_winframe, test_graphtool, test_button

## Running Tests

Executables are backend-specific. Assets and DLLs are auto-copied by CMake POST_BUILD:

```batch
cd build\sdl3\test\Debug
test_label.exe

cd build\sfml\test\Debug
test_label.exe
```

## Important Notes

- Clone with `--recursive` to get all submodules
- Build scripts automatically call VsDevCmd.bat - do not run from vanilla cmd.exe
- Output directory is `build\{sdl3|sfml}\test\Debug\`, not `build\Debug`
- SDL3 backend DLLs (SDL3.dll, SDL3_ttf.dll, SDL3_image.dll, DebugInfoX64.dll) and assets/layouts folders are auto-copied to output
- SFML backend DLLs and assets are auto-copied to `build\sfml\test\Debug\`
- `NOMINMAX` must appear before any `#include` in files using `std::min`/`std::max` because SDL3 headers pull in `windows.h` transitively

## RGBA8888 Pixel Format (CRITICAL)

`Surface::create(w, h)` creates an RGBA8888 surface. On **little-endian x86** (the only relevant target), RGBA8888 memory layout is:

| Byte offset | 0 (LSB) | 1 | 2 | 3 (MSB) |
|------------|---------|---|---|---------|
| Channel | A | B | G | R |

When writing pixels as `uint32_t`, the value MUST be formatted as:

```cpp
uint32_t pixel = (R<<24) | (G<<16) | (B<<8) | A;
// Example: opaque black = 0x000000FF, opaque red = 0xFF0000FF
```

Common mistakes:
- `0xFF000000` = R=255, G=0, B=0, A=0 = **transparent red** (NOT opaque black!)
- `0x000000FF` = R=0, G=0, B=0, A=255 = **opaque black** (correct for black)
- `0xFFFFFFFF` = R=255, G=255, B=255, A=255 = opaque white (correct)
- `0x00000000` = R=0, G=0, B=0, A=0 = transparent (correct)

TL;DR: **Alpha goes in the lowest byte (bits 0-7), Red in the highest byte (bits 24-31).**

## Dependencies (Submodules)

| Submodule | Path | Source |
|----------|------|--------|
| SDL3 | subModules/SDL3 | SeaOceanLiu/UICornerstone-sdl3 |
| SDL3_ttf | subModules/SDL3_ttf | SeaOceanLiu/UICornerstone-sdl3_ttf |
| SDL3_image | subModules/SDL3_image | libsdl-org/SDL_image (tag: release-3.2.4) |
| json | subModules/json | nlohmann/json (tag: v3.12.0) |
| assets | subModules/assets | SeaOceanLiu/UICornerstone-assets |
| libs | subModules/libs | SeaOceanLiu/UICornerstone-libs |
| SFML | subModules/SFML | SFML (v3, Debug DLLs) |

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

### 2026-06-03: Phase 7 — TTF_Text* Caching in Label Layer

**Problem**: `TTF_CreateText()` was called on every `measureText()` / `drawText()` invocation, creating and destroying `TTF_Text*` objects each frame. This is both wasteful and fragile — if `TTF_Text*` creation fails or gets corrupted, all subsequent TTF operations hang/crash.

**Changes**:
- `include/TextRenderer.h`: Added cache-friendly methods — `createText(Font*, const string&)` → `void*`, `destroyText(void*)`, `measureText(void*)`, `drawText(void*, ...)`. Original `measureText(Font*, const string&)` / `drawText(Font*, ...)` kept for backward compatibility (used by EditBox/TextArea for one‑off temp text objects).
- `src/TextRenderer.cpp`: Implemented new methods. `createText` calls `TTF_CreateText()`, `destroyText` calls `TTF_DestroyText()`, `measureText(void*)` calls `TTF_GetTextSize()` on the cached object, `drawText(void*, ...)` calls `TTF_SetTextColor()` + `TTF_DrawRendererText()` on the cached object.
- `include/Label.h`: Added `std::vector<void*> m_cachedTexts` member, `releaseTexts()` method.
- `src/Label.cpp`:
  - `Label::~Label()` / `Label::recreate()` → calls `releaseTexts()` to destroy cached `TTF_Text*` objects.
  - `Label::create()` → after loading font and creating multiline text, creates one `TTF_Text*` per line via `renderer->createText()`.
  - `Label::computeLineOffsets()` → uses cached texts for line width measurement; if `truncateLine()` modifies a line, the cached text is recreated in‑place.
  - `Label::draw()` → draws each line using the cached `TTF_Text*` instead of creating/destroying per frame.

**Tests verified**: All 10 tests build successfully. `test_button.exe` and `test_label.exe` run without crashes (both contain Chinese captions with scaled/2x buttons).

**Status**: All 10 tests build successfully.

### 2026-06-03: Phase 8 — ResourceProvider Abstraction (Complete)

**Problem**: Code directly depended on `ResourceLoader` singleton for file I/O, mixing resource bundle loading with filesystem access. Adding filesystem-only resources (e.g., animation JSON) required awkward workarounds.

**Core Infrastructure**:
- `include/ResourceProvider.h`: Abstract `ResourceProvider` interface — `readFile()` → `shared_ptr<vector<char>>`, `openFileStream()` → `SDL_IOStream*`, `exists()` → `bool`. Factory `createFilesystem(basePath)`.
- `src/ResourceProvider.cpp`: `FilesystemResourceProvider` implementation using `fopen`/`fread`.
- `include/ControlBase.h`: Added `ResourceProvider* m_resourceProvider` member, `getResourceProvider()`/`setResourceProvider()` interface and implementation.
- `src/ControlBase.cpp`: `setResourceProvider()` propagates to children; `inheritRenderer()` also inherits resource provider.
- `include/MainWindow.h`: Added `ResourceProvider* m_resourceProvider` member, creation/destruction, `getResourceProvider()`, `GET_RESOURCEPROVIDER` macro.

**Font Data Lifetime Fix**:
- Root cause: `TTF_OpenFontIO(stream, true)` may reference font data lazily; when `shared_ptr<vector<char>>` went out of scope after `loadFromResource`/`loadFontInternal`, the font referenced freed memory → crash in `TTF_GetTextSize`.
- `include/Label.h`: Added `shared_ptr<vector<char>> m_fontData` member.
- `src/Label.cpp`: `loadFromResource()` stores font data in `m_fontData` instead of a local variable; `releaseFont()` also resets `m_fontData`.
- `include/EditBox.h`: Added `shared_ptr<vector<char>> m_fontData` member.
- `src/EditBox.cpp`: `loadFontInternal()` stores font data in `m_fontData` instead of a local variable.

**Status**: All 10 tests build successfully. `test_button.exe` runs without crash (was crashing before the lifetime fix).

### 2026-06-04: Phase 9 — Remove ResourceLoader (Complete)

**Changes**:
- **`ConstDef.h/cpp`**: Moved `FontName` enum and `fontFiles` map from `ResourceLoader.h`
- **`include/Label.h`**, **`include/Bench.h`**, **`src/EditBox.cpp`**: Removed `#include "ResourceLoader.h"`
- **`src/WinFrame.cpp`**: Inlined RID string constants (cross_up/cross_over/cross_down PNGs)
- **`test/test_button.cpp`**: Inlined RID rotateBtn_jsonc path, removed `detachLoadingThread()` call
- **All test files**: Removed `detachLoadingThread()` calls (no longer needed without async loading thread)
- **`src/Bench.cpp`**: Simplified — loading is now instantaneous (no more async resource bundle loading), removed progress bar drawing code no longer applicable
- **`CMakeLists.txt`**: Removed `ResourceLoader.cpp` from build
- **`include/ResourceLoader.h`**, **`src/ResourceLoader.cpp`**: **Deleted entirely** — old resource bundle system is gone

**Status**: All 10 tests build and run successfully.

### 2026-06-04: Phase 10 — SDL Cursor Abstraction (Complete)

**Problem**: `Label.h` and `WinFrame.h` directly used `SDL_Cursor*` and called `SDL_CreateSystemCursor`/`SDL_SetCursor`/etc. directly.

**Changes**:
- **`include/Cursor.h`**: New abstract `Cursor` class with `SystemCursorType` enum (20 cursor types), static factories `createSystem()`, `getDefault()`, and `setCurrent()`.
- **`src/Cursor.cpp`**: `SDLCursor` implementation wrapping `SDL_Cursor*` with owned/unowned semantics. Maps `SystemCursorType` → `SDL_SystemCursor` for `SDL_CreateSystemCursor`. `getDefault()` returns a static singleton.
- **`include/Label.h`**: Replaced `SDL_Cursor* m_hoverCursor/m_defaultCursor` with `Cursor*`, removed `struct SDL_Cursor;` forward declaration.
- **`src/Label.cpp`**: All `SDL_CreateSystemCursor`/`SDL_GetCursor`/`SDL_SetCursor`/`SDL_DestroyCursor` → `Cursor::createSystem`/`Cursor::getDefault`/`Cursor::setCurrent`/`delete`.
- **`include/WinFrame.h`**: Replaced 5 `SDL_Cursor*` members with `Cursor*`.
- **`src/WinFrame.cpp`**: All cursor creation/setting/destruction migrated to `Cursor` API.
- **`CMakeLists.txt`**: Added `src/Cursor.cpp`.

**Non-backend headers now SDL_Cursor-free**: `Label.h` and `WinFrame.h` no longer reference any SDL cursor types.

**Status**: All 10 tests build and run successfully.

### 2026-06-04: Phase 11 — Remove SDL_Renderer* from ControlBase (Complete)

**Problem**: After Phase 2 introduced `RenderDevice` abstraction, `getRenderer()`/`setRenderer(SDL_Renderer*)` in `ControlBase.h` was dead code — no drawing code used it anymore — but it kept `#include <SDL3/SDL_render.h>` as a hard dependency in the primary control base header.

**Changes**:
- **`include/ControlBase.h`**: Removed `#include <SDL3/SDL_render.h>`, removed `SDL_Renderer *getRenderer/setRenderer` from `Control` interface and `ControlImpl` member/overrides; removed `SDL_Renderer *m_renderer` member
- **`src/ControlBase.cpp`**: Removed `getRenderer()`/`setRenderer()` implementations (8 lines, 25 lines respectively); removed `m_renderer` propagation from `addControl()` and `inheritRenderer()`; removed `m_renderer` from constructors/copy/assign
- **`include/MainWindow.h`**: Removed `#define GET_RENDERER` macro (unused everywhere)
- **`include/ConstDef.h`**: Replaced `SDL_WINDOWPOS_CENTERED`/`SDL_WINDOW_RESIZABLE`/`SDL_WINDOW_HIGH_PIXEL_DENSITY` with hex literal equivalents, removing hidden SDL macro dependency
- **`include/EditBox.h`**: Removed `#include <SDL3/SDL_keyboard.h>` (no SDL keyboard types used in header)
- **`include/Bench.h`** / **`src/Bench.cpp`**: Removed dead `drawCenteredRectangle(SDL_Renderer*)` method
- **`test/test_label.cpp`**: `BENCH->getRenderer()` → `MAINWIN->getRenderer()`

**Result**: `ControlBase.h` no longer has any direct SDL include or SDL type in its public API.

### 2026-06-04: Design Documents Update

**BackendAbstraction_Design.md** updates:
- Progress table (§1.4): Added Phases 7-11 (TTF_Text caching, ResourceProvider, Remove ResourceLoader, SDL Cursor, Remove SDL_Renderer)
- New sections (§10-§14): Detailed design records for Phases 7-11
- §7.3 (current status): Updated to reflect Phase 11 completion; added ResourceProvider/Cursor to ready interfaces
- SFML/raylib sections renumbered to Phase 12/13 and moved to §15-§16
- Summary table (§17) and execution order (§18) updated with all 13 phases
- Note added about numbering divergence between plan and execution

**ResourceLoader_Design.md** updates:
- Marked as deprecated (⚠️), noting replacement by ResourceProvider
- Kept original content for historical reference

### 2026-06-05: Phase 6 — MainWindow Cleanup + InputBackend Standalone + SDL-free MainWindow.h + AppCallbacks Migration (Complete)

**Note**: Phase 6 in the design doc covers Window abstraction, InputBackend, BackendPlugin, Event system, and AppCallbacks. BackendPlugin was previously completed; this session focused on the remaining Window/InputBackend cleanup and all-test migration.

**Infrastructure Changes**:
- **`src/InputBackend.cpp`** (new): Extracted `SDL3InputBackend` from `src/Window.cpp` into its own file; factory uses `window->nativeHandle()` instead of SDL-specific cast
- **`src/Window.cpp`**: Removed `SDL3InputBackend` class and `CreateSDL3InputBackend` factory (moved to InputBackend.cpp)
- **`include/MainWindow.h`**: Removed `#include <SDL3/SDL.h>`, `SDL_Renderer* m_renderer`, `SDL_DisplayID m_displayId`, `getWindow()` (SDL_Window*), `getRenderer()` (SDL_Renderer*); replaced direct SDL calls with `Window` abstract methods; replaced `handleWindowEvent(SDL_WindowEvent&)` with `onWindowResized(int,int)` / `onWindowMoved(int,int)`; renamed `getWindowObject()` → `getWindow()`; now SDL-free
- **`include/Bench.h`**: Replaced `SDL_AppResult` with `int` (removed SDL type from header)
- **`src/Bench.cpp`**: `SDL_APP_CONTINUE` → `0`; `SDL_AppResult` → `int`
- **`src/EditBox.cpp`**: Added explicit `#include <SDL3/SDL_keyboard.h>` and `<SDL3/SDL_keycode.h>` (lost transitive include from MainWindow.h)
- **`src/TextArea.cpp`**: Same SDL keyboard include fix
- **`src/Actor.cpp`**, **`src/CheckBox.cpp`**, **`src/ControlBase.cpp`**, **`src/Label.cpp`**, **`src/LayoutParser.cpp`**, **`src/ProgressBar.cpp`**: Added explicit `#include <SDL3/SDL.h>` (lost transitive include)
- **`include/MainWindow.h`**: Added `run(AppCallbacks*)` for owned-loop mode, plus tick-based `init/processEvents/update/render/shutdown` API
- **`src/MainWindow.cpp`** (new): Implements `run()` — polls InputBackend, handles WindowClose/Resize/Move, dispatches old-style to BENCH, notifies AppCallbacks via `onUpdate()`/`onRender()`/`onEvent()`
- **`src/BackendPlugin.cpp`**: Moved `SDL_Init` + `SDL_SetAppMetadata` from test files into `BackendManager::initialize()`
- **`CMakeLists.txt`**: Added `src/InputBackend.cpp`, `src/MainWindow.cpp`; added `_HAS_STD_BYTE=0` for Windows SDK compatibility

**AppCallbacks & Event Infrastructure**:
- **`include/AppCallbacks.h`** (new): Abstract interface — `onInit()`, `onEvent(const Event&)` (optional, default empty), `onUpdate()`, `onRender()`, `onQuit()`
- **`include/EventTypes.h`**: Added `WindowMoved` event type + `EventWindowMoved` struct; enum now has MouseMove/MouseDown/MouseUp/MouseWheel/KeyDown/KeyUp/TextInput/WindowResize/WindowMoved/WindowClose/FocusGained/FocusLost
- **`include/InputBackend.h`**: Added `pollEvent(Event&)` abstract method — polls SDL events, populates both new (EventType+union) and old (EventName+std::any) fields for dual-run backward compatibility
- **`src/InputBackend.cpp`**: Full `SDL3InputBackend::pollEvent()` implementation handling all 10 SDL event types (mouse, keyboard, text, wheel, window)
- **`include/StateMachine.h`**: Added default `Event` constructor, `EventWindowMoved` in union, `copyUnion()` handles WindowMoved, `<memory>` include

**All 10 Test Files Migrated (SDL callbacks → AppCallbacks)**:
- **`test/test_button.cpp`**: Uses **Mode 1** (owned loop) — `MAINWIN->run(&app)`. Migrated in previous session.
- **The other 9 test files** use **Mode 2** (tick-based API) with SDL callbacks preserved. SDL provides the main loop; the SDL callbacks delegate to MainWindow's Mode 2 lifecycle methods:
  ```
  // SDL_AppInit → g_app.onInit() or MAINWIN->init(&g_app)
  // SDL_AppEvent → unchanged (dispatches to BENCH directly)
  // SDL_AppIterate → MAINWIN->update(&g_app) + MAINWIN->render(&g_app)
  // SDL_AppQuit → MAINWIN->shutdown(&g_app)
  ```
- Each test file adds an AppCallbacks subclass (`LabelApp`, `EditBoxApp`, etc.) with `onInit()`/`onUpdate()`/`onRender()`/`onQuit()`, and a static `g_app` instance. The SDL_App* functions call into these AppCallbacks methods via MainWindow's Mode 2 API.
- **`test/test_graphtool.cpp`**: Custom `SColor(0.941f)` background; no BENCH, uses `DrawingContext` directly; Space/Escape handling stays in `SDL_AppEvent`; timer auto-stepping in `onUpdate()`
- **`test/test_progressbar.cpp`**: Animation logic (`SDL_GetTicks()`) in `onUpdate()`
- **`test/test_layout_advanced.cpp`**: `g_reloader.poll()` in `onUpdate()`
- **`test/test_menu.cpp`**: `testGraphToolInitialize()` in `onInit()`
- **`test/test_editbox.cpp`**: `GET_INPUTBACKEND->startTextInput()` in `onInit()`
- `#define SDL_MAIN_USE_CALLBACKS 1` and `#include <SDL3/SDL_main.h>` are **preserved** in all 9 files (SDL callback mode remains active)

**Result**: Phase 6 complete. All 10 tests build and run successfully. Both Mode 1 (`run()`) and Mode 2 (tick-based API called from within SDL callbacks) are validated.

### 2026-06-05: Phase 12 — Event System Migration (Controls → Union API)

**Changes**: Migrated all 8 control handleEvent() implementations from old Event API (`EventName` + `std::any` + `any_cast`) to new union-based API (`EventType` + union fields):

- **EditBox.cpp**: 7 handlers migrated — MOUSE_LBUTTON_DOWN, MOUSE_MOVING, MOUSE_LBUTTON_UP, TEXT_INPUT, KEY_DOWN, KEY_UP, ON_FOCUS watcher (ON_FOCUS remains old API as custom internal event)
- **TextArea.cpp**: 12 handlers migrated — TEXT_INPUT, KEY_DOWN (×3), mouse scrollbar delegation, MOUSE_LBUTTON_DOWN, MOUSE_WHEEL, MOUSE_MOVING, MOUSE_LBUTTON_UP, KEY_DOWN navigation
- **ScrollBar.cpp**: 3 handlers — MOUSE_LBUTTON_DOWN, MOUSE_LBUTTON_UP, MOUSE_MOVING
- **CheckBox.cpp**: 1 handler — position event check → EventType switch
- **Button.cpp**: 1 handler — position event check + FINGER event fallback
- **Label.cpp**: 1 handler — position event check + FINGER event fallback
- **Menu.cpp (3 classes)**: MenuItem, MenuPanel, MenuBar — all position event checks migrated
- **WinFrame.cpp**: 1 handler — comprehensive hasPos + event type checks migrated

**Key changes per control**:
- Removed all `std::any_cast<shared_ptr<SPoint>>` / `std::any` dependencies
- Removed all `try { ... } catch (...) { }` blocks from event handlers
- Mouse events use `event->mousePos` (for MouseMove) or `event->mouseButton` (for MouseDown/Up)
- Keyboard events use `event->keyEvent` directly
- TextInput uses `event->textInput.text` (char[32])
- MouseWheel uses `event->mouseWheel`
- FINGER_* events (custom, no EventType) retain old API fallback in Button/Label

**Status**: All 10 tests build successfully.

### 2026-06-05: Phase 12 Fix — Event(EventName, any) 构造自动映射

**Bug**: Mode 2（SDL 回调）测试文件通过 `Event(EventName, any)` 旧构造函数创建事件，该构造将 `m_type` 硬编码为 `EventType::None`，导致所有控件的新 API 类型检查（`event->m_type == EventType::MouseDown` 等）失败。

**Root cause**: `StateMachine.h` 中 `Event` 类仅有 `EventName` 前向声明，无法在构造函数体内访问枚举值做映射。

**Fix**:
- `StateMachine.h`: `Event(EventName, any)` 构造声明移至类外（无函数体）
- `EventQueue.h`: 提供内联构造体，完整实现 `EventName → EventType` 映射 + union 字段填充（MouseMove/MouseDown/MouseUp/MouseWheel/TextInput/KeyDown/KeyUp）
- `EventTypes.h`: 移入旧数据结构（`KeyEventData/TextInputEventData/FocusEventData/MouseWheelEventData`）+ `<string>`
- `EditBox.h/TextArea.h`: 删除重复数据结构定义；恢复意外删除的 `#include "Label.h"`（提供 AlignmentMode）

**结果**: 无需修改任何测试文件，旧构造函数自动为新旧双 API 填充数据。所有 10 测试事件响应正常。

### 2026-06-05: Phase 13 — SFML Backend + Remove SDL from Core Sources (In Progress)

**Problem**: Core source files (`src/*.cpp`) still used SDL API calls directly (`SDL_Log`, `SDL_GetTicks`, `SDL_GetBasePath`, `SDL_GetMouseState`, etc.), preventing SFML backend compilation.

**Platform Abstractions**:
- `include/PlatformUtils.h` (new): `Platform::Log()` (printf-based), `Platform::GetTicks()` (std::chrono), `Platform::GetBasePath()` (Win32 GetModuleFileNameA)
- `include/Window.h`: Added `getMousePosition(float& x, float& y)` — returns mouse coords relative to window
- `include/InputBackend.h`: Added `getModState()` — returns keyboard modifier state as int

**Core File SDL Removal (17 files)**:
- `src/ConstDef.cpp`: `SDL_GetBasePath()` → `Platform::GetBasePath()`, `SDL_Log()` → `Platform::Log()`
- `src/ControlBase.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`
- `src/Bench.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`
- `src/Label.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`
- `src/Actor.cpp`: Removed SDL include; `Platform::Log()` for errors
- `src/CheckBox.cpp`: Removed SDL include
- `src/LayoutParser.cpp`: Removed SDL include
- `src/ProgressBar.cpp`: Removed SDL include; added `#define NOMINMAX` before includes
- `src/HotReloader.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`, `SDL_Log()` → `Platform::Log()`
- `src/EditBox.cpp`: Added `#define NOMINMAX` before includes
- `src/TextArea.cpp`: Added `#define NOMINMAX` before includes
- `include/LuotiAni.h`: `fopen`/`fread` → `FILE*` I/O instead of SDL I/O; removed `SDL_IOFromFile`/`SDL_ReadIO`
- `include/ResourceProvider.h`: Removed `SDL_IOStream*` return type and parameter
- `src/ResourceProvider.cpp`: Removed `SDL_IOStream*` dependency
- `include/SColor.h`: Added `NOMINMAX` guard before SDL `windows.h` conflict
- `include/Utility.h`: `SDL_Log()` in `assertMsg` → `Platform::Log()`
- `src/BackendManager.cpp`: `#ifdef` guards around backend-specific `extern BackendAPI`
- `include/InputBackend.h`: Restored SDL3 factory declarations under backend namespace guards

**SFML Backend Implementation**:
- `src/backend/sfml/Window.cpp`: `getMousePosition()` returns `sf::Mouse::getPosition(*m_window)`
- `src/backend/sfml/InputBackend.cpp`: `getModState()` returns `sf::Keyboard::isKeyPressed()` for Ctrl/Alt/Shift
- `src/backend/sfml/RenderDevice.cpp`: Full SFML render device (415 lines) — draws primitives via sf::RenderWindow/OpenGL; Surface via sf::Image; Texture via sf::Texture

**SVG Rasterization (nanosvg)**:
- **Problem**: LuotiAni animation engine uses SVG as native image format. SFML's `sf::Image` doesn't support SVG, so `Surface::loadFromMemory` called with SVG data printed `Failed to load image from memory` to `sf::err()` and returned a SharedSurface wrapping a 0x0 image, silently corrupting animation data.
- **Fix**: Added bundled nanosvg (from SDL3_image's source tree) to SFML backend (`src/backend/sfml/nanosvg.h`, `nanosvgrast.h`). `Surface::loadFromMemory` now detects SVG data by magic bytes (`<?xml`, `<svg`, `<!DOCTYPE`) and rasterizes it via `nsvgParse`/`nsvgRasterize` to RGBA pixels, creating a valid `sf::Image`. Non-SVG paths use the original `sf::Image` constructor with try-catch + 0x0 dimension check.
- `src/backend/sfml/RenderDevice.cpp`: Added `#include <cstdlib>`, `<cmath>`, `<string.h>`, `<stdlib.h>`, `<math.h>` for nanosvg; replaced `Surface::loadFromMemory` with SVG-aware implementation

**Build Results**:
- SDL3 backend: All 10 test executables compile and run (`test_button` verified)
- SFML backend: `UICornerstone.lib` compiles; all 10 test executables compile and link; `test_button` runs without errors (no more `Failed to load image from memory`), animation buttons render via nanosvg SVG rasterization

### 2026-06-06: Phase 13 Fixes — 4 SFML Visual/Performance Issues (Complete)

**Problem**: User reported 4 SFML-specific issues after test restoration:
1. test_editbox: lag after typing
2. test_graphtool: missing middle black in test group 3
3. test_layout_advanced: resize mispositions controls
4. test_progressbar: animation slower

**Changes (14 files)**:

**SFML Window.cpp:84**: Removed `setVerticalSyncEnabled(true)` — matches SDL3 behavior (no vsync), improves frame rate in tests that don't need it.

**SFML RenderDevice.cpp**: Fixed `drawRect(filled=true)` with SBrush::NoPen — now draws pen outline even in filled mode. `fillRect` issues: changed from `sf::VertexBuffer` to per-rectangle `sf::RectangleShape` (avoids primitive batch overflow). `drawTexture` now works with non-power-of-two textures + `sf::Quads`. Added `fillTriangle` implementation via `sf::ConvexShape`. Removed `sf::PrimitiveType::TriangleFan` approach for `drawTriangle`.

**SFML TextRenderer.cpp**: Fixed `drawText(void*, wrapWidth)` crash — null-check cached `m_textObj`. Fixed `drawText(Font*, text, x, y, color)` — now uses cached `sf::Text` (global static map, keyed by text string + font size) instead of creating/destroying per frame. Fixed alignment offsets.

**SFML InputBackend.cpp**: Fixed `getModState()` — now returns `KeyMod::None` explicitly (was returning uninitialized uint16_t). Fixed mouse wheel delta sign (SFML delta is typically -1/+1 vs SDL3 expects -120/+120).

**SFML Window.cpp**: Fixed resize event not using `sf::View` → viewport mismatch. Now calls `m_window->setView(sf::View(sf::FloatRect(0, 0, width, height)))` on resize to match SDL3 auto-viewport behavior.

**SFML BackendPlugin.cpp**: Added `sf::ContextSettings` with `sf::StencilMask` for stencil support (required by `sf::ConvexShape`/`sf::RectangleShape` rendering path).

**SFML Clipboard/TextInput**: Moved clipboard code after `SFML/Window.hpp` to avoid Windows.h macro pollution. Text input state now uses boolean toggle.

**Test files**: Changed all 10 tests from `BENCH->setOnInitial(...)` to direct initialization call (no longer needed without SDL callback mode). Removed SDL_App* callbacks, `SDL_MAIN_HANDLED`, `SDL_main` headers. All tests now use `MAINWIN->run(&app)` (Mode 1) with `main()` as entry point. Maintains `AppCallbacks` pattern.

**Bench.cpp**: Fixed `ControlBase.cpp` missing `break` in Disabled switch case. Fixed `EventQueue.h` mouse-button mapping (default case now maps to `MouseButton::Left` instead of falling through).

**Remaining Issues**: SFML `setVerticalSyncEnabled` was removed to match SDL3 frame rate. If animations still appear slower on SFML, the issue is likely `sf::sleep(sf::milliseconds(1))` in Window.cpp event polling — SDL3 does not sleep between poll cycles.

**Known Issues**:
- NOMINMAX required in source files because SDL3 headers in `subModules/` pull in `windows.h` transitively via public include dir

### 2026-06-07: Raylib InputBackend — Infinite MouseDown Loop Fix (Complete)

**Problem**: Raylib's `IsMouseButtonPressed()` returned `true` every time `pollEvent()` was called because the phase machine reset to `Keyboard` on every `GetTime()` advancement (which advanced between calls within the same frame). This cleared `m_consumedMouseButtons`, causing the same press to be detected again → infinite `MouseDown` event flood.

**Root cause**: `GetTime()` in `pollEvent()` advanced by microseconds between consecutive calls within `processEvents()`'s `while (pollEvent(event))` loop, restarting the phase machine and clearing consumed state each time.

**Fix (4 files)**:
- `include/InputBackend.h`: Added `virtual void newFrame() {}` — signals start of a new frame's event processing. Default empty so SDL3/SFML backends need no changes.
- `src/backend/raylib/InputBackend.cpp`:
  - `newFrame()` override: Resets `m_phase` to `Keyboard`, `m_consumedMouseButtons` to `0`, `m_keyConsumed`/`m_charConsumed` to `false`.
  - Modified `GetTime()` check in `pollEvent()` to NOT reset `m_consumedMouseButtons` — it now only resets `m_keyConsumed`/`m_charConsumed` (for multi-key-per-frame processing). `m_consumedMouseButtons` persists across all `pollEvent()` calls within a render frame.
- `src/MainWindow.cpp`: `processEvents()` calls `inputBackend->newFrame()` before entering the `while (pollEvent(event))` loop.
- Removed all debug `printf` traces from `MainWindow.cpp` and `RenderDevice.cpp`.

**Result**: `test_button` runs without infinite loop. No more `MouseDown` event flooding.

**Status**: Raylib backend builds and runs. No other backends affected.

### 2026-06-07: Phase 14 C2 — Raylib drawTriangle/drawQuad CCW Winding Fix

**Problem**: test_graphtool only showed the red thin line (width=1, uses `DrawLineEx`). All thick lines (width > 1, via `drawQuad` → `DrawTriangle`) and triangle-based fills (filled rounded rects/ellipses/arcs/polygons via `drawTriangle`) were invisible.

**Root cause**: raylib's `DrawTriangle(v1, v2, v3, color)` requires CCW (counter-clockwise) winding and does NOT flip CW triangles internally. The initial `drawTriangle` and `drawQuad` implementations in `src/backend/raylib/RenderDevice.cpp` had the winding logic inverted:
- In y-down coordinates: `signed_area > 0` → CW, `signed_area < 0` → CCW
- Bug: when `area < 0` (CCW), the code **swapped** the last two vertices making the triangle CW; when `area >= 0` (CW), it passed the triangle as-is (still CW)
- Result: **every triangle** was passed as CW to `DrawTriangle` and culled by OpenGL backface culling

**Fix** (`RaylibRenderDevice::drawTriangle` and `drawQuad`):
- CCW input (`area < 0`): pass vertices **as-is** → `DrawTriangle(v0, v1, v2)`
- CW input (`area >= 0`): **swap** last two vertices → `DrawTriangle(v0, v2, v1)`

**Impact**: All triangle-based rendering now works correctly in the raylib backend (thick lines, rounded rects, ellipses, arcs, polygons, styled drawing, checkmarks, menu backgrounds).

**Status**: All 10 tests build and run. SDL3/SFML backends unaffected (they handle winding internally).

### 2026-06-08: Phase 14 D1 — Raylib Resize Freeze & Font Size Fix (Complete)

**Problem (resize freeze)**: Dragging the window border to resize and releasing the mouse caused the raylib backend to freeze. Root cause traced through multiple iterations:

**Root cause chain**:
1. `EndDrawing()` (called inside `present()`) calls `PollInputEvents()` internally.
2. `InputBackend::newFrame()` also called `PollInputEvents()` → double‑polling consumed the `IsMouseButtonPressed/Released` one‑shot state, breaking mouse clicks.
3. Removing `EndDrawing()` from `present()` fixed double‑polling but the resize freeze returned.
4. **Real freeze cause**: during/after a resize drag, `IsWindowResized()` returned TRUE **500+ times** in a single `processEvents()` call, flooding the event loop with duplicate `WindowResize` events of the same dimensions and preventing rendering.
5. After the drag, the `MOUSE_UP` arriving in the same event batch triggered a control handler that froze.

**Fixes (6 files)**:

| File | Change |
|------|--------|
| `src/backend/raylib/TextRenderer.cpp` | Font cache (`unordered_map<FontCacheKey, weak_ptr<RaylibFont>>`). Load font at `size * 96/72` to match SDL3_ttf point→pixel conversion. `getFontHeight` uses `MeasureTextEx("Ay")`. `wrapWidth` scaled by 96/72. |
| `src/backend/raylib/InputBackend.cpp` | `newFrame()` calls `PollInputEvents()` once per frame. Phase order changed to `Keyboard → CharInput → **WindowEvents** → MouseButton → MouseMove → MouseWheel → Done`. `m_resizeDetected` flag suppresses `MOUSE_UP` when a `WindowResize` arrives in the same batch. |
| `src/backend/raylib/RenderDevice.cpp` | `present()` no longer calls `EndDrawing()` (avoids internal `PollInputEvents`). Manually flushes batch + `SwapScreenBuffer()`. 60 Hz `WaitTime` frame limiter. |
| `src/backend/raylib/Window.cpp` | Removed `SetTargetFPS(60)` (moved to RenderDevice). Added `SetTraceLogLevel(LOG_WARNING)`. Fixed flag‑bit mapping (`0x00000020` → `FLAG_WINDOW_RESIZABLE`). |
| `include/MainWindow.h` + `src/MainWindow.cpp` | WindowResize dedup by `(w, h)` — skips events with same dimensions as last processed. 500‑event safety guard in `processEvents`. |

**Key insights**:
- `IsWindowResized()` in the bundled raylib returns TRUE repeatedly for the same dimensions (the flag is set by a GLFW callback but not cleared by the first `IsWindowResized()` call in this raylib build).
- `EndDrawing()` must not be called in `present()` because its internal `PollInputEvents()` collides with the one in `newFrame()`.
- Resize drag produces `MOUSE_UP` that confuses controls when arriving without a visible `MOUSE_DOWN` (the press was on the window border / NC area).

**Status**: All 10 raylib tests build and run. Resize is smooth, mouse clicks work, close works. SDL3 backend unaffected.

## Raylib Backend Notes

- `EndDrawing()` is never called — `present()` manually flushes + swaps
- `PollInputEvents()` is called only in `InputBackend::newFrame()`, once per frame
- `SetTargetFPS` is not used — frame rate limited by `WaitTime` in `RenderDevice::present()`
- Fonts loaded at `size * 96/72` and cached by `(data, size, cpHash)`

### 2026-06-09: Phase 15 — CheckBox/Label Performance Optimization (Complete)

**Problem**: `test_checkbox` took ~48s to initialize 16 checkboxes. Root cause was an O(n²) recreate cascade amplified by missing font cache in SDL3 backend.

**Root cause chain**:
1. `Bench::addControl()` → `resolveChildPercentages()` iterated ALL children, calling `setRect` on every existing child.
2. `CheckBox::setRect()` called `recreate()` unconditionally (no dirty-rect check).
3. Each CheckBox recreate destroyed and re-created its Label caption, including font reloading.
4. SDL3 TextRenderer had **no font cache** — every `loadFontFromMemory()` called `TTF_OpenFontIO()`, taking ~800ms per call.
5. With 16 checkboxes, the cascade produced 120 redundant recreates × 3 font loads = 360+ font opens.

**Changes (5 files)**:

| File | Change |
|------|--------|
| `src/backend/sdl3/TextRenderer.cpp` | Added font cache (`m_fontCache` keyed by `(contentHash, size)`, `m_pathCache` by `(path, size)`). Eliminated redundant `TTF_OpenFontIO` calls. |
| `include/CheckBox.h` | Added `bool m_layoutDone = false` member. |
| `src/CheckBox.cpp` | `setRect` dirty-rect check. `recreate()` resets `m_layoutDone` to `false`. `create()` sets `m_layoutDone = true` after layout. `createCaption()` callback checks `m_layoutDone` before calling `adjustSpaceAssignment()`. |
| `src/Label.cpp` | `setRect` dirty-rect early return. `setParent` dirty-parent early return. |
| `src/backend/raylib/TextRenderer.cpp` | Removed debug `printf` and `Platform::GetTicks()` timing (leftover from earlier session — only removed unused variables, font cache was already present). |

**Debug instrumentation removed from production sources**:
- `Label.cpp`: `g_recreateDepth` thread_local + `[LABEL_RECREATE]` printf
- `CheckBox.cpp`: `[CHECKBOX_RECREATE]` timing printf
- `SDL3 TextRenderer.cpp`: `[FONT_LOAD]` + `[FONT_HIT]` timing printf
- `Raylib TextRenderer.cpp`: `[FONT_HIT]` + `[FONT_RELOAD]` timing printf
- All unused `Platform::GetTicks()` / timing variables removed

**Results**:

| Backend | Before | After |
|---------|--------|-------|
| SDL3    | ~48s   | ~7.2s |
| SFML    | —      | ~6.9s |
| Raylib  | —      | ~4.6s |

All 10 tests build and run on all 3 backends. ~6.5× speedup on SDL3.

**Remaining cost**: ~80 `TTF_CreateText` calls (~25ms each) during the 16-checkbox initialization. Further optimization would require caching `TTF_Text*` objects across recreates (risky — text content can change).

### 2026-06-12: Phase 15 Bugfix Round 1 — 6 Fixes Across All Backends

**Fix 1 — EditBox ON_FOCUS event union corruption (all backends)**:
- Root cause: `Event` class in `include/StateMachine.h` had `customInt` and `customPtr` in the same union. When `EditBox::setFocused()` set both fields, the second assignment (`customPtr = this`) overwrote the first (`customInt = ON_FOCUS`), so `beforeEventHandlingWatcher` could never match `EventName::ON_FOCUS` and never called `setFocused(false)` on the previously-focused EditBox.
- Fix: Moved `customInt` and `customPtr` out of the union as regular members. Updated all constructors, copy/move operators, and `copyUnion()` (removed `case EventType::Custom`).

**Fix 2 — WinFrame title bar drag area (all backends)**:
- Problem: Title bar drag (Step 4) covered the full title bar height at any X. Left edge dragging was possible on non-resizable WinFrames.
- Fix: Added `localMouse.x >= m_edgeMargin` and `localMouse.y >= m_edgeMargin` checks to restrict drag to the interior of the title bar, excluding resize edge zones and the close button area.

**Fix 3 — Raylib Chinese TextInput shows "?"**:
- Problem: `RaylibTextRenderer::loadFontFromMemory()` only loaded ASCII codepoints (0x20–0x7E) on initial font load. `EditBox::insertText()` modified `m_text` but never called `loadFontInternal()`, so the TextRenderer's lazy expansion (via `loadFontFromMemoryWithText`) was never triggered with the new CJK codepoints.
- Fix: Added `loadFontInternal()` call in `EditBox::insertText()` to reload the font with the updated text's codepoints via `loadFontFromMemoryWithText()`. Also added `ensureFontCodepoints()` lazy expansion in the raylib TextRenderer's `drawText()`/`measureText()` for the cached `void*` text path used by Label.

**Fix 4 — Raylib EditBox direction key repeat**:
- Problem: `m_repeatKey` was reset to 0 in `newFrame()`, losing the key repeat state between frames. Holding Left/Right arrow keys did not generate repeated `KeyDown` events.
- Fix: Removed `m_repeatKey = 0;` from `newFrame()` so the repeat state persists across frames. The `IsKeyDown(m_repeatKey)` check ensures stale keys are ignored. Key repeat timing: 350ms initial delay / 50ms repeat interval.

**Fix 5 — Remove DebugTrace**:
- Removed `#include "DebugTrace.h"` from `include/MainWindow.h`. Removed two `DEBUG_STREAM` calls in `onWindowResized()` and `onWindowMoved()`.

**Fix 6 — Add test name to window title**:
- Added `MainWindow::setTitle()` delegating to `Window::setTitle()`.
- Added `MAINWIN->setTitle("test_xxx");` as first line in each of the 10 test files' `onInit()` method.

**Verification**: All 10 tests build and run on all 3 backends (SDL3, SFML, Raylib).

### 2026-06-12: R2-R4 — UICornerstone C ABI Implementation (Compile Complete)

**New files**:
- `include/UICornerstoneAPI.h`: 公有 C ABI 头文件 — `UIBackendCallbacks` 回调查表（7 类回调约 40 个函数指针）、`UIControlHandle`/`UIRenderDeviceHandle` 等 10+ 个不透明句柄、所有 `UICornerstone_*` 函数声明
- `src/CallbackAdapters.h` + `.cpp`: 5 个 Adapter 类（CallbackWindow/RenderDevice/InputBackend/TextRenderer/ResourceProvider）将回调查表委托为现有的 C++ 抽象接口
- `src/UICornerstoneAPI.cpp`: C ABI 实现 — `Init` / `Shutdown` / `SetViewport` / `ProcessEvents` / `Update` / `Render` / `IsQuitRequested` + 6 个控件工厂（Button/Label/CheckBox/EditBox/ProgressBar/Panel）+ 通用控件操作 + `LoadLayout`/`FindControl`/`RegisterAction` 骨架

**编译修复**:
- SRect 成员名：所有 `.x`/`.y`/`.w`/`.h` → `.left`/`.top`/`.width`/`.height`
- CheckBox 文本：通过 `getCaption()->setCaption(text)` 设置
- `Button::setOnClick` lambda：参数包装为 `shared_ptr<Button>`
- `LayoutParser` 接口：使用 `parseLayout()` 而非 `parse()`
- 添加 `#include "StateMachine.h"` 到 CallbackAdapters.cpp 以使用完整 Event 类型
- 所有 4 个新文件保存为 UTF-8 with BOM 解决 MSVC C4819

**验证**：
- `UICornerstone.lib` 编译 0 错误
- 全部 10 个 SDL3 测试编译通过（无回归）

### 2026-06-12: R5 — JSON 布局 C ABI 包装 (Complete)

**`UICornerstone_LoadLayout` 实现**：
- 注册所有 `g_actions` 中的回调到 LayoutParser（通过 `registerHandler` 适配 `UIActionCallback` → `function<void(shared_ptr<Control>)>`）
- 调用 `parser.parseLayout()` 解析 JSON
- 成功后添加根控件到 `BENCH`（`BENCH->addControl(root)`）
- 添加所有 MenuBar 到 `BENCH`
- 遍历 `parser.getAllControlIds()`，将每个控件注册到 `g_controlsById`（使 `UICornerstone_FindControl` 可用）

**`UICornerstone_Render` 修正**：
- 移除 `clear()` 和 `present()` — 调用者负责管理全帧生命周期
- 仅做 `setClipRect(g_viewport)` → `BENCH->draw()` → `clearClipRect()`

**验证**：编译通过，全部 10 个 SDL3 测试无回归。

### 2026-06-15: Phase 16 — RGBA8888 像素格式根因排查 + DLL 桥接验证

**问题**：test_fromsource 中 WinFrame 关闭按钮的黑色 X 不可见（仅深灰色背景），怀疑 DLL 桥接存在问题。

**Root cause**：
- `Surface::create(32, 32)` 创建 RGBA8888 surface。在 little-endian x86 上，字节顺序为 A(LSB), B, G, R(MSB)
- 代码中 `0xFF000000` 实际上是 **R=255, G=0, B=0, A=0** → 透明红色，而非期望的不透明黑色
- 修复后用 `0x000000FF` (R=0,G=0,B=0,A=255) = 不透明黑；`0x505050FF` (R=80,G=80,B=80,A=255) = 不透明深灰

**DLL 桥接验证**：
- 通过 `GetUIBackendCallbacks` 桥接的 `drawTexture` 正确工作（SDL_RenderTexture 经桥接到 SDL3RenderDevice）
- PNG 文件加载和程序化表面两种纹理来源均通过桥接正常工作
- Bench 的 `getRenderDevice()` 通过 `GET_RENDERDEVICE`（`BackendManager::instance()->renderDevice()`）正确获取，无空指针问题
- Hover/Press 状态图片也可见（证明三个状态 Actor 的纹理绘制全部正常）

**关键教训**：
- 初次看到"没有黑 X"是因为黑 X (0,0,0) 在深灰背景 (80,80,80) 上确实容易被忽略，而非 DLL 桥接问题
- 所有后续"sdl3/dll/backend bridge"方向的排查都是误判

**验证**：test_fromsource 运行正常，所有控件可见，点击/悬停交互正常。

### 2026-06-14: test_fromsource — 单文件编译 + 窗口复用 + 控件可见性 + 事件注入

**单文件编译**（CMakeLists.txt）：
- 去掉 `BACKEND_SRC` glob，`test_fromsource.cpp` 通过 `#include` 引入后端 `.cpp` 文件（Window.cpp / RenderDevice.cpp / TextRenderer.cpp / InputBackend.cpp / Cursor.cpp / **BackendPlugin.cpp**）
- 添加 `src/backend/` 到 include 路径供 `BackendBridge.h` 查找

**BackendPlugin.cpp 复用**（取代内联拷贝）：
- `BackendPlugin.cpp` 新增 `#ifdef UICORNERSTONE_REUSE_SDL_WINDOW` 条件编译路径
- 复用路径：`sdl3CreateWindow` 用 `new SDL3Window(g_reuseWindow, g_reuseRenderer)`（不创建新窗口）；`sdl3Init`/`sdl3Destroy` 为空操作
- 默认路径保持原装不动（`CreateSDL3Window` + `SDL_Init`/`SDL_Quit`），不影响现有 10 个测试 + test_api
- `SDL3Backend_SetReuseWindow(g_window, g_renderer)` 在 SDL_AppInit 中 `SDL_CreateWindowAndRenderer` 后调用
- `test_fromsource.cpp` 中的 136 行内联拷贝替换为 `#define UICORNERSTONE_REUSE_SDL_WINDOW` + `#include "../../src/backend/sdl3/BackendPlugin.cpp"`

**窗口复用**：
- `SDL_AppQuit` 调用 `UICornerstone_Shutdown` 清理（`~SDL3Window` 销毁 SDL 句柄）

**控件可见性修复**（UICornerstoneAPI.cpp）：
- 所有 6 个工厂函数新增 `ctl->create()` 调用（Builder/LayoutParser 模式需要显式 create，控制器才真正初始化）
- 所有工厂新增 `ctl->setVisible(true)`
- 原来 Label 的 `m_isCreated` 初始为 `false` → `recreate()` 中的 `if(!m_isCreated) return;` 跳过所有初始化
- 工厂新增 `setFont(FontName::HarmonyOS_Sans_SC_Regular)` 确保 Label 有默认字体
- 新增 `UICornerstone_SetBGColor` API，同时设置 Normal/Hover/Pressed 三态背景色（hover = brighter(0.3), pressed = darker(0.3)）

**事件注入机制**（UICornerstoneAPI.h/.cpp）：
- 新增 `UICornerstone_PushUIEvent(const UIEvent*)` API + 内部 `std::queue<UIEvent>` 队列
- `UICornerstone_ProcessEvents` 改为先处理队列事件，再后备轮询 InputBackend
- 解决 SDL_MAIN_USE_CALLBACKS 模式下 `SDL_PollEvent` 不返回事件的问题

**test_fromsource 事件桥接**：
- `sdlEventToUIEvent()` 函数将 SDL_Event 转为 UIEvent（data buffer 格式）
- `SDL_AppEvent` 中调用 `uiPushUIEvent(&ue)` 注入
- 添加 `UICornerstone_SetOnClick` 回调指针，点击按钮输出 `"Button clicked!"`

**测试验证**：
- SDL3 静态/DLL 全部 11 个目标（10 测试 + test_fromsource）编译通过
- test_fromsource 运行：Button/Label 可见，Hover 变色正常，点击输出 Button clicked!

### 2026-06-13: test_fromsource — DLL 动态加载 + 后端源码编译 (Complete)

**问题**：之前的 `test_fromsource` 使用 `test_api` 模式（`UICornerstone_InitFromPlugin` + `UIBackend_sdl3.dll` + JSON 布局），未满足"只使用 UICornerstone.dll，后端/控件从源码编译，无声明式 UI"的需求。

**设计原则**：
- `UICornerstone.dll` 是唯一的 DLL 依赖（含控件 + C ABI 实现）
- 后端（SDL3 RenderDevice/Window/InputBackend/TextRenderer/Cursor）从源码编译进 exe
- 控件通过 C ABI 工厂函数编程式创建（`UICornerstone_CreateButton/Label/CheckBox/EditBox/ProgressBar`）
- 无 JSON 布局，无 `UIBackend_sdl3.dll` 依赖

**架构图**：
```
test_fromsource.exe
  ├── 动态加载: LoadLibrary("UICornerstone.dll")
  │     → GetProcAddress 解析所有 C ABI 函数指针
  │     → UICornerstone_Init(callbacks) 传入回调查表
  ├── 源码编译 (src/backend/sdl3/*.cpp):
  │     → BackendPlugin.cpp (GetUIBackendCallbacks)
  │     → RenderDevice.cpp, Window.cpp, InputBackend.cpp
  │     → TextRenderer.cpp, Cursor.cpp
  ├── 控件工厂: UICornerstone*.dll C ABI 函数
  └── 帧循环: ProcessEvents → Update → Clear → Render → Present
```

**编译策略**：
- 链接 `UICornerstone_dll`（import lib）供后端源码解析 `Surface::registerFactories` 等 `CORE_API dllimport` 符号
- `UICORNERSTONE_BUILD_SHARED=1` 作用于整个 target，后端源码正确从 DLL 导入符号
- test_fromsource.cpp 尽管 sees dllimport 声明，但只通过 GetProcAddress 函数指针调用，无直接函数引用 → 无 LNK2001
- `test/CMakeLists.txt` + `test_fromsource.cpp`：全新实现，不依赖 `test_api.c` 代码
- 180 帧后自动退出（非手动关闭窗口）

**验证**：
```
=== test_fromsource: UICornerstone.dll + backend from source ===
OK: loaded UICornerstone.dll           # LoadLibrary 成功
SDL3: GetUIBackendCallbacks ready       # 后端源码编译
BackendManager: initialized from callback table  # C ABI Init
Controls: 5/5 created                   # Button/Label/CheckBox/EditBox/ProgressBar
Frame loop...
Done, 180 frames                        # 帧循环正常完成
```

### 2026-06-13: R10b — UICornerstone.dll 拆分：核心 DLL + 后端插件 DLL

**DLL 拆分方案**：
- `UICornerstone.dll`（原名 `UICornerstone_dll.dll`，3.2MB）：只含 CORE_SOURCES（控件、布局、C ABI），不再包含后端实现
- `UIBackend_sdl3.dll`（268KB）：独立的后端插件 DLL，包含 BACKEND_SOURCES（RenderDevice/TextRenderer/Window/InputBackend/Cursor + BackendPlugin），通过 `LoadLibrary` 动态加载
- 静态 `UICornerstone.lib` 保持不变（CORE_SOURCES + BACKEND_SOURCES），10 个现有测试无回归

**关键技术变更**：
- `CMakeLists.txt`：`UICornerstone_dll` target 移除 BACKEND_SOURCES，`OUTPUT_NAME` 设为 `UICornerstone`；新增 `UIBackend_sdl3` SHARED target（连接 UICornerstone_dll 导入库）
- `Surface.h/cpp`、`Cursor.h/cpp`：静态工厂方法（`create`/`loadFromFile`/`loadFromMemory` / `createSystem`/`getDefault`/`setCurrent`）从后端文件迁移到核心，使用 `registerFactories` 委托机制；`CORE_API` 宏控制跨 DLL 导出
- `BackendPlugin.cpp`（三后端）：`GetUIBackendCallbacks` 用 `BACKEND_PLUGIN_EXPORT` 导出；内部调用 `RegisterSDL3SurfaceFactories()` / `RegisterSDL3CursorFactories()` 注册工厂
- `BackendManager.cpp`：`initialize(string)` 路径守卫 `#if !UICORNERSTONE_BUILD_SHARED`，静态链接时也注册工厂
- `UICornerstoneAPI.cpp`：`InitFromPlugin` 移除静态回退，纯 `LoadLibrary` 路径
- `test/CMakeLists.txt`：test_api 额外复制 `UIBackend_sdl3.dll` 到输出目录

**验证**：SDL3 静态/DLL 全部编译通过。test_api 输出显示 `UICornerstone: loaded UIBackend_sdl3.dll`，确认纯动态加载路径。

### 2026-06-13: R9 Bugfix Round — C ABI 事件循环 + 稠密图元修复

**Bug 1 — UI_EVENT_BUTTON 宏偏移错误 (所有点击交互失效)**：
- Root cause: `#define UI_EVENT_BUTTON(ev) (*(int*)(ev)->data)` 从 `data[0]` 读取，但 `bridge_pollEvent` 把鼠标按键值写到 `data[8]`
- 结果：`event.mouseButton.button` 始终是垃圾值，`== MouseButton::Left` 永假 → Button/CheckBox/EditBox 全部无视点击
- 修复：`UI_EVENT_BUTTON(ev)` → `(*(int*)((ev)->data + 8))`

**Bug 2 — 键盘修饰键丢失 (Ctrl-C/V/X/Shift-Arrow 无效)**：
- Root cause: `bridge_pollEvent` 只存 keycode 到 UIEvent，跳过 `key.mod`；`CallbackInputBackend::pollEvent` 硬编码 `KeyMod::None`
- 修复：`bridge_pollEvent` 在 `data[4..5]` 写入 `uint16_t` modifier；新增 `UI_EVENT_KEY_MOD(ev)` 宏；`CallbackInputBackend` 读取并传给 `EventKey`

**Bug 3 — CallbackRenderDevice::drawTriangle/drawQuad 画轮廓而非实心**：
- Root cause: 两个方法都用 `drawLine` 画 3/4 条轮廓线
- 结果：CheckBox 框框和钩钩变成空心线，EditBox/TextArea 选择不可见，所有粗线渲染异常
- 修复链：
  - `UIBackendCallbacks` 末尾新增 `fillTriangle`/`fillQuad` 函数指针
  - `BackendBridge.h` 新增 `bridge_fillTriangle`/`bridge_fillQuad` 委托到原生 `RenderDevice::drawTriangle/drawQuad`（实心）
  - 3 个 `BackendPlugin.cpp` 全部接通新回调
  - `CallbackRenderDevice` 优先调 `fillTriangle`/`fillQuad`，退化为 2 个三角形拼 Quad

**Bug 4 — 剪贴板 stub (Ctrl-C/V 无反应)**：
- Root cause: `CallbackInputBackend::setClipboardText/getClipboardText` 是空实现
- 修复：`UIBackendCallbacks` 新增 `setClipboardText`/`getClipboardText`；`BackendBridge.h` 新增 bridge；3 个 BackendPlugin 接通；Adapter 委托到原生 `InputBackend`

**其他 stub 检查**：`Texture::setBlendMode/setAlphaMod`、`createTextureFromSurface/createRenderTexture`、`setRenderTarget/resetRenderTarget/readPixels` 均为 C ABI 未使用功能，无害暂不改。

**test_api.c 增强**：
- 布局改为 3 行：Button/Label/CheckBox(行0) → EditBox/ProgressBar/Panel(行1) → TextArea(行2)
- 提示标签高度 12→16px；控件间隙 4→8px
- 新增 `hint_textarea`、`ta_demo` ID，共 18 个 ID 全数验证
- 帧循环改为 `while (!UICornerstone_IsQuitRequested())` 等待用户关闭窗口
- 新增 `UICornerstone_Clear`/`UICornerstone_Present` API

**验证**：全部 11 个目标（10 测试 + test_api）SDL3 编译通过。test_api 运行 ALL PASS。

### 2026-06-13: R7 — GetUIBackendCallbacks 三后端实现 (Complete)

**Files**:
- `src/backend/BackendBridge.h` (new, 250 行): 桥接函数头文件，将 `UIBackendCallbacks` 回调查表委托为 C++ 抽象接口（RenderDevice/InputBackend/TextRenderer/Window/ResourceProvider 五个模块全部虚方法已实现）
- `src/backend/sdl3/BackendPlugin.cpp`: 添加 `GetUIBackendCallbacks()` — 用 BackendBridge 初始化 `UIBackendCallbacks` 结构体
- `src/backend/sfml/BackendPlugin.cpp`: 同上
- `src/backend/raylib/BackendPlugin.cpp`: 同上

**`UICornerstone_InitFromPlugin` 双路径**：先 `LoadLibraryA("UIBackend_sdl3.dll")` + `GetProcAddress` 动态加载，失败后回退 `extern "C" GetUIBackendCallbacks()` 静态链接

**桥接策略**：通过 `reinterpret_cast` 将 `void*` 句柄转为 C++ 抽象接口指针，调用对应虚方法（避免每后端单独实现原生调用）

**纹理/字体所有权**：Texture 和 Font 通过堆上分配的 `shared_ptr` 管理，`destroy` 时 `delete` 释放

**验证**：SDK3/SFML/Raylib 三个后端均编译通过，`InitFromPlugin("sdl3")` 静态链接路径验证通过

### 2026-06-13: R8 — CMake DLL 模式 (Complete)

**`UICORNERSTONE_BUILD_DLL` 选项** (默认 OFF):
- `UICornerstone` 目标保持 STATIC（向后兼容）
- `UICornerstone_dll` 独立 SHARED 目标
- `UICORNERSTONE_API` 导出宏 (`__declspec(dllexport/dllimport)`)
- 三后端 DLL 模式编译通过

**"编译两次"策略**：UICornerstone 编译为静态库后，DLL 目标单独编译。以编译时间换 consumer 零感知、CMake 结构清晰、导出控制精确。

**验证**：6 配置全量编译通过（SDK3/SFML/Raylib × 静态/DLL）

### 2026-06-13: R10 — 构建验证（Complete）

**6 配置验证结果**：

| 后端 | 静态 (UICornerstone.lib) | DLL (UICornerstone_dll.dll) |
|------|------------------------|---------------------------|
| SDL3 | 10/10 测试通过 | 3.3MB DLL + 10/10 测试通过 |
| SFML | 10/10 测试通过 | 3.5MB DLL + 测试通过 |
| Raylib | 10/10 测试通过 (LNK4098 已知) | 4.7MB DLL + test_button 通过 |

### 2026-06-13: R9 — 纯 C ABI 测试 (Complete)

**test_api.c**：`test/test_api.c`，只包含 `UICornerstoneAPI.h`，编译为 C（`.c` 扩展名）。

显示所有 6 种静态控件（Button/Label/CheckBox/EditBox/ProgressBar/Panel）的视觉测试：
1. `InitFromPlugin("sdl3")` 创建原生窗口
2. `SetViewport` 设置 800×480 视口
3. `RegisterAction` + `LoadLayout` 从 JSON 布局加载 3×2 网格
4. `FindControl` 验证 9 个控件 ID 全部存在
5. `SetText` 设置 CheckBox 标题和 EditBox 文本
6. 帧循环：`ProcessEvents` → `Update` → `Render`，用户关闭窗口退出
7. `Shutdown`

**Virtual Inheritance 指针调整 Bug**：
- Root cause: `ControlImpl` 使用 `virtual public Control`（`include/ControlBase.h:226`），虚拟继承导致 `Control*` 子对象地址与派生类对象地址不同
- 工厂函数原先 `reinterpret_cast<UIControlHandle>(buttonPtr)` 存储的是派生类地址
- 通用操作 `static_cast<Control*>(voidPtr)` 不做指针调整（`static_cast` 从 `void*` 到 `T*` 相当于 `reinterpret_cast`）
- 修复：所有工厂函数改为 `reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl))`，确保存储 `Control*` 地址
- 影响：6 种控件工厂全部修复，`UICornerstone_CreateButton/Label/CheckBox/EditBox/ProgressBar/Panel`

**验证**：SDL3 静态/DLL、SFML 静态、Raylib 静态全部编译通过，test_api 在所有配置上 ALL PASS。

### 2026-06-12: R6 — BackendManager 回调表初始化 (Complete)

**BackendManager 改造**：
- 新增 `BackendManager::initialize(const UIBackendCallbacks* callbacks)` 方法
- 从回调查表创建 CallbackAdapter 实例（Window/RenderDevice/TextRenderer/InputBackend）
- 适配器通过标准访问器对外暴露，与内置后端路径共存
- `BackendPlugin.h` 包含 `UICornerstoneAPI.h` 以使用 `UIBackendCallbacks` 类型

**UICornerstone_Init/Shutdown 重构**：
- `Init` 委托 BackendManager 创建适配器，不再直接管理
- `Shutdown` 委托 BackendManager::shutdown()，配合 `delete g_resourceProvider`
- 修复 `shutdown()` 中 `m_renderDevice` 未 delete 的泄漏

**验证**：编译通过，全部 10 个 SDL3 测试无回归。