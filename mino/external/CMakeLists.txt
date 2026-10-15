cmake_minimum_required(VERSION 3.24)

# 1. Set the project name and version (C언어 컴파일을 위해 C 언어 추가)
project(mino_external VERSION 1.0.0 LANGUAGES CXX C)

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

# 4-1. SQLite3 Amalgamation 소스 파일 추가
set(SQLITE3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/mino/external/third-party/sqlite3")
list(APPEND SRC_FILES
  "${SQLITE3_DIR}/sqlite3.c"
)

add_library(mino_external STATIC ${SRC_FILES})

# 4-2. Visual Studio setting
if(MSVC)
  target_compile_options(mino_external PRIVATE "/utf-8")
endif()

# 4-3. GCC specific fixes
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(mino_external PUBLIC -Wno-free-nonheap-object)

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(mino_external PUBLIC stdc++fs)
    target_compile_definitions(mino_external PUBLIC JSON_HAS_CPP_20=0)
  endif()
endif()

# 4-4. SQLite3 컴파일 최적화 및 경고 억제 (선택 사항)
if(MSVC)
  target_compile_options(mino_external PRIVATE /wd4244 /wd4267)
else()
  target_compile_options(mino_external PRIVATE -Wno-unused-but-set-variable -Wno-implicit-fallthrough)
endif()

# 5. Specify header file paths

# 5-1. mino_external PUBLIC header path
target_include_directories(
  mino_external
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 5-2. Third-party library header path (SYSTEM PUBLIC: suppress warnings)
target_include_directories(
  mino_external
  SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mino/external/third-party>
    $<INSTALL_INTERFACE:include/mino/external/third-party>
)

# 6. Link libraries (스레드 라이브러리 및 수학/시스템 라이브러리 링크)
target_link_libraries(mino_external PRIVATE Threads::Threads)
if(NOT WIN32)
  target_link_libraries(mino_external PRIVATE dl pthread)
endif()
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

