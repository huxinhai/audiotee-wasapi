#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audiopolicy.h>
#include <psapi.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
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

// Audio format converter helper
class AudioFormatConverter {
public:
    // 将 32-bit float 转换为指定格式
    static void FloatToPCM(const BYTE* floatData, UINT32 numSamples, int outputBits, std::vector<BYTE>& outputData) {
        const float* floatSamples = reinterpret_cast<const float*>(floatData);
        
        if (outputBits == 16) {
            // 转换为 16-bit PCM
            outputData.resize(numSamples * sizeof(int16_t));
            int16_t* int16Data = reinterpret_cast<int16_t*>(outputData.data());
            
            for (UINT32 i = 0; i < numSamples; i++) {
                float sample = floatSamples[i];
                // Float 范围是 [-1.0, 1.0]，转换为 [-32768, 32767]
                sample = sample * 32768.0f;
                
                // 限幅
                if (sample > 32767.0f) sample = 32767.0f;
                if (sample < -32768.0f) sample = -32768.0f;
                
                // 舍入
                int16Data[i] = static_cast<int16_t>(sample >= 0.0f ? sample + 0.5f : sample - 0.5f);
            }
        }
        else if (outputBits == 32) {
            // 转换为 32-bit PCM (保持 float 格式)
            outputData.resize(numSamples * sizeof(float));
            float* outputFloat = reinterpret_cast<float*>(outputData.data());
            
            for (UINT32 i = 0; i < numSamples; i++) {
                outputFloat[i] = floatSamples[i]; // 直接复制
            }
        }
    }
    
    // 将 16-bit PCM 转换为指定格式
    static void Int16ToPCM(const BYTE* int16Data, UINT32 numSamples, int outputBits, std::vector<BYTE>& outputData) {
        const int16_t* samples = reinterpret_cast<const int16_t*>(int16Data);
        
        if (outputBits == 16) {
            // 直接复制
            outputData.resize(numSamples * sizeof(int16_t));
            memcpy(outputData.data(), int16Data, outputData.size());
        }
        else if (outputBits == 32) {
            // 转换为 32-bit float
            outputData.resize(numSamples * sizeof(float));
            float* outputFloat = reinterpret_cast<float*>(outputData.data());
            
            for (UINT32 i = 0; i < numSamples; i++) {
                // 16-bit 范围 [-32768, 32767] 转换为 float [-1.0, 1.0)
                outputFloat[i] = samples[i] * (1.0f / 32768.0f);
            }
        }
    }
    
    // 声道转换
    static void ConvertChannels(const BYTE* inputData, UINT32 inputFrames, int inputChannels, 
                               int outputChannels, int bitsPerSample, std::vector<BYTE>& outputData) {
        if (inputChannels == outputChannels) {
            // 声道数相同，直接复制
            UINT32 bytesPerSample = bitsPerSample / 8;
            outputData.resize(inputFrames * outputChannels * bytesPerSample);
            memcpy(outputData.data(), inputData, outputData.size());
            return;
        }
        
        UINT32 bytesPerSample = bitsPerSample / 8;
        outputData.resize(inputFrames * outputChannels * bytesPerSample);
        
        if (inputChannels == 2 && outputChannels == 1) {
            // 立体声转单声道（取平均值）
            if (bitsPerSample == 16) {
                const int16_t* input = reinterpret_cast<const int16_t*>(inputData);
                int16_t* output = reinterpret_cast<int16_t*>(outputData.data());
                
                for (UINT32 i = 0; i < inputFrames; i++) {
                    int32_t left = input[i * 2];
                    int32_t right = input[i * 2 + 1];
                    output[i] = static_cast<int16_t>((left + right) / 2);
                }
            }
            else if (bitsPerSample == 32) {
                const float* input = reinterpret_cast<const float*>(inputData);
                float* output = reinterpret_cast<float*>(outputData.data());
                
                for (UINT32 i = 0; i < inputFrames; i++) {
                    output[i] = (input[i * 2] + input[i * 2 + 1]) * 0.5f;
                }
            }
        }
        else if (inputChannels == 1 && outputChannels == 2) {
            // 单声道转立体声（复制到两个声道）
            if (bitsPerSample == 16) {
                const int16_t* input = reinterpret_cast<const int16_t*>(inputData);
                int16_t* output = reinterpret_cast<int16_t*>(outputData.data());
                
                for (UINT32 i = 0; i < inputFrames; i++) {
                    output[i * 2] = input[i];     // 左声道
                    output[i * 2 + 1] = input[i]; // 右声道
                }
            }
            else if (bitsPerSample == 32) {
                const float* input = reinterpret_cast<const float*>(inputData);
                float* output = reinterpret_cast<float*>(outputData.data());
                
                for (UINT32 i = 0; i < inputFrames; i++) {
                    output[i * 2] = input[i];     // 左声道
                    output[i * 2 + 1] = input[i]; // 右声道
                }
            }
        }
    }
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
    bool firstProcess = true; // 用于调试首次处理

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
        // 使用最高质量的转换器避免失真
        // SRC_SINC_BEST_QUALITY: 最高质量，适合所有采样率转换
        // SRC_SINC_MEDIUM_QUALITY: 中等质量，对大比率转换可能有失真
        // SRC_SINC_FASTEST: 最快但质量最低
        srcState = src_new(SRC_SINC_BEST_QUALITY, channels, &error);

