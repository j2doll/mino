cmake_minimum_required(VERSION 3.24)

project(network_interface_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "network_interface_example")

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
  include("${CMAKE_SOURCE_DIR}/mino/cmake-module/UseMinoCore.cmake")
  include("${CMAKE_SOURCE_DIR}/mino/cmake-module/UseMinoNetwork.cmake")
  include("${CMAKE_SOURCE_DIR}/mino/cmake-module/UseMinoExternal.cmake")  
else()
	if(EXISTS "${MINO_DIR}/lib/cmake/mino")
		include("${MINO_DIR}/lib/cmake/mino/UseMinoCore.cmake")
		include("${MINO_DIR}/lib/cmake/mino/UseMinoNetwork.cmake")
		include("${MINO_DIR}/lib/cmake/mino/UseMinoExternal.cmake")
	else()
	  message(FATAL_ERROR "UseMinoNetwork.cmake not found. Please check MINO_DIR path.")
	endif()
endif()
use_mino_network(${EXE_NAME} "${MINO_DIR}")

