@echo off
call "d:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

set UICORNERSTONE_ROOT=%~dp0..
set BUILD_DIR=%UICORNERSTONE_ROOT%\build\sfml

if "%1"=="" (
    echo Usage: build_sfml.bat [test_name]
    echo Example: build_sfml.bat            (builds all)
    echo Example: build_sfml.bat test_label (builds single test)
    pause
    exit /b 1
)

set TEST_NAME=%1

echo Building %TEST_NAME% [SFML backend]...

cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DUICORNERSTONE_BACKEND=SFML >nul 2>&1
cmake --build "%BUILD_DIR%" --config Debug --target %TEST_NAME%

echo.
echo Build completed: %TEST_NAME%.exe [SFML]
echo Executable in: %BUILD_DIR%\test\Debug

pause
