cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.1 LANGUAGES CXX)
set(CMAKE_SYSTEM_NAME CMAKE_HOST_SYSTEM_NAME)
set(CMAKE_CXX_COMPILER clang++)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -DNAME=Moraband -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

add_executable(Moraband main.cpp MagicMoves.cpp eval.cpp movegen.cpp position.cpp tt.cpp variation.cpp board.cpp perft.cpp search.cpp uci.cpp tune.cpp zobrist.cpp)
