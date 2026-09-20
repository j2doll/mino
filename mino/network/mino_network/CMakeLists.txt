cmake_minimum_required(VERSION 3.24)

project(mino_network VERSION 1.0.0 LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1. External Dependencies
find_package(Threads REQUIRED)

# 2. Source Files
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# 3. Create Library
add_library(mino_network STATIC ${SRC_FILES})

# 4. Include Directories
target_include_directories(
    mino_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 5. Compiler & Platform Settings
if(MSVC)
    target_compile_options(mino_network PRIVATE "/utf-8")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network PUBLIC stdc++fs)
    endif()
endif()

if(WIN32)
    target_compile_definitions(mino_network PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network PRIVATE ws2_32 iphlpapi)
endif()

# 6. Target Linking
target_link_libraries(
    mino_network
    PUBLIC
        Threads::Threads
        mino_core
)

# 7. Install Rules
install(
    TARGETS mino_network
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

