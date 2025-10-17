cmake_minimum_required(VERSION 3.15)
project(WASAPICapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# Fetch libsamplerate
FetchContent_Declare(
  libsamplerate
  GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
  GIT_TAG master  # Use latest master branch for CMake compatibility
)

# Set policy for libsamplerate compatibility
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

# Make libsamplerate available
FetchContent_MakeAvailable(libsamplerate)

# Add executable
add_executable(wasapi_capture src/wasapi_capture.cpp)

# Include directories - libsamplerate headers
target_include_directories(wasapi_capture PRIVATE 
    ${libsamplerate_SOURCE_DIR}/include
    ${libsamplerate_SOURCE_DIR}/src
    ${libsamplerate_BINARY_DIR}
)

# Link required Windows libraries and libsamplerate
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

