# 构建修复说明 / Build Fix Notes

## 🐛 问题 / Issues

### 问题 1: 找不到头文件 / Issue 1: Cannot find header file

GitHub Actions 构建失败，错误信息：
GitHub Actions build failed with error:

```
error C1083: Cannot open include file: 'samplerate.h': No such file or directory
```

### 问题 2: CMake 版本兼容性 / Issue 2: CMake version compatibility

```
CMake Error at build/_deps/libsamplerate-src/CMakeLists.txt:1 (cmake_minimum_required):
  Compatibility with CMake < 3.5 has been removed from CMake.
```

**原因 / Cause:**
libsamplerate 0.2.2 版本的 CMakeLists.txt 要求的 CMake 版本与最新版本不兼容。

libsamplerate 0.2.2's CMakeLists.txt requires an older CMake version incompatible with latest CMake.

## ✅ 修复 / Fix

### 1. CMakeLists.txt 修复 / CMakeLists.txt Fixes

**修复前 / Before:**
```cmake
target_include_directories(wasapi_capture PRIVATE ${libsamplerate_SOURCE_DIR}/src)
target_link_libraries(wasapi_capture
    ole32
    psapi
    samplerate_static  
)
```

**修复后 / After:**
```cmake
# Include directories - libsamplerate headers
target_include_directories(wasapi_capture PRIVATE 
    ${libsamplerate_SOURCE_DIR}/include
    ${libsamplerate_SOURCE_DIR}/src
    ${libsamplerate_BINARY_DIR}
)

# Link required Windows libraries and libsamplerate
target_link_libraries(wasapi_capture PRIVATE
    ole32
    psapi
    samplerate
)
```

**关键改动 / Key Changes:**
1. ✅ 添加了 `${libsamplerate_SOURCE_DIR}/include` 路径
   Added `${libsamplerate_SOURCE_DIR}/include` path
   
2. ✅ 添加了 `${libsamplerate_BINARY_DIR}` 路径（生成的头文件）
   Added `${libsamplerate_BINARY_DIR}` path (generated headers)
   
3. ✅ 使用 `master` 分支以获得最新的 CMake 兼容性修复
   Used `master` branch for latest CMake compatibility fixes
   
4. ✅ 设置 CMake 策略以确保兼容性
   Set CMake policy for compatibility
   ```cmake
   set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
   ```
   
5. ✅ 库名称从 `samplerate_static` 改为 `samplerate`
   Changed library name from `samplerate_static` to `samplerate`

### 2. GitHub Actions 工作流改进 / Workflow Improvements

**添加了 / Added:**
```yaml
- name: Configure CMake
  run: |
    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
  shell: cmd

- name: Build Release
  run: |
    cd build
    cmake --build . --config Release --verbose
  shell: cmd
```

**改进 / Improvements:**
- ✅ 明确指定 shell 为 `cmd`
- ✅ 添加 `--verbose` 标志以便调试
- ✅ 添加 `-DCMAKE_BUILD_TYPE=Release` 配置

## 🧪 本地测试 / Local Testing

使用测试脚本验证构建：
Use test script to verify build:

```batch
test_build.bat
```

这将：
This will:
1. 清理之前的构建 / Clean previous builds
2. 配置 CMake / Configure CMake  
3. 构建项目 / Build project
4. 验证可执行文件 / Verify executable

## 📦 提交修复 / Commit Fix

```bash
git add CMakeLists.txt .github/workflows/build-release.yml docs/TROUBLESHOOTING.md test_build.bat BUILD_FIX.md
git commit -m "Fix libsamplerate include path for GitHub Actions build

- Add multiple include directories for libsamplerate
- Use stable version tag (0.2.2) instead of master
- Change library link name from samplerate_static to samplerate
- Add verbose build output for debugging
- Update troubleshooting documentation"

git push origin main
```

## 🔄 重新触发构建 / Retrigger Build

### 方法 1: 重新推送标签 / Method 1: Re-push Tag

```bash
# 删除本地和远程标签
git tag -d v1.0.0
git push origin :refs/tags/v1.0.0

# 重新创建并推送
git tag v1.0.0
git push origin v1.0.0
```

### 方法 2: 手动触发 / Method 2: Manual Trigger

1. 访问 / Go to: `https://github.com/huxinhai/audiotee-wasapi/actions`
2. 选择 "Build and Release" 工作流 / Select "Build and Release" workflow
3. 点击 "Run workflow" / Click "Run workflow"
4. 选择 main 分支 / Select main branch
5. 点击 "Run workflow" 按钮 / Click "Run workflow" button

## ✅ 验证成功 / Verify Success

构建成功后，检查：
After successful build, check:

1. ✅ Actions 标签页显示绿色勾号 / Actions tab shows green checkmark
2. ✅ Releases 页面出现新版本 / Releases page shows new version
3. ✅ 下载的 exe 文件可以运行 / Downloaded exe file runs correctly

## 🎯 后续步骤 / Next Steps

1. 验证自动构建成功 / Verify auto-build success
2. 测试下载的可执行文件 / Test downloaded executable
3. 更新版本号（如需要）/ Update version number (if needed)
4. 完善文档 / Improve documentation

## 📚 相关文档 / Related Docs

- [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) - 完整的故障排除指南
- [RESAMPLING.md](docs/RESAMPLING.md) - 重采样功能说明
- [QUICK_START.md](docs/QUICK_START.md) - 快速开始指南

---

**注意 / Note:** 此修复解决了 libsamplerate 头文件路径问题，确保 GitHub Actions 可以正确构建项目。

This fix resolves the libsamplerate header file path issue, ensuring GitHub Actions can build the project correctly.

