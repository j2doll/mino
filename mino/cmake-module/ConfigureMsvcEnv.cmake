# Guard against multiple inclusion
include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Function: configure_vcpkg_manifest
# Purpose : Configure vcpkg manifest directory and mode on MSVC builds
#           only if vcpkg.json does not exist in the top-level source directory.
# Usage   : configure_vcpkg_manifest(<target_dir>)
# -----------------------------------------------------------------------------
function(configure_vcpkg_manifest target_dir)
    # Skip custom configuration if vcpkg.json already exists in the project root directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json")
        message(STATUS "[vcpkg] Found vcpkg.json in root directory (${CMAKE_SOURCE_DIR}). Skipping custom manifest configuration.")
        return()
    endif()

    # Skip custom configuration if not using MSVC
    if(NOT target_dir)
        message(WARNING "[vcpkg] Target directory is not specified.")
        return()
    endif()

    set(VCPKG_MANIFEST_DIR "${target_dir}" CACHE PATH "Path to vcpkg manifest directory" FORCE)
    set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Enable vcpkg manifest mode" FORCE)
    message(STATUS "[vcpkg] Manifest configured at: ${VCPKG_MANIFEST_DIR}")
endfunction()

# -----------------------------------------------------------------------------
# Function: load_cmake_settings_json
# Purpose : Parse CMakeSettings.json and export variables into the CMake cache
# Usage   : load_cmake_settings_json(<settings_dir> [project_root_dir])
# -----------------------------------------------------------------------------
function(load_cmake_settings_json settings_dir)
    # CMakeSettings.json is primarily used for MSVC environments; skip on non-MSVC platforms.
    if(NOT MSVC)
        return() 
    endif()

    # Skip custom configuration if CMakeSettings.json already exists in the project root directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/CMakeSettings.json")
        message(STATUS "[CMakeSettings] Found CMakeSettings.json in root directory (${CMAKE_SOURCE_DIR}). Skipping custom manifest configuration.")
        return()
    endif()

    # Set the path to CMakeSettings.json in the specified settings_dir
    set(settings_file "${settings_dir}/CMakeSettings.json")

    # Optional 2nd argument: custom project root directory for ${projectDir} expansion
    if(ARGC GREATER 1)
        set(proj_root "${ARGV1}")
    else()
        set(proj_root "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
    endif()

    if(NOT EXISTS "${settings_file}")
        message(STATUS "[CMakeSettings] File not found at: ${settings_file}")
        return()
    endif()

    file(READ "${settings_file}" json_content)

    string(JSON var_count ERROR_VARIABLE json_err LENGTH "${json_content}" "configurations" 0 "variables")

    if(NOT json_err AND var_count GREATER 0)
        math(EXPR var_last "${var_count} - 1")

        foreach(idx RANGE ${var_last})
            string(JSON v_name GET "${json_content}" "configurations" 0 "variables" ${idx} "name")
            string(JSON v_value GET "${json_content}" "configurations" 0 "variables" ${idx} "value")

            # Expand ${env.VAR_NAME}
            if(v_value MATCHES [[\$\{env\.([^}]+)\}RS]])
                set(env_var_name "${CMAKE_MATCH_1}")
                set(env_var_val "$ENV{${env_var_name}}")
                string(REPLACE "${CMAKE_MATCH_0}" "${env_var_val}" v_value "${v_value}")
            endif()

            # Expand ${projectDir}
            if(v_value MATCHES [[\$\{projectDir\}]])
                string(REPLACE "${CMAKE_MATCH_0}" "${proj_root}" v_value "${v_value}")
            endif()

            # Set variable to CACHE
            set(${v_name} "${v_value}" CACHE STRING "Loaded from CMakeSettings.json" FORCE)
            message(STATUS "[CMakeSettings.json] Loaded: ${v_name} = ${v_value}")
        endforeach()
    endif()
endfunction()
