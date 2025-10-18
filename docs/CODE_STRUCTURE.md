# Code Structure

This document describes the modular structure of the WASAPI Audio Capture project.

## File Organization

```
src/
├── main.cpp                           # Main entry point and command-line parsing
├── core/                              # Core audio processing modules
│   ├── audio_resampler.h              # Audio resampler class declaration
│   ├── audio_resampler.cpp            # Audio resampler implementation
│   ├── wasapi_capture.h               # WASAPI capture class declaration
│   └── wasapi_capture_impl.cpp        # WASAPI capture implementation
└── utils/                             # Utility modules
    ├── common.h                       # Common definitions, enums, and macros
    ├── error_handler.h                # Error handling class declaration
    └── error_handler.cpp              # Error handling implementation
```

## Module Descriptions

### 1. `common.h`
**Purpose:** Shared definitions used across all modules

**Contents:**
- `SafeRelease<T>()` - Template function for releasing COM objects
- `ErrorCode` enum - Error codes for the application

**Dependencies:** None (base header)

---

### 2. `error_handler.h/cpp`
**Purpose:** Centralized error handling and system diagnostics

**Key Functions:**
- `PrintDetailedError()` - Prints detailed HRESULT error information with solutions
- `CheckSystemRequirements()` - Checks Windows version and privileges

**Dependencies:**
- `common.h`
- Windows APIs: `comdef.h`, `audioclient.h`

**Usage Example:**
```cpp
HRESULT hr = SomeOperation();
if (FAILED(hr)) {
    ErrorHandler::PrintDetailedError(hr, "Operation failed");
}
```

---

### 3. `audio_resampler.h/cpp`
**Purpose:** Audio format conversion using Windows Media Foundation

**Key Methods:**
- `Initialize()` - Set up resampler with input/output formats
- `ProcessAudio()` - Convert audio data
- `Flush()` - Drain remaining buffered data
- `Cleanup()` - Release resources

**Dependencies:**
- `common.h`
- Media Foundation APIs: `mfapi.h`, `mfidl.h`, `mfreadwrite.h`

**Features:**
- Sample rate conversion (8kHz - 192kHz)
- Channel count conversion (1-2 channels)
- Bit depth conversion (16/24/32 bits)
- Buffering management for smooth output

**Usage Example:**
```cpp
AudioResampler resampler;
if (resampler.Initialize(inputFormat, outputFormat)) {
    std::vector<BYTE> outputData;
    resampler.ProcessAudio(inputData, inputSize, outputData);
}
```

---

### 4. `wasapi_capture.h/cpp`
**Purpose:** WASAPI audio capture with loopback support

**Key Methods:**
- `Initialize()` - Set up audio device and format
- `StartCapture()` - Begin event-driven capture
- `StartCapturePolling()` - Begin polling-mode capture (fallback)
- `Stop()` - Stop capture
- `Cleanup()` - Release all resources

**Configuration Methods:**
- `SetSampleRate()` - Set target sample rate
- `SetChannels()` - Set target channel count
- `SetBitDepth()` - Set target bit depth
- `SetChunkDuration()` - Set buffer duration
- `SetMute()` - Enable/disable mute (not implemented)
- `AddIncludeProcess()` / `AddExcludeProcess()` - Process filtering (not implemented)

**Dependencies:**
- `common.h`
- `error_handler.h`
- `audio_resampler.h`
- WASAPI APIs: `mmdeviceapi.h`, `audioclient.h`

**Features:**
- Loopback audio capture (system audio)
- Event-driven mode (no frame drops)
- Polling mode fallback
- Real-time format conversion
- Detailed error reporting

---

### 5. `main.cpp`
**Purpose:** Application entry point and CLI interface

**Responsibilities:**
- Parse command-line arguments
- Validate user input
- Initialize WASAPICapture
- Handle Ctrl+C signal
- Display usage information

**Command-Line Options:**
- `--sample-rate <Hz>` - Target sample rate
- `--channels <count>` - Channel count (1=mono, 2=stereo)
- `--bit-depth <bits>` - Bit depth (16/24/32)
- `--chunk-duration <seconds>` - Buffer duration
- `--mute` - Mute system audio
- `--include-processes <pid>...` - Process filter (include)
- `--exclude-processes <pid>...` - Process filter (exclude)
- `--help` - Show help

---

## Dependency Graph

