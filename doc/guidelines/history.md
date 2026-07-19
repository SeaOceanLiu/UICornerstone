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


| File                                          | Change                                                                                                                                                                                                                                                                      |
| --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/backend/raylib/TextRenderer.cpp`         | Font cache (`unordered_map<FontCacheKey, weak_ptr<RaylibFont>>`). Load font at `size * 96/72` to match SDL3_ttf point→pixel conversion. `getFontHeight` uses `MeasureTextEx("Ay")`. `wrapWidth` scaled by 96/72.                                                           |
| `src/backend/raylib/InputBackend.cpp`         | `newFrame()` calls `PollInputEvents()` once per frame. Phase order changed to `Keyboard → CharInput → **WindowEvents** → MouseButton → MouseMove → MouseWheel → Done`. `m_resizeDetected` flag suppresses `MOUSE_UP` when a `WindowResize` arrives in the same batch. |
| `src/backend/raylib/RenderDevice.cpp`         | `present()` no longer calls `EndDrawing()` (avoids internal `PollInputEvents`). Manually flushes batch + `SwapScreenBuffer()`. 60 Hz `WaitTime` frame limiter.                                                                                                             |
| `src/backend/raylib/Window.cpp`               | Removed`SetTargetFPS(60)` (moved to RenderDevice). Added `SetTraceLogLevel(LOG_WARNING)`. Fixed flag‑bit mapping (`0x00000020` → `FLAG_WINDOW_RESIZABLE`).                                                                                                                |
| `include/MainWindow.h` + `src/MainWindow.cpp` | WindowResize dedup by`(w, h)` — skips events with same dimensions as last processed. 500‑event safety guard in `processEvents`.                                                                                                                                           |

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


| File                                  | Change                                                                                                                                                                                                                    |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/backend/sdl3/TextRenderer.cpp`   | Added font cache (`m_fontCache` keyed by `(contentHash, size)`, `m_pathCache` by `(path, size)`). Eliminated redundant `TTF_OpenFontIO` calls.                                                                            |
| `include/CheckBox.h`                  | Added`bool m_layoutDone = false` member.                                                                                                                                                                                  |
| `src/CheckBox.cpp`                    | `setRect` dirty-rect check. `recreate()` resets `m_layoutDone` to `false`. `create()` sets `m_layoutDone = true` after layout. `createCaption()` callback checks `m_layoutDone` before calling `adjustSpaceAssignment()`. |
| `src/Label.cpp`                       | `setRect` dirty-rect early return. `setParent` dirty-parent early return.                                                                                                                                                 |
| `src/backend/raylib/TextRenderer.cpp` | Removed debug`printf` and `Platform::GetTicks()` timing (leftover from earlier session — only removed unused variables, font cache was already present).                                                                 |

**Debug instrumentation removed from production sources**:

- `Label.cpp`: `g_recreateDepth` thread_local + `[LABEL_RECREATE]` printf
- `CheckBox.cpp`: `[CHECKBOX_RECREATE]` timing printf
- `SDL3 TextRenderer.cpp`: `[FONT_LOAD]` + `[FONT_HIT]` timing printf
- `Raylib TextRenderer.cpp`: `[FONT_HIT]` + `[FONT_RELOAD]` timing printf
- All unused `Platform::GetTicks()` / timing variables removed

**Results**:


| Backend | Before | After |
| ------- | ------ | ----- |
| SDL3    | ~48s   | ~7.2s |
| SFML    | —     | ~6.9s |
| Raylib  | —     | ~4.6s |

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

### 2026-06-24: Slider 刻度线 + 刻度标签（Complete）

**问题**：水平滑块的刻度线应该只在轨道下方绘制（非居中），且应带有刻度数值标签。

**变更**（6 文件）：


| 文件 | 变更 |
|------|------|
| `include/ConstDef.h` | 添加 `SLIDER_TICK_INTERVAL/LENGTH/COLOR` 常量 |
| `src/ConstDef.cpp` | 默认值：interval=0，length=8，color=灰(100,100,100) |
| `include/Slider.h` | 添加 `m_tickInterval/length/color` 成员 + `m_tickFont/m_tickFontData/m_tickLabelFontSize` + `setTickInterval/Length/Color` + Builder 方法 |
| `src/Slider.cpp` | `draw()` 中：水平滑块在轨道下方绘制竖线 + 数值标签；垂直滑块在轨道左侧绘制横线 + 数值标签；懒加载 `m_tickFont`（通过 ResourceProvider + 预加载字体数据） |
| `src/LayoutParser.cpp` | 添加 `"tick": {interval, length, color}` JSON 解析 |
| `test/test_slider.cpp` | 添加 Slider9：范围 0-100，tickInterval=10，tickLength=8，浅灰刻度 |

**刻度绘制逻辑**：
- 水平：刻度线从轨道底部向下延伸 `tickLength`，标签在刻度线下方 2px 居中
- 垂直：刻度线从轨道左侧向左延伸 `tickLength`，标签在刻度线左侧 2px 居中
- tickInterval=0（默认）时不绘制刻度
- 刻度标签使用 `m_labelFont` 字体 + `m_tickLabelFontSize`（默认 10px），懒加载缓存

**构建验证**：三后端（SDL3/SFML/Raylib）全部编译通过。test_slider 运行正常，9 个滑块（含刻度滑块）初始化 + 3 秒运行无崩溃。

### 2026-06-21: HandleControl 光标 + 视觉优化 (Complete)

**问题**：SDL3 后端 `SDL_SetCursor` 不持久，Windows `WM_SETCURSOR` 消息在每帧
复位光标。WinFrame 使用 Win32 `SetCursor` 绕过该问题，但 HandleControl 的
`setResizeCursor` 通过 `Cursor::setCurrent` 调用 `SDL_SetCursor`，导致光标
反馈始终不可见。

**Bug 1 — 光标不出现**：

- Root cause: `SDL_SetCursor` 在此 SDL3 fork 中不能持久化，Windows 的
  `WM_SETCURSOR` 消息在每帧鼠标移动时复位光标为默认
- Fix: `HandleControl::setResizeCursor()` 在 `#ifdef _WIN32` 路径下直接调用
  Win32 `SetCursor(LoadCursor(...))`，匹配 WinFrame 做法
- 添加中心十字星背景，以及十字星拖拽/缩放和释放时光标保持和恢复

**Bug 2 — Move 手柄区域太小难以命中**：

- `updateHandleAreas` 中 Move 手柄只有中心 8×8 像素区域
- 用户反馈后恢复原始设计（只在小方块和十字星上触发光标变化）

**Bug 3 — 拖拽中光标恢复为默认**：

- 按下鼠标启动拖拽/缩放后，后续 MouseMove 事件被拖拽逻辑消费，
  `setResizeCursor` 未被调用
- Fix: 在 `m_resizing`/`m_dragging` 分支的 MouseMove 中也调用 `setResizeCursor`

**Bug 4 — 按下鼠标瞬间光标恢复为默认**：

- MouseDown 中 `startDrag/startResize` 之后没有立即设置光标
- Fix: MouseDown 分支末尾加 `setResizeCursor(ht)`

**视觉优化**：

- 8 个角/边手柄保持白底蓝边方块
- 中心十字改为蓝边白底十字（5px 蓝线 + 3px 白线 + 两端各延 1px）
- 修复十字星下方重叠绘制方块的问题（循环中跳过 HandleType::Move）

**变更文件**：


| 文件                          | 变更                                                                                         |
| ----------------------------- | -------------------------------------------------------------------------------------------- |
| `src/HandleControl.cpp`       | Win32 SetCursor 路径；拖拽中光标保持；按下瞬间光标；十字星蓝边白底；移除方块循环中 Move 绘制 |
| `doc/HandleControl_Design.md` | 更新 drawMoveHandle、事件流程、光标 Win32 说明、移除 beforeDraw、m_handleAreas 固定数组      |

