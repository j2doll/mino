function(use_mino_core EXE_NAME MINO_DIR)
    if(TARGET mino_core)
        message(STATUS "Linking to in-tree target: mino_core")
        target_link_libraries(${EXE_NAME} PRIVATE mino_core)
    else()
        message(STATUS "Linking to out-of-tree(external) target: mino_core")
        if(MINO_DIR)
            find_path(MINO_CORE_INCLUDE_DIR NAMES "mino/core/core.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_CORE_LIBRARY NAMES mino_core
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_CORE_INCLUDE_DIR NAMES "mino/core/core.hpp")
            find_library(MINO_CORE_LIBRARY NAMES mino_core)
        endif()

        if(NOT MINO_CORE_INCLUDE_DIR OR NOT MINO_CORE_LIBRARY)
            message(FATAL_ERROR "mino_core not found. Either add mino as a subdirectory or set -DMINO_DIR=/path/to/mino/install")
        endif()

        add_library(mino_core::mino_core UNKNOWN IMPORTED)
        set_target_properties(mino_core::mino_core PROPERTIES
            IMPORTED_LOCATION "${MINO_CORE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MINO_CORE_INCLUDE_DIR}"
        )

        target_link_libraries(${EXE_NAME} PRIVATE mino_core::mino_core)
        message(STATUS "Using external mino_core from ${MINO_DIR}")
    endif()
endfunction()
