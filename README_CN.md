# WASAPI Audio Capture

中文文档 | [English](README.md)

一个基于 Windows WASAPI（Windows Audio Session API）的系统音频捕获工具，可以录制 Windows 系统正在播放的所有音频（Loopback Recording）。

## 功能特性

- ✅ 捕获系统音频（所有正在播放的声音）
- ✅ **实时音频格式转换**（采样率、声道数、位深）
- ✅ 支持自定义采样率（8000 - 192000 Hz）
- ✅ 支持自定义声道数（1=单声道，2=立体声）
- ✅ 支持自定义位深（16/24/32 位）
- ✅ 内置 Windows Media Foundation 重采样器（高质量）
- ✅ 支持自定义缓冲区大小
- ✅ 事件驱动模式（无丢帧）和轮询模式
- ✅ 输出原始 PCM 音频数据到标准输出
- ✅ 详细的错误诊断和解决方案
- ✅ 支持 Ctrl+C 优雅退出
- 📝 静音模式（暂未实现）
- 📝 进程过滤（暂未实现）

## 系统要求

- **操作系统**: Windows Vista 或更高版本（推荐 Windows 10/11）
- **编译器**: 
  - Visual Studio 2017 或更高版本（推荐 Visual Studio 2022）
  - 或任何支持 C++17 的 MSVC 编译器
- **构建工具**: 
  - CMake 3.15 或更高版本（方法一）
  - Visual Studio Developer Command Prompt（方法二）

## 下载预编译版本

