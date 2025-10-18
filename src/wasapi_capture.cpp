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
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <initguid.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

// Define CLSID for Windows Media Audio Resampler DSP
// This avoids dependency on wmcodecdsp.lib
// {F447B69E-1884-4A7E-8055-346F74D6EDB3}
static const GUID CLSID_CResamplerMediaObject = 
    { 0xf447b69e, 0x1884, 0x4a7e, { 0x80, 0x55, 0x34, 0x6f, 0x74, 0xd6, 0xed, 0xb3 } };

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

// Audio Resampler class for format conversion
class AudioResampler {
private:
    IMFTransform* pResampler = nullptr;
    IMFMediaType* pInputType = nullptr;
    IMFMediaType* pOutputType = nullptr;
    
    WAVEFORMATEX* pInputFormat = nullptr;
    WAVEFORMATEX* pOutputFormat = nullptr;
    
    bool initialized = false;
    
public:
    AudioResampler() {}
    
    ~AudioResampler() {
        Cleanup();
    }
    
    bool Initialize(WAVEFORMATEX* inputFormat, WAVEFORMATEX* outputFormat) {
        if (!inputFormat || !outputFormat) {
            std::cerr << "Invalid input or output format" << std::endl;
            return false;
        }
        
        pInputFormat = inputFormat;
        pOutputFormat = outputFormat;
        
        HRESULT hr = CoCreateInstance(CLSID_CResamplerMediaObject, nullptr, 
                                     CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pResampler));
        if (FAILED(hr)) {
            std::cerr << "Failed to create resampler: 0x" << std::hex << hr << std::dec << std::endl;
            std::cerr << "Make sure Media Foundation is properly initialized" << std::endl;
            return false;
        }
        