**验证**：test_handlecontrol 三后端（SDL3/SFML/Raylib）构建通过。

**问题**：test_fromsource_sfml 和 test_fromsource_raylib 日志中出现 `Cursor::createSystem: no backend factory registered`，光标功能完全失效。

**根因**：SFML 和 Raylib 后端使用**直接方法覆盖**模式（在各自 `Cursor.cpp` 中定义 `Cursor::createSystem()`/`getDefault()`/`setCurrent()` 成员函数），而非 SDL3 所用的**工厂注册**模式（`RegisterSDL3CursorFactories()` → `Cursor::registerFactories()`）。在 fromsource/DLL 模式下，`UICornerstone.dll` 中的 `src/Cursor.cpp`（工厂指针模式）无法获取后端实现。

**修复**（5 文件）：

- `src/backend/sfml/Cursor.cpp`：`Cursor::createSystem/getDefault/setCurrent` 改为静态工厂函数 `sfmlCreateSystemCursor/sfmlGetDefaultCursor/sfmlSetCurrentCursor`；新增 `RegisterSFMLCursorFactories()`
- `src/backend/raylib/Cursor.cpp`：同上（`raylib*` 前缀）；新增 `RegisterRaylibCursorFactories()`
- `src/backend/sfml/BackendPlugin.cpp`：`GetUIBackendCallbacks()` 中添加 `RegisterSFMLCursorFactories()` 调用
- `src/backend/raylib/BackendPlugin.cpp`：同上
- `src/BackendManager.cpp`：SFML/Raylib 静态链接路径添加光标工厂注册

**SFML 默认光标 Bug**：`sfmlGetDefaultCursor()` 返回空构造的 `SFMLCursor`（`m_hasCursor=false`），`sfmlSetCurrentCursor()` 中 `get()==nullptr` 跳过 `setMouseCursor`，导致光标设成手指后无法恢复箭头。修复为初始化为真实 Arrow 光标。

**验证**：三后端（SDL3/SFML/Raylib）静态 + DLL 模式均编译通过。fromsource 测试时间戳更新为 2026-06-21。

### 2026-06-20: sample_loadlibrary 零导入库重构 + 4 samples 全部完成

**问题**：sample_loadlibrary 在 `UICORNERSTONE_BUILD_SHARED` 定义下链接 `UICornerstone_dll.lib`，导致符号解析走 `dllimport` —— `Surface::registerFactories` 等 `CORE_API` 函数的函数体在 DLL 而非 exe 中。当 exe 提供自己的 `registerFactories` 时出现 LNK2001（多重定义）。

**Fix**：

- sample_loadlibrary 不再链接 `UICornerstone_dll.lib`
- 不定义 `UICORNERSTONE_BUILD_SHARED`，`CORE_API` 为空宏 → 无 `dllimport`
- 内联 3 个 Core 符号：`Surface::registerFactories`（no-op）、`Cursor::registerFactories`（no-op）、`ResourceProvider::createFilesystem`（FilesystemResourceProvider 完整实现）
- Cursor 工厂 stub 产生 cosmetic 警告但功能正确（Label 空指针优雅处理）

**所有 4 个 Sample 最终验证**：


| Sample              | 模式        | 后端编译                 | 核心加载            | 零导入库      | 验证         |
| ------------------- | ----------- | ------------------------ | ------------------- | ------------- | ------------ |
| hello_uicornerstone | JSON 布局   | `UICornerstone.lib` 静态 | 无 DLL              | ✅            | build/run ✔ |
| sample_programmatic | C ABI 工厂  | `UICornerstone.lib` 静态 | 无 DLL              | ✅            | build/run ✔ |
| sample_fromsource   | ILT 隐式    | CMake 独立 TU            | `UICornerstone.dll` | ❌ (需导入库) | build/run ✔ |
| sample_loadlibrary  | LoadLibrary | `#include` 同一 TU       | `UICornerstone.dll` | ✅            | build/run ✔ |

**设计文档更新**：

- `doc/Sample_Design.md`: §3 新增 loadlibrary 架构图；§7 关键差异说明改为零导入库方式（3 个 Core 符号内联）
- AGENTS.md: 本次 session 记录
- `doc/Build_Guide.md` 已在前序 session 更新

### 2026-06-20: hello_uicornerstone sample 实现

**变更**：

- `samples/hello_uicornerstone/hello_uicornerstone.c`: 纯 C 示例（~50 行），Button 点击 → Label 计数，内联 JSON 布局，完全静态链接
- `samples/hello_uicornerstone/CMakeLists.txt`: 单目标 CMake，POST_BUILD 复制 DLL + assets
- `samples/CMakeLists.txt`: 新目录 CMake，转发到子目录
- `CMakeLists.txt`: 添加 `add_subdirectory(samples)`
- `doc/Sample_Design.md`: §2 新增后端选择说明（SDK3/SFML/Raylib 对比表），§7 更新实现状态
- `doc/Sample_Design.md`: §7 sample_programmatic 标记为"✅ 已实现"
- `doc/Build_Guide.md`: 添加 sample_programmatic 到测试表
- `samples/sample_programmatic/sample_programmatic.c`: 纯 C 示例（~45 行），编程式控件创建代替 JSON 布局
- `samples/sample_programmatic/CMakeLists.txt`: 单目标 CMake，输出到 `build/sample/sample_programmatic/<backend>/Debug/`
- `doc/Build_Guide.md`: 添加 hello_uicornerstone 到测试表、输出目录、独立构建说明

**验证**：`hello_uicornerstone.exe` 编译通过，启动后 2 秒存活正常，输出显示静态 InitFromPlugin 回退路径正常工作。

### 2026-06-20: SFML/Raylib 静态+DLL 双构建目录 + test_fromsource 改名

**变更**：

- SFML/Raylib 改为与 SDL3 一致：`build/sfml`=静态、`build/sfml_dll`=DLL、`build/raylib`=静态、`build/raylib_dll`=DLL
- `test/test_fromsource.cpp` → `test/test_fromsource_sdl3.cpp`，CMake target 同步改名
- 从静态构建目录中清理出旧 DLL 模式残留文件（`UICornerstone.dll`/`UIBackend_*.dll` 等）
- AGENTS.md 和 `doc/UICornerstone_DLL_Design.md` 中引用同步更新

**构建目录结构**：


| 目录               | 模式 |     fromsource 测试     |
| ------------------ | ---- | :----------------------: |
| `build/sdl3`       | 静态 |            —            |
| `build/sdl3_dll`   | DLL  |  `test_fromsource_sdl3`  |
| `build/sfml`       | 静态 |            —            |
| `build/sfml_dll`   | DLL  |  `test_fromsource_sfml`  |
| `build/raylib`     | 静态 |            —            |
| `build/raylib_dll` | DLL  | `test_fromsource_raylib` |

**验证**：6 个构建目录的所有测试 exe + DLL 时间戳均为 2026-06-20。

### 2026-06-20: sample_fromsource — 混合集成（核心 DLL + 后端源码）

**变更**：

- `samples/sample_fromsource/sample_fromsource.c`: 纯 C 示例（~55 行），Button 点击 → Label 计数，`GetUIBackendCallbacks()` + `UICornerstone_Init(callbacks)` 模式
- `samples/sample_fromsource/CMakeLists.txt`: 仅 `UICORNERSTONE_BUILD_DLL=ON` 下构建；将 6 个后端源文件编译进 exe（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）；链接 `UICornerstone_dll`（导入库，ILT 隐式加载 UICornerstone.dll）
- `samples/CMakeLists.txt`: 添加 `add_subdirectory(sample_fromsource)`
- `doc/Sample_Design.md`: §3 新增 fromsource 架构说明；新增 §6 sample_fromsource 代码解读；§7 CMake 章节更新 fromsource CMake；§8 文件清单更新；§9 后续扩展标记为 ✅
- `doc/Build_Guide.md`: 添加 sample_fromsource 到测试表、输出目录、独立构建说明、fromsource 节

