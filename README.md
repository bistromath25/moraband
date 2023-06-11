# Moraband
Moraband is a UCI chess engine written in C++, and may be used with any chess interfaces supporting the UCI protocol such as [Banksia](https://banksiagui.com) or [Arena](http://www.playwitharena.de).

From the [Star Wars wiki](https://starwars.fandom.com/wiki/Moraband):  
*Moraband, known in antiquity as Korriban, was an Outer Rim planet that was home to the ancient Sith.*  

```
 ___  ___                _                     _ 
 |  \/  |               | |                   | | 
 |      | ___  _ __ __ _| |__   __ _ _ __   __| | 
 | |\/| |/ _ \| '__/ _` | '_ \ / _` | '_ \ / _` | 
 | |  | | (_) | | | (_| | |_) | (_| | | | | (_| | 
 \_|  |_/\___/|_|  \__,_|_.__/ \__,_|_| |_|\__,_|
Moraband, known in antiquity as Korriban, was an
 Outer Rim planet that was home to the ancient Sith

uci
id name Moraband 1.0
id author Brighten Zhang
option name Hash type spin default 256 min 1 max 65536
option name Threads type spin default 1 max 16 min 1
option name Move Overhead type spin default 500 min 0 max 10000
option name Contempt type spin default 0 min -100 max 100
uciok
```

## Details 
- Move Generation
    - [(Magic) Bitboards](https://www.chessprogramming.org/Bitboards)
    - Pseudo-Legal move generation
    - [Zobrist Hashing](https://www.chessprogramming.org/Zobrist_Hashing)
- Search
    - [Alpha-beta search in Nega-max framework](https://www.chessprogramming.org/Negamax)
    - [Iterative deepening](https://www.chessprogramming.org/Internal_Iterative_Deepening)
    - [Transposition table](https://www.chessprogramming.org/Transposition_Table)
    - [Quiescence](https://www.chessprogramming.org/Quiescence_Search)
    - [Capture order based on MVV-LVA](https://www.chessprogramming.org/MVV-LVA)
    - [Killer moves](https://www.chessprogramming.org/Killer_Move)
    - [Check extension/evasion](https://www.chessprogramming.org/Check_Extensions)
    - [Pruning using futility, reverse-futility, null-move and late-moves](https://www.chessprogramming.org/Pruning)
- Evaluation
    - [Material evaluation and Piece square tables](https://www.chessprogramming.org/Piece-Square_Tables)
    - [Pawn structure evaluation](https://www.chessprogramming.org/Pawn_Structure)
    - [Basic King safety using King safety table](https://www.chessprogramming.org/King_Safety)
    - [Tapered evaluation](https://www.chessprogramming.org/Tapered_Eval)

## Compiling
Use the provided makefile. It may be necessary to replace `-mcpu=apple-m1` with `-march=native`.

## Background
Like everyone else, I found myself playing a great deal of chess online during the 2020 lockdowns, encouraged by the advent of Netflix's *Queen's Gambit* and a general lack of things to do. It was also during this time that I developed an interest in algorithmic problem solving, and it wasn't soon before I decided to try my hand at writing my own chess-playing program, a suitable sparring partner, following along closely to BlueFeverSoft's video tutorials. Eventually I lost interest in my endeavor, weighed down by my final years of high school and the (somewhat?) easing Covid restrictions.

I "restarted" this project during the current winter break, having just completed my Fall 2022 1A term at the University of Waterloo. As was the case a few years ago, writing my own chess program is an excellent way to combine passion of chess with my love of computer programming, and no doubt bolster my rather experience-less resume. Morever, it would be an excellent opportunity to practice developing and testing "professional" software in anticipation of my first co-op. 

## Credit and Resources
- [Vice chess engine tutorial](https://www.chessprogramming.org/Vice)
- [Chess programming wiki](https://www.chessprogramming.org/Main_Page)
- Pradyumna Kannan's MagicMoves.cpp and MagicMoves.hpp
- PST and piece evaluation values taken from [Rofchade](http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&sid=b2b59fa572501777ceb19d49fa17614f&start=10)
- Strong, open-source chess engines such as [Stockfish](https://www.chessprogramming.org/Stockfish)

