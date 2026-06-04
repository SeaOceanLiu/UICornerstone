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

### 2026-06-05: Phase 6 — MainWindow Cleanup + InputBackend Standalone + SDL-free MainWindow.h (Partial)

**Note**: Phase 6 in the design doc covers Window abstraction, InputBackend, BackendPlugin, Event system, and AppCallbacks. BackendPlugin was previously completed; this session focused on the remaining Window/InputBackend cleanup.

**Changes**:
- **`src/InputBackend.cpp`** (new): Extracted `SDL3InputBackend` from `src/Window.cpp` into its own file; factory uses `window->nativeHandle()` instead of SDL-specific cast
- **`src/Window.cpp`**: Removed `SDL3InputBackend` class and `CreateSDL3InputBackend` factory (moved to InputBackend.cpp)
- **`include/MainWindow.h`**: Removed `#include <SDL3/SDL.h>`, `SDL_Renderer* m_renderer`, `SDL_DisplayID m_displayId`, `getWindow()` (SDL_Window*), `getRenderer()` (SDL_Renderer*); replaced direct SDL calls with `Window` abstract methods; replaced `handleWindowEvent(SDL_WindowEvent&)` with `onWindowResized(int,int)` / `onWindowMoved(int,int)`; renamed `getWindowObject()` → `getWindow()`; now SDL-free
- **`include/Bench.h`**: Replaced `SDL_AppResult` with `int` (removed SDL type from header)
- **`src/Bench.cpp`**: `SDL_APP_CONTINUE` → `0`; `SDL_AppResult` → `int`
- **`src/EditBox.cpp`**: Added explicit `#include <SDL3/SDL_keyboard.h>` and `<SDL3/SDL_keycode.h>` (lost transitive include from MainWindow.h)
- **`src/TextArea.cpp`**: Same SDL keyboard include fix
- **`src/Actor.cpp`**, **`src/CheckBox.cpp`**, **`src/ControlBase.cpp`**, **`src/Label.cpp`**, **`src/LayoutParser.cpp`**, **`src/ProgressBar.cpp`**: Added explicit `#include <SDL3/SDL.h>` (lost transitive include)
- **All 10 test files**: Replaced `MAINWIN->handleWindowEvent(event->window)` with `MAINWIN->onWindowResized/onWindowMoved(data1, data2)`
- **`test/test_editbox.cpp`**: Replaced `SDL_StartTextInput(MAINWIN->getWindow())` with `GET_INPUTBACKEND->startTextInput()`
- **`test/test_label.cpp`**: Removed stale raw TTF test code (`g_font1`, `g_textEngine`, `g_text1` — leftover from pre-TextRenderer era)
- **`CMakeLists.txt`**: Added `src/InputBackend.cpp`

**Result**: `MainWindow.h` is now SDL-free (no SDL types in public API). InputBackend has its own source file. All SDL dependencies are confined to backend implementation files.

**Remaining Phase 6 work**: Event system union migration (std::any → EventType::union), AppCallbacks abstraction, MainWindow::run() loop.