cmake_minimum_required(VERSION 3.24)

project(mino_example_project)

option(ENABLE_EXAMPLE_CORE            "Build core examples"            OFF)
if(ENABLE_EXAMPLE_CORE)
    add_subdirectory(core)
endif()

option(ENABLE_EXAMPLE_EXTERNAL        "Build external examples"        OFF)
if(ENABLE_EXAMPLE_EXTERNAL)
    add_subdirectory(external)
endif()

option(ENABLE_EXAMPLE_NETWORK         "Build network examples"         OFF)
if(ENABLE_EXAMPLE_NETWORK)
    add_subdirectory(network)
endif()

option(ENABLE_EXAMPLE_NETWORK_CURL    "Build network_curl examples"    OFF)
if(ENABLE_EXAMPLE_NETWORK_CURL)
    add_subdirectory(network_curl)
endif()

option(ENABLE_EXAMPLE_NETWORK_LIBSSH2 "Build network_libssh2 examples" OFF)
if(ENABLE_EXAMPLE_NETWORK_LIBSSH2)
    add_subdirectory(network_libssh2)
endif()

option(ENABLE_EXAMPLE_NETWORK_OPENSSL "Build network_openssl examples" OFF)
if(ENABLE_EXAMPLE_NETWORK_OPENSSL)
    add_subdirectory(network_openssl)
endif()

option(ENABLE_EXAMPLE_TEMPLATE        "Build template examples"        OFF)
if(ENABLE_EXAMPLE_TEMPLATE)
    add_subdirectory(template)
endif()

