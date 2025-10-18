#pragma once

#include "../utils/common.h"
#include <mfapi.h>
#include <mfidl.h>
#include <vector>

// Audio Resampler class for format conversion
class AudioResampler {
private:
    IMFTransform* pResampler = nullptr;
    IMFMediaType* pInputType = nullptr;
    IMFMediaType* pOutputType = nullptr;
    
    WAVEFORMATEX* pInputFormat = nullptr;
    WAVEFORMATEX* pOutputFormat = nullptr;
    
    bool initialized = false;
    
    bool TryGetOutput(std::vector<BYTE>& outputData);
    
public:
    AudioResampler();
    ~AudioResampler();
    
    bool Initialize(WAVEFORMATEX* inputFormat, WAVEFORMATEX* outputFormat);
    bool ProcessAudio(const BYTE* inputData, UINT32 inputSize, std::vector<BYTE>& outputData);
    void Flush(std::vector<BYTE>& outputData);
    void Cleanup();
};

