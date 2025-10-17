# 🚀 提交和发布指南

## ✅ 检查清单

已完成的配置：
- [x] libsamplerate 源码已放在 `third_party/libsamplerate/`
- [x] CMakeLists.txt 已配置为使用本地 libsamplerate
- [x] wasapi_capture.cpp 已移除冲突的 pragma 指令
- [x] **修复了 libsamplerate 的 CMake 版本兼容性** (3.5...3.30)
- [x] .gitignore 已配置正确
- [x] 临时文件已清理

## 📤 提交到 GitHub

### 1. 提交所有更改

```bash
cd /Users/mac/html/audiotee-win

# 添加所有文件（包括 libsamplerate 源码）
git add .

# 提交
git commit -m "feat: 集成 libsamplerate 支持自定义采样率

- 添加 libsamplerate 源码到 third_party/
- 更新 CMakeLists.txt 使用本地 libsamplerate
- 修复 libsamplerate CMake 版本兼容性 (3.5...3.30)
- 实现 AudioResampler 类支持高质量重采样
- 移除冲突的 pragma comment 指令
- 添加下载辅助脚本"

# 推送到 GitHub
git push origin main
```

### 2. 创建发布版本（触发自动构建）

```bash
# 创建标签
git tag -a v1.1.0 -m "v1.1.0: 添加自定义采样率支持"

# 推送标签（这会触发 GitHub Actions 自动构建和发布）
git push origin v1.1.0
```

## 🤖 GitHub Actions 会自动：

1. ✅ 在 Windows 环境中构建项目
2. ✅ 使用本地 libsamplerate 源码编译
3. ✅ 生成 `wasapi_capture.exe`
4. ✅ 创建 GitHub Release
5. ✅ 上传可执行文件到 Release

## 📦 构建内容

Release 将包含：
- `wasapi_capture.exe` - 可执行文件（静态链接，无需额外依赖）
- `docs/QUICK_START.md` - 快速开始指南

## 🔍 查看构建状态

访问：https://github.com/huxinhai/audiotee-wasapi/actions

## 📥 下载使用

用户可以直接从 Release 页面下载：
https://github.com/huxinhai/audiotee-wasapi/releases

---

## 🆘 如果遇到问题

1. **构建失败**：检查 GitHub Actions 日志
2. **找不到 libsamplerate**：确认 `third_party/libsamplerate/CMakeLists.txt` 存在
3. **其他问题**：查看 `docs/TROUBLESHOOTING.md`

