# 音频格式转换说明

## 概述

本项目支持完整的音频格式转换，包括：
- ✅ **采样率转换** (8kHz - 192kHz)
- ✅ **声道转换** (单声道 ↔ 立体声)
- ✅ **位深度转换** (16-bit ↔ 32-bit float)

## 转换流程

### 完整的三步转换流程

```
输入音频 (设备格式)
    ↓
【第一步：位深度转换】
    Float 32-bit → 16-bit 或 32-bit PCM
    或
    16-bit → 32-bit Float
    ↓
【第二步：声道转换】
    立体声 → 单声道 (混音)
    或
    单声道 → 立体声 (复制)
    ↓
【第三步：采样率转换】
    使用 libsamplerate (SRC_SINC_BEST_QUALITY)
    支持任意采样率互转
    ↓
输出音频 (用户指定格式)
```

## 使用的库

### 1. libsamplerate
- **用途**: 采样率转换
- **大小**: ~200 KB
- **质量**: 专业级，SRC_SINC_BEST_QUALITY 算法
- **位置**: `third_party/libsamplerate/`

### 2. AudioFormatConverter (自实现)
- **用途**: 位深度和声道转换
- **大小**: 包含在主程序中
- **支持格式**:
  - 16-bit PCM (int16_t)
  - 32-bit IEEE Float

### 3. AudioResampler (封装类)
- **用途**: 封装 libsamplerate，支持不同位深度
- **功能**: 
  - 自动处理 16-bit 和 32-bit 输入
  - 保持位深度一致性
  - 自动缓冲管理

## 支持的转换

### 采样率
```bash
范围: 8,000 Hz - 192,000 Hz
常用值:
  - 8000   # 电话质量
  - 16000  # 宽带语音
  - 22050  # FM 广播
  - 44100  # CD 质量
  - 48000  # 专业音频/视频
  - 96000  # 高保真
  - 192000 # 超高保真
```

### 声道数
```bash
支持: 1 (单声道) 或 2 (立体声)

转换逻辑:
  立体声 → 单声道: (L + R) / 2
  单声道 → 立体声: L = R = Mono
```

### 位深度
```bash
支持: 16-bit 或 32-bit float

转换逻辑:
  16-bit → Float: sample / 32768.0
  Float → 16-bit: clamp(sample * 32768.0, -32768, 32767)
  
注意: 使用舍入而非截断，减少量化噪声
```

## 命令行示例

### 基本转换
```bash
# 转换为 16kHz 单声道 16-bit (语音识别)
wasapi_capture --sample-rate 16000 --channels 1 --bits-per-sample 16

# 转换为 48kHz 立体声 32-bit (专业录音)
wasapi_capture --sample-rate 48000 --channels 2 --bits-per-sample 32

# 只转换采样率，保持其他参数
wasapi_capture --sample-rate 44100
```

### 完整转换示例
```bash
# 例1: 设备输出 48kHz 立体声 32-bit float
#      转换为 16kHz 单声道 16-bit
wasapi_capture --sample-rate 16000 --channels 1 --bits-per-sample 16

转换流程:
  输入:  48000 Hz, 2 channels, 32 bits float
  ↓ [位深度] 32-bit float → 16-bit PCM
  → 中间: 48000 Hz, 2 channels, 16 bits
  ↓ [声道] 立体声 → 单声道
  → 中间: 48000 Hz, 1 channel, 16 bits
  ↓ [采样率] 48000 Hz → 16000 Hz
  输出:  16000 Hz, 1 channel, 16 bits
```

## 转换质量

### libsamplerate 质量级别
```cpp
SRC_SINC_BEST_QUALITY    // 使用中 - 最高质量
SRC_SINC_MEDIUM_QUALITY  // 未使用 - 中等质量
SRC_SINC_FASTEST         // 未使用 - 最快速度
```

