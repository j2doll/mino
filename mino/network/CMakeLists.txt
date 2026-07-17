cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
project(mino_network VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 정보 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. 외부 의존성 설정 
include(FetchContent) # FetchContent 모듈을 사용하여 외부 라이브러리를 다운로드하고 빌드.

# 3-1. OpenSSL 
FetchContent_Declare(
  OpenSSL
  GIT_REPOSITORY https://github.com/openssl/openssl.git
  GIT_TAG        "openssl-4.0.1"
  FIND_PACKAGE_ARGS CONFIG # 시스템에 설치된 패키지를 먼저 찾고, 없으면 다운로드하여 빌드하도록 지시
)
FetchContent_MakeAvailable(OpenSSL)

# 업스트림 빌드가 실제로 '암호화' 및 'ssl' 타겟을 생성한 경우에만 ALIAS 타겟을 생성.
if(TARGET crypto AND TARGET ssl)
  add_library(OpenSSL::Crypto ALIAS crypto)
  add_library(OpenSSL::SSL ALIAS ssl)
endif()

# 3-2. CURL

# curl 빌드 시 불필요한 테스트나 자체 실행파일(curl.exe) 생성을 방지하기 위한 옵션
set(BUILD_CURL_EXE OFF CACHE INTERNAL "") # curl.exe 생성 방지
set(BUILD_TESTING OFF CACHE INTERNAL "") # 테스트 빌드 방지

# Libpsl(Public Suffix List Library)
find_package(Libpsl QUIET)
if(Libpsl_FOUND)
  message(STATUS "Libpsl found! Enabling Libpsl for CURL.")
  # 이미 설치된 패키지의 설정 그대로 사용
else()
  message(STATUS "Libpsl not found. Disabling Libpsl for CURL to prevent build errors.")
  set(CURL_USE_LIBPSL OFF CACHE INTERNAL "")  # libpsl 비활성화
  # NOTE: 윈도우 환경(Visual Studio / MSVC)에는 Libpsl이 기본적으로 설치되어 있지 않습니다.
endif()

# set(CURL_DISABLE_DICT ON CACHE INTERNAL "") # dict 프로토콜 비활성화
# set(CURL_DISABLE_FILE ON CACHE INTERNAL "") # file 프로토콜 비활성화
# set(CURL_DISABLE_FTP ON CACHE INTERNAL "") # ftp 프로토콜 비활성화
# set(CURL_DISABLE_GOPHER ON CACHE INTERNAL "") # gopher 프로토콜 비활성화
# set(CURL_DISABLE_IMAP ON CACHE INTERNAL "") # imap 프로토콜 비활성화
# set(CURL_DISABLE_LDAP ON CACHE INTERNAL "") # ldap 프로토콜 비활성화
# set(CURL_DISABLE_POP3 ON CACHE INTERNAL "") # pop3 프로토콜 비활성화
# set(CURL_DISABLE_RTSP ON CACHE INTERNAL "") # rtsp 프로토콜 비활성화
# set(CURL_DISABLE_SMTP ON CACHE INTERNAL "") # smtp 프로토콜 비활성화
# set(CURL_DISABLE_TELNET ON CACHE INTERNAL "") # telnet 프로토콜 비활성화
# set(CURL_DISABLE_TFTP ON CACHE INTERNAL "") # tftp 프로토콜 비활성화

FetchContent_Declare(
  CURL
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG        "curl-8_12_0" 
  FIND_PACKAGE_ARGS CONFIG 
)
FetchContent_MakeAvailable(CURL) 

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
  message(WARNING "CURL not found; mino_network will be built without CURL support.")
endif()

# 7. [의존성] network가 core를 사용한다면 여기서 링크.
target_link_libraries(mino_network
    PUBLIC
        mino_core
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
