cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# Set policy for libsamplerate compatibility
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

# Fetch libsamplerate
FetchContent_Declare(
  libsamplerate
  GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
  GIT_TAG master  # Use latest master branch for CMake compatibility
)

# Make libsamplerate available
FetchContent_MakeAvailable(libsamplerate)

# Print available targets for debugging
get_directory_property(all_targets BUILDSYSTEM_TARGETS)
message(STATUS "Available targets: ${all_targets}")

# Add executable
add_executable(wasapi_capture src/wasapi_capture.cpp)

# Include directories - libsamplerate headers
target_include_directories(wasapi_capture PRIVATE 
    ${libsamplerate_SOURCE_DIR}/include
    ${libsamplerate_SOURCE_DIR}/src
    ${libsamplerate_BINARY_DIR}
)

# Add library search paths
link_directories(${libsamplerate_BINARY_DIR}/src)
link_directories(${libsamplerate_BINARY_DIR}/src/Release)
link_directories(${libsamplerate_BINARY_DIR}/src/Debug)

# Link required Windows libraries and libsamplerate
# Debug: check which targets exist
if(TARGET samplerate)
    message(STATUS "✓ Found CMake target: 'samplerate'")
    target_link_libraries(wasapi_capture PRIVATE ole32 psapi samplerate)
elseif(TARGET libsamplerate)
    message(STATUS "✓ Found CMake target: 'libsamplerate'")
    target_link_libraries(wasapi_capture PRIVATE ole32 psapi libsamplerate)
elseif(TARGET SampleRate::samplerate)
    message(STATUS "✓ Found CMake target: 'SampleRate::samplerate'")
    target_link_libraries(wasapi_capture PRIVATE ole32 psapi SampleRate::samplerate)
else()
    message(STATUS "⚠ No CMake target found, linking by library name")
    message(STATUS "  Library search paths: ${libsamplerate_BINARY_DIR}/src")
    target_link_libraries(wasapi_capture PRIVATE
        ole32
        psapi
        samplerate
    )
endif()

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

