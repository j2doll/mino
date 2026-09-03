cmake_minimum_required(VERSION 3.24)

# ===============================================
# TODO: Set MINO_DIR
#
# (1-1) Set MINO_DIR path to the installed or built mino root directory (contains include/ and lib/)
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../mino")
#
# (1-2) Or, Set custom MINO_DIR path here if needed
# set(MINO_DIR "C:/opt/mino") 
#
# NOTE: Set MINO_DIR path with the following directory structure.
# MINO_DIR
#   ├── include
#   │   └── mino
#   └── lib
#       ├── cmake
#       └── *.lib (or *.a)
# ===============================================

project(mino_network_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_network_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

# Use mino network
include("${CMAKE_CURRENT_SOURCE_DIR}/UseMinoNetwork.cmake")
use_mino_network(${EXE_NAME} "${MINO_DIR}")
