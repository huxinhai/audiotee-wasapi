#pragma once

#include "common.h"

// Detailed error information helper
class ErrorHandler {
public:
    static void PrintDetailedError(HRESULT hr, const char* context);
    static void CheckSystemRequirements();
};

