cmake_minimum_required(VERSION 3.24)

project(core_shared_memory_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "core_shared_memory_example") 

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
	file(GLOB MINO_CMAKE_FILES "${CMAKE_SOURCE_DIR}/mino/cmake-module/*.cmake")
	foreach(FILE ${MINO_CMAKE_FILES})
		include(${FILE})
	endforeach()	
else()
	if(EXISTS "${MINO_DIR}/lib/cmake/mino")
		file(GLOB MINO_CMAKE_FILES "${CMAKE_SOURCE_DIR}/lib/cmake/mino/*.cmake")
		foreach(FILE ${MINO_CMAKE_FILES})
			include(${FILE})
		endforeach()	
	else()
	  message(FATAL_ERROR "UseMinoNetwork.cmake not found. Please check MINO_DIR path.")
	endif()
endif()
use_mino_core(${EXE_NAME} "${MINO_DIR}")

