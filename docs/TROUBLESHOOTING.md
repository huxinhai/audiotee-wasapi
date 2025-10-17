# 故障排除指南 / Troubleshooting Guide

## ❌ GitHub Actions 403 错误 / GitHub Actions 403 Error

### 问题描述 / Problem Description

在 GitHub Actions 中创建 Release 时出现 403 错误：
When creating a Release in GitHub Actions, you get a 403 error:

```
⚠️ GitHub release failed with status: 403
❌ Too many retries. Aborting...
```

### 原因 / Cause

GitHub Actions 默认只有读权限，没有创建 Release 的写权限。
GitHub Actions by default only has read permissions, not write permissions to create Releases.

### 🔧 解决方案 / Solution

#### 步骤 1: 进入仓库设置 / Step 1: Go to Repository Settings

1. 打开你的 GitHub 仓库：
   Open your GitHub repository:
   ```
   https://github.com/huxinhai/audiotee-wasapi
   ```

2. 点击顶部的 **"Settings"** 标签页
   Click the **"Settings"** tab at the top

#### 步骤 2: 配置 Workflow 权限 / Step 2: Configure Workflow Permissions

1. 在左侧菜单中找到：
   In the left sidebar, find:
   - **"Actions"** → **"General"**

2. 滚动到页面底部的 **"Workflow permissions"** 部分
   Scroll down to the **"Workflow permissions"** section at the bottom

3. 选择以下选项：
   Select the following option:
   - ✅ **"Read and write permissions"**
   
4. （可选）勾选：
   (Optional) Check:
   - ✅ **"Allow GitHub Actions to create and approve pull requests"**

5. 点击 **"Save"** 按钮保存更改
   Click the **"Save"** button to save changes

#### 步骤 3: 重新运行失败的工作流 / Step 3: Re-run Failed Workflow

1. 进入 **"Actions"** 标签页
   Go to the **"Actions"** tab

2. 找到失败的工作流运行记录
   Find the failed workflow run

3. 点击 **"Re-run all jobs"** 按钮
   Click the **"Re-run all jobs"** button

4. 等待构建完成
   Wait for the build to complete

### ✅ 验证成功 / Verify Success

如果配置正确，你应该看到：
If configured correctly, you should see:

1. ✅ 工作流成功完成（绿色勾）
   Workflow completes successfully (green checkmark)

2. ✅ 在 **"Releases"** 页面出现新版本
   New release appears on the **"Releases"** page

3. ✅ Release 包含以下文件：
   Release contains the following files:
   - `wasapi_capture.exe`
   - `README.md`
   - `README_CN.md`
   - `QUICK_START.md`

---

## 🔄 其他常见问题 / Other Common Issues

### 问题 2: 构建失败 - CMake 错误 / Build Failed - CMake Error

**错误信息 / Error Message:**
```
CMake Error: Could not find Visual Studio
```

**解决方案 / Solution:**
- 确保 GitHub Actions 使用的是 `windows-latest`
- 工作流文件已经配置了正确的 Visual Studio 版本
- Ensure GitHub Actions uses `windows-latest`
- Workflow file is configured with correct Visual Studio version

### 问题 2.5: 构建失败 - 找不到 samplerate.h / Build Failed - Cannot find samplerate.h

**错误信息 / Error Message:**
```
error C1083: Cannot open include file: 'samplerate.h': No such file or directory
```

**原因 / Cause:**
libsamplerate 的 include 路径没有正确配置。

**解决方案 / Solution:**

已在 CMakeLists.txt 中修复。确保包含以下配置：

Fixed in CMakeLists.txt. Ensure it includes:

```cmake
# Include directories - libsamplerate headers
target_include_directories(wasapi_capture PRIVATE 
    ${libsamplerate_SOURCE_DIR}/include
    ${libsamplerate_SOURCE_DIR}/src
    ${libsamplerate_BINARY_DIR}
)
```

