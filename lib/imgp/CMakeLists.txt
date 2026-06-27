cmake_minimum_required(VERSION 3.10)
project(imgp LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(imgp STATIC
    src/util/bit_stream.c
    src/imgp.c
    src/formats/ppm.c
    src/formats/bmp.c
    src/formats/png.c
)

target_include_directories(imgp 
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)