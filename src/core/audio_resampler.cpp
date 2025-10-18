#include "audio_resampler.h"
#include <mfreadwrite.h>
#include <mferror.h>
#include <initguid.h>
#include <iostream>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

// Define CLSID_CResamplerMediaObject manually
static const GUID CLSID_CResamplerMediaObject = {
    0xf447b69e, 0x1884, 0x4a7e, {0x80, 0x55, 0x34, 0x6f, 0x74, 0xd6, 0xed, 0xb3}
};

AudioResampler::AudioResampler() {}

AudioResampler::~AudioResampler() {
    Cleanup();
}

bool AudioResampler::Initialize(WAVEFORMATEX* inputFormat, WAVEFORMATEX* outputFormat) {
    if (!inputFormat || !outputFormat) {
        std::cerr << "Error: Invalid input or output format pointer" << std::endl;
        return false;
    }
    
    pInputFormat = inputFormat;
    pOutputFormat = outputFormat;
    
    std::cerr << "Creating audio resampler..." << std::endl;
    HRESULT hr = CoCreateInstance(CLSID_CResamplerMediaObject, nullptr, 
                                 CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pResampler));
    if (FAILED(hr)) {
        std::cerr << "Failed to create resampler COM object: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    std::cerr << "Resampler COM object created successfully" << std::endl;
    
    // Create input media type
    hr = MFCreateMediaType(&pInputType);
    if (FAILED(hr)) {
        std::cerr << "Failed to create input media type: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // Calculate the correct size for WAVEFORMATEX structure
    UINT32 waveFormatSize = sizeof(WAVEFORMATEX) + pInputFormat->cbSize;
    hr = MFInitMediaTypeFromWaveFormatEx(pInputType, pInputFormat, waveFormatSize);
    if (FAILED(hr)) {
        std::cerr << "Failed to init input media type from WAVEFORMATEX: 0x" << std::hex << hr << std::dec << std::endl;
        std::cerr << "  Format tag: " << pInputFormat->wFormatTag << std::endl;
        std::cerr << "  cbSize: " << pInputFormat->cbSize << std::endl;
        std::cerr << "  Total size: " << waveFormatSize << std::endl;
        return false;
    }
    std::cerr << "Input media type configured" << std::endl;
    
    // Create output media type
    hr = MFCreateMediaType(&pOutputType);
    if (FAILED(hr)) {
        std::cerr << "Failed to create output media type: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // Output format size (simple PCM format)
    UINT32 outputWaveFormatSize = sizeof(WAVEFORMATEX) + pOutputFormat->cbSize;
    hr = MFInitMediaTypeFromWaveFormatEx(pOutputType, pOutputFormat, outputWaveFormatSize);
    if (FAILED(hr)) {
        std::cerr << "Failed to init output media type from WAVEFORMATEX: 0x" << std::hex << hr << std::dec << std::endl;
        std::cerr << "  Format tag: " << pOutputFormat->wFormatTag << std::endl;
        std::cerr << "  cbSize: " << pOutputFormat->cbSize << std::endl;
        return false;
    }
    std::cerr << "Output media type configured" << std::endl;
    
    // Set media types
    std::cerr << "Setting input type on resampler..." << std::endl;
    hr = pResampler->SetInputType(0, pInputType, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to set input type: 0x" << std::hex << hr << std::dec << std::endl;
        std::cerr << "Input format: " << pInputFormat->nSamplesPerSec << "Hz, " 
                  << pInputFormat->nChannels << "ch, " << pInputFormat->wBitsPerSample << "bit" << std::endl;
        return false;
    }
    
    std::cerr << "Setting output type on resampler..." << std::endl;
    hr = pResampler->SetOutputType(0, pOutputType, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to set output type: 0x" << std::hex << hr << std::dec << std::endl;
        std::cerr << "Output format: " << pOutputFormat->nSamplesPerSec << "Hz, " 
                  << pOutputFormat->nChannels << "ch, " << pOutputFormat->wBitsPerSample << "bit" << std::endl;
        return false;
    }
    
    // Process messages
    std::cerr << "Sending control messages to resampler..." << std::endl;
    hr = pResampler->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to send FLUSH message: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    hr = pResampler->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to send BEGIN_STREAMING message: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    hr = pResampler->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to send START_OF_STREAM message: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    initialized = true;
    std::cerr << "Audio resampler initialized successfully!" << std::endl;
    return true;
}

bool AudioResampler::ProcessAudio(const BYTE* inputData, UINT32 inputSize, 
                                 std::vector<BYTE>& outputData) {
    if (!initialized || !inputData || inputSize == 0) {
        return false;
    }
    
    HRESULT hr;
    
    // First, try to drain all available output
    std::vector<BYTE> tempOutput;
    while (TryGetOutput(tempOutput)) {
        outputData.insert(outputData.end(), tempOutput.begin(), tempOutput.end());
        tempOutput.clear();
    }
    
    // Now try to feed input
    IMFSample* pSample = nullptr;
    IMFMediaBuffer* pBuffer = nullptr;
    
    // Create input sample and buffer
    hr = MFCreateSample(&pSample);
    if (FAILED(hr)) return !outputData.empty();
    
    hr = MFCreateMemoryBuffer(inputSize, &pBuffer);
    if (FAILED(hr)) {
        SafeRelease(&pSample);
        return !outputData.empty();
    }
    
    // Copy input data
    BYTE* pBufferData = nullptr;
    hr = pBuffer->Lock(&pBufferData, nullptr, nullptr);
    if (FAILED(hr)) {
        SafeRelease(&pBuffer);
        SafeRelease(&pSample);
        return !outputData.empty();
    }
    
    memcpy(pBufferData, inputData, inputSize);
    pBuffer->Unlock();
    pBuffer->SetCurrentLength(inputSize);
    
    // Add buffer to sample
    pSample->AddBuffer(pBuffer);
    SafeRelease(&pBuffer);
    
    // Try to process input
    hr = pResampler->ProcessInput(0, pSample, 0);
    SafeRelease(&pSample);
    
    if (FAILED(hr) && hr != MF_E_NOTACCEPTING) {
        // Unexpected error
        return !outputData.empty();
    }
    
    // Try to get more output after feeding input
    while (TryGetOutput(tempOutput)) {
        outputData.insert(outputData.end(), tempOutput.begin(), tempOutput.end());
        tempOutput.clear();
    }
    
    return true;
}

bool AudioResampler::TryGetOutput(std::vector<BYTE>& outputData) {
    MFT_OUTPUT_DATA_BUFFER outputBuffer = {};
    MFT_OUTPUT_STREAM_INFO streamInfo = {};
    IMFMediaBuffer* pBuffer = nullptr;
    
    HRESULT hr = pResampler->GetOutputStreamInfo(0, &streamInfo);
    if (FAILED(hr)) return false;
    
    // Create output sample
    hr = MFCreateSample(&outputBuffer.pSample);
    if (FAILED(hr)) return false;
    
    DWORD bufferSize = streamInfo.cbSize > 0 ? streamInfo.cbSize : 8192;
    hr = MFCreateMemoryBuffer(bufferSize, &pBuffer);
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
    } else {
        SafeRelease(&outputBuffer.pSample);
        return false;
    }
}

void AudioResampler::Flush(std::vector<BYTE>& outputData) {
    if (!initialized) return;
    
    // Send drain message
    pResampler->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    
    // Get all remaining output
    std::vector<BYTE> tempOutput;
    while (TryGetOutput(tempOutput)) {
        outputData.insert(outputData.end(), tempOutput.begin(), tempOutput.end());
        tempOutput.clear();
    }
}

void AudioResampler::Cleanup() {
    SafeRelease(&pOutputType);
    SafeRelease(&pInputType);
    SafeRelease(&pResampler);
    initialized = false;
}

