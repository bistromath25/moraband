cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.2 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_SYSTEM_NAME CMAKE_HOST_SYSTEM_NAME)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MAIN_SOURCES
    main.cpp
    MagicMoves.cpp
    eval.cpp
    movegen.cpp
    position.cpp
    tt.cpp
    variation.cpp
    board.cpp
    perft.cpp
    search.cpp
    uci.cpp
    zobrist.cpp
)

option(TUNE "Tuning build" OFF)
if(TUNE)
    message(STATUS "Tuning build")
    add_definitions(-DTUNE)
    list(APPEND MAIN_SOURCES tune.cpp)
endif()

add_executable(Moraband ${MAIN_SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(Moraband PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    target_compile_options(Moraband PRIVATE -march=native)
endif()

target_compile_definitions(Moraband PRIVATE NAME=\"Moraband\")
