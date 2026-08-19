cmake_minimum_required(VERSION 3.24)

project(core_processutil_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "core_processutil_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow user to specify where the mino install/build is located.
# If empty and an in-tree target named `mino_core` exists, that target will be used.
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../mino/core" CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")

# NOTE: Set the path to the installation path with the following directory structure.
# MINO_DIR
#   ├── include
#   │   └── mino
#   └── lib
#       ├── cmake
#       └── *.lib (or *.a)

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

# Use external / in-tree mino_core via reusable module
include("${CMAKE_CURRENT_SOURCE_DIR}/../../template/mino_core_example/UseMinoCore.cmake")
use_mino_core(${EXE_NAME} "${MINO_DIR}")
