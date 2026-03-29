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
id name Moraband 1.3
id author Brighten Zhang
option name Hash type spin default 256 min 1 max 65536
option name Threads type spin default 1 min 1 max 16
option name Move Overhead type spin default 500 min 0 max 10000
option name UCI_Chess960 type check default false
uciok
```

[Challenge Moraband on Lichess!](https://lichess.org/@/morabandbot) 

## Details 
- Move Generation
    - [(Magic) Bitboards](https://www.chessprogramming.org/Bitboards)
    - [Zobrist Hashing](https://www.chessprogramming.org/Zobrist_Hashing)
- Search
    - [Alpha-beta search in Nega-max framework](https://www.chessprogramming.org/Negamax)
    - [Iterative deepening](https://www.chessprogramming.org/Internal_Iterative_Deepening)
    - [Aspiration Windows](https://www.chessprogramming.org/Aspiration_Windows)
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

## Compile
Compile via `cmake`
```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release # Release-NNUE
cmake --build build --parallel
```

## Credit and Resources
- [Vice chess engine tutorial](https://www.chessprogramming.org/Vice)
- [Chess programming wiki](https://www.chessprogramming.org/Main_Page)
- Pradyumna Kannan's `MagicMoves.cpp`, `MagicMoves.hpp`
- PST and piece evaluation values taken from [Rofchade](http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&sid=b2b59fa572501777ceb19d49fa17614f&start=10)
- Strong, open-source chess engines such as [Stockfish](https://www.chessprogramming.org/Stockfish), [Laser](https://github.com/jeffreyan11/laser-chess-engine), [Bit-Genie](https://github.com/Aryan1508/Bit-Genie), [Clover](https://github.com/lucametehau/CloverEngine/tree/master), [Pawn](https://github.com/ruicoelhopedro/pawn) 