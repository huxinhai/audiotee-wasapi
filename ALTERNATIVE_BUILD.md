# 替代构建方案 / Alternative Build Solutions

## 🎯 方案 1: 使用本地 libsamplerate 源码（推荐）

### 下载和设置

```bash
# 1. 下载 libsamplerate
cd /Users/mac/html/audiotee-win
mkdir -p third_party
cd third_party

# 下载 0.2.2 稳定版
curl -L https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz -o libsamplerate-0.2.2.tar.xz

# 解压
tar -xf libsamplerate-0.2.2.tar.xz
mv libsamplerate-0.2.2 libsamplerate

# 删除压缩包
rm libsamplerate-0.2.2.tar.xz
```

### 修改 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Use local libsamplerate instead of FetchContent
add_subdirectory(third_party/libsamplerate)

# Add executable
add_executable(wasapi_capture src/wasapi_capture.cpp)

# Include directories
target_include_directories(wasapi_capture PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/src
    ${CMAKE_BINARY_DIR}/third_party/libsamplerate
)

# Link libraries
target_link_libraries(wasapi_capture PRIVATE
    ole32
    psapi
    samplerate
)

# Set output directory
set_target_properties(wasapi_capture PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(wasapi_capture PRIVATE /W4)
    set_target_properties(wasapi_capture PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()
```

**优点：**
- ✅ 不需要网络下载（GitHub Actions 更快）
- ✅ 版本固定，不会有兼容性惊喜
- ✅ 可以直接修改和调试 libsamplerate

**缺点：**
- ❌ 仓库体积增大（约 500KB）
- ❌ 需要手动更新 libsamplerate

---

## 🎯 方案 2: 使用预编译的库（最简单）

如果有预编译的 Windows 库文件：

### 目录结构
```
audiotee-wasapi/
├── third_party/
│   └── libsamplerate/
│       ├── include/
│       │   └── samplerate.h
│       └── lib/
│           ├── x64/
│           │   ├── Release/
│           │   │   └── samplerate.lib
│           │   └── Debug/
│           │       └── samplerate.lib
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(wasapi_capture src/wasapi_capture.cpp)

# Include directories
target_include_directories(wasapi_capture PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/include
)

# Link libraries
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SAMPLERATE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/lib/x64/Debug/samplerate.lib")
else()
    set(SAMPLERATE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/lib/x64/Release/samplerate.lib")
endif()

target_link_libraries(wasapi_capture PRIVATE
    ole32
    psapi
    ${SAMPLERATE_LIB}
)

# Set output directory
set_target_properties(wasapi_capture PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(wasapi_capture PRIVATE /W4)
    set_target_properties(wasapi_capture PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()
```

**优点：**
- ✅ 最快的构建速度
- ✅ 不需要编译 libsamplerate
- ✅ 完全控制

**缺点：**
- ❌ 需要找到或自己编译预编译库
- ❌ 需要提交二进制文件到仓库

---

## 🎯 方案 3: 使用 vcpkg（现代化）

### 设置 vcpkg

```bash
# 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# 安装 libsamplerate
./vcpkg install libsamplerate:x64-windows-static
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libsamplerate via vcpkg
find_package(SampleRate REQUIRED)

# Add executable
add_executable(wasapi_capture src/wasapi_capture.cpp)

# Link libraries
target_link_libraries(wasapi_capture PRIVATE
    ole32
    psapi
    SampleRate::samplerate
)

# Set output directory
set_target_properties(wasapi_capture PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(wasapi_capture PRIVATE /W4)
    set_target_properties(wasapi_capture PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()
```

### GitHub Actions 配置

```yaml
- name: Setup vcpkg
  run: |
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.bat
    ./vcpkg install libsamplerate:x64-windows-static

- name: Configure CMake
  run: |
    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
```

**优点：**
- ✅ 现代化的包管理
- ✅ 自动处理依赖
- ✅ 易于更新

**缺点：**
- ❌ GitHub Actions 需要安装 vcpkg（增加构建时间）
- ❌ 学习曲线

---

## 🎯 方案 4: 手动编译静态库（一次性工作）

### 步骤

1. **本地编译 libsamplerate：**
   ```bash
   # 下载源码
   curl -L https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz -o libsamplerate.tar.xz
   tar -xf libsamplerate.tar.xz
   cd libsamplerate-0.2.2
   
   # 使用 CMake 编译
   mkdir build && cd build
   cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
   cmake --build . --config Release
   cmake --build . --config Debug
   ```

2. **复制编译好的文件：**
   ```
   third_party/libsamplerate/
   ├── include/
   │   └── samplerate.h
   └── lib/
       ├── Release/
       │   └── samplerate.lib
       └── Debug/
           └── samplerate.lib
   ```

3. **使用方案 2 的 CMakeLists.txt**

---

## 📊 方案对比

| 方案 | 简单度 | 速度 | 可控性 | 适用场景 |
|------|-------|------|--------|---------|
| FetchContent (当前) | ⭐⭐⭐ | ⭐⭐ | ⭐⭐ | 简单项目 |
| 本地源码 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | **推荐** |
| 预编译库 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 需要预先编译 |
| vcpkg | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | 现代化项目 |
| 手动编译 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 一次性设置 |

## 🎯 我的推荐

**对于你的项目，我建议使用方案 1（本地源码）：**

1. ✅ 解决当前所有 CMake 和链接问题
2. ✅ GitHub Actions 构建更快更稳定
3. ✅ 不需要额外的包管理器
4. ✅ 可以轻松调试和修改

### 快速实施方案 1

```bash
# 在项目根目录执行
mkdir -p third_party && cd third_party
curl -L https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz -o lib.tar.xz
tar -xf lib.tar.xz && mv libsamplerate-0.2.2 libsamplerate && rm lib.tar.xz
cd ..

# 然后使用上面方案 1 的 CMakeLists.txt
```

这样就完全避免了网络下载和 CMake 兼容性问题！

想试试这个方案吗？我可以帮你创建新的 CMakeLists.txt。

