cmake_minimum_required(VERSION 3.24)

project(mino_network_curl VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

set(MINO_BROTLI_AVAILABLE FALSE)
if(DEFINED VCPKG_ROOT OR (DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg"))
    find_package(unofficial-brotli CONFIG REQUIRED)
    set(MINO_BROTLI_AVAILABLE TRUE)
else()
    find_package(Brotli CONFIG QUIET)
    if(BROTLI_FOUND)
        set(MINO_BROTLI_AVAILABLE TRUE)
    else()
        find_package(brotli CONFIG QUIET)
        if(BROTLI_FOUND)
            set(MINO_BROTLI_AVAILABLE TRUE)
        endif()
    endif()
endif()

# Source Files
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Create Library
add_library(mino_network_curl STATIC ${SRC_FILES})

# Include Directories & Compile Definitions
target_include_directories(
    mino_network_curl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(mino_network_curl PUBLIC USE_CURL)

# Compiler & Platform Options
if(MSVC)
    target_compile_options(mino_network_curl PRIVATE "/utf-8")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network_curl PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network_curl PUBLIC stdc++fs)
    endif()
endif()

if(WIN32)
    target_compile_definitions(mino_network_curl PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network_curl PRIVATE ws2_32 iphlpapi)
endif()

# Target Linking 
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

if(MINO_BROTLI_AVAILABLE AND TARGET unofficial::brotli::brotlidec)
    target_link_libraries(mino_network_curl PRIVATE unofficial::brotli::brotlidec)
endif()

# Install Rules
install(
    TARGETS mino_network_curl
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
