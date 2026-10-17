cmake_minimum_required(VERSION 3.24)

project(mino_network VERSION 1.0.0 LANGUAGES CXX)

# Default to C++17 if not already specified
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Standard install directory variables (GNUInstallDirs)
include(GNUInstallDirs)

# 1. External Dependencies
find_package(Threads REQUIRED)

# 2. Source Files
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# 3. Create Library
add_library(mino_network STATIC ${SRC_FILES})

# 4. Include Directories
target_include_directories(
    mino_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# 5. Compiler & Platform Specific Settings

# MSVC specific flags
if(MSVC)
    target_compile_options(mino_network PRIVATE "/utf-8")
endif()

# GCC specific settings & std::filesystem link requirements (< GCC 9)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network PUBLIC stdc++fs)
    endif()
endif()

# Clang / AppleClang specific settings (< Clang 9.0 filesystem support)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network PUBLIC c++fs)
    endif()
endif()

# Platform: Windows
if(WIN32)
    target_compile_definitions(mino_network PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network PRIVATE ws2_32 iphlpapi)
endif()

# Platform: macOS
if(APPLE)
    # Required system frameworks or settings for macOS network/system APIs if needed
    # e.g., target_link_libraries(mino_network PRIVATE "-framework CoreFoundation")
endif()

# Platform: Linux / UNIX (excluding Apple)
if(UNIX AND NOT APPLE)
    # Linux-specific network or real-time libraries (if needed)
    # e.g., target_link_libraries(mino_network PRIVATE rt)
endif()

# 6. Target Linking
target_link_libraries(
    mino_network
    PUBLIC
        Threads::Threads
        mino_core
)

# 7. Install Rules
install(
    TARGETS mino_network
    EXPORT minoTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
