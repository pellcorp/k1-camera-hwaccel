SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_CROSS_COMPILER "mips-linux-gnu-")
SET(CMAKE_C_COMPILER "${CMAKE_CROSS_COMPILER}gcc")
SET(CMAKE_CXX_COMPILER "${CMAKE_CROSS_COMPILER}g++")

SET(LOCAL_C_FLAGS "-EL  -march=xburst2")
SET(LOCAL_CXX_FLAGS "-EL  -march=xburst2")
##############################################
