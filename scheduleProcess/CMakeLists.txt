cmake_minimum_required (VERSION 2.8.0)
project (scheduling_algorithms)

set(CMAKE_C_FLAGS "-std=gnu99 -Wall -Werror -g -O0")
#set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Werror -g -O0")


find_library(dyn_array_lib dyn_array)

add_library( process_scheduling SHARED src/process_scheduling.c)
target_link_libraries(process_scheduling ${dyn_array_lib})

install(TARGETS process_scheduling DESTINATION lib)
install(FILES include/processing_scheduling.h DESTINATION include)

add_executable(ps_tester test/tester.c)
target_link_libraries(ps_tester process_scheduling ${dyn_array_lib} m)

add_executable(ps_analysis test/analysis.c)
target_link_libraries(ps_analysis process_scheduling ${dyn_array_lib})

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Werror -O3")
add_executable(pcb_generator src/pcb_generator.cpp)
