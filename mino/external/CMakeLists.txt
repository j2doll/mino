cmake_minimum_required(VERSION 3.24)

# 1. 프로젝트 명과 버전 설정
project(mino_external VERSION 1.0.0 LANGUAGES CXX)

# 2. C++ 표준 정보 설정
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 3. 외부 의존성 설정 
   
# 3-1. spdlog
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")
 
# 3-2. nlohmann json
find_package(nlohmann_json REQUIRED)
message(STATUS "nlohmann_json version: ${nlohmann_json_VERSION}")

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
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
)

# 8. [의존성] external이 core를 사용한다면 여기서 링크.
target_link_libraries(mino_external
    PUBLIC
        mino_core
)

# 9. 설치(Install) 규칙 정의
install(TARGETS mino_external
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
