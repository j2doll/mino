cmake_minimum_required(VERSION 3.24)

# 1. Set the project name and version
#   mino's submodule mino_core
project(mino_core VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ standard information setting
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. External dependency configuration

# 3-0. Threads
find_package(Threads REQUIRED)

# 4. Create a library and configure the source code
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
add_library(mino_core STATIC ${SRC_FILES})
if(MSVC)
    target_compile_options(mino_core PRIVATE "/utf-8")
endif()

# 5. Specify the header file path
# Distinguish the environment at build from the environment when it is installed and deployed later.
target_include_directories(mino_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 6. Define an install rule
install(TARGETS mino_core 
    EXPORT minoTargets # Important: Manage all submodules under a single bundle of exports (minoTargets)
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 7. Public header file installation (copy the entire folder)
# Set the mino folder inside include/ as the target to maintain the include/mino/core/ structure.
install(DIRECTORY include/mino
    DESTINATION include
)
