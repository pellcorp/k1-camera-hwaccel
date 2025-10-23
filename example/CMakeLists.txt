project(CameraStereo LANGUAGES C CXX)
cmake_minimum_required(VERSION 2.8.10)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(get_syncframe syncframe.c)
SET_TARGET_PROPERTIES("get_syncframe" PROPERTIES
    OUTPUT_NAME "get_syncframe")
target_link_libraries(get_syncframe syncframe)

add_executable(get_monoframe monoframe.c)
SET_TARGET_PROPERTIES("get_monoframe" PROPERTIES
    OUTPUT_NAME "get_monoframe")
target_link_libraries(get_monoframe syncframe)

add_executable(reset_syncframe reset_syncframe.c)
SET_TARGET_PROPERTIES("reset_syncframe" PROPERTIES
    OUTPUT_NAME "reset_syncframe")
target_link_libraries(reset_syncframe syncframe)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lstdc++")

add_executable(osd_syncframe_client osd_syncframe_client.cpp)
SET_TARGET_PROPERTIES("osd_syncframe_client" PROPERTIES
    OUTPUT_NAME "osd_syncframe_client")

target_link_libraries (osd_syncframe_client
  pthread
  rt
  osdClient
  syncframe)

install(TARGETS get_syncframe DESTINATION bin)
install(TARGETS get_monoframe DESTINATION bin)
install(TARGETS reset_syncframe DESTINATION bin)
install(TARGETS osd_syncframe_client DESTINATION bin)
