@echo off
call "d:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

set UICONTROLS_ROOT=%~dp0..
set BUILD_DIR=%UICONTROLS_ROOT%\build\raylib

if "%1"=="" (
    echo Usage: build_raylib.bat [test_name]
    echo Example: build_raylib.bat            (builds all)
    echo Example: build_raylib.bat test_label (builds single test)
    pause
    exit /b 1
)

set TEST_NAME=%1

echo Building %TEST_NAME% [raylib backend]...

cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DUICONTROLS_BACKEND=RAYLIB >nul 2>&1
cmake --build "%BUILD_DIR%" --config Debug --target %TEST_NAME%

echo.
echo Build completed: %TEST_NAME%.exe [raylib]
echo Executable in: %BUILD_DIR%\test\Debug

pause
