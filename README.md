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
option name Threads type spin default 1 min 1 max 16
option name Move Overhead type spin default 500 min 0 max 10000
option name Contempt type spin default 0 min -100 max 100
uciok
```

[Challenge Moraband on Lichess!](https://lichess.org/@/morabandbot) 

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
    - Classical
        - [Material evaluation and Piece square tables](https://www.chessprogramming.org/Piece-Square_Tables)
        - [Pawn structure evaluation](https://www.chessprogramming.org/Pawn_Structure)
        - [Basic King safety using King safety table](https://www.chessprogramming.org/King_Safety)
        - [Tapered evaluation](https://www.chessprogramming.org/Tapered_Eval)
    - [NNUE](https://www.chessprogramming.org/NNUE)
        - (768 -> 128) x 2 ->  1
        - Apple NEON intrinsics

## Compile
Compile via `cmake`
```
mkdir build
cd build
cmake ..  # -DTUNE=ON for tuning, -DUSE_NNUE=ON for NNUE
make
```

## Tune
Tune material values using [Texel's method](https://www.chessprogramming.org/Texel%27s_Tuning_Method). Provide a `fens` file containing one FEN per line like below
```
// FEN; result
r1bqk2r/2p2ppp/2p5/p3pn2/1bB5/2NP2P1/PPP1NP1P/R1B1K2R w KQkq -; 0-1
8/8/4kp2/8/5K2/6p1/6P1/8 b - -; 1/2-1/2
r4rk1/3p2pp/p7/1pq2p2/2n2P2/P2Q3P/2P1NRP1/R5K1 w - -; 1/2-1/2
2rqk1n1/p6p/1p1pp3/8/4P3/P1b5/R2N1PPP/3QR1K1 w - -; 1-0
```
and tune via `tune fens`.

## NNUE
Moraband includes support for NNUE (Efficiently Updatable Neural Network) evaluation, using a basic 768 -> 128 -> 1 architecture. The network is a two-layer feedforward model with clipped ReLU activation and a single scalar output. Input features use a 768-dimensional one-hot encoding representing the presence of each of the 12 piece types across all 64 squares, encoded from White's perspective. Inference is accelerated with Apple NEON intrinsics on ARM64 (Apple Silicon) CPUs, with a scalar fallback for other platforms.

## Credit and Resources
- [Vice chess engine tutorial](https://www.chessprogramming.org/Vice)
- [Chess programming wiki](https://www.chessprogramming.org/Main_Page)
- Pradyumna Kannan's `MagicMoves.cpp`, `MagicMoves.hpp`
- PST and piece evaluation values taken from [Rofchade](http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&sid=b2b59fa572501777ceb19d49fa17614f&start=10)
- Strong, open-source chess engines such as [Stockfish](https://www.chessprogramming.org/Stockfish), [Laser](https://github.com/jeffreyan11/laser-chess-engine), [Bit-Genie](https://github.com/Aryan1508/Bit-Genie), [Clover](https://github.com/lucametehau/CloverEngine/tree/master), [Pawn](https://github.com/ruicoelhopedro/pawn) 
- Strong, open-source data such as [Ethereal Tuning Data Dump](https://www.talkchess.com/forum3/viewtopic.php?f=7&t=75350)