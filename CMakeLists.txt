cmake_minimum_required(VERSION 3.14)

# Force use of Clang
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(c_toolbox C)

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

# Add AddressSanitizer for tests
# if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
#     target_compile_options(slice_test PRIVATE -fsanitize=address -fno-omit-frame-pointer -g -O1)
#     target_link_options(slice_test PRIVATE -fsanitize=address)
    
#     # Also apply ASan to the slice library to ensure consistent instrumentation
#     target_compile_options(slice PRIVATE -fsanitize=address -fno-omit-frame-pointer -g -O1)
#     target_link_options(slice PRIVATE -fsanitize=address)
# endif()

enable_testing()
add_test(NAME SliceTest COMMAND slice_test)