如果仍有问题，尝试：

If still having issues, try:

1. 清理构建目录 / Clean build directory:
   ```bash
   rm -rf build
   mkdir build
   cd build
   cmake ..
   ```

2. 检查 FetchContent 是否成功 / Check if FetchContent succeeded:
   - 查看构建日志中的 "Fetching libsamplerate" 消息
   - Look for "Fetching libsamplerate" message in build logs

3. 使用稳定版本标签 / Use stable version tag:
   ```cmake
   GIT_TAG 0.2.2  # Instead of master
   ```

### 问题 3: 找不到源文件 / Source File Not Found

**错误信息 / Error Message:**
```
fatal error: cannot open source file: 'wasapi_capture.cpp'
```

**解决方案 / Solution:**
- 确保源文件在 `src/` 目录中
- 检查 `CMakeLists.txt` 中的路径是否正确
- Ensure source file is in `src/` directory
- Check if paths in `CMakeLists.txt` are correct

### 问题 4: 推送标签失败 / Tag Push Failed

**错误信息 / Error Message:**
```
error: failed to push some refs to 'github.com/...'
```

**解决方案 / Solution:**

```bash
# 检查标签是否已存在
git tag

# 如果标签已存在，先删除
git tag -d v1.0.0
git push origin :refs/tags/v1.0.0

# 重新创建并推送
git tag v1.0.0
git push origin v1.0.0
```

### 问题 5: 权限被拒绝 / Permission Denied (本地)

**错误信息 / Error Message:**
```
Permission denied (publickey)
```

**解决方案 / Solution:**

**选项 A: 使用 HTTPS**
```bash
# 修改远程地址为 HTTPS
git remote set-url origin https://github.com/huxinhai/audiotee-wasapi.git
```

**选项 B: 配置 SSH**
```bash
# 生成 SSH 密钥
ssh-keygen -t ed25519 -C "your_email@example.com"

# 添加到 ssh-agent
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# 复制公钥
cat ~/.ssh/id_ed25519.pub
# 将输出的内容添加到 GitHub Settings → SSH Keys
```

---

## 📞 获取帮助 / Getting Help

### 检查工作流日志 / Check Workflow Logs

1. 进入 **Actions** 标签页
   Go to **Actions** tab

2. 点击失败的工作流
   Click on the failed workflow

3. 点击具体的 job（如 "build"）
   Click on specific job (e.g., "build")

4. 展开失败的步骤查看详细日志
   Expand failed steps to view detailed logs

### 有用的命令 / Useful Commands

```bash
# 查看 Git 配置
git config --list

# 查看远程仓库地址
git remote -v

# 查看所有标签
git tag

# 查看最近的提交
git log --oneline -10

# 检查工作目录状态
git status
```

### 联系方式 / Contact

如果问题仍未解决：
If the problem persists:

1. 查看 [GitHub Actions 文档](https://docs.github.com/en/actions)
2. 在仓库中创建 Issue
3. 查看工作流日志获取更多信息

---

## 🎯 快速参考 / Quick Reference

### 权限设置位置 / Permission Settings Location

```
GitHub 仓库
→ Settings
→ Actions
→ General
→ Workflow permissions
→ 选择 "Read and write permissions"
→ Save
```

### 重新运行工作流 / Re-run Workflow

```
GitHub 仓库
→ Actions
→ 选择失败的工作流
→ Re-run all jobs
```

### 删除并重新创建标签 / Delete and Recreate Tag

```bash
# 本地删除
git tag -d v1.0.0

# 远程删除
git push origin :refs/tags/v1.0.0

# 重新创建
git tag v1.0.0

# 推送
git push origin v1.0.0
```

---

**提示 / Tip**: 大多数 GitHub Actions 问题都与权限相关。确保正确配置 Workflow permissions！
Most GitHub Actions issues are permission-related. Make sure Workflow permissions are correctly configured!

