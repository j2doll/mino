cmake_minimum_required(VERSION 3.24)

project(mino_network_libssh2 VERSION 1.0.0 LANGUAGES CXX)

# Standard installation paths for cross-platform compliance (Linux/macOS/Windows)
include(GNUInstallDirs)

# C++ Standard
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
find_package(Threads REQUIRED)

# Find Libssh2 via CMake Config or PkgConfig fallback
find_package(Libssh2 CONFIG QUIET)
if(NOT TARGET Libssh2::libssh2)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBSSH2 QUIET IMPORTED_TARGET libssh2)
    endif()
endif()

# Fallback check if Libssh2 is found through either method
if(NOT TARGET Libssh2::libssh2 AND NOT TARGET PkgConfig::LIBSSH2)
    message(WARNING "Libssh2 target not found automatically. Ensure it is installed via vcpkg, conan, brew, or system package manager.")
endif()

# Source Files
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Create Library
add_library(mino_network_libssh2 STATIC ${SRC_FILES})

# Include Directories
target_include_directories(
    mino_network_libssh2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Compiler Options
if(MSVC)
    target_compile_options(mino_network_libssh2 PRIVATE "/utf-8")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network_libssh2 PRIVATE -Wall -Wextra -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network_libssh2 PUBLIC stdc++fs)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options(mino_network_libssh2 PRIVATE -Wall -Wextra)
endif()

# Platform-specific configurations
if(WIN32)
    target_compile_definitions(mino_network_libssh2 PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network_libssh2 PRIVATE ws2_32 iphlpapi)
elseif(APPLE)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(SECURITY_FRAMEWORK Security)
    if(CORE_FOUNDATION_FRAMEWORK AND SECURITY_FRAMEWORK)
        target_link_libraries(mino_network_libssh2 PRIVATE ${CORE_FOUNDATION_FRAMEWORK} ${SECURITY_FRAMEWORK})
    endif()
endif()

# Target Linking  
target_link_libraries(
    mino_network_libssh2
    PUBLIC
        Threads::Threads
        mino_core
        mino_network
)

if(TARGET Libssh2::libssh2)
    target_link_libraries(mino_network_libssh2 PUBLIC Libssh2::libssh2)
elseif(TARGET PkgConfig::LIBSSH2)
    target_link_libraries(mino_network_libssh2 PUBLIC PkgConfig::LIBSSH2)
endif()

# Install Rules
install(
    TARGETS mino_network_libssh2
    EXPORT minoTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
