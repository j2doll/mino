cmake_minimum_required(VERSION 3.24)

# 1. Set the project name and version
project(mino_external VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ standard information setting
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. External dependency configuration

# 3-1. Threads
find_package(Threads REQUIRED)
 
# 4. Create a library and configure the source code
file(
  GLOB_RECURSE SRC_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
add_library(mino_external STATIC ${SRC_FILES})
if(MSVC)
  target_compile_options(mino_external PRIVATE "/utf-8")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_external INTERFACE -Wno-free-nonheap-object)
endif()

# 5. Specify the header file path
# Distinguish the environment at build from the environment when it is installed and deployed later.
target_include_directories(
  mino_external
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Use generator expressions for the third-party headers so an absolute
# source-tree path is not exported into the installed target's
# INTERFACE_INCLUDE_DIRECTORIES (this caused the CMake error).
target_include_directories(mino_external PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mino/external/third-party>
  $<INSTALL_INTERFACE:include/mino/external/third-party>
)

# 6. Link the external libraries being used

# 6-1. Threads link settings
target_link_libraries(mino_external PRIVATE Threads::Threads)

# 7. [Dependency] If external uses core, link it here.
target_link_libraries(mino_external PUBLIC mino_core)

# 8. Define install rules
install(
  TARGETS mino_external
  EXPORT
    minoTargets # Important: Manage all submodules under a single bundle of exports (minoTargets)
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# 9. Install public header files (copy the entire folder)
# Set the mino folder inside include/ as the target to maintain the include/mino/core/ structure.
install(DIRECTORY include/mino DESTINATION include)