        if (!srcState) {
            std::cerr << "Failed to create resampler: " << src_strerror(error) << std::endl;
            return false;
        }

        double ratio = (double)outRate / inRate;
        std::cerr << "Resampler initialized successfully:" << std::endl;
        std::cerr << "  Input:  " << inRate << " Hz, " << channels << " channels" << std::endl;
        std::cerr << "  Output: " << outRate << " Hz, " << channels << " channels" << std::endl;
        std::cerr << "  Ratio:  " << ratio << " (using SRC_SINC_BEST_QUALITY)" << std::endl;
        
        if (ratio < 0.1 || ratio > 10.0) {
            std::cerr << "  Warning: Large conversion ratio may affect quality" << std::endl;
        }

        return true;
    }

    // 转换 16-bit PCM 数据
    bool Process(const BYTE* inputData, UINT32 inputFrames, std::vector<BYTE>& outputData) {
        if (!srcState) return false;
        
        if (inputFrames == 0) {
            outputData.clear();
            return true;
        }

        // 计算输出帧数
        double ratio = (double)outputRate / inputRate;
        UINT32 outputFrames = static_cast<UINT32>(inputFrames * ratio) + 16; // 加一些缓冲

        // 转换为 float (libsamplerate 使用 float)
        size_t inputSamples = inputFrames * channels;
        inputBuffer.resize(inputSamples);

        const int16_t* int16Data = reinterpret_cast<const int16_t*>(inputData);
        for (size_t i = 0; i < inputSamples; i++) {
            // 正确的归一化：范围 [-1.0, 1.0)
            inputBuffer[i] = int16Data[i] * (1.0f / 32768.0f);
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
        
        // 调试：输出首次转换的信息
        if (firstProcess) {
            std::cerr << "First resampling:" << std::endl;
            std::cerr << "  Input frames:  " << srcData.input_frames << " -> " 
                     << srcData.input_frames_used << " used" << std::endl;
            std::cerr << "  Output frames: " << srcData.output_frames << " available -> " 
                     << srcData.output_frames_gen << " generated" << std::endl;
            std::cerr << "  Ratio: " << srcData.src_ratio << std::endl;
            firstProcess = false;
        }

        // 转换回 16-bit PCM
        size_t actualOutputSamples = srcData.output_frames_gen * channels;
        outputData.resize(actualOutputSamples * sizeof(int16_t));
        int16_t* outputInt16 = reinterpret_cast<int16_t*>(outputData.data());

        for (size_t i = 0; i < actualOutputSamples; i++) {
            float sample = outputBuffer[i] * 32768.0f;
            // 限幅到 16-bit 范围
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            // 使用舍入而不是截断，减少量化噪声
            outputInt16[i] = static_cast<int16_t>(sample >= 0.0f ? sample + 0.5f : sample - 0.5f);
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
    int requestedBitsPerSample = 0;  // 用户请求的位深度
    int requestedChannels = 0;       // 用户请求的声道数
    double chunkDuration = 0.2;  // seconds
    bool mute = false;
    std::vector<DWORD> includeProcesses;
    std::vector<DWORD> excludeProcesses;

    bool running = false;
    AudioResampler resampler;  // 重采样器
    bool needsResampling = false;  // 是否需要重采样
    bool isFloatFormat = false;     // 是否是 float 格式
    UINT32 bytesPerSample = 2;      // 每个采样的字节数

public:
    WASAPICapture() {}

    ~WASAPICapture() {
        Cleanup();
    }

    void SetSampleRate(int rate) { requestedSampleRate = rate; }
    void SetBitsPerSample(int bits) { requestedBitsPerSample = bits; }
    void SetChannels(int channels) { requestedChannels = channels; }
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
        
        // 检查音频格式 - 只支持 16-bit PCM
        if (pwfx->wFormatTag != WAVE_FORMAT_PCM && pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
            std::cerr << "\nERROR: Unsupported audio format tag: " << pwfx->wFormatTag << std::endl;
            std::cerr << "Only PCM format is supported" << std::endl;
            return false;
        }
        
        // 检测设备音频格式
        bytesPerSample = pwfx->wBitsPerSample / 8;
        
        std::cerr << "\n=== 设备音频格式检测 ===" << std::endl;
        std::cerr << "设备格式: " << pwfx->nSamplesPerSec << "Hz, " 
                  << pwfx->nChannels << " channels, " 
                  << pwfx->wBitsPerSample << " bits" << std::endl;
        
        // 检测是否为 float 格式
        if (pwfx->wBitsPerSample == 32 && pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            WAVEFORMATEXTENSIBLE* pWfxEx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
            if (pWfxEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
                isFloatFormat = true;
                std::cerr << "格式类型: 32-bit IEEE Float" << std::endl;
            }
        }
        else if (pwfx->wBitsPerSample == 16) {
            isFloatFormat = false;
            std::cerr << "格式类型: 16-bit PCM" << std::endl;
        }
        else {
            std::cerr << "\nWARNING: Unsupported bit depth: " << pwfx->wBitsPerSample << std::endl;
            std::cerr << "Supported formats: 16-bit PCM, 32-bit float" << std::endl;
            return false;
        }
        
        // 显示用户请求的输出格式
        std::cerr << "\n=== 输出格式配置 ===" << std::endl;
        if (requestedBitsPerSample > 0) {
            std::cerr << "输出位深度: " << requestedBitsPerSample << " bits" << std::endl;
        } else {
            std::cerr << "输出位深度: 16 bits (默认)" << std::endl;
        }
        
        if (requestedChannels > 0) {
            std::cerr << "输出声道: " << requestedChannels << " channels" << std::endl;
        } else {
            std::cerr << "输出声道: " << pwfx->nChannels << " channels (设备默认)" << std::endl;
        }

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
                        // 处理实际音频数据
                        std::vector<BYTE> processedData;
                        BYTE* currentData = pData;
                        UINT32 currentFrames = numFramesAvailable;
                        int currentChannels = pwfx->nChannels;
                        int currentBits = pwfx->wBitsPerSample;
                        
                        // 第一步：格式转换（如果需要）
                        if (isFloatFormat) {
                            UINT32 numSamples = currentFrames * currentChannels;
                            int outputBits = (requestedBitsPerSample > 0) ? requestedBitsPerSample : 16;
                            AudioFormatConverter::FloatToPCM(pData, numSamples, outputBits, processedData);
                            currentData = processedData.data();
                            currentBits = outputBits;
                        }
                        else if (requestedBitsPerSample > 0 && requestedBitsPerSample != currentBits) {
                            UINT32 numSamples = currentFrames * currentChannels;
                            AudioFormatConverter::Int16ToPCM(pData, numSamples, requestedBitsPerSample, processedData);
                            currentData = processedData.data();
                            currentBits = requestedBitsPerSample;
                        }
                        
                        // 第二步：声道转换（如果需要）
                        if (requestedChannels > 0 && requestedChannels != currentChannels) {
                            std::vector<BYTE> channelConvertedData;
                            AudioFormatConverter::ConvertChannels(currentData, currentFrames, 
                                currentChannels, requestedChannels, currentBits, channelConvertedData);
                            processedData = std::move(channelConvertedData);
                            currentData = processedData.data();
                            currentChannels = requestedChannels;
                        }
                        
                        // 第三步：重采样（如果需要）
                        if (needsResampling) {
                            std::vector<BYTE> resampledData;
                            if (resampler.Process(currentData, currentFrames, resampledData)) {
                                std::cout.write(reinterpret_cast<char*>(resampledData.data()),
                                    resampledData.size());
                            }
                            else {
                                std::cerr << "Resampling failed, skipping frame" << std::endl;
                            }
                        }
                        else {
                            // 直接输出
                            if (processedData.empty()) {
                                UINT32 dataSize = currentFrames * currentChannels * (currentBits / 8);
                                std::cout.write(reinterpret_cast<char*>(currentData), dataSize);
                            }
                            else {
                                std::cout.write(reinterpret_cast<char*>(processedData.data()), processedData.size());
                            }
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
                        // 处理实际音频数据
                        std::vector<BYTE> processedData;
                        BYTE* currentData = pData;
                        UINT32 currentFrames = numFramesAvailable;
                        int currentChannels = pwfx->nChannels;
                        int currentBits = pwfx->wBitsPerSample;
                        
                        // 第一步：格式转换（如果需要）
                        if (isFloatFormat) {
                            UINT32 numSamples = currentFrames * currentChannels;
                            int outputBits = (requestedBitsPerSample > 0) ? requestedBitsPerSample : 16;
                            AudioFormatConverter::FloatToPCM(pData, numSamples, outputBits, processedData);
                            currentData = processedData.data();
                            currentBits = outputBits;
                        }
                        else if (requestedBitsPerSample > 0 && requestedBitsPerSample != currentBits) {
                            UINT32 numSamples = currentFrames * currentChannels;
                            AudioFormatConverter::Int16ToPCM(pData, numSamples, requestedBitsPerSample, processedData);
                            currentData = processedData.data();
                            currentBits = requestedBitsPerSample;
                        }
                        
                        // 第二步：声道转换（如果需要）
                        if (requestedChannels > 0 && requestedChannels != currentChannels) {
                            std::vector<BYTE> channelConvertedData;
                            AudioFormatConverter::ConvertChannels(currentData, currentFrames, 
                                currentChannels, requestedChannels, currentBits, channelConvertedData);
                            processedData = std::move(channelConvertedData);
                            currentData = processedData.data();
                            currentChannels = requestedChannels;
                        }
                        
                        // 第三步：重采样（如果需要）
                        if (needsResampling) {
                            std::vector<BYTE> resampledData;
                            if (resampler.Process(currentData, currentFrames, resampledData)) {
                                std::cout.write(reinterpret_cast<char*>(resampledData.data()),
                                    resampledData.size());
                            }
                        }
                        else {
                            // 直接输出
                            if (processedData.empty()) {
                                UINT32 dataSize = currentFrames * currentChannels * (currentBits / 8);
                                std::cout.write(reinterpret_cast<char*>(currentData), dataSize);
                            }
                            else {
                                std::cout.write(reinterpret_cast<char*>(processedData.data()), processedData.size());
                            }
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
              << "  --bits-per-sample <bits>     Output bit depth: 16 or 32 (default: 16)\n"
              << "  --channels <num>             Output channels: 1 or 2 (default: device default)\n"
              << "  --chunk-duration <seconds>   Duration of each audio chunk (default: 0.2)\n"
              << "  --mute                       Mute system audio while capturing\n"
              << "  --include-processes <pid>... Only capture audio from these process IDs\n"
              << "  --exclude-processes <pid>... Exclude audio from these process IDs\n"
              << "  --help                       Show this help message\n"
              << "\nExamples:\n"
              << "  wasapi_capture --sample-rate 16000 --bits-per-sample 16 --channels 1\n"
              << "  wasapi_capture --sample-rate 48000 --bits-per-sample 32 --channels 2\n"
              << "  wasapi_capture --sample-rate 44100  # Use device default format\n"
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
            else if (arg == "--bits-per-sample") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: --bits-per-sample requires a value" << std::endl;
                    std::cerr << "Example: --bits-per-sample 16" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
                try {
                    int bits = std::stoi(argv[++i]);
                    if (bits != 16 && bits != 32) {
                        std::cerr << "ERROR: Invalid bit depth: " << bits << std::endl;
                        std::cerr << "Valid values: 16 or 32" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetBitsPerSample(bits);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid bit depth value: " << argv[i] << std::endl;
                    std::cerr << "Must be 16 or 32" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
            }
            else if (arg == "--channels") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: --channels requires a value" << std::endl;
                    std::cerr << "Example: --channels 2" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
                try {
                    int channels = std::stoi(argv[++i]);
                    if (channels != 1 && channels != 2) {
                        std::cerr << "ERROR: Invalid channel count: " << channels << std::endl;
                        std::cerr << "Valid values: 1 (mono) or 2 (stereo)" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetChannels(channels);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid channel count value: " << argv[i] << std::endl;
                    std::cerr << "Must be 1 or 2" << std::endl;
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

