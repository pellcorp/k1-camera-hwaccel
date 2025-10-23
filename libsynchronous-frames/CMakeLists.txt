project(SYNCFRAMES)
cmake_minimum_required(VERSION 2.8.10)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LOCAL_CXX_FLAGS} -fPIC")

add_subdirectory(v4l2)
add_subdirectory(syncframe)
add_subdirectory(v4l2enc)
add_subdirectory(test)

option(SYNCFRAME_BUILD_EXAMPLE "Build examples" OFF)
if(SYNCFRAME_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

install(FILES include/syncframe.h DESTINATION include)
install(FILES include/v4l2-jpegenc.h DESTINATION include)
