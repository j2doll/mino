cmake_minimum_required(VERSION 3.24)

project(mino_external_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_external_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow user to specify where the mino install/build is located.
# If empty and in-tree targets named `mino_core` and `mino_external` exist, those targets will be used.
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../mino"
    CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})
if(MSVC)
    target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
endif()

# Prefer in-tree targets if available
if(TARGET mino_external)
    message(STATUS "Linking to in-tree target: mino_external")
    target_link_libraries(${EXE_NAME} PRIVATE mino_external)
elseif(TARGET mino_core)
    message(STATUS "Linking to in-tree target: mino_core")
    target_link_libraries(${EXE_NAME} PRIVATE mino_core)
else()
    message(STATUS "Linking to out-of-tree(external) targets: mino_core / mino_external")

    if(MINO_DIR)
        find_path(MINO_INCLUDE_DIR NAMES "mino/core/core.hpp"
            PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
        find_library(MINO_EXTERNAL_LIBRARY NAMES mino_external
            PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        find_library(MINO_CORE_LIBRARY NAMES mino_core
            PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
    else()
        find_path(MINO_INCLUDE_DIR NAMES "mino/core/core.hpp")
        find_library(MINO_EXTERNAL_LIBRARY NAMES mino_external)
        find_library(MINO_CORE_LIBRARY NAMES mino_core)
    endif()

    if(NOT MINO_INCLUDE_DIR OR (NOT MINO_EXTERNAL_LIBRARY AND NOT MINO_CORE_LIBRARY))
        message(FATAL_ERROR "mino libraries not found. Either add mino as a subdirectory or set -DMINO_DIR=/path/to/mino/install")
    endif()

    # Create imported targets so usage is consistent
    if(NOT TARGET mino_core::mino_core AND MINO_CORE_LIBRARY)
        add_library(mino_core::mino_core UNKNOWN IMPORTED)
        set_target_properties(mino_core::mino_core PROPERTIES
            IMPORTED_LOCATION "${MINO_CORE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
        )
    endif()

    if(NOT TARGET mino_external::mino_external AND MINO_EXTERNAL_LIBRARY)
        add_library(mino_external::mino_external UNKNOWN IMPORTED)
        set_target_properties(mino_external::mino_external PROPERTIES
            IMPORTED_LOCATION "${MINO_EXTERNAL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
        )
    endif()

    # Prefer linking to external (it already links core). Fall back to core if external not available.
    if(TARGET mino_external::mino_external)
        target_link_libraries(${EXE_NAME} PRIVATE mino_external::mino_external)
        message(STATUS "Using external mino_external from ${MINO_DIR}")
    else()
        target_link_libraries(${EXE_NAME} PRIVATE mino_core::mino_core)
        message(STATUS "Using external mino_core from ${MINO_DIR}")
    endif()
endif()

