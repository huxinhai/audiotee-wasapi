# 快速修复指南 / Quick Fix Guide

## 🚨 当前状态 / Current Status

GitHub Actions 构建遇到链接器错误：找不到 `libsamplerate-0.lib`

GitHub Actions build encountered linker error: Cannot find `libsamplerate-0.lib`

### 进展 / Progress
- ✅ CMake 配置成功 / CMake configuration successful
- ✅ 头文件找到 / Header files found
- ✅ 编译成功 / Compilation successful
- ❌ 链接失败 / Linking failed

## ✅ 已应用的修复 / Applied Fixes

### CMakeLists.txt 更新 / CMakeLists.txt Updates

```cmake
# 使用 master 分支（最新修复）
GIT_TAG master  # Instead of 0.2.2

# 添加策略设置
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

# 正确的 include 路径
target_include_directories(wasapi_capture PRIVATE 
    ${libsamplerate_SOURCE_DIR}/include
    ${libsamplerate_SOURCE_DIR}/src
    ${libsamplerate_BINARY_DIR}
)

# 正确的库名称和链接方法
# 检查多个可能的目标名称
if(TARGET samplerate)
    target_link_libraries(wasapi_capture PRIVATE ole32 psapi samplerate)
elseif(TARGET libsamplerate)
    target_link_libraries(wasapi_capture PRIVATE ole32 psapi libsamplerate)
else()
    link_directories(${libsamplerate_BINARY_DIR}/src)
    target_link_libraries(wasapi_capture PRIVATE ole32 psapi samplerate)
endif()
```

## 📦 立即执行 / Execute Now

```bash
# 1. 提交修复
git add CMakeLists.txt BUILD_FIX.md docs/TROUBLESHOOTING.md QUICK_FIX.md test_build.bat
git commit -m "Fix libsamplerate linking issue and CMake compatibility

- Use master branch for latest CMake compatibility
- Set CMAKE_POLICY_DEFAULT_CMP0048 to NEW
- Fix include directories for samplerate.h
- Update library link name to samplerate
- Comprehensive troubleshooting documentation"

# 2. 推送到 GitHub
git push origin main

# 3. 重新触发构建（删除并重新创建标签）
git tag -d v1.0.0
git push origin :refs/tags/v1.0.0
git tag v1.0.0
git push origin v1.0.0
```

## 🔍 验证构建 / Verify Build

1. 访问 / Visit: https://github.com/huxinhai/audiotee-wasapi/actions
2. 等待构建完成 / Wait for build to complete (2-5 min)
3. 检查 Release / Check Release: https://github.com/huxinhai/audiotee-wasapi/releases

## ✅ 预期结果 / Expected Result

```
✓ CMake configuration successful
✓ Fetching libsamplerate from master branch
✓ Building with samplerate.h found
✓ Linking with samplerate library
✓ Creating wasapi_capture.exe
✓ Uploading to GitHub Release
```

## 🐛 如果仍然失败 / If Still Fails

### 选项 1: 检查日志 / Option 1: Check Logs
查看 GitHub Actions 详细日志，找到具体错误。

View detailed GitHub Actions logs to find specific error.

### 选项 2: 本地测试 / Option 2: Local Test
```batch
# Windows
test_build.bat

# 或手动 / Or manually
rmdir /s /q build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 选项 3: 使用特定提交 / Option 3: Use Specific Commit
如果 master 分支有问题，使用特定的稳定提交：

If master branch has issues, use a specific stable commit:

```cmake
GIT_TAG 914f8a07f7b310f85da0f0294f065de3921b8c5b  # Known good commit
```

## 📊 关键变更对比 / Key Changes Comparison

| 项目 / Item | 之前 / Before | 之后 / After |
|-------------|--------------|-------------|
| Git Tag | `0.2.2` | `master` |
| CMake Policy | 未设置 / Not set | `CMP0048 NEW` |
| Include Path | 仅 src / Only src | include, src, binary |
| Library Name | `samplerate_static` | `samplerate` |

## 🎯 为什么这些修复有效 / Why These Fixes Work

1. **master 分支** / Master Branch
   - 包含最新的 CMake 兼容性修复
   - Contains latest CMake compatibility fixes

2. **CMake 策略** / CMake Policy
   - 解决版本兼容性警告/错误
   - Resolves version compatibility warnings/errors

3. **完整的 Include 路径** / Complete Include Paths
   - 确保找到所有必需的头文件
   - Ensures all required headers are found

4. **正确的库名称** / Correct Library Name
   - 匹配 libsamplerate 实际导出的目标
   - Matches actual exported target from libsamplerate

## 📚 更多信息 / More Info

- 完整修复说明: [BUILD_FIX.md](BUILD_FIX.md)
- 故障排除: [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
- 重采样文档: [docs/RESAMPLING.md](docs/RESAMPLING.md)

---

**最后更新 / Last Updated:** 2025-01-17

**状态 / Status:** 🟡 等待测试 / Pending Test

修复已应用，等待 GitHub Actions 验证。

Fixes applied, waiting for GitHub Actions verification.

