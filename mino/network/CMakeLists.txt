cmake_minimum_required(VERSION 3.20)
project(mino_network LANGUAGES CXX)

# 1. 라이브러리 타깃 생성
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

# 2. 헤더 파일 경로 지정
# 빌드할 때 환경과 나중에 install되어 배포되었을 때의 환경을 구분합니다.
target_include_directories(mino_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# [의존성] network가 core를 사용한다면 여기서 링크해 줍니다.
# 이렇게 하면 mino_core의 헤더 경로와 링크 정보가 mino_network로 자동 전파됩니다.
target_link_libraries(mino_network
    PUBLIC
        mino_core
)

# 설치 규칙 정의 (동일한 EXPORT 묶음 이름인 'minoTargets'를 사용해야 합니다)
install(TARGETS mino_network
    EXPORT minoTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/mino
    DESTINATION include
)
