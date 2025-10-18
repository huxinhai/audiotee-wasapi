#include "wasapi_capture.h"
#include "../utils/error_handler.h"
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <functiondiscoverykeys_devpkey.h>
#include <mfapi.h>
#include <propvarutil.h>

#pragma comment(lib, "ole32.lib")

WASAPICapture::WASAPICapture() {}

WASAPICapture::~WASAPICapture() {
    Cleanup();
}

void WASAPICapture::SetSampleRate(int rate) { sampleRate = rate; }
void WASAPICapture::SetChannels(int ch) { channels = ch; }
void WASAPICapture::SetBitDepth(int bits) { bitDepth = bits; }
void WASAPICapture::SetChunkDuration(double duration) { chunkDuration = duration; }
void WASAPICapture::SetMute(bool m) { mute = m; }
void WASAPICapture::AddIncludeProcess(DWORD pid) { includeProcesses.push_back(pid); }
void WASAPICapture::AddExcludeProcess(DWORD pid) { excludeProcesses.push_back(pid); }

ErrorCode WASAPICapture::Initialize() {
    std::cerr << "Initializing WASAPI Audio Capture..." << std::endl;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        ErrorHandler::PrintDetailedError(hr, "Failed to initialize COM library");
        return ErrorCode::COM_INIT_FAILED;
    }

    // Initialize Media Foundation
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        ErrorHandler::PrintDetailedError(hr, "Failed to initialize Media Foundation");
        return ErrorCode::COM_INIT_FAILED;
    }

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                         __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        ErrorHandler::PrintDetailedError(hr, "Failed to create audio device enumerator");
        std::cerr << "\nAdditional Info:" << std::endl;
        std::cerr << "  This error usually means Windows Audio components are not properly installed." << std::endl;
        return ErrorCode::NO_AUDIO_DEVICE;
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
        return ErrorCode::NO_AUDIO_DEVICE;
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
        return ErrorCode::DEVICE_ACCESS_DENIED;
    }

    // Get mix format
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        ErrorHandler::PrintDetailedError(hr, "Failed to get audio format");
        std::cerr << "\nAdditional Info:" << std::endl;
        std::cerr << "  Could not query device audio format. Driver may be corrupted." << std::endl;
        return ErrorCode::AUDIO_FORMAT_NOT_SUPPORTED;
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
        return ErrorCode::INVALID_PARAMETER;
    }

    if (channels > 0 && (channels < 1 || channels > 2)) {
        std::cerr << "\nERROR: Invalid channel count: " << channels << std::endl;
        std::cerr << "Valid values: 1 (mono), 2 (stereo)" << std::endl;
        return ErrorCode::INVALID_PARAMETER;
    }

    if (bitDepth > 0 && (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)) {
        std::cerr << "\nERROR: Invalid bit depth: " << bitDepth << std::endl;
        std::cerr << "Valid values: 16, 24, 32 bits" << std::endl;
        return ErrorCode::INVALID_PARAMETER;
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

        // Create output format as standard PCM
        pOutputFormat = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        if (!pOutputFormat) {
            std::cerr << "Failed to allocate memory for output format" << std::endl;
            return ErrorCode::INSUFFICIENT_BUFFER;
        }

        // Set up as standard PCM format (not extensible)
        ZeroMemory(pOutputFormat, sizeof(WAVEFORMATEX));
        pOutputFormat->wFormatTag = WAVE_FORMAT_PCM;
        pOutputFormat->nChannels = targetChannels;
        pOutputFormat->nSamplesPerSec = targetSampleRate;
        pOutputFormat->wBitsPerSample = targetBitDepth;
        pOutputFormat->nBlockAlign = (targetChannels * targetBitDepth) / 8;
        pOutputFormat->nAvgBytesPerSec = targetSampleRate * pOutputFormat->nBlockAlign;
        pOutputFormat->cbSize = 0;  // No extra data for standard PCM

        // Initialize resampler
        resampler = std::make_unique<AudioResampler>();
        if (!resampler->Initialize(pwfx, pOutputFormat)) {
            std::cerr << "Failed to initialize audio resampler" << std::endl;
            return ErrorCode::AUDIO_FORMAT_NOT_SUPPORTED;
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
        return ErrorCode::INVALID_PARAMETER;
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

        return ErrorCode::AUDIO_FORMAT_NOT_SUPPORTED;
    }

    // Get buffer size
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    if (FAILED(hr)) {
        ErrorHandler::PrintDetailedError(hr, "Failed to get audio buffer size");
        return ErrorCode::INSUFFICIENT_BUFFER;
    }

    std::cerr << "Buffer size: " << bufferFrameCount << " frames ("
              << (double)bufferFrameCount / pwfx->nSamplesPerSec * 1000
              << " ms)" << std::endl;

    // Get capture client
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr)) {
        ErrorHandler::PrintDetailedError(hr, "Failed to get capture client service");
        return ErrorCode::DEVICE_ACCESS_DENIED;
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

    return ErrorCode::SUCCESS;
}

