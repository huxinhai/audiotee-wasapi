# 项目架构 / Project Architecture

## 概述 / Overview

本项目采用**三层架构**设计，将代码按照职责和抽象层次分为三个层次：

This project uses a **three-layer architecture** design, organizing code into three layers based on responsibility and abstraction level:

1. **应用层 (Application Layer)** - 用户交互和程序入口
2. **核心层 (Core Layer)** - 业务逻辑和核心功能
3. **工具层 (Utility Layer)** - 通用工具和基础设施

---

## 层次结构 / Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│                      (应用层)                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  src/main.cpp                                         │  │
│  │  - 命令行参数解析 / CLI parsing                        │  │
│  │  - 用户输入验证 / Input validation                     │  │
│  │  - 程序生命周期管理 / Lifecycle management             │  │
│  │  - 信号处理 / Signal handling                         │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓ depends on
┌─────────────────────────────────────────────────────────────┐
│                       Core Layer                            │
│                       (核心层)                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  src/core/                                            │  │
│  │                                                       │  │
│  │  WASAPICapture (wasapi_capture.h/cpp)                │  │
│  │  ├─ 音频设备管理 / Audio device management            │  │
│  │  ├─ WASAPI 捕获 / WASAPI capture                      │  │
│  │  ├─ 事件驱动模式 / Event-driven mode                  │  │
│  │  └─ 格式协调 / Format coordination                    │  │
│  │                                                       │  │
│  │  AudioResampler (audio_resampler.h/cpp)              │  │
│  │  ├─ 采样率转换 / Sample rate conversion               │  │
│  │  ├─ 声道转换 / Channel conversion                     │  │
│  │  ├─ 位深转换 / Bit depth conversion                   │  │
│  │  └─ 缓冲管理 / Buffer management                      │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓ depends on
┌─────────────────────────────────────────────────────────────┐
│                      Utility Layer                          │
│                      (工具层)                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  src/utils/                                           │  │
│  │                                                       │  │
│  │  ErrorHandler (error_handler.h/cpp)                  │  │
│  │  ├─ HRESULT 错误解析 / HRESULT error parsing          │  │
│  │  ├─ 解决方案建议 / Solution suggestions               │  │
│  │  └─ 系统检查 / System checks                          │  │
│  │                                                       │  │
│  │  Common (common.h)                                    │  │
│  │  ├─ 错误码定义 / Error code definitions               │  │
│  │  ├─ SafeRelease 模板 / SafeRelease template          │  │
│  │  └─ 公共类型 / Common types                           │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓ depends on
                    ┌───────────────────┐
                    │  Windows APIs     │
                    │  - WASAPI         │
                    │  - Media Foundation│
                    │  - COM            │
                    └───────────────────┘
```

---

## 各层详细说明 / Layer Details

### 1. 应用层 / Application Layer

**位置 / Location:** `src/main.cpp`

**职责 / Responsibilities:**
- 🎯 程序入口点 / Program entry point
- 🎯 命令行参数解析 / Command-line argument parsing
- 🎯 用户输入验证 / User input validation
- 🎯 错误消息展示 / Error message display
- 🎯 Ctrl+C 信号处理 / Ctrl+C signal handling

**依赖 / Dependencies:**
- `core/wasapi_capture.h`
- `utils/error_handler.h`

**特点 / Characteristics:**
- ✅ 不包含业务逻辑
- ✅ 只负责用户交互
- ✅ 可以轻松替换为 GUI 或 Web 界面

**示例代码 / Example Code:**
```cpp
int main(int argc, char* argv[]) {
    WASAPICapture capture;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (arg == "--sample-rate") {
            capture.SetSampleRate(std::stoi(argv[++i]));
        }
    }
    
    // Initialize and start
    if (capture.Initialize() == ErrorCode::SUCCESS) {
        capture.StartCapture();
    }
    
    return 0;
}
```

---

### 2. 核心层 / Core Layer

**位置 / Location:** `src/core/`

**模块 / Modules:**

#### 2.1 WASAPICapture
**文件 / Files:** `wasapi_capture.h`, `wasapi_capture_impl.cpp`

**职责 / Responsibilities:**
- 🎯 管理 WASAPI 音频设备
- 🎯 配置音频格式
- 🎯 执行音频捕获（事件驱动 + 轮询模式）
- 🎯 协调格式转换
- 🎯 处理音频数据流

**关键方法 / Key Methods:**
```cpp
class WASAPICapture {
public:
    ErrorCode Initialize();           // 初始化设备
    ErrorCode StartCapture();         // 开始捕获（事件驱动）
    ErrorCode StartCapturePolling();  // 开始捕获（轮询）
    void Stop();                      // 停止捕获
    void Cleanup();                   // 清理资源
    
