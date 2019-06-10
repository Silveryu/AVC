cmake_minimum_required(VERSION 2.8)

SET (CMAKE_BUILD_TYPE "Release")
#SET (CMAKE_BUILD_TYPE "Debug")

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17")
SET (CMAKE_CXX_FLAGS_RELEASE "-O3")
SET (CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address")

SET (BASE_DIR ${CMAKE_SOURCE_DIR} )
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/../bin)

add_executable (LPAencode LPAencode.cpp)
target_link_libraries (LPAencode sndfile)

add_executable (LPAdecode LPAdecode.cpp)
target_link_libraries (LPAdecode sndfile)

add_executable (wavcmp wavcmp.cpp)
target_link_libraries (wavcmp sndfile)