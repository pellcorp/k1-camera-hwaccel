include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

message("${CMAKE_CURRENT_SOURCE_DIR}/../include")

set(syncframe_SRCS
    syncframe.c)
#sgemm.cpp)

add_library(syncframe SHARED ${syncframe_SRCS})

SET_TARGET_PROPERTIES("syncframe" PROPERTIES
  OUTPUT_NAME "syncframe")

target_link_libraries(syncframe
  v4l2_ingenic
  pthread)

install(TARGETS syncframe DESTINATION lib)
