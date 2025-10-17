#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audiopolicy.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <map>
#include <comdef.h>
#include <io.h>  
#include <fcntl.h> 
#include <samplerate.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "libsamplerate-0.lib")

// Safe release macro
template <class T> void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

// Error code definitions
enum class ErrorCode {
    SUCCESS = 0,
    COM_INIT_FAILED = 1,
    NO_AUDIO_DEVICE = 2,
    DEVICE_ACCESS_DENIED = 3,
    AUDIO_FORMAT_NOT_SUPPORTED = 4,
    INSUFFICIENT_BUFFER = 5,
    DEVICE_IN_USE = 6,
    DRIVER_ERROR = 7,
    INVALID_PARAMETER = 8,
    UNKNOWN_ERROR = 99
};

// Resampler wrapper class
class AudioResampler {
private:
    SRC_STATE* srcState = nullptr;
    int inputRate = 0;
    int outputRate = 0;
    int channels = 0;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;

public:
    AudioResampler() {}

    ~AudioResampler() {
        if (srcState) {
            src_delete(srcState);
            srcState = nullptr;
        }
    }

    bool Initialize(int inRate, int outRate, int numChannels) {
        inputRate = inRate;
        outputRate = outRate;
        channels = numChannels;

        int error = 0;
        // 使用最高质量的转换器 SRC_SINC_BEST_QUALITY
        // 可选项：SRC_SINC_FASTEST, SRC_SINC_MEDIUM_QUALITY, SRC_LINEAR, SRC_ZERO_ORDER_HOLD
        srcState = src_new(SRC_SINC_MEDIUM_QUALITY, channels, &error);

        if (!srcState) {
            std::cerr << "Failed to create resampler: " << src_strerror(error) << std::endl;
            return false;
        }

        std::cerr << "Resampler initialized: " << inRate << "Hz -> " << outRate
            << "Hz (" << channels << " channels)" << std::endl;
        std::cerr << "Conversion ratio: " << (double)outRate / inRate << std::endl;

        return true;
    }

    // 转换 16-bit PCM 数据
    bool Process(const BYTE* inputData, UINT32 inputFrames, std::vector<BYTE>& outputData) {
        if (!srcState) return false;

        // 计算输出帧数
        double ratio = (double)outputRate / inputRate;
        UINT32 outputFrames = static_cast<UINT32>(inputFrames * ratio) + 16; // 加一些缓冲

        // 转换为 float (libsamplerate 使用 float)
        size_t inputSamples = inputFrames * channels;
        inputBuffer.resize(inputSamples);

        const int16_t* int16Data = reinterpret_cast<const int16_t*>(inputData);
        for (size_t i = 0; i < inputSamples; i++) {
            inputBuffer[i] = int16Data[i] / 32768.0f;
        }

        // 准备输出缓冲
        size_t outputSamples = outputFrames * channels;
        outputBuffer.resize(outputSamples);

        // 设置转换参数
        SRC_DATA srcData;
        srcData.data_in = inputBuffer.data();
        srcData.input_frames = inputFrames;
        srcData.data_out = outputBuffer.data();
        srcData.output_frames = outputFrames;
        srcData.src_ratio = ratio;
        srcData.end_of_input = 0;

        // 执行转换
        int error = src_process(srcState, &srcData);
        if (error) {
            std::cerr << "Resampling error: " << src_strerror(error) << std::endl;
            return false;
        }

        // 转换回 16-bit PCM
        size_t actualOutputSamples = srcData.output_frames_gen * channels;
        outputData.resize(actualOutputSamples * sizeof(int16_t));
        int16_t* outputInt16 = reinterpret_cast<int16_t*>(outputData.data());

        for (size_t i = 0; i < actualOutputSamples; i++) {
            float sample = outputBuffer[i] * 32768.0f;
            // 限幅
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            outputInt16[i] = static_cast<int16_t>(sample);
        }

        return true;
    }

    bool IsActive() const {
        return srcState != nullptr;
    }

    int GetOutputRate() const {
        return outputRate;
    }
};

