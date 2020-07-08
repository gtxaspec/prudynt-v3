cmake_minimum_required(VERSION 2.6)

project(wyzedom)
aux_source_directory(./src SRC_LIST)

set(CMAKE_CXX_FLAGS "--std=c++11 -g -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(./src)
