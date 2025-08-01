/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef BOARD_H
#define BOARD_H

#include "MagicMoves.hpp"
#include "defs.h"
#include <cmath>
#include <iostream>
#include <string>

/** Bitboard representations of various chess board elements */
extern U64 square_bb[BOARD_SIZE];
extern U64 file_bb[BOARD_SIZE];
extern U64 rank_bb[BOARD_SIZE];
extern U64 pawn_attacks[PLAYER_SIZE][BOARD_SIZE];
extern U64 pawn_push[PLAYER_SIZE][BOARD_SIZE];
extern U64 pawn_dbl_push[PLAYER_SIZE][BOARD_SIZE];
extern U64 between_dia[BOARD_SIZE][BOARD_SIZE];
extern U64 between_hor[BOARD_SIZE][BOARD_SIZE];
extern U64 between[BOARD_SIZE][BOARD_SIZE];
extern U64 coplanar[BOARD_SIZE][BOARD_SIZE];
extern U64 adj_files[BOARD_SIZE];
extern U64 adj_ranks[BOARD_SIZE];
extern U64 in_front[PLAYER_SIZE][BOARD_SIZE];
extern U64 king_net_bb[PLAYER_SIZE][BOARD_SIZE];
extern U64 outpost_area[PLAYER_SIZE];

extern const U64 KNIGHT_MOVES[BOARD_SIZE];
extern const U64 KING_MOVES[BOARD_SIZE];
extern U64 bishopMoves[BOARD_SIZE];
extern U64 rookMoves[BOARD_SIZE];

/** Bitboard constants for board geometry and piece movement */
constexpr U64 DARK_SQUARES = 0xAA55AA55AA55AA55ULL;
constexpr U64 LIGHT_SQUARES = 0x55AA55AA55AA55AAULL;

constexpr U64 RANK_1 = 0x00000000000000FFULL;
constexpr U64 RANK_2 = 0x000000000000FF00ULL;
constexpr U64 RANK_3 = 0x0000000000FF0000ULL;
constexpr U64 RANK_4 = 0x00000000FF000000ULL;
constexpr U64 RANK_5 = 0x000000FF00000000ULL;
constexpr U64 RANK_6 = 0x0000FF0000000000ULL;
constexpr U64 RANK_7 = 0x00FF000000000000ULL;
constexpr U64 RANK_8 = 0xFF00000000000000ULL;
constexpr U64 RANK_PROMOTION = RANK_1 | RANK_8;
constexpr U64 RANK_PAWN_START = RANK_2 | RANK_7;

constexpr U64 FILE_A = 0x8080808080808080ULL;
constexpr U64 FILE_B = 0x4040404040404040ULL;
constexpr U64 FILE_C = 0x2020202020202020ULL;
constexpr U64 FILE_D = 0x1010101010101010ULL;
constexpr U64 FILE_E = 0x0808080808080808ULL;
constexpr U64 FILE_F = 0x0404040404040404ULL;
constexpr U64 FILE_G = 0x0202020202020202ULL;
constexpr U64 FILE_H = 0x0101010101010101ULL;

constexpr U64 NOT_A_FILE = 0x7F7F7F7F7F7F7F7F;
constexpr U64 NOT_H_FILE = 0xFEFEFEFEFEFEFEFE;
constexpr U64 CENTER_FILES = NOT_A_FILE & NOT_H_FILE;
constexpr U64 RIGHTSIDE = 0x0F0F0F0F0F0F0F0F;
constexpr U64 LEFTSIDE = 0xF0F0F0F0F0F0F0F0;

/** Initialize bitboard lookup tables */
void bb_init();

/** Bitboard operations for chess squares */
inline U64 operator&(Square s, U64 u) {
    return square_bb[s] & u;
}

inline U64 operator|(Square s, U64 u) {
    return square_bb[s] & u;
}

inline U64 operator^(Square s, U64 u) {
    return square_bb[s] & u;
}

/** Bit manipulation utilities */
inline int pop_count(U64 bb) {
#if defined(_MSC_VER) && defined(_WIN64)
    return _mm_popcnt_u64(bb);
#elif defined(__GNUC__)
    return __builtin_popcountll(bb);
#else
    constexpr U64 m1 = 0x5555555555555555ull;
    constexpr U64 m2 = 0x3333333333333333ull;
    constexpr U64 m4 = 0x0f0f0f0f0f0f0f0full;
    constexpr U64 h1 = 0x0101010101010101ull;
    bb -= (bb >> 1) & m1;
    bb = (bb & m2) + ((bb >> 2) & m2);
    bb = (bb + (bb >> 4)) & m4;
    return static_cast<int>((bb * h1) >> 56);
#endif
}

inline Square get_lsb(U64 bb) {
    assert(bb);
#if defined(_MSC_VER)
    unsigned long index;
#if defined(_M_AMD64) || defined(__WIN64)
    _BitScanForward64(&index, bb);
    return static_cast<Square>(index);
#else
    if (_BitScanForward(&index, bb)) {
        return static_cast<Square>(index)
    }
    _BitScanForward(&index, bb >> 32);
    return static_cast<Square>(index);
#endif
#elif defined(__GNUC__)
    return static_cast<Square>(__builtin_ctzll(bb));
#else
    constexpr U64 DeBrujin = 0x03f79d71b4cb0a89;
    constexpr int DeBrujin_table[64] = {
        0, 1, 48, 2, 57, 49, 28, 3,
        61, 58, 50, 42, 38, 29, 17, 4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12, 5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19, 9, 13, 8, 7, 6};
    return static_cast<Square>(DeBrujin_table[((bb & -bb) * DeBrujin) >> 58]);
#endif
}

