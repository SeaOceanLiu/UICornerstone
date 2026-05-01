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
- **Available tests**: test_menu, test_label, test_editbox, test_checkbox, test_progressbar

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