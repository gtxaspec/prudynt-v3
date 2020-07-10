cmake_minimum_required(VERSION 2.6)
project(prudynt)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR mips)
set(tools ${PROJECT_SOURCE_DIR}/toolchain)
set(CMAKE_C_COMPILER ${tools}/bin/mips-linux-uclibc-gnu-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/mips-linux-uclibc-gnu-g++)

aux_source_directory(./src SRC_LIST)

set(CMAKE_CXX_FLAGS "--std=c++11 -Wall -Wextra -Wno-unused-parameter -O3")
set(CMAKE_LINKER_FLAGS "-static")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(./src)
