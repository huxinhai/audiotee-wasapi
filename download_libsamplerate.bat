@echo off
REM 下载并设置 libsamplerate 的脚本（Windows PowerShell）

echo ========================================
echo   下载 libsamplerate
echo ========================================
echo.

REM 检查是否有 PowerShell
where powershell >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 错误：未找到 PowerShell
    echo 请手动下载：
    echo https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz
    pause
    exit /b 1
)

REM 创建目录
if not exist third_party mkdir third_party
cd third_party

REM 下载
echo 正在下载 libsamplerate-0.2.2...
powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz' -OutFile 'libsamplerate-0.2.2.tar.xz'}"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ 下载失败！
    echo.
    echo 请手动下载：
    echo 1. 访问：https://github.com/libsndfile/libsamplerate/releases/tag/0.2.2
    echo 2. 下载 libsamplerate-0.2.2.tar.xz
    echo 3. 解压到 third_party\libsamplerate\
    echo.
    pause
    exit /b 1
)

echo.
echo ✅ 下载完成！
echo.
echo 现在请使用 7-Zip 或其他工具解压：
echo   third_party\libsamplerate-0.2.2.tar.xz
echo.
echo 解压后重命名文件夹为：
echo   third_party\libsamplerate\
echo.
echo 然后运行：
echo   mkdir build
echo   cd build  
echo   cmake .. -G "Visual Studio 17 2022" -A x64
echo   cmake --build . --config Release
echo.
pause

