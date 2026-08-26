cmake_minimum_required(VERSION 3.24)

# ============================================================================
# vcpkg.json 
if (MSVC)
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "Path to vcpkg manifest directory" FORCE)
    # set(VCPKG_MANIFEST_DIR "C:/opt/mino/vcpkg.json")
    set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Enable vcpkg manifest mode" FORCE)
endif()
# ============================================================================

project(mino_network_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_network_example")

# ============================================================================
# CMakeSettings.json 
set(SETTINGS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../CMakeSettings.json")
# set(SETTINGS_PATH "C:/opt/mino/CMakeSettings.json")
if(EXISTS "${SETTINGS_PATH}")
    file(READ "${SETTINGS_PATH}" JSON_CONTENT)

    string(JSON VAR_COUNT ERROR_VARIABLE JSON_ERR LENGTH "${JSON_CONTENT}" "configurations" 0 "variables")
    
    if(NOT JSON_ERR AND VAR_COUNT GREATER 0)
        math(EXPR VAR_LAST "${VAR_COUNT} - 1")
        foreach(INDEX RANGE ${VAR_LAST})
            string(JSON V_NAME GET "${JSON_CONTENT}" "configurations" 0 "variables" ${INDEX} "name")
            string(JSON V_VALUE GET "${JSON_CONTENT}" "configurations" 0 "variables" ${INDEX} "value")
            
            if(V_VALUE MATCHES [[\$\{env\.([^}]+)\}]])
                set(ENV_VAR_NAME "${CMAKE_MATCH_1}")
                set(ENV_VAR_VAL "$ENV{${ENV_VAR_NAME}}")
                string(REPLACE "${CMAKE_MATCH_0}" "${ENV_VAR_VAL}" V_VALUE "${V_VALUE}")
            endif()

            if(V_VALUE MATCHES [[\$\{projectDir\}]])
                set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
                string(REPLACE "${CMAKE_MATCH_0}" "${PROJECT_ROOT}" V_VALUE "${V_VALUE}")
            endif()
            
            set(${V_NAME} "${V_VALUE}" CACHE STRING "Loaded from CMakeSettings.json" FORCE)
            message(STATUS "[CMakeSettings.json] Loaded: ${V_NAME} = ${V_VALUE}")
        endforeach()
    endif()
endif()
# ============================================================================

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set MINO_DIR
#
# set(MINO_DIR "C:/opt/mino")
# 
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../mino" CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")
#
# NOTE: Set MINO_DIR path with the following directory structure.
# MINO_DIR
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

# Use mino network
include("${CMAKE_CURRENT_SOURCE_DIR}/UseMinoNetwork.cmake")
use_mino_network(${EXE_NAME} "${MINO_DIR}")
