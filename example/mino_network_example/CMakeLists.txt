cmake_minimum_required(VERSION 3.24)

project(mino_network_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_network_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow user to specify where the mino install/build is located.
# If empty and in-tree target named `mino_network` exists, that target will be used.
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../mino"
    CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})
if(MSVC)
    target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
endif()

# Include helper and invoke it with EXE name and MINO_DIR
include("${CMAKE_CURRENT_SOURCE_DIR}/UseMinoNetwork.cmake")
use_mino_network(${EXE_NAME} "${MINO_DIR}")
