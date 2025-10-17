@echo off
REM Test build script to verify CMake configuration

echo ==========================================
echo Testing WASAPI Capture Build
echo ==========================================
echo.

REM Clean previous build
if exist build_test rmdir /s /q build_test
mkdir build_test
cd build_test

echo.
echo [1/3] Configuring CMake...
echo ==========================================
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ CMake configuration failed!
    echo Check the error messages above.
    cd ..
    pause
    exit /b 1
)

echo.
echo ✓ CMake configuration successful!
echo.
echo [2/3] Building project...
echo ==========================================
cmake --build . --config Release --verbose

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Build failed!
    echo Check the error messages above.
    cd ..
    pause
    exit /b 1
)

echo.
echo ✓ Build successful!
echo.
echo [3/3] Checking output...
echo ==========================================

if exist bin\Release\wasapi_capture.exe (
    echo ✓ Executable found: bin\Release\wasapi_capture.exe
    echo.
    echo File info:
    dir bin\Release\wasapi_capture.exe
    echo.
    echo ==========================================
    echo ✅ All tests passed!
    echo ==========================================
    echo.
    echo You can run the executable with:
    echo   build_test\bin\Release\wasapi_capture.exe --help
) else (
    echo ❌ Executable not found!
    echo Expected location: bin\Release\wasapi_capture.exe
)

cd ..
echo.
pause

