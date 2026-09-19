cmake_minimum_required(VERSION 3.24)

project(mino_network_libssh2 VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
find_package(Threads REQUIRED)
find_package(Libssh2 CONFIG QUIET)
if(NOT TARGET Libssh2::libssh2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED IMPORTED_TARGET libssh2)
endif()

# Source Files
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Create Library
add_library(mino_network_libssh2 STATIC ${SRC_FILES})

# Include Directories
target_include_directories(
    mino_network_libssh2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Compiler & Platform Options
if(MSVC)
    target_compile_options(mino_network_libssh2 PRIVATE "/utf-8")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network_libssh2 PUBLIC -Wno-free-nonheap-object -Wno-deprecated-literal-operator)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(mino_network_libssh2 PUBLIC stdc++fs)
    endif()
endif()

if(WIN32)
    target_compile_definitions(mino_network_libssh2 PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(mino_network_libssh2 PRIVATE ws2_32 iphlpapi)
endif()

# Target Linking  
target_link_libraries(
    mino_network_libssh2
    PUBLIC
        Threads::Threads
        mino_core
        mino_network
)

if(TARGET Libssh2::libssh2)
    target_link_libraries(mino_network_libssh2 PRIVATE Libssh2::libssh2)
elseif(TARGET PkgConfig::LIBSSH2)
    target_link_libraries(mino_network_libssh2 PRIVATE PkgConfig::LIBSSH2)
endif()

# Install Rules
install(
    TARGETS mino_network_libssh2
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)