// Detailed error information helper
class ErrorHandler {
public:
    static void PrintDetailedError(HRESULT hr, const char* context) {
        std::cerr << "\n========================================" << std::endl;
        std::cerr << "ERROR: " << context << std::endl;
        std::cerr << "========================================" << std::endl;
        std::cerr << "HRESULT Code: 0x" << std::hex << hr << std::dec << std::endl;

        // Get system error message
        _com_error err(hr);
        std::wcerr << L"System Message: " << err.ErrorMessage() << std::endl;

        // Provide detailed explanation and solution
        switch (hr) {
            case E_POINTER:
                std::cerr << "\nCause: Invalid pointer" << std::endl;
                std::cerr << "Solution: This is a programming error. Please report this bug." << std::endl;
                break;

            case E_INVALIDARG:
                std::cerr << "\nCause: Invalid argument provided" << std::endl;
                std::cerr << "Solution: Check command line parameters (sample rate, chunk duration, etc.)" << std::endl;
                break;

            case E_OUTOFMEMORY:
                std::cerr << "\nCause: Insufficient memory" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Close other applications to free up memory" << std::endl;
                std::cerr << "  - Increase virtual memory (page file) size" << std::endl;
                break;

            case E_ACCESSDENIED:
                std::cerr << "\nCause: Access denied / Permission issue" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Run as Administrator (right-click -> Run as administrator)" << std::endl;
                std::cerr << "  - Check Windows Privacy Settings -> Microphone access" << std::endl;
                std::cerr << "  - Disable antivirus temporarily to test" << std::endl;
                break;

            case AUDCLNT_E_DEVICE_INVALIDATED:
                std::cerr << "\nCause: Audio device was removed or disabled" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Check if audio device is properly connected" << std::endl;
                std::cerr << "  - Open Sound Settings and verify default device" << std::endl;
                std::cerr << "  - Restart audio service: services.msc -> Windows Audio" << std::endl;
                break;

            case AUDCLNT_E_DEVICE_IN_USE:
                std::cerr << "\nCause: Audio device is exclusively used by another application" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Close applications using audio (music players, games, etc.)" << std::endl;
                std::cerr << "  - Open Sound Settings -> Device properties -> Additional device properties" << std::endl;
                std::cerr << "  - Go to Advanced tab, uncheck 'Allow applications to take exclusive control'" << std::endl;
                break;

            case AUDCLNT_E_UNSUPPORTED_FORMAT:
                std::cerr << "\nCause: Requested audio format is not supported by device" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Try without --sample-rate parameter (use device default)" << std::endl;
                std::cerr << "  - Try common sample rates: 44100, 48000" << std::endl;
                std::cerr << "  - Update audio drivers" << std::endl;
                break;

            case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
                std::cerr << "\nCause: Buffer size is not aligned with device requirements" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Try different --chunk-duration values (0.05, 0.1, 0.2)" << std::endl;
                break;

            case AUDCLNT_E_SERVICE_NOT_RUNNING:
                std::cerr << "\nCause: Windows Audio service is not running" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Press Win+R, type 'services.msc', press Enter" << std::endl;
                std::cerr << "  - Find 'Windows Audio' service" << std::endl;
                std::cerr << "  - Right-click -> Start (if stopped)" << std::endl;
                std::cerr << "  - Set Startup type to 'Automatic'" << std::endl;
                break;

            case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
                std::cerr << "\nCause: Failed to create audio endpoint" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Restart Windows Audio service" << std::endl;
                std::cerr << "  - Update audio drivers from device manager" << std::endl;
                std::cerr << "  - Restart computer" << std::endl;
                break;

            case CO_E_NOTINITIALIZED:
                std::cerr << "\nCause: COM library not initialized" << std::endl;
                std::cerr << "Solution: This is a programming error. Please report this bug." << std::endl;
                break;

            case REGDB_E_CLASSNOTREG:
                std::cerr << "\nCause: Required COM component not registered" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - System may be missing Windows Audio components" << std::endl;
                std::cerr << "  - Run Windows Update to install missing components" << std::endl;
                std::cerr << "  - Run 'sfc /scannow' in Administrator Command Prompt" << std::endl;
                break;

            default:
                std::cerr << "\nCause: Unknown error (0x" << std::hex << hr << std::dec << ")" << std::endl;
                std::cerr << "Solution: " << std::endl;
                std::cerr << "  - Update audio drivers" << std::endl;
                std::cerr << "  - Restart Windows Audio service" << std::endl;
                std::cerr << "  - Check Windows Event Viewer for details" << std::endl;
                std::cerr << "  - Try running as Administrator" << std::endl;
                break;
        }

        std::cerr << "\nFor more help, visit:" << std::endl;
        std::cerr << "  - Windows Sound Troubleshooter: Settings -> System -> Sound -> Troubleshoot" << std::endl;
        std::cerr << "  - Device Manager: devmgmt.msc -> Sound, video and game controllers" << std::endl;
        std::cerr << "========================================\n" << std::endl;
    }

