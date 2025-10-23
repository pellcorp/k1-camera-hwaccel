include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

#message("${CMAKE_CURRENT_SOURCE_DIR}/../include")

set(v4l2enc_SRCS
    v4l2_jpegenc.c)
#sgemm.cpp)

add_library(v4l2enc SHARED ${v4l2enc_SRCS})

SET_TARGET_PROPERTIES("v4l2enc" PROPERTIES
    OUTPUT_NAME "v4l2enc")

target_link_libraries(v4l2enc v4l2_ingenic)
install(TARGETS v4l2enc DESTINATION lib)
