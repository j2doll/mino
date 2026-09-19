cmake_minimum_required(VERSION 3.24)

project(mino_network_openssl VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network_openssl/*.cpp"
)

# Create Library
add_library(mino_network_openssl STATIC ${SRC_FILES})

# Include Directories & Compile Definitions
target_include_directories(
    mino_network_openssl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(mino_network_openssl PUBLIC USE_OPENSSL=1)

# Compiler & Platform Options
if(MSVC)
    target_compile_options(mino_network_openssl PRIVATE "/utf-8")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network_openssl PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network_openssl PUBLIC stdc++fs)
    endif()
endif()

if(WIN32)
    target_compile_definitions(mino_network_openssl PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network_openssl PRIVATE ws2_32 iphlpapi)
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

# Install Rules
install(
    TARGETS mino_network_openssl
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

