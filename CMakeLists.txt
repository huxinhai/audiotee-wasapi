cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Check for local libsamplerate source
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/CMakeLists.txt")
    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "  Using LOCAL libsamplerate source")
    message(STATUS "========================================")
    add_subdirectory(third_party/libsamplerate)
    set(LIBSAMPLERATE_FOUND TRUE)
    set(LIBSAMPLERATE_INCLUDE_DIRS 
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate/src
        ${CMAKE_BINARY_DIR}/third_party/libsamplerate
    )
else()
    message(FATAL_ERROR "
╔════════════════════════════════════════════════════════════════╗
║  ERROR: libsamplerate source not found!                        ║
╚════════════════════════════════════════════════════════════════╝

Please download and extract libsamplerate:

1. Download from:
   https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz

2. Extract to:
   third_party/libsamplerate/

3. Directory structure should be:
   third_party/libsamplerate/
   ├── CMakeLists.txt
   ├── include/
   ├── src/
   └── ...

See SETUP_LIBSAMPLERATE.md for detailed instructions.
")
endif()

# Add executable
add_executable(wasapi_capture src/wasapi_capture.cpp)

# Include directories
target_include_directories(wasapi_capture PRIVATE ${LIBSAMPLERATE_INCLUDE_DIRS})

# Link libraries - use the target created by libsamplerate's CMakeLists.txt
target_link_libraries(wasapi_capture PRIVATE
    ole32
    psapi
    samplerate
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

