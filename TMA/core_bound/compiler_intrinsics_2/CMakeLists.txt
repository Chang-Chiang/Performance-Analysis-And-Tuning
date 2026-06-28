cmake_minimum_required(VERSION 3.5)

project(lab)

set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/inputs/")
set(LAB_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/inputs/")

string(REGEX MATCH "^(.*)[\\/]core_bound[\\/].*$" repo "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_MATCH_1}/tools/labs.cmake)
