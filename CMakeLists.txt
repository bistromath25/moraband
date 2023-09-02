cmake_minimum_required(VERSION 3.18)
project(Moraband VERSION 1.1 LANGUAGES CXX)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER g++)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -pipe -DNAME=Moraband")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

add_executable(Moraband main.cpp MagicMoves.cpp eval.cpp movegen.cpp position.cpp tt.cpp variation.cpp board.cpp perft.cpp search.cpp uci.cpp zobrist.cpp)