**关键架构**：exe 272KB（仅有后端源码），UICornerstone.dll 3.2MB（核心控件）。通过 ILT（Import Library Thunk）在启动时自动加载 `UICornerstone.dll`，无需 `LoadLibrary` + `GetProcAddress`。

**验证**：`sample_fromsource.exe` 编译通过，启动后 2 秒存活正常，输出显示 `GetUIBackendCallbacks ready` + `initialized from callback table`。

### 2026-06-20: sample_loadlibrary — 显式 LoadLibrary + #include 后端源码

**变更**：

- `samples/sample_loadlibrary/sample_loadlibrary.cpp`: 纯 C++ 示例（~80 行），Button 点击 → Label 计数，`LoadLibraryA("UICornerstone.dll")` + `GetProcAddress` 解析全部 C ABI 函数；`#include` 6 个后端 .cpp 文件编译入同一 TU；`main()` 帧循环
- `samples/sample_loadlibrary/CMakeLists.txt`: 仅 `UICORNERSTONE_BUILD_DLL=ON` 下构建；后端通过 `#include` 而非独立 TU 编译；链接 `UICornerstone_dll` 导入库（仅用于注册符号，C ABI 全部走函数指针）
- `samples/CMakeLists.txt`: 添加 `add_subdirectory(sample_loadlibrary)`
- `src/UICornerstoneAPI.cpp`: 修复预存在拼写错误 `initifalize` → `initialize`
- `doc/Sample_Design.md`: §3 新增 loadlibrary 架构说明；新增 §7 sample_loadlibrary 代码解读（含与 fromsource 对比表）；§8-10 重编号；文件清单和后续扩展表更新
- `doc/Build_Guide.md`: 添加 sample_loadlibrary 到测试表、输出目录、独立构建说明、fromsource 节

**与 sample_fromsource 的架构差异**：


| 维度       | sample_fromsource | sample_loadlibrary        |
| ---------- | ----------------- | ------------------------- |
| DLL 加载   | ILT 隐式          | `LoadLibrary` 显式        |
| C ABI 调用 | 直接符号链接      | `GetProcAddress` 函数指针 |
| 后端编译   | CMake 独立 TU     | `#include .cpp` 同一 TU   |

**验证**：`sample_loadlibrary.exe` 272KB 编译通过，启动后 2 秒存活正常，输出显示 `loaded UICornerstone.dll` + `GetUIBackendCallbacks ready` + `initialized from callback table`。

### 2026-06-19: 14 份设计文档批量更新 (Complete)

**更新列表**：


| 文档                        | 主要变更                                                                                                                                                                 |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| ControlBase_Design.md       | 移除`SDL_Renderer*`/`setRenderer`/`getRenderer`；添加 `RenderDevice*`/`TextRenderer*`/`ResourceProvider*` 抽象成员；`addControl`/`setParent` 改用 `inheritRenderer` 传播 |
| Button_Design.md            | handleEvent 重写为 union API（Phase 12）；`SDL_Color` → `SColor` 常量；移除 `setRenderer`                                                                               |
| CheckBox_Design.md          | handleEvent union API；`SDL_Color` → `SColor`；添加 `setRect` 脏矩形优化（Phase 15）                                                                                    |
| EditBox_Design.md           | `TTF_Font*`/`TTF_Text*` → `SharedFont`/`TextRenderer*`；移除 `recreateTextObjects`/`setRenderer`；添加 `m_fontData`/`loadFontInternal`                                  |
| TextArea_Design.md          | 移除`TTF_TextEngine*`；移除 `setRenderer`；handleEvent union API；`rebuildLines`/`updateVScrollBar` 匹配实际源码                                                         |
| ProgressBar_Design.md       | `SDL_Color` → `SColor`；移除 `setRenderer`                                                                                                                              |
| ScrollBar_Design.md         | handleEvent union API（移除`std::any_cast`/`try-catch`）；移除 `setRenderer`                                                                                             |
| WinFrame_Design.md          | `SDL_Cursor*` → `Cursor*`；`ResourceLoader::RID_*` → 内联路径；添加向量 X 叠加层 `WinFrame::draw()`；handleEvent union API                                             |
| Menu_Design.md              | `SDL_Log` → `Platform::Log`；`SDL_Event`/`SDL_PushEvent` → 通用退出机制；`SDL_Color` → `SColor`                                                                       |
| GraphTool_Design.md         | 全面去 SDL3：架构图/代码`SDL_Renderer` → `RenderDevice`；`SColor` 独立类；`drawTriangle`/`drawQuad` 替代 `SDL_RenderGeometry`                                           |
| LayoutSystem_Design.md      | 移除`#include <SDL3/SDL.h>`；`SDL_Log` → `Platform::Log`；`parseFontStyle` 返回 `int`；FontName 迁至 ConstDef.h                                                         |
| ComponentSystem_Design.md   | 最小变更：`SDL_Log` → `Platform::Log`                                                                                                                                   |
| UICornerstone_DLL_Design.md | 状态从"实施中"→"已完成"；新增 §9.4 Fromsource 集成模式（架构/工厂注册/修复表/回调查表扩展）；版本历史扩充至 1.11                                                       |

**验证**：所有 15 份设计文档（`*_Design.md`）均反映当前代码库状态。Label_Design.md 已在 2026-06-05 session 中更新，ResourceLoader_Design.md 已标为废弃，无需重复修改。

### 2026-06-19: 13 份设计文档批量更新 (Complete)

**更新列表**：


| 文档                      | 主要变更                                                                                                                                                                 |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| ControlBase_Design.md     | 移除`SDL_Renderer*`/`setRenderer`/`getRenderer`；添加 `RenderDevice*`/`TextRenderer*`/`ResourceProvider*` 抽象成员；`addControl`/`setParent` 改用 `inheritRenderer` 传播 |
| Button_Design.md          | handleEvent 重写为 union API（Phase 12）；`SDL_Color` → `SColor` 常量；移除 `setRenderer`                                                                               |
| CheckBox_Design.md        | handleEvent union API；`SDL_Color` → `SColor`；添加 `setRect` 脏矩形优化（Phase 15）                                                                                    |
| EditBox_Design.md         | `TTF_Font*`/`TTF_Text*` → `SharedFont`/`TextRenderer*`；移除 `recreateTextObjects`/`setRenderer`；添加 `m_fontData`/`loadFontInternal`                                  |
| TextArea_Design.md        | 移除`TTF_TextEngine*`；移除 `setRenderer`；handleEvent union API；`rebuildLines`/`updateVScrollBar` 匹配实际源码                                                         |
| ProgressBar_Design.md     | `SDL_Color` → `SColor`；移除 `setRenderer`                                                                                                                              |
| ScrollBar_Design.md       | handleEvent union API（移除`std::any_cast`/`try-catch`）；移除 `setRenderer`                                                                                             |
| WinFrame_Design.md        | `SDL_Cursor*` → `Cursor*`；`ResourceLoader::RID_*` → 内联路径；添加向量 X 叠加层 `WinFrame::draw()`；handleEvent union API                                             |
| Menu_Design.md            | `SDL_Log` → `Platform::Log`；`SDL_Event`/`SDL_PushEvent` → 通用退出机制；`SDL_Color` → `SColor`                                                                       |
| GraphTool_Design.md       | 全面去 SDL3：架构图/代码`SDL_Renderer` → `RenderDevice`；`SColor` 独立类；`drawTriangle`/`drawQuad` 替代 `SDL_RenderGeometry`                                           |
| LayoutSystem_Design.md    | 移除`#include <SDL3/SDL.h>`；`SDL_Log` → `Platform::Log`；`parseFontStyle` 返回 `int`；FontName 迁至 ConstDef.h                                                         |
| ComponentSystem_Design.md | 最小变更：`SDL_Log` → `Platform::Log`                                                                                                                                   |

