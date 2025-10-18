#pragma once

#include "../utils/common.h"
#include "audio_resampler.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <memory>

// WASAPI Audio Capture class
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
    WASAPICapture();
    ~WASAPICapture();

    // Configuration setters
    void SetSampleRate(int rate);
    void SetChannels(int ch);
    void SetBitDepth(int bits);
    void SetChunkDuration(double duration);
    void SetMute(bool m);
    void AddIncludeProcess(DWORD pid);
    void AddExcludeProcess(DWORD pid);

    // Main operations
    ErrorCode Initialize();
    ErrorCode StartCapture();
    ErrorCode StartCapturePolling();
    void Stop();
    void Cleanup();

private:
    bool ShouldCaptureProcess(DWORD processId);
};

