cmake_minimum_required(VERSION 3.24)

project(network_log_manager_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "network_log_manager_example")

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

# CMAKE_SOURCE_DIR_PATH 변수를 main.cpp 에서 사용 가능.
target_compile_definitions(${EXE_NAME} PRIVATE CMAKE_SOURCE_DIR_PATH="${CMAKE_CURRENT_LIST_DIR}")

# [필수] logger_manager 를 사용하는 프로그램은 컴파일타임 활성 레벨을 TRACE로 설정
target_compile_definitions(${EXE_NAME} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
# NOTE:
#  SPDLOG_ACTIVE_LEVEL은 매크로를 사용한 코드 자체를
#  컴파일 단계에서 지워버릴지 말지를 결정합니다.
#  예를 들어 SPDLOG_ACTIVE_LEVEL을 SPDLOG_LEVEL_INFO로 설정하면,
#  SPDLOG_LEVEL_DEBUG나 SPDLOG_LEVEL_TRACE 레벨의 로그 코드는
#  컴파일러에 의해 아예 빌드 결과물(바이너리)에서 제외됩니다.

