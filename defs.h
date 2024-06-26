/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef DEFS_H
#define DEFS_H

#include <assert.h>
#include <stdint.h>
#include <string>
#include <sys/time.h>

#ifndef DEBUG
#define ASSERT(x)
#define D(x)
#else
#define ASSERT(x)                                  \
    if (!(x)) {                                    \
        std::cout << "Error: " << #x;              \
        std::cout << "Date: " << __DATE__ << '\n'; \
        std::cout << "Time: " << __TIME__ << '\n'; \
        std::cout << "File: " << __FILE__ << '\n'; \
        std::cout << "Line: " << __LINE__ << '\n'; \
        std::exit(0);                              \
    }
#define D(x) x
#endif

typedef unsigned long long U64;
#define C64(u) u##ULL;

constexpr int BOARD_SIZE = 64;
constexpr int PIECE_TYPES_SIZE = 6;
constexpr int PLAYER_SIZE = 2;
constexpr int PIECE_MAX = 10;
const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq";

constexpr int CASTLE_RIGHTS[BOARD_SIZE] = {
    14, 15, 15, 12, 15, 15, 15, 13,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    11, 15, 15, 3, 15, 15, 15, 7};

constexpr int GAMESTAGE_SIZE = 2;

enum GameStage {
    MIDDLEGAME,
    ENDGAME
};

enum Color {
    WHITE,
    BLACK
};

enum PieceType {
    PIECETYPE_PAWN,
    PIECETYPE_KNIGHT,
    PIECETYPE_BISHOP,
    PIECETYPE_ROOK,
    PIECETYPE_QUEEN,
    PIECETYPE_KING,
    PIECETYPE_NONE
};

enum Square : uint32_t {
    H1,
    G1,
    F1,
    E1,
    D1,
    C1,
    B1,
    A1,
    H2,
    G2,
    F2,
    E2,
    D2,
    C2,
    B2,
    A2,
    H3,
    G3,
    F3,
    E3,
    D3,
    C3,
    B3,
    A3,
    H4,
    G4,
    F4,
    E4,
    D4,
    C4,
    B4,
    A4,
    H5,
    G5,
    F5,
    E5,
    D5,
    C5,
    B5,
    A5,
    H6,
    G6,
    F6,
    E6,
    D6,
    C6,
    B6,
    A6,
    H7,
    G7,
    F7,
    E7,
    D7,
    C7,
    B7,
    A7,
    H8,
    G8,
    F8,
    E8,
    D8,
    C8,
    B8,
    A8,
    no_sq = 64,
    first_sq = 0,
    last_sq = 63
};

enum File {
    file_a,
    file_b,
    file_c,
    file_d,
    file_e,
    file_f,
    file_g,
    file_h
};

inline File file(Square s) {
    return File(s & 0x7);
}

enum Rank {
    rank_1,
    rank_2,
    rank_3,
    rank_4,
    rank_5,
    rank_6,
    rank_7,
    rank_8
};

inline Rank rank(Square s) {
    return Rank(s >> 3);
}

enum CASTLINGRIGHTS {
    WHITE_KINGSIDE_CASTLE = 1,
    WHITE_QUEENSIDE_CASTLE = 2,
    BLACK_KINGSIDE_CASTLE = 4,
    BLACK_QUEENSIDE_CASTLE = 8
};

constexpr int SCORE[PIECE_TYPES_SIZE][PIECE_TYPES_SIZE] = {
    {26, 30, 31, 33, 36, 0},
    {20, 25, 27, 29, 35, 0},
    {19, 21, 24, 28, 34, 0},
    {16, 17, 18, 23, 32, 0},
    {12, 13, 14, 15, 22, 0},
    {7, 8, 9, 10, 11, 0}};

enum Dir {
    N = 8,
    S = -8,
    E = -1,
    W = 1,
    NE = 7,
    NW = 9,
    SE = -9,
    SW = -7
};

const std::string SQUARE_TO_STRING[64] = {
    "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
    "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
    "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
    "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
    "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
    "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
    "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
    "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8"};

enum Prop : uint32_t {
    PROP_QUIET,
    PROP_ATTACK,
    PROP_DOUBLE_PUSH,
    PROP_KINGSIDE_CASTLE,
    PROP_QUEENSIDE_CASTLE,
    PROP_QUEEN_PROMOTION,
    PROP_KNIGHT_PROMOTION,
    PROP_ROOK_PROMOTION,
    PROP_BISHOP_PROMOTION,
    PROP_EN_PASSANT
};

const int NEG_INF = -50000;
const int POS_INF = 50000;
const int MAX_PLY = 50;
const int MAX_GAME_MOVES = 1024;

inline Square &operator++(Square &s) { return s = static_cast<Square>(static_cast<int>(s) + 1); }
inline PieceType &operator++(PieceType &p) { return p = static_cast<PieceType>(static_cast<int>(p) + 1); }
inline File &operator++(File &f) { return f = static_cast<File>(static_cast<int>(f) + 1); }
inline Color operator!(const Color c) { return static_cast<Color>(!static_cast<bool>(c)); }
inline Square operator+(const Square s, const int i) { return static_cast<Square>(static_cast<int>(s) + i); }
inline Square operator-(const Square s, const int i) { return static_cast<Square>(static_cast<int>(s) - i); }
inline Square operator-(const Square s1, const Square s2) { return static_cast<Square>(static_cast<int>(s1) - static_cast<int>(s2)); }

inline int abs(int x) { return x >= 0 ? x : -x; }
inline int clamp(int x, int a, int b) { return std::max(a, std::min(x, b)); }
inline int manhattanDistance(const Square sq1, const Square sq2) { return abs(file(sq1) - file(sq2)) + abs(rank(sq1) - rank(sq2)); }
inline int chebyshevDistance(const Square sq1, const Square sq2) { return std::max(abs(file(sq1) - file(sq2)), abs(rank(sq1) - rank(sq2))); }

inline std::string to_string(File f) {
    return std::string(1, char('h' - f));
}

inline std::string to_string(Rank r) {
    return std::string(1, char('1' + r));
}

inline std::string to_string(Prop p) {
    return p == PROP_QUEEN_PROMOTION    ? "q"
           : p == PROP_KNIGHT_PROMOTION ? "k"
           : p == PROP_ROOK_PROMOTION   ? "r"
           : p == PROP_BISHOP_PROMOTION ? "b"
                                        : "";
}

inline std::string to_string(PieceType p) {
    return p == PIECETYPE_PAWN     ? "P"
           : p == PIECETYPE_KNIGHT ? "N"
           : p == PIECETYPE_BISHOP ? "B"
           : p == PIECETYPE_ROOK   ? "R"
           : p == PIECETYPE_QUEEN  ? "Q"
                                   : "K";
}

#endif