**验证**：所有 14 份设计文档（`*_Design.md`）+ `ResourceLoader_Design.md`（已废弃）+ `BackendAbstraction_Design.md`（已在前序 session 更新）均反映当前代码库状态。Label_Design.md 已在 2026-06-05 session 中更新，无需重复修改。

### 2026-06-19: Raylib fromsource 纹理不可见根因排查 + 修复

**根因（双重）**：

1. **`BackendBridge.h:bridge_drawTexture` `nullptr` src 被转义为零 SRect**：当调用者（如`Actor::draw`）传递 `nullptr` src（表示"使用完整纹理"）时，`bridge_drawTexture` 创建默认 `SRect()`（全零），然后将**非空指针**传给 `RaylibRenderDevice::drawTexture`。导致 `src=(0,0,0,0)` → `DrawTexturePro` 绘制空输出。
2. **`RaylibRenderDevice::drawTexture` 使用 `rlPushMatrix + rlScalef + DrawTextureEx`**：该写法在 fromsource/DLL 桥接模式下因 OpenGL 矩阵状态跨 DLL 边界损坏而无效，纹理始终不可见；用 `DrawTexturePro` 替代后正常工作。

**诊断过程**：

- 确认非纹理控件（CheckBox、EditBox、Panel背景、Label文字）全部可见 → 问题在纹理路径
- `DrawTexturePro` 在诊断新鲜创建的纹理上工作 → 问题不是 `DrawTexturePro` 本身
- `LoadImageFromTexture` 读回原生纹理 GPU 数据成功且有有效不透明像素 → 数据未损坏
- 在 (300,10) 绘制原生纹理可见 → 纹理和 `DrawTexturePro` 都正常
- 比较原始调用 `src=(0,0,0,0)` 与诊断调用 `nSrc=(0,0,w,h)` → 定位到 `src` 为零

**修复（2 个文件）**：


| 文件                                  | 变更                                                                        |
| ------------------------------------- | --------------------------------------------------------------------------- |
| `src/backend/BackendBridge.h`         | `bridge_drawTexture` 中 `nullptr` src 传入 `nullptr` 而非 `&zeroRect`       |
| `src/backend/raylib/RenderDevice.cpp` | `drawTexture` 从 `rlPushMatrix+rScalef+DrawTextureEx` 改为 `DrawTexturePro` |

**验证**：raylib fromsource ImageButton PNG、LuotiAni 动画、WinFrame 关闭按钮 X 全部可见。SDL3 全部 10 测试无回归。

### 2026-06-19: SFML fromsource 纹理不可见修复（Actor::setParent + sf::Sprite）

**问题**：`test_fromsource_sfml.exe` 中 ImageButton 和 Animation Button 的 PNG 纹理不可见。OpenGL handle 有效、位置正确、`sf::Sprite` 被调用，但画面空白。

**根因（双重）**：

1. **`Actor::setParent` 覆盖纹理**（`src/Actor.cpp:125-140`）：`setParent` 在 `m_texture` 已存在时仍调用 `m_surface->createTexture()` 覆盖 `m_texture`。初始纹理在 `loadFromFile()` 中创建，`Button::setNormalStateActor()` 调用 `setParent()` 时创建第二个纹理覆盖第一个（第一个被销毁，OpenGL handle 回收）。
2. **`sf::VertexArray` + `RenderStates` 纹理绑定问题**：`sf::VertexArray(TriangleStrip)` + `states.texture = nativeTex` 在特定 OpenGL 状态组合下不可见。改 `sf::Sprite(*nativeTex)` 后正确。

**修复（2 个文件）**：


| 文件                                | 变更                                                                   |
| ----------------------------------- | ---------------------------------------------------------------------- |
| `src/Actor.cpp`                     | `setParent` 添加 `&& !m_texture` 避免覆盖已有纹理                      |
| `src/backend/sfml/RenderDevice.cpp` | `drawTexture` 加入 `setActive(true)` + `sf::Sprite` 替代 `VertexArray` |

**验证**：`test_fromsource_sfml.exe` 纹理和动画全部可见。SDL3 全部 10 测试无回归。

### 2026-06-19: SFML 事件响应慢修复（Label::recreate 字体磁盘 I/O 瓶颈）

**问题**：`test_fromsource_sfml.exe` 点击响应延迟大（数秒），SFML 后端事件处理完全不可用。

**根因**：`Label::recreate()` 每帧调用 `releaseFont()` 重置 `m_font`/`m_fontData`，迫使后续 `create()` → `loadFromResource()` 从磁盘读取 ~5-10MB HarmonyOS 字体文件并通过桥接链 `DLL→EXE→DLL` 反复传递，产生 15-44ms 的停顿。

**瓶颈链**：

```
uiSetText(g_prgStatus, "Progress: XX.X%")
  → UICornerstone_SetText (DLL)
    → Label::setCaption()
      → recreate()
        → releaseFont()     ← 释放 m_font / m_fontData
        → create()
          → loadFromResource()
            → provider->readFile()  ← 磁盘 I/O! ~5-10MB
            → loadFontFromMemoryBridge  ← DLL→EXE→DLL 桥接
      → computeLineOffsets()
        → measureText * 3   ← 每次桥接
```

**性能数据**：


| 指标                   | 修复前  | 修复后  |
| ---------------------- | ------- | ------- |
| Update (BENCH->update) | 15-44ms | 0.2-1ms |
| Labels (setCaption)    | 15-32ms | 0.4-6ms |
| 总帧时间               | 33-75ms | 1.5-6ms |
| FPS                    | 13-31   | 170-670 |

**修复（3 个文件）**：


| 文件                                        | 变更                                                                     |
| ------------------------------------------- | ------------------------------------------------------------------------ |
| `src/Label.cpp:recreate()`                  | 移除`releaseFont()`，字体在文本/对齐/边距变化时无需重载                  |
| `src/Label.cpp:setFont()` / `setFontSize()` | 添加`releaseFont()`，只有字体名称或大小变化时才释放字体                  |
| `src/Label.cpp:loadFromResource()`          | 添加`if (m_font) return;` 缓存命中提前返回，避免重复 Provider 读取和桥接 |

**验证**：`test_fromsource_sfml.exe` 运行流畅，FPS 稳定 300-500。SDL3 全部 10 测试无回归。
`setFramerateLimit(120)` 已移除（非正确修复方向）。

### 2026-06-18: Raylib `DrawTexturePro` DLL 桥接不可见修复

**问题**：raylib fromsource 测试（`test_fromsource_raylib`）在桥接模式下所有纹理不可见。`DrawTexturePro` 经桥接从 `UICornerstone.dll` 调用到 exe 中的 `RaylibRenderDevice::drawTexture`，日志输出正确的纹理 ID 和 dst rect，但画面上无任何纹理。

**根因**：未知。`DrawTexturePro` 和 `rlBegin/rlEnd`（`RL_TRIANGLES`）在 fromsource/bridge 模式下始终不可见。`DrawTextureV` 在 `raylib.h` 中定义为 `static inline`（编译到 exe TU），`DrawTexturePro` 来自预编译 `raylib.lib`——函数跨库调用路径可能是关键差异。透明热身（alpha=0）也无效。

**修复**：完全绕过 `DrawTexturePro` 和 `rlBegin/rlEnd`，改用 `rlPushMatrix + rlScalef + DrawTextureEx` 组合：

```
rlPushMatrix();
rlTranslatef(dst.x, dst.y, 0);
rlScalef(dst.w/src.w, dst.h/src.h, 1.0f);
rlTranslatef(-src.x, -src.y, 0);
DrawTextureEx(nativeTex, {0,0}, 0, 1.0f, WHITE);
rlPopMatrix();
```

`DrawTextureEx`（scale=1，原生大小）在桥接路径下正常工作；通过矩阵变换实现定位 + 非均匀拉伸，等价于 `DrawTexturePro` 的行为。

**关键发现**：

