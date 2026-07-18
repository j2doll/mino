cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
#  설치본 및 패키지 형태로 만들러면 버전 정보가 필수.
project(mino_project VERSION 1.0.0 LANGUAGES CXX)
  
# 2. C++ 표준 정보 설정
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 
   
# 3. 출력 디렉터리 설정
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
 
# 4. 하위 mino 폴더 (라이브러리 소스 코드)
add_subdirectory(mino) 

# 5. 패키지 추출(Export) 및 CMake 설정 파일 생성
include(CMakePackageConfigHelpers)

# 6. 서브 모듈들이 차곡차곡 쌓아놓은 'minoTargets' 묶음을 파일로 내보내기
install(EXPORT minoTargets
    FILE minoTargets.cmake
    NAMESPACE mino::           # 외부 유저가 mino::core 등으로 사용하게 함
    DESTINATION lib/cmake/mino
)

# 7. 버전 정보 파일 생성
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 8. 외부 프로젝트용 Config 파일 생성
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    "include(CMakeFindDependencyMacro)\n"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/minoTargets.cmake\")\n"
)

# 9. Config 파일들을 설치 경로로 복사
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/minoConfigVersion.cmake
    DESTINATION lib/cmake/mino
)
