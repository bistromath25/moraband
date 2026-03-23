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
    target_compile_options(Moraband PRIVATE -O0 -ggdb -fno-inline -fno-omit-frame-pointer -Wall -Wextra -Wpedantic)
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

# Testing setup with Google Test
enable_testing()

# Include Google Test using FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Test sources
set(TEST_SOURCES
    tests/test_movegen.cpp
    tests/test_position.cpp
    tests/test_castle.cpp
    tests/test_utils.cpp
)

# Create test executable (exclude main.cpp)
set(TEST_MAIN_SOURCES ${MAIN_SOURCES})
list(REMOVE_ITEM TEST_MAIN_SOURCES main.cpp)

add_executable(tests ${TEST_SOURCES} ${TEST_MAIN_SOURCES})

# Link test executable with Google Test
target_link_libraries(tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main
)

# Include directories
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Test-specific compile options
target_compile_options(tests PRIVATE -O0 -ggdb -Wall -Wextra -Wpedantic)

# Register tests
add_test(NAME MoveGenTests COMMAND tests --gtest_filter=MoveGenTest.*)
add_test(NAME PositionTests COMMAND tests --gtest_filter=PositionTest.*)
add_test(NAME CastleTests COMMAND tests --gtest_filter=CastleTest.*)

# Convenience targets for different build types
# These reconfigure and build in the specified mode
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -B ${CMAKE_BINARY_DIR} -S ${CMAKE_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Release
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Moraband --parallel
    COMMENT "Configuring and building in Release mode"
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -B ${CMAKE_BINARY_DIR} -S ${CMAKE_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Moraband --parallel
    COMMENT "Configuring and building in Debug mode"
)

add_custom_target(tune
    COMMAND ${CMAKE_COMMAND} -B ${CMAKE_BINARY_DIR} -S ${CMAKE_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Tune
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Moraband --parallel
    COMMENT "Configuring and building in Tune mode"
)