        // Create input media type
        hr = MFCreateMediaType(&pInputType);
        if (FAILED(hr)) {
            std::cerr << "Failed to create input media type: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        // Set input type properties
        pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        pInputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        pInputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, pInputFormat->nChannels);
        pInputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, pInputFormat->nSamplesPerSec);
        pInputType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, pInputFormat->nBlockAlign);
        pInputType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, pInputFormat->nAvgBytesPerSec);
        pInputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, pInputFormat->wBitsPerSample);
        pInputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        
        // Create output media type
        hr = MFCreateMediaType(&pOutputType);
        if (FAILED(hr)) {
            std::cerr << "Failed to create output media type: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        // Set output type properties
        pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        pOutputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        pOutputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, pOutputFormat->nChannels);
        pOutputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, pOutputFormat->nSamplesPerSec);
        pOutputType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, pOutputFormat->nBlockAlign);
        pOutputType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, pOutputFormat->nAvgBytesPerSec);
        pOutputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, pOutputFormat->wBitsPerSample);
        pOutputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        
        // Set input type on resampler
        hr = pResampler->SetInputType(0, pInputType, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to set input type on resampler: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        // Set output type on resampler
        hr = pResampler->SetOutputType(0, pOutputType, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to set output type on resampler: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        // Send required messages to the resampler
        hr = pResampler->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to begin streaming: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        hr = pResampler->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to start stream: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        initialized = true;
        std::cerr << "Resampler initialized successfully" << std::endl;
        return true;
    }
    
    bool ProcessAudio(const BYTE* inputData, UINT32 inputSize, 
                     std::vector<BYTE>& outputData) {
        if (!initialized || !inputData || inputSize == 0) {
            return false;
        }
        
        HRESULT hr;
        IMFSample* pSample = nullptr;
        IMFMediaBuffer* pBuffer = nullptr;
        
        // Create input sample and buffer
        hr = MFCreateSample(&pSample);
        if (FAILED(hr)) {
            std::cerr << "Failed to create sample: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        hr = MFCreateMemoryBuffer(inputSize, &pBuffer);
        if (FAILED(hr)) {
            SafeRelease(&pSample);
            std::cerr << "Failed to create buffer: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }
        
        // Copy input data
        BYTE* pBufferData = nullptr;
        hr = pBuffer->Lock(&pBufferData, nullptr, nullptr);
        if (FAILED(hr)) {
            SafeRelease(&pBuffer);
            SafeRelease(&pSample);
            return false;
        }
        
        memcpy(pBufferData, inputData, inputSize);
        pBuffer->Unlock();
        pBuffer->SetCurrentLength(inputSize);
        
        // Add buffer to sample
        pSample->AddBuffer(pBuffer);
        SafeRelease(&pBuffer);
        
        // Process input
        hr = pResampler->ProcessInput(0, pSample, 0);
        SafeRelease(&pSample);
        
        if (FAILED(hr)) {
            if (hr != MF_E_NOTACCEPTING) {
                std::cerr << "ProcessInput failed: 0x" << std::hex << hr << std::dec << std::endl;
            }
            return false;
        }
        
        // Get output
        MFT_OUTPUT_DATA_BUFFER outputBuffer = {};
        MFT_OUTPUT_STREAM_INFO streamInfo = {};
        
        hr = pResampler->GetOutputStreamInfo(0, &streamInfo);
        if (FAILED(hr)) {
            return false;
        }
        
        // Create output sample
        hr = MFCreateSample(&outputBuffer.pSample);
        if (FAILED(hr)) {
            return false;
        }
        
        hr = MFCreateMemoryBuffer(streamInfo.cbSize, &pBuffer);
        if (FAILED(hr)) {
            SafeRelease(&outputBuffer.pSample);
            return false;
        }
        
        outputBuffer.pSample->AddBuffer(pBuffer);
        SafeRelease(&pBuffer);
        
        DWORD status = 0;
        hr = pResampler->ProcessOutput(0, 1, &outputBuffer, &status);
        
        if (SUCCEEDED(hr)) {
            // Extract output data
            IMFMediaBuffer* pOutBuffer = nullptr;
            hr = outputBuffer.pSample->ConvertToContiguousBuffer(&pOutBuffer);
            if (SUCCEEDED(hr)) {
                BYTE* pOutData = nullptr;
                DWORD outSize = 0;
                
                hr = pOutBuffer->Lock(&pOutData, nullptr, &outSize);
                if (SUCCEEDED(hr) && outSize > 0) {
                    outputData.assign(pOutData, pOutData + outSize);
                    pOutBuffer->Unlock();
                }
                SafeRelease(&pOutBuffer);
            }
            SafeRelease(&outputBuffer.pSample);
            return true;
        } else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            // Need more input - this is normal
            SafeRelease(&outputBuffer.pSample);
            return true;
        } else {
            std::cerr << "ProcessOutput failed: 0x" << std::hex << hr << std::dec << std::endl;
            SafeRelease(&outputBuffer.pSample);
            return false;
        }
    }
    
    void Cleanup() {
        SafeRelease(&pOutputType);
        SafeRelease(&pInputType);
        SafeRelease(&pResampler);
        initialized = false;
    }
};

class WASAPICapture {
private:
    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;
    WAVEFORMATEX* pwfx = nullptr;
    WAVEFORMATEX* pOutputFormat = nullptr;
    UINT32 bufferFrameCount = 0;

    int sampleRate = 0;  // 0 means use device default
    int channels = 0;    // 0 means use device default
    int bitDepth = 0;    // 0 means use device default
    double chunkDuration = 0.2;  // seconds
    bool mute = false;
    std::vector<DWORD> includeProcesses;
    std::vector<DWORD> excludeProcesses;

    bool running = false;
    bool needsResampling = false;
    std::unique_ptr<AudioResampler> resampler;

public:
    WASAPICapture() {}

    ~WASAPICapture() {
        Cleanup();
    }

    void SetSampleRate(int rate) { sampleRate = rate; }
    void SetChannels(int ch) { channels = ch; }
    void SetBitDepth(int bits) { bitDepth = bits; }
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

        // Initialize Media Foundation
        hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) {
            ErrorHandler::PrintDetailedError(hr, "Failed to initialize Media Foundation");
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

        std::cerr << "Device format: " << pwfx->nSamplesPerSec << "Hz, "
                  << pwfx->nChannels << " channels, "
                  << pwfx->wBitsPerSample << " bits" << std::endl;

        // Check if we need format conversion
        int targetSampleRate = (sampleRate > 0) ? sampleRate : pwfx->nSamplesPerSec;
        int targetChannels = (channels > 0) ? channels : pwfx->nChannels;
        int targetBitDepth = (bitDepth > 0) ? bitDepth : pwfx->wBitsPerSample;

        // Validate parameters
        if (sampleRate > 0 && (sampleRate < 8000 || sampleRate > 192000)) {
            std::cerr << "\nERROR: Invalid sample rate: " << sampleRate << std::endl;
            std::cerr << "Valid range: 8000 - 192000 Hz" << std::endl;
            std::cerr << "Common values: 44100, 48000" << std::endl;
            return false;
        }

        if (channels > 0 && (channels < 1 || channels > 8)) {
            std::cerr << "\nERROR: Invalid channel count: " << channels << std::endl;
            std::cerr << "Valid range: 1 - 8 channels" << std::endl;
            std::cerr << "Common values: 1 (mono), 2 (stereo)" << std::endl;
            return false;
        }

        if (bitDepth > 0 && (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)) {
            std::cerr << "\nERROR: Invalid bit depth: " << bitDepth << std::endl;
            std::cerr << "Valid values: 16, 24, 32 bits" << std::endl;
            return false;
        }

        // Check if we need resampling
        needsResampling = (targetSampleRate != pwfx->nSamplesPerSec) ||
                         (targetChannels != pwfx->nChannels) ||
                         (targetBitDepth != pwfx->wBitsPerSample);

        if (needsResampling) {
            std::cerr << "Format conversion required:" << std::endl;
            std::cerr << "  Input:  " << pwfx->nSamplesPerSec << "Hz, " 
                      << pwfx->nChannels << " channels, " << pwfx->wBitsPerSample << " bits" << std::endl;
            std::cerr << "  Output: " << targetSampleRate << "Hz, " 
                      << targetChannels << " channels, " << targetBitDepth << " bits" << std::endl;

            // Create output format
            pOutputFormat = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
            if (!pOutputFormat) {
                std::cerr << "Failed to allocate memory for output format" << std::endl;
                return false;
            }

            // Copy input format and modify
            memcpy(pOutputFormat, pwfx, sizeof(WAVEFORMATEX));
            pOutputFormat->nSamplesPerSec = targetSampleRate;
            pOutputFormat->nChannels = targetChannels;
            pOutputFormat->wBitsPerSample = targetBitDepth;
            pOutputFormat->nBlockAlign = (targetChannels * targetBitDepth) / 8;
            pOutputFormat->nAvgBytesPerSec = targetSampleRate * pOutputFormat->nBlockAlign;

            // Initialize resampler
            resampler = std::make_unique<AudioResampler>();
            if (!resampler->Initialize(pwfx, pOutputFormat)) {
                std::cerr << "Failed to initialize audio resampler" << std::endl;
                return false;
            }
            std::cerr << "Audio resampler initialized successfully" << std::endl;
        } else {
            std::cerr << "No format conversion needed, using device format" << std::endl;
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

            if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT && sampleRate > 0) {
                std::cerr << "\nAdditional Info:" << std::endl;
                std::cerr << "  Your requested sample rate (" << sampleRate
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
        std::cerr << "========================================" << std::endl;
        std::cerr << "Output Audio Format:" << std::endl;
        if (needsResampling) {
            std::cerr << "  Sample Rate: " << pOutputFormat->nSamplesPerSec << " Hz" << std::endl;
            std::cerr << "  Channels:    " << pOutputFormat->nChannels << std::endl;
            std::cerr << "  Bit Depth:   " << pOutputFormat->wBitsPerSample << " bits" << std::endl;
        } else {
            std::cerr << "  Sample Rate: " << pwfx->nSamplesPerSec << " Hz" << std::endl;
            std::cerr << "  Channels:    " << pwfx->nChannels << std::endl;
            std::cerr << "  Bit Depth:   " << pwfx->wBitsPerSample << " bits" << std::endl;
        }
        std::cerr << "========================================" << std::endl;
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
                        UINT32 outputSize = numFramesAvailable * (needsResampling ? pOutputFormat->nBlockAlign : pwfx->nBlockAlign);
                        std::vector<BYTE> silence(outputSize, 0);
                        std::cout.write(reinterpret_cast<char*>(silence.data()), silence.size());
                    } else {
                        // Process audio data
                        UINT32 inputSize = numFramesAvailable * pwfx->nBlockAlign;
                        
                        if (needsResampling && resampler) {
                            // Use resampler to convert format
                            std::vector<BYTE> outputData;
                            if (resampler->ProcessAudio(pData, inputSize, outputData)) {
                                std::cout.write(reinterpret_cast<char*>(outputData.data()), outputData.size());
                            } else {
                                // Fallback: write original data
                                std::cout.write(reinterpret_cast<char*>(pData), inputSize);
                            }
                        } else {
                            // Write original audio data to stdout
                            std::cout.write(reinterpret_cast<char*>(pData), inputSize);
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
                        // Silent buffer - write zeros
                        UINT32 outputSize = numFramesAvailable * (needsResampling ? pOutputFormat->nBlockAlign : pwfx->nBlockAlign);
                        std::vector<BYTE> silence(outputSize, 0);
                        std::cout.write(reinterpret_cast<char*>(silence.data()), silence.size());
                    } else {
                        // Process audio data
                        UINT32 inputSize = numFramesAvailable * pwfx->nBlockAlign;
                        
                        if (needsResampling && resampler) {
                            // Use resampler to convert format
                            std::vector<BYTE> outputData;
                            if (resampler->ProcessAudio(pData, inputSize, outputData)) {
                                std::cout.write(reinterpret_cast<char*>(outputData.data()), outputData.size());
                            } else {
                                // Fallback: write original data
                                std::cout.write(reinterpret_cast<char*>(pData), inputSize);
                            }
                        } else {
                            // Write original audio data to stdout
                            std::cout.write(reinterpret_cast<char*>(pData), inputSize);
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

        // Cleanup resampler
        if (resampler) {
            resampler.reset();
        }

        if (pwfx) {
            CoTaskMemFree(pwfx);
            pwfx = nullptr;
        }

        if (pOutputFormat) {
            CoTaskMemFree(pOutputFormat);
            pOutputFormat = nullptr;
        }

        SafeRelease(&pCaptureClient);
        SafeRelease(&pAudioClient);
        SafeRelease(&pDevice);
        SafeRelease(&pEnumerator);

        // Shutdown Media Foundation
        MFShutdown();

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
              << "  --channels <count>           Number of channels (default: device default)\n"
              << "  --bit-depth <bits>           Bit depth: 16, 24, or 32 (default: device default)\n"
              << "  --chunk-duration <seconds>   Duration of each audio chunk (default: 0.2)\n"
              << "  --mute                       Mute system audio while capturing\n"
              << "  --include-processes <pid>... Only capture audio from these process IDs\n"
              << "  --exclude-processes <pid>... Exclude audio from these process IDs\n"
              << "  --help                       Show this help message\n"
              << "\nExamples:\n"
              << "  wasapi_capture --sample-rate 48000 --channels 2 --bit-depth 16\n"
              << "  wasapi_capture --sample-rate 44100\n"
              << "  wasapi_capture --channels 1 --bit-depth 24\n"
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
            else if (arg == "--channels") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: --channels requires a value" << std::endl;
                    std::cerr << "Example: --channels 2" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
                try {
                    int ch = std::stoi(argv[++i]);
                    if (ch < 1 || ch > 8) {
                        std::cerr << "ERROR: Channel count out of range: " << ch << std::endl;
                        std::cerr << "Valid range: 1 - 8 channels" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetChannels(ch);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid channel count value: " << argv[i] << std::endl;
                    std::cerr << "Must be a number between 1 and 8" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
            }
            else if (arg == "--bit-depth") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: --bit-depth requires a value" << std::endl;
                    std::cerr << "Example: --bit-depth 16" << std::endl;
                    return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                }
                try {
                    int bits = std::stoi(argv[++i]);
                    if (bits != 16 && bits != 24 && bits != 32) {
                        std::cerr << "ERROR: Invalid bit depth: " << bits << std::endl;
                        std::cerr << "Valid values: 16, 24, 32 bits" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetBitDepth(bits);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid bit depth value: " << argv[i] << std::endl;
                    std::cerr << "Must be 16, 24, or 32" << std::endl;
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

