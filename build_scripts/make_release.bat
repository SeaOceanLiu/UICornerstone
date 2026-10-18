@echo off
set "PATH=%SystemRoot%\System32;%PATH%"
chcp 65001 >nul
setlocal EnableDelayedExpansion

rem ============================================================
rem make_release.bat - 生成 UICornerstone Release 发布包（增量组装）
rem
rem 用法:
rem   make_release.bat              -> 生成 <仓库>/release
rem   make_release.bat D:\out       -> 生成指定目录
rem   make_release.bat [dir] -y     -> 跳过交互暂停
rem
rem 增量语义（2026-10-18）:
rem   不再整删 release；所有复制仅针对"目标缺失 / 源时间戳较新 / 大小不同"的
rem   文件（robocopy 默认比较规则），未变文件保留原有时间戳——后续 robocopy
rem   /MIR 同步到 subModules 时仅传输实际变更。目录树用 /MIR 镜像（复制增量 +
rem   删除源中已不存在的文件）；release 根目录旧 DLL 与 assets/binding/tools
rem   根层旧文件会定点清理（防改名/弃用残留）。
rem
rem 产物:
rem   UICornerstone.dll / UIBackend_{sdl3,sfml,raylib}.dll
rem   SDL3.dll / SDL3_ttf.dll / SDL3_image.dll
rem   sfml-graphics-3.dll / sfml-window-3.dll / sfml-system-3.dll
rem   raylib.dll
rem   assets/ / binding/ / tools/(validate_layout.exe + schema)
rem ============================================================

set "UICORNERSTONE_ROOT=%~dp0.."
set "RELEASE_DIR=%~1"
if "%RELEASE_DIR%"=="" set "RELEASE_DIR=%UICORNERSTONE_ROOT%\release"
if /i "%~2"=="-y" set "NO_PAUSE=1"

echo [%date% %time%] ============================================
echo [%date% %time%] make_release start
echo [%date% %time%] root: %UICORNERSTONE_ROOT%
echo [%date% %time%] release dir: %RELEASE_DIR%
echo [%date% %time%] ============================================

rem ============================================================
rem 0. 清理 build 目录（递归删除非空，强制全新构建，防旧版本残留）
rem ============================================================
echo [%time%] [0/6] Cleaning build dirs (sdl3_dll sfml_dll raylib_dll tools)...
for %%D in (sdl3_dll sfml_dll raylib_dll tools) do (
    if exist "%UICORNERSTONE_ROOT%\build\%%D" (
        rd /s /q "%UICORNERSTONE_ROOT%\build\%%D"
        if exist "%UICORNERSTONE_ROOT%\build\%%D" (
            echo [ERROR] failed to remove build\%%D
            goto :fail
        )
    )
)
echo [%time%] [0/6] Clean done.

rem ============================================================
rem 1. 构建三后端 Release DLL（仅核心 DLL + 后端插件，不编译 test/CABI/sample）
rem ============================================================
call :build_backend sdl3
if errorlevel 1 goto :fail
call :build_backend SFML
if errorlevel 1 goto :fail
call :build_backend RAYLIB
if errorlevel 1 goto :fail
echo [%time%] [1/6] Backends (sdl3/sfml/raylib) DLL built.

rem ============================================================
rem 2. 构建 tools（validate_layout Release）
rem ============================================================
call :build_tools
if errorlevel 1 goto :fail
echo [%time%] [2/6] tools (validate_layout) built.

