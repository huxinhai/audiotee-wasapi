#!/bin/bash
# 下载并设置 libsamplerate 的脚本

set -e

echo "========================================"
echo "  下载 libsamplerate"
echo "========================================"
echo ""

# 创建目录
mkdir -p third_party
cd third_party

# 下载
echo "正在下载 libsamplerate-0.2.2..."
curl -L -o libsamplerate-0.2.2.tar.xz \
    https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz

# 解压
echo "正在解压..."
tar -xf libsamplerate-0.2.2.tar.xz

# 重命名
mv libsamplerate-0.2.2 libsamplerate

# 清理
rm libsamplerate-0.2.2.tar.xz

echo ""
echo "✅ 完成！"
echo ""
echo "目录结构："
ls -la libsamplerate/ | head -10

echo ""
echo "现在可以运行 cmake 构建了："
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  cmake --build . --config Release"

