cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
#  설치본 및 패키지 형태로 만들러면 버전 정보가 필수.
project(mino_project VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 정보 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. 출력 디렉터리 설정
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
 
# 4. 외부 의존성 설정 (vcpkg 우선 검색 / 없으면 FetchContent)
include(FetchContent) # FetchContent 모듈을 사용하여 외부 라이브러리를 다운로드하고 빌드.

# 4-1. spdlog 설정
FetchContent_Declare(
  spdlog 
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.17.0
  FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(spdlog)
# spdlog가 이미 패키지로 설치되어 있으면 FetchContent를 사용하지 않고,
# 설치되어 있지 않으면 FetchContent를 통해 다운로드 및 빌드.

# 4-2. nlohmann/json 설정
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

# 5. 하위 mino 폴더 (라이브러리 소스 코드)
add_subdirectory(mino) 

# 6. 패키지 추출(Export) 및 CMake 설정 파일 생성
include(CMakePackageConfigHelpers)

# 7. 서브 모듈들이 차곡차곡 쌓아놓은 'minoTargets' 묶음을 파일로 내보내기
install(EXPORT minoTargets
    FILE minoTargets.cmake
    NAMESPACE mino::           # 외부 유저가 mino::core 등으로 사용하게 함
    DESTINATION lib/cmake/mino
)

# 8. 버전 정보 파일 생성
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 9. 외부 프로젝트용 Config 파일 생성
# 외부 프로젝트가 find_package(mino)를 호출했을 때, 
# 'spdlog'와 'nlohmann_json'도 자동으로 찾아서 함께 로드하도록 find_dependency를 추가.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    "include(CMakeFindDependencyMacro)\n"
    "find_dependency(spdlog CONFIG)\n"
    "find_dependency(nlohmann_json CONFIG)\n"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/minoTargets.cmake\")\n"
)

# 10. Config 파일들을 설치 경로로 복사
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    DESTINATION lib/cmake/mino
)
