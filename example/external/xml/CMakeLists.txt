cmake_minimum_required(VERSION 3.24)

project(external_xml_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "external_xml_example")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../mino")
# set("C:/opt/mino") 
# NOTE: Set the path to the installation path with the following directory structure.
# MINO_DIR
#   ├── include
#   │   └── mino
#   └── lib
#       ├── cmake
#       └── *.lib (or *.a)

# Gather all .cpp files in the current directory
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

# Use mino external
include("${CMAKE_CURRENT_SOURCE_DIR}/../../template/mino_external_example/UseMinoExternal.cmake")
use_mino_external(${EXE_NAME} "${MINO_DIR}")

target_compile_definitions(${EXE_NAME} PRIVATE CMAKE_SOURCE_DIR_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

