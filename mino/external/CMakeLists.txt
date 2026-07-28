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

# 3-0. Threads
find_package(Threads REQUIRED)
   
# 3-1. spdlog
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")
 
# 3-2. nlohmann json
find_package(nlohmann_json REQUIRED)
message(STATUS "nlohmann_json version: ${nlohmann_json_VERSION}")
# find_package(nlohmann_json QUIET)
# if (nlohmann_json_FOUND)
#     message(STATUS "Found nlohmann_json ${nlohmann_json_VERSION}")
#     if (nlohmann_json_VERSION VERSION_LESS "3.11.0")
#         message(STATUS "nlohmann_json < 3.11.0 -> forcing C++17 for compatibility")
#         # Force C++17 when older nlohmann_json is present
#         set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard" FORCE)
#         set(CMAKE_CXX_STANDARD_REQUIRED ON)
#         set(CMAKE_CXX_EXTENSIONS OFF)
#     endif()
# else()
#     message(FATAL_ERROR "nlohmann_json is required. Install it or enable FetchContent fallback.")
# endif()

# 3-3. yaml-cpp
find_package(yaml-cpp REQUIRED)
message(STATUS "yaml-cpp version: ${yaml-cpp_VERSION}")

# vcpkg 환경이 아닌 일반 Linux/Rocky8 패키지 대응
if(NOT TARGET yaml-cpp::yaml-cpp)
    if(TARGET yaml-cpp)
        # 타깃 이름이 그냥 yaml-cpp로 등록된 경우 ALIAS 생성
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    else()
        # 타깃 자체가 안 만들어진 구버전 패키지인 경우 인터페이스 타깃 생성
        add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
        set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${YAML_CPP_LIBRARIES}"
        )
    endif()
endif()

# 4. 라이브러리 생성 및 소스 코드 설정
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

# 5. 헤더 파일 경로 지정
# 빌드할 때 환경과 나중에 install되어 배포되었을 때의 환경을 구분.
target_include_directories(mino_external
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 6. 사용하는 외부 라이브러리 링크
target_link_libraries(mino_external
    PUBLIC
        $<BUILD_INTERFACE:spdlog::spdlog>
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
        $<BUILD_INTERFACE:yaml-cpp::yaml-cpp>
)

# 7. [의존성] external이 core를 사용한다면 여기서 링크.
target_link_libraries(mino_external
    PUBLIC
        mino_core
)

# 8. 설치(Install) 규칙 정의
install(TARGETS mino_external
    EXPORT minoTargets # 중요: 모든 서브 모듈을 하나의 export 묶음(minoTargets)으로 관리
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 9. 공개 헤더 파일 설치 (폴더 통째로 복사)
# include/mino/core/ 형태를 유지하기 위해 include/ 내부의 mino 폴더를 타깃으로 설정.
install(DIRECTORY include/mino
    DESTINATION include
)
