cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
#   mino 의 하위 모듈 mino_core
project(mino_core VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 정보 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. 라이브러리 생성 및 소스 코드 설정
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
add_library(mino_core STATIC ${SRC_FILES})
if(MSVC)
    target_compile_options(mino_core PRIVATE "/utf-8")
endif()

# 4. 헤더 파일 경로 지정
# 빌드할 때 환경과 나중에 install되어 배포되었을 때의 환경을 구분.
target_include_directories(mino_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 5. 설치(Install) 규칙 정의
install(TARGETS mino_core 
    EXPORT minoTargets # 중요: 모든 서브 모듈을 하나의 export 묶음(minoTargets)으로 관리
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 6. 공개 헤더 파일 설치 (폴더 통째로 복사)
# include/mino/core/ 형태를 유지하기 위해 include/ 내부의 mino 폴더를 타깃으로 설정.
install(DIRECTORY include/mino
    DESTINATION include
)
