cmake_minimum_required(VERSION 3.5)

project(lab)

string(REGEX MATCH "^(.*)[\\/]misc[\\/].*$" repo "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_MATCH_1}/tools/labs.cmake)
