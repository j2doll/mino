cmake_minimum_required(VERSION 3.24)

# 1. Set project name, version, and enable C and CXX languages
project(mino_external VERSION 1.0.0 LANGUAGES CXX C)

# 2. C++ standard configuration
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. External dependencies
find_package(Threads REQUIRED)

# 4. Source file discovery and library target configuration
file(
  GLOB_RECURSE SRC_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

# 4-1. Add SQLite3 Amalgamation source file
set(SQLITE3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/sqlite3")
list(APPEND SRC_FILES
  "${SQLITE3_DIR}/sqlite3.c"
)

add_library(mino_external STATIC ${SRC_FILES})

# 4-2. MSVC-specific settings
if(MSVC)
  target_compile_options(mino_external PRIVATE "/utf-8")
endif()

# 4-3. GCC-specific compiler fixes
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(mino_external PUBLIC -Wno-free-nonheap-object)

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(mino_external PUBLIC stdc++fs)
    target_compile_definitions(mino_external PUBLIC JSON_HAS_CPP_20=0)
  endif()
endif()

# 4-4. SQLite3 compiler optimizations and warning suppressions (optional)
if(MSVC)
  target_compile_options(mino_external PRIVATE /wd4244 /wd4267)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(mino_external PRIVATE -Wno-unused-but-set-variable -Wno-implicit-fallthrough)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(mino_external PRIVATE -Wno-implicit-fallthrough)
endif()

# 5. Header include directories

# 5-1. mino_external PUBLIC header directory
target_include_directories(
  mino_external
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 5-2. Third-party library header directory (SYSTEM PUBLIC: suppress warnings)
# 변경: 설치 시에는 ${CMAKE_INSTALL_PREFIX}/third-party 로 복사되도록 설정
target_include_directories(
  mino_external
  SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party>
    $<INSTALL_INTERFACE:third-party>
)

# 6. Link libraries (Threads and dynamic loading libraries across platforms)
target_link_libraries(mino_external PRIVATE Threads::Threads)

# CMAKE_DL_LIBS automatically handles dl on Linux and resolves to empty on macOS/Windows
if(CMAKE_DL_LIBS)
  target_link_libraries(mino_external PRIVATE ${CMAKE_DL_LIBS})
endif()

target_link_libraries(mino_external PUBLIC mino_core)

# 7. Define installation rules
install(
  TARGETS mino_external
  EXPORT minoTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/mino DESTINATION include)

# 8. Install third-party directory to the install prefix root: <prefix>/third-party
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/"
  DESTINATION third-party
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