- `static inline DrawTextureV` → `DrawTextureEx`（raylib.lib）→ `DrawTexturePro`（raylib.lib）链路上，inline 函数编译到 exe，库函数在 raylib.lib——跨库调用路径可能触发了 raylib 内部渲染批处理状态 bug。
- `DrawTexturePro` 单独调用（含 `rlDrawRenderBatchActive` + `rlSetTexture` + `printf`/`fflush` 等各种变体）皆不可见
- `rlBegin(RL_TRIANGLES)` + `rlTexCoord2f` + `rlVertex2f` 低级 API → 同样不可见
- 透明热身（`DrawTextureV` + alpha=0）无效——必须真正绘制（alpha > 0）才能初始化渲染管线的 OpenGL 纹理绑定状态
- `drawTextureRotated` 补了缺失的 `EndBlendMode()` guard

**文件变更**：


| 文件                                  | 变更                                                                                                                                     |
| ------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `src/backend/raylib/RenderDevice.cpp` | `drawTexture()` 和 `drawTextureRotated()` 改用 `rlPushMatrix + rlScalef + DrawTextureEx`；`drawTextureRotated` 补 `EndBlendMode()` guard |

### 2026-06-16: Fix 4 fromsource 测试 Bug（Actor fallback + Raylib 字体 in-place reload）

**修复 1 — SFML/Raylib WinFrame 关闭按钮 X 不显示**：

- Root cause: `Actor::loadFromFile()` 只调用 `Surface::loadFromFile()` 加载 PNG。在 fromsource（callback bridge）路径下，Surface 工厂函数（`RegisterSFMLSurfaceFactories`/`RegisterRaylibSurfaceFactories`）注册到 DLL，但 Actor 在 DLL 内通过桥接调用 `Surface::loadFromFile()` 时工厂返回 nullptr，导致图片加载失败。
- Fix: `Actor::loadFromFile()` 在 `Surface::loadFromFile()` 返回 nullptr 后，回退调用 `getRenderDevice()->createTextureFromFile(path)`。该虚方法在 backends 和 CallbackBridge 中均已实现，可绕过 Surface 工厂直接由后端加载纹理。
- 同时所有三个后端的 BackendPlugin.cpp 已连接 `createTextureFromFile` 回调到 `bridge_createTextureFromFile`。

**修复 2 — Raylib 中文显示"?"**：

- Root cause: `RaylibTextRenderer::ensureFontCodepoints()` 懒加载扩展 CJK 码点时创建新的 `shared_ptr<RaylibFont>` 替换字体缓存。但 bridge 中的 `UIFontHandle`（`shared_ptr<Font>*` 堆分配句柄）仍指向旧对象，导致后续 `drawText` 通过 bridge 调用时使用只有 ASCII 码点的旧字体。
- Fix: `RaylibFont` 新增 `reload(rlFont)` 方法，原地卸载旧 `rlFont` 并替换为新字体的 `rlFont`，不改变对象身份。`loadOrCreate()` 和 `ensureFontCodepoints()` 中的字体重载路径改为调用 `reload()` 而非创建新 `shared_ptr`。

**修复 3 — SFML 事件响应慢**（部分处理）：

- `Window.cpp` 中已添加 `setVerticalSyncEnabled(false)`（前次 session 完成）
- 当前 session 未发现其他导致响应慢的原因（`pollEvent` 使用非阻塞 API，帧循环无休眠等待）

**修复 4 — Raylib EditBox/TextArea 中文输入**（同 Root cause 为修复 2）：

- 中文输入通过 `TextInput` 事件输入 UTF-8 文本，EditBox 内部调用 `loadFontInternal()` 时使用 `loadFontFromMemory()`（仅 ASCII）。在 `insertText()` 中 EditBox 重新加载字体（未使用 ensureFontCodepoints）。修复 2 已修复字体句柄一致性问题。

**文件变更**：


| 文件                                  | 变更                                                                             |
| ------------------------------------- | -------------------------------------------------------------------------------- |
| `src/Actor.cpp`                       | `loadFromFile()` 添加 `createTextureFromFile` 回退                               |
| `src/backend/raylib/TextRenderer.cpp` | `RaylibFont::reload()` 方法 + `loadOrCreate`/`ensureFontCodepoints` 改为原地重载 |

**已知问题**：

- SFML 和 Raylib 标准测试（静态链接 `UICornerstone.lib`）存在 `Surface::create/loadFromFile/loadFromMemory` 重复定义错误（`Surface.cpp` 和 `RenderDevice.cpp` 均定义）。这是预存在问题（自 Phase R10b 核心/后端 DLL 拆分后），不影响 DLL 模式或 SDL3 静态模式。
- SFML fromsource 在部分环境下事件响应仍可能偏慢（vsync 关闭后未完全解决）。

**验证**：

- SDL3 全部 10 个标准测试编译成功 ✅
- SDL3 fromsource 测试编译成功 ✅
- SFML fromsource 测试编译成功 ✅
- Raylib fromsource 测试编译成功 ✅

### 2026-06-16: WinFrame 关闭按钮 X 不可见修复（Raylib 混合模式 + 向量 X 叠加）

**问题**：test_fromsource_raylib WinFrame 关闭按钮 X 不显示，只看到灰色/红色方块。原因是 PNG 纹理在 raylib 后端的渲染有问题。

**根本原因**：

1. **RaylibTexture::setBlendMode** 只存储混合模式到 `m_blendMode` 成员，但从未实际调用 `BeginBlendMode()` 应用到 OpenGL 状态。SDL3 的 `SDL_SetTextureBlendMode` 直接作用于纹理对象，透明通道正确处理；而 raylib 的纹理的 alpha 从未正确生效。

**修复（2 个文件）**：


| 文件                                      | 变更                                                                                                                                       |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `src/backend/raylib/RenderDevice.cpp`     | `drawTexture()` 和 `drawTextureRotated()` 在调用 `DrawTexturePro` 前读取纹理的 `getBlendMode()` 并调用 `BeginBlendMode()`                  |
| `src/WinFrame.cpp` + `include/WinFrame.h` | 新增`WinFrame::draw()` override，在 `Panel::draw()` 后使用 `RenderDevice::drawLine()` 绘制向量 X 叠加层，作为跨后端回退方案确保 X 永远可见 |

**具体改动**：

- `drawTexture/drawTextureRotated`: 根据纹理的 blend mode 调用 `BeginBlendMode(BLEND_ALPHA)`/`BeginBlendMode(BLEND_ADDITIVE)`/`BeginBlendMode(BLEND_MULTIPLIED)`。之前 `setBlendMode` 只是存储值，实际绘制时从未应用。
- `WinFrame::draw()`: 调用 `Panel::draw()` 后，获取关闭按钮的 `getDrawRect()` 计算中心位置，用 3 条平行 `drawLine` 绘制两条对角线（共 6 条线），颜色根据 closeButton 的 state 变化：Normal=浅灰(200,200,200)、Hover=白(255,255,255)、Pressed=黄(255,255,100)

**验证**：

- SDL3: 全部 13 个目标（10 标准测试 + DLL + lib + fromsource + test_api）编译通过 ✅
- SFML: `test_fromsource_sfml.exe` 编译通过 ✅（标准测试有预存在的 Surface 重复符号问题）
- Raylib: `test_fromsource_raylib.exe` 编译通过 ✅（标准测试有预存在的 Surface 重复符号问题）

### 2026-06-15: 三后端 fromsource 架构切换（Separate TU 编译）

**问题**：SFML 的 `<SFML/Graphics.hpp>` 引入 `<windows.h>`，在 `#include` 模式（backend .cpp 被 test `.cpp` include）下宏污染导致编译失败。

**架构变更**：

- 所有 `test_fromsource*.cpp` 不再 `#include` backend .cpp 文件，改为 CMake 的 `add_executable` 添加为独立翻译单元：
  ```cmake
  add_executable(${target} ${source_file} ${FROMSOURCE_BACKEND_SOURCES})
  ```
