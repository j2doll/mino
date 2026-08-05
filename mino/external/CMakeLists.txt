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

include(FetchContent)

# 3-1. spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  message(STATUS "spdlog not found via find_package; fetching spdlog using FetchContent")
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.11.0
  )
  FetchContent_MakeAvailable(spdlog)
endif()
if(NOT TARGET spdlog::spdlog)
  message(FATAL_ERROR "spdlog was not found or provided. Set CMAKE_PREFIX_PATH or spdlog_DIR, or allow FetchContent to fetch spdlog.")
endif()
message(STATUS "spdlog target available")
message(STATUS "spdlog version: ${spdlog_VERSION}")

# 3-2. nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found via find_package; fetching nlohmann/json using FetchContent (no configure)")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
  )
  FetchContent_GetProperties(nlohmann_json)
  if(NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)
  endif()
  add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
  set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_SOURCE_DIR}/include"
  )
endif()
if(NOT TARGET nlohmann_json::nlohmann_json)
  message(FATAL_ERROR "nlohmann_json was not found or provided. Set CMAKE_PREFIX_PATH or nlohmann_json_DIR, or allow FetchContent to fetch nlohmann_json.")
endif()
message(STATUS "nlohmann_json target available")

# 3-3. yaml-cpp
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
  message(STATUS "yaml-cpp not found via find_package; fetching yaml-cpp using FetchContent (no configure)")
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
  )
  FetchContent_GetProperties(yaml-cpp)
  if(NOT yaml-cpp_POPULATED)
    FetchContent_Populate(yaml-cpp)
  endif()
  # Create an IMPORTED INTERFACE target pointing to the fetched include directory.
  add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
  set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${yaml-cpp_SOURCE_DIR}/include"
  )
endif()

# If a system/vcpkg yaml-cpp exists, alias or prefer it
if(NOT TARGET yaml-cpp::yaml-cpp)
  if(TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
  else()
    add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
    set_target_properties(
      yaml-cpp::yaml-cpp
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${YAML_CPP_LIBRARIES}"
    )
  endif()
endif()
message(STATUS "yaml-cpp target available")

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