ErrorCode WASAPICapture::StartCapture() {
    if (!pAudioClient) return ErrorCode::DEVICE_ACCESS_DENIED;

    // Create event for audio buffer ready notification
    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hEvent == nullptr) {
        std::cerr << "Failed to create event" << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }

    // Set event handle for buffer notifications
    HRESULT hr = pAudioClient->SetEventHandle(hEvent);
    if (FAILED(hr)) {
        std::cerr << "Failed to set event handle, falling back to polling mode" << std::endl;
        CloseHandle(hEvent);
        return StartCapturePolling();
    }

    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "Failed to start audio client" << std::endl;
        CloseHandle(hEvent);
        return ErrorCode::DEVICE_ACCESS_DENIED;
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
                            // Only write if we got output data
                            if (!outputData.empty()) {
                                std::cout.write(reinterpret_cast<char*>(outputData.data()), outputData.size());
                            }
                            // If outputData is empty, resampler is buffering - this is normal
                        } else {
                            std::cerr << "Warning: Resampler ProcessAudio failed, skipping frame" << std::endl;
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
    
    // Flush resampler if needed
    if (needsResampling && resampler) {
        std::vector<BYTE> finalData;
        resampler->Flush(finalData);
        if (!finalData.empty()) {
            std::cout.write(reinterpret_cast<char*>(finalData.data()), finalData.size());
            std::cout.flush();
        }
    }
    
    CloseHandle(hEvent);
    return ErrorCode::SUCCESS;
}

// Fallback polling mode
ErrorCode WASAPICapture::StartCapturePolling() {
    if (!pAudioClient) return ErrorCode::DEVICE_ACCESS_DENIED;

    HRESULT hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "Failed to start audio client" << std::endl;
        return ErrorCode::DEVICE_ACCESS_DENIED;
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
                            // Only write if we got output data
                            if (!outputData.empty()) {
                                std::cout.write(reinterpret_cast<char*>(outputData.data()), outputData.size());
                            }
                            // If outputData is empty, resampler is buffering - this is normal
                        } else {
                            std::cerr << "Warning: Resampler ProcessAudio failed, skipping frame" << std::endl;
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
    
    // Flush resampler if needed
    if (needsResampling && resampler) {
        std::vector<BYTE> finalData;
        resampler->Flush(finalData);
        if (!finalData.empty()) {
            std::cout.write(reinterpret_cast<char*>(finalData.data()), finalData.size());
            std::cout.flush();
        }
    }
    
    return ErrorCode::SUCCESS;
}

void WASAPICapture::Stop() {
    running = false;
}

void WASAPICapture::Cleanup() {
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

bool WASAPICapture::ShouldCaptureProcess(DWORD processId) {
    // If include list is specified, only capture from those processes
    if (!includeProcesses.empty()) {
        for (DWORD pid : includeProcesses) {
            if (pid == processId) return true;
        }
        return false;
    }

    // If exclude list is specified, don't capture from those processes
    if (!excludeProcesses.empty()) {
        for (DWORD pid : excludeProcesses) {
            if (pid == processId) return false;
        }
    }

    return true;
}

