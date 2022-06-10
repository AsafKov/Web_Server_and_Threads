cmake_minimum_required(VERSION 3.22)
project(threads)

set(CMAKE_CXX_STANDARD 11)

add_executable(threads output.c queue.h request.c segel.c server.c)