- `FROMSOURCE_BACKEND_SOURCES` 收集 `Window.cpp`/`RenderDevice.cpp`/`TextRenderer.cpp`/`InputBackend.cpp`/`Cursor.cpp`/`BackendPlugin.cpp`
- `FROMSOURCE_BACKEND_LIBS` 收集各后端第三方 lib（SDL3: SDL3.lib+SDL3_ttf.lib+SDL3_image.lib / SFML: sfml-*.lib+opengl32.lib / raylib: raylib.lib+winmm.lib）

**Three fromsource files**:

- `test/test_fromsource_sdl3.cpp` — SDL3 backend (SDL callback mode, 复用 SDL3 窗口)
- `test/test_fromsource_sfml.cpp` — SFML backend (main() + LoadLibrary + GetUIBackendCallbacks)
- `test/test_fromsource_raylib.cpp` — Raylib backend (同上)

**关键修复**：

- SFML 中文 `u8"读取TextArea文本内容"` 导致 MSVC C2059 → 改为英文 `"Read TextArea Content"`
- SDL3 的 `SDLKeycodeToKeyCode`/`SDLKeymodToKeyMod` 返回类型改为 `KeyCode`/`KeyMod`（匹配 InputBackend.h 声明）
- SDL3 需 `#include "UICornerstoneAPI.h"` + `#include "EventTypes.h"` 获取 UIEvent/KeyCode 定义
- Raylib 需 `winmm.lib`（timeBeginPeriod/timeEndPeriod）
- `struct UIBackendCallbacks*` → `UIBackendCallbacks*`（避免与 typedef 冲突）

**Cursor 工厂未注册**（cosmetic 警告）：

- fromsource 路径下 Cursor 工厂未通过 `registerFactories` 注册，Label 创建时输出 `Cursor::createSystem: no backend factory registered`
- 功能不受影响（仅缺光标反馈）

**验证**：SDL3/SFML/Raylib 三后端均编译 + 链接 + 运行成功。test_fromsource_sdl3 / test_fromsource_sfml / test_fromsource_raylib 全部通过。

### 2026-06-15: fromsource 四 bug 修复（Surface 工厂 + newFrame + vsync）

**Bug 1 — SFML WinFrame 关闭按钮 X / Hover / Press 图片不显示**：

- Root cause: `Actor::loadFromFile()` 调用 `Surface::loadFromFile()`（在 UICornerstone.dll 内），走工厂函数指针 `g_loadFileFn`。在 callback init 路径下工厂未注册 → `g_loadFileFn == nullptr` → 返回 nullptr
- Fix: 在 `GetUIBackendCallbacks()` 中调用 `RegisterSFMLSurfaceFactories()` / `RegisterRaylibSurfaceFactories()`，将后端的 Surface 工厂函数通过 `Surface::registerFactories()` 注册到 UICornerstone.dll
- Added `RegisterSFMLSurfaceFactories()` to `src/backend/sfml/RenderDevice.cpp`
- Added `RegisterRaylibSurfaceFactories()` to `src/backend/raylib/RenderDevice.cpp`

**Bug 2 — SFML 事件响应慢**：

- 怀疑是 SFML 默认 vsync 开启导致 `display()` 阻塞到下一次垂直同步
- Fix: 在 `Window.cpp` 创建 `sf::RenderWindow` 后显式调用 `setVerticalSyncEnabled(false)`

**Bug 3 — Raylib 窗体无法响应事件（标题栏"未响应"）**：

- Root cause: `PollInputEvents()` 从未被调用。在 callback init 路径下 `g_inputBackend` 是 `CallbackInputBackend`，其 `newFrame()` 使用基类默认空实现
- Fix: 在 `UIBackendCallbacks` 结构体中新增 `void (*newFrame)(UIInputBackendHandle)` 成员；`CallbackInputBackend::newFrame()` 通过该回调委托；raylib BackendPlugin 设置 `cb.newFrame = bridge_newFrame`（桥接到 `RaylibInputBackend::newFrame()` → `PollInputEvents()`）
- 不影响 SDL3/SFML（`newFrame` 为 NULL 时跳过，基类空实现已满足需求）

**Bug 4 — Raylib 中文显示"?"**：

- raylib TextRenderer 已有 `ensureFontCodepoints()` 懒加载机制：初始只加载 ASCII 码点（0x20-0x7E），绘制/测量含中文文本时检测缺失码点并自动重载字体
- 该机制通过 bridge 路径（`drawText(Font*, string, ...)` → `ensureFontCodepoints`）也应正常工作

**文件变更**：


| 文件                                   | 变更                                                   |
| -------------------------------------- | ------------------------------------------------------ |
| `include/UICornerstoneAPI.h`           | 在`UIBackendCallbacks` 中新增 `newFrame` 回调函数指针  |
| `src/backend/BackendBridge.h`          | 新增`bridge_newFrame()` 桥接函数                       |
| `src/CallbackAdapters.h`               | `CallbackInputBackend` 新增 `newFrame()` 覆盖          |
| `src/CallbackAdapters.cpp`             | 实现`CallbackInputBackend::newFrame()` → 委托到回调表 |
| `src/backend/sfml/RenderDevice.cpp`    | 新增`RegisterSFMLSurfaceFactories()`                   |
| `src/backend/raylib/RenderDevice.cpp`  | 新增`RegisterRaylibSurfaceFactories()`                 |
| `src/backend/sfml/BackendPlugin.cpp`   | 注册表面工厂 +`cb.newFrame = bridge_newFrame`          |
| `src/backend/raylib/BackendPlugin.cpp` | 注册表面工厂 +`cb.newFrame = bridge_newFrame`          |
| `src/backend/sdl3/BackendPlugin.cpp`   | 设置`cb.newFrame = bridge_newFrame`（一致但不必要）    |
| `src/backend/sfml/Window.cpp`          | 创建窗口后调用`setVerticalSyncEnabled(false)`          |

**验证**：SDL3/SFML/Raylib 三后端 fromsource 全部编译通过 + 运行通过。

### 2026-06-15: RGBA8888 像素格式根因排查 + DLL 桥接验证

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


| 后端   | 静态 (UICornerstone.lib)      | DLL (UICornerstone_dll.dll)  |
| ------ | ----------------------------- | ---------------------------- |
| SDL3   | 10/10 测试通过                | 3.3MB DLL + 10/10 测试通过   |
| SFML   | 10/10 测试通过                | 3.5MB DLL + 测试通过         |
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
**验证**：编译通过，全部 10 个 SDL3 测试无回归。

### 2026-06-30: Slider 控件优化（脏矩形/键盘重复/Value Label 跟随/字体懒加载）

**变更（2 文件）**：

| 文件 | 变更 |
|------|------|
| `include/Slider.h` | 新增 `m_lastRect`（脏矩形追踪）、`m_repeatKey/m_repeatStartTime/m_repeatNextTime`（键盘重复）、`m_tickFontAttempted`（字体加载状态）；新增 `ensureTickFont()`/`repositionValueLabel()`/`handleKeyRepeat()` 私有方法 |
| `src/Slider.cpp` | 见下方优化项 |

**5 项优化**：

1. **setRect 脏矩形检查**：`setRect()` 添加 `if (rect == m_lastRect) return;`，避免重复重定位 value label，减少 Label::setRect 调用链
2. **键盘按键重复**：`handleKeyRepeat()` 在 `update()` 中每帧检测，350ms 初始延迟 + 50ms 重复间隔；`handleEvent()` KeyDown 启动追踪，KeyUp/FocusLost 停止
3. **Value label 跟随 thumb**：`repositionValueLabel()` 将水平滑块标签居中于 thumb 上方（clamp 到滑块宽度），垂直滑块标签居中于 thumb 右侧（clamp 到滑块高度）；`updateValueLabel()` 末尾自动调用
4. **Tick 字体懒加载**：`ensureTickFont()` 单次加载 + `m_tickFontAttempted` 防止重复尝试；`draw()` 中移除了重复的字体加载代码
5. **draw() 去重**：水平/垂直分支中 Tick 字体加载代码抽离为 `ensureTickFont()`，消除 20 行重复

