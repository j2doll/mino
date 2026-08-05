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

# 3-0. Threads
find_package(Threads REQUIRED)

# 3-1. spdlog
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")

# 3-2. nlohmann json
find_package(nlohmann_json REQUIRED)
message(STATUS "nlohmann_json version: ${nlohmann_json_VERSION}")

# 3-3. yaml-cpp
find_package(yaml-cpp REQUIRED)
message(STATUS "yaml-cpp version: ${yaml-cpp_VERSION}")

# Generic Linux/Rocky8 package handling, not vcpkg environment
if(NOT TARGET yaml-cpp::yaml-cpp)
    if(TARGET yaml-cpp)
        # If the target name is simply registered as yaml-cpp, create an alias
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    else()
        # If the target itself is an unbuilt old version package, create the interface target
        add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
        set_target_properties(
            yaml-cpp::yaml-cpp
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${YAML_CPP_LIBRARIES}"
        )
    endif()
endif()

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

# 5. Specify the header file path
# Distinguish the environment at build from the environment when it is installed and deployed later.
target_include_directories(
    mino_external
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 6. Link the external libraries being used
target_link_libraries(
    mino_external
    PUBLIC
        $<BUILD_INTERFACE:spdlog::spdlog>
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
        $<BUILD_INTERFACE:yaml-cpp::yaml-cpp>
)

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
