cmake_minimum_required(VERSION 3.24)

project(mino_super_project)

# mino library
add_subdirectory(mino) 

# example project
option(ENABLE_EXAMPLES "Build example subprojects" OFF)
if(ENABLE_EXAMPLES)
   add_subdirectory(example)
endif()

# test project
option(ENABLE_TESTS    "Build test subprojects"    OFF)
if(ENABLE_TESTS)
   add_subdirectory(tests) 
endif()