    // Configuration
    void SetSampleRate(int rate);
    void SetChannels(int ch);
    void SetBitDepth(int bits);
};
```

**依赖 / Dependencies:**
- `utils/common.h`
- `core/audio_resampler.h`
- `utils/error_handler.h` (仅在实现文件中)

---

#### 2.2 AudioResampler
**文件 / Files:** `audio_resampler.h`, `audio_resampler.cpp`

**职责 / Responsibilities:**
- 🎯 音频采样率转换（8kHz - 192kHz）
- 🎯 声道数转换（1-2 声道）
- 🎯 位深转换（16/24/32 bits）
- 🎯 管理 Media Foundation Transform (MFT)
- 🎯 处理音频缓冲

**关键方法 / Key Methods:**
```cpp
class AudioResampler {
public:
    bool Initialize(WAVEFORMATEX* input, WAVEFORMATEX* output);
    bool ProcessAudio(const BYTE* input, UINT32 size, 
                     std::vector<BYTE>& output);
    void Flush(std::vector<BYTE>& output);
    void Cleanup();
};
```

**技术实现 / Technical Implementation:**
- 使用 Windows Media Foundation (MF)
- IMFTransform 接口
- 智能缓冲策略

**依赖 / Dependencies:**
- `utils/common.h`
- Media Foundation APIs

---

### 3. 工具层 / Utility Layer

**位置 / Location:** `src/utils/`

**模块 / Modules:**

#### 3.1 ErrorHandler
**文件 / Files:** `error_handler.h`, `error_handler.cpp`

**职责 / Responsibilities:**
- 🎯 解析 HRESULT 错误码
- 🎯 提供详细的错误说明
- 🎯 给出针对性的解决方案
- 🎯 检查系统需求

**关键方法 / Key Methods:**
```cpp
class ErrorHandler {
public:
    static void PrintDetailedError(HRESULT hr, const char* context);
    static void CheckSystemRequirements();
};
```

**特点 / Characteristics:**
- ✅ 静态类，无需实例化
- ✅ 可在任何 Windows 项目中重用
- ✅ 支持 30+ 种常见错误

---

#### 3.2 Common
**文件 / Files:** `common.h`

**职责 / Responsibilities:**
- 🎯 定义全局错误码
- 🎯 提供 COM 对象释放工具
- 🎯 包含通用头文件

**内容 / Contents:**
```cpp
// Error codes
enum class ErrorCode {
    SUCCESS = 0,
    COM_INIT_FAILED = 1,
    NO_AUDIO_DEVICE = 2,
    // ...
};

// Safe release template
template <class T> void SafeRelease(T** ppT);
```

**特点 / Characteristics:**
- ✅ 只包含声明，无实现
- ✅ 被所有其他模块依赖
- ✅ 最小化依赖

---

## 依赖规则 / Dependency Rules

### ✅ 允许的依赖 / Allowed Dependencies

```
Application Layer ──> Core Layer
Application Layer ──> Utility Layer
Core Layer ──> Utility Layer
```

### ❌ 禁止的依赖 / Forbidden Dependencies

```
Utility Layer ──X──> Core Layer
Utility Layer ──X──> Application Layer
Core Layer ──X──> Application Layer
```

### 原则 / Principles

1. **单向依赖 / Unidirectional Dependency**
   - 只能从上层依赖下层
   - 禁止循环依赖

2. **层次隔离 / Layer Isolation**
   - 每层只知道下一层的接口
   - 不知道上层的存在

3. **接口稳定 / Stable Interfaces**
   - 下层接口变化影响上层
   - 上层变化不影响下层

---

## 文件组织原则 / File Organization Principles

### 1. 头文件与实现分离 / Header-Implementation Separation

```cpp
// .h 文件：只包含声明
class MyClass {
public:
    void DoSomething();
private:
    int data;
};

// .cpp 文件：包含实现
void MyClass::DoSomething() {
    // implementation
}
```

### 2. 最小化头文件依赖 / Minimize Header Dependencies

```cpp
// ✅ Good: 前向声明
class AudioResampler;  // Forward declaration

class WASAPICapture {
    std::unique_ptr<AudioResampler> resampler;
};