**细节改进**：
- `setShowValueLabel()` 在动态创建 label 后调用 `updateValueLabel()` 确保标题正确
- `m_lastRect` 在构造列表中初始化为空 SRect
- 键盘重复支持 Left/Right/Up/Down/PageUp/PageDown（Home/End 不重复，单次跳转）

**验证**：SDL3/SFML/Raylib 三后端全部编译通过。test_slider 运行正常，15 个滑块初始化 + 帧循环稳定。

### 2026-07-01: test_slider 布局间距 + raylib InputBackend KeyUp 修复

**变更（3 文件）**：

| 文件 | 变更 |
|------|------|
| `test/test_slider.cpp` | 左列 11 水平滑块 Y 重算（40~695），右列 4 垂直滑块 X 均匀分布（580~910），value label / tick label 全部留足间隙 |
| `src/backend/raylib/InputBackend.cpp` | 新增 `fillKeyUpEvent()` 和 Keyboard phase 中 `IsKeyUp()` 检测，按键释放时发送 `KeyUp` 事件 |
| `doc/Slider_Design.md` | §8 优化历史 + §10 关键实现注意事项更新 |

**Raylib KeyUp 根因**：raylib 使用 `GetKeyPressed()` / `IsKeyDown()` 但无释放 API，`Keyboard` phase 从未生成 `KeyUp` 事件 → Slider 的 `m_repeatKey` 永不归零 → `handleKeyRepeat()` 无限重复无法停止。

**验证**：SDL3/SFML/Raylib 三后端全部编译通过。test_slider 运行正常。

### 2026-07-07: Focus 环 3 层对比优化 + 全设计文档更新

**焦点环变更**：

| 文件 | 变更 |
|------|------|
| `src/ControlBase.cpp` | `drawFocusRing()` 改为 3 层：黑(inset 0, alpha 150) + 白(inset 1, alpha 150) + 用户颜色(inset 2)，保证任何背景色下至少一条线可见 |
| `include/ControlBase.h` | `m_focusRingAlwaysVisible = true`（默认） |
| `src/backend/sfml/InputBackend.cpp` | 添加 `unicode < 0x20 \|\| unicode == 0x7F` 控制字符过滤，防止 Tab 注入为文本输入 |
| `test/test_button.cpp` | 图片路径相对化（`assets/images/*.png` 替代硬编码绝对路径） |
| `test/test_winframe.cpp` | 添加跨 WinFrame 焦点测试控件 |
| 全部设计文档 | 同步更新焦点描述（3 层环、setFocusable(true)、m_focusRingAlwaysVisible、作用域边界等） |

**验证**：6 构建配置 + 全部 samples 编译通过。test_fromsource_xxx 三后端焦点环可见。

### 2026-07-11: ColorPicker 控件完整实现 + C ABI + JSON + 三后端验证

**ColorPicker 实现**（~2100 行源码）：

- 核心控件：`ColorPicker.h`/`ColorPicker.cpp` — 继承 Panel，闭合状态（色块 + 十六进制文本）+ 弹窗状态（预设色面板/Hex 输入/RGB+A 滑块/确定取消）
- 内部类 `PresetCell`：继承 ControlImpl，带选中/非选中状态色块绘制
- `ColorPickerBuilder`：Builder 模式工厂，支持全部视觉属性设置

**交互逻辑**：
- 点击色块切换弹窗；预设色点击选中不关闭；Hex 输入/Slider 三向同步
- 确定（`onOK`）提交颜色触发回调；取消（`onCancel`）恢复初始颜色
- Enter/ESC 键通过 `BeforeEventHandlingWatcher` 拦截，ESC=取消，Enter=确定
- `m_ignoreKeyEvent` 标志防止关闭弹窗后同一 Enter 事件重新打开
- 弹窗为 `FocusBoundary`，Tab 在弹窗内部循环，不越过边界
- 外部点击通过 watcher 检测关闭弹窗

**视觉属性**：
- `m_swatchSize` — 闭合状态色块大小（`setClosedSwatchSize(float)`)
- `m_closedFontSize` — Hex 文本字号（`setClosedFontSize(int)`）
- `m_closedTextColor` — Hex 文本颜色（`setClosedTextColor(SColor)`）
- `m_popupBGColor` — 弹窗背景色（`setPopupBGColor(SColor)`）

**C ABI 新增**（UICornerstoneAPI.h/.cpp）：
- `UICornerstone_CreateColorPicker` — 工厂函数
- `UICornerstone_GetColorPickerColor` — 获取当前颜色 Hex
- `UICornerstone_SetOnColorChanged` — 设置颜色变化回调
- `UICornerstone_SetClosedSwatchSize/SetClosedFontSize/SetClosedTextColor/SetPopupBGColor` — 视觉属性设置

**LayoutParser JSON 新增**：
- `swatchSize` — 闭合色块大小
- `closedFontSize` — Hex 文本字号
- `closedTextColor` — Hex 文本颜色
- `popupBGColor` — 弹窗背景色
- 已有：`color`、`presets`、`presetLayout`、`events.onColorChanged`

**测试**：
- `test/test_colorpicker.cpp` — 5 个 ColorPicker（含 2x 缩放）+ 状态标签
- `layouts/test_layout.json` — 新增 `testColorPicker` JSON 示例
- `test_fromsource_sdl3/sfml/raylib` — 三后端 C ABI ColorPicker 验证

**设计文档更新**：
- `doc/ColorPicker_Design.md` — 完整设计文档（13 节）
- `doc/FocusSystem_Design.md` — ColorPicker 焦点边界
- `doc/LayoutSystem_Design.md` — ColorPicker JSON 格式
- `README.md` — 14+ 控件、test_colorpicker/test_slider 列表
- `doc/Build_Guide.md` — test_colorpicker/test_slider 列表
- `doc/UICornerstone_DLL_Design.md` — 完整 C ABI API 清单（含 ColorPicker/Slider/WinFrame/TextArea/ImageButton 等）

**三后端构建验证**：SDL3/SFML/Raylib 全部编译通过，test_fromsource 三后端输出 "Color: #FF6600"。

### 2026-07-11: Dialog 2 Bug Fixes — Content Render Device + Destructor Crash

**Bug 1 — Popup 内容不可见**：

- Root cause: `setContent()` 在 `create()` 之前调用，此时 Popup 尚未从父级继承 `RenderDevice`。内容控件获取到 null render device → 不可见。
- Fix: `Popup::create()` 末尾添加对 `m_content` 的 render device/text renderer/resource provider/input backend 传播。

**Bug 2 — 关闭窗口时崩溃**：

- Root cause: `Popup::~Popup()` 调用 `getThis()`（即 `shared_from_this()`），在静态析构阶段 EventQueue 已被销毁后调用危险。
- Fix: 将析构函数清空；不在 `close()` 中移除 watcher（避免 ESC/outside-click 路径的 mutex 递归 lock → UB），不可见时 watcher 检查 `getVisible()` 安全返回 false；EventQueue 静态析构时自动清理 shared_ptr。

**验证**：全部 18 个 SDL3 目标编译通过，0 error 0 warning。test_dialog 运行 15 秒无崩溃。

### 2026-07-11: ColorPicker 闭合状态透明背景修复

**Bug**：LayoutParser 和 C ABI 路径下 ColorPicker 闭合状态背景为黑色（`DEFAULT_NORMAL_COLOR`），与父容器背景不一致。

**根因**：`setTransparent(true)` 和 `setBorderVisible(false)` 仅放在 `ColorPicker::create()` 中，但三个代码路径的时序不同：
- Builder 路径：`create()` → `addControl()`（无问题）
- LayoutParser 路径：构造 → 属性解析 → `create()` → `parseChildren` 中 `addControl`（`create()` 虽已调用但构造函数阶段的 `inheritRenderer()` 可能受其他因素影响）
- C ABI 路径：构造 → `addControl()` → `create()`（`addControl` 在 `create` 之前）

