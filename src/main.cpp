#include "core/wasapi_capture.h"
#include "utils/error_handler.h"
#include <iostream>
#include <string>

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
              << "  --channels <count>           Number of channels: 1=mono, 2=stereo (default: device default)\n"
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
                    if (ch < 1 || ch > 2) {
                        std::cerr << "ERROR: Channel count out of range: " << ch << std::endl;
                        std::cerr << "Valid values: 1 (mono), 2 (stereo)" << std::endl;
                        return static_cast<int>(ErrorCode::INVALID_PARAMETER);
                    }
                    capture.SetChannels(ch);
                } catch (const std::exception& e) {
                    std::cerr << "ERROR: Invalid channel count value: " << argv[i] << std::endl;
                    std::cerr << "Must be 1 or 2" << std::endl;
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
    ErrorCode result = capture.Initialize();
    if (result != ErrorCode::SUCCESS) {
        std::cerr << "\n!!! INITIALIZATION FAILED !!!" << std::endl;
        std::cerr << "Please check the error messages above for details." << std::endl;
        std::cerr << "Common solutions:" << std::endl;
        std::cerr << "  1. Make sure audio device is working (play some music)" << std::endl;
        std::cerr << "  2. Try running as Administrator" << std::endl;
        std::cerr << "  3. Update audio drivers" << std::endl;
        std::cerr << "  4. Restart Windows Audio service" << std::endl;
        return static_cast<int>(result);
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

