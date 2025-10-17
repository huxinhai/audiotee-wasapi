@echo off
REM Build script for WASAPI Capture

echo Building WASAPI Capture...

REM Navigate to project root
cd /d "%~dp0.."

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the project
cmake --build . --config Release

echo.
echo Build complete! Executable is located at: build\bin\Release\wasapi_capture.exe
echo.

cd ..
pause

