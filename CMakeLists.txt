cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable with all source files
add_executable(wasapi_capture
    src/main.cpp
    src/core/wasapi_capture_impl.cpp
    src/core/audio_resampler.cpp
    src/utils/error_handler.cpp
)

# Link required Windows libraries
target_link_libraries(wasapi_capture
    ole32
    psapi
    mf
    mfplat
    mfreadwrite
)

# Set output directory
set_target_properties(wasapi_capture PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Windows specific settings
if(MSVC)
    target_compile_options(wasapi_capture PRIVATE /W4)
    # Set subsystem to console
    set_target_properties(wasapi_capture PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

