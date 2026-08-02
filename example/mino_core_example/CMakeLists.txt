cmake_minimum_required(VERSION 3.24)

project(mino_core_example VERSION 0.1 LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow user to specify where the mino install/build is located.
# If empty and an in-tree target named `mino_core` exists, that target will be used.
set(MINO_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../mino/core"
 CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")

add_executable(mino_core_example src/main.cpp)

# If building in-tree and the mino_core target exists, use it
if(TARGET mino_core)
    message(STATUS "Linking to in-tree target: mino_core")
    target_link_libraries(mino_core_example PRIVATE mino_core)
else()
    # Try to find headers and library either under MINO_CORE_DIR (if provided) or system paths
    if(MINO_CORE_DIR)
        find_path(MINO_CORE_INCLUDE_DIR NAMES "mino/core/pfr/pfr.hpp"
            PATHS "${MINO_CORE_DIR}/include" "${MINO_CORE_DIR}" NO_DEFAULT_PATH)
        find_library(MINO_CORE_LIBRARY NAMES mino_core
            PATHS "${MINO_CORE_DIR}/lib" "${MINO_CORE_DIR}" NO_DEFAULT_PATH)
    else()
        find_path(MINO_CORE_INCLUDE_DIR NAMES "mino/core/pfr/pfr.hpp")
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

    target_link_libraries(mino_core_example PRIVATE mino_core::mino_core)
    message(STATUS "Using external mino_core from ${MINO_CORE_DIR}")
endif()
