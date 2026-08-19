function(use_mino_network EXE_NAME MINO_DIR)
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
    endif()

    # Warn if caller didn't create the executable target
    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_network: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the executable target before calling this function.")
    endif()

    if(TARGET mino_network)
        message(STATUS "Linking to in-tree target: mino_network")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network)
    elseif(TARGET mino_external)
        message(STATUS "Linking to in-tree target: mino_external")
        target_link_libraries(${EXE_NAME} PRIVATE mino_external)
    elseif(TARGET mino_core)
        message(STATUS "Linking to in-tree target: mino_core")
        if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE mino_core rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE mino_core)
        endif()
    else()
        message(STATUS "Linking to out-of-tree target: mino_network")

        if(MINO_DIR)
            find_path(MINO_INCLUDE_DIR NAMES "mino/network/network.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_NETWORK_LIBRARY NAMES mino_network
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_INCLUDE_DIR NAMES "mino/network/network.hpp")
            find_library(MINO_NETWORK_LIBRARY NAMES mino_network)
        endif()

        if(NOT MINO_INCLUDE_DIR OR NOT MINO_NETWORK_LIBRARY)
            message(FATAL_ERROR "mino_network not found. Either add mino as a subdirectory or set -DMINO_DIR=/path/to/mino/install")
        endif()

        if(NOT TARGET mino_network::mino_network AND MINO_NETWORK_LIBRARY)
            add_library(mino_network::mino_network UNKNOWN IMPORTED)
            set_target_properties(mino_network::mino_network PROPERTIES
                IMPORTED_LOCATION "${MINO_NETWORK_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
            )
        endif()

        # third-party dependencies
        find_package(Threads REQUIRED)

        find_package(OpenSSL REQUIRED)
        if(TARGET crypto AND TARGET ssl)
            add_library(OpenSSL::Crypto ALIAS crypto)
            add_library(OpenSSL::SSL ALIAS ssl)
        endif()

        find_package(CURL REQUIRED)
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

        find_package(httplib REQUIRED)

        # third-party dependencies for mino external module
        find_package(spdlog REQUIRED)
        find_package(nlohmann_json REQUIRED)
        find_package(yaml-cpp CONFIG REQUIRED)

        # link libraries to the executable target
		if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)
        endif()

        target_link_libraries(${EXE_NAME} PRIVATE
            nlohmann_json::nlohmann_json
            spdlog::spdlog
            OpenSSL::SSL
            OpenSSL::Crypto
            CURL::libcurl
            httplib::httplib
        )

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

        # Detect the yaml-cpp target name provided by the package and use it
        if(TARGET yaml-cpp::yaml-cpp)
            set(YAMLCPP_TARGET yaml-cpp::yaml-cpp)
        elseif(TARGET yaml-cpp)
            set(YAMLCPP_TARGET yaml-cpp)
        elseif(TARGET YAML::YAML)
            set(YAMLCPP_TARGET YAML::YAML)
        else()
            message(FATAL_ERROR "yaml-cpp target not found after find_package. Expected one of: yaml-cpp::yaml-cpp, yaml-cpp, YAML::YAML")
        endif()
        target_link_libraries(${EXE_NAME} PRIVATE ${YAMLCPP_TARGET})

        # link mino network library to the executable target
        target_link_libraries(${EXE_NAME} PRIVATE mino_network::mino_network)
        message(STATUS "Using external mino_network from ${MINO_DIR}")
    endif()
endfunction()
