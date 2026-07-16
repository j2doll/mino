cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
#   mino 의 하위 모듈 mino_external
project(mino_external VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 정보 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. 외부 의존성 설정 (vcpkg 우선 검색 / 없으면 FetchContent)
include(FetchContent) # FetchContent 모듈을 사용하여 외부 라이브러리를 다운로드하고 빌드.

# 3-1. spdlog 설정
FetchContent_Declare(
  spdlog 
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.17.0
  FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(spdlog)
# spdlog가 이미 패키지로 설치되어 있으면 FetchContent를 사용하지 않고,
# 설치되어 있지 않으면 FetchContent를 통해 다운로드 및 빌드.

# 3-2. nlohmann/json 설정
# (FetchContent로 가져오거나 이미 패키지로 설치되어 있으면 find_package 사용)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.12.0
  FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(nlohmann_json)
# 사용 방법: 원하는 타겟에서
# target_link_libraries(<your_target> PRIVATE nlohmann_json::nlohmann_json)

# 4. 사용하는 패키지 정보
#   패키지 찾기 또는 설치는 상위 CMakeLists.txt에서 처리하도록 설정.
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 5. 라이브러리 생성 및 소스 코드 설정
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
add_library(mino_external STATIC ${SRC_FILES})
if(MSVC)
    target_compile_options(mino_external PRIVATE "/utf-8")
endif()

# 6. 헤더 파일 경로 지정
# 빌드할 때 환경과 나중에 install되어 배포되었을 때의 환경을 구분.
target_include_directories(mino_external
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 7. 사용하는 외부 라이브러리 링크
target_link_libraries(mino_external
    PUBLIC
        $<BUILD_INTERFACE:spdlog::spdlog>
        $<INSTALL_INTERFACE:spdlog::spdlog>

        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
        $<INSTALL_INTERFACE:nlohmann_json::nlohmann_json>
)

# 8. [의존성] external이 core를 사용한다면 여기서 링크해 줍니다.
target_link_libraries(mino_external
    PUBLIC
        mino_core
)

# 9. 설치(Install) 규칙 정의
install(TARGETS mino_external spdlog nlohmann_json 
    EXPORT minoTargets # 중요: 모든 서브 모듈을 하나의 export 묶음(minoTargets)으로 관리
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 10. 공개 헤더 파일 설치 (폴더 통째로 복사)
# include/mino/core/ 형태를 유지하기 위해 include/ 내부의 mino 폴더를 타깃으로 설정.
install(DIRECTORY include/mino
    DESTINATION include
)
