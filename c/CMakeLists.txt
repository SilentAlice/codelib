cmake_minimum_required(VERSION 3.16)

project(codelib C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(csnip STATIC
    src/csnip_renum.c
)
target_include_directories(csnip PUBLIC include)

add_subdirectory(tests)
