cmake_minimum_required (VERSION 2.8)
project(bitmap)

set(CMAKE_C_FLAGS "-std=c99 -Wall -Werror")
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_library(${PROJECT_NAME} SHARED src/${PROJECT_NAME}.c)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES include/${PROJECT_NAME}.h DESTINATION include)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
	CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)




# tester gibberish

set(CMAKE_BUILD_TYPE Debug)
enable_testing()
add_executable(bitmap_tester test/test.c)
add_test(tester bitmap_tester)
