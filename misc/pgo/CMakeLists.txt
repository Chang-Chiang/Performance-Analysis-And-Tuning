cmake_minimum_required(VERSION 3.5)

project(lab)

include_directories(lua)
file(GLOB EXT_LAB_srcs lua/*.c)
file(GLOB EXT_VALIDATE_srcs lua/*.c)

# 新增选项, 以启用PGO编译
# clang 标志
set(CMAKE_C_FLAGS "-fprofile-instr-generate" ${CMAKE_C_FLAGS})

# gcc 标志
# set(CMAKE_C_FLAGS "-fprofile-generate" ${CMAKE_C_FLAGS})

set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/reference_output.txt")

string(REGEX MATCH "^(.*)[\\/]misc[\\/].*$" repo "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_MATCH_1}/tools/labs.cmake)
