function(use_mino_network EXE_NAME MINO_DIR)
    # 1. Set C++ standard to C++17 if not already defined
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
    endif()

    # 2. Allow user to specify where the mino install/build is located.
    # Warn if caller didn't create the executable target
    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_network: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the executable target before calling this function.")
    endif()

    # 3. Link to mino_network target if it exists, otherwise find and link external mino_network
    if(TARGET mino_network)
        # 3-1. If an in-tree target named `mino_network` exists, link to it directly
        message(STATUS "Linking to in-tree target: mino_network")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network)
    elseif(TARGET mino_core)
        # 3-2. If an in-tree target named `mino_core` exists, link to it directly
        message(STATUS "Linking to in-tree target: mino_core")
        if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE mino_core rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE mino_core)
        endif()
    else()
        # 4-1. If no in-tree target exists, find and link to an external mino_network library
        message(STATUS "Linking to out-of-tree target: mino_network")

        # 4-2. Find mino_network library and include directory
        if(MINO_DIR)
            find_path(MINO_INCLUDE_DIR NAMES "mino/network/network.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_NETWORK_LIBRARY NAMES mino_network
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_INCLUDE_DIR NAMES "mino/network/network.hpp")
            find_library(MINO_NETWORK_LIBRARY NAMES mino_network)
        endif()

        # 4-3. Check if mino_network was found
        if(NOT MINO_INCLUDE_DIR OR NOT MINO_NETWORK_LIBRARY)
            message(FATAL_ERROR "mino_network not found. Either add mino as a subdirectory or set -DMINO_DIR=/path/to/mino/install")
        endif()

        # 4-4. Create an imported target for mino_network if it doesn't already exist
        if(NOT TARGET mino_network::mino_network AND MINO_NETWORK_LIBRARY)
            add_library(mino_network::mino_network UNKNOWN IMPORTED)
            set_target_properties(mino_network::mino_network PROPERTIES
                IMPORTED_LOCATION "${MINO_NETWORK_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
            )
        endif()

        # 4-5. Link third-party dependencies

        # 4-5-1. Threads
        find_package(Threads REQUIRED)
		if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)
        endif()

        # 4-5-2. OpenSSL
        find_package(OpenSSL REQUIRED)
        if(TARGET crypto AND TARGET ssl)
            add_library(OpenSSL::Crypto ALIAS crypto)
            add_library(OpenSSL::SSL ALIAS ssl)
        endif()
        target_link_libraries(${EXE_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

        # 4-5-3. CURL
        find_package(CURL REQUIRED)
        target_link_libraries(${EXE_NAME} PRIVATE CURL::libcurl)

        # 4-5-4. Brotli
        set(MINO_BROTLI_AVAILABLE FALSE)
        if(
            DEFINED VCPKG_ROOT
            OR (DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        )
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
        if(MINO_BROTLI_AVAILABLE)
            # Link whichever brotli target is available in common package namespaces
            if(TARGET unofficial-brotli::brotli)
                target_link_libraries(${EXE_NAME} PRIVATE unofficial-brotli::brotli)
            elseif(TARGET Brotli::brotli)
                target_link_libraries(${EXE_NAME} PRIVATE Brotli::brotli)
            elseif(TARGET brotli::brotli)
                target_link_libraries(${EXE_NAME} PRIVATE brotli::brotli)
            elseif(TARGET Brotli::Brotli)
                target_link_libraries(${EXE_NAME} PRIVATE Brotli::Brotli)
            endif()
        endif()

        # 4-5-5. libssh2
        find_package(Libssh2 CONFIG QUIET)
        if(TARGET Libssh2::libssh2)
            message(STATUS "Found libssh2 via CMake Config (Libssh2::libssh2)")
        else()
            find_package(PkgConfig REQUIRED)
            pkg_check_modules(LIBSSH2 REQUIRED IMPORTED_TARGET libssh2)
            message(STATUS "Found libssh2 via PkgConfig (${LIBSSH2_VERSION})")
        endif()

        if(TARGET Libssh2::libssh2)
            target_link_libraries(${EXE_NAME} PRIVATE Libssh2::libssh2)
        elseif(TARGET PkgConfig::LIBSSH2)
            target_link_libraries(${EXE_NAME} PRIVATE PkgConfig::LIBSSH2)
        else()
            message(FATAL_ERROR "libssh2 target not available.")
        endif()

        # 4-5-6. Windows-specific libraries
        if(WIN32)
            # Link winsock library (Windows only) and IP helper API for GetAdaptersAddresses
            target_link_libraries(${EXE_NAME} PRIVATE ws2_32 iphlpapi)

            # Prevent unnecessary header inclusion and avoid min/max macro conflicts on Windows
            add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
        endif()

        # link mino network library to the executable target
        target_link_libraries(${EXE_NAME} PRIVATE mino_network::mino_network)
        message(STATUS "Using external mino_network from ${MINO_DIR}")
    endif()
endfunction()
