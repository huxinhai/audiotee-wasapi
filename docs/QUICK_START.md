# 快速开始指南 / Quick Start Guide

## 📤 如何提交代码到 GitHub

### 第一步：查看当前状态
```bash
git status
```
这会显示所有已修改和新增的文件。

### 第二步：添加所有文件
```bash
# 添加所有更改的文件
git add .

# 或者单独添加文件
git add .github/workflows/build-release.yml
git add .github/RELEASE_GUIDE.md
git add README.md
git add README_CN.md
```

### 第三步：提交更改
```bash
git commit -m "Add GitHub Actions auto-build and release workflow"
```

### 第四步：推送到 GitHub
```bash
# 如果是第一次推送
git push origin main

# 如果你的默认分支是 master
git push origin master
```

### 第五步：创建第一个发布版本
```bash
# 创建版本标签
git tag v1.0.0

# 推送标签到 GitHub（这会触发自动构建）
git push origin v1.0.0
```

### 第六步：查看自动构建
1. 打开浏览器，访问你的 GitHub 仓库
2. 点击顶部的 **"Actions"** 标签页
3. 你会看到一个正在运行的工作流 "Build and Release"
4. 等待 2-5 分钟，直到显示绿色的 ✓ 

### 第七步：查看发布的文件
1. 在 GitHub 仓库页面，点击 **"Releases"** 标签页
2. 你会看到 v1.0.0 版本
3. 下面会有可下载的文件：
   - `wasapi_capture.exe` - 可执行文件
   - `README.md` - 英文文档
   - `README_CN.md` - 中文文档

---

## 🔄 日常更新流程

当你修改代码后，要发布新版本：

```bash
# 1. 查看更改
git status

# 2. 添加所有更改
git add .

# 3. 提交更改
git commit -m "描述你的更改内容"

# 4. 推送到 GitHub
git push origin main

# 5. 创建新版本（如果需要发布）
git tag v1.0.1
git push origin v1.0.1
```

---

## 📋 完整命令速查

```bash
# 一次性提交所有更改并推送
git add .
git commit -m "你的提交信息"
git push origin main

# 发布新版本
git tag v1.0.0
git push origin v1.0.0
```

---

## ⚠️ 常见问题

### 问题 1: 如果还没有配置远程仓库

```bash
# 你的仓库地址：https://github.com/huxinhai/audiotee-wasapi

# 如果是全新的项目
git init
git add .
git commit -m "Initial commit"
git branch -M main
git remote add origin https://github.com/huxinhai/audiotee-wasapi.git
git push -u origin main

# 然后创建第一个版本
git tag v1.0.0
git push origin v1.0.0
```

### 问题 2: 分支名是 master 而不是 main

```bash
# 将所有 main 改为 master
git push origin master
```

### 问题 3: 权限问题

如果推送时提示权限错误，你需要配置 Git 凭据：

**方法 1：使用 Personal Access Token (推荐)**
1. 访问 GitHub Settings → Developer settings → Personal access tokens
2. 生成新的 token（需要 repo 权限）
3. 推送时使用 token 作为密码

**方法 2：使用 SSH**
```bash
# 生成 SSH 密钥
ssh-keygen -t ed25519 -C "your_email@example.com"

# 添加到 ssh-agent
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# 复制公钥并添加到 GitHub
cat ~/.ssh/id_ed25519.pub

# 修改远程仓库地址为 SSH
git remote set-url origin git@github.com:huxinhai/audiotee-wasapi.git
```

### 问题 4: 如何删除错误的 tag

```bash
# 删除本地 tag
git tag -d v1.0.0

# 删除远程 tag
git push origin :refs/tags/v1.0.0
```

### 问题 5: 查看所有 tag

```bash
git tag
```

---

## 🎯 版本号建议

- **v1.0.0** - 首个正式版本
- **v1.0.1** - 修复 bug
- **v1.1.0** - 添加新功能
- **v2.0.0** - 重大更新或不兼容的更改

---

## 📞 需要帮助？

- 查看 GitHub Actions 日志：仓库 → Actions 标签页
- 查看发布指南：`.github/RELEASE_GUIDE.md`
- Git 基础教程：https://git-scm.com/book/zh/v2

