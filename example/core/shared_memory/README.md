
### NOTICE

- To use shared memory, you must configure the following code in the cmake file.

```cmake
# Set for shared memory function
find_package(Threads REQUIRED)
if(UNIX AND NOT APPLE)
    set(CMAKE_THREAD_PREFER_PTHREAD ON) # Set to prioritize using pthreads
endif()
```

```cmake
# Set for shared memory function
if(UNIX AND NOT APPLE)
    target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads rt)
else()
    target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)
endif()
```
