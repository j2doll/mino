cmake_minimum_required(VERSION 3.20)
project(mino_core LANGUAGES CXX)

# 1. 라이브러리 타깃 생성
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
 
# 2. 헤더 파일 경로 지정
# 빌드할 때 환경과 나중에 install되어 배포되었을 때의 환경을 구분합니다.
target_include_directories(mino_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 3. 설치(Install) 규칙 정의
install(TARGETS mino_core
    EXPORT minoTargets # 중요: 모든 서브 모듈을 하나의 export 묶음(minoTargets)으로 관리
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 4. 공개 헤더 파일 설치 (폴더 통째로 복사)
# include/mino/core/ 형태를 유지하기 위해 include/ 내부의 mino 폴더를 타깃으로 잡습니다.
install(DIRECTORY include/mino
    DESTINATION include
)
