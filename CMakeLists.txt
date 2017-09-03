cmake_minimum_required(VERSION 3.8)
project(tftp)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)
add_executable(tftp ${SOURCE_FILES})