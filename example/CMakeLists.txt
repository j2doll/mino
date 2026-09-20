cmake_minimum_required(VERSION 3.24)

project(mino_example_project)

add_subdirectory(core)

add_subdirectory(external)

add_subdirectory(network)
add_subdirectory(network_curl)
add_subdirectory(network_libssh2)
add_subdirectory(network_openssl)

add_subdirectory(template) # example template for mino core/external/network library




