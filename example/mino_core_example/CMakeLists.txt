cmake_minimum_required(VERSION 3.24)

project(mino_core_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_core_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow user to specify where the mino install/build is located.
# If empty and an in-tree target named `mino_core` exists, that target will be used.
set(MINO_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../mino/core"
 CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")
# NOTE: Set the path to the installation path with the following directory structure.
# MINO_CORE_DIR
#   ├── include
#   │   └── mino
#   └── lib
#       ├── cmake
#       └── *.lib (or *.a)

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})
if(MSVC)
    target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
endif()

# If building in-tree and the mino_core target exists, use it
if(TARGET mino_core)
  # Case A: If the mino_core target already exists in the parent project (In-tree)
  message(STATUS "Linking to in-tree target: mino_core")
  target_link_libraries(${EXE_NAME} PRIVATE mino_core)
else()
  # Case B: Explore external libraries and create virtual targets (Out-of-tree)
  message(STATUS "Linking to out-of-tree(external) target: mino_core")
  if(MINO_CORE_DIR)
      # Try to find headers and library either under MINO_CORE_DIR (if provided) or system paths
      find_path(MINO_CORE_INCLUDE_DIR NAMES "mino/core/core.hpp"
          PATHS "${MINO_CORE_DIR}/include" "${MINO_CORE_DIR}" NO_DEFAULT_PATH)
      find_library(MINO_CORE_LIBRARY NAMES mino_core
          PATHS "${MINO_CORE_DIR}/lib" "${MINO_CORE_DIR}" NO_DEFAULT_PATH)
  else()
      find_path(MINO_CORE_INCLUDE_DIR NAMES "mino/core/core.hpp")
      find_library(MINO_CORE_LIBRARY NAMES mino_core)
  endif()

  if(NOT MINO_CORE_INCLUDE_DIR OR NOT MINO_CORE_LIBRARY)
      message(FATAL_ERROR "mino_core not found. Either add mino as a subdirectory or set -DMINO_CORE_DIR=/path/to/mino/install")
  endif()

  # Create an imported target to keep usage consistent
  add_library(mino_core::mino_core UNKNOWN IMPORTED)
  set_target_properties(mino_core::mino_core PROPERTIES
      IMPORTED_LOCATION "${MINO_CORE_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${MINO_CORE_INCLUDE_DIR}"
  )

  target_link_libraries(${EXE_NAME} PRIVATE mino_core::mino_core)
  message(STATUS "Using external mino_core from ${MINO_CORE_DIR}")
endif()
