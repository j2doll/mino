function(use_mino_external EXE_NAME MINO_DIR)
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE "/utf-8")
    endif()

    # Warn if caller didn't create the executable target (we expect the caller to create it)
    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_external: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the executable target before calling this function.")
    endif()

    if(TARGET mino_external)
        message(STATUS "Linking to in-tree target: mino_external")
        target_link_libraries(${EXE_NAME} PRIVATE mino_external)
    elseif(TARGET mino_core)
        message(STATUS "Linking to in-tree target: mino_core")
        if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE mino_core rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE mino_core)
        endif()
    else()
        message(STATUS "Linking to out-of-tree(external) targets: mino_core / mino_external")

        if(MINO_DIR)
            find_path(MINO_INCLUDE_DIR NAMES "mino/core/core.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_EXTERNAL_LIBRARY NAMES mino_external
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_CORE_LIBRARY NAMES mino_core
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_INCLUDE_DIR NAMES "mino/core/core.hpp")
            find_library(MINO_EXTERNAL_LIBRARY NAMES mino_external)
            find_library(MINO_CORE_LIBRARY NAMES mino_core)
        endif()

        if(NOT MINO_INCLUDE_DIR OR (NOT MINO_EXTERNAL_LIBRARY AND NOT MINO_CORE_LIBRARY))
            message(FATAL_ERROR "mino libraries not found. Either add mino as a subdirectory or set -DMINO_DIR=/path/to/mino/install")
        endif()

        if(NOT TARGET mino_core::mino_core AND MINO_CORE_LIBRARY)
            add_library(mino_core::mino_core UNKNOWN IMPORTED)
            set_target_properties(mino_core::mino_core PROPERTIES
                IMPORTED_LOCATION "${MINO_CORE_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
            )
        endif()

        if(NOT TARGET mino_external::mino_external AND MINO_EXTERNAL_LIBRARY)
            add_library(mino_external::mino_external UNKNOWN IMPORTED)
            set_target_properties(mino_external::mino_external PROPERTIES
                IMPORTED_LOCATION "${MINO_EXTERNAL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
            )
        endif()

        # third-party dependencies
        find_package(Threads REQUIRED)
        find_package(nlohmann_json CONFIG REQUIRED)
        find_package(spdlog CONFIG REQUIRED)
        find_package(yaml-cpp CONFIG REQUIRED)

        if(UNIX AND NOT APPLE)
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads rt)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)
        endif()

        target_link_libraries(${EXE_NAME} PRIVATE
            nlohmann_json::nlohmann_json
            spdlog::spdlog
        )

        # Detect the yaml-cpp target name provided by the package and use it
        if(TARGET yaml-cpp::yaml-cpp)
            set(YAMLCPP_TARGET yaml-cpp::yaml-cpp)
        elseif(TARGET yaml-cpp)
            set(YAMLCPP_TARGET yaml-cpp)
        elseif(TARGET YAML::YAML)
            set(YAMLCPP_TARGET YAML::YAML)
        else()
            message(FATAL_ERROR "yaml-cpp target not found after find_package. Expected one of: yaml-cpp::yaml-cpp, yaml-cpp, YAML::YAML")
        endif()
        target_link_libraries(${EXE_NAME} PRIVATE ${YAMLCPP_TARGET})

        if(TARGET mino_external::mino_external)
            target_link_libraries(${EXE_NAME} PRIVATE mino_external::mino_external)
            message(STATUS "Using external mino_external from ${MINO_DIR}")
        else()
            target_link_libraries(${EXE_NAME} PRIVATE mino_core::mino_core)
            message(STATUS "Using external mino_core from ${MINO_DIR}")
        endif()
    endif()
endfunction()
