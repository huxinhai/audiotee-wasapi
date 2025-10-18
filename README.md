# WASAPI Audio Capture

[中文文档](README_CN.md) | English

A Windows system audio capture tool based on WASAPI (Windows Audio Session API) that can record all audio currently playing on Windows (Loopback Recording).

## Features

- ✅ Capture system audio (all playing sounds)
- ✅ **Real-time audio format conversion** (sample rate, channels, bit depth)
- ✅ Support custom sample rates (8000 - 192000 Hz)
- ✅ Support custom channels (1=mono, 2=stereo)
- ✅ Support custom bit depth (16/24/32 bits)
- ✅ Built-in Windows Media Foundation resampler (high quality)
- ✅ Support custom buffer sizes
- ✅ Event-driven mode (no frame drops) and polling mode
- ✅ Output raw PCM audio data to stdout
- ✅ Detailed error diagnostics and solutions
- ✅ Support Ctrl+C graceful exit
- 📝 Mute mode (not yet implemented)
- 📝 Process filtering (not yet implemented)

## System Requirements

- **Operating System**: Windows Vista or later (Windows 10/11 recommended)
- **Compiler**: 
  - Visual Studio 2017 or later (Visual Studio 2022 recommended)
  - Or any MSVC compiler with C++17 support
- **Build Tools**: 
  - CMake 3.15 or later (Method 1)
  - Visual Studio Developer Command Prompt (Method 2)

## Download Pre-built Binary