rem ============================================================
rem 3. 准备 release 目录（增量：不整删；定点清理旧产物残留）
rem ============================================================
echo [%time%] [3/6] Preparing release (incremental): %RELEASE_DIR%
if not exist "%RELEASE_DIR%" (
    mkdir "%RELEASE_DIR%" || (echo [ERROR] cannot create %RELEASE_DIR% & goto :fail)
)
mkdir "%RELEASE_DIR%\binding" 2>nul
mkdir "%RELEASE_DIR%\tools" 2>nul
rem 清理根目录旧 DLL（改名/弃用的后端等残留；仅 *.dll，不动其它文件）
call :prune_files "%RELEASE_DIR%" "*.dll" "UICornerstone.dll UIBackend_sdl3.dll UIBackend_sfml.dll UIBackend_raylib.dll SDL3.dll SDL3_ttf.dll SDL3_image.dll sfml-graphics-3.dll sfml-window-3.dll sfml-system-3.dll raylib.dll"
rem 清理 assets/binding/tools 根层旧文件（其子目录由 /MIR 镜像自动维护）
call :prune_files "%RELEASE_DIR%\assets" "*" "Windows.jsonc"
call :prune_files "%RELEASE_DIR%\binding" "*" "CMakeLists.txt LICENSE CMakeLists.example.txt"
call :prune_files "%RELEASE_DIR%\tools" "*" "validate_layout.exe PropertyNames.h"
echo [%time%] [3/6] Release dir prepared.

rem ============================================================
rem 4. 复制 核心 DLL + 后端 DLL（仅新增/更新；未变保留时间戳）
rem ============================================================
call :rcopy "%UICORNERSTONE_ROOT%\build\sdl3_dll\Release" "%RELEASE_DIR%" "UICornerstone.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\build\sdl3_dll\Release" "%RELEASE_DIR%" "UIBackend_sdl3.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\build\sfml_dll\Release" "%RELEASE_DIR%" "UIBackend_sfml.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\build\raylib_dll\Release" "%RELEASE_DIR%" "UIBackend_raylib.dll"
if errorlevel 1 goto :fail
echo [%time%] [4/6] Core + backend DLLs synced.

rem ============================================================
rem 5. 复制 运行时 DLL（仅新增/更新）
rem ============================================================
call :rcopy "%UICORNERSTONE_ROOT%\subModules\libs" "%RELEASE_DIR%" "SDL3.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\subModules\libs" "%RELEASE_DIR%" "SDL3_ttf.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\subModules\libs" "%RELEASE_DIR%" "SDL3_image.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\subModules\SFML\bin" "%RELEASE_DIR%" "sfml-graphics-3.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\subModules\SFML\bin" "%RELEASE_DIR%" "sfml-window-3.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\subModules\SFML\bin" "%RELEASE_DIR%" "sfml-system-3.dll"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\subModules\raylib\lib" "%RELEASE_DIR%" "raylib.dll"
if errorlevel 1 goto :fail
echo [%time%] [5/6] Runtime DLLs synced.

rem ============================================================
rem 6. 复制 assets + binding + tools（目录树 /MIR 镜像：增量 + 清理已删除项）
rem ============================================================
rem assets: 仅运行时所需子集（排除 background/config/music/saved/sounds）
mkdir "%RELEASE_DIR%\assets" 2>nul
call :rcopy "%UICORNERSTONE_ROOT%\subModules\assets" "%RELEASE_DIR%\assets" "Windows.jsonc"
if errorlevel 1 goto :fail
for %%A in (animations fonts images) do (
    call :rmirror "%UICORNERSTONE_ROOT%\subModules\assets\%%A" "%RELEASE_DIR%\assets\%%A"
    if errorlevel 1 goto :fail
)

rem 用户手册 docs 站点（跟随发布；含 docs/assets、css、controls/appendix 等全部页面）
call :rmirror "%UICORNERSTONE_ROOT%\docs" "%RELEASE_DIR%\docs"
if errorlevel 1 goto :fail

