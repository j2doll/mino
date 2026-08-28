cmake_minimum_required(VERSION 3.24)

# 1. Set the project name and version
project(mino_network VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ standard information settings
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
 

# 3. External dependency configuration

# 3-1. Threads
find_package(Threads REQUIRED)

# 3-2. OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# Create ALIAS targets only if the upstream build actually created 'crypto' and 'ssl' targets.
if(TARGET crypto AND TARGET ssl)
    add_library(OpenSSL::Crypto ALIAS crypto)
    add_library(OpenSSL::SSL ALIAS ssl)
endif()

# 3-3. CURL
find_package(CURL REQUIRED)
message(STATUS "CURL version: ${CURL_VERSION}")

# 3-4. Brotli  
set(MINO_BROTLI_AVAILABLE FALSE)
if(DEFINED VCPKG_ROOT
   OR (DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg"))
    # Using vcpkg toolchain -> prefer vcpkg namespace
    find_package(unofficial-brotli CONFIG REQUIRED)
    set(MINO_BROTLI_AVAILABLE TRUE)
    message(STATUS "unofficial-brotli found (vcpkg)")
else()
    # Not using vcpkg toolchain: try common CMake package names quietly
    find_package(Brotli CONFIG QUIET)
    if(BROTLI_FOUND)
        set(MINO_BROTLI_AVAILABLE TRUE)
        message(STATUS "Brotli found via standard package config")
    else()
        find_package(brotli CONFIG QUIET)
        if(BROTLI_FOUND)
            set(MINO_BROTLI_AVAILABLE TRUE)
            message(STATUS "brotli found via standard package config")
        else()
            message(
                STATUS
                "Brotli package config not found; continuing without guaranteed brotli targets"
            )
        endif()
    endif()
endif()

# 3-5. libssh2
find_package(Libssh2 CONFIG QUIET)
if(TARGET Libssh2::libssh2)
    message(STATUS "Found libssh2 via CMake Config (Libssh2::libssh2)")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED IMPORTED_TARGET libssh2)
    message(STATUS "Found libssh2 via PkgConfig (${LIBSSH2_VERSION})")
endif()

# 4. Configure the library source code
file(
    GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
add_library(mino_network STATIC ${SRC_FILES})
if(MSVC)
    target_compile_options(mino_network PRIVATE "/utf-8")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mino_network PUBLIC -Wno-free-nonheap-object)
    target_compile_options(mino_network PUBLIC -Wno-deprecated-literal-operator)
endif()

# 5. Specify the header file path
target_include_directories(
    mino_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 6. Link external libraries

# 6-1. Threads link settings
# Ensure consumers of the static library also link the threading library.
# This propagates the necessary pthread linkage to executables that link `mino_network`.
target_link_libraries(mino_network PUBLIC Threads::Threads)

if(WIN32)
    # Link winsock library (Windows only)
    target_link_libraries(mino_network PRIVATE ws2_32)

    # Prevent unnecessary header inclusion and avoid min/max macro conflicts on Windows
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# 6-2. OpenSSL link settings
set(MINO_OPENSSL_LINKS "")

if(TARGET OpenSSL::Crypto AND TARGET OpenSSL::SSL)
    set(MINO_OPENSSL_LINKS OpenSSL::Crypto OpenSSL::SSL)
endif()

if(MINO_OPENSSL_LINKS)
    target_link_libraries(mino_network PUBLIC ${MINO_OPENSSL_LINKS})
    # Define USE_OPENSSL so source files can use #if USE_OPENSSL
    target_compile_definitions(mino_network PUBLIC USE_OPENSSL=1)
else()
    message(
        WARNING
        "OpenSSL not found; mino_network will be built without OpenSSL support."
    )
    # Define explicitly as 0 so code using #if USE_OPENSSL compiles consistently
    target_compile_definitions(mino_network PUBLIC USE_OPENSSL=0)
endif()

# 6-3. CURL link settings
set(MINO_CURL_LINKS "")
if(TARGET CURL::libcurl)
    set(MINO_CURL_LINKS CURL::libcurl)
endif()

if(MINO_CURL_LINKS)
    target_link_libraries(mino_network PUBLIC ${MINO_CURL_LINKS})
    # Define USE_CURL so curl-backed implementation files (guarded by #ifdef USE_CURL)
    # are compiled into the library.
    target_compile_definitions(mino_network PUBLIC USE_CURL)
else()
    message(FATAL_ERROR "CURL not found.")
endif()

# 6-4. libssh2 link settings
if(TARGET Libssh2::libssh2)
    target_link_libraries(mino_network PRIVATE Libssh2::libssh2)
elseif(TARGET PkgConfig::LIBSSH2)
    target_link_libraries(mino_network PRIVATE PkgConfig::LIBSSH2)
else()
    message(FATAL_ERROR "libssh2 target not available.")
endif()

# 6-5. iphlpapi (Windows only)
if(WIN32)
    # Link winsock library (Windows only) and IP helper API for GetAdaptersAddresses
    target_link_libraries(mino_network PRIVATE ws2_32 iphlpapi)

    # Prevent unnecessary header inclusion and avoid min/max macro conflicts on Windows
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# 7. [Dependency] If network uses xxx, link it here.
# target_link_libraries(mino_network PUBLIC mino_core mino_external)
target_link_libraries(mino_network PUBLIC mino_core)

# 8. Define install rules (use the same EXPORT bundle name 'minoTargets')
install(
    TARGETS mino_network
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 9. Install header files as well
install(DIRECTORY include/mino DESTINATION include)
