cmake_minimum_required(VERSION 3.24)

project(network_udp_sender_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "network_udp_sender_example")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MINO_DIR "${CMAKE_SOURCE_DIR}/mino")
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
use_mino_network(${EXE_NAME} "${MINO_DIR}")

