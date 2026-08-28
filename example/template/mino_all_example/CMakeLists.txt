cmake_minimum_required(VERSION 3.24)

# ============================================================================
# TODO: Set MINO_DIR

# (1-1) Set MINO_DIR path to the installed or built mino root directory (contains include/ and lib/)
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../mino" CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")

# (1-2) Or, Set custom MINO_DIR path here if needed
# set(MINO_DIR "C:/opt/mino") 

# NOTE: Set MINO_DIR path with the following directory structure.
# MINO_DIR
#   ├── include
#   │   └── mino
#   └── lib
#       ├── cmake
#       └── *.lib (or *.a)

if (MSVC)
  # Call these functions before project() is defined.
  include("${MINO_DIR}/use-cmake/ConfigureMsvcEnv.cmake")
  configure_vcpkg_manifest("${MINO_DIR}")
  load_cmake_settings_json("${MINO_DIR}")
endif()

project(mino_all_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_all_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

# Use mino core
include("${CMAKE_CURRENT_SOURCE_DIR}/../mino_core_example/UseMinoCore.cmake")
use_mino_core(${EXE_NAME} "${MINO_DIR}")

# Use mino network
include("${CMAKE_CURRENT_SOURCE_DIR}/../mino_network_example/UseMinoNetwork.cmake")
use_mino_network(${EXE_NAME} "${MINO_DIR}")

# Use mino external
include("${CMAKE_CURRENT_SOURCE_DIR}/../mino_external_example/UseMinoExternal.cmake")
use_mino_external(${EXE_NAME} "${MINO_DIR}")