You can download the latest pre-built executable from [GitHub Releases](https://github.com/huxinhai/audiotee-wasapi/releases).

No compilation needed - just download `wasapi_capture.exe` and run it!

## Build Instructions

### Method 1: Using CMake + Visual Studio (Recommended)

This is the recommended build method for most users:

```batch
# Double-click build.bat in project root, or run in command line:
build.bat

# Or run the script in scripts folder directly:
scripts\build.bat
```

After compilation, the executable will be located at:
```
build\bin\Release\wasapi_capture.exe
```

**Detailed Steps:**
1. Ensure Visual Studio 2022 is installed (with C++ desktop development workload)
2. Ensure CMake is installed (download from https://cmake.org/)
3. Double-click `build.bat` in the project root, or run `scripts\build.bat`
4. The build process will automatically create the `build` directory and generate project files
5. The final executable will be at `build\bin\Release\wasapi_capture.exe`

### Method 2: Direct Compilation with cl.exe (Quick)

If you're familiar with Visual Studio Developer Command Prompt, use this method for quick compilation:

```batch
# 1. Open "x64 Native Tools Command Prompt for VS 2022"
#    (Search for "x64 Native Tools" in Start Menu)

# 2. Navigate to project directory
cd /d "project_path"

# 3. Run the build script
scripts\build_simple.bat
```

After compilation, the executable will be located at:
```
wasapi_capture.exe (in project root directory)
```

**Detailed Steps:**
1. Open Start Menu and search for "x64 Native Tools Command Prompt"
2. Enter Visual Studio Developer Command Prompt
3. Use `cd` command to navigate to project directory
4. Run `scripts\build_simple.bat`
5. After compilation, `wasapi_capture.exe` will be generated in the project root directory

### Manual Compilation (Advanced Users)

If you need custom build options:

```batch
# Using CMake
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Or using cl.exe directly
cl.exe /EHsc /O2 /std:c++17 src\wasapi_capture.cpp ole32.lib psapi.lib /Fe:wasapi_capture.exe
```

## Usage

### Basic Usage

```batch
# Capture system audio and save as raw PCM file
wasapi_capture.exe > output.pcm

# Capture system audio and pipe to FFmpeg to convert to MP3
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 output.mp3
```

### Command Line Options

| Option | Description | Example |
|--------|-------------|---------|
| `--sample-rate <Hz>` | Set target sample rate (8000-192000 Hz, default: device default) | `--sample-rate 16000` |
| `--channels <count>` | Set number of channels (1=mono, 2=stereo, default: device default) | `--channels 1` |
| `--bit-depth <bits>` | Set bit depth (16/24/32, default: device default) | `--bit-depth 16` |
| `--chunk-duration <seconds>` | Set audio chunk duration (default: 0.2 seconds) | `--chunk-duration 0.1` |
| `--mute` | Mute system audio while capturing (not yet implemented) | `--mute` |
| `--include-processes <PID>` | Only capture audio from specified process IDs (not yet implemented) | `--include-processes 1234 5678` |
| `--exclude-processes <PID>` | Exclude audio from specified process IDs (not yet implemented) | `--exclude-processes 1234` |
| `--help` or `-h` | Show help message | `--help` |

### Usage Examples

#### Example 1: Basic Recording (Save as Raw PCM)
```batch
wasapi_capture.exe > music.pcm
```
Press `Ctrl+C` to stop recording.

#### Example 2: Convert to WAV Format Using FFmpeg
```batch
# Assuming device sample rate is 48000 Hz, 2 channels, 16 bits
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 output.wav
```

#### Example 3: Real-time Streaming Conversion to MP3
```batch
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 -b:a 192k output.mp3
```

#### Example 4: Convert Audio Format (Resample + Channel Conversion)
```batch
# Convert to 16kHz mono 16-bit (ideal for speech recognition)
wasapi_capture.exe --sample-rate 16000 --channels 1 --bit-depth 16 > speech.pcm

# Convert to 44.1kHz stereo 16-bit (CD quality)
wasapi_capture.exe --sample-rate 44100 --channels 2 --bit-depth 16 > music.pcm

# Convert to 8kHz mono (telephone quality, smallest file size)
wasapi_capture.exe --sample-rate 8000 --channels 1 --bit-depth 16 > phone.pcm
```

#### Example 5: Speech Recognition Pipeline
```batch
# Capture audio in optimal format for speech recognition and pipe to FFmpeg
wasapi_capture.exe --sample-rate 16000 --channels 1 --bit-depth 16 2>nul | ffmpeg -f s16le -ar 16000 -ac 1 -i pipe:0 speech.wav
```

#### Example 6: Adjust Buffer Size
```batch
# Use smaller buffer (lower latency)
wasapi_capture.exe --chunk-duration 0.05 > output.pcm

# Use larger buffer (more stable)
wasapi_capture.exe --chunk-duration 0.5 > output.pcm
```

#### Example 7: Silence Error Messages and Convert to FLAC
```batch
wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 -c:a flac output.flac
```

### About Audio Format

The program outputs **raw PCM audio data** (no file header). The output format can be customized using command-line options:

**Default Behavior** (no format options specified):
- Uses device's native format (typically 48000 Hz, 2 channels, 32-bit float)
- Automatically converted to 16-bit PCM integer format

**Custom Format** (with `--sample-rate`, `--channels`, `--bit-depth`):
- **Sample Rate**: 8000-192000 Hz (e.g., 16000 for speech, 44100 for music)
- **Channels**: 1 (mono) or 2 (stereo)
- **Bit Depth**: 16/24/32 bits (16-bit recommended for compatibility)

**Format Conversion:**
The program uses Windows Media Foundation's built-in resampler for high-quality real-time audio conversion:
- ✅ Automatic format detection (PCM/Float)
- ✅ Sample rate conversion (e.g., 48000→16000 Hz)
- ✅ Channel conversion (e.g., stereo→mono)
- ✅ Bit depth conversion (e.g., 32-bit float→16-bit PCM)

When running the program, the output format will be displayed:
```
========================================
Output Audio Format:
  Sample Rate: 16000 Hz
  Channels:    1
  Bit Depth:   16 bits
========================================
```

**FFmpeg Parameter Mapping:**
| Output Format | FFmpeg Parameters |
|---------------|-------------------|
| 16kHz, Mono, 16-bit | `-f s16le -ar 16000 -ac 1` |
| 44.1kHz, Stereo, 16-bit | `-f s16le -ar 44100 -ac 2` |
| 48kHz, Stereo, 24-bit | `-f s24le -ar 48000 -ac 2` |
| 48kHz, Stereo, 32-bit | `-f s32le -ar 48000 -ac 2` |

## Troubleshooting

### 1. Build Error: "Visual Studio not found"

**Solutions:**
- Ensure Visual Studio 2022 is installed (with "C++ Desktop Development" workload)
- If you have a different version of Visual Studio, modify the generator in `build.bat`:
  ```batch
  # Visual Studio 2019
  cmake .. -G "Visual Studio 16 2019" -A x64
  
  # Visual Studio 2017
  cmake .. -G "Visual Studio 15 2017" -A x64
  ```

### 2. Runtime Error: "No audio device found"

**Solutions:**
1. Right-click the volume icon in the taskbar
2. Select "Open Sound settings"
3. Ensure an output device is available and not disabled
4. Play some audio to test if the device is working

### 3. Runtime Error: "Access denied"

**Solutions:**
- Run as Administrator (right-click -> Run as administrator)
- Check Windows Privacy Settings -> Microphone access permissions
- Temporarily disable antivirus software for testing

### 4. Runtime Error: "Audio device is in use"

**Solutions:**
1. Close applications that exclusively use the audio device (some games, professional audio software)
2. Open "Sound Settings" -> Device properties -> Additional device properties
3. Go to "Advanced" tab
4. Uncheck "Allow applications to take exclusive control of this device"

### 5. Runtime Error: "Unsupported format"

**Solutions:**
- Don't use `--sample-rate` parameter (use device default)
- Try common sample rates: `--sample-rate 44100` or `--sample-rate 48000`
- Update audio drivers

### 6. Audio Stuttering or Frame Drops

**Solutions:**
- Increase buffer size: `--chunk-duration 0.5`
- Close other CPU-intensive programs
- Ensure sufficient available system memory

### 7. No Audio Captured (Output File is Empty or All Zeros)

**Solutions:**
- Ensure system is playing audio
- Check if system volume is at 0
- Verify default playback device settings are correct
- Try playing music and re-run the program

### 8. Format Errors When Using with FFmpeg

**Solutions:**
- Run the program to view the actual audio format (output to stderr)
- Adjust FFmpeg parameters based on the actual format:
  ```batch
  # If 48000 Hz, 2 channels, 16 bits
  wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 48000 -ac 2 -i pipe:0 output.mp3
  
  # If 44100 Hz
  wasapi_capture.exe 2>nul | ffmpeg -f s16le -ar 44100 -ac 2 -i pipe:0 output.mp3
  ```

### Architecture
- Uses Windows WASAPI (Windows Audio Session API) for audio capture
- Loopback mode: Captures audio currently playing on the system (render endpoint playback)
- Supports both event-driven and polling modes
  - **Event-driven mode** (preferred): No latency, no frame drops
  - **Polling mode** (fallback): Periodically checks audio buffer

### Audio Stream Processing
1. Obtain system default audio output device via WASAPI
2. Initialize audio client in Loopback mode
3. Continuously read data from audio buffer
4. Write raw PCM data to standard output (stdout)
5. Output errors and status information to standard error (stderr)

### Error Handling
The program includes a detailed error diagnostic system that provides for common WASAPI error codes:
- Error cause explanation
- Detailed solution steps
- System requirements check (Windows version, administrator privileges, etc.)

## Automated Build and Release

This project uses GitHub Actions for automated building and releasing.

### How to Create a Release

1. **Tag your version:**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. **Automatic build:** GitHub Actions will automatically:
   - Build the project using Visual Studio 2022
   - Compile the release executable
   - Create a new GitHub Release
   - Upload `wasapi_capture.exe` and documentation files

3. **Manual trigger:** You can also manually trigger the build from the "Actions" tab on GitHub.

### GitHub Actions Workflow

The workflow (`.github/workflows/build-release.yml`) includes:
- Automatic compilation on Windows
- CMake + Visual Studio 2022 build
- Artifact upload
- Automatic release creation with executable and documentation

## Developer Information

### Project Structure
```
audiotee-wasapi/
├── .github/
│   └── workflows/
│       └── build-release.yml   # GitHub Actions workflow
├── src/
│   └── wasapi_capture.cpp      # Main program source code
├── scripts/
│   ├── build.bat               # CMake build script
│   └── build_simple.bat        # cl.exe direct compilation script
├── docs/
│   ├── QUICK_START.md          # Quick start guide
│   └── RELEASE_GUIDE.md        # Release guide
├── CMakeLists.txt              # CMake build configuration
├── build.bat                   # Quick build shortcut
├── README.md                   # This file (English)
├── README_CN.md                # Chinese documentation
└── .gitignore                  # Git ignore rules
```

### Dependencies
- `ole32.lib` - COM library
- `psapi.lib` - Process Status API

### Build Requirements
- C++17 standard
- Windows SDK (includes WASAPI headers)

### Exit Codes
The program uses the following exit codes:
- `0` - Success
- `1` - COM initialization failed
- `2` - No audio device found
- `3` - Device access denied
- `4` - Unsupported audio format
- `8` - Invalid parameter
- `99` - Unknown error

## License

This project is open source and free to use and modify.

## Contributing

Issues and Pull Requests are welcome!

## Related Links

- [Windows WASAPI Official Documentation](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi)
- [FFmpeg Official Website](https://ffmpeg.org/)
- [Visual Studio Download](https://visualstudio.microsoft.com/)
- [CMake Download](https://cmake.org/download/)
- [Capture macOS System Audio Output](https://github.com/makeusabrew/audiotee)

## Changelog

### v1.0
- ✅ Implemented basic WASAPI audio capture functionality
- ✅ Support for custom sample rates and buffer sizes
- ✅ Event-driven and polling modes
- ✅ Detailed error diagnostic system
- ✅ Raw PCM output to stdout

---

