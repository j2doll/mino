include_guard(GLOBAL)

function(use_mino_network_openssl EXE_NAME MINO_DIR)
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
    endif()

    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_network_openssl: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the target before calling this function.")
    endif()

    if(TARGET mino_network_openssl)
        message(STATUS "Linking to in-tree target: mino_network_openssl")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network_openssl)

    elseif(TARGET mino_network_ssl)
        message(STATUS "Linking to in-tree target: mino_network_ssl")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network_ssl)

    else()
        message(STATUS "Linking to out-of-tree target: mino_network_openssl")

        if(MINO_DIR)
            find_path(MINO_OPENSSL_INCLUDE_DIR 
                NAMES "mino/network_openssl/mno.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_NETWORK_OPENSSL_LIBRARY 
                NAMES mino_network_openssl mino_network_ssl
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_OPENSSL_INCLUDE_DIR 
                NAMES "mino/network_openssl/mno.hpp")
            find_library(MINO_NETWORK_OPENSSL_LIBRARY 
                NAMES mino_network_openssl mino_network_ssl)
        endif()

        if(NOT MINO_OPENSSL_INCLUDE_DIR OR NOT MINO_NETWORK_OPENSSL_LIBRARY)
            message(FATAL_ERROR "mino_network_openssl not found. Either add the project as a subdirectory or specify -DMINO_DIR=/path/to/mino/install")
        endif()

        target_include_directories(${EXE_NAME} PRIVATE "${MINO_DIR}/third-party")

        if(NOT TARGET mino_network::mino_network_openssl AND MINO_NETWORK_OPENSSL_LIBRARY)
            add_library(mino_network::mino_network_openssl UNKNOWN IMPORTED)
            set_target_properties(mino_network::mino_network_openssl PROPERTIES
                IMPORTED_LOCATION "${MINO_NETWORK_OPENSSL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_OPENSSL_INCLUDE_DIR}"
            )
        endif()

        find_package(Threads REQUIRED)
        if(UNIX AND NOT APPLE)
	        target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads rt)

	        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
		        target_link_libraries(${EXE_NAME} PRIVATE stdc++fs)
	        endif()
        else()
	        target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)
        endif()

        find_package(OpenSSL REQUIRED)
        if(TARGET crypto AND TARGET ssl)
            add_library(OpenSSL::Crypto ALIAS crypto)
            add_library(OpenSSL::SSL ALIAS ssl)
        endif()

        if(TARGET OpenSSL::Crypto AND TARGET OpenSSL::SSL)
            target_link_libraries(${EXE_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        endif()
        target_compile_definitions(${EXE_NAME} PRIVATE USE_OPENSSL=1)

        if(WIN32)
            target_link_libraries(${EXE_NAME} PRIVATE ws2_32 iphlpapi)
            target_compile_definitions(${EXE_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
        endif()

        target_link_libraries(${EXE_NAME} PRIVATE mino_network::mino_network_openssl)
        message(STATUS "Using external mino_network_openssl from ${MINO_DIR}")
    endif()
endfunction()