当前使用 **SRC_SINC_BEST_QUALITY**，适合：
- ✅ 所有采样率转换
- ✅ 大比率转换 (如 48kHz → 8kHz)
- ✅ 音乐和语音
- ⚠️ 可能需要更多 CPU

### 音质保证
- **抗混叠**: libsamplerate 自动处理
- **量化噪声**: 使用舍入而非截断
- **限幅保护**: 防止数值溢出
- **缓冲管理**: 自动处理边界情况

## 性能考虑

### 转换顺序优化
当前顺序: **位深度 → 声道 → 采样率**

原因:
1. 位深度转换最快，先做
2. 声道转换可减少采样率转换的数据量
3. 采样率转换最耗时，放最后

### 内存使用
```
单个音频块 (0.2秒, 48kHz, 2声道, 32-bit):
  输入: 48000 * 0.2 * 2 * 4 = 76,800 bytes (~75 KB)
  
转换缓冲:
  - 位深度转换: 可能增加/减少 50%
  - 声道转换: 可能减少 50%
  - 采样率转换: 根据比率变化
  
总内存: 通常 < 1 MB per chunk
```

## 故障排除

### 问题1: 音质差/有杂音
**可能原因:**
- 采样率比率过大 (>10x 或 <0.1x)
- 位深度损失 (32-bit → 16-bit)

**解决方案:**
```bash
# 检查转换比率警告
Resampler initialized successfully:
  ...
  Warning: Large conversion ratio may affect quality  # 注意这个警告
```

### 问题2: 转换失败
**检查:**
1. 参数是否在支持范围内
2. 查看错误消息
3. 确认 libsamplerate 已正确安装

### 问题3: 输出无声或失真
**可能原因:**
- 位深度转换问题
- 限幅过度

**调试:**
```bash
# 查看转换日志
Resampler initialized successfully:
  Input:  48000 Hz, 2 channels, 32 bits
  Output: 16000 Hz, 1 channel, 16 bits
  Ratio:  0.333333 (using SRC_SINC_BEST_QUALITY)

First resampling:
  Input frames:  9600 -> 9600 used
  Output frames: 3216 available -> 3200 generated
  Ratio: 0.333333
```

## 技术细节

### 数据类型对应
```cpp
16-bit PCM:   int16_t    范围: [-32768, 32767]
32-bit Float: float      范围: [-1.0, 1.0]
```

### 转换精度
```cpp
// 16-bit → Float
float normalized = int16_value * (1.0f / 32768.0f);

// Float → 16-bit (带舍入)
float scaled = float_value * 32768.0f;
if (scaled > 32767.0f) scaled = 32767.0f;
if (scaled < -32768.0f) scaled = -32768.0f;
int16_t result = (scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f);
```

### 声道混音
```cpp
// 立体声 → 单声道
mono = (left + right) / 2;  // 简单平均

// 单声道 → 立体声
left = right = mono;  // 简单复制
```

## 代码示例

### 使用 AudioResampler
```cpp
AudioResampler resampler;

// 初始化: 48kHz → 16kHz, 2声道, 16-bit
resampler.Initialize(48000, 16000, 2, 16);

// 处理数据
std::vector<BYTE> outputData;
if (resampler.Process(inputData, inputFrames, outputData)) {
    // 输出已转换的数据
    std::cout.write((char*)outputData.data(), outputData.size());
}
```

### 使用 AudioFormatConverter
```cpp
// Float → 16-bit
std::vector<BYTE> outputData;
AudioFormatConverter::FloatToPCM(floatData, numSamples, 16, outputData);

// 声道转换
std::vector<BYTE> monoData;
AudioFormatConverter::ConvertChannels(
    stereoData, frames, 
    2, 1,  // 立体声 → 单声道
    16,    // 16-bit
    monoData
);
```

## 相关文档
- [快速开始](QUICK_START.md)
- [重采样详解](RESAMPLING.md)
- [故障排除](TROUBLESHOOTING.md)

