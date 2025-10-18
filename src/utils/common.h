#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// Safe release macro for COM objects
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