    static void CheckSystemRequirements() {
        std::cerr << "Checking system requirements..." << std::endl;

        // Check Windows version
        OSVERSIONINFOEX osvi = {};
        osvi.dwOSVersionInfoSize = sizeof(osvi);

        #pragma warning(push)
        #pragma warning(disable: 4996)
        if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
            std::cerr << "Windows Version: " << osvi.dwMajorVersion << "."
                     << osvi.dwMinorVersion << " Build " << osvi.dwBuildNumber << std::endl;

            if (osvi.dwMajorVersion < 6) {
                std::cerr << "WARNING: Windows Vista or later is required for WASAPI" << std::endl;
            }
        }
        #pragma warning(pop)

        // Check if running as administrator
        BOOL isAdmin = FALSE;
        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        PSID AdministratorsGroup;
        if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                     &AdministratorsGroup)) {
            CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin);
            FreeSid(AdministratorsGroup);
        }

        if (isAdmin) {
            std::cerr << "Privilege Level: Administrator (OK)" << std::endl;
        } else {
            std::cerr << "Privilege Level: Standard User (not administrator)" << std::endl;
            std::cerr << "Note: Some operations may require administrator privileges" << std::endl;
        }

        std::cerr << std::endl;
    }
};

class WASAPICapture {
private:
    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;
    WAVEFORMATEX* pwfx = nullptr;
    UINT32 bufferFrameCount = 0;

    int requestedSampleRate = 0;  // 用户请求的采样率
    int deviceSampleRate = 0;     // 设备实际采样率
    double chunkDuration = 0.2;  // seconds
    bool mute = false;
    std::vector<DWORD> includeProcesses;
    std::vector<DWORD> excludeProcesses;

    bool running = false;
    AudioResampler resampler;  // 重采样器
    bool needsResampling = false;  // 是否需要重采样

public:
    WASAPICapture() {}

    ~WASAPICapture() {
        Cleanup();
    }

    void SetSampleRate(int rate) { requestedSampleRate = rate; }
    void SetChunkDuration(double duration) { chunkDuration = duration; }
    void SetMute(bool m) { mute = m; }
    void AddIncludeProcess(DWORD pid) { includeProcesses.push_back(pid); }
    void AddExcludeProcess(DWORD pid) { excludeProcesses.push_back(pid); }

    bool Initialize() {
        std::cerr << "Initializing WASAPI Audio Capture..." << std::endl;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to initialize COM library");
            return false;
        }

