cmake_minimum_required(VERSION 3.14)
project(c_toolbox C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add Unity
add_library(unity
    third_party/Unity/src/unity.c
)
target_include_directories(unity PUBLIC third_party/Unity/src)

# Our slice lib
add_library(slice STATIC slice.c)
target_include_directories(slice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Test binary
add_executable(slice_test slice_test.c)
target_link_libraries(slice_test PRIVATE slice unity)

enable_testing()
add_test(NAME SliceTest COMMAND slice_test)
