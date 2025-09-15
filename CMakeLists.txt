cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type" FORCE)
endif()

set(AVAILABLE_BUILD_TYPES Debug Release Release-NNUE Tune)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${AVAILABLE_BUILD_TYPES})

if(NOT DEFINED ARCH)
    if(NOT CMAKE_CROSSCOMPILING)
        set(ARCH "native" CACHE STRING "Target architecture (native, avx2, avx512, neon, none)")
    else()
        set(ARCH "none" CACHE STRING "Target architecture (native, avx2, avx512, neon, none)")
    endif()
endif()
set(ALLOWED_ARCHS native avx2 avx512 neon none)
set_property(CACHE ARCH PROPERTY STRINGS ${ALLOWED_ARCHS})
if(NOT ${ARCH} IN_LIST ALLOWED_ARCHS)
    message(FATAL_ERROR "Invalid ARCH '${ARCH}'. Allowed values: ${ALLOWED_ARCHS}")
endif()

message(STATUS "Target architecture: ${ARCH}")

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

if(NOT ARCH STREQUAL "none")
    set_target_properties(Moraband PROPERTIES OUTPUT_NAME "Moraband-${ARCH}")
else()
    set_target_properties(Moraband PROPERTIES OUTPUT_NAME "Moraband")
endif()

if(ARCH STREQUAL "avx2")
    target_compile_definitions(Moraband PRIVATE USE_AVX2)
    target_compile_options(Moraband PRIVATE -mavx2)
elseif(ARCH STREQUAL "avx512")
    target_compile_definitions(Moraband PRIVATE USE_AVX512)
    target_compile_options(Moraband PRIVATE -mavx512f -mavx512bw -mavx512dq -mavx512vl)
elseif(ARCH STREQUAL "neon")
    target_compile_definitions(Moraband PRIVATE USE_NEON)
elseif(ARCH STREQUAL "native")
    if(NOT CMAKE_CROSSCOMPILING)
        target_compile_options(Moraband PRIVATE -march=native)
    else()
        message(WARNING "ARCH=native ignored because cross-compiling")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build")
    target_compile_options(Moraband PRIVATE -O0 -g -Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build")
    target_compile_options(Moraband PRIVATE -O3 -DNDEBUG -Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release-NNUE")
    message(STATUS "Release NNUE build")
    target_compile_definitions(Moraband PRIVATE NDEBUG USE_NNUE)
    target_compile_options(Moraband PRIVATE -O3 -march=native -Wall -Wextra -Wpedantic)
    target_sources(Moraband PRIVATE nnue.cpp)
elseif(CMAKE_BUILD_TYPE STREQUAL "Tune")
    message(STATUS "Tuning build")
    target_compile_definitions(Moraband PRIVATE TUNE)
    list(APPEND MAIN_SOURCES tune.cpp)
    set_source_files_properties(tune.cpp PROPERTIES COMPILE_DEFINITIONS "TUNE")
    target_sources(Moraband PRIVATE tune.cpp)
    target_compile_options(Moraband PRIVATE -O3 -march=native -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(Moraband PRIVATE NAME=\"Moraband\")