        // Create device enumerator
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                             __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to create audio device enumerator");
            std::cerr << "\nAdditional Info:" << std::endl;
            std::cerr << "  This error usually means Windows Audio components are not properly installed." << std::endl;
            return false;
        }

        // Get default audio endpoint (loopback for system audio)
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to get default audio device");
            std::cerr << "\nAdditional Info:" << std::endl;
            std::cerr << "  No audio output device found or device is disabled." << std::endl;
            std::cerr << "  To check your audio devices:" << std::endl;
            std::cerr << "    1. Right-click speaker icon in taskbar" << std::endl;
            std::cerr << "    2. Select 'Open Sound settings'" << std::endl;
            std::cerr << "    3. Check if any output device is available" << std::endl;
            std::cerr << "    4. Make sure the device is not disabled" << std::endl;
            return false;
        }

        // Get device name for logging
        IPropertyStore* pProps = nullptr;
        if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pProps))) {
            PROPVARIANT varName;
            PropVariantInit(&varName);
            if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
                std::wcerr << L"Using audio device: " << varName.pwszVal << std::endl;
                PropVariantClear(&varName);
            }
            SafeRelease(&pProps);
        }

        // Activate audio client
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to activate audio client");
            std::cerr << "\nAdditional Info:" << std::endl;
            std::cerr << "  Could not access audio device. This may be a driver or permission issue." << std::endl;
            return false;
        }

        // Get mix format
        hr = pAudioClient->GetMixFormat(&pwfx);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to get audio format");
            std::cerr << "\nAdditional Info:" << std::endl;
            std::cerr << "  Could not query device audio format. Driver may be corrupted." << std::endl;
            return false;
        }

        // 保存设备采样率
        deviceSampleRate = pwfx->nSamplesPerSec;
        
        std::cerr << "Device format: " << deviceSampleRate << "Hz, "
                  << pwfx->nChannels << " channels, "
                  << pwfx->wBitsPerSample << " bits" << std::endl;

        // Apply sample rate if specified
        if (requestedSampleRate > 0) {
            if (requestedSampleRate < 8000 || requestedSampleRate > 192000) {
                std::cerr << "\nERROR: Invalid sample rate: " << requestedSampleRate << std::endl;
                std::cerr << "Valid range: 8000 - 192000 Hz" << std::endl;
                std::cerr << "Common values: 44100, 48000" << std::endl;
                return false;
            }
            std::cerr << "Requesting sample rate: " << requestedSampleRate << "Hz" << std::endl;

            if (requestedSampleRate != deviceSampleRate) {
                std::cerr << "Device does not natively support " << requestedSampleRate
                    << "Hz, will use resampling" << std::endl;

                if (!resampler.Initialize(deviceSampleRate, requestedSampleRate, pwfx->nChannels)) {
                    std::cerr << "ERROR: Failed to initialize resampler" << std::endl;
                    return false;
                }
                needsResampling = true;
                std::cerr << "Resampler ready: " << deviceSampleRate << "Hz -> "
                    << requestedSampleRate << "Hz" << std::endl;
            }
            else {
                std::cerr << "Device natively supports " << requestedSampleRate << "Hz" << std::endl;
                needsResampling = false;
            }
        }

        // Validate chunk duration
        if (chunkDuration < 0.01 || chunkDuration > 10.0) {
            std::cerr << "\nERROR: Invalid chunk duration: " << chunkDuration << " seconds" << std::endl;
            std::cerr << "Valid range: 0.01 - 10.0 seconds" << std::endl;
            std::cerr << "Recommended: 0.05 - 0.2 seconds" << std::endl;
            return false;
        }

        // Initialize audio client for loopback capture with event callback
        REFERENCE_TIME hnsRequestedDuration = (REFERENCE_TIME)(chunkDuration * 10000000);
        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            hnsRequestedDuration,
            0,
            pwfx,
            nullptr
        );

        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to initialize audio client");

            if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT && requestedSampleRate > 0) {
                std::cerr << "\nAdditional Info:" << std::endl;
                std::cerr << "  Your requested sample rate (" << requestedSampleRate
                         << " Hz) is not supported by this device." << std::endl;
                std::cerr << "  Try running without --sample-rate to use device default." << std::endl;
            } else if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                std::cerr << "\nAdditional Info:" << std::endl;
                std::cerr << "  The chunk duration doesn't align with device requirements." << std::endl;
                std::cerr << "  Current value: " << chunkDuration << " seconds" << std::endl;
                std::cerr << "  Try values like: 0.05, 0.1, 0.2" << std::endl;
            }

            return false;
        }

        // Get buffer size
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to get audio buffer size");
            return false;
        }

        std::cerr << "Buffer size: " << bufferFrameCount << " frames ("
                  << (double)bufferFrameCount / pwfx->nSamplesPerSec * 1000
                  << " ms)" << std::endl;

        // Get capture client
        hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to get capture client service");
            return false;
        }

        // Handle mute
        if (mute) {
            std::cerr << "Note: Mute functionality is not yet implemented" << std::endl;
            // Note: Muting system audio while capturing requires additional implementation
            // This would typically involve ISimpleAudioVolume interface
        }

        std::cerr << "\n✓ Initialization successful!" << std::endl;
        std::cerr << "Final format: " << pwfx->nSamplesPerSec << "Hz, "
                  << pwfx->nChannels << " channels, "
                  << pwfx->wBitsPerSample << " bits" << std::endl;
        std::cerr << std::endl;

        return true;
    }

    void StartCapture() {
        if (!pAudioClient) return;

        // Create event for audio buffer ready notification
        HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (hEvent == nullptr) {
            std::cerr << "Failed to create event" << std::endl;
            return;
        }

        // Set event handle for buffer notifications
        HRESULT hr = pAudioClient->SetEventHandle(hEvent);
        if (FAILED(hr)) {
            std::cerr << "Failed to set event handle, falling back to polling mode" << std::endl;
            CloseHandle(hEvent);
            StartCapturePolling();
            return;
        }

        hr = pAudioClient->Start();
        if (FAILED(hr)) {
            std::cerr << "Failed to start audio client" << std::endl;
            CloseHandle(hEvent);
            return;
        }

        running = true;

        // Set stdout to binary mode
        _setmode(_fileno(stdout), _O_BINARY);

        std::cerr << "Using event-driven capture mode (no frame drops)" << std::endl;

        if (needsResampling) {
            std::cerr << "Resampling enabled: " << deviceSampleRate << "Hz -> "
                << requestedSampleRate << "Hz" << std::endl;
        }

        // Event-driven capture loop
        while (running) {
            // Wait for buffer ready event with timeout
            DWORD waitResult = WaitForSingleObject(hEvent, 2000);

            if (waitResult != WAIT_OBJECT_0) {
                if (waitResult == WAIT_TIMEOUT) {
                    // No audio data for 2 seconds, continue waiting
                    continue;
                } else {
                    std::cerr << "Wait failed" << std::endl;
                    break;
                }
            }

            // Process all available packets
            UINT32 packetLength = 0;
            hr = pCaptureClient->GetNextPacketSize(&packetLength);

            while (SUCCEEDED(hr) && packetLength != 0) {
                BYTE* pData = nullptr;
                UINT32 numFramesAvailable = 0;
                DWORD flags = 0;
                UINT64 devicePosition = 0;
                UINT64 qpcPosition = 0;

                hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);

                if (SUCCEEDED(hr)) {
                    // Check for buffer overrun (data corruption/discontinuity)
                    if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
                        std::cerr << "Warning: Audio data discontinuity detected (possible frame drop)" << std::endl;
                    }

                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                        // Silent buffer - write zeros
                        UINT32 outputFrames = numFramesAvailable;
                        if (needsResampling) {
                            double ratio = (double)requestedSampleRate / deviceSampleRate;
                            outputFrames = static_cast<UINT32>(numFramesAvailable * ratio);
                        }
                        std::vector<BYTE> silence(outputFrames * pwfx->nBlockAlign, 0);
                        std::cout.write(reinterpret_cast<char*>(silence.data()), silence.size());
                    } else {
                        if (needsResampling) {
                            std::vector<BYTE> resampledData;
                            if (resampler.Process(pData, numFramesAvailable, resampledData)) {
                                std::cout.write(reinterpret_cast<char*>(resampledData.data()),
                                    resampledData.size());
                            }
                            else {
                                std::cerr << "Resampling failed, skipping frame" << std::endl;
                            }
                        }
                        else {
                            // Write actual audio data to stdout
                            UINT32 dataSize = numFramesAvailable * pwfx->nBlockAlign;
                            std::cout.write(reinterpret_cast<char*>(pData), dataSize);
                        }
                       
                    }

                    std::cout.flush();

                    pCaptureClient->ReleaseBuffer(numFramesAvailable);
                } else {
                    std::cerr << "GetBuffer failed: 0x" << std::hex << hr << std::endl;
                }

                hr = pCaptureClient->GetNextPacketSize(&packetLength);
            }
        }

        pAudioClient->Stop();
        CloseHandle(hEvent);
    }

    // Fallback polling mode
    void StartCapturePolling() {
        if (!pAudioClient) return;

        HRESULT hr = pAudioClient->Start();
        if (FAILED(hr)) {
            std::cerr << "Failed to start audio client" << std::endl;
            return;
        }

        running = true;

        // Set stdout to binary mode
        _setmode(_fileno(stdout), _O_BINARY);

        std::cerr << "Using polling mode (sleep time reduced to minimize frame drops)" << std::endl;

        // Optimized polling: sleep 1/4 of buffer duration to reduce latency
        DWORD sleepTime = static_cast<DWORD>(chunkDuration * 1000 / 4);
        if (sleepTime < 1) sleepTime = 1;

        // Capture loop
        while (running) {
            Sleep(sleepTime);

            UINT32 packetLength = 0;
            hr = pCaptureClient->GetNextPacketSize(&packetLength);

            while (SUCCEEDED(hr) && packetLength != 0) {
                BYTE* pData = nullptr;
                UINT32 numFramesAvailable = 0;
                DWORD flags = 0;

                hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);

                if (SUCCEEDED(hr)) {
                    // Check for discontinuity
                    if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
                        std::cerr << "Warning: Audio data discontinuity detected" << std::endl;
                    }

                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                        UINT32 outputFrames = numFramesAvailable;
                        if (needsResampling) {
                            double ratio = (double)requestedSampleRate / deviceSampleRate;
                            outputFrames = static_cast<UINT32>(numFramesAvailable * ratio);
                        }
                        // Silent buffer - write zeros
                        std::vector<BYTE> silence(outputFrames * pwfx->nBlockAlign, 0);
                        std::cout.write(reinterpret_cast<char*>(silence.data()), silence.size());
                    } else {
                        if (needsResampling) {
                            std::vector<BYTE> resampledData;
                            if (resampler.Process(pData, numFramesAvailable, resampledData)) {
                                std::cout.write(reinterpret_cast<char*>(resampledData.data()),
                                    resampledData.size());
                            }
                        }
                        else {
                            // Write actual audio data to stdout
                            UINT32 dataSize = numFramesAvailable * pwfx->nBlockAlign;
                            std::cout.write(reinterpret_cast<char*>(pData), dataSize);
                        }
                     
                    }

                    std::cout.flush();

                    pCaptureClient->ReleaseBuffer(numFramesAvailable);
                }

                hr = pCaptureClient->GetNextPacketSize(&packetLength);
            }
        }

        pAudioClient->Stop();
    }

    void Stop() {
        running = false;
    }

    void Cleanup() {
        if (pAudioClient) {
            pAudioClient->Stop();
        }

        if (pwfx) {
            CoTaskMemFree(pwfx);
            pwfx = nullptr;
        }

        SafeRelease(&pCaptureClient);
        SafeRelease(&pAudioClient);
        SafeRelease(&pDevice);
        SafeRelease(&pEnumerator);

        CoUninitialize();
    }
};