rem binding: 仅源码（排除 build 编译产物）
call :rcopy "%UICORNERSTONE_ROOT%\binding" "%RELEASE_DIR%\binding" "CMakeLists.txt"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\binding" "%RELEASE_DIR%\binding" "LICENSE"
if errorlevel 1 goto :fail
for %%B in (src include cmake samples) do (
    call :rmirror "%UICORNERSTONE_ROOT%\binding\%%B" "%RELEASE_DIR%\binding\%%B"
    if errorlevel 1 goto :fail
)
call :rcopy "%UICORNERSTONE_ROOT%\include" "%RELEASE_DIR%\binding\include" "UICornerstoneAPI.h"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\include" "%RELEASE_DIR%\binding\include" "PropertyNames.h"
if errorlevel 1 goto :fail
call :rcopy_ren "%UICORNERSTONE_ROOT%\build_scripts" "CMakeLists.binding_sample.txt" "%RELEASE_DIR%\binding" "CMakeLists.example.txt"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\build\tools\Release" "%RELEASE_DIR%\tools" "validate_layout.exe"
if errorlevel 1 goto :fail
call :rcopy "%UICORNERSTONE_ROOT%\include" "%RELEASE_DIR%\tools" "PropertyNames.h"
if errorlevel 1 goto :fail
call :rmirror "%UICORNERSTONE_ROOT%\docs\schema" "%RELEASE_DIR%\tools\schema"
if errorlevel 1 goto :fail
echo [%time%] [6/6] assets + binding + tools synced.

rem ============================================================
rem 7. 校验清单
rem ============================================================
set "MISSING="
if not exist "%RELEASE_DIR%\tools\validate_layout.exe" set "MISSING=!MISSING! tools/validate_layout.exe"
if not exist "%RELEASE_DIR%\tools\PropertyNames.h" set "MISSING=!MISSING! tools/PropertyNames.h"
if not exist "%RELEASE_DIR%\tools\schema\declarative-ui.schema.json" set "MISSING=!MISSING! tools/schema"
for %%F in (UICornerstone.dll UIBackend_sdl3.dll UIBackend_sfml.dll UIBackend_raylib.dll SDL3.dll SDL3_ttf.dll SDL3_image.dll sfml-graphics-3.dll sfml-window-3.dll sfml-system-3.dll raylib.dll) do (
    if not exist "%RELEASE_DIR%\%%F" set "MISSING=!MISSING! %%F"
)
if not exist "%RELEASE_DIR%\assets"         set "MISSING=!MISSING! assets"
if not exist "%RELEASE_DIR%\docs\index.html" set "MISSING=!MISSING! docs/ (用户手册)"
if not exist "%RELEASE_DIR%\binding\include\UICornerstone.h"     set "MISSING=!MISSING! binding/include/UICornerstone.h"
if not exist "%RELEASE_DIR%\binding\include\UICornerstoneAPI.h" set "MISSING=!MISSING! binding/include/UICornerstoneAPI.h"
if not exist "%RELEASE_DIR%\binding\include\PropertyNames.h"     set "MISSING=!MISSING! binding/include/PropertyNames.h"
if not exist "%RELEASE_DIR%\binding\src\UICornerstone.cpp"       set "MISSING=!MISSING! binding/src/UICornerstone.cpp"
if not exist "%RELEASE_DIR%\binding\CMakeLists.example.txt"      set "MISSING=!MISSING! binding/CMakeLists.example.txt"

if not "%MISSING%"=="" (
    echo [ERROR] missing:%MISSING%
    goto :fail
)

echo [%date% %time%] ============================================
echo [%date% %time%] Release ready: %RELEASE_DIR%
echo [%date% %time%]   core + backend DLLs + runtime DLLs + assets + binding + tools
echo [%date% %time%]   incremental: only new/changed files copied; unchanged keep timestamps
echo [%date% %time%]   tools: validate_layout.exe + PropertyNames.h + schema
echo [%date% %time%]   usage(in release dir): tools\validate_layout.exe layout.json --strict   (缺省从 exe 同目录找 PropertyNames.h、下一级 schema/ 找 schema)
echo [%date% %time%] make_release done
echo [%date% %time%] ============================================
if not defined NO_PAUSE pause
exit /b 0

:fail
echo [%time%] [ERROR] make_release failed.
if not defined NO_PAUSE pause
exit /b 1