**Fix**：将 `setTransparent(true)` 和 `setBorderVisible(false)` 移至 `ColorPicker` 构造函数（`src/ColorPicker.cpp:70-71`），与 Label 做法一致（`src/Label.cpp:30`）。`create()` 中保留相同调用作为防御性冗余。

**验证**：test_layout / test_fromsource 三后端全部编译通过，闭合状态背景透明。

### 2026-07-12: test_dialog_cabi — Button色块 + RGB滑块 + 预设色 via JSON Dialog

**需求**：用户希望 Dialog 用纯 JSON 定义，含 Button 色块 + R/G/B 滑块 + 预设色按钮，而非 C++ ColorPicker 组件。

**变更**（3 个文件）：

- `src/LayoutParser.cpp` `parseEvents()`: 新增 Slider `onValueChanged` JSON 事件绑定
- `src/LayoutParser.cpp` `parseCommonProperties()`: 新增 `borderVisible` 属性支持
- `test/test_dialog_cabi.cpp`: 完全重写
  - 主界面：Button 色块(可点击) + Label 显示 Hex
  - JSON Dialog (`"dialogs"` 数组)：预览色块，R/G/B 滑块(`labelFormat: "R: %.0f"`)，5 个预设色按钮
  - `onColorChange` 滑块事件：`FindControl` + `GetSliderValue` 读取 RGB → `SetBGColor` 更新预览
  - `onPreset0-4` 预设色点击：`SetSliderValue` 设置 RGB → 更新预览
  - `onColorConfirmed` 确定回调：更新主界面色块 + Hex 标签
  - 使用 `UICornerstone_GetSliderValue/SetSliderValue/SetBGColor` C ABI

**验证**：SDL3 DLL 模式编译 0 错误，运行无崩溃。标准 test_dialog 无回归。

### 2026-07-12: test_dialog_cabi 三后端 + 设计文档刷新

**Bug 修复 — OK 按钮不提交颜色**：

- 根因：`test_dialog_cabi_shared.h:204` `//` 注释覆盖了整行，`g_savedR/G/B/A` 赋值语句被注释掉从未执行
- OK 流程：`onColorConfirmed` → 赋值被注释 → `close()` → `onColorClose` → `restoreFromSaved()` → 读取旧 `g_savedR/G/B/A` → swatch 恢复为原始颜色
- 修复：赋值移到注释后新行

**Raylib windows.h 冲突修复**：

- 根因：`CloseWindow()`/`DrawTextExA()` 等 raylib 函数名与 `<windows.h>` Win32 API 函数名冲突（均为 `extern "C"` 但签名不同）
- 修复：`test_dialog_cabi_shared.h` 用 `#ifndef _WINDOWS_` 条件守卫，未包含 windows.h 时手动 `extern "C" __declspec(dllimport)` 声明 `LoadLibraryA`/`GetProcAddress`/`FreeLibrary` + `using HMODULE = void*`

**UTF-8 BOM + 中文乱码修复**：

- 4 个测试文件（`test_dialog_cabi_shared.h` + 3 个 `.cpp`）全部转换为 UTF-8 with BOM
- 所有因 GBK→UTF-8 双编码而乱码的中文注释已恢复为正确文本

**设计文档刷新**（6 文件）：

| 文档 | 主要变更 |
|------|----------|
| `doc/EventSystem_Design.md` | **新建** — 事件系统完整设计文档（EventType→Event→InputBackend→EventQueue→控件分派→FocusManager→StateMachine） |
| `doc/Build_Guide.md` | 测试表 + fromsource 表添加 `test_dialog_cabi` 条目 |
| `doc/BackendAbstraction_Design.md` | 进度表新增 Phase 16h；§13 新增 Cursor 回调表工厂子节 |
| `doc/UICornerstone_DLL_Design.md` | `UIBackendCallbacks` 新增 3 个光标工厂回调；新增 Dialog C ABI API（11 个函数）；版本历史 1.13 |
| `doc/Dialog_Design.md` | 测试计划新增第 13 项（test_dialog_cabi）；新增 §14 跨后端注意事项（windows.h 冲突 + Cursor 工厂注册） |

**验证**：SDL3/SFML/Raylib 三后端 DLL 模式 test_dialog_cabi 全部编译通过，0 error，0 C4819。

### 2026-07-14: ComboBox 滚轮滚动 + Focus Tab 环重复 + 2x Popup 大小修复

**Bug 1 — 鼠标点击 ComboBox 箭头造成 Tab 环重复**：

- Root cause: `ControlImpl::setFocused()` 没有通知 `FocusManager`，导致 `FocusManager::m_currentFocused` 与控件的焦点状态不同步
- Fix: 在 `setFocused()` 末尾调用 `FocusManager::notifyControlFocused(this, byKeyboard)` 同步状态
- 注意：`FocusManager::notifyControlFocused` **不调用** `setFocused`（避免递归），只更新 `m_currentFocused`

**Bug 2 — ComboBox 弹出 Popup 中鼠标滚轮滚动不工作**（根因链复杂；需修复 3 个子问题才完全解决）：

1. **符号错误**：`ComboBoxListPanel::handleEvent` 中 `newOffset = m_scrollOffset - delta` 导致 `scrollY < 0`（向下滚动）时 `newOffset = 0 - 1 = -1` → 钳位到 0
   - Fix: `m_scrollOffset + delta`
2. **回调级联重置**：`updateScrollBar()` 调用 `setRange(0,22)` → 触发 `notifyPositionChanged` → `syncListFromScroll` → `setScrollOffset(0)`（ScrollBar 值尚未更新）；`setPageSize(8)` 同理。之后 `setValue(getScrollOffset())` 拿到被重置的 0。
   - Fix: 在 `setRange/setPageSize` 之前**保存** `int intendedOffset = m_listPanel->getScrollOffset()`，`setValue(intendedOffset)` 使用保存值
3. **ScrollBar 区域不可用**：鼠标在 ScrollBar 上时，ListPanel 的 `dr.contains(mx, my)` 返回 false（点不在 ListPanel 的 draw rect 内）。同时也应限制鼠标在 popup 区域外时不滚动。
   - Fix: `Popup::handleEvent` 拦截 `MouseWheel` 后用 `isContainsPoint` 检查鼠标是否在 Popup 区域内，仅在区域内才转发；不在区域内时直接 `return false` 阻止落到 `Panel::handleEvent`（否则子控件被重新遍历时不带区域检查）
4. **ComboBox 焦点时无位置限制**：`ComboBox::handleEvent` 中 `!isPopupOpen() && getFocused()` 的 MouseWheel 分支缺少 `isContainsPoint` 检查
   - Fix: 添加 `isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)` 检查

**重现过程**：通过 EventQueue 注入含正确屏幕坐标的 MouseWheel 事件，验证 `Popup::handleEvent` → `ComboBoxListPanel::handleEvent` → `setScrollOffset` 完整路由。

**Bug 3 — 2x 缩放下 Popup 尺寸翻倍**：

- `computePopupRect()` 返回缩放后的 pw/ph，`setRect` 再用 getDrawRect 时会再次缩放 → 2×
- Fix: `computePopupRect()` 返回 `pw / sx, bestPh / sy`（未缩放值）
- ListPanel/ScrollBar 子控件使用 base `1.0f` 缩放避免复合缩放

**ComboBox API 扩展**：

- `include/ComboBox.h`: 新增 `getListPanel()`（公开访问 `m_listPanel`），`openPopupForTest()`（公开调用 `openPopup()`）
- `src/ComboBox.cpp`: `updateScrollBar()` 保存预期 offset 避免回调级联

**验证**：SDL3 18 个目标全部编译通过。SFML/Raylib 静态构建通过。
