cmake_minimum_required(VERSION 3.24)

# 1. Project name and version setup
#    Submodule: mino_core
project(mino_core VERSION 1.0.0 LANGUAGES C CXX)

# 2. C++ standard configuration
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. External dependencies
# 3-1. Threads support
find_package(Threads REQUIRED)

# 4. Target library configuration and source collection
# 4-1. Collect source files and create static library
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
add_library(mino_core STATIC ${SRC_FILES})

# 4-2. MSVC compiler settings
if(MSVC)
    target_compile_options(mino_core PRIVATE "/utf-8")
endif()

# 4-3. GCC-specific compiler flags and backward compatibility
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_core PRIVATE -Wno-free-nonheap-object)

    # GCC 8.x std::filesystem support
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_core PUBLIC stdc++fs)
    endif()
endif()

# 4-4. Link threading library
target_link_libraries(mino_core PUBLIC Threads::Threads)

# 5. Header search paths
# Distinguish build directory paths from installed deployment paths
target_include_directories(
    mino_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 6. Installation rules for library targets
install(
    TARGETS mino_core
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 7. Public header installation
# Preserves the directory hierarchy: include/mino/...
install(DIRECTORY include/mino DESTINATION include)
