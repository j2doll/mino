cmake_minimum_required(VERSION 3.24)

project(external_log_factory_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "external_log_factory_example")

# Set C++ standard to C++17 if not already defined
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow user to specify where the mino install/build is located.
# If empty and an in-tree target named `mino_external` exists, that target will be used.
set(MINO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../mino" CACHE PATH "Path to installed or built mino root (contains include/ and lib/)")

# MSVC(Windows) 경로 설정 예제 
# set("C:/opt/mino") # 사전에 Debug 빌드인지 Release 빌드인지 확인할 것

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
if(MSVC)
    target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
endif()

# Use external / in-tree mino_core via reusable module
# include("${CMAKE_CURRENT_SOURCE_DIR}/UseMinoExternal.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../template/mino_external_example/UseMinoExternal.cmake")
use_mino_external(${EXE_NAME} "${MINO_DIR}")

# third-party dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

target_link_libraries(${EXE_NAME} PRIVATE
  nlohmann_json::nlohmann_json
  spdlog::spdlog
  yaml-cpp
)