// ❌ Bad: 不必要的包含
#include "audio_resampler.h"  // Not needed in header
```

### 3. 包含路径规范 / Include Path Convention

```cpp
// 同层文件：相对路径
#include "audio_resampler.h"

// 下层文件：相对路径指向下层
#include "../utils/common.h"

// 系统文件：尖括号
#include <windows.h>
```

---

## 扩展指南 / Extension Guide

### 添加新的核心功能 / Adding New Core Feature

1. 在 `src/core/` 创建 `.h` 和 `.cpp` 文件
2. 只依赖 `utils/` 层
3. 在 `CMakeLists.txt` 添加源文件
4. 更新文档

**示例 / Example:**
```cpp
// src/core/audio_encoder.h
#pragma once
#include "../utils/common.h"

class AudioEncoder {
public:
    bool Initialize();
    bool Encode(const BYTE* input, size_t size);
};
```

### 添加新的工具模块 / Adding New Utility Module

1. 在 `src/utils/` 创建 `.h` 和 `.cpp` 文件
2. 只依赖系统 API，不依赖其他层
3. 设计为可重用的工具类
4. 在 `CMakeLists.txt` 添加源文件

**示例 / Example:**
```cpp
// src/utils/logger.h
#pragma once

class Logger {
public:
    static void Info(const char* msg);
    static void Error(const char* msg);
};
```

---

## 测试策略 / Testing Strategy

### 单元测试 / Unit Tests

```
tests/
├── core/
│   ├── audio_resampler_test.cpp
│   └── wasapi_capture_test.cpp
└── utils/
    └── error_handler_test.cpp
```

### 测试原则 / Testing Principles

1. **工具层测试 / Utility Layer Tests**
   - 最容易测试
   - 无外部依赖
   - 纯逻辑测试

2. **核心层测试 / Core Layer Tests**
   - 需要 Mock 工具层
   - 可能需要 Mock Windows API

3. **应用层测试 / Application Layer Tests**
   - 集成测试
   - 端到端测试

---

## 性能考虑 / Performance Considerations

### 编译性能 / Compile Performance

- ✅ 修改工具层：只重新编译工具层和依赖它的上层
- ✅ 修改核心层：只重新编译核心层和应用层
- ✅ 修改应用层：只重新编译应用层

### 运行时性能 / Runtime Performance

- ✅ 层次结构不影响运行时性能
- ✅ 编译器会进行内联优化
- ✅ 无额外的虚函数调用开销

---

## 最佳实践 / Best Practices

### ✅ 推荐 / Recommended

1. **保持层次清晰**
   - 新功能放在合适的层
   - 不要跨层直接调用

2. **接口优先**
   - 先设计接口（.h）
   - 再实现功能（.cpp）

3. **最小化依赖**
   - 只包含必要的头文件
   - 使用前向声明

4. **文档同步**
   - 代码变化时更新文档
   - 保持架构图最新

### ❌ 避免 / Avoid

1. **循环依赖**
   - A 依赖 B，B 依赖 A

2. **上层依赖**
   - 工具层依赖核心层

3. **过度抽象**
   - 不要为了分层而分层

4. **头文件污染**
   - 不要在头文件中包含大量其他头文件

---

## 总结 / Summary

### 架构优势 / Architecture Benefits

✅ **清晰的职责分离**  
每层有明确的职责，易于理解和维护

✅ **良好的可测试性**  
每层可以独立测试

✅ **高度可重用**  
工具层可以在其他项目中使用

✅ **易于扩展**  
添加新功能时知道放在哪一层

✅ **团队协作友好**  
不同层可以并行开发

### 关键指标 / Key Metrics

| 指标 | 值 | 说明 |
|------|-----|------|
| 层数 | 3 | 应用层、核心层、工具层 |
| 文件数 | 8 | 清晰的模块划分 |
| 平均文件大小 | ~160 行 | 易于阅读和维护 |
| 循环依赖 | 0 | 完全无循环依赖 |
| 可重用模块 | 2 | ErrorHandler, Common |

---

## 参考资料 / References

- [CODE_STRUCTURE.md](CODE_STRUCTURE.md) - 详细的代码结构说明
- [REFACTORING_SUMMARY.md](../REFACTORING_SUMMARY.md) - 重构总结
- [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) - 架构设计原则

---

**最后更新 / Last Updated:** 2025-10-18  
**版本 / Version:** 1.0

