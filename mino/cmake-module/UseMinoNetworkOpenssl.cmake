include_guard(GLOBAL)

function(use_mino_network_openssl EXE_NAME MINO_DIR)
    # 1. MSVC 인코딩 옵션 설정
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
    endif()

    # 2. 대상 실행 파일/라이브러리 타깃 존재 여부 확인
    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_network_openssl: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the target before calling this function.")
    endif()

    # 3. 인트리(In-tree) 타깃 확인 및 우선 링크
    if(TARGET mino_network_openssl)
        message(STATUS "Linking to in-tree target: mino_network_openssl")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network_openssl)

    elseif(TARGET mino_network_ssl)
        message(STATUS "Linking to in-tree target: mino_network_ssl")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network_ssl)

    else()
        # 4. 외부(Out-of-tree)에 빌드/설치된 mino_network_openssl 라이브러리 검색
        message(STATUS "Linking to out-of-tree target: mino_network_openssl")

        # 4-1. 라이브러리 및 헤더 경로 검색
        if(MINO_DIR)
            find_path(MINO_OPENSSL_INCLUDE_DIR 
                NAMES "mino/network/openssl.hpp" "mino/network/ssl.hpp" "mino/network/network_openssl.hpp" "mino/network/network.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_NETWORK_OPENSSL_LIBRARY 
                NAMES mino_network_openssl mino_network_ssl
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_OPENSSL_INCLUDE_DIR 
                NAMES "mino/network/openssl.hpp" "mino/network/ssl.hpp" "mino/network/network_openssl.hpp" "mino/network/network.hpp")
            find_library(MINO_NETWORK_OPENSSL_LIBRARY 
                NAMES mino_network_openssl mino_network_ssl)
        endif()

        # 4-2. 검색 결과 검증
        if(NOT MINO_OPENSSL_INCLUDE_DIR OR NOT MINO_NETWORK_OPENSSL_LIBRARY)
            message(FATAL_ERROR "mino_network_openssl not found. Either add the project as a subdirectory or specify -DMINO_DIR=/path/to/mino/install")
        endif()

        # 4-3. Imported 타깃 생성
        if(NOT TARGET mino_network::mino_network_openssl AND MINO_NETWORK_OPENSSL_LIBRARY)
            add_library(mino_network::mino_network_openssl UNKNOWN IMPORTED)
            set_target_properties(mino_network::mino_network_openssl PROPERTIES
                IMPORTED_LOCATION "${MINO_NETWORK_OPENSSL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_OPENSSL_INCLUDE_DIR}"
            )
        endif()

        # 4-4. 외부 필수 종속성 연결

        # 4-4-1. Threads
        find_package(Threads REQUIRED)
        if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)
        endif()

        # 4-4-2. mino_core 종속성 (인-트리 타깃 존재 시 링크, 아닐 경우 라이브러리 탐색)
        if(TARGET mino_core)
            target_link_libraries(${EXE_NAME} PRIVATE mino_core)
        else()
            if(MINO_DIR)
                find_library(MINO_CORE_LIBRARY NAMES mino_core PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
            else()
                find_library(MINO_CORE_LIBRARY NAMES mino_core)
            endif()
            if(MINO_CORE_LIBRARY)
                target_link_libraries(${EXE_NAME} PRIVATE ${MINO_CORE_LIBRARY})
            endif()
        endif()

        # 4-4-3. OpenSSL
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

        # 4-4-4. Windows 시스템 라이브러리
        if(WIN32)
            target_link_libraries(${EXE_NAME} PRIVATE ws2_32 iphlpapi)
            target_compile_definitions(${EXE_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
        endif()

        # 4-5. 최종 라이브러리 링크
        target_link_libraries(${EXE_NAME} PRIVATE mino_network::mino_network_openssl)
        message(STATUS "Using external mino_network_openssl from ${MINO_DIR}")
    endif()
endfunction()
