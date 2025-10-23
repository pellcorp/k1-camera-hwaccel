include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

message("${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(v4l2_SRCS
    v4l2.c)
#sgemm.cpp)

add_library(v4l2_ingenic STATIC ${v4l2_SRCS})

SET_TARGET_PROPERTIES("v4l2_ingenic" PROPERTIES
    OUTPUT_NAME "v4l2_ingenic")



