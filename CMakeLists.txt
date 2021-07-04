cmake_minimum_required(VERSION 3.15)
project(LJson)

set(CMAKE_CXX_STANDARD 14)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "debug mode")
    set(CXX_FLAGS "${CXX_FLAGS} -g")
else()
    message(STATUS "release mode")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

set(CXX_FLAGS "${CXX_FLAGS} -std=c++11 -Wall -pedantic -Werror -Wextra")

add_subdirectory(Step1)
add_subdirectory(Step2)
