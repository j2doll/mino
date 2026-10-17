cmake_minimum_required(VERSION 3.24)

project(mino_network_curl VERSION 1.0.0 LANGUAGES CXX)

# ==============================================================================
# C++ Standard Configuration
# ==============================================================================
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Platform-Specific Search Hints
# ==============================================================================
# Help CMake find Homebrew-installed OpenSSL/CURL on macOS (Apple Silicon & Intel)
if(APPLE)
    if(EXISTS "/opt/homebrew")
        list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    elseif(EXISTS "/usr/local")
        list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
    endif()
endif()

# ==============================================================================
# Dependencies
# ==============================================================================
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Brotli Dependency Discovery (Handles vcpkg and native package managers)
set(MINO_BROTLI_AVAILABLE FALSE)
if(DEFINED VCPKG_ROOT OR (DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg"))
    find_package(unofficial-brotli CONFIG QUIET)
    if(TARGET unofficial::brotli::brotlidec)
        set(MINO_BROTLI_AVAILABLE TRUE)
    endif()
else()
    find_package(Brotli CONFIG QUIET)
    if(NOT Brotli_FOUND)
        find_package(brotli CONFIG QUIET)
    endif()
    if(TARGET Brotli::brotlidec OR TARGET brotli::brotlidec)
        set(MINO_BROTLI_AVAILABLE TRUE)
    endif()
endif()

# ==============================================================================
# Source Files
# ==============================================================================
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# ==============================================================================
# Target Definition
# ==============================================================================
add_library(mino_network_curl STATIC ${SRC_FILES})

# Include Directories & Preprocessor Definitions
target_include_directories(
    mino_network_curl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(mino_network_curl PUBLIC USE_CURL)

# ==============================================================================
# Compiler & Platform Specific Settings
# ==============================================================================
# MSVC (Windows)
if(MSVC)
    target_compile_options(mino_network_curl PRIVATE "/utf-8")
endif()

# GCC (Linux)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network_curl PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    # std::filesystem requires stdc++fs on GCC versions older than 9.0
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network_curl PUBLIC stdc++fs)
    endif()
endif()

# Clang / AppleClang (macOS, Linux)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(mino_network_curl PUBLIC -Wno-deprecated-declarations)
endif()

# Windows System Libraries & Definitions
if(WIN32)
    target_compile_definitions(mino_network_curl PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network_curl PRIVATE ws2_32 iphlpapi)
endif()

# ==============================================================================
# Target Linking
# ==============================================================================
target_link_libraries(
    mino_network_curl
    PUBLIC
        Threads::Threads
        CURL::libcurl
        mino_core
        mino_network
    PRIVATE
        OpenSSL::Crypto
)

# Link Brotli target dynamically depending on resolution
if(MINO_BROTLI_AVAILABLE)
    if(TARGET unofficial::brotli::brotlidec)
        target_link_libraries(mino_network_curl PRIVATE unofficial::brotli::brotlidec)
    elseif(TARGET Brotli::brotlidec)
        target_link_libraries(mino_network_curl PRIVATE Brotli::brotlidec)
    elseif(TARGET brotli::brotlidec)
        target_link_libraries(mino_network_curl PRIVATE brotli::brotlidec)
    endif()
endif()

# ==============================================================================
# Install Rules
# ==============================================================================
install(
    TARGETS mino_network_curl
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
