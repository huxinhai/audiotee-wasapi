# 项目结构说明 / Project Structure

## 📁 当前项目结构 / Current Structure

```
audiotee-wasapi/
├── .github/
│   └── workflows/
│       └── build-release.yml       # GitHub Actions 自动构建工作流
│                                   # GitHub Actions auto-build workflow
├── src/
│   └── wasapi_capture.cpp          # 主程序源代码
│                                   # Main program source code
├── scripts/
│   ├── build.bat                   # CMake 构建脚本（Windows）
│   │                               # CMake build script (Windows)
│   └── build_simple.bat            # cl.exe 直接编译脚本
│                                   # cl.exe direct compilation script
├── docs/
│   ├── QUICK_START.md              # 快速开始指南
│   │                               # Quick start guide
│   └── RELEASE_GUIDE.md            # 发布指南
│                                   # Release guide
├── CMakeLists.txt                  # CMake 配置文件
│                                   # CMake configuration file
├── build.bat                       # 快速构建快捷方式（调用 scripts/build.bat）
│                                   # Quick build shortcut (calls scripts/build.bat)
├── README.md                       # 英文主文档
│                                   # Main documentation (English)
├── README_CN.md                    # 中文文档
│                                   # Chinese documentation
├── .gitignore                      # Git 忽略规则
│                                   # Git ignore rules
└── PROJECT_STRUCTURE.md            # 本文件 / This file
```

## 📂 目录说明 / Directory Description

### `.github/workflows/`
包含 GitHub Actions 配置文件，用于自动化构建和发布。
Contains GitHub Actions configuration files for automated building and releasing.

### `src/`
存放所有源代码文件。
Contains all source code files.

### `scripts/`
存放构建脚本和其他辅助脚本。
Contains build scripts and other helper scripts.

### `docs/`
存放项目文档（除了主 README）。
Contains project documentation (except main README).

## 🔨 构建说明 / Build Instructions

### 快速构建 / Quick Build
在项目根目录直接运行：
Run in project root:
```bash
build.bat
```

### 使用 CMake / Using CMake
```bash
scripts\build.bat
```
或手动执行 / Or manually:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 使用 cl.exe / Using cl.exe
```bash
scripts\build_simple.bat
```

## 📝 文档位置 / Documentation Location

- **主文档 / Main README**: `README.md` (英文 / English)
- **中文文档 / Chinese README**: `README_CN.md`
- **快速开始 / Quick Start**: `docs/QUICK_START.md`
- **发布指南 / Release Guide**: `docs/RELEASE_GUIDE.md`
- **项目结构 / Project Structure**: `PROJECT_STRUCTURE.md` (本文件 / This file)

## 🎯 为什么要重新组织？ / Why Reorganize?

### 优点 / Benefits

1. **更清晰的结构 / Clearer Structure**
   - 源代码、脚本、文档分离
   - Source code, scripts, and docs are separated

2. **更易维护 / Easier Maintenance**
   - 每个目录有明确的职责
   - Each directory has a clear responsibility

3. **更专业 / More Professional**
   - 符合开源项目的标准结构
   - Follows standard open-source project structure

4. **更易扩展 / Easier to Extend**
   - 添加新功能时目录结构清晰
   - Clear directory structure when adding new features

## 🚀 Git 提交 / Git Commit

重新组织后，需要提交这些更改：
After reorganization, commit these changes:

```bash
# 查看更改
git status

# 添加所有文件
git add .

# 提交
git commit -m "Reorganize project structure: separate src, scripts, and docs"

# 推送到 GitHub
git push origin main
```

## ⚙️ 已更新的文件 / Updated Files

以下文件已经自动更新以适应新结构：
The following files have been automatically updated for the new structure:

- ✅ `CMakeLists.txt` - 更新源文件路径
- ✅ `scripts/build.bat` - 更新路径导航
- ✅ `scripts/build_simple.bat` - 更新源文件路径
- ✅ `.github/workflows/build-release.yml` - 更新构建路径
- ✅ `README.md` - 更新项目结构和构建说明
- ✅ `README_CN.md` - 更新项目结构和构建说明
- ✅ `build.bat` - 新建快捷方式脚本
- ✅ `.gitignore` - 新建忽略规则文件

## 📦 构建产物 / Build Artifacts

构建产物仍然输出到 `build/` 目录（已在 .gitignore 中）：
Build artifacts are still output to `build/` directory (ignored in .gitignore):

```
build/
└── bin/
    └── Release/
        └── wasapi_capture.exe
```

---

**注意 / Note**: 本次重新组织不影响程序功能，只是优化了项目结构。
This reorganization doesn't affect program functionality, it just optimizes the project structure.

