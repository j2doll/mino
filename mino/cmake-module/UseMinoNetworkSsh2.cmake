include_guard(GLOBAL)

function(use_mino_network_ssh2 EXE_NAME MINO_DIR)
    # 1. MSVC 인코딩 옵션 설정
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
    endif()

    # 2. 대상 실행 파일/라이브러리 타깃 존재 여부 확인
    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_network_ssh2: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the target before calling this function.")
    endif()

    # 3. 인트리(In-tree) 타깃 확인 및 우선 링크
    if(TARGET mino_network_libssh2)
        message(STATUS "Linking to in-tree target: mino_network_libssh2")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network_libssh2)

    elseif(TARGET mino_network_ssh2)
        message(STATUS "Linking to in-tree target: mino_network_ssh2")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network_ssh2)

    else()
        # 4. 외부(Out-of-tree)에 빌드/설치된 mino_network_libssh2 라이브러리 검색
        message(STATUS "Linking to out-of-tree target: mino_network_libssh2")

        # 4-1. 라이브러리 및 헤더 경로 검색
        if(MINO_DIR)
            find_path(MINO_SSH2_INCLUDE_DIR 
                NAMES "mino/network/libssh2.hpp" "mino/network/ssh2.hpp" "mino/network/ssh.hpp" "mino/network/network.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_NETWORK_LIBSSH2_LIBRARY 
                NAMES mino_network_libssh2 mino_network_ssh2
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_SSH2_INCLUDE_DIR 
                NAMES "mino/network/libssh2.hpp" "mino/network/ssh2.hpp" "mino/network/ssh.hpp" "mino/network/network.hpp")
            find_library(MINO_NETWORK_LIBSSH2_LIBRARY 
                NAMES mino_network_libssh2 mino_network_ssh2)
        endif()

        # 4-2. 검색 결과 검증
        if(NOT MINO_SSH2_INCLUDE_DIR OR NOT MINO_NETWORK_LIBSSH2_LIBRARY)
            message(FATAL_ERROR "mino_network_libssh2 not found. Either add the project as a subdirectory or specify -DMINO_DIR=/path/to/mino/install")
        endif()

        # 4-3. Imported 타깃 생성
        if(NOT TARGET mino_network::mino_network_libssh2 AND MINO_NETWORK_LIBSSH2_LIBRARY)
            add_library(mino_network::mino_network_libssh2 UNKNOWN IMPORTED)
            set_target_properties(mino_network::mino_network_libssh2 PROPERTIES
                IMPORTED_LOCATION "${MINO_NETWORK_LIBSSH2_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_SSH2_INCLUDE_DIR}"
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

        # 4-4-3. libssh2
        find_package(Libssh2 CONFIG QUIET)
        if(NOT TARGET Libssh2::libssh2)
            find_package(libssh2 CONFIG QUIET)
        endif()

        if(TARGET Libssh2::libssh2)
            message(STATUS "Found libssh2 via CMake Config (Libssh2::libssh2)")
            target_link_libraries(${EXE_NAME} PRIVATE Libssh2::libssh2)
        elseif(TARGET libssh2::libssh2)
            target_link_libraries(${EXE_NAME} PRIVATE libssh2::libssh2)
            message(STATUS "Found libssh2 via CMake Config (libssh2::libssh2)")
        else()
            find_package(PkgConfig QUIET)
            if(PKG_CONFIG_FOUND)
                pkg_check_modules(LIBSSH2 QUIET IMPORTED_TARGET libssh2)
            endif()

            if(TARGET PkgConfig::LIBSSH2)
                message(STATUS "Found libssh2 via PkgConfig (PkgConfig::LIBSSH2)")
                target_link_libraries(${EXE_NAME} PRIVATE PkgConfig::LIBSSH2)
            else()
                if(WIN32)
                    message(FATAL_ERROR "libssh2 not found. Please install it via: vcpkg install libssh2:x64-windows")
                else()
                    message(FATAL_ERROR "libssh2 not found. Please install it via package manager (e.g. libssh2-devel)")
                endif()
            endif()
        endif()

        # 4-4-4. Windows 시스템 라이브러리
        if(WIN32)
            target_link_libraries(${EXE_NAME} PRIVATE ws2_32 iphlpapi)
            target_compile_definitions(${EXE_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
        endif()

        # 4-5. 최종 라이브러리 링크
        target_link_libraries(${EXE_NAME} PRIVATE mino_network::mino_network_libssh2)
        message(STATUS "Using external mino_network_libssh2 from ${MINO_DIR}")
    endif()
endfunction()

