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

# 4-1. Visual Studio setting
if(MSVC)
  target_compile_options(mino_external PRIVATE "/utf-8")
endif()

# 4-2. GCC specific fixes
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(mino_external PUBLIC -Wno-free-nonheap-object)

  # gcc 9 미만 버전 지원 (std::filesystem 링크 및 C++20 char8_t 문제 회피)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    # [수정 1] mino_core가 아니라 mino_external에 링크
    target_link_libraries(mino_external PUBLIC stdc++fs)

    # [수정 2] PUBLIC으로 지정하여 이 라이브러리를 참조하는 헤더들까지 매크로 전파
    target_compile_definitions(mino_external PUBLIC JSON_HAS_CPP_20=0)
  endif()
endif()

# 5. Specify header file paths

# 5-1. mino_external 자체 헤더 경로 (일반 PUBLIC)
target_include_directories(
  mino_external
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 5-2. 서드파티 라이브러리 헤더 경로 (SYSTEM PUBLIC: 경고 억제)
# target_include_directories(<target> SYSTEM <INTERFACE|PUBLIC|PRIVATE> [items1...])
target_include_directories(
  mino_external
  SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mino/external/third-party>
    $<INSTALL_INTERFACE:include/mino/external/third-party>
)

# 6. Link libraries
target_link_libraries(mino_external PRIVATE Threads::Threads)
target_link_libraries(mino_external PUBLIC mino_core)

# 7. Define install rules
install(
  TARGETS mino_external
  EXPORT minoTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/mino DESTINATION include)
