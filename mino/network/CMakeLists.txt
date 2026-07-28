cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
project(mino_network VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 정보 설정
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. 외부 의존성 설정 

find_package(Threads REQUIRED)
 
# 3-1. OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# 업스트림 빌드가 실제로 '암호화' 및 'ssl' 타겟을 생성한 경우에만 ALIAS 타겟을 생성.
if(TARGET crypto AND TARGET ssl)
  add_library(OpenSSL::Crypto ALIAS crypto)
  add_library(OpenSSL::SSL ALIAS ssl)
endif()

# 3-2. CURL
find_package(CURL REQUIRED)
message(STATUS "CURL version: ${CURL_VERSION}")

# 3-3. httplib
find_package(httplib REQUIRED)
message(STATUS "httplib version: ${httplib_VERSION}")

# 3-4. spdlog
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")

# 3-5. nlohmann json
find_package(nlohmann_json QUIET)
if (nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json ${nlohmann_json_VERSION}")
    if (nlohmann_json_VERSION VERSION_LESS "3.11.0")
        message(STATUS "nlohmann_json < 3.11.0 -> forcing C++17 for compatibility")
        # Force C++17 when older nlohmann_json is present
        set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard" FORCE)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        set(CMAKE_CXX_EXTENSIONS OFF)
    endif()
else()
    message(FATAL_ERROR "nlohmann_json is required. Install it or enable FetchContent fallback.")
endif()

# 4. 라이브러리 소스 코드 설정
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
) 
add_library(mino_network STATIC ${SRC_FILES}) 
if(MSVC)
    target_compile_options(mino_network PRIVATE "/utf-8")
endif()

# 5. 헤더 파일 경로 지정
target_include_directories(mino_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 6. 외부 라이브러리 링크 설정

if(WIN32)
    # winsock 라이브러리 링크 (Windows 전용)
    target_link_libraries(mino_network PRIVATE ws2_32)

    # Windows 환경에서 불필요한 헤더 포함을 방지하고, min/max 매크로 충돌을 방지하기 위해 정의 추가
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# 6-1. OpenSSL 링크 설정
set(MINO_OPENSSL_LINKS "")

if(TARGET OpenSSL::Crypto AND TARGET OpenSSL::SSL)
  set(MINO_OPENSSL_LINKS OpenSSL::Crypto OpenSSL::SSL)
endif()

if(MINO_OPENSSL_LINKS)
  target_link_libraries(mino_network
    PUBLIC
      ${MINO_OPENSSL_LINKS}
  )
else()
  message(WARNING "OpenSSL not found; mino_network will be built without OpenSSL support.")
endif()

# 6-2. CURL 링크 설정
set(MINO_CURL_LINKS "")
if(TARGET CURL::libcurl)
  set(MINO_CURL_LINKS CURL::libcurl)
endif()
  
if(MINO_CURL_LINKS)
  target_link_libraries(mino_network PUBLIC ${MINO_CURL_LINKS})
else()
  message(FATAL_ERROR "CURL not found.")
endif()

# 6-3. httplib 링크 설정
set(MINO_HTTP_LIBS "")
# Common CMake package targets for httplib are typically 'httplib::httplib' or 'cpp-httplib::cpp-httplib'.
if(TARGET httplib::httplib)
  set(MINO_HTTP_LIBS httplib::httplib)
elseif(TARGET cpp-httplib::cpp-httplib)
  set(MINO_HTTP_LIBS cpp-httplib::cpp-httplib)
endif()

if(MINO_HTTP_LIBS)
  target_link_libraries(mino_network PUBLIC ${MINO_HTTP_LIBS})
else()
  message(FATAL_ERROR "httplib not found.")
endif()

# 6-4. spdlog 링크 설정
set(MINO_SPDLOG_LIBS "")
# Prefer compiled spdlog target first, fallback to header-only when available.
if(TARGET spdlog::spdlog)
  set(MINO_SPDLOG_LIBS spdlog::spdlog)
elseif(TARGET spdlog::spdlog_header_only)
  set(MINO_SPDLOG_LIBS spdlog::spdlog_header_only)
endif()

if(MINO_SPDLOG_LIBS)
  target_link_libraries(mino_network PUBLIC ${MINO_SPDLOG_LIBS})
else()
  message(FATAL_ERROR "spdlog not found.")
endif()

# 6-5. nlohmann_json 링크 설정
set(MINO_JSON_LIBS "")
if(TARGET nlohmann_json::nlohmann_json)
  set(MINO_JSON_LIBS nlohmann_json::nlohmann_json)
endif()

if(MINO_JSON_LIBS)
  target_link_libraries(mino_network PUBLIC ${MINO_JSON_LIBS})
else()
  message(FATAL_ERROR "nlohmann_json not found.")
endif()


# 7. [의존성] network가 xxx를 사용한다면 여기서 링크.
target_link_libraries(mino_network
    PUBLIC
        mino_core
        mino_external
)

# 8. 설치 규칙 정의 (동일한 EXPORT 묶음 이름인 'minoTargets'를 사용해야 합니다)
install(TARGETS mino_network
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 9. 설치 시 헤더 파일도 함께 설치
install(DIRECTORY include/mino
    DESTINATION include
)