inline U64 get_lsb_bb(U64 bb) {
    assert(bb);
    return 1ULL << get_lsb(bb);
}

inline Square pop_lsb(U64 &bb) {
    U64 t = bb;
    bb &= (bb - 1);
    return get_lsb(t);
}

inline U64 pop_lsb_bb(U64 &bb) {
    assert(bb);
    U64 t = bb;
    bb &= (bb - 1);
    return t ^ bb;
}

inline File get_file(U64 bb) {
    assert(bb);
    return File(get_lsb(bb) % 8);
}

inline Rank get_rank(U64 bb) {
    assert(bb);
    return Rank(get_lsb(bb) / 8);
}

/** Directional shift operations for piece movement */
inline U64 shift_up(U64 bb, const Dir D) {
    return D == N    ? bb << 8
           : D == NE ? (bb & NOT_H_FILE) << 9
           : D == NW ? (bb & NOT_A_FILE) << 7
                     : 0;
}

inline U64 squares_of_color(Square s) {
    return s & LIGHT_SQUARES ? LIGHT_SQUARES : DARK_SQUARES;
}

inline U64 shift(U64 bb, const Dir D) {
    return D == N ? bb << 8 : bb >> 8;
}

inline U64 shift_e(U64 bb, const Dir D) {
    return D == NE ? (bb & NOT_H_FILE) << 7 : (bb & NOT_H_FILE) >> 9;
}

inline U64 shift_w(U64 bb, const Dir D) {
    return D == NW ? (bb & NOT_A_FILE) << 9 : (bb & NOT_A_FILE) >> 7;
}

inline U64 shift_ep(U64 bb, const Dir D) {
    return D == E ? (bb & NOT_H_FILE) >> 1 : (bb & NOT_A_FILE) << 1;
}

/** Bit manipulation helpers */
inline void clear_bit(U64 &bb, int dst) {
    bb &= ~(square_bb[dst]);
}

inline void set_bit(U64 &bb, int dst) {
    bb |= square_bb[dst];
}

inline void move_bit(U64 &bb, int src, int dst) {
    bb ^= square_bb[src] | square_bb[dst];
}

/** Bitboard fill algorithms for sliding piece attacks */
void print_bb(U64);

inline U64 north_fill(U64 gen) {
    gen |= (gen << 8);
    gen |= (gen << 16);
    gen |= (gen << 32);
    return gen;
}

inline U64 south_fill(U64 gen) {
    gen |= (gen >> 8);
    gen |= (gen >> 16);
    gen |= (gen >> 32);
    return gen;
}

inline U64 east_fill(U64 gen) {
    constexpr U64 pr0 = NOT_H_FILE;
    constexpr U64 pr1 = pr0 & (pr0 << 1);
    constexpr U64 pr2 = pr1 & (pr1 << 2);
    gen |= pr0 & (gen << 1);
    gen |= pr1 & (gen << 2);
    gen |= pr2 & (gen << 4);
    return gen;
}

inline U64 north_east_fill(U64 gen) {
    constexpr U64 pr0 = NOT_H_FILE;
    constexpr U64 pr1 = pr0 & (pr0 << 9);
    constexpr U64 pr2 = pr1 & (pr1 << 18);
    gen |= pr0 & (gen << 9);
    gen |= pr1 & (gen << 18);
    gen |= pr2 & (gen << 36);
    return gen;
}

inline U64 south_east_fill(U64 gen) {
    constexpr U64 pr0 = NOT_H_FILE;
    constexpr U64 pr1 = pr0 & (pr0 >> 7);
    constexpr U64 pr2 = pr1 & (pr1 >> 14);
    gen |= pr0 & (gen >> 7);
    gen |= pr1 & (gen >> 14);
    gen |= pr2 & (gen >> 28);
    return gen;
}

inline U64 west_fill(U64 gen) {
    constexpr U64 pr0 = NOT_A_FILE;
    constexpr U64 pr1 = pr0 & (pr0 >> 1);
    constexpr U64 pr2 = pr1 & (pr1 >> 2);
    gen |= pr0 & (gen >> 1);
    gen |= pr1 & (gen >> 2);
    gen |= pr2 & (gen >> 4);
    return gen;
}

inline U64 south_west_fill(U64 gen) {
    constexpr U64 pr0 = NOT_A_FILE;
    constexpr U64 pr1 = pr0 & (pr0 >> 9);
    constexpr U64 pr2 = pr1 & (pr1 >> 18);
    gen |= pr0 & (gen >> 9);
    gen |= pr1 & (gen >> 18);
    gen |= pr2 & (gen >> 36);
    return gen;
}

inline U64 north_west_fill(U64 gen) {
    constexpr U64 pr0 = NOT_A_FILE;
    constexpr U64 pr1 = pr0 & (pr0 << 7);
    constexpr U64 pr2 = pr1 & (pr1 << 14);
    gen |= pr0 & (gen << 7);
    gen |= pr1 & (gen << 14);
    gen |= pr2 & (gen << 28);
    return gen;
}

#endif
