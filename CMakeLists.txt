cmake_minimum_required(VERSION 3.10.0)
project(wacko VERSION 0.1.0 LANGUAGES C)

set(C_STANDARD 11)
set(C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_compile_options("-D_FILE_OFFSET_BITS=64")

add_executable(wacko main.c)

