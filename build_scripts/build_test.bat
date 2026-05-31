@echo off
call "d:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

set UICONTROLS_ROOT=%~dp0..

if "%1"=="" (
    echo Usage: build_test.bat [test_name]
    echo Example: build_test.bat test_label
    echo Available tests: test_menu, test_label, test_editbox, test_checkbox, test_progressbar, test_layout, test_layout_advanced, test_winframe
    pause
    exit /b 1
)

set TEST_NAME=%1

echo Building %TEST_NAME%...
mkdir build 2>nul
cd /d build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug --target %TEST_NAME%

echo.
echo Build completed: %TEST_NAME%.exe

pause
