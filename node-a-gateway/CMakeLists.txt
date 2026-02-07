cmake_minimum_required(VERSION 3.16)

# Enable C11 and strict warnings for MISRA-C compliance
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -Wconversion -Wsign-conversion)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node-a-gateway)