你可以从 [GitHub Releases](https://github.com/huxinhai/audiotee-wasapi/releases) 下载最新的预编译可执行文件。

无需编译 - 直接下载 `wasapi_capture.exe` 即可运行！

## 编译方法

### 方法一：使用 CMake + Visual Studio（推荐）

这是推荐的编译方式，适合大多数用户：

```batch
# 双击项目根目录的 build.bat，或在命令行中执行：
build.bat

# 或者直接运行 scripts 文件夹中的脚本：
scripts\build.bat
```

编译完成后，可执行文件位置：
```
build\bin\Release\wasapi_capture.exe
```

**详细步骤说明：**
1. 确保已安装 Visual Studio 2022（包含 C++ 桌面开发工具）
2. 确保已安装 CMake（可以从 https://cmake.org/ 下载）
3. 双击项目根目录的 `build.bat`，或运行 `scripts\build.bat`
4. 编译过程会自动创建 `build` 目录并生成项目文件
5. 最终生成的可执行文件在 `build\bin\Release\wasapi_capture.exe`

### 方法二：使用 cl.exe 直接编译（快速）

如果你熟悉 Visual Studio 开发者命令提示符，可以使用此方法快速编译：

```batch
# 1. 打开 "x64 Native Tools Command Prompt for VS 2022"
#    （在开始菜单搜索 "x64 Native Tools"）

# 2. 切换到项目目录
cd /d "项目路径"

# 3. 运行编译脚本
scripts\build_simple.bat
```

编译完成后，可执行文件位置：
```
wasapi_capture.exe（在项目根目录）
```

**详细步骤说明：**
1. 打开"开始菜单"，搜索 "x64 Native Tools Command Prompt"
2. 进入 Visual Studio 开发者命令提示符
3. 使用 `cd` 命令切换到项目目录
4. 运行 `scripts\build_simple.bat`
5. 编译完成后会在项目根目录生成 `wasapi_capture.exe`

### 手动编译（高级用户）

如果需要自定义编译选项：

```batch
# 使用 CMake
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# 或直接使用 cl.exe（编译所有源文件）
cl.exe /EHsc /O2 /std:c++17 /I"src" ^
    src\main.cpp ^
    src\core\wasapi_capture_impl.cpp ^
    src\core\audio_resampler.cpp ^
    src\utils\error_handler.cpp ^
    ole32.lib psapi.lib mf.lib mfplat.lib mfreadwrite.lib ^
    /Fe:wasapi_capture.exe
```

## 使用说明

### 基本用法

```batch
# 捕获系统音频并保存为原始 PCM 文件
wasapi_capture.exe > output.pcm

# 捕获系统音频并通过管道传递给 FFmpeg 转换为 MP3
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 output.mp3
```

### 命令行参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `--sample-rate <Hz>` | 设置目标采样率（8000-192000 Hz，默认：使用设备默认值）| `--sample-rate 16000` |
| `--channels <数量>` | 设置声道数（1=单声道，2=立体声，默认：使用设备默认值）| `--channels 1` |
| `--bit-depth <位数>` | 设置位深（16/24/32，默认：使用设备默认值）| `--bit-depth 16` |
| `--chunk-duration <秒>` | 设置音频块持续时间（默认：0.2 秒）| `--chunk-duration 0.1` |
| `--mute` | 捕获时静音系统音频（暂未实现）| `--mute` |
| `--include-processes <PID>` | 只捕获指定进程的音频（暂未实现）| `--include-processes 1234 5678` |
| `--exclude-processes <PID>` | 排除指定进程的音频（暂未实现）| `--exclude-processes 1234` |
| `--help` 或 `-h` | 显示帮助信息 | `--help` |

### 使用示例

#### 示例 1：基本录制（保存为原始 PCM）
```batch
wasapi_capture.exe > music.pcm
```
按 `Ctrl+C` 停止录制。

#### 示例 2：使用 FFmpeg 转换为 WAV 格式
```batch
# 假设设备采样率为 48000 Hz，2 声道，16 位
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 output.wav
```

#### 示例 3：实时流式转换为 MP3
```batch
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 -b:a 192k output.mp3
```

#### 示例 4：音频格式转换（重采样 + 声道转换）
```batch
# 转换为 16kHz 单声道 16 位（适合语音识别）
wasapi_capture.exe --sample-rate 16000 --channels 1 --bit-depth 16 > speech.pcm

# 转换为 44.1kHz 立体声 16 位（CD 音质）
wasapi_capture.exe --sample-rate 44100 --channels 2 --bit-depth 16 > music.pcm

# 转换为 8kHz 单声道（电话音质，文件最小）
wasapi_capture.exe --sample-rate 8000 --channels 1 --bit-depth 16 > phone.pcm
```

#### 示例 5：语音识别流水线
```batch
# 以最适合语音识别的格式捕获音频并通过管道传递给 FFmpeg
wasapi_capture.exe --sample-rate 16000 --channels 1 --bit-depth 16 2>nul | ffmpeg -f s16le -ar 16000 -ac 1 -i pipe:0 speech.wav
```

#### 示例 6：调整缓冲区大小
```batch
# 使用更小的缓冲区（降低延迟）
wasapi_capture.exe --chunk-duration 0.05 > output.pcm

# 使用更大的缓冲区（更稳定）
wasapi_capture.exe --chunk-duration 0.5 > output.pcm
```

#### 示例 7：静默错误信息并转换为 FLAC
```batch
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 -c:a flac output.flac
```

### 关于音频格式

程序输出的是**原始 PCM 音频数据**（无文件头）。输出格式可以通过命令行参数自定义：

**默认行为**（不指定格式参数）：
- 使用设备的原生格式（通常为 48000 Hz，2 声道，32 位浮点）
- 自动转换为 16 位 PCM 整数格式

**自定义格式**（使用 `--sample-rate`、`--channels`、`--bit-depth`）：
- **采样率**: 8000-192000 Hz（如 16000 用于语音，44100 用于音乐）
- **声道数**: 1（单声道）或 2（立体声）
- **位深**: 16/24/32 位（推荐 16 位以获得最佳兼容性）

**格式转换：**
程序使用 Windows Media Foundation 内置的重采样器进行高质量实时音频转换：
- ✅ 自动格式检测（PCM/浮点）
- ✅ 采样率转换（如 48000→16000 Hz）
- ✅ 声道转换（如立体声→单声道）
- ✅ 位深转换（如 32 位浮点→16 位 PCM）

运行程序时，会显示输出格式：
```
========================================
输出音频格式：
  采样率: 16000 Hz
  声道数: 1
  位深:   16 bits
========================================
```

**FFmpeg 参数对照表：**
| 输出格式 | FFmpeg 参数 |
|---------|------------|
| 16kHz, 单声道, 16位 | `-f s16le -ar 16000 -ac 1` |
| 44.1kHz, 立体声, 16位 | `-f s16le -ar 44100 -ac 2` |
| 48kHz, 立体声, 24位 | `-f s24le -ar 48000 -ac 2` |
| 48kHz, 立体声, 32位 | `-f s32le -ar 48000 -ac 2` |

## 常见问题

### 1. 编译错误："找不到 Visual Studio"

**解决方案：**
- 确保已安装 Visual Studio 2022（包含 "C++ 桌面开发" 工作负载）
- 如果安装了不同版本的 Visual Studio，修改 `build.bat` 中的生成器：
  ```batch
  # Visual Studio 2019
  cmake .. -G "Visual Studio 16 2019" -A x64
  
  # Visual Studio 2017
  cmake .. -G "Visual Studio 15 2017" -A x64
  ```

### 2. 运行时错误："No audio device found"

**解决方案：**
1. 右键点击任务栏的音量图标
2. 选择"打开声音设置"
3. 确保有可用的输出设备且未被禁用
4. 播放一些音频测试设备是否正常工作

### 3. 运行时错误："Access denied"

**解决方案：**
- 以管理员身份运行程序（右键 -> 以管理员身份运行）
- 检查 Windows 隐私设置 -> 麦克风访问权限
- 临时禁用杀毒软件测试

### 4. 运行时错误："Audio device is in use"

**解决方案：**
1. 关闭独占使用音频设备的程序（部分游戏、专业音频软件）
2. 打开"声音设置" -> 设备属性 -> 其他设备属性
3. 进入"高级"选项卡
4. 取消勾选"允许应用程序独占控制此设备"

### 5. 运行时错误："Unsupported format"

**解决方案：**
- 不使用 `--sample-rate` 参数（使用设备默认值）
- 尝试常用采样率：`--sample-rate 44100` 或 `--sample-rate 48000`
- 更新音频驱动程序

### 6. 音频出现断断续续或丢帧

**解决方案：**
- 增大缓冲区：`--chunk-duration 0.5`
- 关闭其他占用 CPU 的程序
- 确保系统有足够的可用内存

### 7. 没有声音被捕获（输出文件为空或全为 0）

**解决方案：**
- 确保系统正在播放音频
- 检查系统音量是否为 0
- 验证默认播放设备设置是否正确
- 尝试播放音乐并重新运行程序

### 8. 与 FFmpeg 配合使用时出现格式错误

**解决方案：**
- 运行程序查看实际的音频格式（会输出到 stderr）
- 根据实际格式调整 FFmpeg 参数：
  ```batch
  # 如果是 48000 Hz, 2 声道, 16 位
  wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 output.mp3
  
  # 如果是 44100 Hz
  wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 44100 -ac 2 -i pipe:0 output.mp3
  ```

## 技术细节

### 架构说明
- 使用 Windows WASAPI（Windows Audio Session API）进行音频捕获
- Loopback 模式：捕获系统正在播放的音频（渲染端点回放）
- 支持事件驱动和轮询两种模式
  - **事件驱动模式**（优先）：无延迟、无丢帧
  - **轮询模式**（备用）：定期检查音频缓冲区

### 音频流处理
1. 通过 WASAPI 获取系统默认音频输出设备
2. 以 Loopback 模式初始化音频客户端
3. 持续读取音频缓冲区中的数据
4. 将原始 PCM 数据写入标准输出（stdout）
5. 错误和状态信息输出到标准错误（stderr）

### 错误处理
程序包含详细的错误诊断系统，对于常见的 WASAPI 错误代码会提供：
- 错误原因说明
- 详细的解决方案步骤
- 系统要求检查（Windows 版本、管理员权限等）

## 自动构建和发布

本项目使用 GitHub Actions 进行自动构建和发布。

### 如何创建发布版本

1. **标记版本：**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. **自动构建：** GitHub Actions 将自动：
   - 使用 Visual Studio 2022 构建项目
   - 编译 Release 版可执行文件
   - 创建新的 GitHub Release
   - 上传 `wasapi_capture.exe` 和文档文件

3. **手动触发：** 你也可以在 GitHub 的 "Actions" 标签页手动触发构建。

### GitHub Actions 工作流

工作流程（`.github/workflows/build-release.yml`）包括：
- 在 Windows 环境自动编译
- CMake + Visual Studio 2022 构建
- 构建产物上传
- 自动创建 Release 并附带可执行文件和文档

## 开发者信息

### 项目结构
```
audiotee-wasapi/
├── .github/
│   └── workflows/
│       └── build-release.yml        # GitHub Actions 工作流
├── src/
│   ├── main.cpp                     # 应用程序入口
│   ├── core/                        # 核心音频处理模块
│   │   ├── wasapi_capture.h
│   │   ├── wasapi_capture_impl.cpp
│   │   ├── audio_resampler.h
│   │   └── audio_resampler.cpp
│   └── utils/                       # 工具模块
│       ├── common.h
│       ├── error_handler.h
│       └── error_handler.cpp
├── scripts/
│   ├── build.bat                    # CMake 编译脚本
│   └── build_simple.bat             # cl.exe 直接编译脚本
├── docs/
│   ├── ARCHITECTURE.md              # 架构设计
│   ├── CODE_STRUCTURE.md            # 代码结构
│   ├── QUICK_START.md               # 快速开始指南
│   └── RELEASE_GUIDE.md             # 发布指南
├── CMakeLists.txt                   # CMake 构建配置
├── build.bat                        # 快速构建快捷方式
├── README.md                        # 英文文档
├── README_CN.md                     # 本文件（中文）
├── LAYERED_REFACTORING.md           # 分层重构总结
└── .gitignore                       # Git 忽略规则
```

### 依赖库
- `ole32.lib` - COM 库
- `psapi.lib` - 进程状态 API
- `mf.lib` - Media Foundation
- `mfplat.lib` - Media Foundation 平台
- `mfreadwrite.lib` - Media Foundation 读写

### 编译要求
- C++17 标准
- Windows SDK（包含 WASAPI 头文件）

### 错误代码
程序使用以下退出代码：
- `0` - 成功
- `1` - COM 初始化失败
- `2` - 未找到音频设备
- `3` - 设备访问被拒绝
- `4` - 不支持的音频格式
- `8` - 无效的参数
- `99` - 未知错误

## 许可证

本项目为开源项目，可自由使用和修改。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 相关链接

- [Windows WASAPI 官方文档](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi)
- [FFmpeg 官方网站](https://ffmpeg.org/)
- [Visual Studio 下载](https://visualstudio.microsoft.com/)
- [CMake 下载](https://cmake.org/download/)
- [捕获 macOS 上的系统音频输出](https://github.com/makeusabrew/audiotee)

## 更新日志

### v1.0
- ✅ 实现基本的 WASAPI 音频捕获功能
- ✅ 支持自定义采样率和缓冲区大小
- ✅ 事件驱动和轮询两种模式
- ✅ 详细的错误诊断系统
- ✅ 支持原始 PCM 输出到标准输出

---


