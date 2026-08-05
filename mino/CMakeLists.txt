cmake_minimum_required(VERSION 3.24)

# 1. Set the project name and version
#  Version information is required to create an installable package.
project(mino_project VERSION 1.0.0 LANGUAGES CXX)

# 2. Set C++ standard
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 4. Add subdirectories
add_subdirectory(core)
add_subdirectory(external)
add_subdirectory(network)

# 5. Export package and generate CMake configuration files
include(CMakePackageConfigHelpers)

# 6. Export the 'minoTargets' bundle created by the submodules to a file
install(
    EXPORT minoTargets
    FILE minoTargets.cmake
    NAMESPACE mino:: # Allow external users to use mino::core, etc.
    DESTINATION lib/cmake/mino
)

# 7. Generate version information file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 8. Generate Config file for external projects
file(
    WRITE ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    "include(CMakeFindDependencyMacro)\n"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/minoTargets.cmake\")\n"
)

# 9. Copy Config files to the installation directory
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    DESTINATION lib/cmake/mino
)
