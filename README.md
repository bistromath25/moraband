# Moraband
From the [Star Wars wiki](https://starwars.fandom.com/wiki/Moraband):  
*Moraband, known in antiquity as Korriban, was an Outer Rim planet that was home to the ancient Sith.*  

Moraband is a simple UCI chess engine written in C++, and may be used with any chess interfaces supporting the UCI protocol such as [Banksia](https://banksiagui.com) or [Arena](http://www.playwitharena.de).

## Details 
- Move Generation
    - [(Magic) Bitboards](https://www.chessprogramming.org/Bitboards)
    - Legal move generation
    - [Zobrist Hashing](https://www.chessprogramming.org/Zobrist_Hashing)
- Search
    - [Nega-max search framework](https://www.chessprogramming.org/Negamax)
    - [Traditional alpha-beta with scout search](https://www.chessprogramming.org/Scout)
    - [Iterative deepening and aspiration windows](https://www.chessprogramming.org/Internal_Iterative_Deepening)
    - [Transposition table](https://www.chessprogramming.org/Transposition_Table)
    - [Killer moves](https://www.chessprogramming.org/Killer_Move)
    - [Quiescence](https://www.chessprogramming.org/Quiescence_Search)
    - [Capture order based on MVV-LVA](https://www.chessprogramming.org/MVV-LVA)
    - [Check extension/evasion](https://www.chessprogramming.org/Check_Extensions)
    - [Pruning using futility, reverse-futility, null-move and late-moves](https://www.chessprogramming.org/Pruning)
- Evaluation
    - [Material evaluation and Piece square tables](https://www.chessprogramming.org/Piece-Square_Tables)
    - [Pawn structure evaluation](https://www.chessprogramming.org/Pawn_Structure)
    - [Basic King safety using King safety table](https://www.chessprogramming.org/King_Safety)
    - [Tapered evaluation](https://www.chessprogramming.org/Tapered_Eval)

## Compiling
Use the provided makefile. Moraband is guaranteed to compile and run on M1 Mac.

## Background
Coming soon! Spend more gems to speed this up. 

## Credit and Resources
- [Vice chess engine tutorial](https://www.chessprogramming.org/Vice)
- [Chess programming wiki](https://www.chessprogramming.org/Main_Page)
- Pradyumna Kannan's MagicMoves.cpp and MagicMoves.hpp
- PST and piece evaluation values taken from [Rofchade](http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&sid=b2b59fa572501777ceb19d49fa17614f&start=10)
- Various evaluation and search ideas taken from [Stockfish](https://www.chessprogramming.org/Stockfish)

