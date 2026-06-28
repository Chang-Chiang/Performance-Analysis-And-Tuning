cmake_minimum_required(VERSION 3.5)

project(lab)

include_directories(lua)
file(GLOB EXT_LAB_srcs lua/*.c)
file(GLOB EXT_VALIDATE_srcs lua/*.c)

# Step 3: 使用 profile 数据编译 (GCC)
set(CMAKE_C_FLAGS "-fprofile-use=${CMAKE_CURRENT_SOURCE_DIR}/build_pgo" ${CMAKE_C_FLAGS})

set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/reference_output.txt")

string(REGEX MATCH "^(.*)[\\/]misc[\\/].*$" repo "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_MATCH_1}/tools/labs.cmake)