rem ============================================================
rem 辅助：增量复制（robocopy 默认比较规则：目标缺失 / 源较新 / 大小不同 → 复制；
rem       相同 → 跳过并保留目标时间戳）
rem ============================================================
:rcopy
rem 用法: call :rcopy "src_dir" "dst_dir" "filename"
robocopy "%~1" "%~2" "%~3" /NJH /NJS /NP /NFL /NDL /R:2 /W:1 >nul
if errorlevel 8 (echo [ERROR] robocopy failed: %~1\%~3 & exit /b 1)
exit /b 0

:rmirror
rem 目录树增量镜像（复制新增/更新 + 删除源中已不存在的文件；未变文件保留时间戳）
rem 用法: call :rmirror "src_dir" "dst_dir"
robocopy "%~1" "%~2" /MIR /NJH /NJS /NP /NFL /NDL /R:2 /W:1 >nul
if errorlevel 8 (echo [ERROR] robocopy mirror failed: %~1 -^> %~2 & exit /b 1)
exit /b 0

:rcopy_ren
rem 重命名式增量复制（内容一致则跳过；用于 CMakeLists.binding_sample.txt -> CMakeLists.example.txt）
rem 用法: call :rcopy_ren "src_dir" "src_name" "dst_dir" "dst_name"
if exist "%~3\%~4" (
    fc /b "%~1\%~2" "%~3\%~4" >nul 2>&1
    if not errorlevel 1 exit /b 0
)
copy /y "%~1\%~2" "%~3\%~4" >nul || (echo [ERROR] copy failed: %~2 -^> %~4 & exit /b 1)
exit /b 0

:prune_files
rem 删除指定目录下匹配模式且不在保留列表中的文件（仅文件，跳过子目录；防旧产物残留）
rem 用法: call :prune_files "dir" "pattern" "keep1 keep2 ..."
for %%F in ("%~1\%~2") do (
    if not exist "%%~F\" (
        set "KEEP="
        for %%K in (%~3) do if /i "%%~nxF"=="%%K" set "KEEP=1"
        if not defined KEEP (
            echo [%time%]   prune stale: %%~F
            del /f /q "%%~F" >nul
        )
    )
)
exit /b 0

:build_backend
set "BNAME=%~1"
set "BLOWER="
if /i "%BNAME%"=="sdl3"   (set "BDIR=sdl3_dll"   & set "BOPT=SDL3"   & set "BLOWER=sdl3")
if /i "%BNAME%"=="SFML"   (set "BDIR=sfml_dll"   & set "BOPT=SFML"   & set "BLOWER=sfml")
if /i "%BNAME%"=="RAYLIB" (set "BDIR=raylib_dll" & set "BOPT=RAYLIB" & set "BLOWER=raylib")
set "BUILD_DIR=%UICORNERSTONE_ROOT%\build\%BDIR%"

echo [%time%] Building UICornerstone [%BOPT% Release DLL]...
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DUICORNERSTONE_BACKEND=%BOPT% -DUICORNERSTONE_BUILD_DLL=ON >nul 2>&1
    if errorlevel 1 (echo [ERROR] cmake configure %BOPT% failed & exit /b 1)
)
cmake --build "%BUILD_DIR%" --config Release --target UICornerstone_dll UIBackend_%BLOWER%
if errorlevel 1 (echo [ERROR] cmake build %BOPT% failed & exit /b 1)
echo [%time%]   %BOPT% DLL built.
exit /b 0

:build_tools
set "TOOLS_DIR=%UICORNERSTONE_ROOT%\build\tools"
echo [%time%] Building validate_layout [Release]...
if not exist "%TOOLS_DIR%\CMakeCache.txt" (
    cmake -S "%UICORNERSTONE_ROOT%\tools" -B "%TOOLS_DIR%" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release >nul 2>&1
    if errorlevel 1 (echo [ERROR] cmake configure tools failed & exit /b 1)
)
cmake --build "%TOOLS_DIR%" --config Release --target validate_layout
if errorlevel 1 (echo [ERROR] cmake build validate_layout failed & exit /b 1)
echo [%time%]   validate_layout built.
exit /b 0
