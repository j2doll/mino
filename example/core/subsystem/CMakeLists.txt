cmake_minimum_required(VERSION 3.24)

project(subsystem_ipc_example_project VERSION 0.1 LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# mino 라이브러리 루트 경로
set(MINO_DIR "${CMAKE_SOURCE_DIR}/mino")
# set(MINO_DIR "C:/opt/mino")

# 1. 서브시스템 워커 실행 파일 (example_worker)
set(WORKER_EXE "example_worker")
add_executable(${WORKER_EXE} example_worker.cpp)

# 2. 서브시스템 마스터 실행 파일 (example_master)
set(MASTER_EXE "example_master")
add_executable(${MASTER_EXE} example_master.cpp)

# mino CMake 모듈 로드
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

# mino core 모듈 의존성 연결
use_mino_core("${WORKER_EXE}" "${MINO_DIR}")
use_mino_core("${MASTER_EXE}" "${MINO_DIR}")