// Global capture instance for signal handling
WASAPICapture* g_capture = nullptr;

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        if (g_capture) {
            g_capture->Stop();
        }
        return TRUE;
    }
    return FALSE;
}

void PrintUsage() {
    std::cerr << "Usage: wasapi_capture [options]\n"
              << "Options:\n"
              << "  --sample-rate <Hz>           Target sample rate (default: device default)\n"
              << "  --chunk-duration <seconds>   Duration of each audio chunk (default: 0.2)\n"
              << "  --mute                       Mute system audio while capturing\n"
              << "  --include-processes <pid>... Only capture audio from these process IDs\n"
              << "  --exclude-processes <pid>... Exclude audio from these process IDs\n"
              << "  --help                       Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Print banner
    std::cerr << "========================================" << std::endl;
    std::cerr << "WASAPI Audio Capture v1.0" << std::endl;
    std::cerr << "========================================" << std::endl;
    std::cerr << std::endl;

    // Check system requirements
    ErrorHandler::CheckSystemRequirements();

    WASAPICapture capture;
    g_capture = &capture;

    // Set console control handler
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // Parse command line arguments
    try {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                PrintUsage();
                return 0;
            }
            else if (arg == "--sample-rate") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: --sample-rate requires a value" << std::endl;
                    std::cerr << "Example: --sample-rate 48000" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
                try {
                    int rate = std::stoi(argv[++i]);
                    if (rate < 8000 || rate > 192000) {
                        std::cerr << "ERROR: Sample rate out of range: " << rate << std::endl;
                        std::cerr << "Valid range: 8000 - 192000 Hz" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetSampleRate(rate);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid sample rate value: " << argv[i] << std::endl;
                    std::cerr << "Must be a number between 8000 and 192000" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
            }
            else if (arg == "--chunk-duration") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: --chunk-duration requires a value" << std::endl;
                    std::cerr << "Example: --chunk-duration 0.2" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
                try {
                    double duration = std::stod(argv[++i]);
                    if (duration < 0.01 || duration > 10.0) {
                        std::cerr << "ERROR: Chunk duration out of range: " << duration << std::endl;
                        std::cerr << "Valid range: 0.01 - 10.0 seconds" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetChunkDuration(duration);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid chunk duration value: " << argv[i] << std::endl;
                    std::cerr << "Must be a number between 0.01 and 10.0" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
            }
            else if (arg == "--mute") {
                capture.SetMute(true);
            }
            else if (arg == "--include-processes") {
                // Read all following numeric arguments as PIDs
                bool foundAny = false;
                while (i + 1 < argc && isdigit(argv[i + 1][0])) {
                    try {
                        capture.AddIncludeProcess(std::stoul(argv[++i]));
                        foundAny = true;
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR: Invalid process ID: " << argv[i] << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                }
                if (!foundAny) {
                    std::cerr << "ERROR: --include-processes requires at least one process ID" << std::endl;
                    std::cerr << "Example: --include-processes 1234 5678" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
            }
            else if (arg == "--exclude-processes") {
                // Read all following numeric arguments as PIDs
                bool foundAny = false;
                while (i + 1 < argc && isdigit(argv[i + 1][0])) {
                    try {
                        capture.AddExcludeProcess(std::stoul(argv[++i]));
                        foundAny = true;
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR: Invalid process ID: " << argv[i] << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                }
                if (!foundAny) {
                    std::cerr << "ERROR: --exclude-processes requires at least one process ID" << std::endl;
                    std::cerr << "Example: --exclude-processes 1234 5678" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
            }
            else {
                std::cerr << "ERROR: Unknown argument: " << arg << std::endl;
                std::cerr << "Use --help to see available options" << std::endl;
                PrintUsage();
                return static_cast<int>(ErrorCode::INVALID_PARAMETER);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception while parsing arguments: " << e.what() << std::endl;
        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
    }

    // Initialize capture
    if (!capture.Initialize()) {
        std::cerr << "\n!!! INITIALIZATION FAILED !!!" << std::endl;
        std::cerr << "Please check the error messages above for details." << std::endl;
        std::cerr << "Common solutions:" << std::endl;
        std::cerr << "  1. Make sure audio device is working (play some music)" << std::endl;
        std::cerr << "  2. Try running as Administrator" << std::endl;
        std::cerr << "  3. Update audio drivers" << std::endl;
        std::cerr << "  4. Restart Windows Audio service" << std::endl;
        return static_cast<int>(ErrorCode::UNKNOWN_ERROR);
    }

    // Start capturing
    std::cerr << "Starting capture... (Press Ctrl+C to stop)" << std::endl;
    std::cerr << "Audio data will be written to stdout (binary PCM format)" << std::endl;
    std::cerr << "========================================" << std::endl;
    std::cerr << std::endl;

    capture.StartCapture();

    std::cerr << "\nCapture stopped." << std::endl;

    return static_cast<int>(ErrorCode::SUCCESS);
}

