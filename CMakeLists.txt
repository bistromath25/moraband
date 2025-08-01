cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.2 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_SYSTEM_NAME CMAKE_HOST_SYSTEM_NAME)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -DNAME=Moraband -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv8-a+simd -mfpu=neon -DNDEBUG")

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

option(USE_NNUE "NNUE build" OFF)
if(USE_NNUE)
    message(STATUS "NNUE build")
    add_definitions(-DUSE_NNUE)
    list(APPEND MAIN_SOURCES nnue.cpp)
endif()

option(TUNE "Tuning build" OFF)
if(TUNE)
    message(STATUS "Tuning build")
    add_definitions(-DTUNE)
    list(APPEND MAIN_SOURCES tune.cpp)
endif()

add_executable(Moraband ${MAIN_SOURCES})