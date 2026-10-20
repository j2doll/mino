cmake_minimum_required(VERSION 3.24)

project(network_log_manager_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "network_log_manager_example")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED MINO_DIR)
   set(MINO_DIR "${CMAKE_SOURCE_DIR}/mino")
endif()
# set(MINO_DIR "C:/opt/mino") 

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

if(EXISTS "${CMAKE_SOURCE_DIR}/mino/cmake-module")
    file(GLOB MINO_CMAKE_FILES "${CMAKE_SOURCE_DIR}/mino/cmake-module/Use*.cmake")
    foreach(FILE ${MINO_CMAKE_FILES})
        include(${FILE})
    endforeach()    
else()
    if(EXISTS "${MINO_DIR}/lib/cmake/mino")
        file(GLOB MINO_CMAKE_FILES "${MINO_DIR}/lib/cmake/mino/Use*.cmake")
        foreach(FILE ${MINO_CMAKE_FILES})
            include(${FILE})
        endforeach()    
    else()
        message(FATAL_ERROR "Please check MINO_DIR path.")
    endif()
endif()
# It must be called in reverse order of the dependencies.
use_mino_network(${EXE_NAME} "${MINO_DIR}")
use_mino_core(${EXE_NAME} "${MINO_DIR}")

# The CMAKE_SOURCE_DIR_PATH variable can be used in main.cpp.
target_compile_definitions(${EXE_NAME} PRIVATE CMAKE_SOURCE_DIR_PATH="${CMAKE_CURRENT_LIST_DIR}")

# [Required] Programs using logger_manager must set the compile-time active level to TRACE
target_compile_definitions(${EXE_NAME} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
# NOTE:
# SPDLOG_ACTIVE_LEVEL determines whether code utilizing these macros is stripped out during the compilation phase.
# For example, if you set SPDLOG_ACTIVE_LEVEL to SPDLOG_LEVEL_INFO, any logging code at the SPDLOG_LEVEL_DEBUG or SPDLOG_LEVEL_TRACE levels will be completely excluded from the final build artifact (the binary) by the compiler.


