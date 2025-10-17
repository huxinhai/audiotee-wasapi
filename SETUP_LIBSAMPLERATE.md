# 设置 libsamplerate 本地源码 / Setup Local libsamplerate

## 📥 步骤 1: 下载源码

### 方法 1: 浏览器下载（推荐）

1. 访问: https://github.com/libsndfile/libsamplerate/releases/tag/0.2.2
2. 下载: `libsamplerate-0.2.2.tar.xz` (约 500KB)
3. 将文件保存到项目的 `third_party` 目录

### 方法 2: 命令行下载

```bash
# Mac/Linux
cd /Users/mac/html/audiotee-win
mkdir -p third_party
cd third_party
curl -L -O https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz

# Windows (PowerShell)
cd C:\path\to\audiotee-win
mkdir third_party
cd third_party
Invoke-WebRequest -Uri "https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz" -OutFile "libsamplerate-0.2.2.tar.xz"
```

## 📦 步骤 2: 解压文件

### Mac/Linux
```bash
cd /Users/mac/html/audiotee-win/third_party
tar -xf libsamplerate-0.2.2.tar.xz
mv libsamplerate-0.2.2 libsamplerate
rm libsamplerate-0.2.2.tar.xz  # 可选：删除压缩包
```

### Windows (需要 7-Zip 或 WinRAR)
```powershell
# 使用 7-Zip 解压
7z x libsamplerate-0.2.2.tar.xz
7z x libsamplerate-0.2.2.tar
ren libsamplerate-0.2.2 libsamplerate
del libsamplerate-0.2.2.tar.xz
del libsamplerate-0.2.2.tar
```

或者直接使用图形界面工具解压。

## ✅ 步骤 3: 验证目录结构

解压后应该有以下结构：

```
audiotee-wasapi/
├── third_party/
│   └── libsamplerate/
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── samplerate.h
│       ├── src/
│       │   ├── samplerate.c
│       │   ├── src_sinc.c
│       │   └── ... (其他源文件)
│       └── ... (其他文件)
├── src/
│   └── wasapi_capture.cpp
└── CMakeLists.txt
```

## 🔧 步骤 4: 使用新的 CMakeLists.txt

文件已准备好，见下面。

## 🎯 步骤 5: 构建测试

```bash
# Mac/Linux
rm -rf build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64  # Windows
cmake --build . --config Release

# 或在 Mac 上
cmake .. -G "Xcode"
cmake --build . --config Release
```

## 📝 .gitignore 更新

建议不要忽略 `third_party/libsamplerate`，这样其他人克隆仓库后就能直接构建：

```gitignore
# 在 .gitignore 中
# 不要添加 third_party/ 到忽略列表！

# 但可以忽略压缩包
*.tar.xz
*.tar.gz
*.zip
```

## ✨ 优点

- ✅ 不需要网络（GitHub Actions 快速构建）
- ✅ 版本固定，无兼容性惊喜
- ✅ 完全控制源码
- ✅ 其他开发者克隆后即可构建

## 🚀 后续步骤

完成以上步骤后：
1. 使用新的 CMakeLists.txt（见项目根目录）
2. 测试构建：`test_build.bat`
3. 提交更改到 Git
4. 推送到 GitHub 触发自动构建

