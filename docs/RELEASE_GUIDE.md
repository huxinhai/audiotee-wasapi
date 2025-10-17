# Release Guide / 发布指南

## English

### How to Create a New Release

1. **Commit and push your changes:**
   ```bash
   git add .
   git commit -m "Your commit message"
   git push origin main
   ```

2. **Create and push a version tag:**
   ```bash
   # Create a tag (e.g., v1.0.0, v1.0.1, v2.0.0)
   git tag v1.0.0
   
   # Push the tag to GitHub
   git push origin v1.0.0
   ```

3. **Wait for automatic build:**
   - Go to your GitHub repository
   - Click on "Actions" tab
   - You'll see the workflow running
   - Wait for it to complete (usually takes 2-5 minutes)

4. **Check the release:**
   - Go to "Releases" tab in your repository
   - You should see the new release with:
     - `wasapi_capture.exe` - The compiled executable
     - `README.md` - English documentation
     - `README_CN.md` - Chinese documentation

### Manual Trigger

You can also manually trigger the build without creating a tag:

1. Go to "Actions" tab in your repository
2. Click on "Build and Release" workflow
3. Click "Run workflow" button
4. Select the branch and click "Run workflow"

**Note:** Manual triggers will build the project but won't create a GitHub Release (only for testing purposes).

### Version Naming Convention

- **Major version** (v2.0.0): Breaking changes or major new features
- **Minor version** (v1.1.0): New features, backward compatible
- **Patch version** (v1.0.1): Bug fixes and minor changes

---

## 中文

### 如何创建新版本

1. **提交并推送更改：**
   ```bash
   git add .
   git commit -m "你的提交信息"
   git push origin main
   ```

2. **创建并推送版本标签：**
   ```bash
   # 创建标签（例如：v1.0.0, v1.0.1, v2.0.0）
   git tag v1.0.0
   
   # 推送标签到 GitHub
   git push origin v1.0.0
   ```

3. **等待自动构建：**
   - 进入你的 GitHub 仓库
   - 点击 "Actions" 标签页
   - 你会看到工作流正在运行
   - 等待完成（通常需要 2-5 分钟）

4. **检查发布：**
   - 进入仓库的 "Releases" 标签页
   - 你应该能看到新的发布版本，包含：
     - `wasapi_capture.exe` - 编译后的可执行文件
     - `README.md` - 英文文档
     - `README_CN.md` - 中文文档

### 手动触发

你也可以在不创建标签的情况下手动触发构建：

1. 进入仓库的 "Actions" 标签页
2. 点击 "Build and Release" 工作流
3. 点击 "Run workflow" 按钮
4. 选择分支并点击 "Run workflow"

**注意：** 手动触发只会构建项目但不会创建 GitHub Release（仅用于测试）。

### 版本命名规范

- **主版本** (v2.0.0)：重大变更或主要新功能
- **次版本** (v1.1.0)：新功能，向后兼容
- **修订版本** (v1.0.1)：错误修复和小改动

---

## Troubleshooting / 故障排除

### Build Failed / 构建失败

**Check the logs:**
1. Go to "Actions" tab
2. Click on the failed workflow
3. Check the error messages

**Common issues:**
- CMake configuration error: Check `CMakeLists.txt`
- Compilation error: Check your source code
- Missing files: Ensure all required files are committed

**检查日志：**
1. 进入 "Actions" 标签页
2. 点击失败的工作流
3. 查看错误信息

**常见问题：**
- CMake 配置错误：检查 `CMakeLists.txt`
- 编译错误：检查源代码
- 文件缺失：确保所有必需文件已提交

### Release Not Created / Release 未创建

- Ensure you pushed a tag starting with 'v' (e.g., v1.0.0)
- Check that the workflow completed successfully
- Manual triggers don't create releases, only tag pushes do

- 确保推送的标签以 'v' 开头（例如：v1.0.0）
- 检查工作流是否成功完成
- 手动触发不会创建 Release，只有推送标签才会

### Permission Denied / 权限被拒绝

The workflow uses `GITHUB_TOKEN` which is automatically provided by GitHub. If you encounter permission issues:
1. Go to repository Settings → Actions → General
2. Under "Workflow permissions", ensure "Read and write permissions" is selected

工作流使用 GitHub 自动提供的 `GITHUB_TOKEN`。如果遇到权限问题：
1. 进入仓库设置 → Actions → General
2. 在 "Workflow permissions" 下，确保选择了 "Read and write permissions"

