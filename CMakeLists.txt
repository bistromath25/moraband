cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type" FORCE)
endif()

set(AVAILABLE_BUILD_TYPES Debug Release Tune)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${AVAILABLE_BUILD_TYPES})

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

add_executable(Moraband ${MAIN_SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build")
    target_compile_options(Moraband PRIVATE -O0 -g -Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build")
    target_compile_options(Moraband PRIVATE -O3 -DNDEBUG -Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "Tune")
    message(STATUS "Tuning build")
    target_compile_definitions(Moraband PRIVATE TUNE)
    list(APPEND MAIN_SOURCES tune.cpp)
    set_source_files_properties(tune.cpp PROPERTIES COMPILE_DEFINITIONS "TUNE")
    target_sources(Moraband PRIVATE tune.cpp)
    target_compile_options(Moraband PRIVATE -O3 -march=native -Wall -Wextra -Wpedantic)
endif()

if(NOT CMAKE_CROSSCOMPILING AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(Moraband PRIVATE -march=native)
endif()

target_compile_definitions(Moraband PRIVATE NAME=\"Moraband\")
