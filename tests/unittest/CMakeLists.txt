cmake_minimum_required(VERSION 3.24)

project(mino_unittest_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_unittest")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MINO_DIR "${CMAKE_SOURCE_DIR}/mino")
# set(MINO_DIR "C:/opt/mino")  

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

target_include_directories(${EXE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(${EXE_NAME} PRIVATE CATCH_AMALGAMATED_CUSTOM_MAIN)

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
use_mino_core(${EXE_NAME} "${MINO_DIR}")
use_mino_network(${EXE_NAME} "${MINO_DIR}")
use_mino_external(${EXE_NAME} "${MINO_DIR}")


