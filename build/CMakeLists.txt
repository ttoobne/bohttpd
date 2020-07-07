cmake_minimum_required(VERSION 2.8)
project(bohttpd)

set(CMAKE_C_FLAGS "-std=gnu99 -g -Wall -D_GNU_SOURCE -D__USE_XOPEN -lpthread")
set(DIR_DIR "../src/")

file(GLOB_RECURSE  DIR_SRCS "${DIR_DIR}/*.c")

include_directories(../src/core ../src/http)

add_executable(bohttpd ${DIR_SRCS})
