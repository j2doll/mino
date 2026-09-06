cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 이름과 버전 
project(mino_project VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 버전 
if(NOT DEFINED CMAKE_CXX_STANDARD) 
    set(CMAKE_CXX_STANDARD 17) # 미설정시 C++17 사용
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. 출력 경로 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # 실행 파일 출력 경로
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # 정적 라이브러리 출력 경로
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # 공유 라이브러리 출력 경로

# 4. 하위 프로젝트 디렉토리
add_subdirectory(core)  
add_subdirectory(network) 
add_subdirectory(external) 

# 5. 패키지를 설치할 때 필요한 CMake 모듈을 포함
include(CMakePackageConfigHelpers)

# 6. 하위 모듈에서 생성된 'minoTargets' 번들을 파일로 내보내기
install(
    EXPORT minoTargets
    FILE minoTargets.cmake
    NAMESPACE mino:: # Allow external users to use mino::core, etc.
    DESTINATION lib/cmake/mino
)
# minoTargets.cmake
# - 설치 경로: <CMAKE_INSTALL_PREFIX>/lib/cmake/mino/minoTargets.cmake
# - 역할: 하위 모듈(core, network 등)에서 install(TARGETS ... EXPORT minoTargets) 를 
#         통해 내보낸 타깃들의 메타데이터를 담은 파일입니다.
# - 주요 내용:
#  - 빌드된 실제 라이브러리/실행 파일 경로 매핑
#  - mino::` 네임스페이스가 부여된 임포트 타깃(mino::core, mino::network 등) 정의
#  - 타깃이 요구하는 컴파일 옵션, 인클루드 디렉터리(INTERFACE_INCLUDE_DIRECTORIES), 링크 종속성 정의

# 7. 버전 정보 파일 생성
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
# minoConfigVersion.cmake
# - 설치 경로: <CMAKE_INSTALL_PREFIX>/lib/cmake/mino/minoConfigVersion.cmake
# - 역할: 외부 프로젝트에서 find_package(mino 1.0.0 REQUIRED) 와 같이 버전을
#         명시하여 탐색할 때 호환성을 검증합니다.
# - 주요 내용:
#  - 패키지 버전(1.0.0) 명시
#  - SameMajorVersion 호환성 규칙 적용
#    (예: 외부 프로젝트에서 요구하는 버전이 1.x.x 일 경우 호환 성공,
#    2.0.0 이상이나 0.x 요구 시 실패 처리)

# 8. 외부 프로젝트를 위한 Config 파일 생성
file(
    WRITE ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    "include(CMakeFindDependencyMacro)\n"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/minoTargets.cmake\")\n"
)
# minoConfig.cmake
# - 설치 경로: <CMAKE_INSTALL_PREFIX>/lib/cmake/mino/minoConfig.cmake
# - 역할: 외부 프로젝트가 find_package(mino) 를 호출할 때 CMake가 진입점으로
#         가장 먼저 로드하는 메인 패키지 진입 파일(Config-file)입니다.
# - 주요 내용:
#  - CMakeFindDependencyMacro 모듈 로드
#  - 동일 디렉터리의 minoTargets.cmake 를 include() 하여 타깃들을 외부 프로젝트의
#    빌드 스코프로 불러옴

# 9. 설치 디렉토리로 Config 파일 복사
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    DESTINATION lib/cmake/mino
)

# 10. CMake 모듈 파일들(.cmake) 설치
#  ${CMAKE_SOURCE_DIR}/mino/cmake-module/ 경로 아래의 *.cmake 파일들을
#  <CMAKE_INSTALL_PREFIX>/lib/cmake/mino/ 경로에 설치
if(EXISTS "${CMAKE_SOURCE_DIR}/mino/cmake-module")
  install(
      DIRECTORY "${CMAKE_SOURCE_DIR}/mino/cmake-module/" # NOTE: 경로 끝에 슬래시(/)를 붙이면 해당 디렉터리 자체는 생성하지 않고, 그 안의 내용물(파일 및 하위 폴더)만 대상 경로로 복사
      DESTINATION lib/cmake/mino
      FILES_MATCHING PATTERN "*.cmake"
  )
endif()

