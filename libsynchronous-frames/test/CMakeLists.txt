project(CameraStereo LANGUAGES C CXX)
cmake_minimum_required(VERSION 2.8.10)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(syncframe_test test.c)
SET_TARGET_PROPERTIES("syncframe_test" PROPERTIES
    OUTPUT_NAME "syncframe_test")
target_link_libraries(syncframe_test syncframe v4l2enc)

install(TARGETS syncframe_test DESTINATION bin)
