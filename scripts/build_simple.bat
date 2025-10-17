@echo off
REM Simple build script using cl.exe directly (requires Visual Studio Developer Command Prompt)

echo Building WASAPI Capture with cl.exe...

REM Navigate to project root
cd /d "%~dp0.."

cl.exe /EHsc /O2 /std:c++17 src\wasapi_capture.cpp ole32.lib psapi.lib /Fe:wasapi_capture.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! wasapi_capture.exe created.
    echo.
) else (
    echo.
    echo Build failed!
    echo.
)

pause

