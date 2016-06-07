cmake_minimum_required (VERSION 2.8)
project (page_swap)

set(CMAKE_C_FLAGS "-g -std=c99 -Werror")

find_library(blockstore_lib block_store)
find_library(dyn_array_lib dyn_array)

add_library( page_swap SHARED src/page_swap.c)
target_link_libraries(page_swap ${blockstore_lib} ${dyn_array_lib})

install(TARGETS page_swap DESTINATION lib)
install(FILES include/page_swap.h DESTINATION include)

add_executable(runner test/runner.c)
target_link_libraries(runner page_swap ${dyn_array_lib})

set(CMAKE_CXX_FLAGS "-std=c++0x -Werror -O3")
add_executable(page_generator src/page_generator.cpp)