```
src/main.cpp
  │
  ├─> core/wasapi_capture.h
  │     ├─> utils/common.h
  │     ├─> core/audio_resampler.h ──> utils/common.h
  │     └─> (impl uses) utils/error_handler.h ──> utils/common.h
  │
  └─> utils/error_handler.h ──> utils/common.h
```

## Layer Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│          (src/main.cpp)             │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│          Core Layer                 │
│  (src/core/)                        │
│  - WASAPICapture                    │
│  - AudioResampler                   │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│         Utility Layer               │
│  (src/utils/)                       │
│  - ErrorHandler                     │
│  - Common definitions               │
└─────────────────────────────────────┘
```

## Build System

The project uses CMake for cross-platform build configuration.

**CMakeLists.txt:**
```cmake
add_executable(wasapi_capture
    src/main.cpp
    src/core/wasapi_capture_impl.cpp
    src/core/audio_resampler.cpp
    src/utils/error_handler.cpp
)

target_link_libraries(wasapi_capture
    ole32      # COM library
    psapi      # Process API
    mf         # Media Foundation
    mfplat     # Media Foundation Platform
    mfreadwrite # Media Foundation Read/Write
)
```

## Benefits of Modular Structure

### 1. **Maintainability** ✅
- Each module has a single responsibility
- Easy to locate and fix bugs
- Clear separation of concerns

### 2. **Reusability** ✅
- `AudioResampler` can be used in other projects
- `ErrorHandler` is project-agnostic
- Modules can be tested independently

### 3. **Compile Time** ✅
- Incremental compilation (only changed files recompile)
- Parallel compilation of .cpp files
- Faster development iteration

### 4. **Testability** ✅
- Each class can be unit tested separately
- Easy to mock dependencies
- Clear interfaces for testing

### 5. **Team Collaboration** ✅
- Multiple developers can work on different modules
- Reduced merge conflicts
- Clear ownership boundaries

## Migration from Single File

The original `wasapi_capture.cpp` (1189 lines) has been split into:

| File | Lines | Purpose | Layer |
|------|-------|---------|-------|
| `src/main.cpp` | ~220 | Entry point and CLI | Application |
| `src/core/wasapi_capture.h` | ~50 | Capture interface | Core |
| `src/core/wasapi_capture_impl.cpp` | ~520 | Capture implementation | Core |
| `src/core/audio_resampler.h` | ~30 | Resampler interface | Core |
| `src/core/audio_resampler.cpp` | ~250 | Resampler implementation | Core |
| `src/utils/common.h` | ~25 | Shared definitions | Utility |
| `src/utils/error_handler.h` | ~10 | Error handling interface | Utility |
| `src/utils/error_handler.cpp` | ~170 | Error handling implementation | Utility |
| **Total** | **~1275** | **(+86 lines for clarity)** | |

## Building the Project

### Windows (Visual Studio)
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Windows (MinGW)
```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

### Output
The executable will be located at: `build/bin/wasapi_capture.exe`

## Future Enhancements

Potential areas for further modularization:

1. **Process Filtering Module** - Implement `--include-processes` / `--exclude-processes`
2. **Audio Volume Control** - Implement `--mute` functionality
3. **Configuration File Support** - Add `.ini` or `.json` config file parsing
4. **Logging Module** - Replace `std::cerr` with structured logging
5. **Plugin System** - Allow custom audio processors

## Testing Strategy

### Unit Tests (Recommended)
- Test `AudioResampler` with known input/output
- Test `ErrorHandler` message formatting
- Test command-line parsing in `main.cpp`

### Integration Tests
- Test full capture pipeline
- Test format conversion accuracy
- Test error recovery

### Manual Testing
```cmd
# Test default capture
wasapi_capture > output.pcm

# Test format conversion
wasapi_capture --sample-rate 16000 --channels 1 --bit-depth 16 > output.pcm

# Test with FFmpeg
wasapi_capture --sample-rate 48000 --channels 2 --bit-depth 16 | ffmpeg -f s16le -ar 48000 -ac 2 -i - output.wav
```

## Contributing

When adding new features:

1. **Follow the module pattern** - Create `.h` and `.cpp` pairs
2. **Update CMakeLists.txt** - Add new source files
3. **Document dependencies** - Update this file
4. **Keep modules focused** - One responsibility per module
5. **Use `common.h`** - For shared definitions

## Questions?

For questions about the code structure, please refer to:
- Main README: `../README.md`
- Quick Start Guide: `QUICK_START.md`
- Troubleshooting: `TROUBLESHOOTING.md`

