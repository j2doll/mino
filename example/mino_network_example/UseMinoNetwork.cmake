function(use_mino_network EXE_NAME MINO_DIR)
    # Warn if caller didn't create the executable target
    if(NOT TARGET ${EXE_NAME})
        message(WARNING "use_mino_network: target ${EXE_NAME} does not exist. Proceeding to configure linking; create the executable target before calling this function.")
    endif()

    if(TARGET mino_network)
        message(STATUS "Linking to in-tree target: mino_network")
        target_link_libraries(${EXE_NAME} PRIVATE mino_network)
    else()
        message(STATUS "Linking to out-of-tree target: mino_network")

        if(MINO_DIR)
            find_path(MINO_INCLUDE_DIR NAMES "mino/network/interface/network_interface.hpp"
                PATHS "${MINO_DIR}/include" "${MINO_DIR}" NO_DEFAULT_PATH)
            find_library(MINO_NETWORK_LIBRARY NAMES mino_network
                PATHS "${MINO_DIR}/lib" "${MINO_DIR}" NO_DEFAULT_PATH)
        else()
            find_path(MINO_INCLUDE_DIR NAMES "mino/network/interface/network_interface.hpp")
            find_library(MINO_NETWORK_LIBRARY NAMES mino_network)
        endif()

        if(NOT MINO_INCLUDE_DIR OR NOT MINO_NETWORK_LIBRARY)
            message(FATAL_ERROR "mino_network not found. Either add mino as a subdirectory or set -DMINO_DIR=/path/to/mino/install")
        endif()

        if(NOT TARGET mino_network::mino_network AND MINO_NETWORK_LIBRARY)
            add_library(mino_network::mino_network UNKNOWN IMPORTED)
            set_target_properties(mino_network::mino_network PROPERTIES
                IMPORTED_LOCATION "${MINO_NETWORK_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINO_INCLUDE_DIR}"
            )
        endif()

        target_link_libraries(${EXE_NAME} PRIVATE mino_network::mino_network)
        message(STATUS "Using external mino_network from ${MINO_DIR}")
    endif()
endfunction()
