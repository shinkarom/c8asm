cmake_minimum_required(VERSION 3.13.2)

set(PROJ_NAME c8asm)
project(${PROJ_NAME} CXX)
set(EXE_NAME ${PROJ_NAME})

set(S ${CMAKE_BINARY_DIR}/..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${S}/bin)
include_directories(${S}/src)

file(GLOB SRCS ${S}/src/*.cpp)
file(GLOB HPPS ${S}/src/*.hpp)
file(GLOB HS ${S}/src/*.h)

add_executable(${PROJ_NAME} ${SRCS} ${HPPS} ${HS})
if(TRUE)
 target_compile_options(${EXE_NAME} PUBLIC -g -std=c++17 -Wall)
 target_link_options(${EXE_NAME} PUBLIC)
else()
 target_compile_options(${EXE_NAME} PUBLIC -O2 -std=c++17 -Wall)
 target_link_options(${EXE_NAME} PUBLIC -s)
endif()