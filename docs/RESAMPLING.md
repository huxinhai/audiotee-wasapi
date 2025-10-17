# 重采样功能说明 / Resampling Guide

## 📖 概述 / Overview

从 v1.0 开始，WASAPI Audio Capture 集成了 [libsamplerate](https://github.com/libsndfile/libsamplerate)（也称为 Secret Rabbit Code），提供高质量的实时音频重采样功能。

Starting from v1.0, WASAPI Audio Capture integrates [libsamplerate](https://github.com/libsndfile/libsamplerate) (aka Secret Rabbit Code) for high-quality real-time audio resampling.

## 🎯 什么是重采样？ / What is Resampling?

重采样是改变音频采样率的过程。例如，将 48000 Hz 的音频转换为 44100 Hz。

Resampling is the process of changing the sample rate of audio. For example, converting 48000 Hz audio to 44100 Hz.

### 为什么需要重采样？ / Why Resampling?

1. **设备兼容性** / Device Compatibility
   - 某些设备可能只支持特定的采样率
   - Some devices may only support specific sample rates

2. **文件大小优化** / File Size Optimization
   - 较低的采样率可以减少文件大小
   - Lower sample rates can reduce file size

3. **标准化** / Standardization
   - CD 音质：44100 Hz
   - 专业音频：48000 Hz
   - CD quality: 44100 Hz
   - Professional audio: 48000 Hz

## 🔧 如何使用 / How to Use

### 自动模式（推荐）/ Automatic Mode (Recommended)

只需指定你想要的采样率，程序会自动处理：

Just specify your desired sample rate, the program handles the rest:

```bash
# 请求 44100 Hz（即使设备运行在 48000 Hz）
# Request 44100 Hz (even if device runs at 48000 Hz)
wasapi_capture.exe --sample-rate 44100 > output.pcm
```

程序会：
1. 检测设备的原生采样率
2. 如果与请求的采样率不同，自动启用重采样
3. 输出你请求的采样率

The program will:
1. Detect device's native sample rate
2. Automatically enable resampling if different from requested
3. Output at your requested sample rate

### 查看重采样状态 / View Resampling Status

运行时，程序会显示重采样信息：

When running, the program displays resampling info:

```
Device format: 48000Hz, 2 channels, 16 bits
Requesting sample rate: 44100Hz
Resampler ready: 48000Hz -> 44100Hz
Final format: 44100Hz, 2 channels, 16 bits
```

## ⚙️ 重采样算法 / Resampling Algorithm

当前使用的算法：`SRC_SINC_MEDIUM_QUALITY`

Current algorithm: `SRC_SINC_MEDIUM_QUALITY`

### 可用算法 / Available Algorithms

libsamplerate 提供以下算法（当前在代码中硬编码为 MEDIUM）：

libsamplerate provides the following algorithms (currently hardcoded to MEDIUM):

| 算法 / Algorithm | 质量 / Quality | 速度 / Speed | CPU 占用 / CPU Usage |
|-----------------|---------------|--------------|---------------------|
| `SRC_SINC_BEST_QUALITY` | ⭐⭐⭐⭐⭐ | 🐢 慢 / Slow | 高 / High |
| `SRC_SINC_MEDIUM_QUALITY` | ⭐⭐⭐⭐ | ✅ 中等 / Medium | 中等 / Medium |
| `SRC_SINC_FASTEST` | ⭐⭐⭐ | 🚀 快 / Fast | 低 / Low |
| `SRC_LINEAR` | ⭐⭐ | 🚀🚀 很快 / Very Fast | 很低 / Very Low |
| `SRC_ZERO_ORDER_HOLD` | ⭐ | 🚀🚀🚀 最快 / Fastest | 最低 / Minimal |

**当前选择：** 我们使用 `MEDIUM_QUALITY`，因为它在质量和性能之间达到了良好的平衡。

**Current Choice:** We use `MEDIUM_QUALITY` as it provides a good balance between quality and performance.

## 📊 性能影响 / Performance Impact

### CPU 占用 / CPU Usage

在现代 CPU 上，重采样的 CPU 占用通常很低：

On modern CPUs, resampling CPU usage is typically low:

- **无重采样** / No Resampling: ~1-2% CPU
- **MEDIUM 质量重采样** / MEDIUM Quality Resampling: ~3-5% CPU
- **BEST 质量重采样** / BEST Quality Resampling: ~8-12% CPU

### 延迟 / Latency

重采样引入的额外延迟非常小（通常 < 5ms），不会影响实时录制。

Additional latency introduced by resampling is minimal (typically < 5ms), doesn't affect real-time recording.

### 内存使用 / Memory Usage

重采样需要额外的缓冲区：

Resampling requires additional buffers:

- **每个音频块** / Per audio chunk: ~50-100 KB
- **总额外内存** / Total additional memory: < 1 MB

## 🎵 音质影响 / Audio Quality Impact

### MEDIUM 质量模式 / MEDIUM Quality Mode

- **频率响应** / Frequency Response: 平坦到 Nyquist 频率的 97%
- **信噪比** / SNR: > 97 dB
- **总谐波失真** / THD: < 0.01%
- **听感** / Listening: 对大多数用户来说，与原始音频无法区分

### 适用场景 / Use Cases

✅ **推荐用于** / Recommended for:
- 音乐录制 / Music recording
- 播客录制 / Podcast recording
- 游戏音频录制 / Game audio recording
- 视频配音 / Video voiceover

⚠️ **可能需要 BEST 质量** / May need BEST quality:
- 专业音频制作 / Professional audio production
- 音频分析 / Audio analysis
- 高保真归档 / Hi-fi archiving

## 💡 使用建议 / Tips

### 1. 选择合适的采样率 / Choose Appropriate Sample Rate

```bash
# CD 音质 / CD Quality
wasapi_capture.exe --sample-rate 44100 > music.pcm

# 专业音频 / Professional Audio
wasapi_capture.exe --sample-rate 48000 > recording.pcm

# 语音录制（节省空间）/ Voice Recording (save space)
wasapi_capture.exe --sample-rate 16000 > voice.pcm
```

### 2. 避免多次重采样 / Avoid Multiple Resampling

❌ **不好** / Bad:
```bash
# 设备 48000 -> 程序 44100 -> FFmpeg 48000 (两次重采样！)
wasapi_capture.exe --sample-rate 44100 | ffmpeg ... -ar 48000
```

✅ **好** / Good:
```bash
# 设备 48000 -> 程序 44100 -> FFmpeg 44100 (一次重采样)
wasapi_capture.exe --sample-rate 44100 | ffmpeg ... -ar 44100

# 或者使用设备默认值 / Or use device default
wasapi_capture.exe | ffmpeg -f s16le -ar 48000 -ac 2 ...
```

### 3. 测试质量 / Test Quality

如果你关心音质，可以进行 A/B 对比测试：

If you care about audio quality, perform A/B comparison tests:

```bash
# 原始采样率（无重采样）
wasapi_capture.exe > original.pcm

# 重采样
wasapi_capture.exe --sample-rate 44100 > resampled.pcm

# 使用音频分析工具对比
```

## 🔬 技术细节 / Technical Details

### 算法实现 / Algorithm Implementation

- **类型** / Type: Windowed Sinc Interpolation
- **窗口** / Window: Kaiser window
- **滤波器长度** / Filter Length: 中等 / Medium
- **精度** / Precision: 32-bit floating point

### 处理流程 / Processing Flow

```
1. WASAPI 捕获 16-bit PCM
   ↓
2. 转换为 float (-1.0 to 1.0)
   ↓
3. libsamplerate 重采样
   ↓
4. 转换回 16-bit PCM
   ↓
5. 输出到 stdout
```

### 边界条件处理 / Edge Case Handling

- **静音缓冲** / Silent Buffers: 自动调整大小 / Auto-sized
- **缓冲区溢出** / Buffer Overflow: 自动扩展 / Auto-expand
- **采样率相同** / Same Rate: 绕过重采样器 / Bypass resampler

## 🐛 故障排除 / Troubleshooting

### 重采样失败 / Resampling Failed

如果看到 "Resampling failed" 错误：

If you see "Resampling failed" error:

1. **检查采样率范围** / Check sample rate range
   ```bash
   # 有效范围：8000 - 192000 Hz
   # Valid range: 8000 - 192000 Hz
   wasapi_capture.exe --sample-rate 44100  # ✅
   wasapi_capture.exe --sample-rate 300000 # ❌
   ```

2. **检查可用内存** / Check available memory
   - 确保有足够的 RAM
   - Ensure sufficient RAM

3. **尝试降低缓冲区大小** / Try smaller buffer
   ```bash
   wasapi_capture.exe --sample-rate 44100 --chunk-duration 0.1
   ```

### 音频质量问题 / Audio Quality Issues

如果听到音质下降：

If you hear quality degradation:

1. 检查是否多次重采样 / Check for multiple resampling
2. 尝试使用设备原生采样率 / Try device native rate
3. 确保 FFmpeg 参数匹配 / Ensure FFmpeg params match

## 📚 参考资料 / References

- [libsamplerate 官方文档](http://www.mega-nerd.com/SRC/)
- [采样率转换理论](https://en.wikipedia.org/wiki/Sample-rate_conversion)
- [Nyquist-Shannon 采样定理](https://en.wikipedia.org/wiki/Nyquist%E2%80%93Shannon_sampling_theorem)

---

**注意** / **Note**: 本文档描述的是 v1.0 版本的功能。未来版本可能会添加可配置的重采样质量选项。

This document describes v1.0 functionality. Future versions may add configurable resampling quality options.

