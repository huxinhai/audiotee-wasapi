#include "error_handler.h"
#include <audioclient.h>
#include <iostream>
#include <comdef.h>

void ErrorHandler::PrintDetailedError(HRESULT hr, const char* context) {
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

void ErrorHandler::CheckSystemRequirements() {
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

