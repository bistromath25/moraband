cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type" FORCE)
endif()

set(AVAILABLE_BUILD_TYPES Debug Release Release-NNUE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${AVAILABLE_BUILD_TYPES})

# Include module to check compiler flags
include(CheckCXXCompilerFlag)

# Automatic instruction set detection
set(ARCH "none")

if(NOT CMAKE_CROSSCOMPILING)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        check_cxx_compiler_flag("-mavx512f" HAS_AVX512)
        check_cxx_compiler_flag("-mavx2" HAS_AVX2)

        if(HAS_AVX512)
            set(ARCH "avx512")
        elseif(HAS_AVX2)
            set(ARCH "avx2")
        endif()

    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(ARCH "neon")
    endif()
endif()

message(STATUS "Detected instruction set: ${ARCH}")

# Sources
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

# Append instruction set to executable name
if(NOT ARCH STREQUAL "none")
    set_target_properties(Moraband PROPERTIES OUTPUT_NAME "Moraband-${ARCH}")
endif()

# Compiler flags per instruction set
if(ARCH STREQUAL "avx2")
    target_compile_definitions(Moraband PRIVATE USE_AVX2)
    target_compile_options(Moraband PRIVATE -mavx2)
elseif(ARCH STREQUAL "avx512")
    target_compile_definitions(Moraband PRIVATE USE_AVX512)
    target_compile_options(Moraband PRIVATE -mavx512f -mavx512bw -mavx512dq -mavx512vl)
elseif(ARCH STREQUAL "neon")
    target_compile_definitions(Moraband PRIVATE USE_NEON)
endif()

# Build type flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build")
    target_compile_options(Moraband PRIVATE -O0 -ggdb -fno-inline -fno-omit-frame-pointer -Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Moraband PRIVATE -O3 -DNDEBUG -Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release-NNUE")
    target_compile_definitions(Moraband PRIVATE NDEBUG USE_NNUE)
    target_compile_options(Moraband PRIVATE -O3 -Wall -Wextra -Wpedantic)
    target_sources(Moraband PRIVATE nnue.cpp)
    target_compile_options(Moraband PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()

# Common definition
target_compile_definitions(Moraband PRIVATE NAME=\"Moraband\")
