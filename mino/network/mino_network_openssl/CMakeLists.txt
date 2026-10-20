cmake_minimum_required(VERSION 3.24)

project(mino_network_openssl VERSION 1.0.0 LANGUAGES CXX)

# Include standard installation directory module
include(GNUInstallDirs)

# C++ Standard Configuration
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOS: Add hints for Homebrew/MacPorts OpenSSL installations if not specified
if(APPLE AND NOT DEFINED OPENSSL_ROOT_DIR)
    if(EXISTS "/opt/homebrew/opt/openssl@3")
        set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3")
    elseif(EXISTS "/opt/homebrew/opt/openssl")
        set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl")
    elseif(EXISTS "/usr/local/opt/openssl@3")
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@3")
    elseif(EXISTS "/usr/local/opt/openssl")
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    endif()
endif()

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

if(TARGET crypto AND TARGET ssl)
    add_library(OpenSSL::Crypto ALIAS crypto)
    add_library(OpenSSL::SSL ALIAS ssl)
endif()

# Source Files
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Create Library
add_library(mino_network_openssl STATIC ${SRC_FILES})

#-------------------------------------------------------------
# Include Directories for Third-Party Libraries
target_include_directories(mino_network_openssl SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party
)

# Include Directories & Compile Definitions
target_include_directories(
    mino_network_openssl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(mino_network_openssl PUBLIC USE_OPENSSL=1)

# Compiler & Platform-Specific Configurations
if(MSVC)
    target_compile_options(mino_network_openssl PRIVATE "/utf-8")
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(mino_network_openssl PRIVATE Threads::Threads rt)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
		target_link_libraries(mino_network_openssl PRIVATE stdc++fs)
	endif()
else()
	target_link_libraries(mino_network_openssl PRIVATE Threads::Threads)
endif()

# GCC / Clang / AppleClang options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network_openssl PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network_openssl PUBLIC stdc++fs)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(mino_network_openssl PUBLIC -Wno-deprecated-literal-operator)
endif()

# Platform-specific definitions and dependencies
if(WIN32)
    target_compile_definitions(mino_network_openssl PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network_openssl PRIVATE ws2_32 iphlpapi)
elseif(APPLE)
    # macOS system frameworks if needed by networking/security
    find_library(SECURITY_FRAMEWORK Security)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    if(SECURITY_FRAMEWORK)
        target_link_libraries(mino_network_openssl PRIVATE ${SECURITY_FRAMEWORK})
    endif()
    if(COREFOUNDATION_FRAMEWORK)
        target_link_libraries(mino_network_openssl PRIVATE ${COREFOUNDATION_FRAMEWORK})
    endif()
elseif(UNIX)
    # Generic POSIX / Linux dependencies
    # (Threads library is handled by Threads::Threads)
endif()

# Target Linking  
target_link_libraries(
    mino_network_openssl
    PUBLIC
        Threads::Threads
        mino_core
        mino_network
)

if(TARGET OpenSSL::Crypto AND TARGET OpenSSL::SSL)
    target_link_libraries(mino_network_openssl PUBLIC OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(mino_network_openssl PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

# Installation Rules
install(
    TARGETS mino_network_openssl
    EXPORT minoTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# Install third-party directory to the install prefix root: <prefix>/third-party
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/"
  DESTINATION third-party
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)


