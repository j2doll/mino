cmake_minimum_required(VERSION 3.24)
project(mino_project VERSION 1.0.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 출력 디렉터리 설정
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 하위 mino 폴더 (라이브러리 소스 코드)
add_subdirectory(mino) 

# ------------------------------------------------------------------
# 패키지 추출(Export) 및 CMake 설정 파일 생성
# ------------------------------------------------------------------
include(CMakePackageConfigHelpers)

# 1. 서브 모듈들이 차곡차곡 쌓아놓은 'minoTargets' 묶음을 파일로 내보내기
install(EXPORT minoTargets
    FILE minoTargets.cmake
    NAMESPACE mino::               # 외부 유저가 mino::core 등으로 쓰게 함
    DESTINATION lib/cmake/mino
)

# 2. 버전 정보 파일 생성
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 3. [수정] 외부 프로젝트용 Config 파일 생성
# 외부 프로젝트가 find_package(mino)를 호출했을 때, 
# 'spdlog'도 자동으로 찾아서 함께 로드하도록 find_dependency를 추가해 줍니다.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    "include(CMakeFindDependencyMacro)\n"
    "find_dependency(spdlog CONFIG)\n" # 이 부분을 추가하여 의존성을 전파합니다.
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/minoTargets.cmake\")\n"
)

# 4. Config 파일들을 설치 경로로 복사
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    DESTINATION lib/cmake/mino
)
