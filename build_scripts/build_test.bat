@echo off
call "d:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

set UICORNERSTONE_ROOT=%~dp0..

if "%1"=="" (
    echo Usage: build_test.bat [test_name] [sdl3^|sfml]
    echo Example: build_test.bat test_label
    echo Example: build_test.bat test_label sfml
    echo Available tests: test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced, test_winframe, test_graphtool, test_button
    pause
    exit /b 1
)

set TEST_NAME=%1
set BACKEND=%2
if "%BACKEND%"=="" set BACKEND=sdl3

set BUILD_DIR=%UICORNERSTONE_ROOT%\build\%BACKEND%

echo Building %TEST_NAME% [%BACKEND% backend]...

cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DUICORNERSTONE_BACKEND=%BACKEND% >nul 2>&1

cmake --build "%BUILD_DIR%" --config Debug --target %TEST_NAME%

echo.
echo Build completed: %TEST_NAME%.exe [%BACKEND%]
echo Executable in: %BUILD_DIR%\test\Debug

pause
