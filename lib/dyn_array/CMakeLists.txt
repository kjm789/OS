cmake_minimum_required (VERSION 2.8)
project(dyn_array)

set(CMAKE_C_FLAGS "-std=c99 -Wall -Werror")
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_library(${PROJECT_NAME} SHARED src/${PROJECT_NAME}.c)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES include/${PROJECT_NAME}.h DESTINATION include)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
	CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)




# tester gibberish


# http://www.cmake.org/Wiki/CMakeEmulateMakeCheck

set(CMAKE_BUILD_TYPE Debug)
enable_testing()
add_executable(dyn_array_tester test/tester.c)
add_test(tester dyn_array_tester)

# testing like this just doesn't work well with what I have
# since it's not written for CTest/Check
# but it's enough for a flat did it work or not sort of thing.
# probably not helpful in the slightest, but it's at least
# something to refrence in the future
